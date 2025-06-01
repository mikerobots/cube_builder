#pragma once

#include "GroupTypes.h"
#include "VoxelGroup.h"
#include "GroupHierarchy.h"
#include "GroupOperations.h"
#include "VoxelDataManager.h"
#include "EventDispatcher.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <functional>

namespace VoxelEditor {
namespace Groups {

// Event structures
struct GroupCreatedEvent {
    GroupId groupId;
    std::string name;
    std::vector<VoxelId> voxels;
};

struct GroupModifiedEvent {
    GroupId groupId;
    GroupModificationType type;
    std::string oldName;        // for rename events
    std::string newName;        // for rename events
    bool oldVisible;            // for visibility events
    bool newVisible;            // for visibility events
    Math::Vector3f offset;      // for move events
};

struct GroupDeletedEvent {
    GroupId groupId;
    std::string name;
    std::vector<VoxelId> releasedVoxels;
};

// Group selection and filtering
using GroupPredicate = std::function<bool(const VoxelGroup&)>;
using GroupVisitor = std::function<void(const VoxelGroup&)>;

class GroupManager {
public:
    GroupManager(VoxelData::VoxelDataManager* voxelManager, 
                Events::EventDispatcher* eventDispatcher = nullptr);
    ~GroupManager();
    
    // Group lifecycle
    GroupId createGroup(const std::string& name, const std::vector<VoxelId>& voxels = {});
    bool deleteGroup(GroupId id);
    bool renameGroup(GroupId id, const std::string& newName);
    
    // Group access
    VoxelGroup* getGroup(GroupId id);
    const VoxelGroup* getGroup(GroupId id) const;
    std::vector<GroupId> getAllGroupIds() const;
    bool groupExists(GroupId id) const;
    
    // Group membership
    bool addVoxelToGroup(GroupId id, const VoxelId& voxel);
    bool removeVoxelFromGroup(GroupId id, const VoxelId& voxel);
    std::vector<VoxelId> getGroupVoxels(GroupId id) const;
    GroupId findGroupContaining(const VoxelId& voxel) const;
    std::vector<GroupId> findGroupsContaining(const VoxelId& voxel) const;
    
    // Group visibility
    void hideGroup(GroupId id);
    void showGroup(GroupId id);
    bool isGroupVisible(GroupId id) const;
    void setGroupOpacity(GroupId id, float opacity);
    float getGroupOpacity(GroupId id) const;
    
    // Group properties
    void setGroupColor(GroupId id, const Rendering::Color& color);
    Rendering::Color getGroupColor(GroupId id) const;
    
    // Group locking
    void lockGroup(GroupId id);
    void unlockGroup(GroupId id);
    bool isGroupLocked(GroupId id) const;
    
    // Group operations
    bool moveGroup(GroupId id, const Math::Vector3f& offset);
    GroupId copyGroup(GroupId id, const std::string& newName = "", 
                     const Math::Vector3f& offset = Math::Vector3f(0, 0, 0));
    bool rotateGroup(GroupId id, const Math::Vector3f& eulerAngles, 
                    const Math::Vector3f& pivot = Math::Vector3f(0, 0, 0));
    bool scaleGroup(GroupId id, float scaleFactor, 
                   const Math::Vector3f& pivot = Math::Vector3f(0, 0, 0));
    
    // Batch operations
    GroupId mergeGroups(const std::vector<GroupId>& sourceIds, const std::string& targetName);
    std::vector<GroupId> splitGroup(GroupId sourceId, 
                                   const std::vector<std::vector<VoxelId>>& voxelSets,
                                   const std::vector<std::string>& newNames);
    
    // Hierarchy management
    bool setParentGroup(GroupId child, GroupId parent);
    GroupId getParentGroup(GroupId id) const;
    std::vector<GroupId> getChildGroups(GroupId id) const;
    std::vector<GroupId> getRootGroups() const;
    std::vector<GroupId> getAllDescendants(GroupId id) const;
    bool isAncestor(GroupId ancestor, GroupId descendant) const;
    
    // Group queries
    std::vector<GroupInfo> listGroups() const;
    std::vector<GroupId> findGroupsByName(const std::string& name) const;
    std::vector<GroupId> findGroupsByPredicate(const GroupPredicate& predicate) const;
    std::vector<GroupId> getVisibleGroups() const;
    std::vector<GroupId> getLockedGroups() const;
    
    // Iteration and visitation
    void forEachGroup(const GroupVisitor& visitor) const;
    void forEachGroupInHierarchy(GroupId rootId, const GroupVisitor& visitor) const;
    
    // Statistics and information
    GroupStats getStatistics() const;
    size_t getTotalVoxelCount() const;
    size_t getGroupCount() const;
    Math::BoundingBox getGroupsBounds() const;
    Math::BoundingBox getGroupBounds(GroupId id) const;
    
    // Validation and cleanup
    bool validateGroups() const;
    void cleanupEmptyGroups();
    void cleanupOrphanedVoxels();
    std::vector<VoxelId> findOrphanedVoxels() const;
    
    // Serialization support
    struct GroupManagerData {
        std::vector<std::pair<GroupId, GroupMetadata>> groups;
        std::vector<std::pair<GroupId, std::vector<VoxelId>>> groupVoxels;
        GroupHierarchy::HierarchyData hierarchy;
        GroupId nextGroupId;
    };
    
    GroupManagerData exportData() const;
    bool importData(const GroupManagerData& data);
    
    // Thread safety
    std::mutex& getMutex() const { return m_mutex; }
    
    // Internal method for group operations
    void updateVoxelGroupMembership(const VoxelId& voxel, GroupId oldGroup, GroupId newGroup);
    
private:
    std::unordered_map<GroupId, std::unique_ptr<VoxelGroup>> m_groups;
    std::unique_ptr<GroupHierarchy> m_hierarchy;
    std::unordered_map<VoxelId, std::vector<GroupId>> m_voxelToGroups; // for fast lookup
    
    GroupId m_nextGroupId = 1;
    VoxelData::VoxelDataManager* m_voxelManager;
    Events::EventDispatcher* m_eventDispatcher;
    
    mutable std::mutex m_mutex;
    
    // Helper methods
    GroupId generateGroupId();
    std::string generateUniqueGroupName(const std::string& baseName) const;
    Rendering::Color assignGroupColor() const;
    void updateVoxelMapping(GroupId groupId, const VoxelId& voxel, bool add);
    void removeFromVoxelMapping(GroupId groupId);
    
    // Event dispatching
    void dispatchGroupCreated(GroupId groupId, const std::string& name, 
                             const std::vector<VoxelId>& voxels);
    void dispatchGroupModified(GroupId groupId, GroupModificationType type);
    void dispatchGroupDeleted(GroupId groupId, const std::string& name,
                             const std::vector<VoxelId>& voxels);
    
    // Operation execution
    std::unique_ptr<GroupOperation> createMoveOperation(GroupId groupId, const Math::Vector3f& offset);
    std::unique_ptr<GroupOperation> createCopyOperation(GroupId sourceId, const std::string& newName,
                                                       const Math::Vector3f& offset);
    std::unique_ptr<GroupOperation> createRotateOperation(GroupId groupId, 
                                                         const Math::Vector3f& eulerAngles,
                                                         const Math::Vector3f& pivot);
    std::unique_ptr<GroupOperation> createScaleOperation(GroupId groupId, float scaleFactor,
                                                        const Math::Vector3f& pivot);
};

} // namespace Groups
} // namespace VoxelEditor