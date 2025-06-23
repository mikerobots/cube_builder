#include <gtest/gtest.h>
#include "../../core/voxel_data/VoxelDataManager.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/math/CoordinateTypes.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class IntegrationGroundPlaneConstraintTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
};

// REQ-2.1.4: No voxels shall be placed below Y=0
TEST_F(IntegrationGroundPlaneConstraintTest, IncrementCoordinateYConstraint) {
    // Test with increment coordinates
    // Valid positions (Y >= 0)
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_1cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(10, 1, 10), VoxelResolution::Size_1cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(-50, 0, -50), VoxelResolution::Size_1cm, true));
    
    // Invalid positions (Y < 0)
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(0, -1, 0), VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(100, -10, 100), VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(-100, -100, -100), VoxelResolution::Size_1cm, true));
    
    // Verify only valid voxels were placed
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 3);
}

TEST_F(IntegrationGroundPlaneConstraintTest, WorldCoordinateYConstraint) {
    // Test with world coordinates
    // Valid positions (Y >= 0)
    EXPECT_TRUE(voxelManager->setVoxelAtWorldPos(Vector3f(0.0f, 0.0f, 0.0f), VoxelResolution::Size_4cm, true));
    EXPECT_TRUE(voxelManager->setVoxelAtWorldPos(Vector3f(0.1f, 0.01f, 0.1f), VoxelResolution::Size_4cm, true));
    EXPECT_TRUE(voxelManager->setVoxelAtWorldPos(Vector3f(-0.5f, 0.0f, -0.5f), VoxelResolution::Size_4cm, true));
    
    // Invalid positions (Y < 0)
    EXPECT_FALSE(voxelManager->setVoxelAtWorldPos(Vector3f(0.0f, -0.01f, 0.0f), VoxelResolution::Size_4cm, true));
    EXPECT_FALSE(voxelManager->setVoxelAtWorldPos(Vector3f(1.0f, -0.1f, 1.0f), VoxelResolution::Size_4cm, true));
    EXPECT_FALSE(voxelManager->setVoxelAtWorldPos(Vector3f(-1.0f, -1.0f, -1.0f), VoxelResolution::Size_4cm, true));
    
    // Verify only valid voxels were placed
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 3);
}

TEST_F(IntegrationGroundPlaneConstraintTest, MultipleResolutionYConstraint) {
    // Test Y constraint with different voxel sizes
    // All voxels must have their minimum corner at Y >= 0
    
    // Place voxels at Y = 0 for different resolutions
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_1cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(20, 0, 20), VoxelResolution::Size_4cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(50, 0, 50), VoxelResolution::Size_16cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(100, 0, 100), VoxelResolution::Size_64cm, true));
    
    // Try to place voxels below ground for each resolution
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(200, -1, 200), VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(220, -1, 220), VoxelResolution::Size_4cm, true));
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(250, -1, 250), VoxelResolution::Size_16cm, true));
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(300, -1, 300), VoxelResolution::Size_64cm, true));
    
    // Verify only valid voxels were placed
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_1cm), 1);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_4cm), 1);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_16cm), 1);
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelResolution::Size_64cm), 1);
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 4);
}

TEST_F(IntegrationGroundPlaneConstraintTest, VoxelBottomAtGroundPlane) {
    // Verify that voxels can sit exactly on the ground plane
    // The position represents the minimum corner, so Y=0 means bottom is at ground
    
    Vector3i groundPos(0, 0, 0);
    VoxelPosition voxelPos(groundPos, VoxelResolution::Size_4cm);
    
    // Place voxel at ground level
    ASSERT_TRUE(voxelManager->setVoxel(voxelPos, true));
    
    // Get world bounds to verify bottom is at Y=0
    Vector3f minBounds, maxBounds;
    voxelPos.getWorldBounds(minBounds, maxBounds);
    
    EXPECT_FLOAT_EQ(minBounds.y, 0.0f);  // Bottom at ground
    EXPECT_FLOAT_EQ(maxBounds.y, 0.04f); // Top at 4cm (for 4cm voxel)
}

TEST_F(IntegrationGroundPlaneConstraintTest, EdgeCaseNearZero) {
    // Test edge cases near Y=0 with floating point precision
    
    // Very small negative Y - should fail
    EXPECT_FALSE(voxelManager->setVoxelAtWorldPos(Vector3f(0.0f, -0.0001f, 0.0f), VoxelResolution::Size_1cm, true));
    
    // Exactly zero - should succeed
    EXPECT_TRUE(voxelManager->setVoxelAtWorldPos(Vector3f(0.1f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true));
    
    // Very small positive Y - should succeed (moved to avoid overlap)
    EXPECT_TRUE(voxelManager->setVoxelAtWorldPos(Vector3f(0.5f, 0.0001f, 0.0f), VoxelResolution::Size_1cm, true));
    
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 2);
}

TEST_F(IntegrationGroundPlaneConstraintTest, CombinedConstraints) {
    // Test Y constraint combined with other constraints
    
    // Place a valid voxel
    ASSERT_TRUE(voxelManager->setVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_4cm, true));
    
    // Try to place overlapping voxel with Y < 0
    // Should fail on Y constraint before checking overlap
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(0, -1, 0), VoxelResolution::Size_4cm, true));
    
    // Try to place out-of-bounds voxel with Y < 0
    // Should fail on Y constraint
    EXPECT_FALSE(voxelManager->setVoxel(Vector3i(1000, -1, 1000), VoxelResolution::Size_4cm, true));
    
    // Place valid non-overlapping voxel above ground
    EXPECT_TRUE(voxelManager->setVoxel(Vector3i(10, 10, 10), VoxelResolution::Size_4cm, true));
    
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 2);
}