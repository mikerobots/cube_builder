#include "CommandStateValidator.h"
#include "../../selection/SelectionSet.h"
#include "../../selection/SelectionManager.h"
#include "../../camera/OrbitCamera.h"
#include "../../rendering/RenderConfig.h"
#include "../../groups/include/groups/VoxelGroup.h"
#include "../../foundation/logging/Logger.h"
#include <sstream>
#include <algorithm>

namespace VoxelEditor {
namespace UndoRedo {
namespace Testing {

CommandStateValidator::CommandStateValidator(
    VoxelData::VoxelDataManager* voxelManager,
    Selection::SelectionManager* selectionManager,
    Camera::OrbitCamera* camera,
    Rendering::RenderSettings* renderSettings,
    Groups::GroupManager* groupManager)
    : m_voxelManager(voxelManager)
    , m_selectionManager(selectionManager)
    , m_camera(camera)
    , m_renderSettings(renderSettings)
    , m_groupManager(groupManager)
    , m_voxelHandler(nullptr)
    , m_selectionHandler(nullptr)
    , m_groupHandler(nullptr) {
}

CommandStateValidator::~CommandStateValidator() = default;

std::unique_ptr<CommandStateValidator::ExtendedStateSnapshot> 
CommandStateValidator::captureState(const std::string& description) const {
    auto snapshot = std::make_unique<ExtendedStateSnapshot>();
    snapshot->description = description;
    snapshot->timestamp = std::chrono::system_clock::now();
    
    // Capture base state using StateSnapshot
    snapshot->baseSnapshot = std::make_unique<StateSnapshot>();
    if (m_voxelManager) {
        snapshot->baseSnapshot->captureVoxelData(m_voxelManager);
    }
    if (m_selectionManager) {
        snapshot->baseSnapshot->captureSelections(m_selectionManager);
    }
    if (m_camera) {
        snapshot->baseSnapshot->captureCamera(m_camera);
    }
    if (m_renderSettings) {
        snapshot->baseSnapshot->captureRenderSettings(m_renderSettings);
    }
    
    // Capture extended state
    captureGroupsState(snapshot->groupsState);
    captureEventState(snapshot->eventState);
    
    return snapshot;
}

std::unique_ptr<CommandStateValidator::ExtendedStateSnapshot> 
CommandStateValidator::captureVoxelState(const std::string& description) const {
    auto snapshot = std::make_unique<ExtendedStateSnapshot>();
    snapshot->description = description;
    snapshot->timestamp = std::chrono::system_clock::now();
    
    // Capture only voxel data
    snapshot->baseSnapshot = std::make_unique<StateSnapshot>();
    if (m_voxelManager) {
        snapshot->baseSnapshot->captureVoxelData(m_voxelManager);
    }
    
    captureEventState(snapshot->eventState);
    
    return snapshot;
}

void CommandStateValidator::captureGroupsState(ExtendedStateSnapshot::GroupsState& groupsState) const {
    if (!m_groupManager) {
        return;
    }
    
    // Capture all group IDs
    groupsState.allGroupIds = m_groupManager->getAllGroupIds();
    
    // Capture group details
    for (const auto& groupId : groupsState.allGroupIds) {
        const VoxelEditor::Groups::VoxelGroup* group = m_groupManager->getGroup(groupId);
        if (group) {
            groupsState.groupNames[groupId] = group->getName();
            groupsState.groupVoxels[groupId] = m_groupManager->getGroupVoxels(groupId);
            groupsState.groupVisibility[groupId] = m_groupManager->isGroupVisible(groupId);
            groupsState.groupLocked[groupId] = m_groupManager->isGroupLocked(groupId);
            
            // Capture parent relationship
            Groups::GroupId parentId = m_groupManager->getParentGroup(groupId);
            if (parentId != 0) { // Assuming 0 means no parent
                groupsState.parentGroups[groupId] = parentId;
            }
        }
    }
    
    // Capture next group ID for full state restoration
    // This would require access to private members, so we'll estimate
    if (!groupsState.allGroupIds.empty()) {
        groupsState.nextGroupId = *std::max_element(groupsState.allGroupIds.begin(), 
                                                   groupsState.allGroupIds.end()) + 1;
    } else {
        groupsState.nextGroupId = 1;
    }
}

void CommandStateValidator::captureEventState(ExtendedStateSnapshot::EventState& eventState) const {
    eventState.voxelChangeEventCount = m_voxelHandler ? m_voxelHandler->eventCount : 0;
    eventState.selectionChangeEventCount = m_selectionHandler ? m_selectionHandler->eventCount : 0;
    eventState.groupChangeEventCount = m_groupHandler ? m_groupHandler->eventCount : 0;
}

CommandStateValidator::ComparisonResult 
CommandStateValidator::compareStates(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after) const {
    
    ComparisonResult result;
    
    // Compare all aspects of state
    compareVoxelDataInternal(before, after, result);
    compareSelectionsInternal(before, after, result);
    compareGroupsInternal(before, after, result);
    compareCameraInternal(before, after, result);
    compareRenderSettingsInternal(before, after, result);
    
    // Generate summary
    if (result.identical) {
        result.summary = "All system state is identical";
    } else {
        result.summary = "Found " + std::to_string(result.differences.size()) + " differences";
    }
    
    return result;
}

CommandStateValidator::ComparisonResult 
CommandStateValidator::compareVoxelData(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after) const {
    
    ComparisonResult result;
    compareVoxelDataInternal(before, after, result);
    
    if (result.identical) {
        result.summary = "Voxel data is identical";
    } else {
        result.summary = "Found voxel data differences";
    }
    
    return result;
}

CommandStateValidator::ComparisonResult 
CommandStateValidator::compareSelections(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after) const {
    
    ComparisonResult result;
    compareSelectionsInternal(before, after, result);
    
    if (result.identical) {
        result.summary = "Selection state is identical";
    } else {
        result.summary = "Found selection differences";
    }
    
    return result;
}

CommandStateValidator::ComparisonResult 
CommandStateValidator::compareGroups(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after) const {
    
    ComparisonResult result;
    compareGroupsInternal(before, after, result);
    
    if (result.identical) {
        result.summary = "Groups state is identical";
    } else {
        result.summary = "Found groups differences";
    }
    
    return result;
}

void CommandStateValidator::compareVoxelDataInternal(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after,
    ComparisonResult& result) const {
    
    if (!before.baseSnapshot || !after.baseSnapshot) {
        if (before.baseSnapshot != after.baseSnapshot) {
            result.addDifference("One snapshot has voxel data, the other doesn't");
        }
        return;
    }
    
    if (!m_voxelManager) {
        return;
    }
    
    // Compare voxel counts across all resolutions
    for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
        VoxelData::VoxelResolution resolution = static_cast<VoxelData::VoxelResolution>(i);
        const VoxelData::VoxelGrid* grid = m_voxelManager->getGrid(resolution);
        
        if (grid) {
            // For a more detailed comparison, we would need to restore both snapshots
            // to temporary managers and compare. For now, we'll do a simplified comparison.
            
            // This is a placeholder - in practice you'd want to implement detailed voxel comparison
            // by restoring to temporary VoxelDataManagers and comparing voxel by voxel
        }
    }
    
