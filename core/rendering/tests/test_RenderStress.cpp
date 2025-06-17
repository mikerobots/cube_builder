#include <gtest/gtest.h>
#include "../RenderEngine.h"
#include "../OpenGLRenderer.h"
#include "../ShaderManager.h"
#include "../GroundPlaneGrid.h"
#include "../../voxel_data/VoxelDataManager.h"
#include "../../voxel_data/VoxelTypes.h"
#include "../../visual_feedback/include/visual_feedback/FeedbackRenderer.h"
#include "../../visual_feedback/include/visual_feedback/HighlightRenderer.h"
#include "../../../foundation/math/Vector3f.h"
#include "../../../foundation/math/Matrix4f.h"
#include "../../../foundation/logging/Logger.h"
#include <chrono>
#include <random>
#include <thread>

// Include OpenGL headers
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#include <GLFW/glfw3.h>
#else
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

using namespace VoxelEditor;
using namespace VoxelEditor::Rendering;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Logging;

class RenderStressTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<RenderEngine> renderEngine;
    std::unique_ptr<VoxelDataManager> voxelData;
    std::unique_ptr<FeedbackRenderer> feedbackRenderer;
    
    void SetUp() override {
        // Initialize GLFW
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        
        // Create window with OpenGL context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for tests
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        window = glfwCreateWindow(1920, 1080, "Stress Test", nullptr, nullptr);
        ASSERT_NE(window, nullptr) << "Failed to create GLFW window";
        
        glfwMakeContextCurrent(window);
        
#ifndef __APPLE__
        // Load OpenGL functions on non-Mac platforms
        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
            << "Failed to initialize OpenGL function pointers";
#endif
        
        // Initialize render engine
        renderEngine = std::make_unique<RenderEngine>();
        RenderConfig config;
        ASSERT_TRUE(renderEngine->initialize(config)) << "Failed to initialize render engine";
        
        // Initialize voxel data manager
        voxelData = std::make_unique<VoxelDataManager>();
        voxelData->resizeWorkspace(Vector3f(8.0f, 8.0f, 8.0f)); // Max workspace
        
        // Initialize feedback renderer
        feedbackRenderer = std::make_unique<FeedbackRenderer>(
            renderEngine->getShaderManager(),
            renderEngine->getOpenGLRenderer()
        );
        ASSERT_TRUE(feedbackRenderer->initialize());
    }
    
    void TearDown() override {
        feedbackRenderer.reset();
        renderEngine.reset();
        voxelData.reset();
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
};

// Test maximum voxel count before performance degradation
TEST_F(RenderStressTest, FindMaximumVoxelCount) {
    const double targetFrameTime = 33.3; // 30 FPS minimum
    int maxVoxels = 0;
    
    Logger& logger = Logger::getInstance();
    logger.info("Finding maximum voxel count for 30 FPS...");
    
    // Test increasing voxel counts
    std::vector<int> testCounts = {
        1000, 5000, 10000, 20000, 30000, 40000, 50000, 75000, 100000
    };
    
    Camera::Camera camera;
    camera.setPosition(Vector3f(20.0f, 20.0f, 20.0f));
    camera.setTarget(Vector3f(0.0f, 0.0f, 0.0f));
    camera.setAspectRatio(16.0f/9.0f);
    
    for (int count : testCounts) {
        // Clear previous voxels
        voxelData->clear();
        
        // Add voxels in a distributed pattern
        std::mt19937 rng(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<int> dist(-40, 40); // 8m workspace = -4m to +4m
        
        for (int i = 0; i < count; ++i) {
            Vector3i pos(dist(rng), std::abs(dist(rng)) / 2, dist(rng));
            voxelData->setVoxel(pos, VoxelResolution::Size_8cm, true);
        }
        
        // Measure frame time
        std::vector<double> frameTimes;
        for (int frame = 0; frame < 30; ++frame) {
            auto start = std::chrono::high_resolution_clock::now();
            
            renderEngine->beginFrame();
            renderEngine->setCamera(camera);
            
            VoxelGrid* grid = voxelData->getGrid(VoxelResolution::Size_8cm);
            if (grid) {
                RenderSettings settings;
                settings.renderMode = RenderMode::Solid;
                renderEngine->renderVoxels(*grid, VoxelResolution::Size_8cm, settings);
            }
            
            renderEngine->endFrame();
            glfwSwapBuffers(window);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            frameTimes.push_back(duration.count() / 1000.0);
        }
        
        double avgTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) 
                        / frameTimes.size();
        double fps = 1000.0 / avgTime;
        
        logger.infof("  %d voxels: %.2fms (%.1f FPS)", 
                   count, avgTime, fps);
        
        if (avgTime < targetFrameTime) {
            maxVoxels = count;
        } else {
            break; // Performance threshold exceeded
        }
    }
    
    EXPECT_GE(maxVoxels, 10000) 
        << "Should handle at least 10,000 voxels at 30 FPS";
    
    logger.infof("Maximum voxels at 30 FPS: %d", maxVoxels);
}

