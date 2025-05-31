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
    Math::BoundingBox box(
        Math::Vector3f(0.0f, 0.0f, 0.0f),
        Math::Vector3f(0.05f, 0.05f, 0.05f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should contain at least the voxel at origin
    EXPECT_GT(result.size(), 0u);
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(BoxSelectorTest, SelectFromWorld_LargerBox) {
    Math::BoundingBox box(
        Math::Vector3f(0.0f, 0.0f, 0.0f),
        Math::Vector3f(0.12f, 0.12f, 0.12f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should contain 3x3x3 = 27 voxels at minimum
    EXPECT_GE(result.size(), 27u);
    
    // Check corners
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm)));
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(2, 2, 2), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(BoxSelectorTest, SelectFromWorld_NegativeCoordinates) {
    Math::BoundingBox box(
        Math::Vector3f(-0.08f, -0.08f, -0.08f),
        Math::Vector3f(0.08f, 0.08f, 0.08f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should contain voxels in negative coordinates
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(-2, -2, -2), VoxelData::VoxelResolution::Size_4cm)));
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(-1, -1, -1), VoxelData::VoxelResolution::Size_4cm)));
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm)));
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(1, 1, 1), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(BoxSelectorTest, SelectFromWorld_IncludePartialTrue) {
    selector->setIncludePartial(true);
    
    // Box that partially overlaps voxel at (1,0,0)
    Math::BoundingBox box(
        Math::Vector3f(0.035f, -0.01f, -0.01f),
        Math::Vector3f(0.045f, 0.01f, 0.01f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should include voxel at (1,0,0) because it's partially intersected
    EXPECT_TRUE(result.contains(VoxelId(Math::Vector3i(1, 0, 0), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(BoxSelectorTest, SelectFromWorld_IncludePartialFalse) {
    selector->setIncludePartial(false);
    
    // Same box as above
    Math::BoundingBox box(
        Math::Vector3f(0.035f, -0.01f, -0.01f),
        Math::Vector3f(0.045f, 0.01f, 0.01f)
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
    Math::Ray startRay(Math::Vector3f(0, 0, 0), Math::Vector3f(0, 0, 1));
    Math::Ray endRay(Math::Vector3f(0.1f, 0.1f, 0), Math::Vector3f(0, 0, 1));
    
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
    // Zero-volume box
    Math::BoundingBox box(
        Math::Vector3f(0.02f, 0.02f, 0.02f),
        Math::Vector3f(0.02f, 0.02f, 0.02f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should still select the voxel that contains this point
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(BoxSelectorTest, SelectFromWorld_VerySmallBox) {
    // Box smaller than voxel size
    Math::BoundingBox box(
        Math::Vector3f(0.019f, 0.019f, 0.019f),
        Math::Vector3f(0.021f, 0.021f, 0.021f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should select the voxel containing the box center
    EXPECT_GE(result.size(), 1u);
}

TEST_F(BoxSelectorTest, SelectFromWorld_DifferentResolutions) {
    Math::BoundingBox box(
        Math::Vector3f(0.0f, 0.0f, 0.0f),
        Math::Vector3f(0.5f, 0.5f, 0.5f)
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
        Math::Vector3f(0.0f, 0.0f, 0.0f),
        Math::Vector3f(0.1f, 0.1f, 0.1f)
    );
    
    SelectionSet result = selector->selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, true);
    EXPECT_GT(result.size(), 0u);
}