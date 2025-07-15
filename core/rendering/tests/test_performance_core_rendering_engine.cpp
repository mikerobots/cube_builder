#include <gtest/gtest.h>

// Include OpenGL headers (must come before GLFW and other includes)
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <glad/glad.h>
#endif

#include "../RenderEngine.h"
#include "../OpenGLRenderer.h"
#include "../ShaderManager.h"
#include "../RenderStats.h"
#include "../../voxel_data/VoxelDataManager.h"
#include "../../voxel_data/VoxelTypes.h"
#include "../../camera/OrbitCamera.h"
#include "../../../foundation/math/Vector3f.h"
#include "../../../foundation/math/Matrix4f.h"
#include "../../../foundation/logging/Logger.h"
#include <chrono>
#include <vector>
#include <numeric>

#ifdef HAVE_GLFW
#include <GLFW/glfw3.h>
#endif

using namespace VoxelEditor;
using namespace VoxelEditor::Rendering;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Logging;

class RenderPerformanceTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<RenderEngine> renderEngine;
    std::unique_ptr<VoxelDataManager> voxelData;
    
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
        
        window = glfwCreateWindow(1920, 1080, "Performance Test", nullptr, nullptr);
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
        voxelData->setActiveResolution(VoxelResolution::Size_8cm);
        voxelData->resizeWorkspace(Vector3f(5.0f, 5.0f, 5.0f));
    }
    
    void TearDown() override {
        renderEngine.reset();
        voxelData.reset();
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    // Helper to measure frame time
    double measureFrameTime(int voxelCount) {
        // Place voxels in a grid pattern
        int gridSize = static_cast<int>(std::cbrt(voxelCount));
        int placed = 0;
        
        for (int x = 0; x < gridSize && placed < voxelCount; ++x) {
            for (int y = 0; y < gridSize && placed < voxelCount; ++y) {
                for (int z = 0; z < gridSize && placed < voxelCount; ++z) {
                    voxelData->setVoxel(Vector3i(x * 2, y * 2, z * 2), 
                                       VoxelResolution::Size_8cm, true);
                    placed++;
                }
            }
        }
        
        // Set up camera
        Camera::OrbitCamera camera;
        camera.setPosition(Math::WorldCoordinates(Vector3f(10.0f, 10.0f, 10.0f)));
        camera.setTarget(Math::WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)));
        camera.setAspectRatio(16.0f/9.0f);
        
        // Warm up - render a few frames
        for (int i = 0; i < 10; ++i) {
            renderEngine->beginFrame();
            renderEngine->setCamera(camera);
            
            // Create mesh from voxel data for rendering
            VoxelData::VoxelGrid* grid = voxelData->getGrid(VoxelResolution::Size_8cm);
            if (grid) {
                RenderSettings settings;
                settings.renderMode = RenderMode::Solid;
                renderEngine->renderVoxels(*grid, VoxelResolution::Size_8cm, settings);
            }
            
            renderEngine->endFrame();
            glfwSwapBuffers(window);
        }
        
        // Measure frame times
        std::vector<double> frameTimes;
        const int measureFrames = 100;
        
        for (int i = 0; i < measureFrames; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            renderEngine->beginFrame();
            renderEngine->setCamera(camera);
            
            VoxelData::VoxelGrid* grid = voxelData->getGrid(VoxelResolution::Size_8cm);
            if (grid) {
                RenderSettings settings;
                settings.renderMode = RenderMode::Solid;
                renderEngine->renderVoxels(*grid, VoxelResolution::Size_8cm, settings);
            }
            
            renderEngine->endFrame();
            
            // Include swap buffer time as it's part of the frame
            glfwSwapBuffers(window);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            frameTimes.push_back(duration.count() / 1000.0); // Convert to milliseconds
        }
        
        // Calculate average frame time
        double avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) 
                             / frameTimes.size();
        
        // Clear voxels for next test
        voxelData->clearAll();
        
        return avgFrameTime;
    }
};

// Test 60 FPS with varying voxel counts
// REQ-6.1.1: Grid rendering shall maintain 60 FPS minimum (90+ FPS for VR)
TEST_F(RenderPerformanceTest, Maintain60FPSWithVaryingVoxelCounts) {
    const double targetFrameTime = 16.67; // 60 FPS = 16.67ms per frame
    
    struct TestCase {
        int voxelCount;
        const char* description;
    };
    
    std::vector<TestCase> testCases = {
        {100, "100 voxels"},
        {1000, "1,000 voxels"},
        {5000, "5,000 voxels"},
        {10000, "10,000 voxels"}
    };
    
    Logger& logger = Logger::getInstance();
    logger.info("Performance Test: 60 FPS Target (16.67ms)");
    logger.info("=========================================");
    
    for (const auto& test : testCases) {
        double frameTime = measureFrameTime(test.voxelCount);
        double fps = 1000.0 / frameTime;
        
        logger.infof("%s: %.2fms (%.1f FPS)", 
                   test.description, frameTime, fps);
        
        // Verify we maintain 60 FPS (with 10% tolerance)
        EXPECT_LT(frameTime, targetFrameTime * 1.1) 
            << "Failed to maintain 60 FPS with " << test.voxelCount << " voxels. "
            << "Frame time: " << frameTime << "ms (target: " << targetFrameTime << "ms)";
    }
}

// Test frame time stays under 16ms
// REQ-6.1.2: Preview updates shall complete within 16ms
// REQ-4.1.3: Preview updates shall be smooth and responsive (< 16ms)
TEST_F(RenderPerformanceTest, FrameTimeUnder16ms) {
    const double maxFrameTime = 16.0; // Slightly stricter than 16.67
    
    // Test with a moderate voxel count
    const int voxelCount = 5000;
    double frameTime = measureFrameTime(voxelCount);
    
    EXPECT_LT(frameTime, maxFrameTime) 
        << "Frame time exceeds 16ms limit. Actual: " << frameTime << "ms";
    
    Logger& logger = Logger::getInstance();
    logger.infof("Frame time test: %.2fms (limit: %.0fms)", 
               frameTime, maxFrameTime);
}

