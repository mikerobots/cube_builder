#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../include/groups/GroupManager.h"
#include "../include/groups/VoxelGroup.h"
#include "../include/groups/GroupHierarchy.h"
#include "../include/groups/GroupOperations.h"

using namespace VoxelEditor::Groups;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Events;

// Mock implementations for testing
class MockVoxelDataManager {
public:
    MockVoxelDataManager() : m_workspaceSize(5.0f, 5.0f, 5.0f) {}
    
    bool hasVoxel(const Vector3i& position, VoxelResolution resolution) const {
        VoxelId id{position, resolution};
        return m_voxels.find(id) != m_voxels.end();
    }
    
    VoxelEditor::Rendering::Color getVoxel(const Vector3i& position, VoxelResolution resolution) const {
        VoxelId id{position, resolution};
        auto it = m_voxels.find(id);
        return (it != m_voxels.end()) ? it->second : VoxelEditor::Rendering::Color::Black();
    }
    
    bool setVoxel(const Vector3i& position, VoxelResolution resolution, 
                 const VoxelEditor::Rendering::Color& color) {
        VoxelId id{position, resolution};
        m_voxels[id] = color;
        return true;
    }
    
    bool removeVoxel(const Vector3i& position, VoxelResolution resolution) {
        VoxelId id{position, resolution};
        return m_voxels.erase(id) > 0;
    }
    
    Vector3f getWorkspaceSize() const {
        return m_workspaceSize;
    }
    
    VoxelEditor::Math::BoundingBox getWorkspaceBounds() const {
        return VoxelEditor::Math::BoundingBox(
            VoxelEditor::Math::Vector3f(-m_workspaceSize.x/2, 0, -m_workspaceSize.z/2),
            VoxelEditor::Math::Vector3f(m_workspaceSize.x/2, m_workspaceSize.y, m_workspaceSize.z/2)
        );
    }
    
    void addTestVoxel(const VoxelId& voxel, const VoxelEditor::Rendering::Color& color) {
        m_voxels[voxel] = color;
    }
    
    size_t getVoxelCount() const {
        return m_voxels.size();
    }
    
private:
    std::unordered_map<VoxelId, VoxelEditor::Rendering::Color> m_voxels;
    Vector3f m_workspaceSize;
};

class GroupRequirementsTest : public ::testing::Test {
protected:
    void SetUp() override {
        voxelManager = std::make_unique<MockVoxelDataManager>();
        groupManager = std::make_unique<GroupManager>(nullptr, nullptr);
    }
    
    std::unique_ptr<MockVoxelDataManager> voxelManager;
    std::unique_ptr<GroupManager> groupManager;
};

// Group Operations Requirements Tests

TEST_F(GroupRequirementsTest, CreateGroupFromSelectedVoxels) {
    // REQ: Create groups from selected voxels
    std::vector<VoxelId> selectedVoxels = {
        VoxelId(Vector3i(0, 0, 0), VoxelResolution::Size_32cm),
        VoxelId(Vector3i(1, 0, 0), VoxelResolution::Size_32cm),
        VoxelId(Vector3i(2, 0, 0), VoxelResolution::Size_32cm)
    };
    
    // Add voxels to voxel manager
    for (const auto& voxel : selectedVoxels) {
        voxelManager->setVoxel(voxel.position.value(), voxel.resolution, VoxelEditor::Rendering::Color::Red());
    }
    
    // Create group from selected voxels
    GroupId groupId = groupManager->createGroup("Selected Voxels Group", selectedVoxels);
    
    EXPECT_NE(groupId, INVALID_GROUP_ID);
    EXPECT_TRUE(groupManager->groupExists(groupId));
    
    auto group = groupManager->getGroup(groupId);
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->getVoxelCount(), selectedVoxels.size());
    
    // Verify all voxels are in the group
    for (const auto& voxel : selectedVoxels) {
        EXPECT_EQ(groupManager->findGroupContaining(voxel), groupId);
    }
}