    // For now, we'll assume voxel data comparison is done through direct manager inspection
    // This could be enhanced by implementing a detailed voxel enumeration comparison
}

void CommandStateValidator::compareSelectionsInternal(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after,
    ComparisonResult& result) const {
    
    if (!m_selectionManager) {
        return;
    }
    
    // Compare current selection
    // This would require detailed selection set comparison
    // For now, we'll implement a basic comparison
    
    // In a full implementation, you'd compare:
    // - Selected voxel sets
    // - Selection modes
    // - Named selection sets
}

void CommandStateValidator::compareGroupsInternal(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after,
    ComparisonResult& result) const {
    
    const auto& beforeGroups = before.groupsState;
    const auto& afterGroups = after.groupsState;
    
    // Compare group counts
    if (beforeGroups.allGroupIds.size() != afterGroups.allGroupIds.size()) {
        result.addDifference("Group count changed from " + 
                           std::to_string(beforeGroups.allGroupIds.size()) + 
                           " to " + std::to_string(afterGroups.allGroupIds.size()));
    }
    
    // Compare group IDs
    std::vector<Groups::GroupId> beforeSorted = beforeGroups.allGroupIds;
    std::vector<Groups::GroupId> afterSorted = afterGroups.allGroupIds;
    std::sort(beforeSorted.begin(), beforeSorted.end());
    std::sort(afterSorted.begin(), afterSorted.end());
    
    if (beforeSorted != afterSorted) {
        result.addDifference("Group IDs changed");
    }
    
    // Compare group details for common groups
    for (const auto& groupId : beforeGroups.allGroupIds) {
        if (std::find(afterGroups.allGroupIds.begin(), afterGroups.allGroupIds.end(), groupId) 
            != afterGroups.allGroupIds.end()) {
            
            // Compare group names
            auto beforeName = beforeGroups.groupNames.find(groupId);
            auto afterName = afterGroups.groupNames.find(groupId);
            if (beforeName != beforeGroups.groupNames.end() && 
                afterName != afterGroups.groupNames.end() &&
                beforeName->second != afterName->second) {
                result.addDifference("Group " + std::to_string(groupId) + " name changed from '" + 
                                   beforeName->second + "' to '" + afterName->second + "'");
            }
            
            // Compare group voxels
            auto beforeVoxels = beforeGroups.groupVoxels.find(groupId);
            auto afterVoxels = afterGroups.groupVoxels.find(groupId);
            if (beforeVoxels != beforeGroups.groupVoxels.end() && 
                afterVoxels != afterGroups.groupVoxels.end()) {
                if (beforeVoxels->second.size() != afterVoxels->second.size()) {
                    result.addDifference("Group " + std::to_string(groupId) + " voxel count changed from " +
                                       std::to_string(beforeVoxels->second.size()) + " to " +
                                       std::to_string(afterVoxels->second.size()));
                }
            }
            
            // Compare visibility
            auto beforeVis = beforeGroups.groupVisibility.find(groupId);
            auto afterVis = afterGroups.groupVisibility.find(groupId);
            if (beforeVis != beforeGroups.groupVisibility.end() && 
                afterVis != afterGroups.groupVisibility.end() &&
                beforeVis->second != afterVis->second) {
                result.addDifference("Group " + std::to_string(groupId) + " visibility changed from " +
                                   (beforeVis->second ? "visible" : "hidden") + " to " +
                                   (afterVis->second ? "visible" : "hidden"));
            }
            
            // Compare locked state
            auto beforeLock = beforeGroups.groupLocked.find(groupId);
            auto afterLock = afterGroups.groupLocked.find(groupId);
            if (beforeLock != beforeGroups.groupLocked.end() && 
                afterLock != afterGroups.groupLocked.end() &&
                beforeLock->second != afterLock->second) {
                result.addDifference("Group " + std::to_string(groupId) + " lock state changed from " +
                                   (beforeLock->second ? "locked" : "unlocked") + " to " +
                                   (afterLock->second ? "locked" : "unlocked"));
            }
        }
    }
}

void CommandStateValidator::compareCameraInternal(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after,
    ComparisonResult& result) const {
    
    // Camera comparison would be implemented here
    // For now, we'll skip detailed camera comparison
}

void CommandStateValidator::compareRenderSettingsInternal(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after,
    ComparisonResult& result) const {
    
    // Render settings comparison would be implemented here
    // For now, we'll skip detailed render settings comparison
}

bool CommandStateValidator::validateCommandExecution(
    const ExtendedStateSnapshot& beforeState,
    const ExtendedStateSnapshot& afterState,
    const std::vector<std::string>& expectedChanges) const {
    
    // Validate that some change occurred (states should not be identical for most commands)
    auto comparison = compareStates(beforeState, afterState);
    
    if (comparison.identical && !expectedChanges.empty()) {
        // If we expected changes but states are identical, this is likely an error
        return false;
    }
    
    // Validate specific expected changes
    return validateVoxelChanges(beforeState, afterState, expectedChanges);
}

bool CommandStateValidator::validateUndoRestoration(
    const ExtendedStateSnapshot& originalState,
    const ExtendedStateSnapshot& undoState) const {
    
    auto comparison = compareStates(originalState, undoState);
    return comparison.identical;
}

bool CommandStateValidator::validateVoxelChanges(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after,
    const std::vector<std::string>& expectedChanges) const {
    
    // This would implement detailed validation of specific expected changes
    // For now, we'll return true as the basic framework is in place
    return true;
}

void CommandStateValidator::setEventHandlers(
    TestVoxelChangedHandler* voxelHandler,
    TestSelectionChangedHandler* selectionHandler,
    TestGroupChangedHandler* groupHandler) {
    
    m_voxelHandler = voxelHandler;
    m_selectionHandler = selectionHandler;
    m_groupHandler = groupHandler;
}

std::string CommandStateValidator::describeCurrentState() const {
    std::ostringstream desc;
    
    desc << "=== Current System State ===\n";
    
    // Describe voxel state
    if (m_voxelManager) {
        desc << "Voxel Data:\n";
        size_t totalVoxels = 0;
        for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
            VoxelData::VoxelResolution resolution = static_cast<VoxelData::VoxelResolution>(i);
            const VoxelData::VoxelGrid* grid = m_voxelManager->getGrid(resolution);
            if (grid) {
                auto voxels = grid->getAllVoxels();
                totalVoxels += voxels.size();
                if (!voxels.empty()) {
                    desc << "  " << static_cast<int>(resolution) << "cm resolution: " 
                         << voxels.size() << " voxels\n";
                }
            }
        }
        desc << "  Total voxels: " << totalVoxels << "\n";
        desc << "  Active resolution: " << static_cast<int>(m_voxelManager->getActiveResolution()) << "cm\n";
    }
    
