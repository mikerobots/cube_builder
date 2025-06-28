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
                
                // Validate placement first to get detailed error message
                auto validation = UndoRedo::PlacementCommandFactory::validatePlacement(
                    m_voxelManager, incCoords, m_voxelManager->getActiveResolution()
                );
                
                if (!validation.valid) {
                    return CommandResult::Error(validation.errors.empty() ? "Invalid placement" : validation.errors[0]);
                }
                
                // Create placement command (validation already passed)
                auto cmd = UndoRedo::PlacementCommandFactory::createPlacementCommand(
                    m_voxelManager,
                    incCoords,
                    m_voxelManager->getActiveResolution()
                );
                
                if (cmd && m_historyManager->executeCommand(std::move(cmd))) {
                    // Update rendering
                    requestMeshUpdate();
                    
                    std::stringstream ss;
                    ss << "Placed " << VoxelData::getVoxelSizeName(m_voxelManager->getActiveResolution()) 
                       << " voxel at (" << x << "cm, " << y << "cm, " << z << "cm)";
                    return CommandResult::Success(ss.str());
                }
                
                return CommandResult::Error("Failed to place voxel");
            }),
            
        // RESOLUTION command
        CommandRegistration()
            .withName("resolution")
            .withDescription("Set active voxel resolution")
            .withCategory(CommandCategory::EDIT)
            .withAlias("res")
            .withArg("size", "Resolution (1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm)", "string", true)
            .withHandler([this](const CommandContext& ctx) {
                std::string size = ctx.getArg(0);
                
                VoxelData::VoxelResolution resolution;
                if (size == "1cm") resolution = VoxelData::VoxelResolution::Size_1cm;
                else if (size == "2cm") resolution = VoxelData::VoxelResolution::Size_2cm;
                else if (size == "4cm") resolution = VoxelData::VoxelResolution::Size_4cm;
                else if (size == "8cm") resolution = VoxelData::VoxelResolution::Size_8cm;
                else if (size == "16cm") resolution = VoxelData::VoxelResolution::Size_16cm;
                else if (size == "32cm") resolution = VoxelData::VoxelResolution::Size_32cm;
                else if (size == "64cm") resolution = VoxelData::VoxelResolution::Size_64cm;
                else if (size == "128cm") resolution = VoxelData::VoxelResolution::Size_128cm;
                else if (size == "256cm") resolution = VoxelData::VoxelResolution::Size_256cm;
                else if (size == "512cm") resolution = VoxelData::VoxelResolution::Size_512cm;
                else {
                    return CommandResult::Error("Invalid resolution. Use: 1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm");
                }
                
                m_voxelManager->setActiveResolution(resolution);
                return CommandResult::Success("Resolution set to " + size);
            }),
            
        // DELETE command
        CommandRegistration()
            .withName(Commands::DELETE)
            .withDescription("Delete voxel at position")
            .withCategory(CommandCategory::EDIT)
            .withAliases({"remove", "d"})
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
                
                // Validate removal first to get detailed error message
                auto validation = UndoRedo::PlacementCommandFactory::validateRemoval(
                    m_voxelManager, incCoords, m_voxelManager->getActiveResolution()
                );
                
                if (!validation.valid) {
                    return CommandResult::Error(validation.errors.empty() ? "Invalid removal" : validation.errors[0]);
                }
                
                // Create removal command (validation already passed)
                auto cmd = UndoRedo::PlacementCommandFactory::createRemovalCommand(
                    m_voxelManager,
                    incCoords,
                    m_voxelManager->getActiveResolution()
                );
                
                if (cmd && m_historyManager->executeCommand(std::move(cmd))) {
                    requestMeshUpdate();
                    return CommandResult::Success("Voxel deleted");
                }
                
                return CommandResult::Error("No voxel at specified position");
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
                
                // Calculate bounds - convert from centimeters to meters
                // Fill command receives integer centimeter values, but BoundingBox needs meters
                Math::Vector3f min(
                    static_cast<float>(std::min(x1, x2)) / 100.0f,  // Convert cm to meters
                    static_cast<float>(std::min(y1, y2)) / 100.0f,
                    static_cast<float>(std::min(z1, z2)) / 100.0f
                );
                Math::Vector3f max(
                    static_cast<float>(std::max(x1, x2)) / 100.0f,  // Convert cm to meters
                    static_cast<float>(std::max(y1, y2)) / 100.0f,
                    static_cast<float>(std::max(z1, z2)) / 100.0f
                );
                
                Math::BoundingBox box(min, max);
                
                // Create fill command
                auto cmd = std::make_unique<UndoRedo::VoxelFillCommand>(
                    m_voxelManager,
                    box,
                    m_voxelManager->getActiveResolution(),
                    true  // fill with voxels (not remove)
                );
                
                // Count voxels in the region for feedback
                size_t count = 0;
                // Get voxel size in centimeters
                int resSize = static_cast<int>(VoxelData::getVoxelSize(m_voxelManager->getActiveResolution()) * 100.0f);
                if (resSize == 0) resSize = 1; // Safety check for 1cm voxels
                
                // Use the original centimeter values for counting
                int minX = std::min(x1, x2);
                int minY = std::min(y1, y2);
                int minZ = std::min(z1, z2);
                int maxX = std::max(x1, x2);
                int maxY = std::max(y1, y2);
                int maxZ = std::max(z1, z2);
                
                for (int x = minX; x <= maxX; x += resSize) {
                    for (int y = minY; y <= maxY; y += resSize) {
                        for (int z = minZ; z <= maxZ; z += resSize) {
                            count++;
                        }
                    }
                }
                
                if (m_historyManager->executeCommand(std::move(cmd))) {
                    requestMeshUpdate();
                    
                    std::stringstream ss;
                    ss << "Filled " << count << " voxels";
                    return CommandResult::Success(ss.str());
                }
                
                // Retrieve detailed error message from the failed command
                std::string errorMsg = m_historyManager->getLastCommandError();
                
                // Debug: log what error we got
                Logging::Logger::getInstance().debug("Fill command failed. Error from history: '" + errorMsg + "'");
                
                // If we have a detailed error message, use it
                if (!errorMsg.empty()) {
                    return CommandResult::Error(errorMsg);
                }
                
                // Fallback to generic error if no specific error was set
                // Debug: log the fill region details
                std::stringstream debugMsg;
                debugMsg << "Fill operation failed for region (" 
                         << x1 << "," << y1 << "," << z1 << ") to ("
                         << x2 << "," << y2 << "," << z2 << ") - "
                         << "BoundingBox: (" << box.min.x << "," << box.min.y << "," << box.min.z 
                         << ") to (" << box.max.x << "," << box.max.y << "," << box.max.z << ")";
                Logging::Logger::getInstance().error(debugMsg.str());
                
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