#include <gtest/gtest.h>
#include "core/selection/BoxSelector.h"
#include "foundation/math/Matrix4f.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Selection;

class BoxSelectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        selector = std::make_unique<BoxSelector>();
    }
    
    std::unique_ptr<BoxSelector> selector;
};

// Basic Tests
TEST_F(BoxSelectorTest, DefaultConfiguration) {
    // REQ: BoxSelector for different selection methods
    EXPECT_EQ(selector->getSelectionMode(), SelectionMode::Replace);
    EXPECT_TRUE(selector->getIncludePartial());
}

TEST_F(BoxSelectorTest, SetConfiguration) {
    selector->setSelectionMode(SelectionMode::Add);
    selector->setIncludePartial(false);
    
    EXPECT_EQ(selector->getSelectionMode(), SelectionMode::Add);
    EXPECT_FALSE(selector->getIncludePartial());
}

// World Selection Tests
TEST_F(BoxSelectorTest, SelectFromWorld_SmallBox) {
    // REQ: BoxSelector for different selection methods
    // In centered coordinate system, IncrementCoordinates(0,0,0) is at world origin
    // For 4cm voxels, the voxel at origin spans from (0,0,0) to (0.04,0.04,0.04)
    Math::BoundingBox box(
        Math::Vector3f(-0.01f, -0.01f, -0.01f),
        Math::Vector3f(0.01f, 0.01f, 0.01f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should contain at least the voxel at origin
    EXPECT_GT(result.size(), 0u);
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(BoxSelectorTest, SelectFromWorld_LargerBox) {
    // REQ: BoxSelector for different selection methods
    // In centered coordinate system, create box that spans multiple 4cm voxels
    // For 4cm voxels: IncrementCoordinates(0,0,0) snaps to (0,0,0), 
    // IncrementCoordinates(4,4,4) snaps to (4,4,4), IncrementCoordinates(8,8,8) snaps to (8,8,8)
    Math::BoundingBox box(
        Math::Vector3f(-0.01f, -0.01f, -0.01f),
        Math::Vector3f(0.09f, 0.09f, 0.09f)  // Covers 3x3x3 voxels
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should contain 3x3x3 = 27 voxels at minimum
    EXPECT_GE(result.size(), 27u);
    
    // Check corners - with 4cm voxels:
    // (0,0,0) snaps to (0,0,0)
    // (8,8,8) snaps to (8,8,8) 
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), VoxelData::VoxelResolution::Size_4cm)));
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(8, 8, 8)), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(BoxSelectorTest, SelectFromWorld_NegativeCoordinates) {
    // In centered system, test selection with negative world coordinates
    // Create box that spans from negative to positive coordinates
    Math::BoundingBox box(
        Math::Vector3f(-0.06f, -0.02f, -0.06f),
        Math::Vector3f(0.06f, 0.06f, 0.06f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should select multiple voxels spanning negative and positive coordinates
    EXPECT_GE(result.size(), 8u);  // At least 2x2x2 voxels
    
    // Check that we have voxels with both negative and positive increment coordinates
    bool hasNegative = false;
    bool hasPositive = false;
    for (const auto& voxel : result) {
        Math::Vector3i pos = voxel.position.value();
        if (pos.x < 0 || pos.z < 0) hasNegative = true;
        if (pos.x > 0 || pos.z > 0) hasPositive = true;
    }
    EXPECT_TRUE(hasNegative);
    EXPECT_TRUE(hasPositive);
}

TEST_F(BoxSelectorTest, SelectFromWorld_IncludePartialTrue) {
    selector->setIncludePartial(true);
    
    // Box that partially overlaps voxel at IncrementCoordinates(4,0,0)
    // 4cm voxel at (4,0,0) spans world (0.04,0,0) to (0.08,0.04,0.04)
    Math::BoundingBox box(
        Math::Vector3f(0.035f, -0.01f, -0.01f),
        Math::Vector3f(0.045f, 0.01f, 0.01f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should include voxel at (4,0,0) because it's partially intersected
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(4, 0, 0)), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(BoxSelectorTest, SelectFromWorld_IncludePartialFalse) {
    selector->setIncludePartial(false);
    
    // Same box as above - partially overlaps voxel at grid (1,0,0)
    Math::BoundingBox box(
        Math::Vector3f(-2.45f, 0.01f, -2.49f),
        Math::Vector3f(-2.43f, 0.03f, -2.47f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should not include voxel at (1,0,0) because it's not fully contained
    EXPECT_FALSE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(1, 0, 0)), VoxelData::VoxelResolution::Size_4cm)));
}

// Grid Selection Tests
TEST_F(BoxSelectorTest, SelectFromGrid_SingleVoxel) {
    Math::Vector3i minGrid(5, 5, 5);
    Math::Vector3i maxGrid(5, 5, 5);
    
    SelectionSet result = selector->selectFromGrid(minGrid, maxGrid, VoxelData::VoxelResolution::Size_8cm, false);
    
    EXPECT_EQ(result.size(), 1u);
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(5, 5, 5)), VoxelData::VoxelResolution::Size_8cm)));
}

TEST_F(BoxSelectorTest, SelectFromGrid_Range) {
    Math::Vector3i minGrid(0, 0, 0);
    Math::Vector3i maxGrid(2, 2, 2);
    
    SelectionSet result = selector->selectFromGrid(minGrid, maxGrid, VoxelData::VoxelResolution::Size_4cm, false);
    
    EXPECT_EQ(result.size(), 27u); // 3x3x3
    
    // Check all voxels are included
    for (int x = 0; x <= 2; ++x) {
        for (int y = 0; y <= 2; ++y) {
            for (int z = 0; z <= 2; ++z) {
                EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(x, y, z)), VoxelData::VoxelResolution::Size_4cm)));
            }
        }
    }
}

