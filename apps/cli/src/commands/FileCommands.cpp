#include "cli/FileCommands.h"
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/CommandTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "selection/SelectionManager.h"
#include "groups/GroupManager.h"
#include "undo_redo/HistoryManager.h"
#include "camera/CameraController.h"
#include "camera/OrbitCamera.h"
#include "file_io/FileManager.h"
#include "file_io/STLExporter.h"
#include "file_io/Project.h"
#include "file_io/FileTypes.h"
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sstream>

namespace VoxelEditor {

// Helper function for file validation
static bool isValidFilename(const std::string& filename, const std::string& expectedExtension) {
    if (filename.empty()) return false;
    if (filename == "/dev/null") return false;
    
    if (!expectedExtension.empty() && 
        (filename.length() < expectedExtension.length() || 
         filename.substr(filename.length() - expectedExtension.length()) != expectedExtension)) {
        return false;
    }
    
    return true;
}

std::vector<CommandRegistration> FileCommands::getCommands() {
    return {
        // NEW command
        CommandRegistration()
            .withName(Commands::NEW)
            .withDescription("Create a new project")
            .withCategory(CommandCategory::FILE)
            .withHandler([](Application* app, const CommandContext& ctx) {
                app->getVoxelManager()->clearAll();
                app->getSelectionManager()->selectNone();
                
                // Clear all groups
                auto groupIds = app->getGroupManager()->getAllGroupIds();
                for (auto id : groupIds) {
                    app->getGroupManager()->deleteGroup(id);
                }
                
                app->getHistoryManager()->clearHistory();
                app->setCurrentProject("");
                app->getCameraController()->setViewPreset(Camera::ViewPreset::ISOMETRIC);
                
                return CommandResult::Success("New project created.");
            }),
            
        // OPEN/LOAD command
        CommandRegistration()
            .withName(Commands::OPEN)
            .withDescription("Open a project file")
            .withCategory(CommandCategory::FILE)
            .withAlias("load")
            .withArg("filename", "Path to project file", "string", true)
            .withHandler([](Application* app, const CommandContext& ctx) {
                std::string filename = ctx.getArg(0);
                
                if (!isValidFilename(filename, ".vxl")) {
                    return CommandResult::Error("Invalid filename. File must end with .vxl");
                }
                
                FileIO::Project project;
                auto result = app->getFileManager()->loadProject(filename, project);
                
                if (!result.success) {
                    return CommandResult::Error("Failed to open project: " + result.message);
                }
                
                // Clear current state
                app->getVoxelManager()->clearAll();
                app->getSelectionManager()->selectNone();
                
                // Apply loaded project data
                // ... (rest of implementation)
                
                app->setCurrentProject(filename);
                return CommandResult::Success("Project loaded from: " + filename);
            }),
            
        // SAVE command
        CommandRegistration()
            .withName(Commands::SAVE)
            .withDescription("Save the current project")
            .withCategory(CommandCategory::FILE)
            .withHandler([](Application* app, const CommandContext& ctx) {
                const std::string& currentProject = app->getCurrentProject();
                
                if (currentProject.empty()) {
                    return CommandResult::Error("No project file specified. Use 'save <filename>' or 'save_as'");
                }
                
                // Create project data
                FileIO::Project project;
                project.metadata.name = currentProject;
                project.metadata.description = "VoxelEditor Project";
                project.metadata.version = FileIO::FileVersion{1, 0, 0};
                
                // Add timestamp
                auto now = std::chrono::system_clock::now();
                project.metadata.created = now;
                project.metadata.modified = now;
                
                // Gather project data
                project.voxelData = app->getVoxelManager();
                project.camera = std::make_shared<Camera::OrbitCamera>(*app->getCameraController()->getCamera());
                
                // Attempt to save
                auto result = app->getFileManager()->saveProject(currentProject, project);
                
                if (!result.success) {
                    return CommandResult::Error("Failed to save project: " + result.message);
                }
                
                return CommandResult::Success("Project saved to: " + currentProject);
            }),
            
        // SAVE_AS command
        CommandRegistration()
            .withName(Commands::SAVE_AS)
            .withDescription("Save project with a new name")
            .withCategory(CommandCategory::FILE)
            .withArg("filename", "New filename for the project", "string", true)
            .withHandler([](Application* app, const CommandContext& ctx) {
                std::string filename = ctx.getArg(0);
                
                if (!isValidFilename(filename, ".vxl")) {
                    return CommandResult::Error("Invalid filename. File must end with .vxl");
                }
                
                // Set as current project and delegate to save
                app->setCurrentProject(filename);
                
                // Call save handler
                CommandContext saveCtx(app, Commands::SAVE, {});
                auto saveHandler = CommandRegistry::getInstance().findHandler(Commands::SAVE);
                if (saveHandler) {
                    return saveHandler(app, saveCtx);
                }
                
                return CommandResult::Error("Internal error: Save handler not found");
            }),
            
        // EXPORT command
        CommandRegistration()
            .withName(Commands::EXPORT)
            .withDescription("Export voxel model to various formats")
            .withCategory(CommandCategory::FILE)
            .withArg("filename", "Output filename", "string", true)
            .withArg("format", "Export format (stl, obj, ply)", "string", false, "stl")
            .withHandler([](Application* app, const CommandContext& ctx) {
                std::string filename = ctx.getArg(0);
                std::string format = ctx.getArg(1);
                
                if (filename.empty()) {
                    return CommandResult::Error("Filename required");
                }
                
                // Currently only STL is supported
                if (format != "stl") {
                    return CommandResult::Error("Unsupported format. Currently only 'stl' is supported.");
                }
                
                // Ensure filename has correct extension
                const std::string stlExt = ".stl";
                if (filename.length() < stlExt.length() || 
                    filename.substr(filename.length() - stlExt.length()) != stlExt) {
                    filename += stlExt;
                }
                
                // Generate surface mesh
                auto surfaceGen = app->getSurfaceGenerator();
                auto mesh = surfaceGen->generateSurface(*app->getVoxelManager());
                
                if (!mesh || mesh->triangles.empty()) {
                    return CommandResult::Error("No geometry to export. Place some voxels first.");
                }
                
                // Export to STL
                FileIO::STLExporter exporter;
                if (!exporter.exportBinary(filename, *mesh)) {
                    return CommandResult::Error("Failed to export STL file");
                }
                
                std::stringstream ss;
                ss << "Exported " << mesh->triangles.size() << " triangles to: " << filename;
                return CommandResult::Success(ss.str());
            })
    };
}

// Register this module automatically
REGISTER_COMMAND_MODULE(FileCommands)

} // namespace VoxelEditor