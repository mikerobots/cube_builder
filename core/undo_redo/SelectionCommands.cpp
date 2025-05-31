#include "SelectionCommands.h"
#include "../../foundation/logging/Logger.h"
#include <sstream>

namespace VoxelEditor {
namespace UndoRedo {

// ModifySelectionCommand implementation
ModifySelectionCommand::ModifySelectionCommand(Selection::SelectionManager* selectionManager,
                                             const Selection::SelectionSet& selection,
                                             Selection::SelectionMode mode)
    : m_selectionManager(selectionManager)
    , m_selection(selection)
    , m_mode(mode) {
    
    if (!m_selectionManager) {
        throw std::invalid_argument("ModifySelectionCommand: SelectionManager cannot be null");
    }
    
    // Capture current selection
    m_previousSelection = m_selectionManager->getSelectionCopy();
}

bool ModifySelectionCommand::execute() {
    m_selectionManager->select(m_selection, m_mode);
    m_executed = true;
    return true;
}

bool ModifySelectionCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    m_selectionManager->select(m_previousSelection, Selection::SelectionMode::Replace);
    m_executed = false;
    return true;
}

std::string ModifySelectionCommand::getName() const {
    switch (m_mode) {
        case Selection::SelectionMode::Replace:
            return "Replace Selection";
        case Selection::SelectionMode::Add:
            return "Add to Selection";
        case Selection::SelectionMode::Subtract:
            return "Subtract from Selection";
        case Selection::SelectionMode::Intersect:
            return "Intersect Selection";
        default:
            return "Modify Selection";
    }
}

size_t ModifySelectionCommand::getMemoryUsage() const {
    return sizeof(*this) + 
           m_selection.size() * sizeof(Selection::VoxelId) +
           m_previousSelection.size() * sizeof(Selection::VoxelId);
}

// ClearSelectionCommand implementation
ClearSelectionCommand::ClearSelectionCommand(Selection::SelectionManager* selectionManager)
    : m_selectionManager(selectionManager) {
    
    if (!m_selectionManager) {
        throw std::invalid_argument("ClearSelectionCommand: SelectionManager cannot be null");
    }
    
    // Capture current selection
    m_previousSelection = m_selectionManager->getSelectionCopy();
}

bool ClearSelectionCommand::execute() {
    m_selectionManager->selectNone();
    m_executed = true;
    return true;
}

bool ClearSelectionCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    m_selectionManager->select(m_previousSelection, Selection::SelectionMode::Replace);
    m_executed = false;
    return true;
}

size_t ClearSelectionCommand::getMemoryUsage() const {
    return sizeof(*this) + m_previousSelection.size() * sizeof(Selection::VoxelId);
}

// SelectionSetOperationCommand implementation
SelectionSetOperationCommand::SelectionSetOperationCommand(Selection::SelectionManager* selectionManager,
                                                         const Selection::SelectionSet& operand,
                                                         Operation operation)
    : m_selectionManager(selectionManager)
    , m_operand(operand)
    , m_operation(operation) {
    
    if (!m_selectionManager) {
        throw std::invalid_argument("SelectionSetOperationCommand: SelectionManager cannot be null");
    }
    
    // Capture current selection
    m_previousSelection = m_selectionManager->getSelectionCopy();
}

bool SelectionSetOperationCommand::execute() {
    switch (m_operation) {
        case Operation::Union:
            m_selectionManager->unionWith(m_operand);
            break;
        case Operation::Intersection:
            m_selectionManager->intersectWith(m_operand);
            break;
        case Operation::Subtract:
            m_selectionManager->subtractFrom(m_operand);
            break;
    }
    
    m_executed = true;
    return true;
}

bool SelectionSetOperationCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    m_selectionManager->select(m_previousSelection, Selection::SelectionMode::Replace);
    m_executed = false;
    return true;
}

