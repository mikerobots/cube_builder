#include "cli/SelectCommands.h"
#include "cli/Application.h"
#include "cli/CommandTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "selection/SelectionManager.h"
#include "selection/SelectionTypes.h"
#include "groups/GroupManager.h"
#include "groups/GroupTypes.h"
#include "undo_redo/HistoryManager.h"
#include "undo_redo/VoxelCommands.h"
#include "undo_redo/PlacementCommands.h"
#include "math/CoordinateTypes.h"
#include "math/BoundingBox.h"
#include "math/Vector3f.h"
#include <sstream>
#include <iomanip>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>

namespace VoxelEditor {

std::vector<CommandRegistration> SelectCommands::getCommands() {
    return {
        // SELECT command - select voxel at position
        CommandRegistration()
            .withName(Commands::SELECT)
            .withDescription("Select voxels at position")
            .withCategory(CommandCategory::SELECT)
            .withAlias("sel")
            .withArg("x", "X coordinate", "int", true)
            .withArg("y", "Y coordinate", "int", true)
            .withArg("z", "Z coordinate", "int", true)
            .withHandler([this](const CommandContext& ctx) {
                Math::Vector3i pos(ctx.getIntArg(0), ctx.getIntArg(1), ctx.getIntArg(2));
                
                if (m_voxelManager->hasVoxel(pos, m_voxelManager->getActiveResolution())) {
                    Selection::VoxelId id(pos, m_voxelManager->getActiveResolution());
                    m_selectionManager->selectVoxel(id);
                    return CommandResult::Success("Voxel selected");
                }
                return CommandResult::Error("No voxel at position");
            }),
            
        // SELECT_BOX command - select voxels in box region
        CommandRegistration()
            .withName(Commands::SELECT_BOX)
            .withDescription("Select voxels in box region (coordinates must include units: cm or m)")
            .withCategory(CommandCategory::SELECT)
            .withAlias("selbox")
            .withArg("x1", "Start X with units (e.g. -100cm or -1m)", "coordinate", true)
            .withArg("y1", "Start Y with units (e.g. 0cm or 0m)", "coordinate", true)
            .withArg("z1", "Start Z with units (e.g. -100cm or -1m)", "coordinate", true)
            .withArg("x2", "End X with units (e.g. 100cm or 1m)", "coordinate", true)
            .withArg("y2", "End Y with units (e.g. 200cm or 2m)", "coordinate", true)
            .withArg("z2", "End Z with units (e.g. 100cm or 1m)", "coordinate", true)
            .withHandler([this](const CommandContext& ctx) {
                // Parse coordinates with units
                auto x1_opt = ctx.getCoordinateArg(0);
                auto y1_opt = ctx.getCoordinateArg(1);
                auto z1_opt = ctx.getCoordinateArg(2);
                auto x2_opt = ctx.getCoordinateArg(3);
                auto y2_opt = ctx.getCoordinateArg(4);
                auto z2_opt = ctx.getCoordinateArg(5);
                
                // Check if all coordinates were parsed successfully
                if (!x1_opt || !y1_opt || !z1_opt) {
                    return CommandResult::Error("Invalid start coordinates. Must include units (e.g., -100cm or -1m)");
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
                
                Math::Vector3f min(x1, y1, z1);
                Math::Vector3f max(x2, y2, z2);
                
                Math::BoundingBox box(min, max);
                m_selectionManager->selectBox(box, m_voxelManager->getActiveResolution());
                size_t count = m_selectionManager->getSelectionSize();
                
                return CommandResult::Success("Selected " + std::to_string(count) + " voxels");
            }),
            
        // SELECT_SPHERE command - select voxels in sphere region
        CommandRegistration()
            .withName(Commands::SELECT_SPHERE)
            .withDescription("Select voxels in sphere region (coordinates must include units: cm or m)")
            .withCategory(CommandCategory::SELECT)
            .withAlias("selsphere")
            .withArg("x", "Center X with units (e.g. 0cm or 0m)", "coordinate", true)
            .withArg("y", "Center Y with units (e.g. 50cm or 0.5m)", "coordinate", true)
            .withArg("z", "Center Z with units (e.g. 0cm or 0m)", "coordinate", true)
            .withArg("radius", "Radius with units (e.g. 100cm or 1m)", "coordinate", true)
            .withHandler([this](const CommandContext& ctx) {
                // Parse coordinates with units
                auto x_opt = ctx.getCoordinateArg(0);
                auto y_opt = ctx.getCoordinateArg(1);
                auto z_opt = ctx.getCoordinateArg(2);
                auto radius_opt = ctx.getCoordinateArg(3);
                
                // Check if all coordinates were parsed successfully
                if (!x_opt || !y_opt || !z_opt) {
                    return CommandResult::Error("Invalid center coordinates. Must include units (e.g., 0cm or 0m)");
                }
                if (!radius_opt) {
                    return CommandResult::Error("Invalid radius. Must include units (e.g., 100cm or 1m)");
                }
                
                int x = *x_opt;
                int y = *y_opt;
                int z = *z_opt;
                int radius = *radius_opt;
                
                if (radius <= 0) {
                    return CommandResult::Error("Radius must be positive");
                }
                
                Math::Vector3f center(x, y, z);
                m_selectionManager->selectSphere(center, static_cast<float>(radius), 
                                                       m_voxelManager->getActiveResolution());
                size_t count = m_selectionManager->getSelectionSize();
                
                return CommandResult::Success("Selected " + std::to_string(count) + " voxels");
            }),
            
        // SELECT_ALL command
        CommandRegistration()
            .withName(Commands::SELECT_ALL)
            .withDescription("Select all voxels")
            .withCategory(CommandCategory::SELECT)
            .withAlias("selall")
            .withHandler([this](const CommandContext& ctx) {
                m_selectionManager->selectAll();
                size_t count = m_selectionManager->getSelectionSize();
                return CommandResult::Success("Selected " + std::to_string(count) + " voxels");
            }),
            
        // SELECT_NONE command
        CommandRegistration()
            .withName(Commands::SELECT_NONE)
            .withDescription("Clear selection")
            .withCategory(CommandCategory::SELECT)
            .withAliases({"selnone", "deselect"})
            .withHandler([this](const CommandContext& ctx) {
                m_selectionManager->selectNone();
                return CommandResult::Success("Selection cleared");
            }),
            
        // SELECT_RESOLUTION command
        CommandRegistration()
            .withName("select-resolution")
            .withDescription("Select all voxels of a specific resolution")
            .withCategory(CommandCategory::SELECT)
            .withAlias("selres")
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
                
                // Get all voxels of the specified resolution
                auto voxels = m_voxelManager->getAllVoxels(resolution);
                
                // Clear current selection and select all voxels of this resolution
                m_selectionManager->selectNone();
                for (const auto& voxelPos : voxels) {
                    Selection::VoxelId id(voxelPos.incrementPos.value(), voxelPos.resolution);
                    m_selectionManager->selectVoxel(id);
                }
                
                size_t count = m_selectionManager->getSelectionSize();
                return CommandResult::Success("Selected " + std::to_string(count) + " voxels at " + size + " resolution");
            }),
            
        // INVERT_SELECTION command
        CommandRegistration()
            .withName("invert-selection")
            .withDescription("Invert current selection")
            .withCategory(CommandCategory::SELECT)
            .withAlias("selinvert")
            .withHandler([this](const CommandContext& ctx) {
                // Get current selection
                auto currentSelection = m_selectionManager->getSelection();
                std::set<Selection::VoxelId> currentSet(currentSelection.begin(), currentSelection.end());
                
                // Clear selection
                m_selectionManager->selectNone();
                
                // Get all voxels
                auto allVoxels = m_voxelManager->getAllVoxels();
                
                // Select all voxels that were not in the original selection
                for (const auto& voxelPos : allVoxels) {
                    Selection::VoxelId id(voxelPos.incrementPos.value(), voxelPos.resolution);
                    if (currentSet.find(id) == currentSet.end()) {
                        m_selectionManager->selectVoxel(id);
                    }
                }
                
                size_t count = m_selectionManager->getSelectionSize();
                return CommandResult::Success("Inverted selection: " + std::to_string(count) + " voxels selected");
            }),
            
        // SELECTION_INFO command
        CommandRegistration()
            .withName("selection-info")
            .withDescription("Show selection information")
            .withCategory(CommandCategory::SELECT)
            .withAliases({"selinfo", "si"})
            .withHandler([this](const CommandContext& ctx) {
                size_t count = m_selectionManager->getSelectionSize();
                
                if (count == 0) {
                    return CommandResult::Success("No voxels selected");
                }
                
                std::stringstream ss;
                ss << "Selection Information:\n";
                ss << "  Total voxels: " << count << "\n";
                
                // Count voxels by resolution
                std::map<VoxelData::VoxelResolution, size_t> resolutionCounts;
                auto selection = m_selectionManager->getSelection();
                
                for (const auto& voxel : selection) {
                    resolutionCounts[voxel.resolution]++;
                }
                
                ss << "  By resolution:\n";
                for (const auto& [res, cnt] : resolutionCounts) {
                    ss << "    " << VoxelData::getVoxelSizeName(res) << ": " << cnt << " voxels\n";
                }
                
                // Calculate bounds of selection
                if (!selection.empty()) {
                    auto firstVoxel = selection.begin();
                    Math::Vector3f minBounds(static_cast<float>(firstVoxel->position.x()),
                                           static_cast<float>(firstVoxel->position.y()),
                                           static_cast<float>(firstVoxel->position.z()));
                    Math::Vector3f maxBounds = minBounds;
                    
                    for (const auto& voxel : selection) {
                        minBounds.x = std::min(minBounds.x, static_cast<float>(voxel.position.x()));
                        minBounds.y = std::min(minBounds.y, static_cast<float>(voxel.position.y()));
                        minBounds.z = std::min(minBounds.z, static_cast<float>(voxel.position.z()));
                        maxBounds.x = std::max(maxBounds.x, static_cast<float>(voxel.position.x()));
                        maxBounds.y = std::max(maxBounds.y, static_cast<float>(voxel.position.y()));
                        maxBounds.z = std::max(maxBounds.z, static_cast<float>(voxel.position.z()));
                    }
                    
                    ss << "  Bounds:\n";
                    ss << "    Min: (" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << ")\n";
                    ss << "    Max: (" << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")\n";
                    ss << "    Size: (" << (maxBounds.x - minBounds.x + 1) << ", " 
                       << (maxBounds.y - minBounds.y + 1) << ", " 
                       << (maxBounds.z - minBounds.z + 1) << ")\n";
                }
                
                return CommandResult::Success(ss.str());
            }),
            
        // DELETE_SELECTED command
        CommandRegistration()
            .withName("delete-selected")
            .withDescription("Delete all selected voxels")
            .withCategory(CommandCategory::SELECT)
            .withAliases({"delsel", "ds"})
            .withHandler([this](const CommandContext& ctx) {
                auto selection = m_selectionManager->getSelection();
                
                if (selection.empty()) {
                    return CommandResult::Error("No voxels selected");
                }
                
                // Use BulkVoxelEditCommand for efficient deletion of multiple voxels
                std::vector<UndoRedo::BulkVoxelEditCommand::VoxelChange> changes;
                changes.reserve(selection.size());
                
                for (const auto& voxel : selection) {
                    changes.emplace_back(
                        Math::IncrementCoordinates(voxel.position),
                        voxel.resolution,
                        true,  // oldValue (voxel exists)
                        false  // newValue (delete voxel)
                    );
                }
                
                auto bulkCmd = std::make_unique<UndoRedo::BulkVoxelEditCommand>(
                    m_voxelManager,
                    changes
                );
                
                size_t count = selection.size();
                
                if (m_historyManager->executeCommand(std::move(bulkCmd))) {
                    // Clear selection after deletion
                    m_selectionManager->selectNone();
                    
                    // Update the voxel mesh
                    requestMeshUpdate();
                    
                    return CommandResult::Success("Deleted " + std::to_string(count) + " selected voxels");
                }
                
                return CommandResult::Error("Failed to delete selected voxels");
            }),
            
        // GROUP_SELECTED command
        CommandRegistration()
            .withName("group-selected")
            .withDescription("Create group from selected voxels")
            .withCategory(CommandCategory::SELECT)
            .withAliases({"groupsel", "gs"})
            .withArg("name", "Group name", "string", true)
            .withHandler([this](const CommandContext& ctx) {
                std::string name = ctx.getArg(0);
                if (name.empty()) {
                    return CommandResult::Error("Group name required");
                }
                
                auto selection = m_selectionManager->getSelection();
                if (selection.empty()) {
                    return CommandResult::Error("No voxels selected");
                }
                
                // Convert Selection::VoxelId to Groups::VoxelId
                std::vector<Groups::VoxelId> groupVoxels;
                groupVoxels.reserve(selection.size());
                for (const auto& voxel : selection) {
                    groupVoxels.emplace_back(voxel.position, voxel.resolution);
                }
                
                Groups::GroupId id = m_groupManager->createGroup(name, groupVoxels);
                if (id != Groups::INVALID_GROUP_ID) {
                    return CommandResult::Success("Created group '" + name + "' with " + 
                        std::to_string(groupVoxels.size()) + " voxels");
                }
                return CommandResult::Error("Failed to create group");
            })
    };
}

// Auto-register this module
REGISTER_COMMAND_MODULE(SelectCommands)

} // namespace VoxelEditor