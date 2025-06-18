#include "../include/groups/GroupOperations.h"
#include "../include/groups/GroupManager.h"
#include "../include/groups/VoxelGroup.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace VoxelEditor {
namespace Groups {

// MoveGroupOperation implementation
MoveGroupOperation::MoveGroupOperation(GroupManager* groupManager, 
                                     VoxelData::VoxelDataManager* voxelManager,
                                     GroupId groupId, const Math::WorldCoordinates& offset)
    : m_groupManager(groupManager)
    , m_voxelManager(voxelManager)
    , m_groupId(groupId)
    , m_offset(offset) {
}

bool MoveGroupOperation::execute() {
    if (m_executed) return false;
    
    auto group = m_groupManager->getGroup(m_groupId);
    if (!group) return false;
    
    // Get all voxels in the group
    auto voxelList = group->getVoxelList();
    m_voxelMoves.clear();
    m_voxelMoves.reserve(voxelList.size());
    
    // Calculate new positions for all voxels
    Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
    
    for (const auto& voxel : voxelList) {
        // Use VoxelId's getWorldPosition() method which handles centered coordinate system
        Math::WorldCoordinates worldCoords = voxel.getWorldPosition();
        Math::Vector3f worldPos = worldCoords.value();
        Math::Vector3f newWorldPos = worldPos + m_offset.value();
        
        // Convert back to grid coordinates using the centralized converter
        Math::IncrementCoordinates newGridCoords = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(newWorldPos));
        
        VoxelId newVoxel(newGridCoords, voxel.resolution);
        m_voxelMoves.emplace_back(voxel, newVoxel);
    }
    
    // Validate new positions are within workspace
    std::vector<VoxelId> newPositions;
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        newPositions.push_back(newPos);
    }
    
    // Create workspace bounds using centered coordinate system
    Math::BoundingBox workspaceBounds(
        Math::Vector3f(-workspaceSize.x/2, 0.0f, -workspaceSize.z/2),
        Math::Vector3f(workspaceSize.x/2, workspaceSize.y, workspaceSize.z/2)
    );
    if (!GroupOperationUtils::validateVoxelPositions(newPositions, workspaceBounds)) {
        m_voxelMoves.clear();
        return false;
    }
    
    // Check for collisions with existing voxels
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        // Position is already in IncrementCoordinates, no conversion needed
        Math::IncrementCoordinates newIncrement = newPos.position;
        
        if (m_voxelManager->hasVoxel(newIncrement, newPos.resolution)) {
            // Check if it's not the same voxel we're moving
            bool isSameVoxel = false;
            for (const auto& [checkOld, checkNew] : m_voxelMoves) {
                if (checkOld == newPos) {
                    isSameVoxel = true;
                    break;
                }
            }
            if (!isSameVoxel) {
                m_voxelMoves.clear();
                return false;
            }
        }
    }
    
    // Clear the group and rebuild with moved voxels
    group->clearVoxels();
    
    // Remove old voxels from VoxelDataManager and add new ones
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        // Convert grid coordinates to increment coordinates for VoxelDataManager
        Math::IncrementCoordinates oldIncrement = oldPos.position;
        Math::IncrementCoordinates newIncrement = newPos.position;
        
        m_voxelManager->setVoxel(oldIncrement, oldPos.resolution, false);
        m_voxelManager->setVoxel(newIncrement, newPos.resolution, true);
        group->addVoxel(newPos);
    }
    
    m_executed = true;
    return true;
}

bool MoveGroupOperation::undo() {
    if (!m_executed) return false;
    
    auto group = m_groupManager->getGroup(m_groupId);
    if (!group) return false;
    
    // Clear the group and rebuild with original voxels
    group->clearVoxels();
    
    // Remove new voxels from VoxelDataManager and restore old ones
    Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        // Convert grid coordinates to increment coordinates for VoxelDataManager
        Math::IncrementCoordinates oldIncrement = oldPos.position;
        Math::IncrementCoordinates newIncrement = newPos.position;
        
        m_voxelManager->setVoxel(newIncrement, newPos.resolution, false);
        m_voxelManager->setVoxel(oldIncrement, oldPos.resolution, true);
        group->addVoxel(oldPos);
    }
    
    m_executed = false;
    return true;
}

std::string MoveGroupOperation::getDescription() const {
    std::stringstream ss;
    ss << "Move group " << m_groupId << " by (" 
       << m_offset.x() << ", " << m_offset.y() << ", " << m_offset.z() << ")";
    return ss.str();
}

