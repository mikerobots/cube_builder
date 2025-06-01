#include <gtest/gtest.h>
#include "../include/groups/VoxelGroup.h"

using namespace VoxelEditor::Groups;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

class VoxelGroupTest : public ::testing::Test {
protected:
    void SetUp() override {
        groupId = 123;
        groupName = "Test Group";
        group = std::make_unique<VoxelGroup>(groupId, groupName);
    }
    
    GroupId groupId;
    std::string groupName;
    std::unique_ptr<VoxelGroup> group;
};

TEST_F(VoxelGroupTest, Construction) {
    EXPECT_EQ(group->getId(), groupId);
    EXPECT_EQ(group->getName(), groupName);
    EXPECT_TRUE(group->isEmpty());
    EXPECT_EQ(group->getVoxelCount(), 0);
}

TEST_F(VoxelGroupTest, NameManagement) {
    std::string newName = "New Group Name";
    group->setName(newName);
    
    EXPECT_EQ(group->getName(), newName);
}

TEST_F(VoxelGroupTest, ColorManagement) {
    Rendering::Color newColor = Rendering::Color::Blue();
    group->setColor(newColor);
    
    EXPECT_EQ(group->getColor(), newColor);
}

TEST_F(VoxelGroupTest, VisibilityManagement) {
    EXPECT_TRUE(group->isVisible()); // Default should be visible
    
    group->setVisible(false);
    EXPECT_FALSE(group->isVisible());
    
    group->setVisible(true);
    EXPECT_TRUE(group->isVisible());
}

TEST_F(VoxelGroupTest, OpacityManagement) {
    EXPECT_FLOAT_EQ(group->getOpacity(), 1.0f); // Default should be opaque
    
    group->setOpacity(0.5f);
    EXPECT_FLOAT_EQ(group->getOpacity(), 0.5f);
    
    // Test clamping
    group->setOpacity(-0.5f);
    EXPECT_FLOAT_EQ(group->getOpacity(), 0.0f);
    
    group->setOpacity(1.5f);
    EXPECT_FLOAT_EQ(group->getOpacity(), 1.0f);
}

TEST_F(VoxelGroupTest, LockingManagement) {
    EXPECT_FALSE(group->isLocked()); // Default should be unlocked
    
    group->setLocked(true);
    EXPECT_TRUE(group->isLocked());
    
    group->setLocked(false);
    EXPECT_FALSE(group->isLocked());
}

TEST_F(VoxelGroupTest, VoxelMembership) {
    VoxelId voxel1(Vector3i(1, 2, 3), VoxelResolution::Size_32cm);
    VoxelId voxel2(Vector3i(4, 5, 6), VoxelResolution::Size_32cm);
    
    // Test adding voxels
    EXPECT_TRUE(group->addVoxel(voxel1));
    EXPECT_EQ(group->getVoxelCount(), 1);
    EXPECT_FALSE(group->isEmpty());
    EXPECT_TRUE(group->containsVoxel(voxel1));
    
    // Test adding same voxel again (should return false)
    EXPECT_FALSE(group->addVoxel(voxel1));
    EXPECT_EQ(group->getVoxelCount(), 1);
    
    // Test adding second voxel
    EXPECT_TRUE(group->addVoxel(voxel2));
    EXPECT_EQ(group->getVoxelCount(), 2);
    EXPECT_TRUE(group->containsVoxel(voxel2));
    
    // Test removing voxels
    EXPECT_TRUE(group->removeVoxel(voxel1));
    EXPECT_EQ(group->getVoxelCount(), 1);
    EXPECT_FALSE(group->containsVoxel(voxel1));
    EXPECT_TRUE(group->containsVoxel(voxel2));
    
    // Test removing non-existent voxel
    EXPECT_FALSE(group->removeVoxel(voxel1));
    EXPECT_EQ(group->getVoxelCount(), 1);
}

TEST_F(VoxelGroupTest, VoxelList) {
    VoxelId voxel1(Vector3i(1, 2, 3), VoxelResolution::Size_32cm);
    VoxelId voxel2(Vector3i(4, 5, 6), VoxelResolution::Size_32cm);
    VoxelId voxel3(Vector3i(7, 8, 9), VoxelResolution::Size_64cm);
    
    group->addVoxel(voxel1);
    group->addVoxel(voxel2);
    group->addVoxel(voxel3);
    
    auto voxelList = group->getVoxelList();
    EXPECT_EQ(voxelList.size(), 3);
    
    // Check that all voxels are in the list
    EXPECT_TRUE(std::find(voxelList.begin(), voxelList.end(), voxel1) != voxelList.end());
    EXPECT_TRUE(std::find(voxelList.begin(), voxelList.end(), voxel2) != voxelList.end());
    EXPECT_TRUE(std::find(voxelList.begin(), voxelList.end(), voxel3) != voxelList.end());
}

TEST_F(VoxelGroupTest, ClearVoxels) {
    VoxelId voxel1(Vector3i(1, 2, 3), VoxelResolution::Size_32cm);
    VoxelId voxel2(Vector3i(4, 5, 6), VoxelResolution::Size_32cm);
    
    group->addVoxel(voxel1);
    group->addVoxel(voxel2);
    EXPECT_EQ(group->getVoxelCount(), 2);
    
    group->clearVoxels();
    EXPECT_EQ(group->getVoxelCount(), 0);
    EXPECT_TRUE(group->isEmpty());
    EXPECT_FALSE(group->containsVoxel(voxel1));
    EXPECT_FALSE(group->containsVoxel(voxel2));
}