// Test mixed resolution voxel rendering
TEST_F(RenderStressTest, MixedResolutionPerformance) {
    Logger& logger = Logger::getInstance();
    logger.info("\nTesting mixed resolution voxel rendering...");
    
    // Add voxels of different sizes
    std::vector<std::pair<VoxelResolution, int>> resolutions = {
        {VoxelResolution::Size_1cm, 100},
        {VoxelResolution::Size_4cm, 500},
        {VoxelResolution::Size_8cm, 1000},
        {VoxelResolution::Size_16cm, 500},
        {VoxelResolution::Size_32cm, 200}
    };
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-20, 20);
    
    for (const auto& [res, count] : resolutions) {
        voxelData->setActiveResolution(res);
        for (int i = 0; i < count; ++i) {
            Vector3i pos(dist(rng), std::abs(dist(rng)) / 4, dist(rng));
            voxelData->setVoxel(pos, res, true);
        }
    }
    
    // Measure performance
    Camera::Camera camera;
    camera.setPosition(Vector3f(15.0f, 15.0f, 15.0f));
    camera.setTarget(Vector3f(0.0f, 0.0f, 0.0f));
    camera.setAspectRatio(16.0f/9.0f);
    
    std::vector<double> frameTimes;
    for (int i = 0; i < 50; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        renderEngine->beginFrame();
        renderEngine->setCamera(camera);
        
        // Render each resolution
        for (const auto& [res, _] : resolutions) {
            VoxelGrid* grid = voxelData->getGrid(res);
            if (grid && grid->getVoxelCount() > 0) {
                RenderSettings settings;
                settings.renderMode = RenderMode::Solid;
                renderEngine->renderVoxels(*grid, res, settings);
            }
        }
        
        renderEngine->endFrame();
        glfwSwapBuffers(window);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        frameTimes.push_back(duration.count() / 1000.0);
    }
    
    double avgTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) 
                    / frameTimes.size();
    
    logger.infof("Mixed resolution rendering: %.2fms (%.1f FPS)",
               avgTime, 1000.0 / avgTime);
    
    EXPECT_LT(avgTime, 33.3) << "Mixed resolution rendering should maintain 30+ FPS";
}

// Test complex scene with all visual elements
TEST_F(RenderStressTest, ComplexSceneRendering) {
    Logger& logger = Logger::getInstance();
    logger.info("\nTesting complex scene rendering...");
    
    // Add voxels
    const int voxelCount = 5000;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-30, 30);
    
    voxelData->setActiveResolution(VoxelResolution::Size_8cm);
    for (int i = 0; i < voxelCount; ++i) {
        Vector3i pos(dist(rng), std::abs(dist(rng)) / 3, dist(rng));
        voxelData->setVoxel(pos, VoxelResolution::Size_8cm, true);
    }
    
    // Set up camera
    Camera::Camera camera;
    camera.setPosition(Vector3f(10.0f, 10.0f, 10.0f));
    camera.setTarget(Vector3f(0.0f, 0.0f, 0.0f));
    camera.setAspectRatio(16.0f/9.0f);
    
    // Add visual feedback elements
    Face highlightFace;
    highlightFace.center = Vector3f(0.0f, 0.0f, 0.0f);
    highlightFace.normal = Vector3f(0.0f, 1.0f, 0.0f);
    highlightFace.size = 0.08f;
    
    Vector3i previewPos(5, 0, 5);
    
    // Measure complex scene performance
    std::vector<double> frameTimes;
    for (int i = 0; i < 100; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        renderEngine->beginFrame();
        renderEngine->setCamera(camera);
        
        // Render ground plane
        renderEngine->setGroundPlaneGridVisible(true);
        renderEngine->renderGroundPlaneGrid(Vector3f(0.0f, 0.0f, 0.0f));
        
        // Render voxels
        VoxelGrid* grid = voxelData->getGrid(VoxelResolution::Size_8cm);
        if (grid) {
            RenderSettings settings;
            settings.renderMode = RenderMode::Solid;
            renderEngine->renderVoxels(*grid, VoxelResolution::Size_8cm, settings);
        }
        
        // Render feedback
        feedbackRenderer->renderFaceHighlight(highlightFace);
        feedbackRenderer->renderVoxelPreview(previewPos, VoxelResolution::Size_8cm);
        
        renderEngine->endFrame();
        glfwSwapBuffers(window);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        frameTimes.push_back(duration.count() / 1000.0);
    }
    
    double avgTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) 
                    / frameTimes.size();
    const RenderStats& stats = renderEngine->getRenderStats();
    
    logger.info("Complex scene performance:");
    logger.infof("  Frame time: %.2fms (%.1f FPS)", 
               avgTime, 1000.0 / avgTime);
    logger.infof("  Draw calls: %d", stats.drawCalls);
    logger.infof("  Vertices: %d", stats.vertices);
    logger.infof("  Triangles: %d", stats.triangles);
    
    EXPECT_LT(avgTime, 20.0) << "Complex scene should render under 20ms";
}

