#include <gtest/gtest.h>
#include <thread>
#include "../include/groups/GroupHierarchy.h"

using namespace VoxelEditor::Groups;

class GroupHierarchyTest : public ::testing::Test {
protected:
    void SetUp() override {
        hierarchy = std::make_unique<GroupHierarchy>();
    }
    
    std::unique_ptr<GroupHierarchy> hierarchy;
};

TEST_F(GroupHierarchyTest, EmptyHierarchy) {
    EXPECT_TRUE(hierarchy->getRootGroups().empty());
    EXPECT_EQ(hierarchy->getTotalGroups(), 0);
    EXPECT_EQ(hierarchy->getMaxDepth(), 0);
    EXPECT_TRUE(hierarchy->isValid());
}

TEST_F(GroupHierarchyTest, AddChild) {
    // REQ: Group hierarchy support (nested groups)
    GroupId parent = 1;
    GroupId child = 2;
    
    EXPECT_TRUE(hierarchy->addChild(parent, child));
    
    EXPECT_EQ(hierarchy->getParent(child), parent);
    EXPECT_EQ(hierarchy->getChildren(parent).size(), 1);
    EXPECT_EQ(hierarchy->getChildren(parent)[0], child);
}

TEST_F(GroupHierarchyTest, RemoveChild) {
    GroupId parent = 1;
    GroupId child = 2;
    
    hierarchy->addChild(parent, child);
    EXPECT_TRUE(hierarchy->removeChild(parent, child));
    
    EXPECT_EQ(hierarchy->getParent(child), INVALID_GROUP_ID);
    EXPECT_TRUE(hierarchy->getChildren(parent).empty());
}

TEST_F(GroupHierarchyTest, SetParent) {
    // REQ: Group hierarchy support (nested groups)
    GroupId child = 1;
    GroupId parent1 = 2;
    GroupId parent2 = 3;
    
    // Set initial parent
    EXPECT_TRUE(hierarchy->setParent(child, parent1));
    EXPECT_EQ(hierarchy->getParent(child), parent1);
    
    // Change parent
    EXPECT_TRUE(hierarchy->setParent(child, parent2));
    EXPECT_EQ(hierarchy->getParent(child), parent2);
    EXPECT_TRUE(hierarchy->getChildren(parent1).empty());
    EXPECT_EQ(hierarchy->getChildren(parent2).size(), 1);
    
    // Remove parent
    EXPECT_TRUE(hierarchy->setParent(child, INVALID_GROUP_ID));
    EXPECT_EQ(hierarchy->getParent(child), INVALID_GROUP_ID);
    EXPECT_TRUE(hierarchy->getChildren(parent2).empty());
}

TEST_F(GroupHierarchyTest, CycleDetection) {
    // REQ: Group hierarchy support (nested groups)
    GroupId group1 = 1;
    GroupId group2 = 2;
    GroupId group3 = 3;
    
    // Create chain: 1 -> 2 -> 3
    hierarchy->addChild(group1, group2);
    hierarchy->addChild(group2, group3);
    
    // Try to create cycle: 3 -> 1
    EXPECT_FALSE(hierarchy->addChild(group3, group1));
    
    // Should still be able to add non-cyclic relationships
    GroupId group4 = 4;
    EXPECT_TRUE(hierarchy->addChild(group3, group4));
}

TEST_F(GroupHierarchyTest, GetAllDescendants) {
    // REQ: Group hierarchy support (nested groups)
    GroupId root = 1;
    GroupId child1 = 2;
    GroupId child2 = 3;
    GroupId grandchild1 = 4;
    GroupId grandchild2 = 5;
    
    hierarchy->addChild(root, child1);
    hierarchy->addChild(root, child2);
    hierarchy->addChild(child1, grandchild1);
    hierarchy->addChild(child1, grandchild2);
    
    auto descendants = hierarchy->getAllDescendants(root);
    EXPECT_EQ(descendants.size(), 4);
    
    // Check all descendants are present
    EXPECT_TRUE(std::find(descendants.begin(), descendants.end(), child1) != descendants.end());
    EXPECT_TRUE(std::find(descendants.begin(), descendants.end(), child2) != descendants.end());
    EXPECT_TRUE(std::find(descendants.begin(), descendants.end(), grandchild1) != descendants.end());
    EXPECT_TRUE(std::find(descendants.begin(), descendants.end(), grandchild2) != descendants.end());
}

