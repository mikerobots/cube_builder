#include "../include/groups/GroupHierarchy.h"
#include <algorithm>
#include <queue>

namespace VoxelEditor {
namespace Groups {

bool GroupHierarchy::addChild(GroupId parent, GroupId child) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check for cycles
    if (wouldCreateCycle(parent, child)) {
        return false;
    }
    
    // Remove child from its current parent if it has one
    auto parentIt = m_parentMap.find(child);
    if (parentIt != m_parentMap.end()) {
        GroupId oldParent = parentIt->second;
        removeChild(oldParent, child);
    }
    
    // Set new parent-child relationship
    m_parentMap[child] = parent;
    m_childrenMap[parent].push_back(child);
    
    return true;
}

bool GroupHierarchy::removeChild(GroupId parent, GroupId child) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto childrenIt = m_childrenMap.find(parent);
    if (childrenIt != m_childrenMap.end()) {
        auto& children = childrenIt->second;
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            children.erase(it);
            m_parentMap.erase(child);
            
            // Clean up empty children vector
            if (children.empty()) {
                m_childrenMap.erase(childrenIt);
            }
            return true;
        }
    }
    
    return false;
}

bool GroupHierarchy::setParent(GroupId child, GroupId parent) {
    if (parent == INVALID_GROUP_ID) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto parentIt = m_parentMap.find(child);
        if (parentIt != m_parentMap.end()) {
            removeChild(parentIt->second, child);
            return true;
        }
        return false;
    }
    
    return addChild(parent, child);
}

void GroupHierarchy::removeFromHierarchy(GroupId groupId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Remove as child from parent
    auto parentIt = m_parentMap.find(groupId);
    if (parentIt != m_parentMap.end()) {
        GroupId parent = parentIt->second;
        auto childrenIt = m_childrenMap.find(parent);
        if (childrenIt != m_childrenMap.end()) {
            auto& children = childrenIt->second;
            children.erase(std::remove(children.begin(), children.end(), groupId), children.end());
            if (children.empty()) {
                m_childrenMap.erase(childrenIt);
            }
        }
        m_parentMap.erase(parentIt);
    }
    
    // Move children to parent or make them root groups
    auto childrenIt = m_childrenMap.find(groupId);
    if (childrenIt != m_childrenMap.end()) {
        for (GroupId child : childrenIt->second) {
            m_parentMap.erase(child);
        }
        m_childrenMap.erase(childrenIt);
    }
}

GroupId GroupHierarchy::getParent(GroupId child) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_parentMap.find(child);
    return (it != m_parentMap.end()) ? it->second : INVALID_GROUP_ID;
}

std::vector<GroupId> GroupHierarchy::getChildren(GroupId parent) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_childrenMap.find(parent);
    return (it != m_childrenMap.end()) ? it->second : std::vector<GroupId>();
}

std::vector<GroupId> GroupHierarchy::getAllDescendants(GroupId parent) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<GroupId> result;
    std::unordered_set<GroupId> visited;
    getAllDescendantsRecursive(parent, visited, result);
    return result;
}

std::vector<GroupId> GroupHierarchy::getAllAncestors(GroupId child) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<GroupId> result;
    std::unordered_set<GroupId> visited;
    getAllAncestorsRecursive(child, visited, result);
    return result;
}

std::vector<GroupId> GroupHierarchy::getRootGroups() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<GroupId> roots;
    
    // Find all groups that appear in children map but not in parent map
    for (const auto& [parent, children] : m_childrenMap) {
        if (m_parentMap.find(parent) == m_parentMap.end()) {
            roots.push_back(parent);
        }
    }
    
    return roots;
}

bool GroupHierarchy::hasParent(GroupId child) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_parentMap.find(child) != m_parentMap.end();
}

bool GroupHierarchy::hasChildren(GroupId parent) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_childrenMap.find(parent);
    return it != m_childrenMap.end() && !it->second.empty();
}

bool GroupHierarchy::isAncestor(GroupId ancestor, GroupId descendant) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    GroupId current = descendant;
    std::unordered_set<GroupId> visited;
    
    while (current != INVALID_GROUP_ID) {
        if (visited.count(current)) {
            // Cycle detected
            break;
        }
        visited.insert(current);
        
        auto it = m_parentMap.find(current);
        if (it == m_parentMap.end()) {
            break;
        }
        
        current = it->second;
        if (current == ancestor) {
            return true;
        }
    }
    
    return false;
}

bool GroupHierarchy::isDescendant(GroupId descendant, GroupId ancestor) const {
    return isAncestor(ancestor, descendant);
}

bool GroupHierarchy::wouldCreateCycle(GroupId parent, GroupId child) const {
    // A cycle would be created if parent is a descendant of child
    return isAncestor(child, parent);
}

int GroupHierarchy::getDepth(GroupId groupId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    int depth = 0;
    GroupId current = groupId;
    std::unordered_set<GroupId> visited;
    
    while (current != INVALID_GROUP_ID) {
        if (visited.count(current)) {
            // Cycle detected
            return -1;
        }
        visited.insert(current);
        
        auto it = m_parentMap.find(current);
        if (it == m_parentMap.end()) {
            break;
        }
        
        current = it->second;
        depth++;
    }
    
    return depth;
}