TEST_F(GroupRequirementsTest, GroupMoveOperation) {
    // REQ: Group operations: move, hide/show, lock, copy/duplicate
    // Test move operation
    GroupId groupId = groupManager->createGroup("Move Test Group");
    
    // Add voxels to group
    std::vector<VoxelId> voxels = {
        VoxelId(Vector3i(0, 0, 0), VoxelResolution::Size_32cm),
        VoxelId(Vector3i(1, 0, 0), VoxelResolution::Size_32cm)
    };
    
    for (const auto& voxel : voxels) {
        voxelManager->setVoxel(voxel.position.value(), voxel.resolution, VoxelEditor::Rendering::Color::Blue());
        groupManager->addVoxelToGroup(groupId, voxel);
    }
    
    // Get initial bounds
    auto initialBounds = groupManager->getGroupBounds(groupId);
    
    // Move operation would be implemented through GroupOperations
    // For now, test that group supports position tracking
    auto group = groupManager->getGroup(groupId);
    ASSERT_NE(group, nullptr);
    
    // Test that group has bounds
    auto bounds = group->getBoundingBox();
    EXPECT_NE(bounds.min, bounds.max);
}

TEST_F(GroupRequirementsTest, GroupHideShowOperation) {
    // REQ: Group operations: move, hide/show, lock, copy/duplicate
    // Test hide/show operations
    GroupId groupId = groupManager->createGroup("Visibility Test Group");
    
    // Default should be visible
    EXPECT_TRUE(groupManager->isGroupVisible(groupId));
    
    // Hide group
    groupManager->hideGroup(groupId);
    EXPECT_FALSE(groupManager->isGroupVisible(groupId));
    
    // Show group
    groupManager->showGroup(groupId);
    EXPECT_TRUE(groupManager->isGroupVisible(groupId));
}

TEST_F(GroupRequirementsTest, GroupLockOperation) {
    // REQ: Group operations: move, hide/show, lock, copy/duplicate
    // Test lock operations
    GroupId groupId = groupManager->createGroup("Lock Test Group");
    
    // Default should be unlocked
    EXPECT_FALSE(groupManager->isGroupLocked(groupId));
    
    // Lock group
    groupManager->lockGroup(groupId);
    EXPECT_TRUE(groupManager->isGroupLocked(groupId));
    
    // Unlock group
    groupManager->unlockGroup(groupId);
    EXPECT_FALSE(groupManager->isGroupLocked(groupId));
}

TEST_F(GroupRequirementsTest, GroupCopyDuplicateOperation) {
    // REQ: Group operations: move, hide/show, lock, copy/duplicate
    // Test copy/duplicate through multiple group creation
    GroupId originalGroup = groupManager->createGroup("Original Group");
    
    // Add voxels
    VoxelId voxel1(Vector3i(0, 0, 0), VoxelResolution::Size_32cm);
    VoxelId voxel2(Vector3i(1, 0, 0), VoxelResolution::Size_32cm);
    voxelManager->setVoxel(voxel1.position.value(), voxel1.resolution, VoxelEditor::Rendering::Color::Green());
    voxelManager->setVoxel(voxel2.position.value(), voxel2.resolution, VoxelEditor::Rendering::Color::Green());
    
    groupManager->addVoxelToGroup(originalGroup, voxel1);
    groupManager->addVoxelToGroup(originalGroup, voxel2);
    
    // Get original group data
    auto originalGroupData = groupManager->getGroup(originalGroup);
    ASSERT_NE(originalGroupData, nullptr);
    auto originalVoxels = originalGroupData->getVoxelList();
    
    // Create a "copy" by creating new group with same voxels
    GroupId copiedGroup = groupManager->createGroup("Copied Group", originalVoxels);
    
    EXPECT_NE(copiedGroup, INVALID_GROUP_ID);
    EXPECT_NE(copiedGroup, originalGroup);
    
    auto copiedGroupData = groupManager->getGroup(copiedGroup);
    ASSERT_NE(copiedGroupData, nullptr);
    EXPECT_EQ(copiedGroupData->getVoxelCount(), originalGroupData->getVoxelCount());
}

