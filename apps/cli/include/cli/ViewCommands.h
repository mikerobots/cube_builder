#pragma once

#include "CommandRegistry.h"
#include "Application.h"

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
class ViewCommands : public CommandModule, public ICommandModule {
public:
    ViewCommands() : CommandModule(nullptr) {}
    
    void setApplication(Application* app) override {
        m_app = app;
        if (app) {
            // Initialize all the pointers from CommandModule base class
            m_voxelManager = app->getVoxelManager();
            m_historyManager = app->getHistoryManager();
            m_cameraController = app->getCameraController();
            m_renderEngine = app->getRenderEngine();
            m_selectionManager = app->getSelectionManager();
            m_groupManager = app->getGroupManager();
            m_fileManager = app->getFileManager();
            m_eventDispatcher = app->getEventDispatcher();
            m_renderWindow = app->getRenderWindow();
        }
    }
    
    std::vector<CommandRegistration> getCommands() override;
};

} // namespace VoxelEditor