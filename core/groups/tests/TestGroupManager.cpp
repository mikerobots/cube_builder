#include <gtest/gtest.h>
#include "../include/groups/GroupManager.h"
#include <VoxelDataManager.h>
#include <EventDispatcher.h>

using namespace VoxelEditor::Groups;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Events;

// Mock implementations
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
    
    void addTestVoxel(const VoxelId& voxel, const Rendering::Color& color) {
        m_voxels[voxel] = color;
    }
    
private:
    std::unordered_map<VoxelId, Rendering::Color> m_voxels;
};

class MockEventDispatcher : public EventDispatcher {
public:
    template<typename EventType>
    void dispatch(const EventType& event) {
        m_eventCount++;
        m_lastEventType = typeid(EventType).name();
    }
    
    int getEventCount() const { return m_eventCount; }
    std::string getLastEventType() const { return m_lastEventType; }
    void reset() { m_eventCount = 0; m_lastEventType.clear(); }
    
private:
    int m_eventCount = 0;
    std::string m_lastEventType;
};

class GroupManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        voxelManager = std::make_unique<MockVoxelDataManager>();
        eventDispatcher = std::make_unique<MockEventDispatcher>();
        groupManager = std::make_unique<GroupManager>(voxelManager.get(), eventDispatcher.get());
    }
    
    std::unique_ptr<MockVoxelDataManager> voxelManager;
    std::unique_ptr<MockEventDispatcher> eventDispatcher;
    std::unique_ptr<GroupManager> groupManager;
};

TEST_F(GroupManagerTest, CreateGroup) {
    std::string groupName = "Test Group";
    GroupId id = groupManager->createGroup(groupName);
    
    EXPECT_NE(id, INVALID_GROUP_ID);
    EXPECT_TRUE(groupManager->groupExists(id));
    
    auto group = groupManager->getGroup(id);
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->getName(), groupName);
    EXPECT_EQ(group->getId(), id);
    EXPECT_EQ(eventDispatcher->getEventCount(), 1);
}

TEST_F(GroupManagerTest, CreateGroupWithVoxels) {
    std::vector<VoxelId> voxels = {
        VoxelId(Vector3i(0, 0, 0), VoxelResolution::Size_32cm),
        VoxelId(Vector3i(1, 0, 0), VoxelResolution::Size_32cm)
    };
    
    // Add voxels to voxel manager
    for (const auto& voxel : voxels) {
        voxelManager->setVoxel(voxel.position, voxel.resolution, Rendering::Color::Red());
    }
    
    GroupId id = groupManager->createGroup("Group with Voxels", voxels);
    
    auto group = groupManager->getGroup(id);
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->getVoxelCount(), 2);
    
    // Check voxel membership
    for (const auto& voxel : voxels) {
        EXPECT_EQ(groupManager->findGroupContaining(voxel), id);
    }
}

TEST_F(GroupManagerTest, DeleteGroup) {
    GroupId id = groupManager->createGroup("To Delete");
    EXPECT_TRUE(groupManager->groupExists(id));
    
    eventDispatcher->reset();
    EXPECT_TRUE(groupManager->deleteGroup(id));
    EXPECT_FALSE(groupManager->groupExists(id));
    EXPECT_EQ(groupManager->getGroup(id), nullptr);
    EXPECT_EQ(eventDispatcher->getEventCount(), 1);
    
    // Try to delete non-existent group
    EXPECT_FALSE(groupManager->deleteGroup(id));
}

TEST_F(GroupManagerTest, RenameGroup) {
    GroupId id = groupManager->createGroup("Original Name");
    std::string newName = "New Name";
    
    eventDispatcher->reset();
    EXPECT_TRUE(groupManager->renameGroup(id, newName));
    
    auto group = groupManager->getGroup(id);
    EXPECT_EQ(group->getName(), newName);
    EXPECT_EQ(eventDispatcher->getEventCount(), 1);
}