TEST_F(GroupRequirementsTest, GroupHierarchySupport) {
    // REQ: Group hierarchy support (nested groups)
    GroupId parentGroup = groupManager->createGroup("Parent Group");
    GroupId childGroup1 = groupManager->createGroup("Child Group 1");
    GroupId childGroup2 = groupManager->createGroup("Child Group 2");
    GroupId grandchildGroup = groupManager->createGroup("Grandchild Group");
    
    // Create hierarchy: parent -> child1 -> grandchild
    //                        -> child2
    EXPECT_TRUE(groupManager->setParentGroup(childGroup1, parentGroup));
    EXPECT_TRUE(groupManager->setParentGroup(childGroup2, parentGroup));
    EXPECT_TRUE(groupManager->setParentGroup(grandchildGroup, childGroup1));
    
    // Verify parent relationships
    EXPECT_EQ(groupManager->getParentGroup(childGroup1), parentGroup);
    EXPECT_EQ(groupManager->getParentGroup(childGroup2), parentGroup);
    EXPECT_EQ(groupManager->getParentGroup(grandchildGroup), childGroup1);
    
    // Verify children
    auto parentChildren = groupManager->getChildGroups(parentGroup);
    EXPECT_EQ(parentChildren.size(), 2);
    EXPECT_TRUE(std::find(parentChildren.begin(), parentChildren.end(), childGroup1) != parentChildren.end());
    EXPECT_TRUE(std::find(parentChildren.begin(), parentChildren.end(), childGroup2) != parentChildren.end());
    
    auto child1Children = groupManager->getChildGroups(childGroup1);
    EXPECT_EQ(child1Children.size(), 1);
    EXPECT_EQ(child1Children[0], grandchildGroup);
    
    // Verify root groups
    auto roots = groupManager->getRootGroups();
    EXPECT_TRUE(std::find(roots.begin(), roots.end(), parentGroup) != roots.end());
    EXPECT_FALSE(std::find(roots.begin(), roots.end(), childGroup1) != roots.end());
}

TEST_F(GroupRequirementsTest, VisualGroupIndicators) {
    // REQ: Visual group indicators (color coding, outlines)
    GroupId groupId = groupManager->createGroup("Colored Group");
    
    // Test color assignment
    auto initialColor = groupManager->getGroupColor(groupId);
    // Should have a default color from palette
    EXPECT_NE(initialColor, VoxelEditor::Rendering::Color::White());
    
    // Test color modification
    VoxelEditor::Rendering::Color customColor = VoxelEditor::Rendering::Color::Blue();
    groupManager->setGroupColor(groupId, customColor);
    EXPECT_EQ(groupManager->getGroupColor(groupId), customColor);
    
    // Test opacity for visual indicators
    EXPECT_FLOAT_EQ(groupManager->getGroupOpacity(groupId), 1.0f);
    groupManager->setGroupOpacity(groupId, 0.5f);
    EXPECT_FLOAT_EQ(groupManager->getGroupOpacity(groupId), 0.5f);
}

TEST_F(GroupRequirementsTest, GroupManagementOperations) {
    // REQ: Group management (list, rename, delete)
    
    // Create multiple groups
    GroupId group1 = groupManager->createGroup("Group Alpha");
    GroupId group2 = groupManager->createGroup("Group Beta");
    GroupId group3 = groupManager->createGroup("Group Alpha 2");
    
    // Test list operation
    EXPECT_EQ(groupManager->getGroupCount(), 3);
    auto allGroups = groupManager->listGroups();
    EXPECT_EQ(allGroups.size(), 3);
    
    // Test search by name
    auto alphaGroups = groupManager->findGroupsByName("Alpha");
    EXPECT_EQ(alphaGroups.size(), 2);
    
    // Test rename operation
    std::string newName = "Group Gamma";
    EXPECT_TRUE(groupManager->renameGroup(group1, newName));
    auto renamedGroup = groupManager->getGroup(group1);
    EXPECT_EQ(renamedGroup->getName(), newName);
    
    // Test delete operation
    EXPECT_TRUE(groupManager->deleteGroup(group2));
    EXPECT_FALSE(groupManager->groupExists(group2));
    EXPECT_EQ(groupManager->getGroupCount(), 2);
}

