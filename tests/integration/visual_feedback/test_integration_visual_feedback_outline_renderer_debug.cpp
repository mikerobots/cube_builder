#include <gtest/gtest.h>

// Include OpenGL headers
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>
#include <memory>
#include <cstdlib>
#include <iostream>
#include "visual_feedback/OutlineRenderer.h"
#include "visual_feedback/FeedbackTypes.h"
#include "camera/QuaternionOrbitCamera.h"
#include "math/Vector3i.h"
#include "math/BoundingBox.h"
#include "voxel_data/VoxelTypes.h"
#include "logging/Logger.h"

namespace VoxelEditor {

class OutlineRendererDebugTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<VisualFeedback::OutlineRenderer> renderer;
    std::unique_ptr<Camera::QuaternionOrbitCamera> camera;
    
    void SetUp() override {
        // Skip in CI environment
        if (std::getenv("CI") != nullptr) {
            GTEST_SKIP() << "Skipping OpenGL tests in CI environment";
        }
        
        // Setup logging
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::ConsoleOutput>("TestLog"));
        
        // Initialize GLFW
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        
        // Configure OpenGL context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        // Create window
        window = glfwCreateWindow(800, 600, "OutlineRenderer Debug Test", nullptr, nullptr);
        ASSERT_NE(window, nullptr) << "Failed to create GLFW window";
        
        // Make context current
        glfwMakeContextCurrent(window);
        
        // Load OpenGL functions
#ifndef __APPLE__
        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) << "Failed to initialize GLAD";
#endif
        
        // Clear any existing GL errors
        while (glGetError() != GL_NO_ERROR) {}
        
        // Log OpenGL info
        std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
        std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        
        // Check for required extensions
        GLint numExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        std::cout << "Number of extensions: " << numExtensions << std::endl;
        
        // Create renderer and camera
        renderer = std::make_unique<VisualFeedback::OutlineRenderer>();
        camera = std::make_unique<Camera::QuaternionOrbitCamera>();
        camera->setViewPreset(Camera::ViewPreset::ISOMETRIC);
    }
    
    void TearDown() override {
        if (window) {
            renderer.reset();
            camera.reset();
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }
    
    void checkGLError(const std::string& operation) {
        GLenum error = glGetError();
        EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error " << error << " during " << operation;
    }
    
    void logOpenGLState() {
        GLint vao, vbo, ebo, program;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ebo);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        
        std::cout << "OpenGL State:" << std::endl;
        std::cout << "  VAO: " << vao << std::endl;
        std::cout << "  VBO: " << vbo << std::endl;
        std::cout << "  EBO: " << ebo << std::endl;
        std::cout << "  Program: " << program << std::endl;
        
        // Check viewport
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        std::cout << "  Viewport: " << viewport[0] << ", " << viewport[1] 
                  << ", " << viewport[2] << ", " << viewport[3] << std::endl;
    }
};

TEST_F(OutlineRendererDebugTest, DetailedRenderSingleVoxelOutline) {
    std::cout << "\n=== Testing Single Voxel Outline ===" << std::endl;
    
    // Set viewport
    glViewport(0, 0, 800, 600);
    checkGLError("glViewport");
    
    // Clear to ensure clean state
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    checkGLError("glClear");
    
    std::cout << "Initial OpenGL state:" << std::endl;
    logOpenGLState();
    
    // Create a single voxel outline
    renderer->beginBatch();
    
    Math::Vector3i position(0, 0, 0);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    VisualFeedback::OutlineStyle style = VisualFeedback::OutlineStyle::VoxelPreview();
    
    std::cout << "Adding voxel outline at position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
    renderer->renderVoxelOutline(position, resolution, style);
    
    renderer->endBatch();
    
    std::cout << "\nBefore renderBatch:" << std::endl;
    logOpenGLState();
    
    // Render the batch
    std::cout << "\nCalling renderBatch..." << std::endl;
    EXPECT_NO_THROW(renderer->renderBatch(*camera));
    
    std::cout << "\nAfter renderBatch:" << std::endl;
    logOpenGLState();
    
    checkGLError("renderBatch single voxel");
    
    renderer->clearBatch();
}

TEST_F(OutlineRendererDebugTest, MinimalLineTest) {
    std::cout << "\n=== Testing Minimal Line ===" << std::endl;
    
    // Set viewport
    glViewport(0, 0, 800, 600);
    
    // Create the simplest possible line
    renderer->beginBatch();
    
    std::vector<Math::Vector3f> points = {
        Math::Vector3f(0, 0, 0),
        Math::Vector3f(1, 0, 0)
    };
    
    VisualFeedback::OutlineStyle style = VisualFeedback::OutlineStyle::VoxelPreview();
    renderer->renderCustomOutline(points, style, false); // not closed
    
    renderer->endBatch();
    
    std::cout << "Rendering minimal line..." << std::endl;
    EXPECT_NO_THROW(renderer->renderBatch(*camera));
    checkGLError("renderBatch minimal line");
    
    renderer->clearBatch();
}

} // namespace VoxelEditor