TEST_F(GroupManagerTest, VoxelMembership) {
    GroupId group1 = groupManager->createGroup("Group 1");
    GroupId group2 = groupManager->createGroup("Group 2");
    
    VoxelId voxel(Vector3i(0, 0, 0), VoxelResolution::Size_32cm);
    voxelManager->setVoxel(voxel.position, voxel.resolution, Rendering::Color::Red());
    
    // Add voxel to group1
    EXPECT_TRUE(groupManager->addVoxelToGroup(group1, voxel));
    EXPECT_EQ(groupManager->findGroupContaining(voxel), group1);
    
    // Add same voxel to group2 (multiple group membership)
    EXPECT_TRUE(groupManager->addVoxelToGroup(group2, voxel));
    
    auto groups = groupManager->findGroupsContaining(voxel);
    EXPECT_EQ(groups.size(), 2);
    EXPECT_TRUE(std::find(groups.begin(), groups.end(), group1) != groups.end());
    EXPECT_TRUE(std::find(groups.begin(), groups.end(), group2) != groups.end());
    
    // Remove from group1
    EXPECT_TRUE(groupManager->removeVoxelFromGroup(group1, voxel));
    EXPECT_EQ(groupManager->findGroupContaining(voxel), group2);
}

TEST_F(GroupManagerTest, GroupVisibility) {
    GroupId id = groupManager->createGroup("Visible Group");
    
    // Default should be visible
    EXPECT_TRUE(groupManager->isGroupVisible(id));
    
    eventDispatcher->reset();
    groupManager->hideGroup(id);
    EXPECT_FALSE(groupManager->isGroupVisible(id));
    EXPECT_EQ(eventDispatcher->getEventCount(), 1);
    
    groupManager->showGroup(id);
    EXPECT_TRUE(groupManager->isGroupVisible(id));
    EXPECT_EQ(eventDispatcher->getEventCount(), 2);
}

TEST_F(GroupManagerTest, GroupOpacity) {
    GroupId id = groupManager->createGroup("Opaque Group");
    
    // Default should be opaque
    EXPECT_FLOAT_EQ(groupManager->getGroupOpacity(id), 1.0f);
    
    groupManager->setGroupOpacity(id, 0.5f);
    EXPECT_FLOAT_EQ(groupManager->getGroupOpacity(id), 0.5f);
}

TEST_F(GroupManagerTest, GroupColor) {
    GroupId id = groupManager->createGroup("Colored Group");
    
    auto initialColor = groupManager->getGroupColor(id);
    EXPECT_NE(initialColor, Rendering::Color::White()); // Should have assigned a palette color
    
    Rendering::Color newColor = Rendering::Color::Blue();
    groupManager->setGroupColor(id, newColor);
    EXPECT_EQ(groupManager->getGroupColor(id), newColor);
}

TEST_F(GroupManagerTest, GroupLocking) {
    GroupId id = groupManager->createGroup("Lockable Group");
    
    // Default should be unlocked
    EXPECT_FALSE(groupManager->isGroupLocked(id));
    
    groupManager->lockGroup(id);
    EXPECT_TRUE(groupManager->isGroupLocked(id));
    
    groupManager->unlockGroup(id);
    EXPECT_FALSE(groupManager->isGroupLocked(id));
}

TEST_F(GroupManagerTest, GroupHierarchy) {
    GroupId parent = groupManager->createGroup("Parent");
    GroupId child1 = groupManager->createGroup("Child 1");
    GroupId child2 = groupManager->createGroup("Child 2");
    
    EXPECT_TRUE(groupManager->setParentGroup(child1, parent));
    EXPECT_TRUE(groupManager->setParentGroup(child2, parent));
    
    EXPECT_EQ(groupManager->getParentGroup(child1), parent);
    EXPECT_EQ(groupManager->getParentGroup(child2), parent);
    
    auto children = groupManager->getChildGroups(parent);
    EXPECT_EQ(children.size(), 2);
    EXPECT_TRUE(std::find(children.begin(), children.end(), child1) != children.end());
    EXPECT_TRUE(std::find(children.begin(), children.end(), child2) != children.end());
    
    auto roots = groupManager->getRootGroups();
    EXPECT_TRUE(std::find(roots.begin(), roots.end(), parent) != roots.end());
    EXPECT_FALSE(std::find(roots.begin(), roots.end(), child1) != roots.end());
}

