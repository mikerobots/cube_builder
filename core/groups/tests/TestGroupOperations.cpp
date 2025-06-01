#include <gtest/gtest.h>
#include "../include/groups/GroupOperations.h"
#include "../include/groups/GroupManager.h"
#include "../include/groups/VoxelGroup.h"
#include <VoxelDataManager.h>

using namespace VoxelEditor::Groups;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

// Mock VoxelDataManager for testing
class MockVoxelDataManager : public VoxelDataManager {
public:
    MockVoxelDataManager() : VoxelDataManager(nullptr, nullptr) {}
    
    bool hasVoxel(const Vector3i& position, VoxelResolution resolution) const override {
        VoxelId id{position, resolution};
        return m_voxels.find(id) != m_voxels.end();
    }
    
    Rendering::Color getVoxel(const Vector3i& position, VoxelResolution resolution) const override {
        VoxelId id{position, resolution};
        auto it = m_voxels.find(id);
        return (it != m_voxels.end()) ? it->second : Rendering::Color::Black();
    }
    
    bool setVoxel(const Vector3i& position, VoxelResolution resolution, 
                 const Rendering::Color& color) override {
        VoxelId id{position, resolution};
        m_voxels[id] = color;
        return true;
    }
    
    bool removeVoxel(const Vector3i& position, VoxelResolution resolution) override {
        VoxelId id{position, resolution};
        return m_voxels.erase(id) > 0;
    }
    
    BoundingBox getWorkspaceBounds() const override {
        return BoundingBox(Vector3f(-10, -10, -10), Vector3f(10, 10, 10));
    }
    
private:
    std::unordered_map<VoxelId, Rendering::Color> m_voxels;
};

class GroupOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        voxelManager = std::make_unique<MockVoxelDataManager>();
        groupManager = std::make_unique<GroupManager>(voxelManager.get());
        
        // Create a test group with some voxels
        testGroupId = groupManager->createGroup("Test Group");
        
        // Add test voxels
        for (int i = 0; i < 3; ++i) {
            VoxelId voxel(Vector3i(i, 0, 0), VoxelResolution::Size_32cm);
            voxelManager->setVoxel(voxel.position, voxel.resolution, Rendering::Color::Red());
            groupManager->addVoxelToGroup(testGroupId, voxel);
        }
    }
    
    std::unique_ptr<MockVoxelDataManager> voxelManager;
    std::unique_ptr<GroupManager> groupManager;
    GroupId testGroupId;
};

TEST_F(GroupOperationsTest, MoveGroupOperation) {
    Vector3f offset(1.0f, 0.0f, 0.0f);
    MoveGroupOperation moveOp(groupManager.get(), voxelManager.get(), testGroupId, offset);
    
    // Execute move
    EXPECT_TRUE(moveOp.execute());
    EXPECT_EQ(moveOp.getType(), GroupModificationType::Moved);
    
    // Check voxels were moved
    auto group = groupManager->getGroup(testGroupId);
    ASSERT_NE(group, nullptr);
    
    auto voxels = group->getVoxelList();
    EXPECT_EQ(voxels.size(), 3);
    
    // Verify new positions
    for (const auto& voxel : voxels) {
        EXPECT_GE(voxel.position.x, 3); // Should be moved right
    }
    
    // Test undo
    EXPECT_TRUE(moveOp.undo());
    
    // Verify positions restored
    voxels = group->getVoxelList();
    for (const auto& voxel : voxels) {
        EXPECT_LT(voxel.position.x, 3); // Should be back to original
    }
}

TEST_F(GroupOperationsTest, CopyGroupOperation) {
    std::string newName = "Copied Group";
    Vector3f offset(0.0f, 2.0f, 0.0f);
    CopyGroupOperation copyOp(groupManager.get(), voxelManager.get(), 
                             testGroupId, newName, offset);
    
    // Execute copy
    EXPECT_TRUE(copyOp.execute());
    EXPECT_EQ(copyOp.getType(), GroupModificationType::Created);
    
    GroupId copiedId = copyOp.getCreatedGroupId();
    EXPECT_NE(copiedId, INVALID_GROUP_ID);
    EXPECT_NE(copiedId, testGroupId);
    
    // Check copied group
    auto copiedGroup = groupManager->getGroup(copiedId);
    ASSERT_NE(copiedGroup, nullptr);
    EXPECT_EQ(copiedGroup->getName(), newName);
    EXPECT_EQ(copiedGroup->getVoxelCount(), 3);
    
    // Verify voxels are offset
    auto copiedVoxels = copiedGroup->getVoxelList();
    for (const auto& voxel : copiedVoxels) {
        EXPECT_GT(voxel.position.y, 0); // Should be moved up
    }
    
    // Test undo
    EXPECT_TRUE(copyOp.undo());
    EXPECT_EQ(groupManager->getGroup(copiedId), nullptr);
}

