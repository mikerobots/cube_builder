#include <gtest/gtest.h>
#include "../RenderStats.h"
#include <thread>
#include <chrono>

using namespace VoxelEditor::Rendering;

class RenderStatsTest : public ::testing::Test {
protected:
    void SetUp() override {
        stats = RenderStats();
    }
    
    void TearDown() override {}
    
    RenderStats stats;
};

TEST_F(RenderStatsTest, DefaultConstruction) {
    EXPECT_EQ(stats.frameCount, 0);
    EXPECT_FLOAT_EQ(stats.frameTime, 0.0f);
    EXPECT_FLOAT_EQ(stats.averageFrameTime, 0.0f);
    EXPECT_FLOAT_EQ(stats.fps, 0.0f);
    EXPECT_FLOAT_EQ(stats.minFrameTime, 1000.0f);
    EXPECT_FLOAT_EQ(stats.maxFrameTime, 0.0f);
    
    EXPECT_EQ(stats.trianglesRendered, 0);
    EXPECT_EQ(stats.verticesProcessed, 0);
    EXPECT_EQ(stats.drawCalls, 0);
    EXPECT_EQ(stats.instancedDrawCalls, 0);
    
    EXPECT_EQ(stats.vertexBufferMemory, 0);
    EXPECT_EQ(stats.indexBufferMemory, 0);
    EXPECT_EQ(stats.textureMemory, 0);
    EXPECT_EQ(stats.totalGPUMemory, 0);
    
    EXPECT_FLOAT_EQ(stats.cpuTime, 0.0f);
    EXPECT_FLOAT_EQ(stats.gpuTime, 0.0f);
    EXPECT_EQ(stats.stateChanges, 0);
    EXPECT_EQ(stats.shaderSwitches, 0);
    EXPECT_EQ(stats.textureBinds, 0);
    
    EXPECT_EQ(stats.objectsCulled, 0);
    EXPECT_EQ(stats.objectsRendered, 0);
    EXPECT_EQ(stats.totalObjects, 0);
    
    EXPECT_EQ(stats.activeShaders, 0);
    EXPECT_EQ(stats.activeTextures, 0);
    EXPECT_EQ(stats.activeBuffers, 0);
}

TEST_F(RenderStatsTest, UpdateFrameStats) {
    // First frame update
    float deltaTime1 = 0.016f; // 60 FPS
    stats.update(deltaTime1);
    
    EXPECT_EQ(stats.frameCount, 1);
    EXPECT_FLOAT_EQ(stats.frameTime, 16.0f); // 16ms
    EXPECT_FLOAT_EQ(stats.fps, 62.5f); // 1/0.016
    EXPECT_FLOAT_EQ(stats.minFrameTime, 16.0f);
    EXPECT_FLOAT_EQ(stats.maxFrameTime, 16.0f);
    
    // Second frame update with different timing
    float deltaTime2 = 0.033f; // 30 FPS
    stats.update(deltaTime2);
    
    EXPECT_EQ(stats.frameCount, 2);
    EXPECT_FLOAT_EQ(stats.frameTime, 33.0f); // 33ms
    EXPECT_NEAR(stats.fps, 30.3f, 0.1f); // 1/0.033
    EXPECT_FLOAT_EQ(stats.minFrameTime, 16.0f);
    EXPECT_FLOAT_EQ(stats.maxFrameTime, 33.0f);
    
    // Third frame update with very fast timing
    float deltaTime3 = 0.008f; // 125 FPS
    stats.update(deltaTime3);
    
    EXPECT_EQ(stats.frameCount, 3);
    EXPECT_FLOAT_EQ(stats.frameTime, 8.0f); // 8ms
    EXPECT_FLOAT_EQ(stats.fps, 125.0f); // 1/0.008
    EXPECT_FLOAT_EQ(stats.minFrameTime, 8.0f);
    EXPECT_FLOAT_EQ(stats.maxFrameTime, 33.0f);
}

TEST_F(RenderStatsTest, DrawCallTracking) {
    // Add regular draw calls
    stats.addDrawCall(100, 300, false);
    EXPECT_EQ(stats.drawCalls, 1);
    EXPECT_EQ(stats.instancedDrawCalls, 0);
    EXPECT_EQ(stats.trianglesRendered, 100);
    EXPECT_EQ(stats.verticesProcessed, 300);
    
    // Add instanced draw call
    stats.addDrawCall(50, 150, true);
    EXPECT_EQ(stats.drawCalls, 1);
    EXPECT_EQ(stats.instancedDrawCalls, 1);
    EXPECT_EQ(stats.trianglesRendered, 150);
    EXPECT_EQ(stats.verticesProcessed, 450);
    
    // Test average triangles per draw call
    EXPECT_FLOAT_EQ(stats.getAverageTrianglesPerDrawCall(), 75.0f); // 150 / 2
}

TEST_F(RenderStatsTest, StateChangeTracking) {
    EXPECT_EQ(stats.stateChanges, 0);
    EXPECT_EQ(stats.shaderSwitches, 0);
    EXPECT_EQ(stats.textureBinds, 0);
    
    stats.addStateChange();
    stats.addStateChange();
    EXPECT_EQ(stats.stateChanges, 2);
    
    stats.addShaderSwitch();
    EXPECT_EQ(stats.shaderSwitches, 1);
    
    stats.addTextureBind();
    stats.addTextureBind();
    stats.addTextureBind();
    EXPECT_EQ(stats.textureBinds, 3);
}