TEST_F(GroupManagerTest, GroupQueries) {
    groupManager->createGroup("Alpha Group");
    groupManager->createGroup("Beta Group");
    groupManager->createGroup("Alpha Test");
    
    auto alphaGroups = groupManager->findGroupsByName("Alpha");
    EXPECT_EQ(alphaGroups.size(), 2);
    
    // Test predicate search
    auto lockedGroups = groupManager->findGroupsByPredicate(
        [](const VoxelGroup& group) { return group.isLocked(); }
    );
    EXPECT_EQ(lockedGroups.size(), 0);
    
    // Lock one group
    groupManager->lockGroup(alphaGroups[0]);
    
    lockedGroups = groupManager->getLockedGroups();
    EXPECT_EQ(lockedGroups.size(), 1);
}

TEST_F(GroupManagerTest, GroupStatistics) {
    // Create groups with voxels
    GroupId group1 = groupManager->createGroup("Group 1");
    GroupId group2 = groupManager->createGroup("Group 2");
    
    for (int i = 0; i < 5; ++i) {
        VoxelId voxel(Vector3i(i, 0, 0), VoxelResolution::Size_32cm);
        voxelManager->setVoxel(voxel.position, voxel.resolution, Rendering::Color::Red());
        groupManager->addVoxelToGroup(group1, voxel);
    }
    
    for (int i = 0; i < 3; ++i) {
        VoxelId voxel(Vector3i(0, i, 0), VoxelResolution::Size_32cm);
        voxelManager->setVoxel(voxel.position, voxel.resolution, Rendering::Color::Blue());
        groupManager->addVoxelToGroup(group2, voxel);
    }
    
    auto stats = groupManager->getStatistics();
    EXPECT_EQ(stats.totalGroups, 2);
    EXPECT_EQ(stats.totalVoxels, 8);
    EXPECT_EQ(stats.maxGroupSize, 5);
    EXPECT_FLOAT_EQ(stats.averageGroupSize, 4.0f);
    
    EXPECT_EQ(groupManager->getTotalVoxelCount(), 8);
    EXPECT_EQ(groupManager->getGroupCount(), 2);
}

TEST_F(GroupManagerTest, GroupIteration) {
    std::vector<GroupId> createdIds;
    for (int i = 0; i < 3; ++i) {
        createdIds.push_back(groupManager->createGroup("Group " + std::to_string(i)));
    }
    
    int count = 0;
    groupManager->forEachGroup([&count](const VoxelGroup& group) {
        count++;
    });
    EXPECT_EQ(count, 3);
    
    // Test hierarchy iteration
    groupManager->setParentGroup(createdIds[1], createdIds[0]);
    groupManager->setParentGroup(createdIds[2], createdIds[1]);
    
    count = 0;
    groupManager->forEachGroupInHierarchy(createdIds[0], 
        [&count](const VoxelGroup& group) {
            count++;
        });
    EXPECT_EQ(count, 3); // Should visit all 3 groups in hierarchy
}

TEST_F(GroupManagerTest, GroupBounds) {
    GroupId id = groupManager->createGroup("Bounded Group");
    
    // Add voxels at known positions
    std::vector<VoxelId> voxels = {
        VoxelId(Vector3i(0, 0, 0), VoxelResolution::Size_32cm),
        VoxelId(Vector3i(2, 2, 2), VoxelResolution::Size_32cm)
    };
    
    for (const auto& voxel : voxels) {
        voxelManager->setVoxel(voxel.position, voxel.resolution, Rendering::Color::Red());
        groupManager->addVoxelToGroup(id, voxel);
    }
    
    auto bounds = groupManager->getGroupBounds(id);
    float voxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    
    EXPECT_LE(bounds.min.x, 0.0f);
    EXPECT_LE(bounds.min.y, 0.0f);
    EXPECT_LE(bounds.min.z, 0.0f);
    EXPECT_GE(bounds.max.x, 2.0f * voxelSize);
    EXPECT_GE(bounds.max.y, 2.0f * voxelSize);
    EXPECT_GE(bounds.max.z, 2.0f * voxelSize);
}

