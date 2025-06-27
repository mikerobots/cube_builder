#pragma once

#include "CommandRegistry.h"
#include "Application.h"

namespace VoxelEditor {

/**
 * @brief System and utility commands
 * 
 * Commands for system operations:
 * - help: Show available commands
 * - status: Show current state
 * - debug: Debug utilities
 * - quit/exit: Exit the application
 * - version: Show version info
 */
class SystemCommands : public CommandModule, public ICommandModule {
public:
    SystemCommands() : CommandModule(nullptr) {}
    
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