#pragma once

#include "CommandRegistry.h"

namespace VoxelEditor {

/**
 * @brief View control commands module
 * 
 * Commands for controlling camera and view settings:
 * - camera: Set camera view preset (front/back/left/right/top/bottom/iso/default)
 * - zoom: Zoom camera in/out
 * - rotate: Rotate camera
 * - reset_view: Reset camera to default view
 * - grid: Toggle ground plane grid visibility
 * - center: Center camera on origin, voxels, or specific coordinates
 * - camera-info: Show current camera information
 * - shader: Switch between shader modes or list available shaders
 * - edges: Toggle edge/wireframe overlay rendering
 * - screenshot: Take a screenshot of the current view
 */
class ViewCommands : public ICommandModule {
public:
    std::vector<CommandRegistration> getCommands() override;
};

} // namespace VoxelEditor