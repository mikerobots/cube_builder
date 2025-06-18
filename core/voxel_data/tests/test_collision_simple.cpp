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
    
    // Place a large voxel at the origin
    VoxelEditor::Math::Vector3f voxelPos(0.0f, 0.0f, 0.0f);
    ASSERT_TRUE(manager.setVoxelAtWorldPos(voxelPos, VoxelResolution::Size_16cm, true));
    
    // Try to place a small voxel at the same position - should fail (overlap)
    EXPECT_FALSE(manager.setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true));
    
    // Try to place small voxels far away - should succeed (no overlap)
    EXPECT_TRUE(manager.setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(1.0f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true));
    EXPECT_TRUE(manager.setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.0f, 0.0f, 1.0f), VoxelResolution::Size_1cm, true));
    EXPECT_TRUE(manager.setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(-1.0f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true));
}