TEST_F(GroupHierarchyTest, GetAllAncestors) {
    // REQ: Group hierarchy support (nested groups)
    GroupId root = 1;
    GroupId parent = 2;
    GroupId child = 3;
    GroupId grandchild = 4;
    
    hierarchy->addChild(root, parent);
    hierarchy->addChild(parent, child);
    hierarchy->addChild(child, grandchild);
    
    auto ancestors = hierarchy->getAllAncestors(grandchild);
    EXPECT_EQ(ancestors.size(), 3);
    
    // Check all ancestors are present
    EXPECT_TRUE(std::find(ancestors.begin(), ancestors.end(), child) != ancestors.end());
    EXPECT_TRUE(std::find(ancestors.begin(), ancestors.end(), parent) != ancestors.end());
    EXPECT_TRUE(std::find(ancestors.begin(), ancestors.end(), root) != ancestors.end());
}

TEST_F(GroupHierarchyTest, GetRootGroups) {
    // REQ: Group hierarchy support (nested groups)
    // REQ-9.2.5: CLI shall support group commands (group create/hide/show/list)
    GroupId root1 = 1;
    GroupId root2 = 2;
    GroupId child1 = 3;
    GroupId child2 = 4;
    
    hierarchy->addChild(root1, child1);
    hierarchy->addChild(root2, child2);
    
    auto roots = hierarchy->getRootGroups();
    EXPECT_EQ(roots.size(), 2);
    EXPECT_TRUE(std::find(roots.begin(), roots.end(), root1) != roots.end());
    EXPECT_TRUE(std::find(roots.begin(), roots.end(), root2) != roots.end());
}

TEST_F(GroupHierarchyTest, IsAncestor) {
    GroupId root = 1;
    GroupId parent = 2;
    GroupId child = 3;
    
    hierarchy->addChild(root, parent);
    hierarchy->addChild(parent, child);
    
    EXPECT_TRUE(hierarchy->isAncestor(root, child));
    EXPECT_TRUE(hierarchy->isAncestor(parent, child));
    EXPECT_FALSE(hierarchy->isAncestor(child, root));
    EXPECT_FALSE(hierarchy->isAncestor(child, parent));
}

TEST_F(GroupHierarchyTest, IsDescendant) {
    GroupId root = 1;
    GroupId parent = 2;
    GroupId child = 3;
    
    hierarchy->addChild(root, parent);
    hierarchy->addChild(parent, child);
    
    EXPECT_TRUE(hierarchy->isDescendant(child, root));
    EXPECT_TRUE(hierarchy->isDescendant(child, parent));
    EXPECT_FALSE(hierarchy->isDescendant(root, child));
    EXPECT_FALSE(hierarchy->isDescendant(parent, child));
}

TEST_F(GroupHierarchyTest, GetDepth) {
    GroupId root = 1;
    GroupId level1 = 2;
    GroupId level2 = 3;
    GroupId level3 = 4;
    
    hierarchy->addChild(root, level1);
    hierarchy->addChild(level1, level2);
    hierarchy->addChild(level2, level3);
    
    EXPECT_EQ(hierarchy->getDepth(root), 0);
    EXPECT_EQ(hierarchy->getDepth(level1), 1);
    EXPECT_EQ(hierarchy->getDepth(level2), 2);
    EXPECT_EQ(hierarchy->getDepth(level3), 3);
}

TEST_F(GroupHierarchyTest, GetMaxDepth) {
    GroupId root = 1;
    GroupId branch1 = 2;
    GroupId branch2 = 3;
    GroupId deep1 = 4;
    GroupId deep2 = 5;
    GroupId deep3 = 6;
    
    // Create two branches with different depths
    hierarchy->addChild(root, branch1);
    hierarchy->addChild(root, branch2);
    hierarchy->addChild(branch1, deep1);
    hierarchy->addChild(deep1, deep2);
    hierarchy->addChild(deep2, deep3);
    
    EXPECT_EQ(hierarchy->getMaxDepth(), 4);
}

TEST_F(GroupHierarchyTest, RemoveFromHierarchy) {
    GroupId parent = 1;
    GroupId middle = 2;
    GroupId child1 = 3;
    GroupId child2 = 4;
    
    hierarchy->addChild(parent, middle);
    hierarchy->addChild(middle, child1);
    hierarchy->addChild(middle, child2);
    
    hierarchy->removeFromHierarchy(middle);
    
    // Middle should be removed
    EXPECT_EQ(hierarchy->getParent(middle), INVALID_GROUP_ID);
    EXPECT_TRUE(hierarchy->getChildren(middle).empty());
    
    // Children should be orphaned
    EXPECT_EQ(hierarchy->getParent(child1), INVALID_GROUP_ID);
    EXPECT_EQ(hierarchy->getParent(child2), INVALID_GROUP_ID);
}