TEST_F(GroupOperationsTest, RotateGroupOperation) {
    Vector3f eulerAngles(0, 0, Math::degreesToRadians(90)); // 90 degrees around Z
    Vector3f pivot(1.0f, 0.0f, 0.0f);
    RotateGroupOperation rotateOp(groupManager.get(), voxelManager.get(),
                                 testGroupId, eulerAngles, pivot);
    
    // Execute rotation
    EXPECT_TRUE(rotateOp.execute());
    EXPECT_EQ(rotateOp.getType(), GroupModificationType::Rotated);
    
    // Check description
    std::string desc = rotateOp.getDescription();
    EXPECT_TRUE(desc.find("90") != std::string::npos);
    
    // Test undo
    EXPECT_TRUE(rotateOp.undo());
}

TEST_F(GroupOperationsTest, ScaleGroupOperation) {
    Vector3f pivot(1.0f, 0.0f, 0.0f);
    float scaleFactor = 2.0f;
    ScaleGroupOperation scaleOp(groupManager.get(), voxelManager.get(),
                               testGroupId, scaleFactor, pivot);
    
    // Note: Current implementation only supports integer scale factors
    EXPECT_TRUE(scaleOp.execute());
    EXPECT_EQ(scaleOp.getType(), GroupModificationType::Scaled);
    
    // Check that group has more voxels after scaling up
    auto group = groupManager->getGroup(testGroupId);
    EXPECT_GT(group->getVoxelCount(), 3);
    
    // Test undo
    EXPECT_TRUE(scaleOp.undo());
    EXPECT_EQ(group->getVoxelCount(), 3);
}

TEST_F(GroupOperationsTest, MergeGroupsOperation) {
    // Create additional groups
    GroupId group2 = groupManager->createGroup("Group 2");
    GroupId group3 = groupManager->createGroup("Group 3");
    
    // Add voxels to additional groups
    VoxelId voxel2(Vector3i(0, 1, 0), VoxelResolution::Size_32cm);
    VoxelId voxel3(Vector3i(0, 2, 0), VoxelResolution::Size_32cm);
    
    voxelManager->setVoxel(voxel2.position, voxel2.resolution, Rendering::Color::Green());
    voxelManager->setVoxel(voxel3.position, voxel3.resolution, Rendering::Color::Blue());
    
    groupManager->addVoxelToGroup(group2, voxel2);
    groupManager->addVoxelToGroup(group3, voxel3);
    
    // Merge groups
    std::vector<GroupId> sourceIds = {testGroupId, group2, group3};
    std::string targetName = "Merged Group";
    MergeGroupsOperation mergeOp(groupManager.get(), sourceIds, targetName);
    
    EXPECT_TRUE(mergeOp.execute());
    EXPECT_EQ(mergeOp.getType(), GroupModificationType::Created);
    
    GroupId mergedId = mergeOp.getTargetGroupId();
    EXPECT_NE(mergedId, INVALID_GROUP_ID);
    
    // Check merged group
    auto mergedGroup = groupManager->getGroup(mergedId);
    ASSERT_NE(mergedGroup, nullptr);
    EXPECT_EQ(mergedGroup->getName(), targetName);
    EXPECT_EQ(mergedGroup->getVoxelCount(), 5); // 3 + 1 + 1
    
    // Check source groups are deleted
    EXPECT_EQ(groupManager->getGroup(testGroupId), nullptr);
    EXPECT_EQ(groupManager->getGroup(group2), nullptr);
    EXPECT_EQ(groupManager->getGroup(group3), nullptr);
    
    // Test undo
    EXPECT_TRUE(mergeOp.undo());
    EXPECT_EQ(groupManager->getGroup(mergedId), nullptr);
}

TEST_F(GroupOperationsTest, SplitGroupOperation) {
    // Get current voxels
    auto group = groupManager->getGroup(testGroupId);
    auto allVoxels = group->getVoxelList();
    
    // Split into two sets
    std::vector<std::vector<VoxelId>> voxelSets;
    voxelSets.push_back({allVoxels[0], allVoxels[1]});
    voxelSets.push_back({allVoxels[2]});
    
    std::vector<std::string> newNames = {"Split 1", "Split 2"};
    
    SplitGroupOperation splitOp(groupManager.get(), testGroupId, voxelSets, newNames);
    
    EXPECT_TRUE(splitOp.execute());
    EXPECT_EQ(splitOp.getType(), GroupModificationType::Created);
    
    auto createdIds = splitOp.getCreatedGroupIds();
    EXPECT_EQ(createdIds.size(), 2);
    
    // Check created groups
    auto split1 = groupManager->getGroup(createdIds[0]);
    auto split2 = groupManager->getGroup(createdIds[1]);
    
    ASSERT_NE(split1, nullptr);
    ASSERT_NE(split2, nullptr);
    
    EXPECT_EQ(split1->getName(), "Split 1");
    EXPECT_EQ(split1->getVoxelCount(), 2);
    
    EXPECT_EQ(split2->getName(), "Split 2");
    EXPECT_EQ(split2->getVoxelCount(), 1);
    
    // Original group should be deleted if empty
    EXPECT_EQ(groupManager->getGroup(testGroupId), nullptr);
    
    // Test undo
    EXPECT_TRUE(splitOp.undo());
    
    // Check split groups are deleted
    EXPECT_EQ(groupManager->getGroup(createdIds[0]), nullptr);
    EXPECT_EQ(groupManager->getGroup(createdIds[1]), nullptr);
}

