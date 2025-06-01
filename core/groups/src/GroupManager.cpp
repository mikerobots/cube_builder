#include "../include/groups/GroupManager.h"
#include <algorithm>
#include <sstream>

namespace VoxelEditor {
namespace Groups {

GroupManager::GroupManager(VoxelData::VoxelDataManager* voxelManager,
                         Events::EventDispatcher* eventDispatcher)
    : m_voxelManager(voxelManager)
    , m_eventDispatcher(eventDispatcher)
    , m_hierarchy(std::make_unique<GroupHierarchy>()) {
}

GroupManager::~GroupManager() = default;

GroupId GroupManager::createGroup(const std::string& name, const std::vector<VoxelId>& voxels) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    GroupId id = generateGroupId();
    std::string uniqueName = name.empty() ? generateUniqueGroupName("Group") : name;
    
    auto group = std::make_unique<VoxelGroup>(id, uniqueName);
    
    // Set a color from the palette
    group->setColor(assignGroupColor());
    
    // Add voxels if provided
    for (const auto& voxel : voxels) {
        group->addVoxel(voxel);
        updateVoxelMapping(id, voxel, true);
    }
    
    m_groups[id] = std::move(group);
    
    dispatchGroupCreated(id, uniqueName, voxels);
    
    return id;
}

bool GroupManager::deleteGroup(GroupId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it == m_groups.end()) {
        return false;
    }
    
    // Get group info before deletion
    std::string name = it->second->getName();
    auto voxels = it->second->getVoxelList();
    
    // Remove from hierarchy
    m_hierarchy->removeFromHierarchy(id);
    
    // Remove from voxel mapping
    removeFromVoxelMapping(id);
    
    // Delete the group
    m_groups.erase(it);
    
    dispatchGroupDeleted(id, name, voxels);
    
    return true;
}

bool GroupManager::renameGroup(GroupId id, const std::string& newName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it == m_groups.end()) {
        return false;
    }
    
    std::string oldName = it->second->getName();
    it->second->setName(newName);
    
    if (m_eventDispatcher) {
        GroupModifiedEvent event;
        event.groupId = id;
        event.type = GroupModificationType::Renamed;
        event.oldName = oldName;
        event.newName = newName;
        m_eventDispatcher->dispatch(event);
    }
    
    return true;
}

VoxelGroup* GroupManager::getGroup(GroupId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_groups.find(id);
    return (it != m_groups.end()) ? it->second.get() : nullptr;
}

const VoxelGroup* GroupManager::getGroup(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_groups.find(id);
    return (it != m_groups.end()) ? it->second.get() : nullptr;
}

std::vector<GroupId> GroupManager::getAllGroupIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<GroupId> ids;
    ids.reserve(m_groups.size());
    
    for (const auto& [id, group] : m_groups) {
        ids.push_back(id);
    }
    
    return ids;
}

bool GroupManager::groupExists(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_groups.find(id) != m_groups.end();
}

bool GroupManager::addVoxelToGroup(GroupId id, const VoxelId& voxel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it == m_groups.end()) {
        return false;
    }
    
    if (it->second->addVoxel(voxel)) {
        updateVoxelMapping(id, voxel, true);
        return true;
    }
    
    return false;
}

bool GroupManager::removeVoxelFromGroup(GroupId id, const VoxelId& voxel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it == m_groups.end()) {
        return false;
    }
    
    if (it->second->removeVoxel(voxel)) {
        updateVoxelMapping(id, voxel, false);
        return true;
    }
    
    return false;
}

std::vector<VoxelId> GroupManager::getGroupVoxels(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it == m_groups.end()) {
        return {};
    }
    
    return it->second->getVoxelList();
}

GroupId GroupManager::findGroupContaining(const VoxelId& voxel) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_voxelToGroups.find(voxel);
    if (it != m_voxelToGroups.end() && !it->second.empty()) {
        return it->second.front();
    }
    
    return INVALID_GROUP_ID;
}

std::vector<GroupId> GroupManager::findGroupsContaining(const VoxelId& voxel) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_voxelToGroups.find(voxel);
    if (it != m_voxelToGroups.end()) {
        return it->second;
    }
    
    return {};
}

