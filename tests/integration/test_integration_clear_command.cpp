#include <gtest/gtest.h>
#include "../../core/voxel_data/VoxelDataManager.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/math/CoordinateTypes.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class IntegrationClearCommandTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
};

TEST_F(IntegrationClearCommandTest, ClearAllVoxels) {
    // Add voxels at multiple resolutions
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_1cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(10, 0, 10), VoxelResolution::Size_1cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(20, 0, 20), VoxelResolution::Size_4cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(40, 0, 40), VoxelResolution::Size_4cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(60, 0, 60), VoxelResolution::Size_16cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(100, 0, 100), VoxelResolution::Size_64cm, true));
    
    // Verify voxels were placed
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_1cm), 2);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_4cm), 2);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_16cm), 1);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_64cm), 1);
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 6);
    
    // Clear all voxels
    voxelManager->clearAll();
    
    // Verify all voxels are cleared
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_1cm), 0);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_4cm), 0);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_16cm), 0);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_64cm), 0);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_256cm), 0);
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 0);
    
    // Verify voxels are actually gone
    EXPECT_FALSE(voxelManager->getVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_1cm));
    EXPECT_FALSE(voxelManager->getVoxel(Vector3i(20, 0, 20), VoxelResolution::Size_4cm));
    EXPECT_FALSE(voxelManager->getVoxel(Vector3i(60, 0, 60), VoxelResolution::Size_16cm));
    EXPECT_FALSE(voxelManager->getVoxel(Vector3i(100, 0, 100), VoxelResolution::Size_64cm));
}

TEST_F(IntegrationClearCommandTest, ClearSpecificResolution) {
    // Add voxels at multiple resolutions
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_1cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(10, 0, 10), VoxelResolution::Size_1cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(20, 0, 20), VoxelResolution::Size_4cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(40, 0, 40), VoxelResolution::Size_4cm, true));
    
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 4);
    
    // Clear only 1cm voxels
    voxelManager->clearResolution(VoxelResolution::Size_1cm);
    
    // Verify only 1cm voxels are cleared
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_1cm), 0);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_4cm), 2);
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 2);
    
    // Verify specific voxels
    EXPECT_FALSE(voxelManager->getVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_1cm));
    EXPECT_FALSE(voxelManager->getVoxel(Vector3i(10, 0, 10), VoxelResolution::Size_1cm));
    EXPECT_TRUE(voxelManager->getVoxel(Vector3i(20, 0, 20), VoxelResolution::Size_4cm));
    EXPECT_TRUE(voxelManager->getVoxel(Vector3i(40, 0, 40), VoxelResolution::Size_4cm));
}

TEST_F(IntegrationClearCommandTest, ClearActiveResolution) {
    // Set active resolution
    voxelManager->setActiveResolution(VoxelResolution::Size_4cm);
    
    // Add voxels at multiple resolutions
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_1cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(20, 0, 20), VoxelResolution::Size_4cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(40, 0, 40), VoxelResolution::Size_4cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(60, 0, 60), VoxelResolution::Size_16cm, true));
    
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 4);
    
    // Clear active resolution (4cm)
    voxelManager->clearActiveResolution();
    
    // Verify only 4cm voxels are cleared
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_1cm), 1);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_4cm), 0);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_16cm), 1);
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 2);
}

TEST_F(IntegrationClearCommandTest, ClearEmptyManager) {
    // Verify clearing empty manager doesn't crash
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 0);
    
    voxelManager->clearAll();
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 0);
    
    voxelManager->clearResolution(VoxelResolution::Size_4cm);
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 0);
    
    voxelManager->clearActiveResolution();
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 0);
}

TEST_F(IntegrationClearCommandTest, ClearAndReusePositions) {
    // Add voxels
    Vector3i pos1(0, 0, 0);
    Vector3i pos2(20, 0, 20);
    
    ASSERT_TRUE(voxelManager->setVoxel(pos1, VoxelResolution::Size_4cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(pos2, VoxelResolution::Size_4cm, true));
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 2);
    
    // Clear all
    voxelManager->clearAll();
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 0);
    
    // Reuse the same positions
    EXPECT_TRUE(voxelManager->setVoxel(pos1, VoxelResolution::Size_4cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(pos2, VoxelResolution::Size_4cm, true));
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 2);
    
    // Verify voxels are placed correctly
    EXPECT_TRUE(voxelManager->getVoxel(pos1, VoxelResolution::Size_4cm));
    EXPECT_TRUE(voxelManager->getVoxel(pos2, VoxelResolution::Size_4cm));
}

TEST_F(IntegrationClearCommandTest, ClearAliasMethod) {
    // Test that clear() is an alias for clearAll()
    
    // Add some voxels
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_1cm, true));
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(20, 0, 20), VoxelResolution::Size_4cm, true));
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 2);
    
    // Use clear() method
    voxelManager->clear();
    
    // Verify all voxels are cleared
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 0);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_1cm), 0);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_4cm), 0);
}

TEST_F(IntegrationClearCommandTest, MemoryOptimizationAfterClear) {
    // Add many voxels
    for (int i = 0; i < 100; i += 10) {
        voxelManager->setVoxel(Vector3i(i, 0, i), VoxelResolution::Size_1cm, true);
        voxelManager->setVoxel(Vector3i(i+200, 0, i), VoxelResolution::Size_4cm, true);
    }
    
    size_t memoryBefore = voxelManager->getMemoryUsage();
    EXPECT_GT(memoryBefore, 0);
    
    // Clear all voxels
    voxelManager->clearAll();
    
    // Optimize memory
    voxelManager->optimizeMemory();
    
    size_t memoryAfter = voxelManager->getMemoryUsage();
    
    // Memory usage should be reduced after clearing and optimizing
    EXPECT_LT(memoryAfter, memoryBefore);
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 0);
}