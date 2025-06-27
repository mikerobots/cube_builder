#include <gtest/gtest.h>
#include "core/voxel_data/VoxelDataManager.h"
#include "core/voxel_data/VoxelTypes.h"
#include "foundation/math/CoordinateConverter.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Vector3i.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class CoordinateConsistencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_voxelData = std::make_unique<VoxelDataManager>();
        m_workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
    }

    std::unique_ptr<VoxelDataManager> m_voxelData;
    Vector3f m_workspaceSize;
};

TEST_F(CoordinateConsistencyTest, VoxelAtOriginHasBottomAtY0) {
    // Place a 1cm voxel at origin (0,0,0)
    Vector3i pos(0, 0, 0);
    m_voxelData->setVoxel(pos, VoxelResolution::Size_1cm, true);
    
    // Create VoxelPosition and get world bounds
    VoxelPosition voxelPos(IncrementCoordinates(pos), VoxelResolution::Size_1cm);
    Vector3f minBounds, maxBounds;
    voxelPos.getWorldBounds(minBounds, maxBounds);
    
    // Verify bottom face is at Y=0
    EXPECT_FLOAT_EQ(minBounds.y, 0.0f) << "Bottom face should be at Y=0";
    EXPECT_FLOAT_EQ(maxBounds.y, 0.01f) << "Top face should be at Y=0.01 for 1cm voxel";
    
    // Verify the voxel exists at this position
    EXPECT_TRUE(m_voxelData->getVoxel(pos, VoxelResolution::Size_1cm));
}

TEST_F(CoordinateConsistencyTest, LargerVoxelBottomAlignment) {
    // Test with different resolutions
    std::vector<std::pair<VoxelResolution, float>> resolutions = {
        {VoxelResolution::Size_2cm, 0.02f},
        {VoxelResolution::Size_4cm, 0.04f},
        {VoxelResolution::Size_8cm, 0.08f},
        {VoxelResolution::Size_16cm, 0.16f},
        {VoxelResolution::Size_32cm, 0.32f}
    };
    
    for (const auto& [resolution, expectedHeight] : resolutions) {
        Vector3i pos(100, 0, 100); // Place away from origin to test general case
        m_voxelData->setVoxel(pos, resolution, true);
        
        VoxelPosition voxelPos(IncrementCoordinates(pos), resolution);
        Vector3f minBounds, maxBounds;
        voxelPos.getWorldBounds(minBounds, maxBounds);
        
        // Bottom should be at Y=0 (ground plane)
        EXPECT_FLOAT_EQ(minBounds.y, 0.0f) 
            << "Bottom face should be at Y=0 for resolution " << static_cast<int>(resolution);
        
        // Top should be at Y=voxelSize
        EXPECT_NEAR(maxBounds.y, expectedHeight, 0.0001f) 
            << "Top face should be at Y=" << expectedHeight << " for resolution " << static_cast<int>(resolution);
    }
}

TEST_F(CoordinateConsistencyTest, CollisionDetectionUsesBottomBasedBounds) {
    // Place a 4cm voxel at (0,0,0)
    Vector3i pos1(0, 0, 0);
    m_voxelData->setVoxel(pos1, VoxelResolution::Size_4cm, true);
    
    // Try to place another 4cm voxel that would overlap if using bottom-based bounds
    // If voxel at (0,0,0) has bottom at Y=0 and top at Y=4cm,
    // then voxel at (0,1,0) with bottom at Y=1cm would overlap
    Vector3i pos2(0, 1, 0); // Y=1cm in increments
    bool wouldOverlap = m_voxelData->wouldOverlap(pos2, VoxelResolution::Size_4cm);
    
    EXPECT_TRUE(wouldOverlap) << "Voxels should overlap when using bottom-based positioning";
    
    // Verify we can place at Y=4cm (just above the first voxel)
    Vector3i pos3(0, 4, 0); // Y=4cm in increments
    wouldOverlap = m_voxelData->wouldOverlap(pos3, VoxelResolution::Size_4cm);
    EXPECT_FALSE(wouldOverlap) << "Voxels should not overlap when placed exactly on top";
}

TEST_F(CoordinateConsistencyTest, PlacementValidationUsesBottomBasedChecks) {
    // Test boundary validation with bottom-based positioning
    // Workspace is 5m x 5m x 5m, so max Y in increments is 500
    
    // Try to place a 32cm voxel at Y=480cm (bottom at 4.8m)
    // This should be valid because top would be at 4.8m + 0.32m = 5.12m
    // But our workspace is only 5m tall, so this should be invalid
    Vector3i pos(0, 480, 0);
    Vector3f worldPos(0.0f, 4.8f, 0.0f);
    
    auto placementContext = VoxelEditor::Input::PlacementUtils::getPlacementContext(
        WorldCoordinates(worldPos), VoxelResolution::Size_32cm, false, m_workspaceSize);
    
    // The placement should be invalid because it would exceed workspace
    EXPECT_NE(placementContext.validation, VoxelEditor::Input::PlacementValidationResult::Valid) 
        << "32cm voxel with bottom at Y=4.8m should exceed 5m workspace";
    
    // Verify that Y=468cm is the highest valid position for a 32cm voxel
    // (468cm + 32cm = 500cm = 5m)
    pos = Vector3i(0, 468, 0);
    worldPos = Vector3f(0.0f, 4.68f, 0.0f);
    
    placementContext = VoxelEditor::Input::PlacementUtils::getPlacementContext(
        WorldCoordinates(worldPos), VoxelResolution::Size_32cm, false, m_workspaceSize);
    
    EXPECT_EQ(placementContext.validation, VoxelEditor::Input::PlacementValidationResult::Valid) 
        << "32cm voxel with bottom at Y=4.68m should fit in 5m workspace";
}

