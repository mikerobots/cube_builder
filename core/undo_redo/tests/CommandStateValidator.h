#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <gtest/gtest.h>
#include "../StateSnapshot.h"
#include "../../voxel_data/VoxelDataManager.h"
#include "../../selection/SelectionManager.h"
#include "../../camera/OrbitCamera.h"
#include "../../rendering/RenderConfig.h"
#include "../../groups/include/groups/GroupManager.h"
#include "../../visual_feedback/include/visual_feedback/FeedbackTypes.h"
#include "../../foundation/math/Vector3i.h"

namespace VoxelEditor {
namespace UndoRedo {
namespace Testing {

// Forward declarations for test event handlers
class TestVoxelChangedHandler;
class TestSelectionChangedHandler;
class TestGroupChangedHandler;

/**
 * @brief Comprehensive state validation infrastructure for command unit tests
 * 
 * This class provides tools to capture complete system state before and after
 * command execution, enabling thorough validation of state changes and ensuring
 * commands work correctly and undo operations maintain state integrity.
 * 
 * REQ-11.1.4: Command unit tests shall validate state changes before and after execution
 */
class CommandStateValidator {
public:
    struct ComparisonResult {
        bool identical;
        std::vector<std::string> differences;
        std::string summary;
        
        ComparisonResult() : identical(true) {}
        
        void addDifference(const std::string& diff) {
            differences.push_back(diff);
            identical = false;
        }
        
        std::string generateReport() const {
            if (identical) {
                return "States are identical";
            }
            
            std::string report = "State differences found:\n";
            for (const auto& diff : differences) {
                report += "- " + diff + "\n";
            }
            return report;
        }
    };
    
    /**
     * @brief Complete system state snapshot with groups support
     */
    struct ExtendedStateSnapshot {
        std::unique_ptr<StateSnapshot> baseSnapshot;
        
        // Groups state
        struct GroupsState {
            std::vector<Groups::GroupId> allGroupIds;
            std::unordered_map<Groups::GroupId, std::string> groupNames;
            std::unordered_map<Groups::GroupId, std::vector<Groups::VoxelId>> groupVoxels;
            std::unordered_map<Groups::GroupId, bool> groupVisibility;
            std::unordered_map<Groups::GroupId, bool> groupLocked;
            std::unordered_map<Groups::GroupId, Groups::GroupId> parentGroups; // child -> parent
            Groups::GroupId nextGroupId;
        } groupsState;
        
        // Event state (for validation)
        struct EventState {
            int voxelChangeEventCount;
            int selectionChangeEventCount;
            int groupChangeEventCount;
        } eventState;
        
        std::string description;
        std::chrono::system_clock::time_point timestamp;
    };

public:
    /**
     * @brief Constructor 
     * @param voxelManager VoxelDataManager instance to monitor
     * @param selectionManager SelectionManager instance to monitor (optional)
     * @param camera OrbitCamera instance to monitor (optional)
     * @param renderSettings RenderSettings instance to monitor (optional)
     * @param groupManager GroupManager instance to monitor (optional)
     */
    CommandStateValidator(
        VoxelData::VoxelDataManager* voxelManager,
        Selection::SelectionManager* selectionManager = nullptr,
        Camera::OrbitCamera* camera = nullptr,
        Rendering::RenderSettings* renderSettings = nullptr,
        Groups::GroupManager* groupManager = nullptr);
    
    ~CommandStateValidator();
    
    // State capture methods
    /**
     * @brief Capture complete system state
     * @param description Descriptive name for this state snapshot
     * @return Unique pointer to captured state
     */
    std::unique_ptr<ExtendedStateSnapshot> captureState(const std::string& description = "") const;
    
    /**
     * @brief Capture only voxel data state
     * @param description Descriptive name for this state snapshot
     * @return Unique pointer to captured voxel state
     */
    std::unique_ptr<ExtendedStateSnapshot> captureVoxelState(const std::string& description = "") const;
    
    // State comparison methods
    /**
     * @brief Compare two complete state snapshots
     * @param before First state snapshot
     * @param after Second state snapshot
     * @return Comparison result with detailed differences
     */
    ComparisonResult compareStates(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after) const;
    