// CopyGroupOperation implementation
CopyGroupOperation::CopyGroupOperation(GroupManager* groupManager,
                                     VoxelData::VoxelDataManager* voxelManager,
                                     GroupId sourceId, const std::string& newName,
                                     const Math::WorldCoordinates& offset)
    : m_groupManager(groupManager)
    , m_voxelManager(voxelManager)
    , m_sourceId(sourceId)
    , m_newName(newName)
    , m_offset(offset) {
}

bool CopyGroupOperation::execute() {
    if (m_executed) return false;
    
    auto sourceGroup = m_groupManager->getGroup(m_sourceId);
    if (!sourceGroup) return false;
    
    // Create new group
    m_createdGroupId = m_groupManager->createGroup(m_newName);
    if (m_createdGroupId == INVALID_GROUP_ID) return false;
    
    auto newGroup = m_groupManager->getGroup(m_createdGroupId);
    if (!newGroup) {
        m_groupManager->deleteGroup(m_createdGroupId);
        return false;
    }
    
    // Copy metadata
    auto metadata = sourceGroup->getMetadata();
    metadata.name = m_newName;
    newGroup->setMetadata(metadata);
    
    // Copy voxels with offset
    auto sourceVoxels = sourceGroup->getVoxelList();
    m_createdVoxels.clear();
    m_createdVoxels.reserve(sourceVoxels.size());
    
    Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
    
    
    for (const auto& voxel : sourceVoxels) {
        // Use VoxelId's getWorldPosition() method which handles centered coordinate system
        Math::WorldCoordinates worldCoords = voxel.getWorldPosition();
        Math::Vector3f worldPos = worldCoords.value();
        Math::Vector3f newWorldPos = worldPos + m_offset.value();
        
        // Convert back to grid coordinates using the centralized converter
        Math::IncrementCoordinates newGridCoords = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(newWorldPos));
        
        VoxelId newVoxel(newGridCoords, voxel.resolution);
        
        // Always create the copied voxel (even if there's a collision)
        // Position is already in IncrementCoordinates, no conversion needed
        Math::IncrementCoordinates newIncrement = newVoxel.position;
        m_voxelManager->setVoxel(newIncrement, newVoxel.resolution, true);
        newGroup->addVoxel(newVoxel);
        m_createdVoxels.push_back(newVoxel);
    }
    
    m_executed = true;
    return true;
}

bool CopyGroupOperation::undo() {
    if (!m_executed) return false;
    
    // Remove all created voxels
    Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
    for (const auto& voxel : m_createdVoxels) {
        // Convert grid coordinates to increment coordinates for VoxelDataManager
        Math::IncrementCoordinates increment = voxel.position;
        m_voxelManager->setVoxel(increment, voxel.resolution, false);
    }
    
    // Delete the created group
    m_groupManager->deleteGroup(m_createdGroupId);
    m_createdGroupId = INVALID_GROUP_ID;
    m_createdVoxels.clear();
    
    m_executed = false;
    return true;
}

std::string CopyGroupOperation::getDescription() const {
    std::stringstream ss;
    ss << "Copy group " << m_sourceId << " as \"" << m_newName << "\"";
    if (m_offset.value().length() > 0) {
        ss << " with offset (" << m_offset.x() << ", " << m_offset.y() << ", " << m_offset.z() << ")";
    }
    return ss.str();
}

// RotateGroupOperation implementation
RotateGroupOperation::RotateGroupOperation(GroupManager* groupManager,
                                         VoxelData::VoxelDataManager* voxelManager,
                                         GroupId groupId, const Math::Vector3f& eulerAngles,
                                         const Math::Vector3f& pivot)
    : m_groupManager(groupManager)
    , m_voxelManager(voxelManager)
    , m_groupId(groupId)
    , m_eulerAngles(eulerAngles)
    , m_pivot(pivot) {
}