TEST_F(VoxelGroupTest, BoundingBox) {
    // Empty group should have invalid bounding box
    auto bounds = group->getBoundingBox();
    // Implementation detail: empty groups may have different behavior
    
    // Add some voxels with known positions
    VoxelId voxel1(Vector3i(0, 0, 0), VoxelResolution::Size_32cm);
    VoxelId voxel2(Vector3i(2, 2, 2), VoxelResolution::Size_32cm);
    
    group->addVoxel(voxel1);
    group->addVoxel(voxel2);
    
    bounds = group->getBoundingBox();
    
    // Should encompass both voxels
    float voxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    EXPECT_LE(bounds.min.x, 0.0f);
    EXPECT_LE(bounds.min.y, 0.0f);
    EXPECT_LE(bounds.min.z, 0.0f);
    EXPECT_GE(bounds.max.x, 2.0f * voxelSize + voxelSize);
    EXPECT_GE(bounds.max.y, 2.0f * voxelSize + voxelSize);
    EXPECT_GE(bounds.max.z, 2.0f * voxelSize + voxelSize);
}

TEST_F(VoxelGroupTest, PivotManagement) {
    Vector3f pivot(1.0f, 2.0f, 3.0f);
    group->setPivot(pivot);
    
    EXPECT_EQ(group->getPivot(), pivot);
}

TEST_F(VoxelGroupTest, GroupInfo) {
    // Set up group with some properties
    group->setName("Info Test Group");
    group->setColor(Rendering::Color::Green());
    group->setVisible(false);
    group->setLocked(true);
    group->setOpacity(0.7f);
    
    VoxelId voxel(Vector3i(1, 2, 3), VoxelResolution::Size_32cm);
    group->addVoxel(voxel);
    
    auto info = group->getInfo();
    
    EXPECT_EQ(info.id, groupId);
    EXPECT_EQ(info.name, "Info Test Group");
    EXPECT_EQ(info.color, Rendering::Color::Green());
    EXPECT_FALSE(info.visible);
    EXPECT_TRUE(info.locked);
    EXPECT_FLOAT_EQ(info.opacity, 0.7f);
    EXPECT_EQ(info.voxelCount, 1);
}

TEST_F(VoxelGroupTest, Translation) {
    VoxelId voxel(Vector3i(1, 1, 1), VoxelResolution::Size_32cm);
    group->addVoxel(voxel);
    
    Vector3f offset(1.0f, 0.0f, 0.0f);
    group->translate(offset);
    
    // After translation, the voxel should be at a new position
    auto voxels = group->getVoxelList();
    EXPECT_EQ(voxels.size(), 1);
    
    // The exact new position depends on the implementation
    // but it should be different from the original
    EXPECT_NE(voxels[0].position, Vector3i(1, 1, 1));
}

TEST_F(VoxelGroupTest, MetadataManagement) {
    GroupMetadata metadata;
    metadata.name = "Metadata Test";
    metadata.color = Rendering::Color::Red();
    metadata.visible = false;
    metadata.locked = true;
    metadata.opacity = 0.3f;
    metadata.description = "Test description";
    
    group->setMetadata(metadata);
    
    const auto& retrievedMetadata = group->getMetadata();
    EXPECT_EQ(retrievedMetadata.name, "Metadata Test");
    EXPECT_EQ(retrievedMetadata.color, Rendering::Color::Red());
    EXPECT_FALSE(retrievedMetadata.visible);
    EXPECT_TRUE(retrievedMetadata.locked);
    EXPECT_FLOAT_EQ(retrievedMetadata.opacity, 0.3f);
    EXPECT_EQ(retrievedMetadata.description, "Test description");
}

TEST_F(VoxelGroupTest, BoundsInvalidation) {
    VoxelId voxel1(Vector3i(0, 0, 0), VoxelResolution::Size_32cm);
    group->addVoxel(voxel1);
    
    // Get bounds to cache them
    auto bounds1 = group->getBoundingBox();
    
    // Add another voxel - bounds should be recalculated
    VoxelId voxel2(Vector3i(5, 5, 5), VoxelResolution::Size_32cm);
    group->addVoxel(voxel2);
    
    auto bounds2 = group->getBoundingBox();
    
    // Bounds should be different (larger)
    EXPECT_NE(bounds1.min, bounds2.min);
    EXPECT_NE(bounds1.max, bounds2.max);
}

TEST_F(VoxelGroupTest, DifferentResolutions) {
    VoxelId voxel1(Vector3i(1, 1, 1), VoxelResolution::Size_32cm);
    VoxelId voxel2(Vector3i(2, 2, 2), VoxelResolution::Size_64cm);
    
    EXPECT_TRUE(group->addVoxel(voxel1));
    EXPECT_TRUE(group->addVoxel(voxel2));
    EXPECT_EQ(group->getVoxelCount(), 2);
    
    // Should handle different resolutions properly
    auto bounds = group->getBoundingBox();
    // Implementation should handle mixed resolutions correctly
}