void GroupManager::hideGroup(GroupId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it != m_groups.end()) {
        bool oldVisible = it->second->isVisible();
        it->second->setVisible(false);
        
        if (oldVisible && m_eventDispatcher) {
            GroupModifiedEvent event;
            event.groupId = id;
            event.type = GroupModificationType::VisibilityChanged;
            event.oldVisible = oldVisible;
            event.newVisible = false;
            m_eventDispatcher->dispatch(event);
        }
    }
}

void GroupManager::showGroup(GroupId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it != m_groups.end()) {
        bool oldVisible = it->second->isVisible();
        it->second->setVisible(true);
        
        if (!oldVisible && m_eventDispatcher) {
            GroupModifiedEvent event;
            event.groupId = id;
            event.type = GroupModificationType::VisibilityChanged;
            event.oldVisible = oldVisible;
            event.newVisible = true;
            m_eventDispatcher->dispatch(event);
        }
    }
}

bool GroupManager::isGroupVisible(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    return (it != m_groups.end()) ? it->second->isVisible() : true;
}

void GroupManager::setGroupOpacity(GroupId id, float opacity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it != m_groups.end()) {
        it->second->setOpacity(opacity);
        dispatchGroupModified(id, GroupModificationType::PropertiesChanged);
    }
}

float GroupManager::getGroupOpacity(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    return (it != m_groups.end()) ? it->second->getOpacity() : 1.0f;
}

void GroupManager::setGroupColor(GroupId id, const Rendering::Color& color) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it != m_groups.end()) {
        it->second->setColor(color);
        dispatchGroupModified(id, GroupModificationType::PropertiesChanged);
    }
}

Rendering::Color GroupManager::getGroupColor(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    return (it != m_groups.end()) ? it->second->getColor() : Rendering::Color::White();
}

void GroupManager::lockGroup(GroupId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it != m_groups.end()) {
        it->second->setLocked(true);
        dispatchGroupModified(id, GroupModificationType::LockChanged);
    }
}

void GroupManager::unlockGroup(GroupId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it != m_groups.end()) {
        it->second->setLocked(false);
        dispatchGroupModified(id, GroupModificationType::LockChanged);
    }
}

bool GroupManager::isGroupLocked(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    return (it != m_groups.end()) ? it->second->isLocked() : false;
}

bool GroupManager::moveGroup(GroupId id, const Math::Vector3f& offset) {
    auto operation = createMoveOperation(id, offset);
    if (operation && operation->execute()) {
        dispatchGroupModified(id, GroupModificationType::Moved);
        return true;
    }
    return false;
}

GroupId GroupManager::copyGroup(GroupId id, const std::string& newName, 
                               const Math::Vector3f& offset) {
    std::string name = newName.empty() ? generateUniqueGroupName("Copy") : newName;
    auto operation = createCopyOperation(id, name, offset);
    
    if (operation && operation->execute()) {
        auto copyOp = static_cast<CopyGroupOperation*>(operation.get());
        GroupId newId = copyOp->getCreatedGroupId();
        dispatchGroupCreated(newId, name, getGroupVoxels(newId));
        return newId;
    }
    
    return INVALID_GROUP_ID;
}

bool GroupManager::rotateGroup(GroupId id, const Math::Vector3f& eulerAngles,
                             const Math::Vector3f& pivot) {
    auto operation = createRotateOperation(id, eulerAngles, pivot);
    if (operation && operation->execute()) {
        dispatchGroupModified(id, GroupModificationType::Rotated);
        return true;
    }
    return false;
}

bool GroupManager::scaleGroup(GroupId id, float scaleFactor,
                            const Math::Vector3f& pivot) {
    auto operation = createScaleOperation(id, scaleFactor, pivot);
    if (operation && operation->execute()) {
        dispatchGroupModified(id, GroupModificationType::Scaled);
        return true;
    }
    return false;
}

GroupId GroupManager::mergeGroups(const std::vector<GroupId>& sourceIds, 
                                const std::string& targetName) {
    auto operation = std::make_unique<MergeGroupsOperation>(this, sourceIds, targetName);
    
    if (operation && operation->execute()) {
        auto mergeOp = static_cast<MergeGroupsOperation*>(operation.get());
        GroupId targetId = mergeOp->getTargetGroupId();
        dispatchGroupCreated(targetId, targetName, getGroupVoxels(targetId));
        return targetId;
    }
    
    return INVALID_GROUP_ID;
}