bool RotateGroupOperation::execute() {
    if (m_executed) return false;
    
    auto group = m_groupManager->getGroup(m_groupId);
    if (!group) return false;
    
    // Get all voxels in the group
    auto voxelList = group->getVoxelList();
    m_voxelMoves.clear();
    m_voxelMoves.reserve(voxelList.size());
    
    // Create rotation matrix from Euler angles
    float cx = std::cos(m_eulerAngles.x);
    float sx = std::sin(m_eulerAngles.x);
    float cy = std::cos(m_eulerAngles.y);
    float sy = std::sin(m_eulerAngles.y);
    float cz = std::cos(m_eulerAngles.z);
    float sz = std::sin(m_eulerAngles.z);
    
    // Calculate new positions for all voxels
    Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
    
    for (const auto& voxel : voxelList) {
        // Use the proper coordinate conversion
        Math::WorldCoordinates worldCoords = voxel.getWorldPosition();
        Math::Vector3f worldPos = worldCoords.value();
        
        // Translate to pivot
        Math::Vector3f relPos = worldPos - m_pivot;
        
        // Apply rotation (ZYX order)
        Math::Vector3f rotated;
        // Z rotation
        float tempX = relPos.x * cz - relPos.y * sz;
        float tempY = relPos.x * sz + relPos.y * cz;
        relPos.x = tempX;
        relPos.y = tempY;
        
        // Y rotation
        tempX = relPos.x * cy + relPos.z * sy;
        float tempZ = -relPos.x * sy + relPos.z * cy;
        relPos.x = tempX;
        relPos.z = tempZ;
        
        // X rotation
        tempY = relPos.y * cx - relPos.z * sx;
        tempZ = relPos.y * sx + relPos.z * cx;
        relPos.y = tempY;
        relPos.z = tempZ;
        
        // Translate back from pivot
        Math::Vector3f newWorldPos = relPos + m_pivot;
        
        // Convert back to grid coordinates using the centralized converter
        Math::IncrementCoordinates newGridCoords = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(newWorldPos));
        
        VoxelId newVoxel(newGridCoords, voxel.resolution);
        
        m_voxelMoves.emplace_back(voxel, newVoxel);
    }
    
    // Check for overlaps
    std::unordered_set<VoxelId> newPositions;
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        if (!newPositions.insert(newPos).second) {
            // Duplicate position after rotation
            m_voxelMoves.clear();
            return false;
        }
    }
    
    // Remove old voxels
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        // Convert grid coordinates to increment coordinates for VoxelDataManager
        Math::IncrementCoordinates oldIncrement = oldPos.position;
        m_voxelManager->setVoxel(oldIncrement, oldPos.resolution, false);
        group->removeVoxel(oldPos);
    }
    
    // Add new voxels
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        // Position is already in IncrementCoordinates, no conversion needed
        Math::IncrementCoordinates newIncrement = newPos.position;
        m_voxelManager->setVoxel(newIncrement, newPos.resolution, true);
        group->addVoxel(newPos);
    }
    
    m_executed = true;
    return true;
}

bool RotateGroupOperation::undo() {
    if (!m_executed) return false;
    
    auto group = m_groupManager->getGroup(m_groupId);
    if (!group) return false;
    
    // Get workspace size for coordinate conversion
    Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
    
    // Remove rotated voxels
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        // Position is already in IncrementCoordinates, no conversion needed
        Math::IncrementCoordinates newIncrement = newPos.position;
        m_voxelManager->setVoxel(newIncrement, newPos.resolution, false);
        group->removeVoxel(newPos);
    }
    
    // Restore original voxels
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        // Convert grid coordinates to increment coordinates for VoxelDataManager
        Math::IncrementCoordinates oldIncrement = oldPos.position;
        m_voxelManager->setVoxel(oldIncrement, oldPos.resolution, true);
        group->addVoxel(oldPos);
    }
    
    m_executed = false;
    return true;
}

std::string RotateGroupOperation::getDescription() const {
    std::stringstream ss;
    ss << "Rotate group " << m_groupId << " by (" 
       << Math::radiansToDegrees(m_eulerAngles.x) << "°, " 
       << Math::radiansToDegrees(m_eulerAngles.y) << "°, " 
       << Math::radiansToDegrees(m_eulerAngles.z) << "°)";
    return ss.str();
}

// ScaleGroupOperation implementation
ScaleGroupOperation::ScaleGroupOperation(GroupManager* groupManager,
                                       VoxelData::VoxelDataManager* voxelManager,
                                       GroupId groupId, float scaleFactor,
                                       const Math::Vector3f& pivot)
    : m_groupManager(groupManager)
    , m_voxelManager(voxelManager)
    , m_groupId(groupId)
    , m_scaleFactor(scaleFactor)
    , m_pivot(pivot) {
}

