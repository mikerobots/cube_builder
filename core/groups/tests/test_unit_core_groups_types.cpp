#include <gtest/gtest.h>
#include <thread>
#include "../include/groups/GroupTypes.h"

using namespace VoxelEditor::Groups;
namespace Math = VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using VoxelEditor::Math::Vector3f;
using VoxelEditor::Math::Vector3i;

class GroupTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test setup
    }
};

TEST_F(GroupTypesTest, VoxelIdConstruction) {
    Vector3i position(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    VoxelId voxel(position, resolution);
    
    EXPECT_EQ(voxel.position.value(), position);
    EXPECT_EQ(voxel.resolution, resolution);
}

TEST_F(GroupTypesTest, VoxelIdEquality) {
    VoxelId voxel1(Vector3i(1, 2, 3), VoxelResolution::Size_32cm);
    VoxelId voxel2(Vector3i(1, 2, 3), VoxelResolution::Size_32cm);
    VoxelId voxel3(Vector3i(1, 2, 4), VoxelResolution::Size_32cm);
    VoxelId voxel4(Vector3i(1, 2, 3), VoxelResolution::Size_64cm);
    
    EXPECT_EQ(voxel1, voxel2);
    EXPECT_NE(voxel1, voxel3);
    EXPECT_NE(voxel1, voxel4);
}

TEST_F(GroupTypesTest, VoxelIdHash) {
    VoxelId voxel1(Vector3i(1, 2, 3), VoxelResolution::Size_32cm);
    VoxelId voxel2(Vector3i(1, 2, 3), VoxelResolution::Size_32cm);
    VoxelId voxel3(Vector3i(1, 2, 4), VoxelResolution::Size_32cm);
    
    EXPECT_EQ(voxel1.hash(), voxel2.hash());
    EXPECT_NE(voxel1.hash(), voxel3.hash());
    
    // Test std::hash compatibility
    std::hash<VoxelId> hasher;
    EXPECT_EQ(hasher(voxel1), hasher(voxel2));
    EXPECT_NE(hasher(voxel1), hasher(voxel3));
}

TEST_F(GroupTypesTest, GroupMetadataConstruction) {
    // REQ-8.1.8: Format shall store group definitions and metadata
    // REQ: Group metadata storage in file format
    GroupMetadata metadata;
    
    EXPECT_TRUE(metadata.name.empty());
    EXPECT_TRUE(metadata.visible);
    EXPECT_FALSE(metadata.locked);
    EXPECT_FLOAT_EQ(metadata.opacity, 1.0f);
    EXPECT_TRUE(metadata.pivot.isEqualTo(Vector3f(0, 0, 0)));
    
    // Check that timestamps are set
    EXPECT_NE(metadata.created.time_since_epoch().count(), 0);
    EXPECT_NE(metadata.modified.time_since_epoch().count(), 0);
}

TEST_F(GroupTypesTest, GroupMetadataUpdateModified) {
    // REQ: Group metadata storage in file format
    GroupMetadata metadata;
    auto initialModified = metadata.modified;
    
    // Small delay to ensure time difference
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    metadata.updateModified();
    
    EXPECT_GT(metadata.modified, initialModified);
}

TEST_F(GroupTypesTest, GroupInfoConstruction) {
    // REQ-9.2.5: CLI shall support group commands (group create/hide/show/list)
    GroupId id = 123;
    GroupMetadata metadata;
    metadata.name = "Test Group";
    metadata.color = VoxelEditor::Rendering::Color::Red();
    metadata.visible = false;
    metadata.locked = true;
    metadata.opacity = 0.5f;
    
    GroupInfo info(id, metadata);
    
    EXPECT_EQ(info.id, id);
    EXPECT_EQ(info.name, "Test Group");
    EXPECT_EQ(info.color, VoxelEditor::Rendering::Color::Red());
    EXPECT_FALSE(info.visible);
    EXPECT_TRUE(info.locked);
    EXPECT_FLOAT_EQ(info.opacity, 0.5f);
    EXPECT_EQ(info.voxelCount, 0);
    EXPECT_EQ(info.parentId, INVALID_GROUP_ID);
    EXPECT_TRUE(info.childIds.empty());
}

TEST_F(GroupTypesTest, GroupTransformConstruction) {
    GroupTransform transform1;
    EXPECT_TRUE(transform1.translation.isEqualTo(Vector3f(0, 0, 0)));
    EXPECT_EQ(transform1.rotation, Vector3f(0, 0, 0));
    EXPECT_EQ(transform1.scale, Vector3f(1, 1, 1));
    EXPECT_TRUE(transform1.isIdentity());
    
    Vector3f translation(1, 2, 3);
    Math::WorldCoordinates worldTranslation(translation);
    GroupTransform transform2(worldTranslation);
    EXPECT_TRUE(transform2.translation.isEqualTo(translation));
    EXPECT_FALSE(transform2.isIdentity());
}

TEST_F(GroupTypesTest, GroupTransformIdentity) {
    GroupTransform identity;
    EXPECT_TRUE(identity.isIdentity());
    
    GroupTransform nonIdentity;
    nonIdentity.translation = Math::WorldCoordinates(Vector3f(0.1f, 0, 0));
    EXPECT_FALSE(nonIdentity.isIdentity());
    
    nonIdentity.translation = Math::WorldCoordinates(Vector3f(0, 0, 0));
    nonIdentity.rotation = Vector3f(0.1f, 0, 0);
    EXPECT_FALSE(nonIdentity.isIdentity());
    
    nonIdentity.rotation = Vector3f(0, 0, 0);
    nonIdentity.scale = Vector3f(1.1f, 1, 1);
    EXPECT_FALSE(nonIdentity.isIdentity());
}

TEST_F(GroupTypesTest, GroupColorPalette) {
    // REQ: Visual group indicators (color coding, outlines)
    const auto& palette = GroupColorPalette::getDefaultPalette();
    
    EXPECT_FALSE(palette.empty());
    EXPECT_GE(palette.size(), 5); // Should have at least a few colors
    
    // Test color retrieval by index
    auto color1 = GroupColorPalette::getColorForIndex(0);
    auto color2 = GroupColorPalette::getColorForIndex(palette.size());
    EXPECT_EQ(color1, color2); // Should wrap around
    
    // Test random color
    auto randomColor = GroupColorPalette::getRandomColor();
    bool found = false;
    for (const auto& paletteColor : palette) {
        if (randomColor.r == paletteColor.r && 
            randomColor.g == paletteColor.g && 
            randomColor.b == paletteColor.b) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(GroupTypesTest, GroupStatsDefault) {
    GroupStats stats;
    
    EXPECT_EQ(stats.totalGroups, 0);
    EXPECT_EQ(stats.totalVoxels, 0);
    EXPECT_EQ(stats.maxGroupSize, 0);
    EXPECT_EQ(stats.maxHierarchyDepth, 0);
    EXPECT_FLOAT_EQ(stats.averageGroupSize, 0.0f);
    EXPECT_EQ(stats.memoryUsage, 0);
}

TEST_F(GroupTypesTest, GroupModificationTypes) {
    // Test that all modification types are defined
    GroupModificationType type = GroupModificationType::Created;
    EXPECT_EQ(type, GroupModificationType::Created);
    
    type = GroupModificationType::Deleted;
    EXPECT_EQ(type, GroupModificationType::Deleted);
    
    type = GroupModificationType::Moved;
    EXPECT_EQ(type, GroupModificationType::Moved);
    
    type = GroupModificationType::VisibilityChanged;
    EXPECT_EQ(type, GroupModificationType::VisibilityChanged);
}

TEST_F(GroupTypesTest, InvalidGroupId) {
    EXPECT_EQ(INVALID_GROUP_ID, 0);
    
    GroupId validId = 1;
    EXPECT_NE(validId, INVALID_GROUP_ID);
}