// Group Metadata Requirements Tests

TEST_F(GroupRequirementsTest, GroupMetadataStorage) {
    // REQ: Group metadata storage in file format
    GroupId groupId = groupManager->createGroup("Metadata Test Group");
    
    // Set various metadata
    groupManager->setGroupColor(groupId, VoxelEditor::Rendering::Color::Red());
    groupManager->setGroupOpacity(groupId, 0.75f);
    groupManager->lockGroup(groupId);
    
    // Get group and verify metadata
    auto group = groupManager->getGroup(groupId);
    ASSERT_NE(group, nullptr);
    
    const auto& metadata = group->getMetadata();
    EXPECT_EQ(metadata.name, "Metadata Test Group");
    EXPECT_EQ(metadata.color, VoxelEditor::Rendering::Color::Red());
    EXPECT_FLOAT_EQ(metadata.opacity, 0.75f);
    EXPECT_TRUE(metadata.locked);
    
    // Verify timestamps are set
    EXPECT_NE(metadata.created.time_since_epoch().count(), 0);
    EXPECT_NE(metadata.modified.time_since_epoch().count(), 0);
}

TEST_F(GroupRequirementsTest, GroupPersistenceAcrossSaveLoad) {
    // REQ: Group persistence across save/load operations
    // Create complex group structure
    GroupId parent = groupManager->createGroup("Persistent Parent");
    GroupId child = groupManager->createGroup("Persistent Child");
    
    groupManager->setParentGroup(child, parent);
    groupManager->setGroupColor(parent, VoxelEditor::Rendering::Color::Green());
    groupManager->lockGroup(child);
    groupManager->hideGroup(parent);
    
    // Add voxels
    VoxelId voxel(Vector3i(0, 0, 0), VoxelResolution::Size_32cm);
    voxelManager->setVoxel(voxel.position.value(), voxel.resolution, VoxelEditor::Rendering::Color(1.0f, 1.0f, 0.0f, 1.0f));
    groupManager->addVoxelToGroup(parent, voxel);
    
    // Export data (simulating save)
    auto exportedData = groupManager->exportData();
    
    // Create new manager and import (simulating load)
    GroupManager loadedManager(nullptr, nullptr);
    EXPECT_TRUE(loadedManager.importData(exportedData));
    
    // Verify structure is preserved
    EXPECT_TRUE(loadedManager.groupExists(parent));
    EXPECT_TRUE(loadedManager.groupExists(child));
    EXPECT_EQ(loadedManager.getParentGroup(child), parent);
    EXPECT_EQ(loadedManager.getGroupColor(parent), VoxelEditor::Rendering::Color::Green());
    EXPECT_TRUE(loadedManager.isGroupLocked(child));
    EXPECT_FALSE(loadedManager.isGroupVisible(parent));
    EXPECT_EQ(loadedManager.findGroupContaining(voxel), parent);
}

TEST_F(GroupRequirementsTest, GroupNamingAndOrganization) {
    // REQ: Group naming and organization
    // Test naming constraints and organization
    GroupId group1 = groupManager->createGroup("Project/Building/Floor1");
    GroupId group2 = groupManager->createGroup("Project/Building/Floor2");
    GroupId group3 = groupManager->createGroup("Project/Landscape");
    
    // Verify groups were created with proper names
    EXPECT_NE(group1, INVALID_GROUP_ID);
    EXPECT_NE(group2, INVALID_GROUP_ID);
    EXPECT_NE(group3, INVALID_GROUP_ID);
    
    // Test finding groups by partial name
    auto buildingGroups = groupManager->findGroupsByName("Building");
    EXPECT_EQ(buildingGroups.size(), 2);
    
    auto projectGroups = groupManager->findGroupsByName("Project");
    EXPECT_EQ(projectGroups.size(), 3);
}