bool ScaleGroupOperation::execute() {
    if (m_executed) return false;
    if (m_scaleFactor <= 0) return false;
    
    auto group = m_groupManager->getGroup(m_groupId);
    if (!group) return false;
    
    // Scaling voxels is complex because we need to maintain the discrete grid
    // For now, we'll implement a simple approach that works for integer scale factors
    // TODO: Implement more sophisticated scaling with resampling
    
    auto voxelList = group->getVoxelList();
    m_voxelMoves.clear();
    
    // For non-integer scales, we'd need to implement voxel resampling
    // For now, return false for non-integer scales
    if (std::abs(m_scaleFactor - std::round(m_scaleFactor)) > 0.001f) {
        return false;
    }
    
    int scale = static_cast<int>(std::round(m_scaleFactor));
    
    // Get workspace size for coordinate conversions
    Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
    
    if (scale > 1) {
        // Scaling up: each voxel becomes a cube of voxels
        
        for (const auto& voxel : voxelList) {
            // Use the centralized coordinate converter
            Math::WorldCoordinates worldCoords = Math::CoordinateConverter::incrementToWorld(
                voxel.position);
            Math::Vector3f worldPos = worldCoords.value();
            float voxelSize = VoxelData::getVoxelSize(voxel.resolution);
            
            Math::Vector3f relPos = worldPos - m_pivot;
            
            for (int dx = 0; dx < scale; ++dx) {
                for (int dy = 0; dy < scale; ++dy) {
                    for (int dz = 0; dz < scale; ++dz) {
                        Math::Vector3f newRelPos = relPos * m_scaleFactor + 
                            Math::Vector3f(dx * voxelSize, dy * voxelSize, dz * voxelSize);
                        Math::Vector3f newWorldPos = newRelPos + m_pivot;
                        
                        // Convert back to grid coordinates using the centralized converter
                        Math::IncrementCoordinates newGridCoords = Math::CoordinateConverter::worldToIncrement(
                            Math::WorldCoordinates(newWorldPos));
                        
                        VoxelId newVoxel(newGridCoords, voxel.resolution);
                        
                        m_voxelMoves.emplace_back(voxel, newVoxel);
                    }
                }
            }
        }
    } else {
        // Scaling down is not implemented yet
        return false;
    }
    
    // Execute the moves
    // First remove all old voxels
    for (const auto& voxel : voxelList) {
        // Position is already in IncrementCoordinates, no conversion needed
        Math::IncrementCoordinates oldIncrement = voxel.position;
        m_voxelManager->setVoxel(oldIncrement, voxel.resolution, false);
        group->removeVoxel(voxel);
    }
    
    // Add new voxels
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        // Position is already in IncrementCoordinates, no conversion needed
        Math::IncrementCoordinates newIncrement = newPos.position;
        m_voxelManager->setVoxel(newIncrement, newPos.resolution, true);
        group->addVoxel(newPos);
    }
    
    m_executed = true;
    return true;
}

bool ScaleGroupOperation::undo() {
    if (!m_executed) return false;
    
    auto group = m_groupManager->getGroup(m_groupId);
    if (!group) return false;
    
    // Remove scaled voxels
    std::unordered_set<VoxelId> uniqueNewPositions;
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        uniqueNewPositions.insert(newPos);
    }
    
    // Get workspace size for coordinate conversion
    Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
    
    for (const auto& voxel : uniqueNewPositions) {
        // Convert grid coordinates to increment coordinates for VoxelDataManager
        Math::IncrementCoordinates increment = voxel.position;
        m_voxelManager->setVoxel(increment, voxel.resolution, false);
        group->removeVoxel(voxel);
    }
    
    // Restore original voxels
    std::unordered_set<VoxelId> uniqueOldPositions;
    for (const auto& [oldPos, newPos] : m_voxelMoves) {
        uniqueOldPositions.insert(oldPos);
    }
    
    for (const auto& voxel : uniqueOldPositions) {
        // Convert grid coordinates to increment coordinates for VoxelDataManager
        Math::IncrementCoordinates increment = voxel.position;
        m_voxelManager->setVoxel(increment, voxel.resolution, true);
        group->addVoxel(voxel);
    }
    
    m_executed = false;
    return true;
}

std::string ScaleGroupOperation::getDescription() const {
    std::stringstream ss;
    ss << "Scale group " << m_groupId << " by factor " << m_scaleFactor;
    return ss.str();
}

// MergeGroupsOperation implementation
MergeGroupsOperation::MergeGroupsOperation(GroupManager* groupManager,
                                         const std::vector<GroupId>& sourceIds,
                                         const std::string& targetName)
    : m_groupManager(groupManager)
    , m_sourceIds(sourceIds)
    , m_targetName(targetName) {
}

