#pragma once

#include "VoxelCommands.h"
#include "../voxel_data/VoxelDataManager.h"
#include "../voxel_data/VoxelTypes.h"
#include "../../foundation/math/Vector3i.h"
#include <memory>

namespace VoxelEditor {
namespace UndoRedo {

/**
 * @brief Factory class for creating placement and removal commands
 * 
 * This class provides convenience methods for creating undo/redo commands
 * for voxel placement and removal operations, with appropriate validation
 * and error handling.
 */
class PlacementCommandFactory {
public:
    /**
     * @brief Create a voxel placement command
     * @param voxelManager The voxel data manager
     * @param position The position to place the voxel
     * @param resolution The resolution of the voxel to place
     * @return Unique pointer to the command, or nullptr if invalid
     */
    static std::unique_ptr<Command> createPlacementCommand(
        VoxelData::VoxelDataManager* voxelManager,
        const Math::Vector3i& position,
        VoxelData::VoxelResolution resolution);
    
    /**
     * @brief Create a voxel removal command
     * @param voxelManager The voxel data manager
     * @param position The position to remove the voxel from
     * @param resolution The resolution of the voxel to remove
     * @return Unique pointer to the command, or nullptr if invalid
     */
    static std::unique_ptr<Command> createRemovalCommand(
        VoxelData::VoxelDataManager* voxelManager,
        const Math::Vector3i& position,
        VoxelData::VoxelResolution resolution);
    
    /**
     * @brief Validate a placement operation
     * @param voxelManager The voxel data manager
     * @param position The position to place the voxel
     * @param resolution The resolution of the voxel to place
     * @return ValidationResult with any errors or warnings
     */
    static ValidationResult validatePlacement(
        VoxelData::VoxelDataManager* voxelManager,
        const Math::Vector3i& position,
        VoxelData::VoxelResolution resolution);
    
    /**
     * @brief Validate a removal operation
     * @param voxelManager The voxel data manager
     * @param position The position to remove the voxel from
     * @param resolution The resolution of the voxel to remove
     * @return ValidationResult with any errors or warnings
     */
    static ValidationResult validateRemoval(
        VoxelData::VoxelDataManager* voxelManager,
        const Math::Vector3i& position,
        VoxelData::VoxelResolution resolution);
};

/**
 * @brief Specialized command for voxel placement with enhanced validation
 * 
 * This command wraps VoxelEditCommand but provides additional validation
 * and context-specific behavior for placement operations.
 */
class VoxelPlacementCommand : public Command {
public:
    VoxelPlacementCommand(VoxelData::VoxelDataManager* voxelManager,
                         const Math::Vector3i& position,
                         VoxelData::VoxelResolution resolution);
    
    bool execute() override;
    bool undo() override;
    bool canUndo() const override;
    
    std::string getName() const override { return "Place Voxel"; }
    std::string getDescription() const override;
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override;
    
    bool canMergeWith(const Command& other) const override;
    std::unique_ptr<Command> mergeWith(std::unique_ptr<Command> other) override;
    
    ValidationResult validate() const override;
    
    // Getters for debugging/logging
    Math::Vector3i getPosition() const { return m_position; }
    VoxelData::VoxelResolution getResolution() const { return m_resolution; }
    
private:
    std::unique_ptr<VoxelEditCommand> m_baseCommand;
    Math::Vector3i m_position;
    VoxelData::VoxelResolution m_resolution;
    VoxelData::VoxelDataManager* m_voxelManager;
};

/**
 * @brief Specialized command for voxel removal with enhanced validation
 * 
 * This command wraps VoxelEditCommand but provides additional validation
 * and context-specific behavior for removal operations.
 */
class VoxelRemovalCommand : public Command {
public:
    VoxelRemovalCommand(VoxelData::VoxelDataManager* voxelManager,
                       const Math::Vector3i& position,
                       VoxelData::VoxelResolution resolution);
    
    bool execute() override;
    bool undo() override;
    bool canUndo() const override;
    
    std::string getName() const override { return "Remove Voxel"; }
    std::string getDescription() const override;
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override;
    
    bool canMergeWith(const Command& other) const override;
    std::unique_ptr<Command> mergeWith(std::unique_ptr<Command> other) override;
    
    ValidationResult validate() const override;
    
    // Getters for debugging/logging
    Math::Vector3i getPosition() const { return m_position; }
    VoxelData::VoxelResolution getResolution() const { return m_resolution; }
    
private:
    std::unique_ptr<VoxelEditCommand> m_baseCommand;
    Math::Vector3i m_position;
    VoxelData::VoxelResolution m_resolution;
    VoxelData::VoxelDataManager* m_voxelManager;
};

} // namespace UndoRedo
} // namespace VoxelEditor