std::string SelectionSetOperationCommand::getName() const {
    switch (m_operation) {
        case Operation::Union:
            return "Union Selection";
        case Operation::Intersection:
            return "Intersect Selection";
        case Operation::Subtract:
            return "Subtract Selection";
        default:
            return "Selection Set Operation";
    }
}

size_t SelectionSetOperationCommand::getMemoryUsage() const {
    return sizeof(*this) + 
           m_operand.size() * sizeof(Selection::VoxelId) +
           m_previousSelection.size() * sizeof(Selection::VoxelId);
}

// SelectRegionCommand implementation
SelectRegionCommand::SelectRegionCommand(Selection::SelectionManager* selectionManager,
                                       const Selection::SelectionRegion& region,
                                       VoxelData::VoxelResolution resolution,
                                       Selection::SelectionMode mode)
    : m_selectionManager(selectionManager)
    , m_region(region)
    , m_resolution(resolution)
    , m_mode(mode) {
    
    if (!m_selectionManager) {
        throw std::invalid_argument("SelectRegionCommand: SelectionManager cannot be null");
    }
    
    // Capture current selection
    m_previousSelection = m_selectionManager->getSelectionCopy();
}

bool SelectRegionCommand::execute() {
    m_selectionManager->selectRegion(m_region, m_resolution, m_mode);
    m_executed = true;
    return true;
}

bool SelectRegionCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    m_selectionManager->select(m_previousSelection, Selection::SelectionMode::Replace);
    m_executed = false;
    return true;
}

size_t SelectRegionCommand::getMemoryUsage() const {
    return sizeof(*this) + m_previousSelection.size() * sizeof(Selection::VoxelId);
}

// InvertSelectionCommand implementation
InvertSelectionCommand::InvertSelectionCommand(Selection::SelectionManager* selectionManager)
    : m_selectionManager(selectionManager) {
    
    if (!m_selectionManager) {
        throw std::invalid_argument("InvertSelectionCommand: SelectionManager cannot be null");
    }
    
    // Capture current selection
    m_previousSelection = m_selectionManager->getSelectionCopy();
}

bool InvertSelectionCommand::execute() {
    m_selectionManager->selectInverse();
    m_executed = true;
    return true;
}

bool InvertSelectionCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    m_selectionManager->select(m_previousSelection, Selection::SelectionMode::Replace);
    m_executed = false;
    return true;
}

size_t InvertSelectionCommand::getMemoryUsage() const {
    return sizeof(*this) + m_previousSelection.size() * sizeof(Selection::VoxelId);
}

// SaveSelectionSetCommand implementation
SaveSelectionSetCommand::SaveSelectionSetCommand(Selection::SelectionManager* selectionManager,
                                               const std::string& name)
    : m_selectionManager(selectionManager)
    , m_setName(name) {
    
    if (!m_selectionManager) {
        throw std::invalid_argument("SaveSelectionSetCommand: SelectionManager cannot be null");
    }
    
    // Check if a set with this name already exists
    if (m_selectionManager->hasSelectionSet(m_setName)) {
        m_existedBefore = true;
        // Load it to save its content
        m_selectionManager->loadSelectionSet(m_setName);
        m_previousSetContent = m_selectionManager->getSelectionCopy();
    }
}

bool SaveSelectionSetCommand::execute() {
    m_selectionManager->saveSelectionSet(m_setName);
    m_executed = true;
    return true;
}

bool SaveSelectionSetCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    if (m_existedBefore) {
        // Restore the previous content
        m_selectionManager->select(m_previousSetContent, Selection::SelectionMode::Replace);
        m_selectionManager->saveSelectionSet(m_setName);
    } else {
        // Delete the set
        m_selectionManager->deleteSelectionSet(m_setName);
    }
    
    m_executed = false;
    return true;
}

size_t SaveSelectionSetCommand::getMemoryUsage() const {
    return sizeof(*this) + 
           m_setName.capacity() +
           m_previousSetContent.size() * sizeof(Selection::VoxelId);
}

}
}