// Test performance with 10,000+ voxels
// REQ-6.2.1: System shall handle 10,000+ voxels without degradation
TEST_F(RenderPerformanceTest, HandleLargeVoxelCountsWithoutDegradation) {
    struct LargeScaleTest {
        int voxelCount;
        double maxAcceptableTime; // milliseconds
    };
    
    std::vector<LargeScaleTest> tests = {
        {10000, 20.0},  // 10k voxels should render in 20ms (50 FPS)
        {20000, 33.3},  // 20k voxels should render in 33ms (30 FPS minimum)
        {30000, 50.0}   // 30k voxels should render in 50ms (20 FPS minimum)
    };
    
    Logger& logger = Logger::getInstance();
    logger.info("\nLarge Scale Performance Test:");
    logger.info("==============================");
    
    for (const auto& test : tests) {
        double frameTime = measureFrameTime(test.voxelCount);
        double fps = 1000.0 / frameTime;
        
        logger.infof("%d voxels: %.2fms (%.1f FPS)", 
                   test.voxelCount, frameTime, fps);
        
        EXPECT_LT(frameTime, test.maxAcceptableTime)
            << "Performance degradation with " << test.voxelCount << " voxels. "
            << "Frame time: " << frameTime << "ms (max: " << test.maxAcceptableTime << "ms)";
    }
}

// Test render stats accuracy
TEST_F(RenderPerformanceTest, RenderStatsAccuracy) {
    const int voxelCount = 1000;
    
    // Place voxels
    for (int i = 0; i < voxelCount; ++i) {
        voxelData->setVoxel(Vector3i(i % 10, (i / 10) % 10, i / 100), 
                           VoxelResolution::Size_8cm, true);
    }
    
    // Set up camera
    Camera::OrbitCamera camera;
    camera.setPosition(Math::WorldCoordinates(Vector3f(10.0f, 10.0f, 10.0f)));
    camera.setTarget(Math::WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)));
    camera.setAspectRatio(16.0f/9.0f);
    
    // Render and get stats
    renderEngine->beginFrame();
    renderEngine->setCamera(camera);
    
    VoxelData::VoxelGrid* grid = voxelData->getGrid(VoxelResolution::Size_8cm);
    if (grid) {
        RenderSettings settings;
        settings.renderMode = RenderMode::Solid;
        renderEngine->renderVoxels(*grid, VoxelResolution::Size_8cm, settings);
    }
    
    renderEngine->endFrame();
    
    const RenderStats& stats = renderEngine->getRenderStats();
    
    // Verify stats are reasonable
    EXPECT_GT(stats.drawCalls, 0) << "No draw calls recorded";
    EXPECT_GT(stats.verticesProcessed, 0) << "No vertices recorded";
    EXPECT_GT(stats.trianglesRendered, 0) << "No triangles recorded";
    EXPECT_GE(stats.frameTime, 0.0f) << "Invalid frame time";
    
    Logger& logger = Logger::getInstance();
    logger.infof("\nRender Stats for %d voxels:", voxelCount);
    logger.infof("  Draw calls: %d", stats.drawCalls);
    logger.infof("  Vertices: %d", stats.verticesProcessed);
    logger.infof("  Triangles: %d", stats.trianglesRendered);
    logger.infof("  Frame time: %.2fms", stats.frameTime);
}

// Test performance consistency (no spikes)
TEST_F(RenderPerformanceTest, ConsistentFrameTiming) {
    const int voxelCount = 5000;
    const int frames = 200;
    
    // Place voxels
    for (int i = 0; i < voxelCount; ++i) {
        voxelData->setVoxel(Vector3i(i % 20, (i / 20) % 20, i / 400), 
                           VoxelResolution::Size_8cm, true);
    }
    
    // Set up camera
    Camera::OrbitCamera camera;
    camera.setPosition(Math::WorldCoordinates(Vector3f(15.0f, 15.0f, 15.0f)));
    camera.setTarget(Math::WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)));
    camera.setAspectRatio(16.0f/9.0f);
    
    // Collect frame times
    std::vector<double> frameTimes;
    for (int i = 0; i < frames; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        renderEngine->beginFrame();
        renderEngine->setCamera(camera);
        
        VoxelData::VoxelGrid* grid = voxelData->getGrid(VoxelResolution::Size_8cm);
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
    
    // Calculate statistics
    double avgTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) / frameTimes.size();
    double maxTime = *std::max_element(frameTimes.begin(), frameTimes.end());
    double minTime = *std::min_element(frameTimes.begin(), frameTimes.end());
    
    // Calculate standard deviation
    double variance = 0.0;
    for (double time : frameTimes) {
        variance += (time - avgTime) * (time - avgTime);
    }
    double stdDev = std::sqrt(variance / frameTimes.size());
    
    Logger& logger = Logger::getInstance();
    logger.infof("\nFrame Time Consistency Test (%d frames):", frames);
    logger.infof("  Average: %.2fms", avgTime);
    logger.infof("  Min: %.2fms", minTime);
    logger.infof("  Max: %.2fms", maxTime);
    logger.infof("  Std Dev: %.2fms", stdDev);
    
    // Verify consistency
    EXPECT_LT(stdDev, 2.0) << "Frame time variance too high";
    EXPECT_LT(maxTime - minTime, 10.0) << "Frame time spikes detected";
    EXPECT_LT(maxTime, avgTime * 2.0) << "Severe frame time spike detected";
}