bool MergeGroupsOperation::execute() {
    if (m_executed) return false;
    if (m_sourceIds.empty()) return false;
    
    // Create target group
    m_targetGroupId = m_groupManager->createGroup(m_targetName);
    if (m_targetGroupId == INVALID_GROUP_ID) return false;
    
    auto targetGroup = m_groupManager->getGroup(m_targetGroupId);
    if (!targetGroup) {
        m_groupManager->deleteGroup(m_targetGroupId);
        return false;
    }
    
    // Store original groups for undo
    m_originalGroups.clear();
    m_originalGroups.reserve(m_sourceIds.size());
    
    // Merge all source groups into target
    for (GroupId sourceId : m_sourceIds) {
        auto sourceGroup = m_groupManager->getGroup(sourceId);
        if (!sourceGroup) continue;
        
        // Store original state
        auto voxels = sourceGroup->getVoxelList();
        m_originalGroups.emplace_back(sourceId, voxels);
        
        // Move voxels to target group
        for (const auto& voxel : voxels) {
            sourceGroup->removeVoxel(voxel);
            targetGroup->addVoxel(voxel);
            m_groupManager->updateVoxelGroupMembership(voxel, sourceId, m_targetGroupId);
        }
    }
    
    // Delete empty source groups
    for (GroupId sourceId : m_sourceIds) {
        m_groupManager->deleteGroup(sourceId);
    }
    
    m_executed = true;
    return true;
}

bool MergeGroupsOperation::undo() {
    if (!m_executed) return false;
    
    // Recreate original groups
    for (const auto& [groupId, voxels] : m_originalGroups) {
        auto newId = m_groupManager->createGroup("Restored Group " + std::to_string(groupId));
        auto group = m_groupManager->getGroup(newId);
        if (group) {
            for (const auto& voxel : voxels) {
                group->addVoxel(voxel);
                m_groupManager->updateVoxelGroupMembership(voxel, m_targetGroupId, newId);
            }
        }
    }
    
    // Delete target group
    m_groupManager->deleteGroup(m_targetGroupId);
    m_targetGroupId = INVALID_GROUP_ID;
    
    m_executed = false;
    return true;
}

std::string MergeGroupsOperation::getDescription() const {
    std::stringstream ss;
    ss << "Merge " << m_sourceIds.size() << " groups into \"" << m_targetName << "\"";
    return ss.str();
}

// SplitGroupOperation implementation
SplitGroupOperation::SplitGroupOperation(GroupManager* groupManager,
                                       GroupId sourceId,
                                       const std::vector<std::vector<VoxelId>>& voxelSets,
                                       const std::vector<std::string>& newNames)
    : m_groupManager(groupManager)
    , m_sourceId(sourceId)
    , m_voxelSets(voxelSets)
    , m_newNames(newNames) {
}

bool SplitGroupOperation::execute() {
    if (m_executed) return false;
    if (m_voxelSets.size() != m_newNames.size()) return false;
    
    auto sourceGroup = m_groupManager->getGroup(m_sourceId);
    if (!sourceGroup) return false;
    
    // Store original voxels for undo
    m_originalVoxels = sourceGroup->getVoxelList();
    
    // Create new groups
    m_createdGroupIds.clear();
    m_createdGroupIds.reserve(m_voxelSets.size());
    
    for (size_t i = 0; i < m_voxelSets.size(); ++i) {
        GroupId newId = m_groupManager->createGroup(m_newNames[i]);
        if (newId == INVALID_GROUP_ID) {
            // Rollback
            for (GroupId id : m_createdGroupIds) {
                m_groupManager->deleteGroup(id);
            }
            m_createdGroupIds.clear();
            return false;
        }
        
        m_createdGroupIds.push_back(newId);
        auto newGroup = m_groupManager->getGroup(newId);
        
        // Move voxels to new group
        for (const auto& voxel : m_voxelSets[i]) {
            if (sourceGroup->containsVoxel(voxel)) {
                sourceGroup->removeVoxel(voxel);
                newGroup->addVoxel(voxel);
                m_groupManager->updateVoxelGroupMembership(voxel, m_sourceId, newId);
            }
        }
    }
    
    // Delete source group if empty
    if (sourceGroup->isEmpty()) {
        m_groupManager->deleteGroup(m_sourceId);
    }
    
    m_executed = true;
    return true;
}