TEST_F(CoordinateConsistencyTest, VoxelBoundsCalculationConsistency) {
    // Test that VoxelTypes::getWorldBounds matches VoxelDataManager's internal calculations
    Vector3i incrementPos(50, 25, 75);
    
    // Get bounds from VoxelTypes
    VoxelPosition voxelPos(IncrementCoordinates(incrementPos), VoxelResolution::Size_8cm);
    Vector3f minBounds, maxBounds;
    voxelPos.getWorldBounds(minBounds, maxBounds);
    
    // Place voxel and verify collision detection uses same bounds
    m_voxelData->setVoxel(incrementPos, VoxelResolution::Size_8cm, true);
    
    // Try to place overlapping voxels at the bounds edges
    // Just below bottom face (should not overlap)
    Vector3i belowPos(50, 24, 75); // Y=24cm, just below Y=25cm
    EXPECT_FALSE(m_voxelData->wouldOverlap(belowPos, VoxelResolution::Size_1cm))
        << "1cm voxel just below should not overlap";
    
    // At bottom face (should overlap)
    Vector3i atBottomPos(50, 25, 75); // Same position
    EXPECT_TRUE(m_voxelData->wouldOverlap(atBottomPos, VoxelResolution::Size_1cm))
        << "1cm voxel at same position should overlap";
    
    // Just below top face (should overlap) 
    Vector3i belowTopPos(50, 32, 75); // Y=32cm, just below top at Y=33cm
    EXPECT_TRUE(m_voxelData->wouldOverlap(belowTopPos, VoxelResolution::Size_1cm))
        << "1cm voxel just below top should overlap";
    
    // At top face (should not overlap)
    Vector3i atTopPos(50, 33, 75); // Y=33cm, at top face
    EXPECT_FALSE(m_voxelData->wouldOverlap(atTopPos, VoxelResolution::Size_1cm))
        << "1cm voxel at top face should not overlap";
}

TEST_F(CoordinateConsistencyTest, CoordinateConverterAlignment) {
    // Test that coordinate converter properly handles bottom-based positioning
    Vector3i incrementPosVec(0, 0, 0);
    IncrementCoordinates incrementPos(incrementPosVec);
    
    // Convert to world position
    auto worldPos = CoordinateConverter::incrementToWorld(incrementPos);
    
    // For bottom-based positioning at origin, world position should be (0, 0, 0)
    EXPECT_FLOAT_EQ(worldPos.x(), 0.0f);
    EXPECT_FLOAT_EQ(worldPos.y(), 0.0f) << "Y should be 0 for bottom-based positioning";
    EXPECT_FLOAT_EQ(worldPos.z(), 0.0f);
    
    // Test round-trip conversion
    auto backToIncrement = CoordinateConverter::worldToIncrement(worldPos);
    EXPECT_EQ(backToIncrement.value(), incrementPosVec);
    
    // Test with non-zero position
    incrementPosVec = Vector3i(100, 50, 200);
    incrementPos = IncrementCoordinates(incrementPosVec);
    worldPos = CoordinateConverter::incrementToWorld(incrementPos);
    
    // World position should be increments * 0.01 (1cm per increment)
    EXPECT_FLOAT_EQ(worldPos.x(), 1.0f);  // 100 * 0.01
    EXPECT_FLOAT_EQ(worldPos.y(), 0.5f);  // 50 * 0.01
    EXPECT_FLOAT_EQ(worldPos.z(), 2.0f);  // 200 * 0.01
}

TEST_F(CoordinateConsistencyTest, MultiResolutionBottomAlignment) {
    // Test that voxels of different resolutions align properly at bottom
    // Place a 16cm voxel at origin
    Vector3i largeVoxelPos(0, 0, 0);
    m_voxelData->setVoxel(largeVoxelPos, VoxelResolution::Size_16cm, true);
    
    // Place 1cm voxels next to it - they should align at bottom
    for (int x = 16; x < 32; x++) {
        Vector3i smallVoxelPos(x, 0, 0);
        m_voxelData->setVoxel(smallVoxelPos, VoxelResolution::Size_1cm, true);
    }
    
    // Verify all voxels have bottom at Y=0
    VoxelPosition largeVoxelPosition(IncrementCoordinates(largeVoxelPos), VoxelResolution::Size_16cm);
    Vector3f largeMinBounds, largeMaxBounds;
    largeVoxelPosition.getWorldBounds(largeMinBounds, largeMaxBounds);
    EXPECT_FLOAT_EQ(largeMinBounds.y, 0.0f) << "16cm voxel bottom should be at Y=0";
    
    VoxelPosition smallVoxelPosition(IncrementCoordinates(Vector3i(16, 0, 0)), VoxelResolution::Size_1cm);
    Vector3f smallMinBounds, smallMaxBounds;
    smallVoxelPosition.getWorldBounds(smallMinBounds, smallMaxBounds);
    EXPECT_FLOAT_EQ(smallMinBounds.y, 0.0f) << "1cm voxel bottom should be at Y=0";
    
    // Both should have bottom at same Y coordinate
    EXPECT_FLOAT_EQ(largeMinBounds.y, smallMinBounds.y) 
        << "Voxels of different sizes should align at bottom";
}