std::vector<GroupId> GroupManager::splitGroup(GroupId sourceId,
                                            const std::vector<std::vector<VoxelId>>& voxelSets,
                                            const std::vector<std::string>& newNames) {
    auto operation = std::make_unique<SplitGroupOperation>(this, sourceId, voxelSets, newNames);
    
    if (operation && operation->execute()) {
        auto splitOp = static_cast<SplitGroupOperation*>(operation.get());
        auto createdIds = splitOp->getCreatedGroupIds();
        
        for (size_t i = 0; i < createdIds.size(); ++i) {
            dispatchGroupCreated(createdIds[i], newNames[i], voxelSets[i]);
        }
        
        return createdIds;
    }
    
    return {};
}

bool GroupManager::setParentGroup(GroupId child, GroupId parent) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!groupExists(child) || (parent != INVALID_GROUP_ID && !groupExists(parent))) {
        return false;
    }
    
    return m_hierarchy->setParent(child, parent);
}

GroupId GroupManager::getParentGroup(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hierarchy->getParent(id);
}

std::vector<GroupId> GroupManager::getChildGroups(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hierarchy->getChildren(id);
}

std::vector<GroupId> GroupManager::getRootGroups() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hierarchy->getRootGroups();
}

std::vector<GroupId> GroupManager::getAllDescendants(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hierarchy->getAllDescendants(id);
}

bool GroupManager::isAncestor(GroupId ancestor, GroupId descendant) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hierarchy->isAncestor(ancestor, descendant);
}

std::vector<GroupInfo> GroupManager::listGroups() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<GroupInfo> infos;
    infos.reserve(m_groups.size());
    
    for (const auto& [id, group] : m_groups) {
        auto info = group->getInfo();
        info.parentId = m_hierarchy->getParent(id);
        info.childIds = m_hierarchy->getChildren(id);
        infos.push_back(info);
    }
    
    return infos;
}

std::vector<GroupId> GroupManager::findGroupsByName(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<GroupId> results;
    
    for (const auto& [id, group] : m_groups) {
        if (group->getName().find(name) != std::string::npos) {
            results.push_back(id);
        }
    }
    
    return results;
}

std::vector<GroupId> GroupManager::findGroupsByPredicate(const GroupPredicate& predicate) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<GroupId> results;
    
    for (const auto& [id, group] : m_groups) {
        if (predicate(*group)) {
            results.push_back(id);
        }
    }
    
    return results;
}

std::vector<GroupId> GroupManager::getVisibleGroups() const {
    return findGroupsByPredicate([](const VoxelGroup& group) {
        return group.isVisible();
    });
}

std::vector<GroupId> GroupManager::getLockedGroups() const {
    return findGroupsByPredicate([](const VoxelGroup& group) {
        return group.isLocked();
    });
}

void GroupManager::forEachGroup(const GroupVisitor& visitor) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& [id, group] : m_groups) {
        visitor(*group);
    }
}

void GroupManager::forEachGroupInHierarchy(GroupId rootId, const GroupVisitor& visitor) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(rootId);
    if (it != m_groups.end()) {
        visitor(*it->second);
        
        auto children = m_hierarchy->getChildren(rootId);
        for (GroupId childId : children) {
            forEachGroupInHierarchy(childId, visitor);
        }
    }
}

GroupStats GroupManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    GroupStats stats;
    
    stats.totalGroups = m_groups.size();
    stats.totalVoxels = 0;
    stats.maxGroupSize = 0;
    
    for (const auto& [id, group] : m_groups) {
        size_t voxelCount = group->getVoxelCount();
        stats.totalVoxels += voxelCount;
        stats.maxGroupSize = std::max(stats.maxGroupSize, voxelCount);
    }
    
    stats.averageGroupSize = (stats.totalGroups > 0) ? 
        static_cast<float>(stats.totalVoxels) / stats.totalGroups : 0.0f;
    
    stats.maxHierarchyDepth = m_hierarchy->getMaxDepth();
    
    // Estimate memory usage
    stats.memoryUsage = sizeof(GroupManager) + 
                       m_groups.size() * (sizeof(GroupId) + sizeof(std::unique_ptr<VoxelGroup>)) +
                       stats.totalVoxels * sizeof(VoxelId);
    
    return stats;
}

