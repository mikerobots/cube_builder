#include <gtest/gtest.h>
#include <chrono>
#include "../VoxelDataManager.h"
#include "../SparseOctree.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class VoxelDataRequirementsPerfTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        manager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> manager;
};

// REQ-4.3.1: Sparse storage efficiency performance test
TEST_F(VoxelDataRequirementsPerfTest, SparseStoragePerformance) {
    // Test memory efficiency with sparse voxel placement
    size_t initialMemory = manager->getMemoryUsage();
    
    // Add 1000 voxels in a very sparse pattern
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        // Spread voxels across the workspace
        int x = (i * 13) % 200 - 100;  // Range: -100 to 99
        int y = (i * 7) % 50;           // Range: 0 to 49
        int z = (i * 11) % 200 - 100;  // Range: -100 to 99
        
        manager->setVoxel(IncrementCoordinates(x, y, z), VoxelResolution::Size_1cm, true);
    }
    
    size_t finalMemory = manager->getMemoryUsage();
    size_t memoryPerVoxel = (finalMemory - initialMemory) / count;
    
    // REQ-4.3.1: Memory usage should be proportional to actual voxel count
    // Allow up to 1KB per voxel for sparse storage overhead
    EXPECT_LT(memoryPerVoxel, 1024) << "Memory per voxel: " << memoryPerVoxel << " bytes";
}

// REQ-6.1.4: Resolution switching should be < 16ms
TEST_F(VoxelDataRequirementsPerfTest, ResolutionSwitchingPerformance_REQ_6_1_4) {
    // Add some voxels to make the test more realistic
    for (int i = 0; i < 100; ++i) {
        manager->setVoxel(IncrementCoordinates(i, 0, i), VoxelResolution::Size_1cm, true);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Switch between all resolutions
    for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
        manager->setActiveResolution(static_cast<VoxelResolution>(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // REQ-6.1.4: Resolution switching must complete in < 16ms
    EXPECT_LT(duration.count(), 16000) << "Resolution switching took " 
                                       << duration.count() << " microseconds";
}