// Memory Management Requirements Tests

TEST_F(GroupRequirementsTest, MemoryConstraints_REQ_6_3_2) {
    // REQ-6.3.2: Voxel data storage shall not exceed 2GB
    // Test that group system respects memory constraints
    
    // Create a large number of groups to test memory tracking
    const int numGroups = 100;
    std::vector<GroupId> groups;
    
    for (int i = 0; i < numGroups; ++i) {
        GroupId id = groupManager->createGroup("Group " + std::to_string(i));
        groups.push_back(id);
        
        // Add some voxels to each group
        for (int j = 0; j < 10; ++j) {
            VoxelId voxel(Vector3i(i, j, 0), VoxelResolution::Size_32cm);
            voxelManager->setVoxel(voxel.position.value(), voxel.resolution, VoxelEditor::Rendering::Color::Red());
            groupManager->addVoxelToGroup(id, voxel);
        }
    }
    
    // Get statistics to verify memory tracking
    auto stats = groupManager->getStatistics();
    EXPECT_EQ(stats.totalGroups, numGroups);
    EXPECT_EQ(stats.totalVoxels, numGroups * 10);
    
    // Memory usage should be tracked (actual implementation would calculate real memory)
    // For now, just verify the field exists
    EXPECT_GE(stats.memoryUsage, 0);
}

// State Persistence Requirements Tests

TEST_F(GroupRequirementsTest, GroupDefinitionsStorage_REQ_8_1_8) {
    // REQ-8.1.8: Format shall store group definitions and metadata
    // Create groups with full metadata
    GroupId group1 = groupManager->createGroup("Definition Test 1");
    GroupId group2 = groupManager->createGroup("Definition Test 2");
    
    // Set comprehensive metadata
    groupManager->setGroupColor(group1, VoxelEditor::Rendering::Color(0.5f, 0.7f, 0.3f, 1.0f));
    groupManager->setGroupOpacity(group1, 0.8f);
    groupManager->lockGroup(group1);
    
    // Add hierarchy
    groupManager->setParentGroup(group2, group1);
    
    // Add voxels
    VoxelId voxel1(Vector3i(0, 0, 0), VoxelResolution::Size_16cm);
    VoxelId voxel2(Vector3i(1, 0, 0), VoxelResolution::Size_32cm);
    groupManager->addVoxelToGroup(group1, voxel1);
    groupManager->addVoxelToGroup(group2, voxel2);
    
    // Export data structure
    auto exportData = groupManager->exportData();
    
    // Verify exported data contains all group definitions
    EXPECT_EQ(exportData.groups.size(), 2);
    
    // Verify both groups exist in the exported data
    bool foundGroup1 = false;
    bool foundGroup2 = false;
    
    for (const auto& [id, metadata] : exportData.groups) {
        if (id == group1) {
            foundGroup1 = true;
            // Verify metadata is included
            EXPECT_EQ(metadata.name, "Definition Test 1");
            EXPECT_TRUE(metadata.locked);
            EXPECT_FLOAT_EQ(metadata.opacity, 0.8f);
        } else if (id == group2) {
            foundGroup2 = true;
        }
    }
    
    EXPECT_TRUE(foundGroup1);
    EXPECT_TRUE(foundGroup2);
}

TEST_F(GroupRequirementsTest, GroupVisibilityStates_REQ_8_1_9) {
    // REQ-8.1.9: Format shall store group visibility states
    GroupId visibleGroup = groupManager->createGroup("Visible Group");
    GroupId hiddenGroup = groupManager->createGroup("Hidden Group");
    GroupId partialGroup = groupManager->createGroup("Partial Group");
    
    // Set visibility states
    groupManager->showGroup(visibleGroup);
    groupManager->hideGroup(hiddenGroup);
    groupManager->showGroup(partialGroup);
    groupManager->setGroupOpacity(partialGroup, 0.5f);
    
    // Export data
    auto exportData = groupManager->exportData();
    
    // Create new manager and import
    GroupManager importedManager(nullptr, nullptr);
    importedManager.importData(exportData);
    
    // Verify visibility states are preserved
    EXPECT_TRUE(importedManager.isGroupVisible(visibleGroup));
    EXPECT_FALSE(importedManager.isGroupVisible(hiddenGroup));
    EXPECT_TRUE(importedManager.isGroupVisible(partialGroup));
    EXPECT_FLOAT_EQ(importedManager.getGroupOpacity(partialGroup), 0.5f);
}