size_t GroupManager::getTotalVoxelCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    
    for (const auto& [id, group] : m_groups) {
        count += group->getVoxelCount();
    }
    
    return count;
}

size_t GroupManager::getGroupCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_groups.size();
}

Math::BoundingBox GroupManager::getGroupsBounds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Math::BoundingBox bounds;
    bool first = true;
    
    for (const auto& [id, group] : m_groups) {
        if (!group->isEmpty()) {
            auto groupBounds = group->getBoundingBox();
            if (first) {
                bounds = groupBounds;
                first = false;
            } else {
                bounds = bounds.merge(groupBounds);
            }
        }
    }
    
    return bounds;
}

Math::BoundingBox GroupManager::getGroupBounds(GroupId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_groups.find(id);
    if (it != m_groups.end()) {
        return it->second->getBoundingBox();
    }
    
    return Math::BoundingBox();
}

bool GroupManager::validateGroups() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check hierarchy consistency
    if (!m_hierarchy->isValid()) {
        return false;
    }
    
    // Check voxel mapping consistency
    for (const auto& [voxelId, groupIds] : m_voxelToGroups) {
        for (GroupId groupId : groupIds) {
            auto it = m_groups.find(groupId);
            if (it == m_groups.end() || !it->second->containsVoxel(voxelId)) {
                return false;
            }
        }
    }
    
    // Check all group voxels are in mapping
    for (const auto& [groupId, group] : m_groups) {
        auto voxels = group->getVoxelList();
        for (const auto& voxel : voxels) {
            auto mapIt = m_voxelToGroups.find(voxel);
            if (mapIt == m_voxelToGroups.end()) {
                return false;
            }
            
            const auto& groupIds = mapIt->second;
            if (std::find(groupIds.begin(), groupIds.end(), groupId) == groupIds.end()) {
                return false;
            }
        }
    }
    
    return true;
}

void GroupManager::cleanupEmptyGroups() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<GroupId> toDelete;
    
    for (const auto& [id, group] : m_groups) {
        if (group->isEmpty()) {
            toDelete.push_back(id);
        }
    }
    
    for (GroupId id : toDelete) {
        deleteGroup(id);
    }
}

void GroupManager::cleanupOrphanedVoxels() {
    // This would interact with VoxelDataManager to remove voxels not in any group
    // Implementation depends on the policy - do we allow ungrouped voxels?
}

std::vector<VoxelId> GroupManager::findOrphanedVoxels() const {
    // This would query VoxelDataManager for all voxels and check against groups
    // For now, return empty as we need VoxelDataManager integration
    return {};
}

GroupManager::GroupManagerData GroupManager::exportData() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    GroupManagerData data;
    
    // Export groups
    for (const auto& [id, group] : m_groups) {
        data.groups.emplace_back(id, group->getMetadata());
        data.groupVoxels.emplace_back(id, group->getVoxelList());
    }
    
    // Export hierarchy
    data.hierarchy = m_hierarchy->exportData();
    data.nextGroupId = m_nextGroupId;
    
    return data;
}

bool GroupManager::importData(const GroupManagerData& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clear existing data
    m_groups.clear();
    m_voxelToGroups.clear();
    m_hierarchy = std::make_unique<GroupHierarchy>();
    
    // Import groups
    for (const auto& [id, metadata] : data.groups) {
        auto group = std::make_unique<VoxelGroup>(id, metadata.name);
        group->setMetadata(metadata);
        m_groups[id] = std::move(group);
    }
    
    // Import voxels
    for (const auto& [id, voxels] : data.groupVoxels) {
        auto it = m_groups.find(id);
        if (it != m_groups.end()) {
            for (const auto& voxel : voxels) {
                it->second->addVoxel(voxel);
                updateVoxelMapping(id, voxel, true);
            }
        }
    }
    
    // Import hierarchy
    m_hierarchy->importData(data.hierarchy);
    m_nextGroupId = data.nextGroupId;
    
    return validateGroups();
}