bool SplitGroupOperation::undo() {
    if (!m_executed) return false;
    
    // Recreate source group if it was deleted
    auto sourceGroup = m_groupManager->getGroup(m_sourceId);
    if (!sourceGroup) {
        m_sourceId = m_groupManager->createGroup("Restored Group " + std::to_string(m_sourceId));
        sourceGroup = m_groupManager->getGroup(m_sourceId);
    }
    
    // Move voxels back to source group
    for (size_t i = 0; i < m_createdGroupIds.size(); ++i) {
        auto group = m_groupManager->getGroup(m_createdGroupIds[i]);
        if (group) {
            auto voxels = group->getVoxelList();
            for (const auto& voxel : voxels) {
                group->removeVoxel(voxel);
                sourceGroup->addVoxel(voxel);
                m_groupManager->updateVoxelGroupMembership(voxel, m_createdGroupIds[i], m_sourceId);
            }
        }
        m_groupManager->deleteGroup(m_createdGroupIds[i]);
    }
    
    m_createdGroupIds.clear();
    m_executed = false;
    return true;
}

std::string SplitGroupOperation::getDescription() const {
    std::stringstream ss;
    ss << "Split group " << m_sourceId << " into " << m_voxelSets.size() << " groups";
    return ss.str();
}

// GroupOperationUtils implementation
namespace GroupOperationUtils {

VoxelId transformVoxel(const VoxelId& voxel, const GroupTransform& transform) {
    // Use default workspace size if not provided
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    
    // Use VoxelId's getWorldPosition() method which handles centered coordinate system
    Math::WorldCoordinates worldCoords = voxel.getWorldPosition();
    Math::Vector3f worldPos = worldCoords.value();
    
    // Apply transformation
    worldPos = worldPos + transform.translation;
    // TODO: Apply rotation and scale
    
    // Convert back to grid coordinates using the centralized converter
    Math::IncrementCoordinates newGridCoords = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(worldPos));
    
    return VoxelId(newGridCoords, voxel.resolution);
}

Math::BoundingBox calculateBounds(const std::vector<VoxelId>& voxels) {
    if (voxels.empty()) {
        return Math::BoundingBox();
    }
    
    Math::Vector3f minPos(std::numeric_limits<float>::max());
    Math::Vector3f maxPos(std::numeric_limits<float>::lowest());
    
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f); // Default workspace size
    
    for (const auto& voxel : voxels) {
        Math::BoundingBox voxelBounds = voxel.getBounds();
        
        minPos = Math::Vector3f::min(minPos, voxelBounds.min);
        maxPos = Math::Vector3f::max(maxPos, voxelBounds.max);
    }
    
    return Math::BoundingBox(minPos, maxPos);
}

Math::Vector3f calculateOptimalPivot(const std::vector<VoxelId>& voxels) {
    if (voxels.empty()) {
        return Math::Vector3f(0, 0, 0);
    }
    
    Math::Vector3f center(0, 0, 0);
    float totalWeight = 0;
    
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f); // Default workspace size
    
    for (const auto& voxel : voxels) {
        Math::WorldCoordinates worldCoords = voxel.getWorldPosition();
        Math::Vector3f worldPos = worldCoords.value();
        
        // Get the center of the voxel
        float voxelSize = voxel.getVoxelSize();
        Math::Vector3f voxelCenter = worldPos + Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
        
        center = center + voxelCenter;
        totalWeight += 1.0f;
    }
    
    return center / totalWeight;
}

bool validateVoxelPositions(const std::vector<VoxelId>& voxels,
                           const Math::BoundingBox& workspaceBounds) {
    // Calculate workspace size from the bounds
    Math::Vector3f workspaceSize = workspaceBounds.max - workspaceBounds.min;
    
    for (const auto& voxel : voxels) {
        // Get voxel bounds in world space using the correct workspace size
        Math::BoundingBox voxelBounds = voxel.getBounds();
        
        if (!workspaceBounds.contains(voxelBounds.min) || !workspaceBounds.contains(voxelBounds.max)) {
            return false;
        }
    }
    return true;
}

std::string generateUniqueName(const std::string& baseName,
                             const std::vector<std::string>& existingNames) {
    std::string name = baseName;
    int counter = 1;
    
    while (std::find(existingNames.begin(), existingNames.end(), name) != existingNames.end()) {
        name = baseName + " " + std::to_string(counter);
        counter++;
    }
    
    return name;
}

} // namespace GroupOperationUtils

} // namespace Groups
} // namespace VoxelEditor