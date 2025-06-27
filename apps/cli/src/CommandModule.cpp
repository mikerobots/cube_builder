#include "cli/CommandRegistry.h"
#include "cli/Application.h"
#include "cli/RenderWindow.h"
#include "voxel_data/VoxelDataManager.h"
#include "undo_redo/HistoryManager.h"
#include "camera/CameraController.h"
#include "rendering/RenderEngine.h"
#include "selection/SelectionManager.h"
#include "groups/GroupManager.h"
#include "file_io/FileManager.h"
#include "events/EventDispatcher.h"

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
    if (m_renderWindow) {
        m_renderWindow->requestMeshUpdate();
    }
}

} // namespace VoxelEditor