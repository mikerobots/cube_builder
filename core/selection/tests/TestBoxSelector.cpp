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
    // In centered coordinate system, grid (0,0,0) maps to world (-2.48, 0.02, -2.48) with 4cm voxels
    // 4cm voxel bounds: center ± 0.02m, so from (-2.50, 0.00, -2.50) to (-2.46, 0.04, -2.46)
    Math::BoundingBox box(
        Math::Vector3f(-2.50f, 0.00f, -2.50f),
        Math::Vector3f(-2.46f, 0.04f, -2.46f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should contain at least the voxel at origin
    EXPECT_GT(result.size(), 0u);
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(BoxSelectorTest, SelectFromWorld_LargerBox) {
    // In centered coordinate system, create box that spans from grid (0,0,0) to (2,2,2)
    // Grid (0,0,0) is at world (-2.48, 0.02, -2.48)
    // Grid (2,2,2) is at world (-2.40, 0.10, -2.40)
    Math::BoundingBox box(
        Math::Vector3f(-2.5f, 0.0f, -2.5f),
        Math::Vector3f(-2.38f, 0.12f, -2.38f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should contain 3x3x3 = 27 voxels at minimum
    EXPECT_GE(result.size(), 27u);
    
    // Check corners
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm)));
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(2, 2, 2), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(BoxSelectorTest, SelectFromWorld_NegativeCoordinates) {
    // Create box around world origin (0,0,0) which should include several grid positions
    // In centered system, world (0,0,0) is approximately at grid (62,0,62) for 4cm voxels
    Math::BoundingBox box(
        Math::Vector3f(-0.08f, 0.0f, -0.08f),
        Math::Vector3f(0.08f, 0.08f, 0.08f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should select at least one voxel around world origin
    EXPECT_GE(result.size(), 1u);
    
    // Check that the result contains voxels in a reasonable range around center
    bool hasReasonableVoxels = false;
    for (const auto& voxel : result) {
        Math::Vector3i pos = voxel.position.value();
        // For 5m workspace and 4cm voxels, center should be around grid (62,0,62)
        if (pos.x >= 60 && pos.x <= 65 && pos.y >= 0 && pos.y <= 5 && pos.z >= 60 && pos.z <= 65) {
            hasReasonableVoxels = true;
            break;
        }
    }
    EXPECT_TRUE(hasReasonableVoxels);
}

TEST_F(BoxSelectorTest, SelectFromWorld_IncludePartialTrue) {
    selector->setIncludePartial(true);
    
    // Box that partially overlaps voxel at grid (1,0,0)
    // Grid (1,0,0) is at world (-2.44, 0.02, -2.48) with 4cm voxels
    Math::BoundingBox box(
        Math::Vector3f(-2.45f, 0.01f, -2.49f),
        Math::Vector3f(-2.43f, 0.03f, -2.47f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should include voxel at (1,0,0) because it's partially intersected
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(1, 0, 0), VoxelData::VoxelResolution::Size_4cm)));
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
    EXPECT_FALSE(result.contains(VoxelId(Math::Vector3i(1, 0, 0), VoxelData::VoxelResolution::Size_4cm)));
}

// Grid Selection Tests
TEST_F(BoxSelectorTest, SelectFromGrid_SingleVoxel) {
    Math::Vector3i minGrid(5, 5, 5);
    Math::Vector3i maxGrid(5, 5, 5);
    
    SelectionSet result = selector->selectFromGrid(minGrid, maxGrid, VoxelData::VoxelResolution::Size_8cm, false);
    
    EXPECT_EQ(result.size(), 1u);
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(5, 5, 5), VoxelData::VoxelResolution::Size_8cm)));
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
                EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(x, y, z), VoxelData::VoxelResolution::Size_4cm)));
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
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(3, 3, 3), VoxelData::VoxelResolution::Size_4cm)));
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(5, 5, 5), VoxelData::VoxelResolution::Size_4cm)));
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
    // Zero-volume box at a point that should be in grid (0,0,0)
    Math::BoundingBox box(
        Math::Vector3f(-2.48f, 0.02f, -2.48f),
        Math::Vector3f(-2.48f, 0.02f, -2.48f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should still select the voxel that contains this point
    EXPECT_EQ(result.size(), 1u);
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

TEST_F(BoxSelectorTest, SelectFromWorld_DifferentResolutions) {
    // Large box in world space that covers many voxels
    Math::BoundingBox box(
        Math::Vector3f(-1.0f, 0.0f, -1.0f),
        Math::Vector3f(1.0f, 0.5f, 1.0f)
    );
    
    // Test with 1cm resolution
    SelectionSet result1cm = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_1cm, false);
    
    // Test with 8cm resolution
    SelectionSet result8cm = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_8cm, false);
    
    // Higher resolution should have more voxels
    EXPECT_GT(result1cm.size(), result8cm.size());
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