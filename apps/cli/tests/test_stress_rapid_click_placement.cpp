#include <gtest/gtest.h>
#include "../include/cli/Application.h"
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "input/InputManager.h"
#include "undo_redo/HistoryManager.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Vector3i.h"
#include "foundation/logging/Logger.h"
#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include <algorithm>

// Include OpenGL headers for window context
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#else
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Logging;

class RapidClickStressTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Application> app;
    VoxelDataManager* voxelManager = nullptr;
    Input::InputManager* inputManager = nullptr;
    
    void SetUp() override {
        // Initialize GLFW for window context
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        
        // Create a minimal window for OpenGL context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for tests
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        window = glfwCreateWindow(800, 600, "Stress Test", nullptr, nullptr);
        ASSERT_NE(window, nullptr) << "Failed to create GLFW window";
        
        glfwMakeContextCurrent(window);
        
#ifndef __APPLE__
        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
            << "Failed to initialize OpenGL function pointers";
#endif
        
        // Create application with minimal setup
        int argc = 2;
        char arg0[] = "test";
        char arg1[] = "--test-mode";
        char* argv[] = {arg0, arg1, nullptr};
        
        app = std::make_unique<Application>();
        ASSERT_TRUE(app->initialize(argc, argv)) << "Failed to initialize application";
        
        // Get components for testing
        voxelManager = app->getVoxelManager();
        inputManager = app->getInputManager();
        
        ASSERT_NE(voxelManager, nullptr) << "VoxelDataManager not available";
        ASSERT_NE(inputManager, nullptr) << "InputManager not available";
        
        // Set up basic test environment
        voxelManager->setActiveResolution(VoxelResolution::Size_8cm);
        voxelManager->resizeWorkspace(Vector3f(5.0f, 5.0f, 5.0f)); // 5m workspace for testing
    }
    
    void TearDown() override {
        app.reset();
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    // Simulate rapid voxel placement directly
    void simulateVoxelPlacement(const Vector3i& pos) {
        voxelManager->setVoxel(pos, voxelManager->getActiveResolution(), true);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    // Simulate rapid voxel removal
    void simulateVoxelRemoval(const Vector3i& pos) {
        voxelManager->setVoxel(pos, voxelManager->getActiveResolution(), false);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    // Generate random voxel coordinates within workspace
    Vector3i getRandomVoxelCoords(std::mt19937& rng) {
        std::uniform_int_distribution<int> coordDist(-25, 25); // 5m workspace = -2.5m to +2.5m
        return Vector3i(coordDist(rng), std::abs(coordDist(rng)) / 2, coordDist(rng));
    }
};

// Test rapid sequential voxel placements at the same location
TEST_F(RapidClickStressTest, RapidSequentialClicks) {
    Logger& logger = Logger::getInstance();
    logger.info("Testing rapid sequential voxel placements at same location...");
    
    const int placementCount = 100;
    const Vector3i pos(0, 0, 0); // Single position for all placements
    
    int initialVoxelCount = voxelManager->getTotalVoxelCount();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Perform rapid placements
    for (int i = 0; i < placementCount; ++i) {
        simulateVoxelPlacement(pos);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    int finalVoxelCount = voxelManager->getTotalVoxelCount();
    int voxelsAdded = finalVoxelCount - initialVoxelCount;
    
    double avgPlacementTime = static_cast<double>(duration.count()) / placementCount;
    
    logger.info("Rapid sequential placements results:");
    logger.info("  Total time: " + std::to_string(duration.count()) + "ms");
    logger.info("  Average per placement: " + std::to_string(avgPlacementTime) + "ms");
    logger.info("  Voxels added: " + std::to_string(voxelsAdded) + "/" + std::to_string(placementCount));
    
    // Performance expectations
    EXPECT_LT(avgPlacementTime, 10.0) << "Each placement should process under 10ms";
    EXPECT_GT(voxelsAdded, 0) << "Should place at least some voxels";
    EXPECT_LT(duration.count(), 2000) << "100 placements should complete under 2 seconds";
    
    // System should remain stable (no crashes)
    EXPECT_TRUE(app->isRunning()) << "Application should remain running after rapid placements";
}

// Test rapid voxel placements at random positions (most stressful scenario)
TEST_F(RapidClickStressTest, RapidClicksWithMouseMovement) {
    Logger& logger = Logger::getInstance();
    logger.info("Testing rapid voxel placements at random positions...");
    
    const int placementCount = 200;
    std::mt19937 rng(42); // Fixed seed for reproducibility
    
    int initialVoxelCount = voxelManager->getTotalVoxelCount();
    std::vector<double> placementTimes;
    
    auto overallStart = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < placementCount; ++i) {
        auto placementStart = std::chrono::high_resolution_clock::now();
        
        // Get random voxel position
        Vector3i pos = getRandomVoxelCoords(rng);
        
        // Simulate voxel placement
        simulateVoxelPlacement(pos);
        
        auto placementEnd = std::chrono::high_resolution_clock::now();
        auto placementDuration = std::chrono::duration_cast<std::chrono::microseconds>(placementEnd - placementStart);
        placementTimes.push_back(placementDuration.count() / 1000.0); // Convert to milliseconds
    }
    
    auto overallEnd = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(overallEnd - overallStart);
    
    int finalVoxelCount = voxelManager->getTotalVoxelCount();
    int voxelsAdded = finalVoxelCount - initialVoxelCount;
    
    // Calculate statistics
    double avgTime = std::accumulate(placementTimes.begin(), placementTimes.end(), 0.0) / placementTimes.size();
    double maxTime = *std::max_element(placementTimes.begin(), placementTimes.end());
    double minTime = *std::min_element(placementTimes.begin(), placementTimes.end());
    
    logger.info("Rapid placements at random positions results:");
    logger.info("  Total time: " + std::to_string(totalDuration.count()) + "ms");
    logger.info("  Average per placement: " + std::to_string(avgTime) + "ms");
    logger.info("  Max placement time: " + std::to_string(maxTime) + "ms");
    logger.info("  Min placement time: " + std::to_string(minTime) + "ms");
    logger.info("  Voxels added: " + std::to_string(voxelsAdded) + "/" + std::to_string(placementCount) + " (" + std::to_string(100.0 * voxelsAdded / placementCount) + "%)");
    
    // Performance requirements
    EXPECT_LT(avgTime, 15.0) << "Average placement time should be under 15ms with position changes";
    EXPECT_LT(maxTime, 100.0) << "No single placement should take longer than 100ms";
    EXPECT_LT(totalDuration.count(), 5000) << "200 placements should complete under 5 seconds";
    
    // System stability
    EXPECT_TRUE(app->isRunning()) << "Application should remain stable";
}

// Test alternating voxel place/remove operations rapidly
TEST_F(RapidClickStressTest, AlternatingLeftRightClicks) {
    Logger& logger = Logger::getInstance();
    logger.info("Testing alternating voxel place/remove operations...");
    
    const int operationPairs = 50; // 100 total operations
    std::mt19937 rng(123);
    
    int initialVoxelCount = voxelManager->getTotalVoxelCount();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < operationPairs; ++i) {
        Vector3i pos = getRandomVoxelCoords(rng);
        
        // Place voxel
        simulateVoxelPlacement(pos);
        
        // Small delay then remove voxel
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        simulateVoxelRemoval(pos);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    int finalVoxelCount = voxelManager->getTotalVoxelCount();
    int netVoxelChange = finalVoxelCount - initialVoxelCount;
    
    logger.info("Alternating operations results:");
    logger.info("  Total time: " + std::to_string(duration.count()) + "ms");
    logger.info("  Net voxel change: " + std::to_string(netVoxelChange));
    logger.info("  Average per operation pair: " + std::to_string(static_cast<double>(duration.count()) / operationPairs) + "ms");
    
    // The net change should be minimal (some voxels might remain due to placement logic)
    EXPECT_LT(std::abs(netVoxelChange), operationPairs / 2) 
        << "Most voxels should be cancelled out by remove operations";
    EXPECT_LT(duration.count(), 3000) << "Alternating operations should complete under 3 seconds";
    EXPECT_TRUE(app->isRunning()) << "Application should remain stable";
}

// Test clicking under memory pressure (many existing voxels)
TEST_F(RapidClickStressTest, ClickingUnderMemoryPressure) {
    Logger& logger = Logger::getInstance();
    logger.info("Testing clicking under memory pressure...");
    
    // First, populate the scene with many voxels
    const int preExistingVoxels = 5000;
    std::mt19937 rng(456);
    std::uniform_int_distribution<int> coordDist(-25, 25); // 5m workspace = -2.5m to +2.5m
    
    voxelManager->setActiveResolution(VoxelResolution::Size_8cm);
    
    // Add many voxels to create memory pressure
    for (int i = 0; i < preExistingVoxels; ++i) {
        Vector3i pos(coordDist(rng), std::abs(coordDist(rng)) / 2, coordDist(rng));
        voxelManager->setVoxel(pos, VoxelResolution::Size_8cm, true);
    }
    
    int voxelCountBeforeClicks = voxelManager->getTotalVoxelCount();
    logger.info("Created " + std::to_string(voxelCountBeforeClicks) + " pre-existing voxels");
    
    // Now perform rapid clicking
    const int rapidClicks = 50;
    std::vector<double> clickTimes;
    
    for (int i = 0; i < rapidClicks; ++i) {
        auto placementStart = std::chrono::high_resolution_clock::now();
        
        Vector3i pos = getRandomVoxelCoords(rng);
        simulateVoxelPlacement(pos);
        
        auto placementEnd = std::chrono::high_resolution_clock::now();
        auto placementDuration = std::chrono::duration_cast<std::chrono::microseconds>(placementEnd - placementStart);
        clickTimes.push_back(placementDuration.count() / 1000.0);
    }
    
    int finalVoxelCount = voxelManager->getTotalVoxelCount();
    
    double avgPlacementTime = std::accumulate(clickTimes.begin(), clickTimes.end(), 0.0) / clickTimes.size();
    double maxPlacementTime = *std::max_element(clickTimes.begin(), clickTimes.end());
    
    logger.info("Memory pressure placement results:");
    logger.info("  Pre-existing voxels: " + std::to_string(voxelCountBeforeClicks));
    logger.info("  Final voxel count: " + std::to_string(finalVoxelCount));
    logger.info("  Average placement time: " + std::to_string(avgPlacementTime) + "ms");
    logger.info("  Max placement time: " + std::to_string(maxPlacementTime) + "ms");
    
    // Performance should degrade gracefully under memory pressure
    EXPECT_LT(avgPlacementTime, 25.0) << "Placements should still process under 25ms with memory pressure";
    EXPECT_LT(maxPlacementTime, 200.0) << "Even worst-case placements should be under 200ms";
    EXPECT_TRUE(app->isRunning()) << "Application should remain stable under memory pressure";
}

// Test rapid resolution switching with clicks
TEST_F(RapidClickStressTest, RapidResolutionSwitchingWithClicks) {
    Logger& logger = Logger::getInstance();
    logger.info("Testing rapid resolution switching with clicks...");
    
    const std::vector<VoxelResolution> resolutions = {
        VoxelResolution::Size_1cm,
        VoxelResolution::Size_4cm,
        VoxelResolution::Size_8cm,
        VoxelResolution::Size_16cm,
        VoxelResolution::Size_32cm
    };
    
    const int clicksPerResolution = 10;
    std::mt19937 rng(789);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (size_t resIndex = 0; resIndex < resolutions.size(); ++resIndex) {
        VoxelResolution res = resolutions[resIndex];
        voxelManager->setActiveResolution(res);
        
        // Perform placements at this resolution
        for (int placement = 0; placement < clicksPerResolution; ++placement) {
            Vector3i pos = getRandomVoxelCoords(rng);
            simulateVoxelPlacement(pos);
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    int totalVoxels = voxelManager->getTotalVoxelCount();
    
    logger.info("Resolution switching results:");
    logger.info("  Total time: " + std::to_string(duration.count()) + "ms");
    logger.info("  Total voxels placed: " + std::to_string(totalVoxels));
    logger.info("  Resolutions tested: " + std::to_string(resolutions.size()));
    
    EXPECT_GT(totalVoxels, 0) << "Should place voxels across different resolutions";
    EXPECT_LT(duration.count(), 3000) << "Resolution switching with placements should be efficient";
    EXPECT_TRUE(app->isRunning()) << "Application should remain stable";
}

// Test system recovery after stress
TEST_F(RapidClickStressTest, SystemRecoveryAfterStress) {
    Logger& logger = Logger::getInstance();
    logger.info("Testing system recovery after stress...");
    
    // First, stress the system
    const int stressClicks = 100;
    std::mt19937 rng(101112);
    
    for (int i = 0; i < stressClicks; ++i) {
        Vector3i pos = getRandomVoxelCoords(rng);
        simulateVoxelPlacement(pos);
        
        // Some placements with very short intervals
        if (i % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    }
    
    int voxelCountAfterStress = voxelManager->getTotalVoxelCount();
    logger.info("Voxels after stress: " + std::to_string(voxelCountAfterStress));
    
    // Allow system to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test that system is still responsive
    Vector3i testPos(1, 0, 1);
    
    auto responseStart = std::chrono::high_resolution_clock::now();
    simulateVoxelPlacement(testPos);
    auto responseEnd = std::chrono::high_resolution_clock::now();
    
    auto responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(responseEnd - responseStart);
    
    int finalVoxelCount = voxelManager->getTotalVoxelCount();
    
    logger.info("System recovery results:");
    logger.info("  Response time after stress: " + std::to_string(responseTime.count()) + "ms");
    logger.info("  System responsive: " + std::string((responseTime.count() < 50) ? "YES" : "NO"));
    logger.info("  Final voxel count: " + std::to_string(finalVoxelCount));
    
    // System should recover quickly and remain responsive
    EXPECT_LT(responseTime.count(), 50) << "System should respond quickly after stress";
    EXPECT_GE(finalVoxelCount, voxelCountAfterStress) << "Should be able to add more voxels";
    EXPECT_TRUE(app->isRunning()) << "Application should remain running after stress recovery";
}