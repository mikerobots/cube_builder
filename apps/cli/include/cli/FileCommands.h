#pragma once

#include "CommandRegistry.h"
#include "Application.h"

namespace VoxelEditor {

/**
 * @brief File operation commands module
 * 
 * This module handles all file-related commands using a dynamic registration system.
 * To add a new file command, simply add it to the getCommands() method.
 */
class FileCommands : public CommandModule, public ICommandModule {
public:
    FileCommands() : CommandModule(nullptr) {}
    
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
            
            // Sync current project with application
            m_currentProject = app->getCurrentProject();
        }
    }
    
    std::vector<CommandRegistration> getCommands() override;
    
private:
    std::string m_currentProject;  // Track current project filename
};

} // namespace VoxelEditor