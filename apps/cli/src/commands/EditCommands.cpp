#include "cli/EditCommands.h"
#include "cli/Application.h"
#include "cli/CommandTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "undo_redo/HistoryManager.h"
#include "undo_redo/VoxelCommands.h"
#include "undo_redo/PlacementCommands.h"
#include "math/CoordinateTypes.h"
#include "math/BoundingBox.h"
#include <sstream>
#include <iomanip>

namespace VoxelEditor {

REGISTER_COMMAND_MODULE(EditCommands)

std::vector<CommandRegistration> EditCommands::getCommands() {
    return {
        // PLACE command
        CommandRegistration()
            .withName(Commands::PLACE)
            .withDescription("Place a voxel at specified coordinates")
            .withCategory(CommandCategory::EDIT)
            .withAlias("p")
            .withArg("x", "X coordinate with units (e.g. 0cm, 5m)", "coordinate", true)
            .withArg("y", "Y coordinate with units", "coordinate", true)
            .withArg("z", "Z coordinate with units", "coordinate", true)
            .withHandler([this](const CommandContext& ctx) {
                // Parse coordinates with units
                auto x_opt = ctx.getCoordinateArg(0);
                auto y_opt = ctx.getCoordinateArg(1);
                auto z_opt = ctx.getCoordinateArg(2);
                
                // Check if all coordinates were parsed successfully
                if (!x_opt || !y_opt || !z_opt) {
                    return CommandResult::Error("Invalid coordinates. Must include units (e.g., 0cm or 0m)");
                }
                
                int x = *x_opt;
                int y = *y_opt;
                int z = *z_opt;
                
                // Create increment coordinates
                auto incCoords = Math::IncrementCoordinates(Math::Vector3i(x, y, z));
                
                // Validate placement
                if (!m_voxelManager->canPlaceVoxel(incCoords, m_voxelManager->getActiveResolution())) {
                    return CommandResult::Error("Cannot place voxel at position - collision detected");
                }
                
                // Create placement command
                auto cmd = UndoRedo::PlacementCommandFactory::createPlacementCommand(
                    m_voxelManager.get(),
                    incCoords,
                    m_voxelManager->getActiveResolution()
                );
                
                if (cmd && m_historyManager->executeCommand(std::move(cmd))) {
                    // Update rendering
                    requestMeshUpdate();
                    
                    std::stringstream ss;
                    ss << "Placed " << VoxelData::toString(m_voxelManager->getActiveResolution()) 
                       << " voxel at (" << x << "cm, " << y << "cm, " << z << "cm)";
                    return CommandResult::Success(ss.str());
                }
                
                return CommandResult::Error("Failed to place voxel");
            }),
            
        // DELETE command
        CommandRegistration()
            .withName(Commands::DELETE)
            .withDescription("Delete voxel at position")
            .withCategory(CommandCategory::EDIT)
            .withAliases({"remove", "d"})
            .withArg("x", "X coordinate", "int", true)
            .withArg("y", "Y coordinate", "int", true)
            .withArg("z", "Z coordinate", "int", true)
            .withHandler([this](const CommandContext& ctx) {
                int x = ctx.getIntArg(0);
                int y = ctx.getIntArg(1);
                int z = ctx.getIntArg(2);
                
                auto cmd = UndoRedo::PlacementCommandFactory::createRemovalCommand(
                    m_voxelManager.get(),
                    Math::IncrementCoordinates(Math::Vector3i(x, y, z)),
                    m_voxelManager->getActiveResolution()
                );
                
                if (cmd && m_historyManager->executeCommand(std::move(cmd))) {
                    requestMeshUpdate();
                    return CommandResult::Success("Voxel deleted");
                }
                
                return CommandResult::Error("No voxel at position");
            }),
            
        // FILL command
        CommandRegistration()
            .withName(Commands::FILL)
            .withDescription("Fill a box region with voxels")
            .withCategory(CommandCategory::EDIT)
            .withAlias("f")
            .withArg("x1", "Start X", "int", true)
            .withArg("y1", "Start Y", "int", true)
            .withArg("z1", "Start Z", "int", true)
            .withArg("x2", "End X", "int", true)
            .withArg("y2", "End Y", "int", true)
            .withArg("z2", "End Z", "int", true)
            .withHandler([this](const CommandContext& ctx) {
                int x1 = ctx.getIntArg(0);
                int y1 = ctx.getIntArg(1);
                int z1 = ctx.getIntArg(2);
                int x2 = ctx.getIntArg(3);
                int y2 = ctx.getIntArg(4);
                int z2 = ctx.getIntArg(5);
                
                // Calculate bounds
                Math::Vector3f min(
                    static_cast<float>(std::min(x1, x2)),
                    static_cast<float>(std::min(y1, y2)),
                    static_cast<float>(std::min(z1, z2))
                );
                Math::Vector3f max(
                    static_cast<float>(std::max(x1, x2)),
                    static_cast<float>(std::max(y1, y2)),
                    static_cast<float>(std::max(z1, z2))
                );
                
                Math::BoundingBox box(min, max);
                
                // Create fill command
                auto cmd = std::make_unique<UndoRedo::FillVoxelsCommand>(
                    m_voxelManager.get(),
                    box,
                    m_voxelManager->getActiveResolution()
                );
                
                size_t count = cmd->getVoxelCount();
                
                if (m_historyManager->executeCommand(std::move(cmd))) {
                    requestMeshUpdate();
                    
                    std::stringstream ss;
                    ss << "Filled " << count << " voxels";
                    return CommandResult::Success(ss.str());
                }
                
                return CommandResult::Error("Fill operation failed");
            }),
            
        // UNDO command
        CommandRegistration()
            .withName(Commands::UNDO)
            .withDescription("Undo last operation")
            .withCategory(CommandCategory::EDIT)
            .withAlias("u")
            .withHandler([this](const CommandContext& ctx) {
                if (m_historyManager->undo()) {
                    requestMeshUpdate();
                    return CommandResult::Success("Undo completed");
                }
                return CommandResult::Error("Nothing to undo");
            }),
            
        // REDO command
        CommandRegistration()
            .withName(Commands::REDO)
            .withDescription("Redo last undone operation")
            .withCategory(CommandCategory::EDIT)
            .withAlias("r")
            .withHandler([this](const CommandContext& ctx) {
                if (m_historyManager->redo()) {
                    requestMeshUpdate();
                    return CommandResult::Success("Redo completed");
                }
                return CommandResult::Error("Nothing to redo");
            })
    };
}

} // namespace VoxelEditor