TEST_F(RenderStatsTest, CullingStats) {
    stats.setCullingStats(1000, 750, 250);
    
    EXPECT_EQ(stats.totalObjects, 1000);
    EXPECT_EQ(stats.objectsRendered, 750);
    EXPECT_EQ(stats.objectsCulled, 250);
    EXPECT_FLOAT_EQ(stats.getCullingEfficiency(), 0.25f); // 250 / 1000
    
    // Test edge case with no objects
    stats.setCullingStats(0, 0, 0);
    EXPECT_FLOAT_EQ(stats.getCullingEfficiency(), 0.0f);
}

TEST_F(RenderStatsTest, MemoryStats) {
    size_t vbMem = 1024 * 1024;      // 1 MB
    size_t ibMem = 512 * 1024;       // 512 KB
    size_t texMem = 4 * 1024 * 1024; // 4 MB
    
    stats.setMemoryStats(vbMem, ibMem, texMem);
    
    EXPECT_EQ(stats.vertexBufferMemory, vbMem);
    EXPECT_EQ(stats.indexBufferMemory, ibMem);
    EXPECT_EQ(stats.textureMemory, texMem);
    EXPECT_EQ(stats.totalGPUMemory, vbMem + ibMem + texMem);
    EXPECT_EQ(stats.getTotalMemoryMB(), 5); // ~5.5 MB rounded down
}

TEST_F(RenderStatsTest, ResourceCounts) {
    stats.setResourceCounts(15, 32, 48);
    
    EXPECT_EQ(stats.activeShaders, 15);
    EXPECT_EQ(stats.activeTextures, 32);
    EXPECT_EQ(stats.activeBuffers, 48);
}

TEST_F(RenderStatsTest, UtilizationCalculations) {
    stats.frameTime = 16.0f; // 16ms frame time
    stats.cpuTime = 8.0f;    // 8ms CPU time
    stats.gpuTime = 12.0f;   // 12ms GPU time
    
    EXPECT_FLOAT_EQ(stats.getCPUUtilization(), 50.0f); // 8/16 * 100
    EXPECT_FLOAT_EQ(stats.getGPUUtilization(), 75.0f); // 12/16 * 100
    
    // Test edge case with zero frame time
    stats.frameTime = 0.0f;
    EXPECT_FLOAT_EQ(stats.getCPUUtilization(), 0.0f);
    EXPECT_FLOAT_EQ(stats.getGPUUtilization(), 0.0f);
}

TEST_F(RenderStatsTest, AverageFPSCalculation) {
    stats.averageFrameTime = 16.0f; // 16ms average
    EXPECT_FLOAT_EQ(stats.getAverageFPS(), 62.5f); // 1000/16
    
    stats.averageFrameTime = 0.0f;
    EXPECT_FLOAT_EQ(stats.getAverageFPS(), 0.0f);
}

TEST_F(RenderStatsTest, ResetOperations) {
    // Set up some initial values
    stats.frameTime = 16.0f;
    stats.trianglesRendered = 1000;
    stats.verticesProcessed = 3000;
    stats.drawCalls = 10;
    stats.cpuTime = 8.0f;
    stats.gpuTime = 12.0f;
    stats.stateChanges = 25;
    stats.shaderSwitches = 5;
    stats.textureBinds = 15;
    stats.objectsCulled = 100;
    stats.objectsRendered = 400;
    stats.totalObjects = 500;
    
    // Reset per-frame statistics
    stats.reset();
    
    // Per-frame stats should be reset
    EXPECT_FLOAT_EQ(stats.frameTime, 0.0f);
    EXPECT_EQ(stats.trianglesRendered, 0);
    EXPECT_EQ(stats.verticesProcessed, 0);
    EXPECT_EQ(stats.drawCalls, 0);
    EXPECT_FLOAT_EQ(stats.cpuTime, 0.0f);
    EXPECT_FLOAT_EQ(stats.gpuTime, 0.0f);
    EXPECT_EQ(stats.stateChanges, 0);
    EXPECT_EQ(stats.shaderSwitches, 0);
    EXPECT_EQ(stats.textureBinds, 0);
    EXPECT_EQ(stats.objectsCulled, 0);
    EXPECT_EQ(stats.objectsRendered, 0);
    EXPECT_EQ(stats.totalObjects, 0);
}

class RenderTimerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RenderTimerTest, BasicTiming) {
    RenderTimer timer;
    
    // Sleep for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    float elapsed = timer.getElapsedMs();
    EXPECT_GE(elapsed, 5.0f);  // At least 5ms (allowing for timing variance and scheduler delays)
    EXPECT_LE(elapsed, 50.0f); // Less than 50ms (allowing for significant system load and scheduler delays)
}

TEST_F(RenderTimerTest, RestartFunctionality) {
    RenderTimer timer;
    
    // Sleep and get elapsed time
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    float elapsed1 = timer.restart();
    
    EXPECT_GE(elapsed1, 2.0f);  // At least 2ms (allowing for timing variance)
    EXPECT_LE(elapsed1, 25.0f); // Less than 25ms (allowing for significant system load and scheduler delays)
    
    // Sleep again and get new elapsed time
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    float elapsed2 = timer.getElapsedMs();
    
    EXPECT_GE(elapsed2, 2.0f);  // At least 2ms (allowing for timing variance)
    EXPECT_LE(elapsed2, 25.0f); // Less than 25ms (allowing for significant system load and scheduler delays)
    EXPECT_LT(elapsed2, elapsed1 + 10.0f); // Should be much less than the combined time (allowing for scheduler delays)
}