TEST_F(GroupOperationsTest, GroupOperationUtils_TransformVoxel) {
    VoxelId voxel(Vector3i(1, 0, 0), VoxelResolution::Size_32cm);
    GroupTransform transform(Vector3f(1.0f, 0.0f, 0.0f));
    
    VoxelId transformed = GroupOperationUtils::transformVoxel(voxel, transform);
    
    // Should be moved by the translation
    EXPECT_EQ(transformed.position.x, 4); // 1 + (1.0 / 0.32)
    EXPECT_EQ(transformed.position.y, 0);
    EXPECT_EQ(transformed.position.z, 0);
    EXPECT_EQ(transformed.resolution, voxel.resolution);
}

TEST_F(GroupOperationsTest, GroupOperationUtils_CalculateBounds) {
    std::vector<VoxelId> voxels = {
        VoxelId(Vector3i(0, 0, 0), VoxelResolution::Size_32cm),
        VoxelId(Vector3i(2, 2, 2), VoxelResolution::Size_32cm),
        VoxelId(Vector3i(-1, -1, -1), VoxelResolution::Size_32cm)
    };
    
    auto bounds = GroupOperationUtils::calculateBounds(voxels);
    
    float voxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    EXPECT_FLOAT_EQ(bounds.min.x, -1 * voxelSize);
    EXPECT_FLOAT_EQ(bounds.min.y, -1 * voxelSize);
    EXPECT_FLOAT_EQ(bounds.min.z, -1 * voxelSize);
    EXPECT_FLOAT_EQ(bounds.max.x, 3 * voxelSize); // 2 + 1 voxel size
}

TEST_F(GroupOperationsTest, GroupOperationUtils_CalculateOptimalPivot) {
    std::vector<VoxelId> voxels = {
        VoxelId(Vector3i(0, 0, 0), VoxelResolution::Size_32cm),
        VoxelId(Vector3i(2, 0, 0), VoxelResolution::Size_32cm),
        VoxelId(Vector3i(1, 0, 0), VoxelResolution::Size_32cm)
    };
    
    auto pivot = GroupOperationUtils::calculateOptimalPivot(voxels);
    
    float voxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    // Center should be at (1, 0, 0) in voxel coords, plus half voxel offset
    EXPECT_FLOAT_EQ(pivot.x, (0.5f + 2.5f + 1.5f) / 3.0f * voxelSize);
    EXPECT_FLOAT_EQ(pivot.y, 0.5f * voxelSize);
    EXPECT_FLOAT_EQ(pivot.z, 0.5f * voxelSize);
}

TEST_F(GroupOperationsTest, GroupOperationUtils_ValidateVoxelPositions) {
    BoundingBox workspaceBounds(Vector3f(-5, -5, -5), Vector3f(5, 5, 5));
    
    std::vector<VoxelId> validVoxels = {
        VoxelId(Vector3i(0, 0, 0), VoxelResolution::Size_32cm),
        VoxelId(Vector3i(1, 1, 1), VoxelResolution::Size_32cm)
    };
    
    std::vector<VoxelId> invalidVoxels = {
        VoxelId(Vector3i(100, 0, 0), VoxelResolution::Size_32cm), // Outside bounds
        VoxelId(Vector3i(0, 0, 0), VoxelResolution::Size_32cm)
    };
    
    EXPECT_TRUE(GroupOperationUtils::validateVoxelPositions(validVoxels, workspaceBounds));
    EXPECT_FALSE(GroupOperationUtils::validateVoxelPositions(invalidVoxels, workspaceBounds));
}

TEST_F(GroupOperationsTest, GroupOperationUtils_GenerateUniqueName) {
    std::vector<std::string> existingNames = {"Group 1", "Group 2", "Group 3"};
    
    std::string unique1 = GroupOperationUtils::generateUniqueName("Group", existingNames);
    EXPECT_EQ(unique1, "Group");
    
    std::string unique2 = GroupOperationUtils::generateUniqueName("Group 1", existingNames);
    EXPECT_EQ(unique2, "Group 1 1");
    
    existingNames.push_back("Group 1 1");
    std::string unique3 = GroupOperationUtils::generateUniqueName("Group 1", existingNames);
    EXPECT_EQ(unique3, "Group 1 2");
}