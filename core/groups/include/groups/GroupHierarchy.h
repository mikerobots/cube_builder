#pragma once

#include "GroupTypes.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>

namespace VoxelEditor {
namespace Groups {

class GroupHierarchy {
public:
    GroupHierarchy() = default;
    ~GroupHierarchy() = default;
    
    // Hierarchy management
    bool addChild(GroupId parent, GroupId child);
    bool removeChild(GroupId parent, GroupId child);
    bool setParent(GroupId child, GroupId parent);
    void removeFromHierarchy(GroupId groupId);
    
    // Hierarchy queries
    GroupId getParent(GroupId child) const;
    std::vector<GroupId> getChildren(GroupId parent) const;
    std::vector<GroupId> getAllDescendants(GroupId parent) const;
    std::vector<GroupId> getAllAncestors(GroupId child) const;
    std::vector<GroupId> getRootGroups() const;
    
    // Relationship checks
    bool hasParent(GroupId child) const;
    bool hasChildren(GroupId parent) const;
    bool isAncestor(GroupId ancestor, GroupId descendant) const;
    bool isDescendant(GroupId descendant, GroupId ancestor) const;
    bool wouldCreateCycle(GroupId parent, GroupId child) const;
    
    // Hierarchy information
    int getDepth(GroupId groupId) const;
    int getMaxDepth() const;
    size_t getTotalGroups() const;
    
    // Validation
    bool isValid() const;
    std::vector<GroupId> findOrphans(const std::unordered_set<GroupId>& allGroups) const;
    std::vector<std::pair<GroupId, GroupId>> findCycles() const;
    
    // Serialization support
    struct HierarchyData {
        std::unordered_map<GroupId, GroupId> parentMap;
        std::unordered_map<GroupId, std::vector<GroupId>> childrenMap;
    };
    
    HierarchyData exportData() const;
    void importData(const HierarchyData& data);
    
    // Thread safety
    std::mutex& getMutex() const { return m_mutex; }
    
private:
    std::unordered_map<GroupId, GroupId> m_parentMap;
    std::unordered_map<GroupId, std::vector<GroupId>> m_childrenMap;
    mutable std::mutex m_mutex;
    
    void getAllDescendantsRecursive(GroupId parent, std::unordered_set<GroupId>& visited, 
                                   std::vector<GroupId>& result) const;
    void getAllAncestorsRecursive(GroupId child, std::unordered_set<GroupId>& visited,
                                 std::vector<GroupId>& result) const;
    bool hasCycleRecursive(GroupId start, GroupId current, std::unordered_set<GroupId>& visited,
                          std::unordered_set<GroupId>& recursionStack) const;
};

} // namespace Groups
} // namespace VoxelEditor