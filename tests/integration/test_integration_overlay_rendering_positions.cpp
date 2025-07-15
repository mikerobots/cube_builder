#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
extern "C" {
#include <glad/glad.h>
}

#include <memory>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>

#include "rendering/OpenGLRenderer.h"
#include "rendering/RenderConfig.h"
#include "rendering/RenderTypes.h"
#include "camera/OrbitCamera.h"
#include "visual_feedback/FeedbackRenderer.h"
#include "visual_feedback/OverlayRenderer.h"
#include "voxel_data/VoxelDataManager.h"
#include "math/BoundingBox.h"
#include "math/Vector3f.h"
#include "events/EventDispatcher.h"
#include "logging/Logger.h"

namespace VoxelEditor {
namespace Integration {
namespace Tests {

using namespace Math;
using namespace Rendering;
using namespace Camera;
using namespace VisualFeedback;
using namespace VoxelData;

class OverlayRenderingPositionTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<OpenGLRenderer> renderer;
    std::unique_ptr<OverlayRenderer> overlayRenderer;
    std::unique_ptr<OrbitCamera> camera;
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
    
    int width = 800;
    int height = 600;
    
    void SetUp() override {
        // Initialize GLFW
        if (!glfwInit()) {
            GTEST_SKIP() << "Failed to initialize GLFW";
        }
        
        // Configure for OpenGL 3.3 Core Profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        // Create window
        window = glfwCreateWindow(width, height, "Overlay Position Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            GTEST_SKIP() << "Failed to create GLFW window";
        }
        
        // Make context current
        glfwMakeContextCurrent(window);
        
        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glfwDestroyWindow(window);
            glfwTerminate();
            GTEST_SKIP() << "Failed to initialize GLAD";
        }
        
        // Set viewport
        glViewport(0, 0, width, height);
        
        // Create components
        renderer = std::make_unique<OpenGLRenderer>();
        RenderConfig config;
        config.windowWidth = width;
        config.windowHeight = height;
        
        if (!renderer->initializeContext(config)) {
            GTEST_SKIP() << "Failed to initialize OpenGL renderer";
        }
        
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
        overlayRenderer = std::make_unique<OverlayRenderer>();
        camera = std::make_unique<OrbitCamera>(eventDispatcher.get());
        
        // Set camera to top view
        camera->setViewPreset(ViewPreset::TOP);
        // Don't override the default distance which is properly set for visibility
        
        // For top view looking down Y axis, we need to set up vector along Z
        camera->setUp(Math::WorldCoordinates(Math::Vector3f(0, 0, -1)));
        
        // For top view, use orthographic projection for better visibility
        camera->setProjectionType(ProjectionType::ORTHOGRAPHIC);
        camera->setOrthographicSize(10.0f); // View 10m area
        
        // Force camera matrices to update
        auto viewMatrix = camera->getViewMatrix();
        auto projMatrix = camera->getProjectionMatrix();
        
        // Debug output camera info
        auto pos = camera->getPosition();
        auto target = camera->getTarget();
        auto up = camera->getUp();
        std::cerr << "Camera setup - Pos: (" << pos.x() << "," << pos.y() << "," << pos.z() 
                  << ") Target: (" << target.x() << "," << target.y() << "," << target.z()
                  << ") Up: (" << up.x() << "," << up.y() << "," << up.z() << ")" << std::endl;
        
        // Clear any OpenGL errors from setup
        while (glGetError() != GL_NO_ERROR) {}
    }
    
    void TearDown() override {
        overlayRenderer.reset();
        camera.reset();
        voxelManager.reset();
        eventDispatcher.reset();
        renderer.reset();
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    void checkGLError(const std::string& operation) {
        GLenum error = glGetError();
        EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error " << error << " during " << operation;
    }
    
    // Capture framebuffer to vector of RGB pixels
    std::vector<uint8_t> captureFramebuffer() {
        std::vector<uint8_t> pixels(width * height * 3);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        return pixels;
    }
    
    // Count pixels matching a color criterion
    struct PixelCounts {
        int green = 0;      // Grid lines
        int gray = 0;       // Grid lines  
        int yellow = 0;     // Highlights
        int nonBlack = 0;   // Any rendered content
    };
    
    PixelCounts analyzePixels(const std::vector<uint8_t>& pixels) {
        PixelCounts counts;
        
        for (size_t i = 0; i < pixels.size(); i += 3) {
            uint8_t r = pixels[i];
            uint8_t g = pixels[i + 1];
            uint8_t b = pixels[i + 2];
            
            if (r > 10 || g > 10 || b > 10) {
                counts.nonBlack++;
            }
            
            // Green pixels (for outlines)
            if (r < 100 && g > 150 && b < 100) {
                counts.green++;
            }
            
            // Gray pixels (for grid lines) 
            if (std::abs(r - g) < 30 && std::abs(g - b) < 30 && r > 50) {
                counts.gray++;
            }
            
            // Yellow pixels (for highlights)
            if (r > 200 && g > 200 && b < 100) {
                counts.yellow++;
            }
        }
        
        return counts;
    }
    
    void renderFrame() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        
        checkGLError("clear and setup");
    }
};

// Test ground plane grid rendering in top view
TEST_F(OverlayRenderingPositionTest, GroundPlaneGridTopView) {
    // Set up proper rendering state first
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);
    