GroupId GroupManager::generateGroupId() {
    return m_nextGroupId++;
}

std::string GroupManager::generateUniqueGroupName(const std::string& baseName) const {
    std::vector<std::string> existingNames;
    for (const auto& [id, group] : m_groups) {
        existingNames.push_back(group->getName());
    }
    
    return GroupOperationUtils::generateUniqueName(baseName, existingNames);
}

Rendering::Color GroupManager::assignGroupColor() const {
    // Use the next color from the palette based on group count
    return GroupColorPalette::getColorForIndex(m_groups.size());
}

void GroupManager::updateVoxelMapping(GroupId groupId, const VoxelId& voxel, bool add) {
    auto& groupIds = m_voxelToGroups[voxel];
    
    if (add) {
        if (std::find(groupIds.begin(), groupIds.end(), groupId) == groupIds.end()) {
            groupIds.push_back(groupId);
        }
    } else {
        groupIds.erase(std::remove(groupIds.begin(), groupIds.end(), groupId), groupIds.end());
        if (groupIds.empty()) {
            m_voxelToGroups.erase(voxel);
        }
    }
}

void GroupManager::removeFromVoxelMapping(GroupId groupId) {
    for (auto& [voxel, groups] : m_voxelToGroups) {
        groups.erase(std::remove(groups.begin(), groups.end(), groupId), groups.end());
    }
    
    // Clean up empty entries
    for (auto it = m_voxelToGroups.begin(); it != m_voxelToGroups.end(); ) {
        if (it->second.empty()) {
            it = m_voxelToGroups.erase(it);
        } else {
            ++it;
        }
    }
}

void GroupManager::dispatchGroupCreated(GroupId groupId, const std::string& name,
                                      const std::vector<VoxelId>& voxels) {
    if (m_eventDispatcher) {
        GroupCreatedEvent event;
        event.groupId = groupId;
        event.name = name;
        event.voxels = voxels;
        m_eventDispatcher->dispatch(event);
    }
}

void GroupManager::dispatchGroupModified(GroupId groupId, GroupModificationType type) {
    if (m_eventDispatcher) {
        GroupModifiedEvent event;
        event.groupId = groupId;
        event.type = type;
        m_eventDispatcher->dispatch(event);
    }
}

void GroupManager::dispatchGroupDeleted(GroupId groupId, const std::string& name,
                                      const std::vector<VoxelId>& voxels) {
    if (m_eventDispatcher) {
        GroupDeletedEvent event;
        event.groupId = groupId;
        event.name = name;
        event.releasedVoxels = voxels;
        m_eventDispatcher->dispatch(event);
    }
}

std::unique_ptr<GroupOperation> GroupManager::createMoveOperation(GroupId groupId, 
                                                                const Math::Vector3f& offset) {
    return std::make_unique<MoveGroupOperation>(this, m_voxelManager, groupId, offset);
}

std::unique_ptr<GroupOperation> GroupManager::createCopyOperation(GroupId sourceId,
                                                                const std::string& newName,
                                                                const Math::Vector3f& offset) {
    return std::make_unique<CopyGroupOperation>(this, m_voxelManager, sourceId, newName, offset);
}

std::unique_ptr<GroupOperation> GroupManager::createRotateOperation(GroupId groupId,
                                                                  const Math::Vector3f& eulerAngles,
                                                                  const Math::Vector3f& pivot) {
    return std::make_unique<RotateGroupOperation>(this, m_voxelManager, groupId, eulerAngles, pivot);
}

std::unique_ptr<GroupOperation> GroupManager::createScaleOperation(GroupId groupId,
                                                                 float scaleFactor,
                                                                 const Math::Vector3f& pivot) {
    return std::make_unique<ScaleGroupOperation>(this, m_voxelManager, groupId, scaleFactor, pivot);
}

// Add this method to the public interface for GroupOperations to use
void GroupManager::updateVoxelGroupMembership(const VoxelId& voxel, GroupId oldGroup, GroupId newGroup) {
    std::lock_guard<std::mutex> lock(m_mutex);
    updateVoxelMapping(oldGroup, voxel, false);
    updateVoxelMapping(newGroup, voxel, true);
}

} // namespace Groups
} // namespace VoxelEditor