    /**
     * @brief Compare only voxel data between two snapshots
     * @param before First state snapshot
     * @param after Second state snapshot
     * @return Comparison result focusing on voxel differences
     */
    ComparisonResult compareVoxelData(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after) const;
    
    /**
     * @brief Compare only selection state between two snapshots
     * @param before First state snapshot
     * @param after Second state snapshot
     * @return Comparison result focusing on selection differences
     */
    ComparisonResult compareSelections(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after) const;
    
    /**
     * @brief Compare only groups state between two snapshots
     * @param before First state snapshot
     * @param after Second state snapshot
     * @return Comparison result focusing on group differences
     */
    ComparisonResult compareGroups(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after) const;
    
    // Validation helpers
    /**
     * @brief Validate that a command properly changes state
     * @param beforeState State before command execution
     * @param afterState State after command execution
     * @param expectedChanges Description of expected changes
     * @return true if state changes match expectations
     */
    bool validateCommandExecution(
        const ExtendedStateSnapshot& beforeState,
        const ExtendedStateSnapshot& afterState,
        const std::vector<std::string>& expectedChanges = {}) const;
    
    /**
     * @brief Validate that undo restores exact previous state
     * @param originalState State before command execution
     * @param undoState State after undo operation
     * @return true if states are identical
     */
    bool validateUndoRestoration(
        const ExtendedStateSnapshot& originalState,
        const ExtendedStateSnapshot& undoState) const;
    
    // Event tracking integration
    /**
     * @brief Set event handlers for tracking state changes
     * @param voxelHandler Handler for voxel change events
     * @param selectionHandler Handler for selection change events (optional)
     * @param groupHandler Handler for group change events (optional)
     */
    void setEventHandlers(
        TestVoxelChangedHandler* voxelHandler,
        TestSelectionChangedHandler* selectionHandler = nullptr,
        TestGroupChangedHandler* groupHandler = nullptr);
    
    // Convenience methods for common test patterns
    /**
     * @brief Execute a command with full state validation
     * @param command Command to execute
     * @param expectedVoxelChanges Expected number of voxel change events
     * @param expectedChanges Description of expected state changes
     * @return true if command executed successfully and state changes are valid
     */
    template<typename CommandType>
    bool executeAndValidate(
        CommandType& command,
        int expectedVoxelChanges = 1,
        const std::vector<std::string>& expectedChanges = {});
    
    /**
     * @brief Execute undo with full state validation
     * @param command Command to undo
     * @param originalState State before original execution
     * @return true if undo succeeded and restored original state
     */
    template<typename CommandType>
    bool undoAndValidate(
        CommandType& command,
        const ExtendedStateSnapshot& originalState);
    
    // State inspection utilities
    /**
     * @brief Get detailed description of current system state
     * @return Human-readable description of all system state
     */
    std::string describeCurrentState() const;
    
    /**
     * @brief Get summary of differences between two states
     * @param before First state
     * @param after Second state
     * @return Human-readable summary of key differences
     */
    std::string describeDifferences(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after) const;

private:
    // System component references
    VoxelData::VoxelDataManager* m_voxelManager;
    Selection::SelectionManager* m_selectionManager;
    Camera::OrbitCamera* m_camera;
    Rendering::RenderSettings* m_renderSettings;
    Groups::GroupManager* m_groupManager;
    
    // Event handlers for tracking
    TestVoxelChangedHandler* m_voxelHandler;
    TestSelectionChangedHandler* m_selectionHandler;
    TestGroupChangedHandler* m_groupHandler;
    
    // Helper methods for state capture
    void captureGroupsState(ExtendedStateSnapshot::GroupsState& groupsState) const;
    void captureEventState(ExtendedStateSnapshot::EventState& eventState) const;
    
    // Helper methods for state comparison
    void compareVoxelDataInternal(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after,
        ComparisonResult& result) const;
    
    void compareSelectionsInternal(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after,
        ComparisonResult& result) const;
    
    void compareGroupsInternal(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after,
        ComparisonResult& result) const;
    
    void compareCameraInternal(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after,
        ComparisonResult& result) const;
    
    void compareRenderSettingsInternal(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after,
        ComparisonResult& result) const;
    
