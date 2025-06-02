// Standard library includes first
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include <algorithm>
#include <cstddef>  // for offsetof

// Application headers
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/RenderWindow.h"
#include "cli/MouseInteraction.h"

// Core includes
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "rendering/RenderEngine.h"
#include "rendering/OpenGLRenderer.h"
#include "rendering/RenderConfig.h"
#include "rendering/RenderTypes.h"
#include "rendering/RenderContext.h"
#include "input/InputManager.h"
#include "selection/SelectionManager.h"
#include "selection/SelectionTypes.h"
#include "undo_redo/HistoryManager.h"
#include "surface_gen/SurfaceGenerator.h"
#include "visual_feedback/FeedbackRenderer.h"
#include "groups/GroupManager.h"
#include "groups/GroupTypes.h"
#include "file_io/FileManager.h"

// Foundation includes  
#include "events/EventDispatcher.h"
#include "logging/Logger.h"
#include "config/ConfigManager.h"
#include "math/Matrix4f.h"
#include "math/Vector2f.h"
#include "math/Vector3f.h"

namespace VoxelEditor {

Application::Application() = default;
Application::~Application() = default;

bool Application::initialize(int argc, char* argv[]) {
    std::cout << "Initializing Voxel Editor..." << std::endl;
    
    if (!initializeFoundation()) {
        return false;
    }
    
    if (!initializeCoreSystem()) {
        return false;
    }
    
    if (!initializeRendering()) {
        return false;
    }
    
    if (!initializeCLI()) {
        return false;
    }
    
    registerCommands();
    
    m_running = true;
    
    std::cout << "Initialization complete!" << std::endl;
    std::cout << "Type 'help' for available commands.\n" << std::endl;
    
    return true;
}

int Application::run() {
    // Main loop
    while (m_running && m_renderWindow->isOpen()) {
        // Poll window events
        m_renderWindow->pollEvents();
        
        // Process command input (non-blocking)
        processInput();
        
        // Update systems
        m_inputManager->update(0.016f); // 60 FPS target
        m_mouseInteraction->update();
        
        // Render
        render();
        
        // Swap buffers
        m_renderWindow->swapBuffers();
        
        // Small sleep to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    shutdown();
    return 0;
}

void Application::shutdown() {
    std::cout << "\nShutting down..." << std::endl;
    
    // Cleanup in reverse order
    m_mouseInteraction.reset();
    m_renderWindow.reset();
    m_commandProcessor.reset();
    
    // Core systems
    m_fileManager.reset();
    m_groupManager.reset();
    m_feedbackRenderer.reset();
    m_surfaceGenerator.reset();
    m_historyManager.reset();
    m_selectionManager.reset();
    m_inputManager.reset();
    // m_renderEngine.reset();
    m_openGLRenderer.reset();
    m_cameraController.reset();
    m_voxelManager.reset();
    
    // Foundation (singletons don't need reset, only EventDispatcher)
    m_eventDispatcher.reset();
}

bool Application::initializeFoundation() {
    try {
        // Event system first
        m_eventDispatcher = std::make_unique<Events::EventDispatcher>();
        
        // Logger is a singleton
        Logging::Logger::getInstance().setLevel(Logging::LogLevel::Info);
        // Note: Logger doesn't have enableComponent(), console output is added by default
        
        // Config is a singleton
        Config::ConfigManager::getInstance().setValue("workspace.size", 5.0f);
        Config::ConfigManager::getInstance().setValue("workspace.min", 2.0f);
        Config::ConfigManager::getInstance().setValue("workspace.max", 8.0f);
        Config::ConfigManager::getInstance().setValue("voxel.defaultResolution", static_cast<int>(VoxelData::VoxelResolution::Size_8cm));
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize foundation: " << e.what() << std::endl;
        return false;
    }
}

bool Application::initializeCoreSystem() {
    try {
        // Voxel data manager - constructor only takes EventDispatcher
        m_voxelManager = std::make_unique<VoxelData::VoxelDataManager>(
            m_eventDispatcher.get()
        );
        
        // Camera controller
        m_cameraController = std::make_unique<Camera::CameraController>(m_eventDispatcher.get());
        m_cameraController->setViewPreset(Camera::ViewPreset::ISOMETRIC);
        
        // Input manager
        m_inputManager = std::make_unique<Input::InputManager>(m_eventDispatcher.get());
        
        // Selection manager
        m_selectionManager = std::make_unique<Selection::SelectionManager>(
            m_voxelManager.get(),
            m_eventDispatcher.get()
        );
        
        // History manager
        m_historyManager = std::make_unique<UndoRedo::HistoryManager>();
        m_historyManager->setMaxHistorySize(20);
        
        // Surface generator
        m_surfaceGenerator = std::make_unique<SurfaceGen::SurfaceGenerator>();
        
        // Group manager
        m_groupManager = std::make_unique<Groups::GroupManager>(
            m_voxelManager.get(),
            m_eventDispatcher.get()
        );
        
        // File manager
        m_fileManager = std::make_unique<FileIO::FileManager>();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize core systems: " << e.what() << std::endl;
        return false;
    }
}

bool Application::initializeRendering() {
    try {
        // Create render window
        m_renderWindow = std::make_unique<RenderWindow>(this);
        if (!m_renderWindow->create(1280, 720, "Voxel Editor")) {
            return false;
        }
        
        // Create OpenGL renderer
        m_openGLRenderer = std::make_unique<Rendering::OpenGLRenderer>();
        Rendering::RenderConfig renderConfig;
        renderConfig.windowWidth = 1280;
        renderConfig.windowHeight = 720;
        renderConfig.samples = 4;
        renderConfig.vsync = true;
        
        if (!m_openGLRenderer->initializeContext(renderConfig)) {
            return false;
        }
        
        // TODO: Create render engine when implementation is complete
        // m_renderEngine = std::make_unique<Rendering::RenderEngine>(m_eventDispatcher.get());
        // if (!m_renderEngine->initialize(renderConfig)) {
        //     return false;
        // }
        
        // Create feedback renderer (pass nullptr for now)
        m_feedbackRenderer = std::make_unique<VisualFeedback::FeedbackRenderer>(nullptr);
        
        // Set up window callbacks
        m_renderWindow->setMouseCallback([this](const MouseEvent& event) {
            // Mouse interaction handles all mouse events
            // The InputManager integration is handled internally by MouseInteraction
        });
        
        m_renderWindow->setKeyCallback([this](const KeyEvent& event) {
            Input::KeyEvent coreEvent;
            coreEvent.key = static_cast<Input::KeyCode>(event.key);
            coreEvent.type = event.pressed ? Input::KeyEventType::Press : Input::KeyEventType::Release;
            coreEvent.repeat = event.repeat;
            coreEvent.modifiers = Input::ModifierFlags::None;
            if (event.shift) coreEvent.modifiers = static_cast<Input::ModifierFlags>(static_cast<int>(coreEvent.modifiers) | static_cast<int>(Input::ModifierFlags::Shift));
            if (event.ctrl) coreEvent.modifiers = static_cast<Input::ModifierFlags>(static_cast<int>(coreEvent.modifiers) | static_cast<int>(Input::ModifierFlags::Ctrl));
            if (event.alt) coreEvent.modifiers = static_cast<Input::ModifierFlags>(static_cast<int>(coreEvent.modifiers) | static_cast<int>(Input::ModifierFlags::Alt));
            
            m_inputManager->injectKeyboardEvent(coreEvent);
        });
        
        m_renderWindow->setResizeCallback([this](int width, int height) {
            if (m_cameraController) {
                m_cameraController->setViewportSize(width, height);
            }
            // TODO: Set viewport when RenderEngine is implemented
            // if (m_renderEngine) {
            //     m_renderEngine->setViewport(0, 0, width, height);
            // }
        });
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize rendering: " << e.what() << std::endl;
        return false;
    }
}

bool Application::initializeCLI() {
    try {
        m_commandProcessor = std::make_unique<CommandProcessor>(this);
        m_mouseInteraction = std::make_unique<MouseInteraction>(this);
        m_mouseInteraction->initialize();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize CLI: " << e.what() << std::endl;
        return false;
    }
}

// registerCommands() is implemented in Commands.cpp

void Application::processInput() {
    // Non-blocking input processing
    // In a real implementation, this would use a separate thread or
    // non-blocking console I/O
    
    // For now, we'll use a simple blocking approach
    static std::string input;
    static bool waitingForInput = true;
    
    if (waitingForInput) {
        std::cout << "> " << std::flush;
        waitingForInput = false;
    }
    
    // Check if input is available (platform-specific)
    // For now, we'll use std::getline which blocks
    if (std::getline(std::cin, input)) {
        auto result = m_commandProcessor->execute(input);
        
        if (!result.message.empty()) {
            std::cout << result.message << std::endl;
        }
        
        if (result.shouldExit) {
            m_running = false;
        }
        
        waitingForInput = true;
    }
}

void Application::render() {
    // TODO: Implement rendering when OpenGL integration is complete
    // For now, just clear the screen
    if (m_renderWindow) {
        // Clear is handled by OpenGL context in RenderWindow
    }
    
    
    // Visual feedback would be rendered here
    // m_feedbackRenderer->render(*m_cameraController->getCamera(), context);
}

} // namespace VoxelEditor
