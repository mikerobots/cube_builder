#include "cli/SystemCommands.h"
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/BuildInfo.h"
#include "cli/RenderWindow.h"
#include "cli/MouseInteraction.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "voxel_data/VoxelGrid.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "camera/OrbitCamera.h"
#include "rendering/RenderEngine.h"
#include "rendering/RenderTypes.h"
#include "selection/SelectionManager.h"
#include "groups/GroupManager.h"
#include "undo_redo/HistoryManager.h"
#include "surface_gen/MeshSmoother.h"
#include "math/CoordinateTypes.h"
#include "math/Vector3f.h"
#include "math/Vector4f.h"
#include "math/Matrix4f.h"
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdlib>

namespace VoxelEditor {

// Forward declaration for simple validation command
CommandResult executeSimpleValidateCommand(const CommandContext& ctx);

std::vector<CommandRegistration> SystemCommands::getCommands() {
    return {
        // HELP command
        CommandRegistration()
            .withName(Commands::HELP)
            .withDescription("Show available commands")
            .withCategory(CommandCategory::HELP)
            .withArg("command", "Command to show help for (optional)", "string", false, "")
            .withHandler([this](const CommandContext& ctx) -> CommandResult {
                std::string command = ctx.getArg(0, "");
                
                if (command.empty()) {
                    // No command specified, show general help
                    return CommandResult::Success(m_app->getCommandProcessor()->getHelp());
                } else {
                    // Show help for specific command
                    return CommandResult::Success(m_app->getCommandProcessor()->getHelp(command));
                }
            }),
            
        // STATUS command
        CommandRegistration()
            .withName(Commands::STATUS)
            .withDescription("Show editor status")
            .withCategory(CommandCategory::SYSTEM)
            .withAlias("info")
            .withAlias("stats")
            .withHandler([this](const CommandContext& ctx) {
                std::stringstream ss;
                ss << "Voxel Editor Status\n";
                ss << "==================\n";
                
                // Project info
                if (!m_app->getCurrentProject().empty()) {
                    ss << "Project: " << m_app->getCurrentProject() << "\n";
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
                ss << "  Level: " << m_app->getSmoothingLevel();
                if (m_app->getSmoothingLevel() > 0) {
                    std::string algoName;
                    switch (m_app->getSmoothingAlgorithm()) {
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
                ss << "  Preview: " << (m_app->isSmoothPreviewEnabled() ? "on" : "off") << "\n";
                
                return CommandResult::Success(ss.str());
            }),
            
        // DEBUG command
        CommandRegistration()
            .withName("debug")
            .withDescription("Debug commands for troubleshooting")
            .withCategory(CommandCategory::SYSTEM)
            .withArg("subcommand", "What to debug: camera, voxels, render, frustum, ray, grid", "string", true)
            .withHandler([this](const CommandContext& ctx) {
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
                    
                    // Note: Access to voxel meshes would need to be exposed via Application
                    ss << "\nVoxel mesh information not available in this context\n";
                    
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
                    if (m_app->getMouseInteraction()) {
                        bool currentState = m_app->getMouseInteraction()->isRayVisualizationEnabled();
                        m_app->getMouseInteraction()->setRayVisualizationEnabled(!currentState);
                        
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
                    // Toggle debug grid overlay - not implemented in this version
                    return CommandResult::Error("Debug grid overlay feature not yet implemented");
                }
                else {
                    return CommandResult::Error("Unknown debug subcommand. Use: camera, voxels, render, frustum, ray, or grid");
                }
            }),
            
        // QUIT command
        CommandRegistration()
            .withName(Commands::QUIT)
            .withDescription("Exit the application")
            .withCategory(CommandCategory::SYSTEM)
            .withHandler([this](const CommandContext& ctx) {
                return CommandResult::Exit("Goodbye!");
            }),
            
        // EXIT command (alias for quit)
        CommandRegistration()
            .withName(Commands::EXIT)
            .withDescription("Exit the application")
            .withCategory(CommandCategory::SYSTEM)
            .withHandler([this](const CommandContext& ctx) {
                return CommandResult::Exit("Goodbye!");
            }),
            
        // VERSION command (alias for build)
        CommandRegistration()
            .withName("version")
            .withDescription("Show version information")
            .withCategory(CommandCategory::SYSTEM)
            .withHandler([this](const CommandContext& ctx) {
                // Redirect to build command
                return m_app->getCommandProcessor()->executeCommand("build", {});
            }),
            
        // WORKSPACE info command
        CommandRegistration()
            .withName("workspace-info")
            .withDescription("Show workspace information")
            .withCategory(CommandCategory::SYSTEM)
            .withAlias("ws-info")
            .withHandler([this](const CommandContext& ctx) {
                auto wsSize = m_voxelManager->getWorkspaceSize();
                auto resolution = m_voxelManager->getActiveResolution();
                float voxelSize = VoxelData::getVoxelSize(resolution);
                
                std::stringstream ss;
                ss << "Workspace Information\n";
                ss << "====================\n";
                ss << "Size: " << wsSize.x << " x " << wsSize.y << " x " << wsSize.z << " meters\n";
                ss << "Volume: " << (wsSize.x * wsSize.y * wsSize.z) << " m³\n";
                ss << "\nCurrent Resolution: " << VoxelData::getVoxelSizeName(resolution) << " (" << voxelSize << "m)\n";
                ss << "Max voxels per axis:\n";
                ss << "  X: " << static_cast<int>(wsSize.x / voxelSize) << " voxels\n";
                ss << "  Y: " << static_cast<int>(wsSize.y / voxelSize) << " voxels\n";
                ss << "  Z: " << static_cast<int>(wsSize.z / voxelSize) << " voxels\n";
                
                // Calculate bounds in increment coordinates
                int halfX = static_cast<int>(wsSize.x * 50.0f); // Convert to cm
                int halfY = static_cast<int>(wsSize.y * 50.0f);
                int halfZ = static_cast<int>(wsSize.z * 50.0f);
                
                ss << "\nWorkspace bounds (increment coordinates):\n";
                ss << "  X: [" << -halfX << ", " << halfX << "] cm\n";
                ss << "  Y: [0, " << (halfY * 2) << "] cm (ground plane at Y=0)\n";
                ss << "  Z: [" << -halfZ << ", " << halfZ << "] cm\n";
                
                return CommandResult::Success(ss.str());
            }),
            
        // SETTINGS command
        CommandRegistration()
            .withName(Commands::SETTINGS)
            .withDescription("Show current settings")
            .withCategory(CommandCategory::SYSTEM)
            .withAlias("config")
            .withHandler([this](const CommandContext& ctx) {
                std::stringstream ss;
                ss << "Current Settings\n";
                ss << "===============\n";
                
                // Display settings
                ss << "\nDisplay:\n";
                ss << "  Show edges: " << (m_app->getShowEdges() ? "on" : "off") << "\n";
                ss << "  Debug grid: off\n"; // Not implemented in this version
                if (m_renderEngine) {
                    ss << "  Ground plane grid: " << (m_renderEngine->isGroundPlaneGridVisible() ? "on" : "off") << "\n";
                }
                
                // Smoothing settings
                ss << "\nSmoothing:\n";
                ss << "  Level: " << m_app->getSmoothingLevel() << "\n";
                ss << "  Algorithm: ";
                switch (m_app->getSmoothingAlgorithm()) {
                    case SurfaceGen::MeshSmoother::Algorithm::None:
                        ss << "None\n";
                        break;
                    case SurfaceGen::MeshSmoother::Algorithm::Laplacian:
                        ss << "Laplacian\n";
                        break;
                    case SurfaceGen::MeshSmoother::Algorithm::Taubin:
                        ss << "Taubin\n";
                        break;
                    case SurfaceGen::MeshSmoother::Algorithm::BiLaplacian:
                        ss << "BiLaplacian\n";
                        break;
                }
                ss << "  Preview: " << (m_app->isSmoothPreviewEnabled() ? "on" : "off") << "\n";
                
                // Voxel settings
                ss << "\nVoxel:\n";
                ss << "  Active resolution: " << VoxelData::getVoxelSizeName(m_voxelManager->getActiveResolution()) << "\n";
                
                // Camera settings
                auto camera = m_cameraController->getCamera();
                ss << "\nCamera:\n";
                ss << "  FOV: " << camera->getFieldOfView() << "°\n";
                ss << "  Near plane: " << camera->getNearPlane() << "m\n";
                ss << "  Far plane: " << camera->getFarPlane() << "m\n";
                
                return CommandResult::Success(ss.str());
            }),
            
        // BENCHMARK command
        CommandRegistration()
            .withName("benchmark")
            .withDescription("Run performance benchmarks")
            .withCategory(CommandCategory::SYSTEM)
            .withAlias("bench")
            .withAlias("perf")
            .withHandler([this](const CommandContext& ctx) {
                std::stringstream ss;
                ss << "Running performance benchmarks...\n\n";
                
                // Benchmark 1: Voxel placement
                {
                    auto start = std::chrono::high_resolution_clock::now();
                    const int numOps = 1000;
                    
                    for (int i = 0; i < numOps; ++i) {
                        int x = (i % 10) * 10;
                        int y = ((i / 10) % 10) * 10;
                        int z = ((i / 100) % 10) * 10;
                        m_voxelManager->setVoxel(Math::IncrementCoordinates(x, y, z), 
                                                       m_voxelManager->getActiveResolution(), 
                                                       true);
                    }
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    
                    ss << "Voxel Placement:\n";
                    ss << "  " << numOps << " operations in " << duration.count() << " µs\n";
                    ss << "  " << (duration.count() / numOps) << " µs per operation\n";
                    ss << "  " << (numOps * 1000000.0 / duration.count()) << " ops/second\n\n";
                    
                    // Clean up
                    m_voxelManager->clearAll();
                }
                
                // Benchmark 2: Mesh generation
                {
                    // First place some voxels
                    for (int x = -50; x <= 50; x += 10) {
                        for (int y = 0; y <= 50; y += 10) {
                            for (int z = -50; z <= 50; z += 10) {
                                m_voxelManager->setVoxel(Math::IncrementCoordinates(x, y, z), 
                                                               m_voxelManager->getActiveResolution(), 
                                                               true);
                            }
                        }
                    }
                    
                    auto start = std::chrono::high_resolution_clock::now();
                    requestMeshUpdate();
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    
                    size_t voxelCount = m_voxelManager->getVoxelCount();
                    ss << "Mesh Generation:\n";
                    ss << "  " << voxelCount << " voxels processed in " << duration.count() << " ms\n";
                    if (duration.count() > 0) {
                        ss << "  " << (voxelCount * 1000.0 / duration.count()) << " voxels/second\n";
                    }
                    
                    // Clean up
                    m_voxelManager->clearAll();
                }
                
                // Benchmark 3: Ray casting (if not headless)
                // TODO: Add ray casting benchmark once screenToRay is implemented
                /*
                if (m_renderWindow) {
                    auto start = std::chrono::high_resolution_clock::now();
                    const int numRays = 10000;
                    
                    for (int i = 0; i < numRays; ++i) {
                        float x = (i % 100) / 100.0f;
                        float y = (i / 100) / 100.0f;
                        m_cameraController->getCamera()->screenToRay(x, y);
                    }
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    
                    ss << "\nRay Casting:\n";
                    ss << "  " << numRays << " rays in " << duration.count() << " µs\n";
                    ss << "  " << (duration.count() / numRays) << " µs per ray\n";
                    ss << "  " << (numRays * 1000000.0 / duration.count()) << " rays/second\n";
                }
                */
                
                ss << "\nBenchmark complete.";
                return CommandResult::Success(ss.str());
            }),
            
        // DEBUG-INFO command
        CommandRegistration()
            .withName("debug-info")
            .withDescription("Show detailed debug information")
            .withCategory(CommandCategory::SYSTEM)
            .withAlias("dbg-info")
            .withHandler([this](const CommandContext& ctx) {
                std::stringstream ss;
                ss << "Detailed Debug Information\n";
                ss << "=========================\n\n";
                
                // System info
                ss << "System:\n";
                ss << "  Platform: " << 
                    #ifdef _WIN32
                        "Windows"
                    #elif __APPLE__
                        "macOS"
                    #elif __linux__
                        "Linux"
                    #else
                        "Unknown"
                    #endif
                    << "\n";
                ss << "  Build type: " << 
                    #ifdef NDEBUG
                        "Release"
                    #else
                        "Debug"
                    #endif
                    << "\n";
                
                // OpenGL info (if not headless)
                if (m_renderWindow) {
                    ss << "\nOpenGL:\n";
                    // These would normally be retrieved via glGetString but we'll keep it simple
                    ss << "  Context created successfully\n";
                }
                
                // Memory info
                ss << "\nMemory:\n";
                ss << "  Voxel data: " << (m_voxelManager->getMemoryUsage() / 1024.0) << " KB\n";
                ss << "  Total voxels: " << m_voxelManager->getVoxelCount() << "\n";
                
                // Component status
                ss << "\nComponents:\n";
                ss << "  VoxelManager: " << (m_voxelManager ? "OK" : "ERROR") << "\n";
                ss << "  CameraController: " << (m_cameraController ? "OK" : "ERROR") << "\n";
                ss << "  CommandProcessor: " << (m_app->getCommandProcessor() ? "OK" : "ERROR") << "\n";
                ss << "  SelectionManager: " << (m_selectionManager ? "OK" : "ERROR") << "\n";
                ss << "  GroupManager: " << (m_groupManager ? "OK" : "ERROR") << "\n";
                ss << "  HistoryManager: " << (m_historyManager ? "OK" : "ERROR") << "\n";
                ss << "  FileManager: " << (m_fileManager ? "OK" : "ERROR") << "\n";
                ss << "  RenderEngine: " << (m_renderEngine ? "OK" : "N/A (headless)") << "\n";
                ss << "  RenderWindow: " << (m_renderWindow ? "OK" : "N/A (headless)") << "\n";
                
                return CommandResult::Success(ss.str());
            }),
            
        // SLEEP command
        CommandRegistration()
            .withName("sleep")
            .withDescription("Pause execution for specified seconds")
            .withCategory(CommandCategory::SYSTEM)
            .withAlias("wait")
            .withAlias("pause")
            .withArg("seconds", "Number of seconds to sleep", "float", true)
            .withHandler([this](const CommandContext& ctx) {
                float seconds = ctx.getFloatArg(0, 1.0f);
                if (seconds < 0 || seconds > 10) {
                    return CommandResult::Error("Sleep time must be between 0 and 10 seconds");
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(seconds * 1000)));
                return CommandResult::Success("Slept for " + std::to_string(seconds) + " seconds");
            }),
            
        // VALIDATE command
        CommandRegistration()
            .withName(Commands::VALIDATE)
            .withDescription("Validate the rendering pipeline and diagnose issues")
            .withCategory(CommandCategory::SYSTEM)
            .withAlias("check")
            .withAlias("diag")
            .withHandler([this](const CommandContext& ctx) {
                return executeSimpleValidateCommand(ctx);
            }),
            
        // BUILD command
        CommandRegistration()
            .withName(Commands::BUILD)
            .withDescription("Show build information")
            .withCategory(CommandCategory::SYSTEM)
            .withAlias("buildinfo")
            .withAlias("version")
            .withHandler([this](const CommandContext& ctx) {
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
            }),
            
        // WORKSPACE command
        CommandRegistration()
            .withName("workspace")
            .withDescription("Set workspace dimensions")
            .withCategory(CommandCategory::SYSTEM)
            .withAlias("ws")
            .withArg("width", "Width in meters (2-8m)", "float", true)
            .withArg("height", "Height in meters (2-8m)", "float", true)
            .withArg("depth", "Depth in meters (2-8m)", "float", true)
            .withHandler([this](const CommandContext& ctx) {
                float width = ctx.getFloatArg(0);
                float height = ctx.getFloatArg(1);
                float depth = ctx.getFloatArg(2);
                
                // Validate dimensions
                const float MIN_SIZE = 2.0f;
                const float MAX_SIZE = 8.0f;
                
                if (width < MIN_SIZE || width > MAX_SIZE ||
                    height < MIN_SIZE || height > MAX_SIZE ||
                    depth < MIN_SIZE || depth > MAX_SIZE) {
                    return CommandResult::Error("Workspace dimensions must be between 2m and 8m");
                }
                
                // Resize workspace
                m_voxelManager->resizeWorkspace(Math::Vector3f(width, height, depth));
                
                // Update ground plane grid if rendering is enabled
                if (m_renderEngine) {
                    m_renderEngine->updateGroundPlaneGrid(Math::Vector3f(width, height, depth));
                }
                
                std::stringstream ss;
                ss << "Workspace resized to " << width << "x" << height << "x" << depth << " meters";
                return CommandResult::Success(ss.str());
            }),
            
        // CLEAR command
        CommandRegistration()
            .withName(Commands::CLEAR)
            .withDescription("Clear all voxels")
            .withCategory(CommandCategory::SYSTEM)
            .withAlias("cls")
            .withHandler([this](const CommandContext& ctx) {
                m_voxelManager->clearAll();
                m_selectionManager->selectNone();
                m_historyManager->clearHistory();
                requestMeshUpdate();
                return CommandResult::Success("All voxels cleared");
            })
    };
}

// Auto-register this module
REGISTER_COMMAND_MODULE(SystemCommands)

} // namespace VoxelEditor