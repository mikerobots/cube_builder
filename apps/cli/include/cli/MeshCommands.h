#pragma once

#include "CommandRegistry.h"

namespace VoxelEditor {

/**
 * @brief Mesh and surface generation commands module
 * 
 * Commands for mesh operations and surface generation:
 * - smooth: Control mesh smoothing settings (level, algorithm, preview)
 * - mesh: Mesh validation and information (info, validate, repair)
 * - surface-export: Export surface mesh to various formats (STL, OBJ)
 * - surface-preview: Enable/disable real-time surface preview
 * - surface-settings: Configure surface generation settings
 * 
 * These commands provide comprehensive control over the surface generation
 * pipeline, including smoothing algorithms, quality settings, and export options.
 */
class MeshCommands : public ICommandModule {
public:
    std::vector<CommandRegistration> getCommands() override;
};

} // namespace VoxelEditor