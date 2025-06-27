#pragma once

#include "CommandRegistry.h"
#include "Application.h"

namespace VoxelEditor {

/**
 * @brief Edit operation commands module
 * 
 * Commands for editing voxels:
 * - place: Place a voxel at specified location
 * - delete/remove: Remove voxel at location
 * - fill: Fill a box region with voxels
 * - undo: Undo last operation
 * - redo: Redo last undone operation
 */
class EditCommands : public CommandModule, public ICommandModule {
public:
    EditCommands() : CommandModule(nullptr) {}
    
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