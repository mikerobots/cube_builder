#include "cli/CommandRegistry.h"
#include "cli/Application.h"

namespace VoxelEditor {

CommandModule::CommandModule(Application* app) : m_app(app) {
    if (app) {
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

void CommandModule::requestMeshUpdate() {
    if (m_app) {
        m_app->requestMeshUpdate();
    }
}

} // namespace VoxelEditor