    // Helper methods for validation
    bool validateVoxelChanges(
        const ExtendedStateSnapshot& before,
        const ExtendedStateSnapshot& after,
        const std::vector<std::string>& expectedChanges) const;
};

// Test event handlers for comprehensive state tracking
class TestVoxelChangedHandler : public Events::EventHandler<Events::VoxelChangedEvent> {
public:
    void handleEvent(const Events::VoxelChangedEvent& event) override {
        eventCount++;
        lastEvent = event;
        allEvents.push_back(event);
    }
    
    void reset() {
        eventCount = 0;
        allEvents.clear();
    }
    
    int eventCount = 0;
    Events::VoxelChangedEvent lastEvent{Math::Vector3i::Zero(), VoxelData::VoxelResolution::Size_1cm, false, false};
    std::vector<Events::VoxelChangedEvent> allEvents;
};

class TestSelectionChangedHandler : public Events::EventHandler<Events::SelectionChangedEvent> {
public:
    void handleEvent(const Events::SelectionChangedEvent& event) override {
        eventCount++;
        lastEvent = event;
        allEvents.push_back(event);
    }
    
    void reset() {
        eventCount = 0;
        allEvents.clear();
    }
    
    int eventCount = 0;
    Events::SelectionChangedEvent lastEvent{Events::SelectionChangedEvent::ChangeType::ADDED, 0};
    std::vector<Events::SelectionChangedEvent> allEvents;
};

class TestGroupChangedHandler : public Events::EventHandler<Groups::GroupModifiedEvent> {
public:
    void handleEvent(const Groups::GroupModifiedEvent& event) override {
        eventCount++;
        lastEvent = event;
        allEvents.push_back(event);
    }
    
    void reset() {
        eventCount = 0;
        allEvents.clear();
    }
    
    int eventCount = 0;
    Groups::GroupModifiedEvent lastEvent;
    std::vector<Groups::GroupModifiedEvent> allEvents;
};

// Template method implementations
template<typename CommandType>
bool CommandStateValidator::executeAndValidate(
    CommandType& command,
    int expectedVoxelChanges,
    const std::vector<std::string>& expectedChanges) {
    
    // Capture state before execution
    auto beforeState = captureState("Before command execution");
    
    // Reset event counters
    if (m_voxelHandler) m_voxelHandler->reset();
    if (m_selectionHandler) m_selectionHandler->reset();
    if (m_groupHandler) m_groupHandler->reset();
    
    // Execute command
    bool executeResult = command.execute();
    if (!executeResult) {
        ADD_FAILURE() << "Command execution failed";
        return false;
    }
    
    // Capture state after execution
    auto afterState = captureState("After command execution");
    
    // Validate event counts
    if (m_voxelHandler && m_voxelHandler->eventCount != expectedVoxelChanges) {
        ADD_FAILURE() << "Expected " << expectedVoxelChanges << " voxel change events, got " 
                      << m_voxelHandler->eventCount;
        return false;
    }
    
    // Validate state changes
    if (!validateCommandExecution(*beforeState, *afterState, expectedChanges)) {
        ADD_FAILURE() << "Command state validation failed:\n" 
                      << describeDifferences(*beforeState, *afterState);
        return false;
    }
    
    return true;
}

template<typename CommandType>  
bool CommandStateValidator::undoAndValidate(
    CommandType& command,
    const ExtendedStateSnapshot& originalState) {
    
    // Reset event counters
    if (m_voxelHandler) m_voxelHandler->reset();
    if (m_selectionHandler) m_selectionHandler->reset();
    if (m_groupHandler) m_groupHandler->reset();
    
    // Execute undo
    bool undoResult = command.undo();
    if (!undoResult) {
        ADD_FAILURE() << "Command undo failed";
        return false;
    }
    
    // Capture state after undo
    auto undoState = captureState("After undo");
    
    // Validate that state was restored
    if (!validateUndoRestoration(originalState, *undoState)) {
        ADD_FAILURE() << "Undo state validation failed:\n"
                      << describeDifferences(originalState, *undoState);
        return false;
    }
    
    return true;
}

} // namespace Testing
} // namespace UndoRedo
} // namespace VoxelEditor