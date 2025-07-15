#pragma once

#include "GroupTypes.h"
#include "../../voxel_data/VoxelDataManager.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/BoundingBox.h"
#include <vector>
#include <memory>
#include <unordered_set>

namespace VoxelEditor {
namespace Groups {

// Forward declarations
class GroupManager;
class VoxelGroup;

// Base class for group operations
class GroupOperation {
public:
    virtual ~GroupOperation() = default;
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual std::string getDescription() const = 0;
    virtual GroupModificationType getType() const = 0;
};

// Move group operation
class MoveGroupOperation : public GroupOperation {
public:
    MoveGroupOperation(GroupManager* groupManager, VoxelData::VoxelDataManager* voxelManager,
                      GroupId groupId, const Math::WorldCoordinates& offset);
    
    bool execute() override;
    bool undo() override;
    std::string getDescription() const override;
    GroupModificationType getType() const override { return GroupModificationType::Moved; }
    
private:
    GroupManager* m_groupManager;
    VoxelData::VoxelDataManager* m_voxelManager;
    GroupId m_groupId;
    Math::WorldCoordinates m_offset;
    std::vector<std::pair<VoxelId, VoxelId>> m_voxelMoves; // old -> new positions
    bool m_executed = false;
};

// Copy group operation
class CopyGroupOperation : public GroupOperation {
public:
    CopyGroupOperation(GroupManager* groupManager, VoxelData::VoxelDataManager* voxelManager,
                      GroupId sourceId, const std::string& newName, 
                      const Math::WorldCoordinates& offset = Math::WorldCoordinates(Math::Vector3f(0, 0, 0)));
    
    bool execute() override;
    bool undo() override;
    std::string getDescription() const override;
    GroupModificationType getType() const override { return GroupModificationType::Created; }
    
    GroupId getCreatedGroupId() const { return m_createdGroupId; }
    
private:
    GroupManager* m_groupManager;
    VoxelData::VoxelDataManager* m_voxelManager;
    GroupId m_sourceId;
    GroupId m_createdGroupId = INVALID_GROUP_ID;
    std::string m_newName;
    Math::WorldCoordinates m_offset;
    std::vector<VoxelId> m_createdVoxels;
    bool m_executed = false;
};

// Rotate group operation
class RotateGroupOperation : public GroupOperation {
public:
    RotateGroupOperation(GroupManager* groupManager, VoxelData::VoxelDataManager* voxelManager,
                        GroupId groupId, const Math::Vector3f& eulerAngles, 
                        const Math::WorldCoordinates& pivot);
    
    bool execute() override;
    bool undo() override;
    std::string getDescription() const override;
    GroupModificationType getType() const override { return GroupModificationType::Rotated; }
    
private:
    GroupManager* m_groupManager;
    VoxelData::VoxelDataManager* m_voxelManager;
    GroupId m_groupId;
    Math::Vector3f m_eulerAngles;
    Math::WorldCoordinates m_pivot;
    std::vector<std::pair<VoxelId, VoxelId>> m_voxelMoves;
    bool m_executed = false;
};

// Scale group operation
class ScaleGroupOperation : public GroupOperation {
public:
    ScaleGroupOperation(GroupManager* groupManager, VoxelData::VoxelDataManager* voxelManager,
                       GroupId groupId, float scaleFactor, const Math::WorldCoordinates& pivot);
    
    bool execute() override;
    bool undo() override;
    std::string getDescription() const override;
    GroupModificationType getType() const override { return GroupModificationType::Scaled; }
    
private:
    GroupManager* m_groupManager;
    VoxelData::VoxelDataManager* m_voxelManager;
    GroupId m_groupId;
    float m_scaleFactor;
    Math::WorldCoordinates m_pivot;
    std::vector<std::pair<VoxelId, VoxelId>> m_voxelMoves;
    bool m_executed = false;
};

// Merge groups operation
class MergeGroupsOperation : public GroupOperation {
public:
    MergeGroupsOperation(GroupManager* groupManager, const std::vector<GroupId>& sourceIds,
                        const std::string& targetName);
    
    bool execute() override;
    bool undo() override;
    std::string getDescription() const override;
    GroupModificationType getType() const override { return GroupModificationType::Created; }
    
    GroupId getTargetGroupId() const { return m_targetGroupId; }
    
private:
    GroupManager* m_groupManager;
    std::vector<GroupId> m_sourceIds;
    GroupId m_targetGroupId = INVALID_GROUP_ID;
    std::string m_targetName;
    std::vector<std::pair<GroupId, std::vector<VoxelId>>> m_originalGroups; // for undo
    bool m_executed = false;
};

// Split group operation
class SplitGroupOperation : public GroupOperation {
public:
    SplitGroupOperation(GroupManager* groupManager, GroupId sourceId,
                       const std::vector<std::vector<VoxelId>>& voxelSets,
                       const std::vector<std::string>& newNames);
    
    bool execute() override;
    bool undo() override;
    std::string getDescription() const override;
    GroupModificationType getType() const override { return GroupModificationType::Created; }
    
    const std::vector<GroupId>& getCreatedGroupIds() const { return m_createdGroupIds; }
    
private:
    GroupManager* m_groupManager;
    GroupId m_sourceId;
    std::vector<std::vector<VoxelId>> m_voxelSets;
    std::vector<std::string> m_newNames;
    std::vector<GroupId> m_createdGroupIds;
    std::vector<VoxelId> m_originalVoxels; // for undo
    bool m_executed = false;
};

// Utility functions for group operations
namespace GroupOperationUtils {
    // Calculate new voxel position after transformation
    VoxelId transformVoxel(const VoxelId& voxel, const GroupTransform& transform);
    
    // Calculate bounding box for a set of voxels
    Math::BoundingBox calculateBounds(const std::vector<VoxelId>& voxels);
    
    // Find optimal pivot point for a group
    Math::WorldCoordinates calculateOptimalPivot(const std::vector<VoxelId>& voxels);
    
    // Validate that voxel positions are within workspace bounds
    bool validateVoxelPositions(const std::vector<VoxelId>& voxels, 
                               const Math::BoundingBox& workspaceBounds);
    
    // Generate unique name for a group
    std::string generateUniqueName(const std::string& baseName, 
                                  const std::vector<std::string>& existingNames);
}

} // namespace Groups
} // namespace VoxelEditor