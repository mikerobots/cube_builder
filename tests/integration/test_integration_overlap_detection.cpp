#include <gtest/gtest.h>
#include "../../core/voxel_data/VoxelDataManager.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/math/CoordinateTypes.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class IntegrationOverlapDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
};

// REQ-5.2.1: Voxels shall not overlap with existing voxels
TEST_F(IntegrationOverlapDetectionTest, SameResolutionOverlapPrevention) {
    // Place a 4cm voxel at origin
    Vector3i pos(0, 0, 0);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    ASSERT_TRUE(voxelManager->setVoxel(pos, resolution, true));
    EXPECT_EQ(voxelManager->getVoxelCount(resolution), 1);
    
    // Try to place another voxel at the same position - redundant operation succeeds
    EXPECT_TRUE(voxelManager->setVoxel(pos, resolution, true));
    EXPECT_EQ(voxelManager->getVoxelCount(resolution), 1);
    
    // Remove the voxel
    EXPECT_TRUE(voxelManager->setVoxel(pos, resolution, false));
    EXPECT_EQ(voxelManager->getVoxelCount(resolution), 0);
    
    // Place first voxel again
    EXPECT_TRUE(voxelManager->setVoxel(pos, resolution, true));
    EXPECT_EQ(voxelManager->getVoxelCount(resolution), 1);
    
    // Try to place a different voxel that would overlap - should fail
    Vector3i overlapPos(1, 1, 1); // Within the 4cm voxel bounds
    EXPECT_FALSE(voxelManager->setVoxel(overlapPos, resolution, true));
    EXPECT_EQ(voxelManager->getVoxelCount(resolution), 1);
    
    // Verify the original voxel is still there
    EXPECT_TRUE(voxelManager->getVoxel(pos, resolution));
}

TEST_F(IntegrationOverlapDetectionTest, DifferentResolutionOverlapPrevention) {
    // Place a 16cm voxel at (0, 0, 0)
    Vector3i largePos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largePos, VoxelResolution::Size_16cm, true));
    
    // Try to place smaller voxels that would overlap
    // 16cm voxel covers 0-16cm in each dimension
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(8, 8, 8), VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(15, 15, 15), VoxelResolution::Size_1cm, true));
    
    // Place a 1cm voxel outside the 16cm voxel - should succeed
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(17, 0, 0), VoxelResolution::Size_1cm, true));
    
    // Verify voxel counts
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_16cm), 1);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_1cm), 1);
}

TEST_F(IntegrationOverlapDetectionTest, ComplexOverlapScenario) {
    // Create a complex scene with multiple voxels
    // Place 4cm voxels in a pattern
    std::vector<Vector3i> positions4cm = {
        Vector3i(0, 0, 0),
        Vector3i(8, 0, 0),   // 2 voxels apart (no overlap)
        Vector3i(0, 8, 0),
        Vector3i(0, 0, 8)
    };
    
    for (const auto& pos : positions4cm) {
        ASSERT_TRUE(voxelManager->setVoxel(pos, VoxelResolution::Size_4cm, true));
    }
    
    // Try to place 1cm voxels in overlapping positions
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(2, 2, 2), VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(9, 1, 1), VoxelResolution::Size_1cm, true));
    
    // Place 1cm voxels in non-overlapping positions
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(5, 5, 5), VoxelResolution::Size_1cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(12, 12, 12), VoxelResolution::Size_1cm, true));
    
    // Verify final counts
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_4cm), 4);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_1cm), 2);
}

TEST_F(IntegrationOverlapDetectionTest, RedundantOperationHandling) {
    Vector3i pos(10, 10, 10);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Place a voxel
    ASSERT_TRUE(voxelManager->setVoxel(pos, resolution, true));
    
    // Try to place the same voxel again - should succeed (redundant operation)
    EXPECT_TRUE(voxelManager->setVoxel(pos, resolution, true));
    EXPECT_EQ(voxelManager->getVoxelCount(resolution), 1);
    
    // Remove the voxel
    EXPECT_TRUE(voxelManager->setVoxel(pos, resolution, false));
    EXPECT_EQ(voxelManager->getVoxelCount(resolution), 0);
    
    // Try to remove again - should succeed (redundant operation)
    EXPECT_TRUE(voxelManager->setVoxel(pos, resolution, false));
    EXPECT_EQ(voxelManager->getVoxelCount(resolution), 0);
}

TEST_F(IntegrationOverlapDetectionTest, WorldSpaceOverlapDetection) {
    // Test overlap detection using world space coordinates
    Vector3f worldPos1(0.1f, 0.1f, 0.1f);  // 10cm, 10cm, 10cm
    Vector3f worldPos2(0.11f, 0.11f, 0.11f);  // 11cm, 11cm, 11cm - would overlap with 4cm voxel
    
    // Place first voxel
    ASSERT_TRUE(voxelManager->setVoxelAtWorldPos(worldPos1, VoxelResolution::Size_4cm, true));
    
    // Try to place overlapping voxel
    EXPECT_FALSE(voxelManager->setVoxelAtWorldPos(worldPos2, VoxelResolution::Size_4cm, true));
    
    // Place non-overlapping voxel
    Vector3f worldPos3(0.2f, 0.2f, 0.2f);  // 20cm, 20cm, 20cm
    EXPECT_TRUE(voxelManager->setVoxelAtWorldPos(worldPos3, VoxelResolution::Size_4cm, true));
    
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_4cm), 2);
}

// REQ-5.2.2: System shall validate placement before allowing it
TEST_F(IntegrationOverlapDetectionTest, PlacementValidationOrder) {
    // This test verifies that all validations occur before placement
    
    // Try to place at invalid position (Y < 0) - should fail early
    Vector3i invalidPos(0, -1, 0);
    EXPECT_FALSE(voxelManager->setVoxel(invalidPos, VoxelResolution::Size_1cm, true));
    
    // Place a valid voxel
    Vector3i validPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(validPos, VoxelResolution::Size_4cm, true));
    
    // Try to place overlapping voxel at invalid Y - should fail on Y check first
    Vector3i overlapInvalidPos(2, -1, 2);
    EXPECT_FALSE(voxelManager->setVoxel(overlapInvalidPos, VoxelResolution::Size_4cm, true));
    
    // Verify only one voxel was placed
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 1);
}