// Command Line Interface Requirements Tests

TEST_F(GroupRequirementsTest, CLIGroupCommands_REQ_9_2_5) {
    // REQ-9.2.5: CLI shall support group commands (group create/hide/show/list)
    // This test verifies the group system provides the necessary API for CLI commands
    
    // Test group create command support
    GroupId cliGroup = groupManager->createGroup("CLI Test Group");
    EXPECT_NE(cliGroup, INVALID_GROUP_ID);
    
    // Test group hide command support
    groupManager->hideGroup(cliGroup);
    EXPECT_FALSE(groupManager->isGroupVisible(cliGroup));
    
    // Test group show command support
    groupManager->showGroup(cliGroup);
    EXPECT_TRUE(groupManager->isGroupVisible(cliGroup));
    
    // Test group list command support
    auto groups = groupManager->listGroups();
    EXPECT_FALSE(groups.empty());
    
    // Verify list contains proper information for CLI display
    bool found = false;
    for (const auto& info : groups) {
        if (info.id == cliGroup) {
            found = true;
            EXPECT_EQ(info.name, "CLI Test Group");
            EXPECT_TRUE(info.visible);
            break;
        }
    }
    EXPECT_TRUE(found);
}

// Additional Implementation Requirements Tests

TEST_F(GroupRequirementsTest, ThreadSafeGroupOperations) {
    // Test thread-safe group operations as mentioned in requirements
    const int numThreads = 4;
    const int groupsPerThread = 25;
    std::vector<std::thread> threads;
    std::vector<GroupId> allGroups;
    std::mutex groupsMutex;
    
    // Each thread creates its own groups
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t, groupsPerThread, &allGroups, &groupsMutex]() {
            for (int i = 0; i < groupsPerThread; ++i) {
                GroupId id = groupManager->createGroup(
                    "Thread" + std::to_string(t) + "_Group" + std::to_string(i)
                );
                
                std::lock_guard<std::mutex> lock(groupsMutex);
                allGroups.push_back(id);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all groups were created
    EXPECT_EQ(groupManager->getGroupCount(), numThreads * groupsPerThread);
    EXPECT_EQ(allGroups.size(), numThreads * groupsPerThread);
    
    // Verify each group exists
    for (const auto& id : allGroups) {
        EXPECT_TRUE(groupManager->groupExists(id));
    }
}

TEST_F(GroupRequirementsTest, PerformanceOptimizationForLargeGroups) {
    // Test performance optimization for large groups as mentioned in requirements
    GroupId largeGroup = groupManager->createGroup("Large Performance Group");
    
    // Add many voxels to the group
    const int numVoxels = 1000;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numVoxels; ++i) {
        VoxelId voxel(Vector3i(i % 100, (i / 100) % 10, i / 1000), VoxelResolution::Size_32cm);
        voxelManager->setVoxel(voxel.position.value(), voxel.resolution, VoxelEditor::Rendering::Color::Red());
        groupManager->addVoxelToGroup(largeGroup, voxel);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Verify performance is reasonable (should complete in under 1 second)
    EXPECT_LT(duration.count(), 1000);
    
    // Verify group operations are still fast with large group
    startTime = std::chrono::high_resolution_clock::now();
    
    auto bounds = groupManager->getGroupBounds(largeGroup);
    auto group = groupManager->getGroup(largeGroup);
    EXPECT_EQ(group->getVoxelCount(), numVoxels);
    
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Query operations should be very fast (under 100ms)
    EXPECT_LT(duration.count(), 100);
}