TEST_F(GroupHierarchyTest, HasParentAndChildren) {
    GroupId parent = 1;
    GroupId child = 2;
    GroupId orphan = 3;
    
    hierarchy->addChild(parent, child);
    
    EXPECT_TRUE(hierarchy->hasChildren(parent));
    EXPECT_FALSE(hierarchy->hasChildren(child));
    EXPECT_FALSE(hierarchy->hasChildren(orphan));
    
    EXPECT_FALSE(hierarchy->hasParent(parent));
    EXPECT_TRUE(hierarchy->hasParent(child));
    EXPECT_FALSE(hierarchy->hasParent(orphan));
}

TEST_F(GroupHierarchyTest, GetTotalGroups) {
    GroupId group1 = 1;
    GroupId group2 = 2;
    GroupId group3 = 3;
    GroupId group4 = 4;
    
    hierarchy->addChild(group1, group2);
    hierarchy->addChild(group2, group3);
    hierarchy->addChild(group1, group4);
    
    EXPECT_EQ(hierarchy->getTotalGroups(), 4);
}

TEST_F(GroupHierarchyTest, CycleDetectionComplex) {
    // Test more complex cycle scenarios
    GroupId a = 1, b = 2, c = 3, d = 4, e = 5;
    
    // Create hierarchy structure
    // Note: A child can only have one parent, so addChild(c, d) will move d from b to c
    hierarchy->addChild(a, b);
    hierarchy->addChild(a, c);
    hierarchy->addChild(b, d);
    // This moves d from b to c, so structure is now: a->b and a->c->d
    hierarchy->addChild(c, d);
    hierarchy->addChild(d, e);
    
    // Structure is now: a->b, a->c->d->e
    
    // Try to create various cycles
    EXPECT_FALSE(hierarchy->addChild(e, a)); // Back to root would create e->a->c->d->e
    EXPECT_TRUE(hierarchy->addChild(d, b)); // This is allowed: a->c->d->b (no cycle)
    EXPECT_FALSE(hierarchy->addChild(e, c)); // Would create cycle: c->d->e->c
}

TEST_F(GroupHierarchyTest, FindOrphans) {
    std::unordered_set<GroupId> validGroups = {1, 2, 3};
    
    // Add some relationships
    hierarchy->addChild(1, 2);
    hierarchy->addChild(2, 3);
    hierarchy->addChild(4, 5); // 4 and 5 are not in validGroups
    
    auto orphans = hierarchy->findOrphans(validGroups);
    EXPECT_EQ(orphans.size(), 2);
    EXPECT_TRUE(std::find(orphans.begin(), orphans.end(), 4) != orphans.end());
    EXPECT_TRUE(std::find(orphans.begin(), orphans.end(), 5) != orphans.end());
}

TEST_F(GroupHierarchyTest, FindCycles) {
    // Create a hierarchy with a cycle
    hierarchy->addChild(1, 2);
    hierarchy->addChild(2, 3);
    
    // Force a cycle by manipulating internal data (normally prevented)
    GroupHierarchy::HierarchyData data = hierarchy->exportData();
    data.parentMap[1] = 3; // Create cycle: 1->2->3->1
    hierarchy->importData(data);
    
    auto cycles = hierarchy->findCycles();
    EXPECT_FALSE(cycles.empty());
}

TEST_F(GroupHierarchyTest, ExportImport) {
    // REQ-8.1.8: Format shall store group definitions and metadata
    // REQ: Group persistence across save/load operations
    // Create a complex hierarchy
    hierarchy->addChild(1, 2);
    hierarchy->addChild(1, 3);
    hierarchy->addChild(2, 4);
    hierarchy->addChild(3, 5);
    
    // Export data
    auto data = hierarchy->exportData();
    
    // Create new hierarchy and import
    GroupHierarchy newHierarchy;
    newHierarchy.importData(data);
    
    // Verify structure is preserved
    EXPECT_EQ(newHierarchy.getParent(2), 1);
    EXPECT_EQ(newHierarchy.getParent(3), 1);
    EXPECT_EQ(newHierarchy.getParent(4), 2);
    EXPECT_EQ(newHierarchy.getParent(5), 3);
    EXPECT_EQ(newHierarchy.getChildren(1).size(), 2);
}

TEST_F(GroupHierarchyTest, DISABLED_ThreadSafety) {
    // Basic thread safety test
    const int numThreads = 4;
    const int opsPerThread = 100;
    std::vector<std::thread> threads;
    
    // Each thread adds its own branch
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t, opsPerThread]() {
            GroupId base = 1000 * (t + 1);
            for (int i = 0; i < opsPerThread; ++i) {
                hierarchy->addChild(base + i, base + i + 1);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify hierarchy is still valid
    EXPECT_TRUE(hierarchy->isValid());
}