    // Begin overlay frame
    overlayRenderer->beginFrame(width, height);
    
    // Render ground plane grid
    Vector3f center(0, 0, 0);
    float extent = 2.5f;
    Vector3f cursorPos(0, 0, 0);
    bool enableDynamicOpacity = false;
    
    EXPECT_NO_THROW(overlayRenderer->renderGroundPlaneGrid(center, extent, cursorPos, enableDynamicOpacity, *camera));
    checkGLError("renderGroundPlaneGrid");
    
    // End overlay frame - note: lines already rendered by flushLineBatch in renderGroundPlaneGrid
    overlayRenderer->endFrame();
    // Don't call renderFrame() as it clears the framebuffer after overlay rendering
    
    // Capture and analyze pixels
    auto pixels = captureFramebuffer();
    auto counts = analyzePixels(pixels);
    
    // Debug: Save screenshot for analysis
    std::ofstream debugFile("debug_grid_top_view.ppm");
    if (debugFile.is_open()) {
        debugFile << "P3\n" << width << " " << height << "\n255\n";
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 3;
                debugFile << (int)pixels[idx] << " " << (int)pixels[idx+1] << " " << (int)pixels[idx+2] << "\n";
            }
        }
        debugFile.close();
        std::cerr << "Debug screenshot saved to debug_grid_top_view.ppm" << std::endl;
    } else {
        std::cerr << "Failed to create debug file" << std::endl;
    }
    
    // Basic validation - we should have some rendered content
    EXPECT_GT(counts.nonBlack, 0) << "No pixels rendered - overlay system may not be working. Debug image saved to debug_grid_top_view.ppm";
    
    // If we have content, verify it looks like a grid
    if (counts.nonBlack > 0) {
        EXPECT_GT(counts.gray, 50) << "Expected gray grid lines in top view";
    }
}

// Test outline box rendering at specific positions
TEST_F(OverlayRenderingPositionTest, OutlineBoxPositions) {
    struct TestCase {
        Vector3f worldPos;
        std::string description;
    };
    
    TestCase testCases[] = {
        {Vector3f(0, 0, 0), "Origin"},
        {Vector3f(1.0f, 0, 0), "1m right"},
        {Vector3f(0, 0, 1.0f), "1m forward"}
    };
    
    for (const auto& test : testCases) {
        // Clear framebuffer for each test case
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Begin overlay frame
        overlayRenderer->beginFrame(width, height);
        
        // Render outline box at position
        BoundingBox box(test.worldPos, test.worldPos + Vector3f(0.32f, 0.32f, 0.32f));
        
        // Use the overlay renderer's bounding box rendering capability
        std::vector<BoundingBox> boxes = {box};
        EXPECT_NO_THROW(overlayRenderer->renderBoundingBoxes(boxes, Rendering::Color::Green(), *camera));
        checkGLError("renderBoundingBoxes " + test.description);
        
        // End overlay frame - note: lines already rendered by flushLineBatch in renderBoundingBoxes
        overlayRenderer->endFrame();
        // Don't call renderFrame() as it clears the framebuffer after overlay rendering
        
        // Capture and analyze pixels
        auto pixels = captureFramebuffer();
        auto counts = analyzePixels(pixels);
        
        // Verify we have some rendered content
        EXPECT_GT(counts.nonBlack, 0) << "No content rendered for " << test.description;
        
        // If we have content, check for green outline pixels
        if (counts.nonBlack > 0) {
            EXPECT_GT(counts.green, 0) << "Expected green outline pixels for " << test.description;
        }
    }
}

// Test functional rendering without visual validation  
TEST_F(OverlayRenderingPositionTest, FunctionalRenderingTest) {
    // This test focuses on ensuring the rendering pipeline works without exceptions
    
    // Begin overlay frame
    EXPECT_NO_THROW(overlayRenderer->beginFrame(width, height));
    checkGLError("beginFrame");
    
    // Test various overlay rendering calls
    Vector3f center(0, 0, 0);
    float extent = 2.5f;
    Vector3f cursorPos(0, 0, 0);
    
    EXPECT_NO_THROW(overlayRenderer->renderGroundPlaneGrid(center, extent, cursorPos, false, *camera));
    checkGLError("renderGroundPlaneGrid");
    
    BoundingBox box(Vector3f(-1, 0, -1), Vector3f(1, 1, 1));
    std::vector<BoundingBox> boxes = {box};
    EXPECT_NO_THROW(overlayRenderer->renderBoundingBoxes(boxes, Rendering::Color::Green(), *camera));
    checkGLError("renderBoundingBoxes");
    
    // End overlay frame
    EXPECT_NO_THROW(overlayRenderer->endFrame());
    checkGLError("endFrame");
    
    // Render frame
    renderFrame();
    checkGLError("renderFrame");
    
    // The test passes if no exceptions were thrown and no OpenGL errors occurred
}

} // namespace Tests
} // namespace Integration
} // namespace VoxelEditor