// Test rapid scene updates
TEST_F(RenderStressTest, RapidSceneUpdates) {
    Logger& logger = Logger::getInstance();
    logger.info("\nTesting rapid scene updates...");
    
    Camera::Camera camera;
    camera.setPosition(Vector3f(10.0f, 10.0f, 10.0f));
    camera.setTarget(Vector3f(0.0f, 0.0f, 0.0f));
    camera.setAspectRatio(16.0f/9.0f);
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-20, 20);
    
    // Simulate rapid voxel placement/removal
    std::vector<double> updateTimes;
    voxelData->setActiveResolution(VoxelResolution::Size_8cm);
    
    for (int frame = 0; frame < 100; ++frame) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Add/remove voxels each frame
        for (int i = 0; i < 10; ++i) {
            Vector3i pos(dist(rng), std::abs(dist(rng)) / 4, dist(rng));
            bool add = (frame + i) % 2 == 0;
            voxelData->setVoxel(pos, VoxelResolution::Size_8cm, add);
        }
        
        // Render
        renderEngine->beginFrame();
        renderEngine->setCamera(camera);
        
        VoxelGrid* grid = voxelData->getGrid(VoxelResolution::Size_8cm);
        if (grid) {
            RenderSettings settings;
            settings.renderMode = RenderMode::Solid;
            renderEngine->renderVoxels(*grid, VoxelResolution::Size_8cm, settings);
        }
        
        renderEngine->endFrame();
        glfwSwapBuffers(window);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        updateTimes.push_back(duration.count() / 1000.0);
    }
    
    double avgTime = std::accumulate(updateTimes.begin(), updateTimes.end(), 0.0) 
                    / updateTimes.size();
    double maxTime = *std::max_element(updateTimes.begin(), updateTimes.end());
    
    logger.info("Rapid update performance:");
    logger.infof("  Average: %.2fms", avgTime);
    logger.infof("  Max spike: %.2fms", maxTime);
    
    EXPECT_LT(avgTime, 20.0) << "Updates should average under 20ms";
    EXPECT_LT(maxTime, 50.0) << "Update spikes should stay under 50ms";
}

// Test memory usage under stress
TEST_F(RenderStressTest, MemoryUsageUnderStress) {
    Logger& logger = Logger::getInstance();
    logger.info("\nTesting memory usage under stress...");
    
    // Note: This is a simplified test. Real memory profiling would require
    // platform-specific tools or integration with memory tracking systems.
    
    const int iterations = 10;
    const int voxelsPerIteration = 5000;
    
    for (int iter = 0; iter < iterations; ++iter) {
        // Add voxels
        std::mt19937 rng(iter);
        std::uniform_int_distribution<int> dist(-40, 40);
        
        for (int i = 0; i < voxelsPerIteration; ++i) {
            Vector3i pos(dist(rng), std::abs(dist(rng)) / 2, dist(rng));
            voxelData->setVoxel(pos, VoxelResolution::Size_8cm, true);
        }
        
        // Render a frame
        Camera::Camera camera;
        camera.setPosition(Vector3f(20.0f, 20.0f, 20.0f));
        camera.setTarget(Vector3f(0.0f, 0.0f, 0.0f));
        camera.setAspectRatio(16.0f/9.0f);
        
        renderEngine->beginFrame();
        renderEngine->setCamera(camera);
        
        VoxelGrid* grid = voxelData->getGrid(VoxelResolution::Size_8cm);
        if (grid) {
            RenderSettings settings;
            settings.renderMode = RenderMode::Solid;
            renderEngine->renderVoxels(*grid, VoxelResolution::Size_8cm, settings);
        }
        
        renderEngine->endFrame();
        glfwSwapBuffers(window);
        
        // Clear every other iteration to test allocation/deallocation
        if (iter % 2 == 1) {
            voxelData->clear();
        }
    }
    
    // If we get here without crashing or running out of memory, test passes
    SUCCEED() << "Memory stress test completed without issues";
}