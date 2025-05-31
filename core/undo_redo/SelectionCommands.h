#pragma once

#include "Command.h"
#include "../selection/SelectionManager.h"
#include "../selection/SelectionSet.h"
#include "../selection/SelectionTypes.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/BoundingBox.h"
#include "../../foundation/math/Matrix4f.h"
#include <memory>
#include <vector>

namespace VoxelEditor {
namespace UndoRedo {

// Command for modifying the current selection
class ModifySelectionCommand : public Command {
public:
    ModifySelectionCommand(Selection::SelectionManager* selectionManager,
                          const Selection::SelectionSet& selection,
                          Selection::SelectionMode mode);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override;
    CommandType getType() const override { return CommandType::Selection; }
    size_t getMemoryUsage() const override;
    
private:
    Selection::SelectionManager* m_selectionManager;
    Selection::SelectionSet m_selection;
    Selection::SelectionMode m_mode;
    Selection::SelectionSet m_previousSelection;
};

// Command for clearing selection
class ClearSelectionCommand : public Command {
public:
    explicit ClearSelectionCommand(Selection::SelectionManager* selectionManager);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Clear Selection"; }
    CommandType getType() const override { return CommandType::Selection; }
    size_t getMemoryUsage() const override;
    
private:
    Selection::SelectionManager* m_selectionManager;
    Selection::SelectionSet m_previousSelection;
};

// Command for set operations on selection
class SelectionSetOperationCommand : public Command {
public:
    enum class Operation {
        Union,
        Intersection,
        Subtract
    };
    
    SelectionSetOperationCommand(Selection::SelectionManager* selectionManager,
                                const Selection::SelectionSet& operand,
                                Operation operation);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override;
    CommandType getType() const override { return CommandType::Selection; }
    size_t getMemoryUsage() const override;
    
private:
    Selection::SelectionManager* m_selectionManager;
    Selection::SelectionSet m_operand;
    Operation m_operation;
    Selection::SelectionSet m_previousSelection;
};

// Command for selecting a region
class SelectRegionCommand : public Command {
public:
    SelectRegionCommand(Selection::SelectionManager* selectionManager,
                       const Selection::SelectionRegion& region,
                       VoxelData::VoxelResolution resolution,
                       Selection::SelectionMode mode);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Select Region"; }
    CommandType getType() const override { return CommandType::Selection; }
    size_t getMemoryUsage() const override;
    
private:
    Selection::SelectionManager* m_selectionManager;
    Selection::SelectionRegion m_region;
    VoxelData::VoxelResolution m_resolution;
    Selection::SelectionMode m_mode;
    Selection::SelectionSet m_previousSelection;
};

// Command for inverting selection
class InvertSelectionCommand : public Command {
public:
    explicit InvertSelectionCommand(Selection::SelectionManager* selectionManager);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Invert Selection"; }
    CommandType getType() const override { return CommandType::Selection; }
    size_t getMemoryUsage() const override;
    
private:
    Selection::SelectionManager* m_selectionManager;
    Selection::SelectionSet m_previousSelection;
};

// Command for saving/loading selection sets
class SaveSelectionSetCommand : public Command {
public:
    SaveSelectionSetCommand(Selection::SelectionManager* selectionManager,
                           const std::string& name);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Save Selection Set"; }
    CommandType getType() const override { return CommandType::Selection; }
    size_t getMemoryUsage() const override;
    
private:
    Selection::SelectionManager* m_selectionManager;
    std::string m_setName;
    bool m_existedBefore = false;
    Selection::SelectionSet m_previousSetContent;
};

}
}