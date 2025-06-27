#pragma once

#include "CommandRegistry.h"

namespace VoxelEditor {

/**
 * @brief Selection operation commands module
 * 
 * Commands for managing voxel selection:
 * - select: Select voxel at position
 * - select-box: Select voxels in box region
 * - select-sphere: Select voxels in sphere region
 * - select-all: Select all voxels
 * - select-none: Clear selection
 * - select-resolution: Select all voxels of a specific resolution
 * - invert-selection: Invert current selection
 * - selection-info: Show selection information
 * - delete-selected: Delete all selected voxels
 * - group-selected: Create group from selection
 */
class SelectCommands : public ICommandModule {
public:
    std::vector<CommandRegistration> getCommands() override;
};

} // namespace VoxelEditor