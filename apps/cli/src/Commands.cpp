// Standard library includes first
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include <algorithm>
#include <thread>
#include <cstdlib>
#include <chrono>
#include <ctime>



// Application headers
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/RenderWindow.h"
#include "cli/BuildInfo.h"
#include "cli/MouseInteraction.h"

// Core includes
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "camera/OrbitCamera.h"
#include "rendering/RenderEngine.h"
#include "file_io/FileManager.h"
#include "file_io/STLExporter.h"
#include "file_io/Project.h"
#include "groups/GroupManager.h"
#include "groups/GroupTypes.h"
#include "selection/SelectionManager.h"
#include "selection/SelectionTypes.h"
#include "surface_gen/SurfaceGenerator.h"
#include "surface_gen/MeshSmoother.h"
#include "surface_gen/MeshValidator.h"
#include "undo_redo/HistoryManager.h"
#include "undo_redo/VoxelCommands.h"
#include "undo_redo/PlacementCommands.h"
#include "visual_feedback/FeedbackRenderer.h"
#include "math/BoundingBox.h"
#include "math/CoordinateTypes.h"

namespace VoxelEditor {

// Forward declaration for simple validation command
CommandResult executeSimpleValidateCommand(const CommandContext& ctx);

// DEPRECATED: This function is replaced by the new CommandRegistry system
/*
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
            FileIO::LoadOptions options;
            auto result = m_fileManager->loadProject(filename, project, options);
            if (result.success) {
                // Clear current voxel data
                for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
                    auto resolution = static_cast<VoxelData::VoxelResolution>(i);
                    auto voxels = m_voxelManager->getAllVoxels(resolution);
                    for (const auto& voxelPos : voxels) {
                        m_voxelManager->setVoxel(voxelPos.incrementPos.value(), voxelPos.resolution, false);
                    }
                }
                
                // Copy loaded voxel data to app's voxel manager
                for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
                    auto resolution = static_cast<VoxelData::VoxelResolution>(i);
                    auto voxels = project.voxelData->getAllVoxels(resolution);
                    for (const auto& voxelPos : voxels) {
                        m_voxelManager->setVoxel(voxelPos.incrementPos.value(), voxelPos.resolution, true);
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
            project.initializeDefaults();
            
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
            project.initializeDefaults();
            
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
            
            // Apply smoothing if enabled
            if (m_smoothingLevel > 0) {
                SurfaceGen::MeshSmoother smoother;
                SurfaceGen::MeshSmoother::SmoothingConfig config;
                config.smoothingLevel = m_smoothingLevel;
                config.algorithm = m_smoothingAlgorithm;
                config.preserveTopology = true;
                config.preserveBoundaries = true;
                config.minFeatureSize = 1.0f; // 1mm minimum feature size for 3D printing
                config.usePreviewQuality = false; // Full quality for export
                
                // Apply smoothing with progress callback
                std::cout << "Applying smoothing (level " << m_smoothingLevel << ")..." << std::flush;
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
        }
    });
    
    // Edit Operations
    m_commandProcessor->registerCommand({
        Commands::PLACE,
        "Place a voxel at position (coordinates must include units: cm or m)",
        CommandCategory::EDIT,
        {"add", "set"},
        {
            {"x", "X coordinate with units (e.g. 100cm or 1m)", "coordinate", true, ""},
            {"y", "Y coordinate with units (e.g. 50cm or 0.5m)", "coordinate", true, ""},
            {"z", "Z coordinate with units (e.g. -100cm or -1m)", "coordinate", true, ""}
        },
        [this](const CommandContext& ctx) {
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
                m_voxelManager.get(),
                Math::IncrementCoordinates(Math::Vector3i(x, y, z)),
                m_voxelManager->getActiveResolution()
            );
            
            if (!cmd) {
                // Get validation details for error message
                auto validation = UndoRedo::PlacementCommandFactory::validatePlacement(
                    m_voxelManager.get(),
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
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::DELETE,
        "Delete a voxel at position (coordinates must include units: cm or m)",
        CommandCategory::EDIT,
        {"remove", "del"},
        {
            {"x", "X coordinate with units (e.g. 100cm or 1m)", "coordinate", true, ""},
            {"y", "Y coordinate with units (e.g. 50cm or 0.5m)", "coordinate", true, ""},
            {"z", "Z coordinate with units (e.g. -100cm or -1m)", "coordinate", true, ""}
        },
        [this](const CommandContext& ctx) {
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
                m_voxelManager.get(),
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
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::FILL,
        "Fill a box region with voxels (coordinates must include units: cm or m)",
        CommandCategory::EDIT,
        {},
        {
            {"x1", "Start X with units (e.g. 0cm or 0m)", "coordinate", true, ""},
            {"y1", "Start Y with units (e.g. 0cm or 0m)", "coordinate", true, ""},
            {"z1", "Start Z with units (e.g. -100cm or -1m)", "coordinate", true, ""},
            {"x2", "End X with units (e.g. 200cm or 2m)", "coordinate", true, ""},
            {"y2", "End Y with units (e.g. 100cm or 1m)", "coordinate", true, ""},
            {"z2", "End Z with units (e.g. 100cm or 1m)", "coordinate", true, ""}
        },
        [this](const CommandContext& ctx) {
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
                m_voxelManager.get(),
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
                // Update the voxel mesh after undo
                requestMeshUpdate();
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
                // Update the voxel mesh after redo
                requestMeshUpdate();
                return CommandResult::Success("Redone");
            }
            return CommandResult::Error("Nothing to redo");
        }
    });
    
    // View Controls
    m_commandProcessor->registerCommand({
        "center",
        "Center camera on origin or voxels",
        CommandCategory::VIEW,
        {"focus", "home"},
        {{"target", "Center target (origin/voxels/x,y,z)", "string", false, "voxels"}},
        [this](const CommandContext& ctx) {
            std::string target = ctx.getArgCount() > 0 ? ctx.getArg(0) : "voxels";
            
            Math::WorldCoordinates focusPoint;
            
            if (target == "origin") {
                // Focus on world origin
                focusPoint = Math::WorldCoordinates(0.0f, 0.0f, 0.0f);
            } else if (target == "voxels") {
                // Calculate center of all voxels
                Math::BoundingBox bounds;
                bool hasVoxels = false;
                
                auto* grid = m_voxelManager->getGrid(m_voxelManager->getActiveResolution());
                if (grid) {
                    float voxelSize = VoxelData::getVoxelSize(m_voxelManager->getActiveResolution());
                    
                    // Use VoxelDataManager's getAllVoxels() for proper coordinate handling
                    auto allVoxels = m_voxelManager->getAllVoxels();
                    for (const auto& voxel : allVoxels) {
                        // Use grid->gridToWorld() for proper coordinate conversion
                        Math::Vector3f voxelCenter = grid->incrementToWorld(voxel.incrementPos).value();
                        // Add half voxel size to get center
                        voxelCenter += Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
                        
                        if (!hasVoxels) {
                            bounds.min = bounds.max = voxelCenter;
                            hasVoxels = true;
                        } else {
                            bounds.expand(voxelCenter);
                        }
                    }
                }
                
                if (hasVoxels) {
                    focusPoint = Math::WorldCoordinates(bounds.getCenter());
                } else {
                    // No voxels, focus on workspace center (origin in centered coordinate system)
                    focusPoint = Math::WorldCoordinates(0.0f, 0.0f, 0.0f);
                }
            } else {
                // Try to parse as coordinates
                std::vector<float> coords;
                std::stringstream ss(target);
                std::string coord;
                while (std::getline(ss, coord, ',')) {
                    try {
                        coords.push_back(std::stof(coord));
                    } catch (...) {
                        return CommandResult::Error("Invalid coordinates: " + target);
                    }
                }
                
                if (coords.size() != 3) {
                    return CommandResult::Error("Expected 3 coordinates (x,y,z) or 'origin' or 'voxels'");
                }
                
                focusPoint = Math::WorldCoordinates(coords[0], coords[1], coords[2]);
            }
            
            // Set camera target to focus point
            auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(m_cameraController->getCamera());
            if (orbitCamera) {
                orbitCamera->setTarget(focusPoint);
                orbitCamera->focusOn(focusPoint);
                
                return CommandResult::Success("Camera centered on " + target);
            }
            
            return CommandResult::Error("Failed to center camera");
        }
    });
    
    m_commandProcessor->registerCommand({
        "camera-info",
        "Show current camera information",
        CommandCategory::VIEW,
        {"cam-info", "ci"},
        {},
        [this](const CommandContext& ctx) {
            auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(m_cameraController->getCamera());
            if (orbitCamera) {
                auto pos = orbitCamera->getPosition();
                auto target = orbitCamera->getTarget();
                float distance = orbitCamera->getDistance();
                float yaw = orbitCamera->getYaw();
                float pitch = orbitCamera->getPitch();
                
                std::stringstream ss;
                ss << "Camera Info:\n";
                ss << "  Position: (" << std::fixed << std::setprecision(2) 
                   << pos.x() << ", " << pos.y() << ", " << pos.z() << ")\n";
                ss << "  Target: (" << target.x() << ", " << target.y() << ", " << target.z() << ")\n";
                ss << "  Distance: " << distance << "\n";
                ss << "  Yaw: " << yaw << "°\n";
                ss << "  Pitch: " << pitch << "°\n";
                
                return CommandResult::Success(ss.str());
            }
            return CommandResult::Error("Camera info not available");
        }
    });
    
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
            
            // After setting preset, ensure camera stays at reasonable distance
            if (viewPreset == Camera::ViewPreset::ISOMETRIC) {
                m_cameraController->getCamera()->setDistance(3.0f);
            }
            
            // Force camera matrix update by accessing them
            // This ensures the lazy evaluation happens immediately
            m_cameraController->getCamera()->getViewMatrix();
            m_cameraController->getCamera()->getProjectionMatrix();
            
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
            
            // Force camera matrix update
            m_cameraController->getCamera()->getViewMatrix();
            
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
            
            // Force camera matrix update
            m_cameraController->getCamera()->getViewMatrix();
            
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
            
            // Force camera matrix update
            m_cameraController->getCamera()->getViewMatrix();
            m_cameraController->getCamera()->getProjectionMatrix();
            
            return CommandResult::Success("Camera reset to default view");
        }
    });
    
    m_commandProcessor->registerCommand({
        Commands::GRID,
        "Toggle ground plane grid visibility",
        CommandCategory::VIEW,
        {"groundplane"},
        {{"state", "on/off/toggle (optional)", "string", false, "toggle"}},
        [this](const CommandContext& ctx) {
            // Check if we're in headless mode
            if (!m_renderEngine) {
                return CommandResult::Error("Grid command not available in headless mode");
            }
            
            std::string state = ctx.getArg(0, "toggle");
            
            bool currentState = m_renderEngine->isGroundPlaneGridVisible();
            bool newState;
            
            if (state == "on") {
                newState = true;
            } else if (state == "off") {
                newState = false;
            } else if (state == "toggle") {
                newState = !currentState;
            } else {
                return CommandResult::Error("Invalid state. Use 'on', 'off', or 'toggle'");
            }
            
            m_renderEngine->setGroundPlaneGridVisible(newState);
            
            // Update the ground plane grid with workspace size if turning on
            if (newState) {
                m_renderEngine->updateGroundPlaneGrid(m_voxelManager->getWorkspaceSize());
            }
            
            return CommandResult::Success("Ground plane grid " + std::string(newState ? "enabled" : "disabled"));
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
            const auto& args = ctx.getArgs();
            
            // Parse each dimension, allowing optional "m" suffix
            std::vector<float> dimensions;
            std::vector<bool> hasUnitsList;
            
            for (size_t i = 0; i < 3 && i < args.size(); ++i) {
                std::string arg = args[i];
                
                // Check if argument ends with 'm' and remove it
                bool hasMeters = false;
                if (!arg.empty() && arg.back() == 'm') {
                    hasMeters = true;
                    arg = arg.substr(0, arg.length() - 1);
                }
                hasUnitsList.push_back(hasMeters);
                
                // Validate the numeric part
                bool isValid = true;
                bool hasDecimal = false;
                for (size_t j = 0; j < arg.length(); ++j) {
                    char c = arg[j];
                    if (c == '-' && j == 0) continue; // Allow negative sign at start
                    if (c == '.') {
                        if (hasDecimal) {
                            isValid = false; // Multiple decimals
                            break;
                        }
                        hasDecimal = true;
                        continue;
                    }
                    if (!std::isdigit(c)) {
                        isValid = false;
                        break;
                    }
                }
                
                if (!isValid || arg.empty() || arg == "-" || arg == ".") {
                    return CommandResult::Error("Invalid workspace dimension: '" + args[i] + "'. Expected numeric values in meters (e.g., 5, 5.5, 5m, or 5.5m)");
                }
                
                // Parse the float value
                try {
                    size_t idx = 0;
                    float value = std::stof(arg, &idx);
                    // Check that the entire string was consumed
                    if (idx != arg.length()) {
                        return CommandResult::Error("Invalid numeric value: '" + args[i] + "'");
                    }
                    dimensions.push_back(value);
                } catch (...) {
                    return CommandResult::Error("Invalid numeric value: '" + args[i] + "'");
                }
            }
            
            if (dimensions.size() != 3) {
                return CommandResult::Error("Workspace requires 3 dimensions");
            }
            
            // Check for consistent unit usage - all dimensions must either have 'm' or none
            bool firstHasUnit = hasUnitsList[0];
            for (size_t i = 1; i < hasUnitsList.size(); ++i) {
                if (hasUnitsList[i] != firstHasUnit) {
                    return CommandResult::Error("Mixed units detected. All dimensions must either have 'm' suffix or none");
                }
            }
            
            float width = dimensions[0];
            float height = dimensions[1];
            float depth = dimensions[2];
            
            // Validate dimensions
            if (width <= 0 || height <= 0 || depth <= 0) {
                return CommandResult::Error("Workspace dimensions must be positive values");
            }
            
            if (width < 2 || height < 2 || depth < 2) {
                return CommandResult::Error("Workspace dimensions must be at least 2 meters");
            }
            
            if (width > 8 || height > 8 || depth > 8) {
                return CommandResult::Error("Workspace dimensions cannot exceed 8 meters");
            }
            
            Math::Vector3f size(width, height, depth);
            if (m_voxelManager->resizeWorkspace(size)) {
                return CommandResult::Success("Workspace resized to " + 
                    std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(depth) + " meters");
            }
            return CommandResult::Error("Failed to resize workspace");
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
        "Select voxels in box region (coordinates must include units: cm or m)",
        CommandCategory::SELECT,
        {"selbox"},
        {
            {"x1", "Start X with units (e.g. -100cm or -1m)", "coordinate", true, ""},
            {"y1", "Start Y with units (e.g. 0cm or 0m)", "coordinate", true, ""},
            {"z1", "Start Z with units (e.g. -100cm or -1m)", "coordinate", true, ""},
            {"x2", "End X with units (e.g. 100cm or 1m)", "coordinate", true, ""},
            {"y2", "End Y with units (e.g. 200cm or 2m)", "coordinate", true, ""},
            {"z2", "End Z with units (e.g. 100cm or 1m)", "coordinate", true, ""}
        },
        [this](const CommandContext& ctx) {
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
    
    // Screenshot command
    m_commandProcessor->registerCommand({
        "screenshot",
        "Take a screenshot of the current view",
        CommandCategory::VIEW,
        {"ss", "capture"},
        {{"filename", "Output filename (.png)", "string", true, ""}},
        [this](const CommandContext& ctx) {
            // Check if we're in headless mode
            if (!m_renderWindow || !m_renderEngine) {
                return CommandResult::Error("Screenshot command not available in headless mode");
            }
            
            std::string filename = ctx.getArg(0);
            if (filename.empty()) {
                return CommandResult::Error("Filename required");
            }
            
            // Add .png extension if not present
            if (filename.find(".png") == std::string::npos) {
                filename += ".png";
            }
            
            // Render the scene before taking screenshot
            render();
            
            // Don't swap before screenshot - read from back buffer
            // m_renderWindow->swapBuffers();
            
            if (m_renderWindow->saveScreenshot(filename)) {
                return CommandResult::Success("Screenshot saved: " + filename);
            }
            return CommandResult::Error("Failed to save screenshot");
        }
    });
    
    // Sleep command for testing
    m_commandProcessor->registerCommand({
        "sleep",
        "Pause execution for specified seconds",
        CommandCategory::SYSTEM,
        {"wait", "pause"},
        {{"seconds", "Number of seconds to sleep", "float", true, ""}},
        [this](const CommandContext& ctx) {
            float seconds = ctx.getFloatArg(0, 1.0f);
            if (seconds < 0 || seconds > 10) {
                return CommandResult::Error("Sleep time must be between 0 and 10 seconds");
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(seconds * 1000)));
            return CommandResult::Success("Slept for " + std::to_string(seconds) + " seconds");
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
            
            // Smoothing settings
            ss << "\nSmoothing Settings:\n";
            ss << "  Level: " << m_smoothingLevel;
            if (m_smoothingLevel > 0) {
                std::string algoName;
                switch (m_smoothingAlgorithm) {
                    case SurfaceGen::MeshSmoother::Algorithm::Laplacian:
                        algoName = "Laplacian";
                        break;
                    case SurfaceGen::MeshSmoother::Algorithm::Taubin:
                        algoName = "Taubin";
                        break;
                    case SurfaceGen::MeshSmoother::Algorithm::BiLaplacian:
                        algoName = "BiLaplacian";
                        break;
                    default:
                        algoName = "None";
                }
                ss << " (" << algoName << ")";
            }
            ss << "\n";
            ss << "  Preview: " << (m_smoothPreviewEnabled ? "on" : "off") << "\n";
            
            return CommandResult::Success(ss.str());
        }
    });
    
    // Shader command
    m_commandProcessor->registerCommand({
        "shader",
        "Switch between shader modes or list available shaders",
        CommandCategory::VIEW,
        {},
        {{"mode", "Shader mode: basic, enhanced, flat, or 'list' to show all", "string", false, "list"}},
        [this](const CommandContext& ctx) {
            // Check if we're in headless mode
            if (!m_renderEngine) {
                return CommandResult::Error("Shader command not available in headless mode");
            }
            
            std::string mode = ctx.getArgCount() > 0 ? ctx.getArg(0) : "list";
            
            if (mode == "list") {
                // List all available shaders
                std::stringstream ss;
                ss << "Available shaders:\n";
                ss << "  basic    - Standard Phong lighting (ambient + diffuse + specular)\n";
                ss << "  enhanced - Multiple lights with face-dependent brightness (default)\n";
                ss << "  flat     - Simple flat shading with maximum face distinction\n";
                
                // Show current shader
                std::string currentShader = "unknown";
                if (m_defaultShaderId != Rendering::InvalidId) {
                    // Check which shader is currently active
                    if (m_defaultShaderId == m_renderEngine->getBuiltinShader("basic")) {
                        currentShader = "basic";
                    } else if (m_defaultShaderId == m_renderEngine->getBuiltinShader("enhanced")) {
                        currentShader = "enhanced";
                    } else if (m_defaultShaderId == m_renderEngine->getBuiltinShader("flat")) {
                        currentShader = "flat";
                    }
                }
                ss << "\nCurrent shader: " << currentShader;
                
                return CommandResult::Success(ss.str());
            }
            
            std::string shaderName;
            if (mode == "basic") {
                shaderName = "basic";
            } else if (mode == "enhanced") {
                shaderName = "enhanced";
            } else if (mode == "flat") {
                shaderName = "flat";
            } else {
                return CommandResult::Error("Unknown shader mode. Use: basic, enhanced, flat, or list");
            }
            
            // Get the shader ID from render engine
            auto shaderId = m_renderEngine->getBuiltinShader(shaderName);
            if (shaderId == Rendering::InvalidId) {
                return CommandResult::Error("Shader '" + shaderName + "' not found");
            }
            
            // Set as default shader
            m_defaultShaderId = shaderId;
            
            // Update the voxel mesh to trigger re-render with new shader
            requestMeshUpdate();
            
            return CommandResult::Success("Shader mode set to: " + mode);
        }
    });
    
    // Edge rendering toggle
    m_commandProcessor->registerCommand({
        "edges",
        "Toggle edge/wireframe overlay rendering",
        CommandCategory::VIEW,
        {},
        {{"state", "on/off to enable/disable edges, or 'toggle' to switch", "string", false, "toggle"}},
        [this](const CommandContext& ctx) {
            std::string state = ctx.getArgCount() > 0 ? ctx.getArg(0) : "toggle";
            
            if (state == "toggle") {
                m_showEdges = !m_showEdges;
            } else if (state == "on") {
                m_showEdges = true;
            } else if (state == "off") {
                m_showEdges = false;
            } else {
                return CommandResult::Error("Invalid state. Use: on, off, or toggle");
            }
            
            // Trigger re-render
            requestMeshUpdate();
            
            return CommandResult::Success("Edge rendering " + std::string(m_showEdges ? "enabled" : "disabled"));
        }
    });
    
    // Debug camera command
    m_commandProcessor->registerCommand({
        "debug",
        "Debug commands for troubleshooting",
        CommandCategory::SYSTEM,
        {},
        {
            {"subcommand", "What to debug: camera, voxels, render, frustum, ray, grid", "string", true, ""}
        },
        [this](const CommandContext& ctx) {
            std::string subcommand = ctx.getArg(0);
            
            if (subcommand == "camera") {
                std::stringstream ss;
                ss << "Camera Debug Info\n";
                ss << "================\n";
                
                auto camera = m_cameraController->getCamera();
                auto pos = camera->getPosition();
                auto target = camera->getTarget();
                auto up = camera->getUp();
                
                ss << "Position: (" << pos.x() << ", " << pos.y() << ", " << pos.z() << ")\n";
                ss << "Target: (" << target.x() << ", " << target.y() << ", " << target.z() << ")\n";
                ss << "Up: (" << up.x() << ", " << up.y() << ", " << up.z() << ")\n";
                
                // Try to cast to OrbitCamera to get distance
                if (auto orbitCamera = dynamic_cast<const Camera::OrbitCamera*>(camera)) {
                    ss << "Distance: " << orbitCamera->getDistance() << "\n";
                }
                
                ss << "FOV: " << camera->getFieldOfView() << " degrees\n";
                ss << "Near/Far: " << camera->getNearPlane() << " / " << camera->getFarPlane() << "\n";
                
                // View matrix
                auto viewMatrix = camera->getViewMatrix();
                ss << "\nView Matrix:\n";
                for (int i = 0; i < 4; i++) {
                    ss << "  ";
                    for (int j = 0; j < 4; j++) {
                        ss << std::fixed << std::setprecision(3) << viewMatrix.m[i*4+j] << " ";
                    }
                    ss << "\n";
                }
                
                // Projection matrix
                auto projMatrix = camera->getProjectionMatrix();
                ss << "\nProjection Matrix:\n";
                for (int i = 0; i < 4; i++) {
                    ss << "  ";
                    for (int j = 0; j < 4; j++) {
                        ss << std::fixed << std::setprecision(3) << projMatrix.m[i*4+j] << " ";
                    }
                    ss << "\n";
                }
                
                return CommandResult::Success(ss.str());
            }
            else if (subcommand == "voxels") {
                std::stringstream ss;
                ss << "Voxel Debug Info\n";
                ss << "===============\n";
                
                size_t voxelCount = m_voxelManager->getVoxelCount();
                ss << "Total voxels: " << voxelCount << "\n";
                
                auto resolution = m_voxelManager->getActiveResolution();
                float voxelSize = VoxelData::getVoxelSize(resolution);
                ss << "Resolution: " << VoxelData::getVoxelSizeName(resolution) 
                   << " (" << voxelSize << "m)\n";
                
                auto wsSize = m_voxelManager->getWorkspaceSize();
                ss << "Workspace size: " << wsSize.x << " x " << wsSize.y << " x " << wsSize.z << " meters\n";
                
                // List first 10 voxels
                if (voxelCount > 0) {
                    auto allVoxels = m_voxelManager->getAllVoxels();
                    size_t displayCount = std::min(size_t(10), allVoxels.size());
                    ss << "\nFirst " << displayCount << " voxels:\n";
                    
                    for (size_t i = 0; i < displayCount; i++) {
                        const auto& voxelPos = allVoxels[i];
                        
                        // Use grid->gridToWorld() for proper coordinate conversion
                        const VoxelData::VoxelGrid* grid = m_voxelManager->getGrid(voxelPos.resolution);
                        Math::WorldCoordinates worldPos = grid ? grid->incrementToWorld(voxelPos.incrementPos) : Math::WorldCoordinates::zero();
                        
                        ss << "  [" << i << "] Grid(" << voxelPos.incrementPos.x() << "," << voxelPos.incrementPos.y() 
                           << "," << voxelPos.incrementPos.z() << ") -> World(" 
                           << worldPos.x() << "," << worldPos.y() << "," << worldPos.z() << ")\n";
                    }
                }
                
                return CommandResult::Success(ss.str());
            }
            else if (subcommand == "render") {
                std::stringstream ss;
                ss << "Render Debug Info\n";
                ss << "================\n";
                
                if (!m_renderEngine) {
                    return CommandResult::Error("Render engine not initialized");
                }
                
                auto stats = m_renderEngine->getRenderStats();
                ss << "FPS: " << stats.fps << "\n";
                ss << "Frame time: " << stats.frameTime << " ms\n";
                ss << "Draw calls: " << stats.drawCalls << "\n";
                ss << "Triangles: " << stats.trianglesRendered << "\n";
                ss << "Vertices: " << stats.verticesProcessed << "\n";
                
                // RenderEngine handles OpenGL error checking internally
                ss << "\nRender engine status: " << (m_renderEngine->isInitialized() ? "Initialized" : "Not initialized") << "\n";
                
                // Check if we have meshes
                ss << "\nVoxel meshes: " << m_voxelMeshes.size() << "\n";
                for (size_t i = 0; i < m_voxelMeshes.size(); i++) {
                    ss << "  Mesh " << i << ": " << m_voxelMeshes[i].vertices.size() 
                       << " vertices, " << m_voxelMeshes[i].indices.size() << " indices\n";
                }
                
                return CommandResult::Success(ss.str());
            }
            else if (subcommand == "frustum") {
                std::stringstream ss;
                ss << "Frustum Debug Info\n";
                ss << "==================\n";
                
                auto camera = m_cameraController->getCamera();
                auto viewProj = camera->getViewProjectionMatrix();
                
                // Test if workspace center is visible (origin in centered coordinate system)
                Math::Vector3f center(0.0f, 0.0f, 0.0f);
                Math::Vector4f centerClip = viewProj * Math::Vector4f(center.x, center.y, center.z, 1.0f);
                
                ss << "Workspace center (" << center.x << "," << center.y << "," << center.z << ")\n";
                ss << "  Clip space: (" << centerClip.x << "," << centerClip.y << "," 
                   << centerClip.z << "," << centerClip.w << ")\n";
                
                if (centerClip.w != 0) {
                    float ndcX = centerClip.x / centerClip.w;
                    float ndcY = centerClip.y / centerClip.w;
                    float ndcZ = centerClip.z / centerClip.w;
                    ss << "  NDC: (" << ndcX << "," << ndcY << "," << ndcZ << ")\n";
                    
                    bool visible = (ndcX >= -1 && ndcX <= 1 && 
                                   ndcY >= -1 && ndcY <= 1 && 
                                   ndcZ >= -1 && ndcZ <= 1);
                    ss << "  Visible: " << (visible ? "YES" : "NO") << "\n";
                }
                
                // Test first voxel if any
                if (m_voxelManager->getVoxelCount() > 0) {
                    auto allVoxels = m_voxelManager->getAllVoxels();
                    if (!allVoxels.empty()) {
                        const auto& voxelPos = allVoxels[0];
                        float voxelSize = VoxelData::getVoxelSize(voxelPos.resolution);
                        
                        // Use grid->gridToWorld() for proper coordinate conversion
                        const VoxelData::VoxelGrid* grid = m_voxelManager->getGrid(voxelPos.resolution);
                        Math::WorldCoordinates worldPos = grid ? grid->incrementToWorld(voxelPos.incrementPos) : Math::WorldCoordinates::zero();
                        Math::Vector4f clipPos = viewProj * Math::Vector4f(worldPos.x(), worldPos.y(), worldPos.z(), 1.0f);
                        
                        ss << "\nFirst voxel at grid(" << voxelPos.incrementPos.x() << "," << voxelPos.incrementPos.y() 
                           << "," << voxelPos.incrementPos.z() << ")\n";
                        ss << "  World: (" << worldPos.x() << "," << worldPos.y() << "," << worldPos.z() << ")\n";
                        ss << "  Clip: (" << clipPos.x << "," << clipPos.y << "," << clipPos.z << "," << clipPos.w << ")\n";
                        
                        if (clipPos.w != 0) {
                            float ndcX = clipPos.x / clipPos.w;
                            float ndcY = clipPos.y / clipPos.w;
                            float ndcZ = clipPos.z / clipPos.w;
                            ss << "  NDC: (" << ndcX << "," << ndcY << "," << ndcZ << ")\n";
                            
                            bool visible = (ndcX >= -1 && ndcX <= 1 && 
                                           ndcY >= -1 && ndcY <= 1 && 
                                           ndcZ >= -1 && ndcZ <= 1);
                            ss << "  Visible: " << (visible ? "YES" : "NO") << "\n";
                        }
                    }
                }
                
                return CommandResult::Success(ss.str());
            }
            else if (subcommand == "triangle") {
                // Check if we're in headless mode
                if (!m_renderEngine) {
                    return CommandResult::Error("Triangle debug command not available in headless mode");
                }
                
                std::stringstream ss;
                ss << "Triangle Test Debug\n";
                ss << "==================\n";
                
                // Create a simple triangle mesh using the core rendering system
                Rendering::Mesh triangleMesh;
                
                // Add vertices
                triangleMesh.vertices.resize(3);
                triangleMesh.vertices[0].position = Math::WorldCoordinates(-0.5f, -0.5f, 0.0f);
                triangleMesh.vertices[0].color = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f); // Red
                triangleMesh.vertices[1].position = Math::WorldCoordinates(0.5f, -0.5f, 0.0f);
                triangleMesh.vertices[1].color = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f);
                triangleMesh.vertices[2].position = Math::WorldCoordinates(0.0f, 0.5f, 0.0f);
                triangleMesh.vertices[2].color = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f);
                
                // Add indices
                triangleMesh.indices = {0, 1, 2};
                
                // Clear and render
                m_renderEngine->beginFrame();
                m_renderEngine->clear(Rendering::ClearFlags::All, Rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));
                
                // Set identity transform and basic material
                Rendering::Transform transform;
                Rendering::Material material;
                material.albedo = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f); // Red
                material.shader = m_renderEngine->getBuiltinShader("basic");
                
                m_renderEngine->renderMesh(triangleMesh, transform, material);
                m_renderEngine->endFrame();
                m_renderEngine->present();
                
                ss << "Triangle rendered using core rendering system\n";
                
                // Save screenshot for verification
                std::string screenshotFile = "debug_triangle.ppm";
                m_renderWindow->saveScreenshot(screenshotFile);
                ss << "Screenshot saved to: " << screenshotFile << "\n";
                
                return CommandResult::Success(ss.str());
            }
            else if (subcommand == "ray") {
                // Toggle ray visualization
                if (m_mouseInteraction) {
                    bool currentState = m_mouseInteraction->isRayVisualizationEnabled();
                    m_mouseInteraction->setRayVisualizationEnabled(!currentState);
                    
                    std::stringstream ss;
                    ss << "Ray visualization " << (!currentState ? "enabled" : "disabled") << "\n";
                    ss << "Yellow rays will now be drawn from the camera through the mouse cursor\n";
                    ss << "to help debug ray-casting issues.\n";
                    
                    return CommandResult::Success(ss.str());
                } else {
                    return CommandResult::Error("Mouse interaction not available");
                }
            }
            else if (subcommand == "grid") {
                // Toggle debug grid overlay
                m_debugGridVisible = !m_debugGridVisible;
                
                std::stringstream ss;
                ss << "Debug grid overlay " << (m_debugGridVisible ? "enabled" : "disabled") << "\n";
                ss << "1cm increment grid will now be " << (m_debugGridVisible ? "visible" : "hidden") << "\n";
                ss << "to help verify placement accuracy.\n";
                
                return CommandResult::Success(ss.str());
            }
            else {
                return CommandResult::Error("Unknown debug subcommand. Use: camera, voxels, render, frustum, ray, or grid");
            }
        }
    });
    
    // Validate command
    m_commandProcessor->registerCommand({
        Commands::VALIDATE,
        "Validate the rendering pipeline and diagnose issues",
        CommandCategory::SYSTEM,
        {"check", "diag"},
        {},
        executeSimpleValidateCommand
    });
    
    // Build command
    m_commandProcessor->registerCommand({
        Commands::BUILD,
        "Show build information",
        CommandCategory::SYSTEM,
        {"buildinfo", "version"},
        {},
        [](const CommandContext& ctx) {
            std::stringstream ss;
            ss << "Voxel Editor Build Information\n";
            ss << "==============================\n";
            
            // Version
            ss << "Version: " << CLI::VERSION_STRING << "\n";
            
            // Build date and time
            ss << "Built: " << CLI::BUILD_DATE << " " << CLI::BUILD_TIME << "\n";
            
            // Calculate how long ago it was built
            auto now = std::chrono::system_clock::now();
            auto nowTime = std::chrono::system_clock::to_time_t(now);
            auto buildTime = static_cast<time_t>(CLI::BUILD_TIMESTAMP);
            auto diff = nowTime - buildTime;
            
            // Format time difference
            if (diff < 60) {
                ss << "Built " << diff << " seconds ago\n";
            } else if (diff < 3600) {
                ss << "Built " << (diff / 60) << " minutes ago\n";
            } else if (diff < 86400) {
                ss << "Built " << (diff / 3600) << " hours ago\n";
            } else {
                ss << "Built " << (diff / 86400) << " days ago\n";
            }
            
            // Git information
            if (std::string(CLI::GIT_COMMIT_HASH) != "unknown") {
                ss << "\nGit Information:\n";
                ss << "  Branch: " << CLI::GIT_BRANCH << "\n";
                ss << "  Commit: " << CLI::GIT_COMMIT_HASH << "\n";
            }
            
            // Build configuration
            ss << "\nBuild Configuration:\n";
            ss << "  Type: " << CLI::BUILD_TYPE << "\n";
            ss << "  Compiler: " << CLI::COMPILER_ID << " " << CLI::COMPILER_VERSION << "\n";
            
            return CommandResult::Success(ss.str());
        }
    });
    
    // Smooth command
    m_commandProcessor->registerCommand({
        Commands::SMOOTH,
        "Control mesh smoothing settings",
        CommandCategory::MESH,
        {},
        {{"level", "Smoothing level (0-10+) or 'preview' subcommand", "string", false, ""},
         {"on_off", "For 'preview': on/off", "string", false, ""},
         {"algorithm", "For 'algorithm': laplacian/taubin/bilaplacian", "string", false, ""}},
        [this](const CommandContext& ctx) {
            if (ctx.getArgCount() == 0) {
                // Display current smoothing settings
                std::stringstream ss;
                ss << "Current smoothing settings:\n";
                ss << "  Level: " << m_smoothingLevel << "\n";
                
                // Get algorithm name
                std::string algoName;
                switch (m_smoothingAlgorithm) {
                    case SurfaceGen::MeshSmoother::Algorithm::None:
                        algoName = "None (raw dual contouring)";
                        break;
                    case SurfaceGen::MeshSmoother::Algorithm::Laplacian:
                        algoName = "Laplacian (basic smoothing)";
                        break;
                    case SurfaceGen::MeshSmoother::Algorithm::Taubin:
                        algoName = "Taubin (feature-preserving)";
                        break;
                    case SurfaceGen::MeshSmoother::Algorithm::BiLaplacian:
                        algoName = "BiLaplacian (aggressive smoothing)";
                        break;
                }
                ss << "  Algorithm: " << algoName << "\n";
                ss << "  Preview: " << (m_smoothPreviewEnabled ? "on" : "off") << "\n";
                
                return CommandResult::Success(ss.str());
            }
            
            std::string arg1 = ctx.getArg(0);
            
            // Handle subcommands
            if (arg1 == "preview") {
                if (ctx.getArgCount() < 2) {
                    return CommandResult::Error("Usage: smooth preview on|off");
                }
                
                std::string onOff = ctx.getArg(1);
                if (onOff == "on") {
                    m_smoothPreviewEnabled = true;
                    // TODO: Trigger preview mesh generation when implemented
                    return CommandResult::Success("Smoothing preview enabled");
                } else if (onOff == "off") {
                    m_smoothPreviewEnabled = false;
                    return CommandResult::Success("Smoothing preview disabled");
                } else {
                    return CommandResult::Error("Invalid option. Use 'on' or 'off'");
                }
            } else if (arg1 == "algorithm") {
                if (ctx.getArgCount() < 2) {
                    return CommandResult::Error("Usage: smooth algorithm laplacian|taubin|bilaplacian");
                }
                
                std::string algoName = ctx.getArg(1);
                if (algoName == "laplacian") {
                    m_smoothingAlgorithm = SurfaceGen::MeshSmoother::Algorithm::Laplacian;
                    return CommandResult::Success("Smoothing algorithm set to Laplacian");
                } else if (algoName == "taubin") {
                    m_smoothingAlgorithm = SurfaceGen::MeshSmoother::Algorithm::Taubin;
                    return CommandResult::Success("Smoothing algorithm set to Taubin");
                } else if (algoName == "bilaplacian") {
                    m_smoothingAlgorithm = SurfaceGen::MeshSmoother::Algorithm::BiLaplacian;
                    return CommandResult::Success("Smoothing algorithm set to BiLaplacian");
                } else {
                    return CommandResult::Error("Invalid algorithm. Choose from: laplacian, taubin, bilaplacian");
                }
            } else {
                // Try to parse as smoothing level
                int level = ctx.getIntArg(0, -1);
                if (level < 0) {
                    return CommandResult::Error("Invalid smoothing level. Must be 0 or greater");
                }
                
                m_smoothingLevel = level;
                
                // Auto-select algorithm based on level
                m_smoothingAlgorithm = SurfaceGen::MeshSmoother::getAlgorithmForLevel(level);
                
                std::stringstream ss;
                ss << "Smoothing level set to " << level;
                if (level > 10) {
                    ss << " (maximum smoothing)";
                }
                
                return CommandResult::Success(ss.str());
            }
        }
    });
    
    // Mesh commands
    m_commandProcessor->registerCommand({
        Commands::MESH,
        "Mesh validation and information",
        CommandCategory::MESH,
        {},
        {{"subcommand", "validate|info|repair", "string", true, ""}},
        [this](const CommandContext& ctx) {
            std::string subcommand = ctx.getArg(0);
            
            if (subcommand == "validate") {
                // Generate mesh first
                SurfaceGen::SurfaceGenerator surfaceGen(m_eventDispatcher.get());
                auto surfaceMesh = surfaceGen.generateMultiResMesh(*m_voxelManager, m_voxelManager->getActiveResolution());
                
                // Apply smoothing if enabled
                if (m_smoothingLevel > 0) {
                    SurfaceGen::MeshSmoother smoother;
                    SurfaceGen::MeshSmoother::SmoothingConfig config;
                    config.smoothingLevel = m_smoothingLevel;
                    config.algorithm = m_smoothingAlgorithm;
                    config.preserveTopology = true;
                    config.preserveBoundaries = true;
                    config.minFeatureSize = 1.0f;
                    config.usePreviewQuality = false;
                    
                    surfaceMesh = smoother.smooth(surfaceMesh, config);
                }
                
                // Validate mesh
                SurfaceGen::MeshValidator validator;
                auto validation = validator.validate(surfaceMesh);
                
                std::stringstream ss;
                ss << "Mesh Validation Results:\n";
                ss << "  Watertight: " << (validation.isWatertight ? "Yes" : "No") << "\n";
                ss << "  Manifold: " << (validation.isManifold ? "Yes" : "No") << "\n";
                ss << "  Valid topology: " << (validation.isValid ? "Yes" : "No") << "\n";
                
                if (!validation.errors.empty()) {
                    ss << "\nErrors found:\n";
                    for (const auto& error : validation.errors) {
                        ss << "  - " << error << "\n";
                    }
                }
                
                if (!validation.warnings.empty()) {
                    ss << "\nWarnings:\n";
                    for (const auto& warning : validation.warnings) {
                        ss << "  - " << warning << "\n";
                    }
                }
                
                return CommandResult::Success(ss.str());
                
            } else if (subcommand == "info") {
                // Generate mesh first
                SurfaceGen::SurfaceGenerator surfaceGen(m_eventDispatcher.get());
                auto surfaceMesh = surfaceGen.generateMultiResMesh(*m_voxelManager, m_voxelManager->getActiveResolution());
                
                // Apply smoothing if enabled
                if (m_smoothingLevel > 0) {
                    SurfaceGen::MeshSmoother smoother;
                    SurfaceGen::MeshSmoother::SmoothingConfig config;
                    config.smoothingLevel = m_smoothingLevel;
                    config.algorithm = m_smoothingAlgorithm;
                    config.preserveTopology = true;
                    config.preserveBoundaries = true;
                    config.minFeatureSize = 1.0f;
                    config.usePreviewQuality = false;
                    
                    surfaceMesh = smoother.smooth(surfaceMesh, config);
                }
                
                std::stringstream ss;
                ss << "Mesh Information:\n";
                ss << "  Vertices: " << surfaceMesh.getVertexCount() << "\n";
                ss << "  Triangles: " << surfaceMesh.getTriangleCount() << "\n";
                ss << "  Memory usage: " << (surfaceMesh.getMemoryUsage() / 1024) << " KB\n";
                
                // Calculate bounds
                surfaceMesh.calculateBounds();
                auto bounds = surfaceMesh.bounds;
                ss << "\nBounding box:\n";
                ss << "  Min: (" << bounds.min.x << ", " << bounds.min.y << ", " << bounds.min.z << ")\n";
                ss << "  Max: (" << bounds.max.x << ", " << bounds.max.y << ", " << bounds.max.z << ")\n";
                ss << "  Size: (" << (bounds.max.x - bounds.min.x) << ", " 
                   << (bounds.max.y - bounds.min.y) << ", " 
                   << (bounds.max.z - bounds.min.z) << ")\n";
                
                if (m_smoothingLevel > 0) {
                    ss << "\nSmoothing applied:\n";
                    ss << "  Level: " << m_smoothingLevel << "\n";
                    ss << "  Algorithm: ";
                    switch (m_smoothingAlgorithm) {
                        case SurfaceGen::MeshSmoother::Algorithm::Laplacian:
                            ss << "Laplacian\n";
                            break;
                        case SurfaceGen::MeshSmoother::Algorithm::Taubin:
                            ss << "Taubin\n";
                            break;
                        case SurfaceGen::MeshSmoother::Algorithm::BiLaplacian:
                            ss << "BiLaplacian\n";
                            break;
                        default:
                            ss << "None\n";
                    }
                }
                
                return CommandResult::Success(ss.str());
                
            } else if (subcommand == "repair") {
                // Note: MeshBuilder repair functions need to be implemented
                return CommandResult::Success("Mesh repair functionality is pending implementation in MeshBuilder");
            } else {
                return CommandResult::Error("Invalid subcommand. Use: validate, info, or repair");
            }
        }
    });
}
*/

// New registerCommands implementation - delegates to CommandRegistry
void Application::registerCommands() {
    // This method is no longer used - commands are registered through CommandRegistry
    // The CommandRegistry system is initialized in Application::initialize()
}

} // namespace VoxelEditor