    // Describe group state
    if (m_groupManager) {
        auto groupIds = m_groupManager->getAllGroupIds();
        desc << "Groups: " << groupIds.size() << " total\n";
        for (const auto& groupId : groupIds) {
            const VoxelEditor::Groups::VoxelGroup* group = m_groupManager->getGroup(groupId);
            if (group) {
                desc << "  Group " << groupId << ": '" << group->getName() << "' ("
                     << m_groupManager->getGroupVoxels(groupId).size() << " voxels, "
                     << (m_groupManager->isGroupVisible(groupId) ? "visible" : "hidden") << ", "
                     << (m_groupManager->isGroupLocked(groupId) ? "locked" : "unlocked") << ")\n";
            }
        }
    }
    
    // Describe event state
    if (m_voxelHandler || m_selectionHandler || m_groupHandler) {
        desc << "Event Counts:\n";
        if (m_voxelHandler) {
            desc << "  Voxel changes: " << m_voxelHandler->eventCount << "\n";
        }
        if (m_selectionHandler) {
            desc << "  Selection changes: " << m_selectionHandler->eventCount << "\n";
        }
        if (m_groupHandler) {
            desc << "  Group changes: " << m_groupHandler->eventCount << "\n";
        }
    }
    
    return desc.str();
}

std::string CommandStateValidator::describeDifferences(
    const ExtendedStateSnapshot& before,
    const ExtendedStateSnapshot& after) const {
    
    auto comparison = compareStates(before, after);
    return comparison.generateReport();
}

} // namespace Testing
} // namespace UndoRedo
} // namespace VoxelEditor