#include <gtest/gtest.h>
#include "../VoxelDataManager.h"

using namespace VoxelEditor::VoxelData;
// Don't import Math namespace to avoid VoxelResolution ambiguity
// using namespace VoxelEditor::Math;

TEST(CollisionSimple, BasicOverlap) {
    VoxelDataManager manager;
    
    // Place a 1cm voxel at a specific world position
    VoxelEditor::Math::Vector3f pos1(0.1f, 0.0f, 0.1f);
    ASSERT_TRUE(manager.setVoxelAtWorldPos(pos1, VoxelResolution::Size_1cm, true));
    
    // Try to place another 1cm voxel at the same position - should fail
    EXPECT_FALSE(manager.setVoxelAtWorldPos(pos1, VoxelResolution::Size_1cm, true));
    
    // Try to place a 1cm voxel at a different position - should succeed
    VoxelEditor::Math::Vector3f pos2(0.11f, 0.0f, 0.1f);
    EXPECT_TRUE(manager.setVoxelAtWorldPos(pos2, VoxelResolution::Size_1cm, true));
}

TEST(CollisionSimple, DifferentSizeOverlap) {
    VoxelDataManager manager;
    
    // When we place a 16cm voxel at world (0.16, 0, 0.16), it snaps to its grid
    // Grid pos (16,0,16) in 16cm grid -> world (0.06, 0, 0.06) to (0.22, 0, 0.22)
    VoxelEditor::Math::Vector3f voxelPos(0.16f, 0.0f, 0.16f);
    ASSERT_TRUE(manager.setVoxelAtWorldPos(voxelPos, VoxelResolution::Size_16cm, true));
    
    // Try to place 1cm voxels at various positions
    // These should fail (inside the 16cm voxel which occupies 0.06 to 0.22)
    EXPECT_FALSE(manager.setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.06f, 0.0f, 0.06f), VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(manager.setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.21f, 0.0f, 0.21f), VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(manager.setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.10f, 0.0f, 0.10f), VoxelResolution::Size_1cm, true));
    
    // These should succeed (outside the 16cm voxel)
    EXPECT_TRUE(manager.setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.05f, 0.0f, 0.06f), VoxelResolution::Size_1cm, true));
    EXPECT_TRUE(manager.setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.22f, 0.0f, 0.06f), VoxelResolution::Size_1cm, true));
    EXPECT_TRUE(manager.setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.06f, 0.0f, 0.22f), VoxelResolution::Size_1cm, true));
}