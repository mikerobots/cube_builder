#pragma once

#include "CommandRegistry.h"
#include "Application.h"

namespace VoxelEditor {

/**
 * @brief Mesh and surface generation commands module
 * 
 * Commands for mesh operations and surface generation:
 * - smooth: Control mesh smoothing settings (level, algorithm, preview)
 * - mesh: Mesh validation and information (info, validate, repair)
 * - surface-export: Export surface mesh to various formats (STL, OBJ)
 * - surface-preview: Enable/disable real-time surface preview
 * - surface-settings: Configure surface generation settings
 * 
 * These commands provide comprehensive control over the surface generation
 * pipeline, including smoothing algorithms, quality settings, and export options.
 */
class MeshCommands : public CommandModule, public ICommandModule {
public:
    MeshCommands() : CommandModule(nullptr) {}
    
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