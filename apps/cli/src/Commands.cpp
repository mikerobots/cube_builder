// Standard library includes first
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include <algorithm>

// Application headers
#include "cli/Application.h"
#include "cli/CommandProcessor.h"

// Core includes
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "file_io/FileManager.h"
#include "file_io/STLExporter.h"
#include "file_io/Project.h"
#include "groups/GroupManager.h"
#include "groups/GroupTypes.h"
#include "selection/SelectionManager.h"
#include "selection/SelectionTypes.h"
#include "surface_gen/SurfaceGenerator.h"
#include "undo_redo/HistoryManager.h"
#include "undo_redo/VoxelCommands.h"
#include "visual_feedback/FeedbackRenderer.h"
#include "math/BoundingBox.h"

namespace VoxelEditor {

void Application::registerCommands() {
    // File Operations
    m_commandProcessor->registerCommand({
        Commands::NEW,
        "Create a new project",
        CommandCategory::FILE,
        {},
        {},
        [this](const CommandContext& ctx) {
            m_voxelManager->clearAll();
            m_selectionManager->selectNone();
            // Clear all groups
            auto groupIds = m_groupManager->getAllGroupIds();
            for (auto id : groupIds) {
                m_groupManager->deleteGroup(id);
            }
            m_historyManager->clearHistory();
            m_currentProject.clear();
            m_cameraController->setViewPreset(Camera::ViewPreset::ISOMETRIC);
            return CommandResult::Success("New project created.");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::OPEN,
        "Open a project file",
        CommandCategory::FILE,
        {"load"},
        {{"filename", "Path to project file", "string", true, ""}},
        [this](const CommandContext& ctx) {
            std::string filename = ctx.getArg(0);
            if (filename.empty()) {
                return CommandResult::Error("Filename required");
            }
            
            FileIO::Project project;
            FileIO::LoadOptions options;
            auto result = m_fileManager->loadProject(filename, project, options);
            if (result.success) {
                m_currentProject = filename;
                return CommandResult::Success("Project loaded: " + filename);
            }
            return CommandResult::Error("Failed to load project: " + filename);
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::SAVE,
        "Save the current project",
        CommandCategory::FILE,
        {},
        {{"filename", "Path to save file (optional)", "string", false, ""}},
        [this](const CommandContext& ctx) {
            std::string filename = ctx.getArg(0, m_currentProject);
            if (filename.empty()) {
                return CommandResult::Error("No filename specified and no current project");
            }
            
            // Create project from current state
            FileIO::Project project;
            // TODO: Project needs to be populated with current data
            // For now, FileManager should handle serialization internally
            
            FileIO::SaveOptions options;
            auto result = m_fileManager->saveProject(filename, project, options);
            if (result.success) {
                m_currentProject = filename;
                return CommandResult::Success("Project saved: " + filename);
            }
            return CommandResult::Error("Failed to save project");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::SAVE_AS,
        "Save the project with a new name",
        CommandCategory::FILE,
        {},
        {{"filename", "Path to save file", "string", true, ""}},
        [this](const CommandContext& ctx) {
            std::string filename = ctx.getArg(0);
            if (filename.empty()) {
                return CommandResult::Error("Filename required");
            }
            
            // Create project from current state
            FileIO::Project project;
            // TODO: Project needs to be populated with current data
            // For now, FileManager should handle serialization internally
            
            FileIO::SaveOptions options;
            auto result = m_fileManager->saveProject(filename, project, options);
            if (result.success) {
                m_currentProject = filename;
                return CommandResult::Success("Project saved as: " + filename);
            }
            return CommandResult::Error("Failed to save project");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::EXPORT,
        "Export to STL format",
        CommandCategory::FILE,
        {},
        {{"filename", "Path to STL file", "string", true, ""}},
        [this](const CommandContext& ctx) {
            std::string filename = ctx.getArg(0);
            if (filename.empty()) {
                return CommandResult::Error("Filename required");
            }
            
            // Generate surface mesh
            SurfaceGen::SurfaceGenerator surfaceGen(m_eventDispatcher.get());
            auto surfaceMesh = surfaceGen.generateMultiResMesh(*m_voxelManager, m_voxelManager->getActiveResolution());
            
            // Convert to Rendering::Mesh for STL export
            Rendering::Mesh renderMesh;
            renderMesh.vertices.reserve(surfaceMesh.vertices.size());
            for (size_t i = 0; i < surfaceMesh.vertices.size(); ++i) {
                Rendering::Vertex vertex;
                vertex.position = surfaceMesh.vertices[i];
                if (i < surfaceMesh.normals.size()) {
                    vertex.normal = surfaceMesh.normals[i];
                }
                renderMesh.vertices.push_back(vertex);
            }
            renderMesh.indices = surfaceMesh.indices;
            
            FileIO::STLExportOptions options;
            options.format = FileIO::STLFormat::Binary;
            
            auto result = m_fileManager->exportSTL(filename, renderMesh, options);
            if (result.success) {
                return CommandResult::Success("Exported to: " + filename);
            }
            return CommandResult::Error("Failed to export STL");
        }
    });
    
    // Edit Operations
    m_commandProcessor->registerCommand({
        Commands::PLACE,
        "Place a voxel at position",
        CommandCategory::EDIT,
        {"add", "set"},
        {
            {"x", "X coordinate", "int", true, ""},
            {"y", "Y coordinate", "int", true, ""},
            {"z", "Z coordinate", "int", true, ""}
        },
        [this](const CommandContext& ctx) {
            int x = ctx.getIntArg(0);
            int y = ctx.getIntArg(1);
            int z = ctx.getIntArg(2);
            
            auto cmd = std::make_unique<UndoRedo::VoxelEditCommand>(
                m_voxelManager.get(),
                Math::Vector3i(x, y, z),
                m_voxelManager->getActiveResolution(),
                true // place voxel
            );
            
            m_historyManager->executeCommand(std::move(cmd));
            return CommandResult::Success("Voxel placed at (" + 
                std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::DELETE,
        "Delete a voxel at position",
        CommandCategory::EDIT,
        {"remove", "del"},
        {
            {"x", "X coordinate", "int", true, ""},
            {"y", "Y coordinate", "int", true, ""},
            {"z", "Z coordinate", "int", true, ""}
        },
        [this](const CommandContext& ctx) {
            int x = ctx.getIntArg(0);
            int y = ctx.getIntArg(1);
            int z = ctx.getIntArg(2);
            
            auto cmd = std::make_unique<UndoRedo::VoxelEditCommand>(
                m_voxelManager.get(),
                Math::Vector3i(x, y, z),
                m_voxelManager->getActiveResolution(),
                false // remove voxel
            );
            
            m_historyManager->executeCommand(std::move(cmd));
            return CommandResult::Success("Voxel deleted at (" + 
                std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::FILL,
        "Fill a box region with voxels",
        CommandCategory::EDIT,
        {},
        {
            {"x1", "Start X", "int", true, ""},
            {"y1", "Start Y", "int", true, ""},
            {"z1", "Start Z", "int", true, ""},
            {"x2", "End X", "int", true, ""},
            {"y2", "End Y", "int", true, ""},
            {"z2", "End Z", "int", true, ""}
        },
        [this](const CommandContext& ctx) {
            Math::Vector3i start(ctx.getIntArg(0), ctx.getIntArg(1), ctx.getIntArg(2));
            Math::Vector3i end(ctx.getIntArg(3), ctx.getIntArg(4), ctx.getIntArg(5));
            
            // Create bounds
            Math::Vector3f minF(std::min(start.x, end.x), std::min(start.y, end.y), std::min(start.z, end.z));
            Math::Vector3f maxF(std::max(start.x, end.x), std::max(start.y, end.y), std::max(start.z, end.z));
            Math::BoundingBox region(minF, maxF);
            
            auto cmd = std::make_unique<UndoRedo::VoxelFillCommand>(
                m_voxelManager.get(),
                region,
                m_voxelManager->getActiveResolution(),
                true // fill with voxels
            );
            
            m_historyManager->executeCommand(std::move(cmd));
            
            // Calculate volume filled
            int width = std::abs(end.x - start.x) + 1;
            int height = std::abs(end.y - start.y) + 1;
            int depth = std::abs(end.z - start.z) + 1;
            int volume = width * height * depth;
            
            return CommandResult::Success("Filled " + std::to_string(volume) + " voxels");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::UNDO,
        "Undo last operation",
        CommandCategory::EDIT,
        {"u"},
        {},
        [this](const CommandContext& ctx) {
            if (m_historyManager->undo()) {
                return CommandResult::Success("Undone");
            }
            return CommandResult::Error("Nothing to undo");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::REDO,
        "Redo last undone operation",
        CommandCategory::EDIT,
        {"r"},
        {},
        [this](const CommandContext& ctx) {
            if (m_historyManager->redo()) {
                return CommandResult::Success("Redone");
            }
            return CommandResult::Error("Nothing to redo");
        }
    });
    
    // View Controls
    m_commandProcessor->registerCommand({
        Commands::CAMERA,
        "Set camera view preset",
        CommandCategory::VIEW,
        {"view"},
        {{"preset", "View preset (front/back/left/right/top/bottom/iso/default)", "string", true, ""}},
        [this](const CommandContext& ctx) {
            std::string preset = ctx.getArg(0);
            
            Camera::ViewPreset viewPreset;
            if (preset == "front") viewPreset = Camera::ViewPreset::FRONT;
            else if (preset == "back") viewPreset = Camera::ViewPreset::BACK;
            else if (preset == "left") viewPreset = Camera::ViewPreset::LEFT;
            else if (preset == "right") viewPreset = Camera::ViewPreset::RIGHT;
            else if (preset == "top") viewPreset = Camera::ViewPreset::TOP;
            else if (preset == "bottom") viewPreset = Camera::ViewPreset::BOTTOM;
            else if (preset == "iso") viewPreset = Camera::ViewPreset::ISOMETRIC;
            else if (preset == "default") viewPreset = Camera::ViewPreset::ISOMETRIC;
            else {
                return CommandResult::Error("Unknown preset: " + preset);
            }
            
            m_cameraController->setViewPreset(viewPreset);
            return CommandResult::Success("Camera set to " + preset + " view");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::ZOOM,
        "Zoom camera in/out",
        CommandCategory::VIEW,
        {"z"},
        {{"factor", "Zoom factor (e.g., 1.5 to zoom in, 0.8 to zoom out)", "float", true, ""}},
        [this](const CommandContext& ctx) {
            float factor = ctx.getFloatArg(0, 1.0f);
            if (factor <= 0) {
                return CommandResult::Error("Zoom factor must be positive");
            }
            
            float currentDistance = m_cameraController->getCamera()->getDistance();
            m_cameraController->getCamera()->setDistance(currentDistance / factor);
            
            return CommandResult::Success("Zoomed by factor " + std::to_string(factor));
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::ROTATE,
        "Rotate camera",
        CommandCategory::VIEW,
        {"rot"},
        {
            {"x", "X rotation in degrees", "float", true, ""},
            {"y", "Y rotation in degrees", "float", true, ""}
        },
        [this](const CommandContext& ctx) {
            float deltaX = ctx.getFloatArg(0, 0.0f);
            float deltaY = ctx.getFloatArg(1, 0.0f);
            
            dynamic_cast<Camera::OrbitCamera*>(m_cameraController->getCamera())->orbit(
                deltaX,
                deltaY
            );
            
            return CommandResult::Success("Camera rotated");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::RESET_VIEW,
        "Reset camera to default view",
        CommandCategory::VIEW,
        {"reset"},
        {},
        [this](const CommandContext& ctx) {
            m_cameraController->setViewPreset(Camera::ViewPreset::ISOMETRIC);
            return CommandResult::Success("Camera reset to default view");
        }
    });
    
    // Resolution control
    m_commandProcessor->registerCommand({
        "resolution",
        "Set voxel resolution",
        CommandCategory::EDIT,
        {"res"},
        {{"size", "Resolution (1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm)", "string", true, ""}},
        [this](const CommandContext& ctx) {
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
        }
    });
    
    // Workspace control
    m_commandProcessor->registerCommand({
        "workspace",
        "Set workspace dimensions",
        CommandCategory::EDIT,
        {"ws"},
        {
            {"width", "Width in meters", "float", true, ""},
            {"height", "Height in meters", "float", true, ""},
            {"depth", "Depth in meters", "float", true, ""}
        },
        [this](const CommandContext& ctx) {
            float width = ctx.getFloatArg(0);
            float height = ctx.getFloatArg(1);
            float depth = ctx.getFloatArg(2);
            
            Math::Vector3f size(width, height, depth);
            if (m_voxelManager->resizeWorkspace(size)) {
                return CommandResult::Success("Workspace resized to " + 
                    std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(depth) + " meters");
            }
            return CommandResult::Error("Failed to resize workspace. Check size constraints (2-8m³)");
        }
    });
    
    // Selection commands
    m_commandProcessor->registerCommand({
        Commands::SELECT,
        "Select voxels at position",
        CommandCategory::SELECT,
        {"sel"},
        {
            {"x", "X coordinate", "int", true, ""},
            {"y", "Y coordinate", "int", true, ""},
            {"z", "Z coordinate", "int", true, ""}
        },
        [this](const CommandContext& ctx) {
            Math::Vector3i pos(ctx.getIntArg(0), ctx.getIntArg(1), ctx.getIntArg(2));
            
            if (m_voxelManager->hasVoxel(pos, m_voxelManager->getActiveResolution())) {
                Selection::VoxelId id(pos, m_voxelManager->getActiveResolution());
                m_selectionManager->selectVoxel(id);
                return CommandResult::Success("Voxel selected");
            }
            return CommandResult::Error("No voxel at position");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::SELECT_BOX,
        "Select voxels in box region",
        CommandCategory::SELECT,
        {"selbox"},
        {
            {"x1", "Start X", "int", true, ""},
            {"y1", "Start Y", "int", true, ""},
            {"z1", "Start Z", "int", true, ""},
            {"x2", "End X", "int", true, ""},
            {"y2", "End Y", "int", true, ""},
            {"z2", "End Z", "int", true, ""}
        },
        [this](const CommandContext& ctx) {
            Math::Vector3f min(ctx.getIntArg(0), ctx.getIntArg(1), ctx.getIntArg(2));
            Math::Vector3f max(ctx.getIntArg(3), ctx.getIntArg(4), ctx.getIntArg(5));
            
            Math::BoundingBox box(min, max);
            m_selectionManager->selectBox(box, m_voxelManager->getActiveResolution());
            size_t count = m_selectionManager->getSelectionSize();
            
            return CommandResult::Success("Selected " + std::to_string(count) + " voxels");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::SELECT_ALL,
        "Select all voxels",
        CommandCategory::SELECT,
        {"selall"},
        {},
        [this](const CommandContext& ctx) {
            m_selectionManager->selectAll();
            size_t count = m_selectionManager->getSelectionSize();
            return CommandResult::Success("Selected " + std::to_string(count) + " voxels");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::SELECT_NONE,
        "Clear selection",
        CommandCategory::SELECT,
        {"selnone", "deselect"},
        {},
        [this](const CommandContext& ctx) {
            m_selectionManager->selectNone();
            return CommandResult::Success("Selection cleared");
        }
    });
    
    // Group commands
    m_commandProcessor->registerCommand({
        Commands::GROUP,
        "Create group from selection",
        CommandCategory::GROUP,
        {"g"},
        {{"name", "Group name", "string", true, ""}},
        [this](const CommandContext& ctx) {
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
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::GROUP_LIST,
        "List all groups",
        CommandCategory::GROUP,
        {"groups", "gl"},
        {},
        [this](const CommandContext& ctx) {
            auto groupIds = m_groupManager->getAllGroupIds();
            std::vector<Groups::GroupInfo> groups;
            for (auto id : groupIds) {
                auto group = m_groupManager->getGroup(id);
                if (group) {
                    Groups::GroupInfo info;
                    info.id = id;
                    info.name = group->getName();
                    info.voxelCount = group->getVoxelCount();
                    info.visible = group->isVisible();
                    info.locked = group->isLocked();
                    groups.push_back(info);
                }
            }
            if (groups.empty()) {
                return CommandResult::Success("No groups");
            }
            
            std::stringstream ss;
            ss << "Groups:\n";
            for (const auto& info : groups) {
                ss << "  " << std::left << std::setw(20) << info.name 
                   << " (" << info.voxelCount << " voxels";
                if (!info.visible) ss << ", hidden";
                if (info.locked) ss << ", locked";
                ss << ")\n";
            }
            
            return CommandResult::Success(ss.str());
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::GROUP_HIDE,
        "Hide a group",
        CommandCategory::GROUP,
        {"hide"},
        {{"name", "Group name", "string", true, ""}},
        [this](const CommandContext& ctx) {
            std::string name = ctx.getArg(0);
            auto groupIds = m_groupManager->findGroupsByName(name);
            
            if (!groupIds.empty()) {
                // Hide the first group with matching name
                m_groupManager->hideGroup(groupIds[0]);
                return CommandResult::Success("Group '" + name + "' hidden");
            }
            return CommandResult::Error("Group not found: " + name);
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::GROUP_SHOW,
        "Show a group",
        CommandCategory::GROUP,
        {"show"},
        {{"name", "Group name", "string", true, ""}},
        [this](const CommandContext& ctx) {
            std::string name = ctx.getArg(0);
            auto groupIds = m_groupManager->findGroupsByName(name);
            
            if (!groupIds.empty()) {
                // Show the first group with matching name
                m_groupManager->showGroup(groupIds[0]);
                return CommandResult::Success("Group '" + name + "' shown");
            }
            return CommandResult::Error("Group not found: " + name);
        }
    });
    
    // Status command
    m_commandProcessor->registerCommand({
        Commands::STATUS,
        "Show editor status",
        CommandCategory::SYSTEM,
        {"info", "stats"},
        {},
        [this](const CommandContext& ctx) {
            std::stringstream ss;
            ss << "Voxel Editor Status\n";
            ss << "==================\n";
            
            // Project info
            if (!m_currentProject.empty()) {
                ss << "Project: " << m_currentProject << "\n";
            } else {
                ss << "Project: <unsaved>\n";
            }
            
            // Resolution
            auto res = m_voxelManager->getActiveResolution();
            ss << "Resolution: " << VoxelData::getVoxelSizeName(res) << "\n";
            
            // Workspace
            auto wsSize = m_voxelManager->getWorkspaceSize();
            ss << "Workspace: " << wsSize.x << "x" << wsSize.y << "x" << wsSize.z << " meters\n";
            
            // Voxel count
            size_t voxelCount = m_voxelManager->getVoxelCount();
            ss << "Voxels: " << voxelCount << "\n";
            
            // Selection
            size_t selCount = m_selectionManager->getSelectionSize();
            ss << "Selected: " << selCount << " voxels\n";
            
            // Groups
            auto groupIds = m_groupManager->getAllGroupIds();
            ss << "Groups: " << groupIds.size() << "\n";
            
            // Memory
            size_t memUsage = m_voxelManager->getMemoryUsage();
            ss << "Memory: " << (memUsage / (1024.0 * 1024.0)) << " MB\n";
            
            return CommandResult::Success(ss.str());
        }
    });
}

} // namespace VoxelEditor