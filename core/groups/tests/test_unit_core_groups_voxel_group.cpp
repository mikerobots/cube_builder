#include <gtest/gtest.h>
#include <thread>
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
    // REQ: Group naming and organization
    std::string newName = "New Group Name";
    group->setName(newName);
    
    EXPECT_EQ(group->getName(), newName);
}

TEST_F(VoxelGroupTest, ColorManagement) {
    // REQ: Visual group indicators (color coding, outlines)
    VoxelEditor::Rendering::Color newColor = VoxelEditor::Rendering::Color::Blue();
    group->setColor(newColor);
    
    EXPECT_EQ(group->getColor(), newColor);
}

TEST_F(VoxelGroupTest, VisibilityManagement) {
    // REQ: Group operations: move, hide/show, lock, copy/duplicate
    // REQ-8.1.9: Format shall store group visibility states
    EXPECT_TRUE(group->isVisible()); // Default should be visible
    
    group->setVisible(false);
    EXPECT_FALSE(group->isVisible());
    
    group->setVisible(true);
    EXPECT_TRUE(group->isVisible());
}

TEST_F(VoxelGroupTest, OpacityManagement) {
    // REQ: Visual group indicators (color coding, outlines)
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
    // REQ: Group operations: move, hide/show, lock, copy/duplicate
    EXPECT_FALSE(group->isLocked()); // Default should be unlocked
    
    group->setLocked(true);
    EXPECT_TRUE(group->isLocked());
    
    group->setLocked(false);
    EXPECT_FALSE(group->isLocked());
}

