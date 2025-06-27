#include "cli/EditCommands.h"
#include "cli/Application.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "undo_redo/HistoryManager.h"
#include "undo_redo/VoxelCommands.h"
#include "undo_redo/PlacementCommands.h"
#include "math/CoordinateTypes.h"
#include "math/CoordinateConverter.h"
#include "math/BoundingBox.h"
#include <sstream>
#include <algorithm>

namespace VoxelEditor {

std::vector<CommandRegistration> EditCommands::getCommands() {
    return {
        // PLACE command
        CommandRegistration()
            .withName(Commands::PLACE)
            .withDescription("Place a voxel at position (coordinates must include units: cm or m)")
            .withCategory(CommandCategory::EDIT)
            .withAliases({"add", "set"})
            .withArg("x", "X coordinate with units (e.g. 100cm or 1m)", "coordinate", true)
            .withArg("y", "Y coordinate with units (e.g. 50cm or 0.5m)", "coordinate", true)
            .withArg("z", "Z coordinate with units (e.g. -100cm or -1m)", "coordinate", true)
            .withHandler([this](const CommandContext& ctx) {
                // Parse coordinates with units
                auto x_opt = ctx.getCoordinateArg(0);
                auto y_opt = ctx.getCoordinateArg(1);
                auto z_opt = ctx.getCoordinateArg(2);
                
                // Check if all coordinates were parsed successfully
                if (!x_opt) {
                    return CommandResult::Error("Invalid X coordinate. Must include units (e.g., 100cm or 1m)");
                }
                if (!y_opt) {
                    return CommandResult::Error("Invalid Y coordinate. Must include units (e.g., 50cm or 0.5m)");
                }
                if (!z_opt) {
                    return CommandResult::Error("Invalid Z coordinate. Must include units (e.g., -100cm or -1m)");
                }
                
                int x = *x_opt;
                int y = *y_opt;
                int z = *z_opt;
                
                // Use PlacementCommandFactory for validation
                auto cmd = UndoRedo::PlacementCommandFactory::createPlacementCommand(
                    m_voxelManager,
                    Math::IncrementCoordinates(Math::Vector3i(x, y, z)),
                    m_voxelManager->getActiveResolution()
                );
                
                if (!cmd) {
                    // Get validation details for error message
                    auto validation = UndoRedo::PlacementCommandFactory::validatePlacement(
                        m_voxelManager,
                        Math::IncrementCoordinates(Math::Vector3i(x, y, z)),
                        m_voxelManager->getActiveResolution()
                    );
                    
                    std::string errorMsg = "Cannot place voxel: ";
                    if (!validation.errors.empty()) {
                        errorMsg += validation.errors[0];
                    } else {
                        errorMsg += "Invalid position";
                    }
                    return CommandResult::Error(errorMsg);
                }
                
                bool success = m_historyManager->executeCommand(std::move(cmd));
                if (!success) {
                    return CommandResult::Error("Failed to place voxel at (" + 
                        std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
                }
                
                // Update the voxel mesh for rendering
                requestMeshUpdate();
                
                return CommandResult::Success("Voxel placed at (" + 
                    std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
            }),
            
        // DELETE command
        CommandRegistration()
            .withName(Commands::DELETE)
            .withDescription("Delete a voxel at position (coordinates must include units: cm or m)")
            .withCategory(CommandCategory::EDIT)
            .withAliases({"remove", "del"})
            .withArg("x", "X coordinate with units (e.g. 100cm or 1m)", "coordinate", true)
            .withArg("y", "Y coordinate with units (e.g. 50cm or 0.5m)", "coordinate", true)
            .withArg("z", "Z coordinate with units (e.g. -100cm or -1m)", "coordinate", true)
            .withHandler([this](const CommandContext& ctx) {
                // Parse coordinates with units
                auto x_opt = ctx.getCoordinateArg(0);
                auto y_opt = ctx.getCoordinateArg(1);
                auto z_opt = ctx.getCoordinateArg(2);
                
                // Check if all coordinates were parsed successfully
                if (!x_opt) {
                    return CommandResult::Error("Invalid X coordinate. Must include units (e.g., 100cm or 1m)");
                }
                if (!y_opt) {
                    return CommandResult::Error("Invalid Y coordinate. Must include units (e.g., 50cm or 0.5m)");
                }
                if (!z_opt) {
                    return CommandResult::Error("Invalid Z coordinate. Must include units (e.g., -100cm or -1m)");
                }
                
                int x = *x_opt;
                int y = *y_opt;
                int z = *z_opt;
                
                // Use PlacementCommandFactory for removal
                auto cmd = UndoRedo::PlacementCommandFactory::createRemovalCommand(
                    m_voxelManager,
                    Math::IncrementCoordinates(Math::Vector3i(x, y, z)),
                    m_voxelManager->getActiveResolution()
                );
                
                if (!cmd) {
                    return CommandResult::Error("No voxel at specified position");
                }
                
                m_historyManager->executeCommand(std::move(cmd));
                
                // Update the voxel mesh for rendering
                requestMeshUpdate();
                
                return CommandResult::Success("Voxel deleted at (" + 
                    std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
            }),
            
        // FILL command
        CommandRegistration()
            .withName(Commands::FILL)
            .withDescription("Fill a box region with voxels (coordinates must include units: cm or m)")
            .withCategory(CommandCategory::EDIT)
            .withArg("x1", "Start X with units (e.g. 0cm or 0m)", "coordinate", true)
            .withArg("y1", "Start Y with units (e.g. 0cm or 0m)", "coordinate", true)
            .withArg("z1", "Start Z with units (e.g. -100cm or -1m)", "coordinate", true)
            .withArg("x2", "End X with units (e.g. 200cm or 2m)", "coordinate", true)
            .withArg("y2", "End Y with units (e.g. 100cm or 1m)", "coordinate", true)
            .withArg("z2", "End Z with units (e.g. 100cm or 1m)", "coordinate", true)
            .withHandler([this](const CommandContext& ctx) {
                // Parse start coordinates with units
                auto x1_opt = ctx.getCoordinateArg(0);
                auto y1_opt = ctx.getCoordinateArg(1);
                auto z1_opt = ctx.getCoordinateArg(2);
                
                // Parse end coordinates with units
                auto x2_opt = ctx.getCoordinateArg(3);
                auto y2_opt = ctx.getCoordinateArg(4);
                auto z2_opt = ctx.getCoordinateArg(5);
                
                // Check if all coordinates were parsed successfully
                if (!x1_opt || !y1_opt || !z1_opt) {
                    return CommandResult::Error("Invalid start coordinates. Must include units (e.g., 0cm or 0m)");
                }
                if (!x2_opt || !y2_opt || !z2_opt) {
                    return CommandResult::Error("Invalid end coordinates. Must include units (e.g., 100cm or 1m)");
                }
                
                int x1 = *x1_opt;
                int y1 = *y1_opt;
                int z1 = *z1_opt;
                int x2 = *x2_opt;
                int y2 = *y2_opt;
                int z2 = *z2_opt;
                
                // Validate that all Y coordinates are >= 0 (ground plane constraint)
                if (y1 < 0 || y2 < 0) {
                    return CommandResult::Error("Fill command failed - Y coordinates must be >= 0 (cannot place voxels below ground plane)");
                }
                
                Math::IncrementCoordinates startInc(x1, y1, z1);
                Math::IncrementCoordinates endInc(x2, y2, z2);
                
                // Convert increment coordinates to world coordinates
                Math::Vector3f startWorld = Math::CoordinateConverter::incrementToWorld(startInc).value();
                Math::Vector3f endWorld = Math::CoordinateConverter::incrementToWorld(endInc).value();
                
                // Create bounds
                Math::Vector3f minF(std::min(startWorld.x, endWorld.x), std::min(startWorld.y, endWorld.y), std::min(startWorld.z, endWorld.z));
                Math::Vector3f maxF(std::max(startWorld.x, endWorld.x), std::max(startWorld.y, endWorld.y), std::max(startWorld.z, endWorld.z));
                Math::BoundingBox region(minF, maxF);
                
                auto cmd = std::make_unique<UndoRedo::VoxelFillCommand>(
                    m_voxelManager,
                    region,
                    m_voxelManager->getActiveResolution(),
                    true // fill with voxels
                );
                
                bool success = m_historyManager->executeCommand(std::move(cmd));
                
                if (!success) {
                    return CommandResult::Error("Fill command failed - some positions may be invalid (e.g., below ground plane)");
                }
                
                // Update the voxel mesh for rendering
                requestMeshUpdate();
                
                // Calculate volume filled
                int width = std::abs(x2 - x1) + 1;
                int height = std::abs(y2 - y1) + 1;
                int depth = std::abs(z2 - z1) + 1;
                int volume = width * height * depth;
                
                return CommandResult::Success("Filled " + std::to_string(volume) + " voxels");
            }),
            
        // UNDO command
        CommandRegistration()
            .withName(Commands::UNDO)
            .withDescription("Undo last operation")
            .withCategory(CommandCategory::EDIT)
            .withAliases({"u"})
            .withHandler([this](const CommandContext& ctx) {
                if (m_historyManager->undo()) {
                    // Update the voxel mesh after undo
                    requestMeshUpdate();
                    return CommandResult::Success("Undone");
                }
                return CommandResult::Error("Nothing to undo");
            }),
            
        // REDO command
        CommandRegistration()
            .withName(Commands::REDO)
            .withDescription("Redo last undone operation")
            .withCategory(CommandCategory::EDIT)
            .withAliases({"r"})
            .withHandler([this](const CommandContext& ctx) {
                if (m_historyManager->redo()) {
                    // Update the voxel mesh after redo
                    requestMeshUpdate();
                    return CommandResult::Success("Redone");
                }
                return CommandResult::Error("Nothing to redo");
            })
    };
}

// Auto-register this module
REGISTER_COMMAND_MODULE(EditCommands)

} // namespace VoxelEditor