int GroupHierarchy::getMaxDepth() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    int maxDepth = 0;
    
    for (const auto& [groupId, parent] : m_parentMap) {
        int depth = getDepth(groupId);
        if (depth > maxDepth) {
            maxDepth = depth;
        }
    }
    
    return maxDepth;
}

size_t GroupHierarchy::getTotalGroups() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::unordered_set<GroupId> allGroups;
    
    for (const auto& [child, parent] : m_parentMap) {
        allGroups.insert(child);
        allGroups.insert(parent);
    }
    
    for (const auto& [parent, children] : m_childrenMap) {
        allGroups.insert(parent);
        for (GroupId child : children) {
            allGroups.insert(child);
        }
    }
    
    return allGroups.size();
}

bool GroupHierarchy::isValid() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check for cycles
    auto cycles = findCycles();
    if (!cycles.empty()) {
        return false;
    }
    
    // Check consistency between parent and children maps
    for (const auto& [child, parent] : m_parentMap) {
        auto childrenIt = m_childrenMap.find(parent);
        if (childrenIt == m_childrenMap.end()) {
            return false;
        }
        
        const auto& children = childrenIt->second;
        if (std::find(children.begin(), children.end(), child) == children.end()) {
            return false;
        }
    }
    
    for (const auto& [parent, children] : m_childrenMap) {
        for (GroupId child : children) {
            auto parentIt = m_parentMap.find(child);
            if (parentIt == m_parentMap.end() || parentIt->second != parent) {
                return false;
            }
        }
    }
    
    return true;
}

std::vector<GroupId> GroupHierarchy::findOrphans(const std::unordered_set<GroupId>& allGroups) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<GroupId> orphans;
    
    // Find groups referenced in hierarchy but not in allGroups
    std::unordered_set<GroupId> hierarchyGroups;
    for (const auto& [child, parent] : m_parentMap) {
        hierarchyGroups.insert(child);
        hierarchyGroups.insert(parent);
    }
    
    for (GroupId groupId : hierarchyGroups) {
        if (allGroups.find(groupId) == allGroups.end()) {
            orphans.push_back(groupId);
        }
    }
    
    return orphans;
}

std::vector<std::pair<GroupId, GroupId>> GroupHierarchy::findCycles() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::pair<GroupId, GroupId>> cycles;
    std::unordered_set<GroupId> visited;
    std::unordered_set<GroupId> recursionStack;
    
    for (const auto& [groupId, parent] : m_parentMap) {
        if (visited.find(groupId) == visited.end()) {
            if (hasCycleRecursive(groupId, groupId, visited, recursionStack)) {
                cycles.emplace_back(groupId, parent);
            }
        }
    }
    
    return cycles;
}

GroupHierarchy::HierarchyData GroupHierarchy::exportData() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    HierarchyData data;
    data.parentMap = m_parentMap;
    data.childrenMap = m_childrenMap;
    return data;
}

void GroupHierarchy::importData(const HierarchyData& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_parentMap = data.parentMap;
    m_childrenMap = data.childrenMap;
}

void GroupHierarchy::getAllDescendantsRecursive(GroupId parent, 
                                               std::unordered_set<GroupId>& visited,
                                               std::vector<GroupId>& result) const {
    auto it = m_childrenMap.find(parent);
    if (it == m_childrenMap.end()) {
        return;
    }
    
    for (GroupId child : it->second) {
        if (visited.count(child)) {
            continue; // Avoid infinite recursion on cycles
        }
        
        visited.insert(child);
        result.push_back(child);
        getAllDescendantsRecursive(child, visited, result);
    }
}

void GroupHierarchy::getAllAncestorsRecursive(GroupId child,
                                             std::unordered_set<GroupId>& visited,
                                             std::vector<GroupId>& result) const {
    auto it = m_parentMap.find(child);
    if (it == m_parentMap.end()) {
        return;
    }
    
    GroupId parent = it->second;
    if (visited.count(parent)) {
        return; // Avoid infinite recursion on cycles
    }
    
    visited.insert(parent);
    result.push_back(parent);
    getAllAncestorsRecursive(parent, visited, result);
}

bool GroupHierarchy::hasCycleRecursive(GroupId start, GroupId current,
                                      std::unordered_set<GroupId>& visited,
                                      std::unordered_set<GroupId>& recursionStack) const {
    visited.insert(current);
    recursionStack.insert(current);
    
    auto it = m_parentMap.find(current);
    if (it != m_parentMap.end()) {
        GroupId parent = it->second;
        
        if (recursionStack.count(parent)) {
            return true; // Cycle found
        }
        
        if (visited.find(parent) == visited.end()) {
            if (hasCycleRecursive(start, parent, visited, recursionStack)) {
                return true;
            }
        }
    }
    
    recursionStack.erase(current);
    return false;
}

} // namespace Groups
} // namespace VoxelEditor