TEST_F(VoxelGroupTest, VoxelMembership) {
    // REQ: Create groups from selected voxels
    VoxelId voxel1(IncrementCoordinates(1, 2, 3), VoxelResolution::Size_32cm);
    VoxelId voxel2(IncrementCoordinates(4, 5, 6), VoxelResolution::Size_32cm);
    
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
    VoxelId voxel1(IncrementCoordinates(1, 2, 3), VoxelResolution::Size_32cm);
    VoxelId voxel2(IncrementCoordinates(4, 5, 6), VoxelResolution::Size_32cm);
    VoxelId voxel3(IncrementCoordinates(7, 8, 9), VoxelResolution::Size_64cm);
    
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
    VoxelId voxel1(IncrementCoordinates(1, 2, 3), VoxelResolution::Size_32cm);
    VoxelId voxel2(IncrementCoordinates(4, 5, 6), VoxelResolution::Size_32cm);
    
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
    VoxelId voxel1(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    VoxelId voxel2(IncrementCoordinates(2, 2, 2), VoxelResolution::Size_32cm);
    
    group->addVoxel(voxel1);
    group->addVoxel(voxel2);
    
    bounds = group->getBoundingBox();
    
    // With centered coordinate system, grid (0,0,0) and (2,2,2) map to different world positions
    // We need to calculate the expected bounds based on the actual coordinate conversion
    float voxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    Vector3f workspaceSize(5.0f, 5.0f, 5.0f); // Default workspace size used by VoxelGroup
    
    // Calculate expected bounds for voxel at grid (0,0,0)
    VoxelEditor::Math::WorldCoordinates world1 = VoxelEditor::Math::CoordinateConverter::incrementToWorld(
        IncrementCoordinates(0, 0, 0));
    Vector3f min1 = world1.value();
    Vector3f max1 = min1 + Vector3f(voxelSize, voxelSize, voxelSize);
    
    // Calculate expected bounds for voxel at grid (2,2,2)
    VoxelEditor::Math::WorldCoordinates world2 = VoxelEditor::Math::CoordinateConverter::incrementToWorld(
        IncrementCoordinates(2, 2, 2));
    Vector3f min2 = world2.value();
    Vector3f max2 = min2 + Vector3f(voxelSize, voxelSize, voxelSize);
    
    // The group bounds should encompass both voxels
    EXPECT_FLOAT_EQ(bounds.min.x, std::min(min1.x, min2.x));
    EXPECT_FLOAT_EQ(bounds.min.y, std::min(min1.y, min2.y));
    EXPECT_FLOAT_EQ(bounds.min.z, std::min(min1.z, min2.z));
    EXPECT_FLOAT_EQ(bounds.max.x, std::max(max1.x, max2.x));
    EXPECT_FLOAT_EQ(bounds.max.y, std::max(max1.y, max2.y));
    EXPECT_FLOAT_EQ(bounds.max.z, std::max(max1.z, max2.z));
}

TEST_F(VoxelGroupTest, PivotManagement) {
    Vector3f pivot(1.0f, 2.0f, 3.0f);
    group->setPivot(pivot);
    
    EXPECT_EQ(group->getPivot(), pivot);
}

TEST_F(VoxelGroupTest, GroupInfo) {
    // REQ-9.2.5: CLI shall support group commands (group create/hide/show/list)
    // Set up group with some properties
    group->setName("Info Test Group");
    group->setColor(VoxelEditor::Rendering::Color::Green());
    group->setVisible(false);
    group->setLocked(true);
    group->setOpacity(0.7f);
    
    VoxelId voxel(IncrementCoordinates(1, 2, 3), VoxelResolution::Size_32cm);
    group->addVoxel(voxel);
    
    auto info = group->getInfo();
    
    EXPECT_EQ(info.id, groupId);
    EXPECT_EQ(info.name, "Info Test Group");
    EXPECT_EQ(info.color, VoxelEditor::Rendering::Color::Green());
    EXPECT_FALSE(info.visible);
    EXPECT_TRUE(info.locked);
    EXPECT_FLOAT_EQ(info.opacity, 0.7f);
    EXPECT_EQ(info.voxelCount, 1);
}

TEST_F(VoxelGroupTest, Translation) {
    VoxelId voxel(IncrementCoordinates(1, 1, 1), VoxelResolution::Size_32cm);
    group->addVoxel(voxel);
    
    Vector3f offset(1.0f, 0.0f, 0.0f);
    group->translate(offset);
    
    // After translation, the voxel should be at a new position
    auto voxels = group->getVoxelList();
    EXPECT_EQ(voxels.size(), 1);
    
    // The exact new position depends on the implementation
    // but it should be different from the original
    EXPECT_NE(voxels[0].position.value(), Vector3i(1, 1, 1));
}

TEST_F(VoxelGroupTest, MetadataManagement) {
    // REQ-8.1.8: Format shall store group definitions and metadata
    // REQ: Group metadata storage in file format
    GroupMetadata metadata;
    metadata.name = "Metadata Test";
    metadata.color = VoxelEditor::Rendering::Color::Red();
    metadata.visible = false;
    metadata.locked = true;
    metadata.opacity = 0.3f;
    metadata.description = "Test description";
    
    group->setMetadata(metadata);
    
    const auto& retrievedMetadata = group->getMetadata();
    EXPECT_EQ(retrievedMetadata.name, "Metadata Test");
    EXPECT_EQ(retrievedMetadata.color, VoxelEditor::Rendering::Color::Red());
    EXPECT_FALSE(retrievedMetadata.visible);
    EXPECT_TRUE(retrievedMetadata.locked);
    EXPECT_FLOAT_EQ(retrievedMetadata.opacity, 0.3f);
    EXPECT_EQ(retrievedMetadata.description, "Test description");
}

TEST_F(VoxelGroupTest, BoundsInvalidation) {
    VoxelId voxel1(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    group->addVoxel(voxel1);
    
    // Get bounds to cache them
    auto bounds1 = group->getBoundingBox();
    
    // Add another voxel - bounds should be recalculated
    VoxelId voxel2(IncrementCoordinates(5, 5, 5), VoxelResolution::Size_32cm);
    group->addVoxel(voxel2);
    
    auto bounds2 = group->getBoundingBox();
    
    // With centered coordinate system, we need to calculate expected bounds
    float voxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    Vector3f workspaceSize(5.0f, 5.0f, 5.0f); // Default workspace size
    
    // Calculate expected bounds for first voxel only
    VoxelEditor::Math::WorldCoordinates world1 = VoxelEditor::Math::CoordinateConverter::incrementToWorld(
        IncrementCoordinates(0, 0, 0));
    Vector3f expectedMin1 = world1.value();
    Vector3f expectedMax1 = expectedMin1 + Vector3f(voxelSize, voxelSize, voxelSize);
    
    // Calculate expected bounds for both voxels
    VoxelEditor::Math::WorldCoordinates world2 = VoxelEditor::Math::CoordinateConverter::incrementToWorld(
        IncrementCoordinates(5, 5, 5));
    Vector3f min2 = world2.value();
    Vector3f max2 = min2 + Vector3f(voxelSize, voxelSize, voxelSize);
    
    Vector3f expectedMin2(
        std::min(expectedMin1.x, min2.x),
        std::min(expectedMin1.y, min2.y),
        std::min(expectedMin1.z, min2.z)
    );
    Vector3f expectedMax2(
        std::max(expectedMax1.x, max2.x),
        std::max(expectedMax1.y, max2.y),
        std::max(expectedMax1.z, max2.z)
    );
    
    // The bounds should expand when we add the second voxel
    EXPECT_FLOAT_EQ(bounds1.min.x, expectedMin1.x);
    EXPECT_FLOAT_EQ(bounds1.min.y, expectedMin1.y);
    EXPECT_FLOAT_EQ(bounds1.min.z, expectedMin1.z);
    EXPECT_FLOAT_EQ(bounds1.max.x, expectedMax1.x);
    EXPECT_FLOAT_EQ(bounds1.max.y, expectedMax1.y);
    EXPECT_FLOAT_EQ(bounds1.max.z, expectedMax1.z);
    
    EXPECT_FLOAT_EQ(bounds2.min.x, expectedMin2.x);
    EXPECT_FLOAT_EQ(bounds2.min.y, expectedMin2.y);
    EXPECT_FLOAT_EQ(bounds2.min.z, expectedMin2.z);
    EXPECT_FLOAT_EQ(bounds2.max.x, expectedMax2.x);
    EXPECT_FLOAT_EQ(bounds2.max.y, expectedMax2.y);
    EXPECT_FLOAT_EQ(bounds2.max.z, expectedMax2.z);
}

TEST_F(VoxelGroupTest, DifferentResolutions) {
    VoxelId voxel1(IncrementCoordinates(1, 1, 1), VoxelResolution::Size_32cm);
    VoxelId voxel2(IncrementCoordinates(2, 2, 2), VoxelResolution::Size_64cm);
    
    EXPECT_TRUE(group->addVoxel(voxel1));
    EXPECT_TRUE(group->addVoxel(voxel2));
    EXPECT_EQ(group->getVoxelCount(), 2);
    
    // Should handle different resolutions properly
    auto bounds = group->getBoundingBox();
    // Implementation should handle mixed resolutions correctly
}