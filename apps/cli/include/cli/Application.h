#pragma once

#include <memory>
#include <string>

// Include singleton headers for complete types
#include "../../foundation/logging/Logger.h"
#include "../../foundation/config/ConfigManager.h"

// Include core types we need
#include "../../core/rendering/RenderTypes.h"

// Forward declarations of core systems
namespace VoxelEditor {
    namespace VoxelData { class VoxelDataManager; }
    namespace Camera { class CameraController; class OrbitCamera; }
    namespace Rendering { class OpenGLRenderer; class RenderEngine; }
    namespace Input { class InputManager; }
    namespace Selection { class SelectionManager; }
    namespace UndoRedo { class HistoryManager; }
    namespace SurfaceGen { class SurfaceGenerator; }
    namespace VisualFeedback { class FeedbackRenderer; }
    namespace Groups { class GroupManager; }
    namespace FileIO { class FileManager; }
    
    namespace Events { class EventDispatcher; }
    namespace Logging { class Logger; }
    namespace Config { class ConfigManager; }
}

namespace VoxelEditor {

class CommandProcessor;
class RenderWindow;
class MouseInteraction;
class VoxelMeshGenerator;

class Application {
public:
    Application();
    ~Application();
    
    // Application lifecycle
    bool initialize(int argc, char* argv[]);
    int run();
    void shutdown();
    
    // System access
    VoxelData::VoxelDataManager* getVoxelManager() const { return m_voxelManager.get(); }
    Camera::CameraController* getCameraController() const { return m_cameraController.get(); }
    Rendering::RenderEngine* getRenderEngine() const { return m_renderEngine.get(); }
    Rendering::OpenGLRenderer* getOpenGLRenderer() const { return m_openGLRenderer.get(); }
    Input::InputManager* getInputManager() const { return m_inputManager.get(); }
    Selection::SelectionManager* getSelectionManager() const { return m_selectionManager.get(); }
    UndoRedo::HistoryManager* getHistoryManager() const { return m_historyManager.get(); }
    SurfaceGen::SurfaceGenerator* getSurfaceGenerator() const { return m_surfaceGenerator.get(); }
    VisualFeedback::FeedbackRenderer* getFeedbackRenderer() const { return m_feedbackRenderer.get(); }
    Groups::GroupManager* getGroupManager() const { return m_groupManager.get(); }
    FileIO::FileManager* getFileManager() const { return m_fileManager.get(); }
    Events::EventDispatcher* getEventDispatcher() const { return m_eventDispatcher.get(); }
    Logging::Logger* getLogger() const { return &Logging::Logger::getInstance(); }
    Config::ConfigManager* getConfigManager() const { return &Config::ConfigManager::getInstance(); }
    
    // Application state
    bool isRunning() const { return m_running; }
    void quit() { m_running = false; }
    
    // Project management
    const std::string& getCurrentProject() const { return m_currentProject; }
    void setCurrentProject(const std::string& path) { m_currentProject = path; }
    
    // Window management
    RenderWindow* getRenderWindow() const { return m_renderWindow.get(); }
    
    // Rendering updates
    void requestMeshUpdate() { updateVoxelMesh(); }
    void updateVoxelMeshes() { updateVoxelMesh(); } // Alias for tests
    void render(); // Make render public for tests
    
    // Headless mode
    bool isHeadless() const { return m_headless; }
    void setHeadless(bool headless) { m_headless = headless; }
    
    // Test support methods
    void setHoverPosition(const Math::Vector3i& pos) { m_hoverPosition = pos; }
    CommandProcessor* getCommandProcessor() const { return m_commandProcessor.get(); }
    
private:
    // Core systems
    std::unique_ptr<VoxelData::VoxelDataManager> m_voxelManager;
    std::unique_ptr<Camera::CameraController> m_cameraController;
    std::unique_ptr<Rendering::OpenGLRenderer> m_openGLRenderer;
    std::unique_ptr<Rendering::RenderEngine> m_renderEngine;
    std::unique_ptr<Input::InputManager> m_inputManager;
    std::unique_ptr<Selection::SelectionManager> m_selectionManager;
    std::unique_ptr<UndoRedo::HistoryManager> m_historyManager;
    std::unique_ptr<SurfaceGen::SurfaceGenerator> m_surfaceGenerator;
    std::unique_ptr<VisualFeedback::FeedbackRenderer> m_feedbackRenderer;
    std::unique_ptr<Groups::GroupManager> m_groupManager;
    std::unique_ptr<FileIO::FileManager> m_fileManager;
    
    // Foundation systems
    std::unique_ptr<Events::EventDispatcher> m_eventDispatcher;
    // Logger and ConfigManager are singletons, not stored as members
    
    // CLI components
    std::unique_ptr<CommandProcessor> m_commandProcessor;
    std::unique_ptr<RenderWindow> m_renderWindow;
    std::unique_ptr<MouseInteraction> m_mouseInteraction;
    std::unique_ptr<VoxelMeshGenerator> m_meshGenerator;
    
    // Application state
    bool m_running = false;
    bool m_headless = false;
    std::string m_currentProject;
    Math::Vector3i m_hoverPosition;
    
    // Current scene data for rendering
    std::vector<Rendering::Mesh> m_voxelMeshes;
    std::vector<Rendering::Mesh> m_edgeMeshes;  // Wireframe edge meshes
    Rendering::ShaderId m_defaultShaderId = Rendering::InvalidId;
    bool m_showEdges = true;  // Toggle for edge rendering
    
    // Private methods
    bool initializeFoundation();
    bool initializeCoreSystem();
    bool initializeRendering();
    bool initializeCLI();
    void registerCommands();
    void processInput();
    
    // Rendering helpers  
    void updateVoxelMesh();
    void createScene();
};

} // namespace VoxelEditor