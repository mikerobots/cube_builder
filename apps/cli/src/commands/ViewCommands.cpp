#include "cli/ViewCommands.h"
#include "cli/Application.h"
#include "cli/RenderWindow.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "camera/OrbitCamera.h"
#include "rendering/RenderEngine.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "voxel_data/VoxelGrid.h"
#include "math/BoundingBox.h"
#include "math/CoordinateTypes.h"
#include "math/Vector3f.h"
#include <sstream>
#include <iomanip>
#include <vector>

namespace VoxelEditor {

std::vector<CommandRegistration> ViewCommands::getCommands() {
    return {
        // CAMERA command
        CommandRegistration()
            .withName(Commands::CAMERA)
            .withDescription("Set camera view preset")
            .withCategory(CommandCategory::VIEW)
            .withAlias("view")
            .withArg("preset", "View preset (front/back/left/right/top/bottom/iso/default)", "string", true)
            .withHandler([this](const CommandContext& ctx) {
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
            }),
            
        // ZOOM command
        CommandRegistration()
            .withName(Commands::ZOOM)
            .withDescription("Zoom camera in/out")
            .withCategory(CommandCategory::VIEW)
            .withAlias("z")
            .withArg("factor", "Zoom factor (e.g., 1.5 to zoom in, 0.8 to zoom out)", "float", true)
            .withHandler([this](const CommandContext& ctx) {
                float factor = ctx.getFloatArg(0, 1.0f);
                if (factor <= 0) {
                    return CommandResult::Error("Zoom factor must be positive");
                }
                
                float currentDistance = m_cameraController->getCamera()->getDistance();
                m_cameraController->getCamera()->setDistance(currentDistance / factor);
                
                // Force camera matrix update
                m_cameraController->getCamera()->getViewMatrix();
                
                return CommandResult::Success("Zoomed by factor " + std::to_string(factor));
            }),
            
        // ROTATE command
        CommandRegistration()
            .withName(Commands::ROTATE)
            .withDescription("Rotate camera")
            .withCategory(CommandCategory::VIEW)
            .withAlias("rot")
            .withArg("x", "X rotation in degrees", "float", true)
            .withArg("y", "Y rotation in degrees", "float", true)
            .withHandler([this](const CommandContext& ctx) {
                float deltaX = ctx.getFloatArg(0, 0.0f);
                float deltaY = ctx.getFloatArg(1, 0.0f);
                
                dynamic_cast<Camera::OrbitCamera*>(m_cameraController->getCamera())->orbit(
                    deltaX,
                    deltaY
                );
                
                // Force camera matrix update
                m_cameraController->getCamera()->getViewMatrix();
                
                return CommandResult::Success("Camera rotated");
            }),
            
        // RESET_VIEW command
        CommandRegistration()
            .withName(Commands::RESET_VIEW)
            .withDescription("Reset camera to default view")
            .withCategory(CommandCategory::VIEW)
            .withAlias("reset")
            .withHandler([this](const CommandContext& ctx) {
                m_cameraController->setViewPreset(Camera::ViewPreset::ISOMETRIC);
                
                // Force camera matrix update
                m_cameraController->getCamera()->getViewMatrix();
                m_cameraController->getCamera()->getProjectionMatrix();
                
                return CommandResult::Success("Camera reset to default view");
            }),
            
        // GRID command
        CommandRegistration()
            .withName(Commands::GRID)
            .withDescription("Toggle ground plane grid visibility")
            .withCategory(CommandCategory::VIEW)
            .withAlias("groundplane")
            .withArg("state", "on/off/toggle (optional)", "string", false, "toggle")
            .withHandler([this](const CommandContext& ctx) {
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
            }),
            
        // CENTER command
        CommandRegistration()
            .withName("center")
            .withDescription("Center camera on origin or voxels")
            .withCategory(CommandCategory::VIEW)
            .withAlias("focus")
            .withAlias("home")
            .withArg("target", "Center target (origin/voxels/x,y,z)", "string", false, "voxels")
            .withHandler([this](const CommandContext& ctx) {
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
            }),
            
        // CAMERA-INFO command
        CommandRegistration()
            .withName("camera-info")
            .withDescription("Show current camera information")
            .withCategory(CommandCategory::VIEW)
            .withAlias("cam-info")
            .withAlias("ci")
            .withHandler([this](const CommandContext& ctx) {
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
            }),
            
        // SHADER command
        CommandRegistration()
            .withName("shader")
            .withDescription("Switch between shader modes or list available shaders")
            .withCategory(CommandCategory::VIEW)
            .withArg("mode", "Shader mode: basic, enhanced, flat, or 'list' to show all", "string", false, "list")
            .withHandler([this](const CommandContext& ctx) {
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
                    auto currentShaderId = m_app->getDefaultShaderId();
                    if (currentShaderId != Rendering::InvalidId) {
                        // Check which shader is currently active
                        if (currentShaderId == m_renderEngine->getBuiltinShader("basic")) {
                            currentShader = "basic";
                        } else if (currentShaderId == m_renderEngine->getBuiltinShader("enhanced")) {
                            currentShader = "enhanced";
                        } else if (currentShaderId == m_renderEngine->getBuiltinShader("flat")) {
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
                m_app->setDefaultShaderId(shaderId);
                
                // Update the voxel mesh to trigger re-render with new shader
                requestMeshUpdate();
                
                return CommandResult::Success("Shader mode set to: " + mode);
            }),
            
        // EDGES command
        CommandRegistration()
            .withName("edges")
            .withDescription("Toggle edge/wireframe overlay rendering")
            .withCategory(CommandCategory::VIEW)
            .withArg("state", "on/off to enable/disable edges, or 'toggle' to switch", "string", false, "toggle")
            .withHandler([this](const CommandContext& ctx) {
                std::string state = ctx.getArgCount() > 0 ? ctx.getArg(0) : "toggle";
                
                bool currentState = m_app->getShowEdges();
                bool newState;
                
                if (state == "toggle") {
                    newState = !currentState;
                } else if (state == "on") {
                    newState = true;
                } else if (state == "off") {
                    newState = false;
                } else {
                    return CommandResult::Error("Invalid state. Use: on, off, or toggle");
                }
                
                m_app->setShowEdges(newState);
                
                // Trigger re-render
                requestMeshUpdate();
                
                return CommandResult::Success("Edge rendering " + std::string(newState ? "enabled" : "disabled"));
            }),
            
        // SCREENSHOT command
        CommandRegistration()
            .withName("screenshot")
            .withDescription("Take a screenshot of the current view")
            .withCategory(CommandCategory::VIEW)
            .withAlias("ss")
            .withAlias("capture")
            .withArg("filename", "Output filename (.png)", "string", true)
            .withHandler([this](const CommandContext& ctx) {
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
                m_app->render();
                
                // Don't swap before screenshot - read from back buffer
                // m_renderWindow->swapBuffers();
                
                if (m_renderWindow->saveScreenshot(filename)) {
                    return CommandResult::Success("Screenshot saved: " + filename);
                }
                return CommandResult::Error("Failed to save screenshot");
            })
    };
}

// Auto-register this module
REGISTER_COMMAND_MODULE(ViewCommands)

} // namespace VoxelEditor