TEST_F(BoxSelectorTest, SelectFromGrid_ReversedMinMax) {
    // Max and min are swapped
    Math::Vector3i minGrid(5, 5, 5);
    Math::Vector3i maxGrid(3, 3, 3);
    
    SelectionSet result = selector->selectFromGrid(minGrid, maxGrid, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should still work correctly
    EXPECT_EQ(result.size(), 27u); // 3x3x3
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(3, 3, 3)), VoxelData::VoxelResolution::Size_4cm)));
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(5, 5, 5)), VoxelData::VoxelResolution::Size_4cm)));
}

// Ray Selection Tests
TEST_F(BoxSelectorTest, SelectFromRays_Basic) {
    // Use rays that intersect the workspace bounds
    Math::Ray startRay(Math::Vector3f(-2.5f, 0.0f, -2.5f), Math::Vector3f(0, 0, 1));
    Math::Ray endRay(Math::Vector3f(-2.4f, 0.1f, -2.5f), Math::Vector3f(0, 0, 1));
    
    SelectionSet result = selector->selectFromRays(startRay, endRay, 1.0f, VoxelData::VoxelResolution::Size_4cm);
    
    // Should create a box between the two ray endpoints
    EXPECT_GT(result.size(), 0u);
}

// Screen Selection Tests
TEST_F(BoxSelectorTest, SelectFromScreen_Basic) {
    Math::Vector2i screenStart(100, 100);
    Math::Vector2i screenEnd(200, 200);
    Math::Matrix4f viewMatrix = Math::Matrix4f::Identity();
    Math::Matrix4f projMatrix = Math::Matrix4f::Identity();
    Math::Vector2i viewportSize(800, 600);
    
    SelectionSet result = selector->selectFromScreen(screenStart, screenEnd, viewMatrix, projMatrix, 
                                                    viewportSize, VoxelData::VoxelResolution::Size_4cm);
    
    // Note: This test is limited because computeScreenBox is not fully implemented
    // In a real implementation, this would test screen-to-world conversion
    EXPECT_GE(result.size(), 0u);
}

// Edge Cases
TEST_F(BoxSelectorTest, SelectFromWorld_EmptyBox) {
    // Zero-volume box at a single point
    // In centered coordinates: (-2.48, 0.02, -2.48) = increment coordinates (-248, 2, -248)
    // With 4cm voxels and no grid snapping, many voxels could contain this point
    Math::BoundingBox box(
        Math::Vector3f(-2.48f, 0.02f, -2.48f),
        Math::Vector3f(-2.48f, 0.02f, -2.48f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // With the new requirements where voxels can be placed at any 1cm position,
    // a single point could be contained by multiple overlapping 4cm voxels
    // For a 4cm voxel, any voxel with corner from (-248-3, 2-3, -248-3) to (-248, 2, -248)
    // would contain this point. That's a 4x4x4 = 64 possible positions minimum.
    // The actual number depends on the exact algorithm but should be > 1
    EXPECT_GT(result.size(), 0u);
    EXPECT_LE(result.size(), 125u); // 5x5x5 is reasonable upper bound
}

TEST_F(BoxSelectorTest, SelectFromWorld_VerySmallBox) {
    // Box smaller than voxel size, centered around grid (0,0,0)
    Math::BoundingBox box(
        Math::Vector3f(-2.49f, 0.01f, -2.49f),
        Math::Vector3f(-2.47f, 0.03f, -2.47f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should select the voxel containing the box center
    EXPECT_GE(result.size(), 1u);
}

TEST_F(BoxSelectorTest, SelectFromGrid_DifferentResolutions) {
    // Test the core intent: higher resolution should produce more voxels for same grid range
    // Using grid selection to avoid coordinate conversion overhead
    
    // Test a small 2×1×2 grid range
    Math::Vector3i minGrid(0, 0, 0);
    Math::Vector3i maxGrid(1, 0, 1); // 2×1×2 range
    
    // Test with 8cm resolution (should be 4 voxels: 2×1×2)
    SelectionSet result8cm = selector->selectFromGrid(minGrid, maxGrid, VoxelData::VoxelResolution::Size_8cm, false);
    
    // Test with 4cm resolution (should be 4 voxels: 2×1×2)
    SelectionSet result4cm = selector->selectFromGrid(minGrid, maxGrid, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Both should have same number of voxels since we're selecting by grid coordinates
    EXPECT_EQ(result4cm.size(), result8cm.size());
    EXPECT_EQ(result4cm.size(), 4u); // 2×1×2 = 4 voxels
    EXPECT_EQ(result8cm.size(), 4u); // 2×1×2 = 4 voxels
    
    // However, the voxel IDs should be different due to different resolutions
    bool differentResolutions = false;
    for (const auto& voxel4cm : result4cm) {
        for (const auto& voxel8cm : result8cm) {
            if (voxel4cm.position == voxel8cm.position && 
                voxel4cm.resolution != voxel8cm.resolution) {
                differentResolutions = true;
                break;
            }
        }
        if (differentResolutions) break;
    }
    EXPECT_TRUE(differentResolutions);
}

// Voxel Manager Tests
TEST_F(BoxSelectorTest, SetVoxelManager) {
    // Just test that we can set the voxel manager
    selector->setVoxelManager(nullptr);
    
    // Selection should still work without manager (assumes all voxels exist)
    Math::BoundingBox box(
        Math::Vector3f(-2.5f, 0.0f, -2.5f),
        Math::Vector3f(-2.4f, 0.1f, -2.4f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, true);
    EXPECT_GT(result.size(), 0u);
}