TEST_F(GroupManagerTest, CleanupEmptyGroups) {
    GroupId empty1 = groupManager->createGroup("Empty 1");
    GroupId empty2 = groupManager->createGroup("Empty 2");
    GroupId notEmpty = groupManager->createGroup("Not Empty");
    
    // Add voxel to one group
    VoxelId voxel(Vector3i(0, 0, 0), VoxelResolution::Size_32cm);
    voxelManager->setVoxel(voxel.position, voxel.resolution, Rendering::Color::Red());
    groupManager->addVoxelToGroup(notEmpty, voxel);
    
    EXPECT_EQ(groupManager->getGroupCount(), 3);
    
    groupManager->cleanupEmptyGroups();
    
    EXPECT_EQ(groupManager->getGroupCount(), 1);
    EXPECT_FALSE(groupManager->groupExists(empty1));
    EXPECT_FALSE(groupManager->groupExists(empty2));
    EXPECT_TRUE(groupManager->groupExists(notEmpty));
}

TEST_F(GroupManagerTest, ExportImport) {
    // Create complex group structure
    GroupId parent = groupManager->createGroup("Parent");
    GroupId child1 = groupManager->createGroup("Child 1");
    GroupId child2 = groupManager->createGroup("Child 2");
    
    groupManager->setParentGroup(child1, parent);
    groupManager->setParentGroup(child2, parent);
    groupManager->setGroupColor(parent, Rendering::Color::Red());
    groupManager->lockGroup(child1);
    
    // Add voxels
    VoxelId voxel1(Vector3i(0, 0, 0), VoxelResolution::Size_32cm);
    VoxelId voxel2(Vector3i(1, 0, 0), VoxelResolution::Size_32cm);
    voxelManager->setVoxel(voxel1.position, voxel1.resolution, Rendering::Color::Red());
    voxelManager->setVoxel(voxel2.position, voxel2.resolution, Rendering::Color::Blue());
    
    groupManager->addVoxelToGroup(parent, voxel1);
    groupManager->addVoxelToGroup(child1, voxel2);
    
    // Export data
    auto exportedData = groupManager->exportData();
    
    // Create new manager and import
    GroupManager newManager(voxelManager.get());
    EXPECT_TRUE(newManager.importData(exportedData));
    
    // Verify structure is preserved
    EXPECT_EQ(newManager.getGroupCount(), 3);
    EXPECT_TRUE(newManager.groupExists(parent));
    EXPECT_TRUE(newManager.groupExists(child1));
    EXPECT_TRUE(newManager.groupExists(child2));
    
    EXPECT_EQ(newManager.getParentGroup(child1), parent);
    EXPECT_EQ(newManager.getParentGroup(child2), parent);
    EXPECT_EQ(newManager.getGroupColor(parent), Rendering::Color::Red());
    EXPECT_TRUE(newManager.isGroupLocked(child1));
    
    EXPECT_EQ(newManager.findGroupContaining(voxel1), parent);
    EXPECT_EQ(newManager.findGroupContaining(voxel2), child1);
}

TEST_F(GroupManagerTest, Validation) {
    // Create valid structure
    GroupId group = groupManager->createGroup("Valid Group");
    VoxelId voxel(Vector3i(0, 0, 0), VoxelResolution::Size_32cm);
    voxelManager->setVoxel(voxel.position, voxel.resolution, Rendering::Color::Red());
    groupManager->addVoxelToGroup(group, voxel);
    
    EXPECT_TRUE(groupManager->validateGroups());
    
    // Note: Testing invalid states would require manipulating internal state
    // which is not easily accessible through the public interface
}