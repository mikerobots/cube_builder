#pragma once

#include "CommandRegistry.h"
#include "Application.h"

namespace VoxelEditor {

/**
 * @brief Selection operation commands module
 * 
 * Commands for managing voxel selection:
 * - select: Select voxel at position
 * - select-box: Select voxels in box region
 * - select-sphere: Select voxels in sphere region
 * - select-all: Select all voxels
 * - select-none: Clear selection
 * - select-resolution: Select all voxels of a specific resolution
 * - invert-selection: Invert current selection
 * - selection-info: Show selection information
 * - delete-selected: Delete all selected voxels
 * - group-selected: Create group from selection
 */
class SelectCommands : public CommandModule, public ICommandModule {
public:
    SelectCommands() : CommandModule(nullptr) {}
    
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