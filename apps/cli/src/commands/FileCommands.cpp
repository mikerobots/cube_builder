#include "cli/FileCommands.h"
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/CommandTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "selection/SelectionManager.h"
#include "groups/GroupManager.h"
#include "undo_redo/HistoryManager.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "camera/OrbitCamera.h"
#include "file_io/FileManager.h"
#include "file_io/STLExporter.h"
#include "file_io/Project.h"
#include "file_io/FileTypes.h"
#include "surface_gen/SurfaceGenerator.h"
#include "surface_gen/MeshSmoother.h"
#include "rendering/RenderTypes.h"
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdlib>

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
            .withHandler([this](const CommandContext& ctx) {
                m_voxelManager->clearAll();
                m_selectionManager->selectNone();
                // Clear all groups
                auto groupIds = m_groupManager->getAllGroupIds();
                for (auto id : groupIds) {
                    m_groupManager->deleteGroup(id);
                }
                m_historyManager->clearHistory();
                m_currentProject.clear();
                m_app->setCurrentProject("");
                m_cameraController->setViewPreset(Camera::ViewPreset::ISOMETRIC);
                return CommandResult::Success("New project created.");
            }),
            
        // OPEN/LOAD command
        CommandRegistration()
            .withName(Commands::OPEN)
            .withDescription("Open a project file")
            .withCategory(CommandCategory::FILE)
            .withAlias("load")
            .withArg("filename", "Path to project file", "string", true)
            .withHandler([this](const CommandContext& ctx) {
                std::string filename = ctx.getArg(0);
                if (filename.empty()) {
                    return CommandResult::Error("Filename required");
                }
                
                // Validate filename
                if (filename == "/dev/null") {
                    return CommandResult::Error("Invalid filename: /dev/null");
                }
                
                // Check file extension
                const std::string validExtension = ".vxl";
                if (filename.length() < validExtension.length() || 
                    filename.substr(filename.length() - validExtension.length()) != validExtension) {
                    return CommandResult::Error("Invalid file extension. File must end with .vxl");
                }
                
                FileIO::Project project;
                // Initialize project components before loading
                // This is required because BinaryFormat expects these to be non-null
                // Use a temporary EventDispatcher to avoid conflicts
                auto tempEventDispatcher = std::make_shared<Events::EventDispatcher>();
                project.voxelData = std::make_shared<VoxelData::VoxelDataManager>(tempEventDispatcher.get());
                project.groupData = std::make_shared<Groups::GroupManager>(project.voxelData.get(), tempEventDispatcher.get());
                project.camera = std::make_shared<Camera::OrbitCamera>();
                project.currentSelection = std::make_shared<Selection::SelectionSet>();
                
                FileIO::LoadOptions options;
                auto result = m_fileManager->loadProject(filename, project, options);
                if (result.success) {
                    // Clear current voxel data
                    for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
                        auto resolution = static_cast<VoxelData::VoxelResolution>(i);
                        auto voxels = m_voxelManager->getAllVoxels(resolution);
                        for (const auto& voxelPos : voxels) {
                            m_voxelManager->setVoxel(voxelPos.incrementPos, voxelPos.resolution, false);
                        }
                    }
                    
                    // Copy loaded voxel data to app's voxel manager
                    size_t totalVoxelsLoaded = 0;
                    for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
                        auto resolution = static_cast<VoxelData::VoxelResolution>(i);
                        auto voxels = project.voxelData->getAllVoxels(resolution);
                        for (const auto& voxelPos : voxels) {
                            m_voxelManager->setVoxel(voxelPos.incrementPos, voxelPos.resolution, true);
                            totalVoxelsLoaded++;
                        }
                    }
                    
                    // Restore workspace settings
                    m_voxelManager->resizeWorkspace(project.workspace.size);
                    m_voxelManager->setActiveResolution(project.workspace.defaultResolution);
                    
                    // Restore camera state
                    if (m_cameraController && m_cameraController->getCamera() && project.camera) {
                        auto appCamera = m_cameraController->getCamera();
                        appCamera->setPosition(project.camera->getPosition());
                        appCamera->setTarget(project.camera->getTarget());
                        appCamera->setDistance(project.camera->getDistance());
                    }
                    
                    m_currentProject = filename;
                    m_app->setCurrentProject(filename);
                    
                    // Request mesh update to render loaded voxels
                    requestMeshUpdate();
                    
                    return CommandResult::Success("Project loaded: " + filename);
                }
                return CommandResult::Error("Failed to load project: " + filename);
            }),
            
        // SAVE command
        CommandRegistration()
            .withName(Commands::SAVE)
            .withDescription("Save the current project")
            .withCategory(CommandCategory::FILE)
            .withArg("filename", "Path to save file (optional)", "string", false)
            .withHandler([this](const CommandContext& ctx) {
                std::string filename = ctx.getArg(0, m_currentProject);
                if (filename.empty()) {
                    return CommandResult::Error("No filename specified and no current project");
                }
                
                // Validate filename
                if (filename == "/dev/null") {
                    return CommandResult::Error("Invalid filename: /dev/null");
                }
                
                // Check file extension
                const std::string validExtension = ".vxl";
                if (filename.length() < validExtension.length() || 
                    filename.substr(filename.length() - validExtension.length()) != validExtension) {
                    return CommandResult::Error("Invalid file extension. File must end with .vxl");
                }
                
                // Create project from current state
                FileIO::Project project;
                // Don't call initializeDefaults() - it creates a dangling EventDispatcher
                // Instead, manually initialize the shared_ptrs with proper objects
                project.voxelData = std::make_shared<VoxelData::VoxelDataManager>(m_eventDispatcher);
                project.groupData = std::make_shared<Groups::GroupManager>(project.voxelData.get(), m_eventDispatcher);
                project.camera = std::make_shared<Camera::OrbitCamera>();
                project.currentSelection = std::make_shared<Selection::SelectionSet>();
                
                // Initialize metadata
                project.metadata.created = std::chrono::system_clock::now();
                project.metadata.modified = project.metadata.created;
                project.metadata.version = FileIO::FileVersion::Current();
                project.metadata.applicationVersion = "1.0.0";
                
                // Populate project with current application state
                // Copy voxel data
                for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
                    auto resolution = static_cast<VoxelData::VoxelResolution>(i);
                    auto voxels = m_voxelManager->getAllVoxels(resolution);
                    for (const auto& voxelPos : voxels) {
                        project.voxelData->setVoxel(voxelPos.incrementPos.value(), voxelPos.resolution, true);
                    }
                }
                
                // Set workspace size
                project.workspace.size = m_voxelManager->getWorkspaceSize();
                project.workspace.defaultResolution = m_voxelManager->getActiveResolution();
                
                // Copy camera state
                if (m_cameraController && m_cameraController->getCamera()) {
                    auto appCamera = m_cameraController->getCamera();
                    project.camera->setPosition(appCamera->getPosition());
                    project.camera->setTarget(appCamera->getTarget());
                    project.camera->setDistance(appCamera->getDistance());
                }
                
                // Set metadata
                project.setName("Voxel Editor Project");
                project.setAuthor(std::getenv("USER") ? std::getenv("USER") : "Unknown");
                
                FileIO::SaveOptions options;
                auto result = m_fileManager->saveProject(filename, project, options);
                if (result.success) {
                    m_currentProject = filename;
                    m_app->setCurrentProject(filename);
                    return CommandResult::Success("Project saved: " + filename);
                }
                return CommandResult::Error("Failed to save project");
            }),
            
        // SAVE_AS command
        CommandRegistration()
            .withName(Commands::SAVE_AS)
            .withDescription("Save the project with a new name")
            .withCategory(CommandCategory::FILE)
            .withArg("filename", "Path to save file", "string", true)
            .withHandler([this](const CommandContext& ctx) {
                std::string filename = ctx.getArg(0);
                if (filename.empty()) {
                    return CommandResult::Error("Filename required");
                }
                
                // Validate filename
                if (filename == "/dev/null") {
                    return CommandResult::Error("Invalid filename: /dev/null");
                }
                
                // Check file extension
                const std::string validExtension = ".vxl";
                if (filename.length() < validExtension.length() || 
                    filename.substr(filename.length() - validExtension.length()) != validExtension) {
                    return CommandResult::Error("Invalid file extension. File must end with .vxl");
                }
                
                // Create project from current state
                FileIO::Project project;
                // Don't call initializeDefaults() - it creates a dangling EventDispatcher
                // Instead, manually initialize the shared_ptrs with proper objects
                project.voxelData = std::make_shared<VoxelData::VoxelDataManager>(m_eventDispatcher);
                project.groupData = std::make_shared<Groups::GroupManager>(project.voxelData.get(), m_eventDispatcher);
                project.camera = std::make_shared<Camera::OrbitCamera>();
                project.currentSelection = std::make_shared<Selection::SelectionSet>();
                
                // Initialize metadata
                project.metadata.created = std::chrono::system_clock::now();
                project.metadata.modified = project.metadata.created;
                project.metadata.version = FileIO::FileVersion::Current();
                project.metadata.applicationVersion = "1.0.0";
                
                // Populate project with current application state
                // Copy voxel data
                for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
                    auto resolution = static_cast<VoxelData::VoxelResolution>(i);
                    auto voxels = m_voxelManager->getAllVoxels(resolution);
                    for (const auto& voxelPos : voxels) {
                        project.voxelData->setVoxel(voxelPos.incrementPos.value(), voxelPos.resolution, true);
                    }
                }
                
                // Set workspace size
                project.workspace.size = m_voxelManager->getWorkspaceSize();
                project.workspace.defaultResolution = m_voxelManager->getActiveResolution();
                
                // Copy camera state
                if (m_cameraController && m_cameraController->getCamera()) {
                    auto appCamera = m_cameraController->getCamera();
                    project.camera->setPosition(appCamera->getPosition());
                    project.camera->setTarget(appCamera->getTarget());
                    project.camera->setDistance(appCamera->getDistance());
                }
                
                // Set metadata
                project.setName("Voxel Editor Project");
                project.setAuthor(std::getenv("USER") ? std::getenv("USER") : "Unknown");
                
                FileIO::SaveOptions options;
                auto result = m_fileManager->saveProject(filename, project, options);
                if (result.success) {
                    m_currentProject = filename;
                    m_app->setCurrentProject(filename);
                    return CommandResult::Success("Project saved as: " + filename);
                }
                return CommandResult::Error("Failed to save project");
            }),
            
        // EXPORT command
        CommandRegistration()
            .withName(Commands::EXPORT)
            .withDescription("Export to STL format")
            .withCategory(CommandCategory::FILE)
            .withArg("filename", "Path to STL file", "string", true)
            .withHandler([this](const CommandContext& ctx) {
                std::string filename = ctx.getArg(0);
                if (filename.empty()) {
                    return CommandResult::Error("Filename required");
                }
                
                // Generate surface mesh
                SurfaceGen::SurfaceGenerator surfaceGen(m_eventDispatcher);
                auto surfaceMesh = surfaceGen.generateMultiResMesh(*m_voxelManager, m_voxelManager->getActiveResolution());
                
                // Apply smoothing if enabled
                if (m_app->getSmoothingLevel() > 0) {
                    SurfaceGen::MeshSmoother smoother;
                    SurfaceGen::MeshSmoother::SmoothingConfig config;
                    config.smoothingLevel = m_app->getSmoothingLevel();
                    config.algorithm = m_app->getSmoothingAlgorithm();
                    config.preserveTopology = true;
                    config.preserveBoundaries = true;
                    config.minFeatureSize = 1.0f; // 1mm minimum feature size for 3D printing
                    config.usePreviewQuality = false; // Full quality for export
                    
                    // Apply smoothing with progress callback
                    std::cout << "Applying smoothing (level " << m_app->getSmoothingLevel() << ")..." << std::flush;
                    surfaceMesh = smoother.smooth(surfaceMesh, config, 
                        [](float progress) {
                            // Update progress display
                            std::cout << "\rApplying smoothing... " 
                                      << std::fixed << std::setprecision(0)
                                      << (progress * 100.0f) << "%" << std::flush;
                            return true; // Continue processing
                        });
                    std::cout << "\rApplying smoothing... Done!    " << std::endl;
                    
                    if (surfaceMesh.vertices.empty()) {
                        return CommandResult::Error("Smoothing operation failed or was cancelled");
                    }
                }
                
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
                options.validateWatertight = false; // Disable validation for now
                
                auto result = m_fileManager->exportSTL(filename, renderMesh, options);
                if (result.success) {
                    return CommandResult::Success("Exported to: " + filename);
                }
                return CommandResult::Error("Failed to export STL");
            })
    };
}

// Register this module automatically
REGISTER_COMMAND_MODULE(FileCommands)

} // namespace VoxelEditor