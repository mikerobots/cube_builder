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

// Silence OpenGL deprecation warnings on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

// OpenGL headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenGL/glext.h>
#endif

// OpenGL function pointers for VAO (needed because our GLAD is a stub)
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint *arrays);

static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;

// Define missing GL constants
#ifndef GL_VERTEX_ARRAY_BINDING
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#endif

// Application headers
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/RenderWindow.h"
#include "cli/MouseInteraction.h"
#include "cli/VoxelMeshGenerator.h"

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
#include "math/Vector4f.h"

namespace VoxelEditor {

Application::Application() = default;

Application::~Application() {
    cleanupGL();
}

bool Application::initialize(int argc, char* argv[]) {
    std::cout << "Initializing Voxel Editor..." << std::endl;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless" || arg == "-h") {
            m_headless = true;
            std::cout << "Running in headless mode (no rendering)" << std::endl;
        }
    }
    
    if (!initializeFoundation()) {
        return false;
    }
    
    if (!initializeCoreSystem()) {
        return false;
    }
    
    if (!m_headless) {
        if (!initializeRendering()) {
            return false;
        }
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
    if (m_headless) {
        // Headless mode - just process commands
        while (m_running) {
            processInput();
        }
    } else {
        // Normal mode with rendering
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
        
        // Set camera to look at center of 5x5x5 voxel grid
        m_cameraController->setViewPreset(Camera::ViewPreset::ISOMETRIC);
        // Center of 5x5x5 grid at 8cm voxels is at (0.2, 0.2, 0.2)
        m_cameraController->getCamera()->setTarget(Math::Vector3f(0.20f, 0.20f, 0.20f));
        m_cameraController->getCamera()->setDistance(1.3f);  // Optimal distance for 5x5x5 grid
        
        // Debug: Camera setup for 5x5x5 voxel grid
        std::cout << "DEBUG: Setting up camera to view center of 5x5x5 grid at (0.2, 0.2, 0.2)" << std::endl;
        
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
        
        if (!m_headless) {
            m_mouseInteraction = std::make_unique<MouseInteraction>(this);
            m_mouseInteraction->initialize();
            m_meshGenerator = std::make_unique<VoxelMeshGenerator>();
            
            // Check OpenGL version
            const GLubyte* version = glGetString(GL_VERSION);
            const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
            std::cout << "OpenGL Version: " << version << std::endl;
            std::cout << "GLSL Version: " << glslVersion << std::endl;
        
        // Check for VAO extensions
        const GLubyte* extensions = glGetString(GL_EXTENSIONS);
        std::string extStr(reinterpret_cast<const char*>(extensions));
        bool hasVAO = extStr.find("GL_ARB_vertex_array_object") != std::string::npos;
        bool hasVAOApple = extStr.find("GL_APPLE_vertex_array_object") != std::string::npos;
        std::cout << "VAO extensions: ARB=" << hasVAO << " APPLE=" << hasVAOApple << std::endl;
        
        // Load VAO functions (needed because our GLAD is a stub)
        // Try ARB extension first for OpenGL 2.1
        glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) glfwGetProcAddress("glGenVertexArraysARB");
        glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) glfwGetProcAddress("glBindVertexArrayARB");
        glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) glfwGetProcAddress("glDeleteVertexArraysARB");
        
        // Fall back to core functions if ARB not available
        if (!glGenVertexArrays) {
            glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) glfwGetProcAddress("glGenVertexArrays");
        }
        if (!glBindVertexArray) {
            glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) glfwGetProcAddress("glBindVertexArray");
        }
        if (!glDeleteVertexArrays) {
            glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) glfwGetProcAddress("glDeleteVertexArrays");
        }
        
        // Check for Apple extension as last resort
        if (!glGenVertexArrays) {
            glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) glfwGetProcAddress("glGenVertexArraysAPPLE");
        }
        if (!glBindVertexArray) {
            glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) glfwGetProcAddress("glBindVertexArrayAPPLE");
        }
        if (!glDeleteVertexArrays) {
            glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) glfwGetProcAddress("glDeleteVertexArraysAPPLE");
        }
        
            if (!glGenVertexArrays || !glBindVertexArray || !glDeleteVertexArrays) {
                std::cerr << "Warning: VAO functions not available, will use client-side vertex arrays" << std::endl;
                // Don't fail - we can work without VAOs in OpenGL 2.1
            } else {
                std::cout << "VAO functions loaded successfully" << std::endl;
            }
            
            // Create shader program for voxel rendering
            if (!createShaderProgram()) {
                std::cerr << "Failed to create shader program" << std::endl;
                return false;
            }
            
            // Create initial mesh (empty)
            updateVoxelMesh();
        }
        
        // Subscribe to voxel change events
        // TODO: Implement proper event handlers when EventHandler supports lambdas
        // For now, voxel mesh will be updated manually when needed
        
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
    if (m_headless || !m_renderWindow || !m_openGLRenderer) {
        return;
    }
    
    // Make sure we're rendering to the window
    m_renderWindow->makeContextCurrent();
    
    // Double check we have the right context
    GLint currentFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    // Debug: framebuffer check - commented out to reduce spam
    // std::cout << "Current framebuffer: " << currentFBO << std::endl;
    
    // Make absolutely sure we're drawing to the default framebuffer
    if (currentFBO != 0) {
        std::cerr << "WARNING: Not drawing to default framebuffer!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    // Clear the screen
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set viewport to window size
    glViewport(0, 0, m_renderWindow->getWidth(), m_renderWindow->getHeight());
    
    // TEST RENDERING DISABLED - immediate mode rendering confirmed working
    // Continue to voxel rendering below
    
    // TEST TRIANGLE REMOVED - proceeding to voxel rendering
    
    // Render voxels if we have any
    if (m_voxelShaderProgram && m_voxelIndexCount > 0) {
        glUseProgram(m_voxelShaderProgram);
        
        // Set uniforms
        auto camera = m_cameraController->getCamera();
        auto viewMatrix = camera->getViewMatrix();
        auto projMatrix = camera->getProjectionMatrix();
        
        // Debug: Print matrices on first few frames
        static int frameCount = 0;
        if (frameCount < 3) {
            std::cout << "\n=== Frame " << frameCount << " Debug Info ===" << std::endl;
            std::cout << "Camera position: " << camera->getPosition().x << ", " 
                      << camera->getPosition().y << ", " << camera->getPosition().z << std::endl;
            std::cout << "View Matrix:" << std::endl;
            for (int i = 0; i < 4; i++) {
                std::cout << "  [";
                for (int j = 0; j < 4; j++) {
                    std::cout << viewMatrix.m[i * 4 + j] << " ";
                }
                std::cout << "]" << std::endl;
            }
            std::cout << "Projection Matrix:" << std::endl;
            for (int i = 0; i < 4; i++) {
                std::cout << "  [";
                for (int j = 0; j < 4; j++) {
                    std::cout << projMatrix.m[i * 4 + j] << " ";
                }
                std::cout << "]" << std::endl;
            }
            std::cout << "Rendering " << m_voxelIndexCount << " indices" << std::endl;
            frameCount++;
        }
        
        // Set matrices
        GLint modelLoc = glGetUniformLocation(m_voxelShaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(m_voxelShaderProgram, "view");
        GLint projLoc = glGetUniformLocation(m_voxelShaderProgram, "projection");
        
        // Debug: print camera info
        auto camPos = camera->getPosition();
        if (frameCount % 60 == 0) { // Print every 60 frames
            std::cout << "Camera pos: (" << camPos.x << ", " << camPos.y << ", " << camPos.z << ")" << std::endl;
            std::cout << "View matrix: " << viewMatrix.m[12] << ", " << viewMatrix.m[13] << ", " << viewMatrix.m[14] << std::endl;
        }
        
        Math::Matrix4f identity;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, identity.m);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix.m);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix.m);
        
        // Don't set light uniforms since we're not using them in the simplified shader
        
        // Clear any previous errors
        while (glGetError() != GL_NO_ERROR) {}
        
        // Check if shader program is active
        GLint currentProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        std::cout << "Current shader program: " << currentProgram << " (expected " << m_voxelShaderProgram << ")" << std::endl;
        
        // Check viewport
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        std::cout << "Viewport: " << viewport[0] << ", " << viewport[1] << ", " << viewport[2] << ", " << viewport[3] << std::endl;
        
        // Draw voxels
        if (m_voxelVAO != 0 && glBindVertexArray) {
            glBindVertexArray(m_voxelVAO);
        } else {
            // Manual vertex setup for OpenGL 2.1 without VAOs
            std::cout << "Using manual vertex setup (no VAO)" << std::endl;
            glBindBuffer(GL_ARRAY_BUFFER, m_voxelVBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_voxelEBO);
            
            // Get attribute locations
            GLint posLoc = glGetAttribLocation(m_voxelShaderProgram, "aPos");
            GLint normalLoc = glGetAttribLocation(m_voxelShaderProgram, "aNormal");
            GLint colorLoc = glGetAttribLocation(m_voxelShaderProgram, "aColor");
            
            std::cout << "Attribute locations: pos=" << posLoc << " normal=" << normalLoc << " color=" << colorLoc << std::endl;
            std::cout << "VBO=" << m_voxelVBO << " EBO=" << m_voxelEBO << std::endl;
            
            // Setup vertex attributes
            if (posLoc >= 0) {
                glEnableVertexAttribArray(posLoc);
                glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex), (void*)offsetof(Rendering::Vertex, position));
            }
            if (normalLoc >= 0) {
                glEnableVertexAttribArray(normalLoc);
                glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex), (void*)offsetof(Rendering::Vertex, normal));
            }
            if (colorLoc >= 0) {
                glEnableVertexAttribArray(colorLoc);
                glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex), (void*)offsetof(Rendering::Vertex, color));
            }
        }
        
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "Error after binding VAO: " << err << std::endl;
        }
        
        // Disable depth testing and culling for debugging
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        
        // Draw all triangles
        std::cout << "Drawing " << m_voxelIndexCount << " indices with glDrawElements" << std::endl;
        
        // Additional state checks before drawing
        GLint currentVBO = 0, currentEBO = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &currentVBO);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &currentEBO);
        std::cout << "  Current VBO: " << currentVBO << " (expected " << m_voxelVBO << ")" << std::endl;
        std::cout << "  Current EBO: " << currentEBO << " (expected " << m_voxelEBO << ")" << std::endl;
        
        // Check if attributes are enabled
        GLint posLoc = glGetAttribLocation(m_voxelShaderProgram, "aPos");
        GLint posEnabled = 0;
        glGetVertexAttribiv(posLoc, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &posEnabled);
        std::cout << "  Position attribute enabled: " << posEnabled << std::endl;
        
        glDrawElements(GL_TRIANGLES, m_voxelIndexCount, GL_UNSIGNED_INT, 0);
        
        // Check for OpenGL errors
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after draw: " << err << std::endl;
            
            // Additional debugging
            GLint vao = 0;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
            std::cerr << "  Current VAO: " << vao << " (expected " << m_voxelVAO << ")" << std::endl;
            
            GLint elementBuffer = 0;
            glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementBuffer);
            std::cerr << "  Current element buffer: " << elementBuffer << std::endl;
        }
        
        if (m_voxelVAO != 0 && glBindVertexArray) {
            glBindVertexArray(0);
        } else {
            // Disable vertex attributes if not using VAO
            GLint posLoc = glGetAttribLocation(m_voxelShaderProgram, "aPos");
            GLint normalLoc = glGetAttribLocation(m_voxelShaderProgram, "aNormal");
            GLint colorLoc = glGetAttribLocation(m_voxelShaderProgram, "aColor");
            
            if (posLoc >= 0) glDisableVertexAttribArray(posLoc);
            if (normalLoc >= 0) glDisableVertexAttribArray(normalLoc);
            if (colorLoc >= 0) glDisableVertexAttribArray(colorLoc);
        }
        
        glUseProgram(0);
    }
    
    // DEBUG: Test if ANY rendering works
    static bool debugRender = true;
    if (debugRender) {
        // Save current state
        GLint currentProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        
        glUseProgram(0);  // No shader
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(-1, 1, -1, 1, -1, 1);  // Simple orthographic projection
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        
        // Draw a colored pattern to verify rendering
        glBegin(GL_QUADS);
        // Red square top-left
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(-0.8f, 0.8f);
        glVertex2f(-0.4f, 0.8f);
        glVertex2f(-0.4f, 0.4f);
        glVertex2f(-0.8f, 0.4f);
        
        // Green square top-right
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex2f(0.4f, 0.8f);
        glVertex2f(0.8f, 0.8f);
        glVertex2f(0.8f, 0.4f);
        glVertex2f(0.4f, 0.4f);
        
        // Blue square bottom-left
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex2f(-0.8f, -0.4f);
        glVertex2f(-0.4f, -0.4f);
        glVertex2f(-0.4f, -0.8f);
        glVertex2f(-0.8f, -0.8f);
        
        // Yellow square bottom-right
        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex2f(0.4f, -0.4f);
        glVertex2f(0.8f, -0.4f);
        glVertex2f(0.8f, -0.8f);
        glVertex2f(0.4f, -0.8f);
        glEnd();
        
        // Restore state
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glUseProgram(currentProgram);
        
        // Print message once
        static bool printed = false;
        if (!printed) {
            std::cout << "DEBUG: Drawing colored squares pattern" << std::endl;
            printed = true;
        }
    }
    
    // TODO: Render visual feedback (selection highlights, etc)
    // m_feedbackRenderer->render(*m_cameraController->getCamera(), context);
}

bool Application::createShaderProgram() {
    std::cout << "Creating shader program..." << std::endl;
    
    // Vertex shader source - GLSL 120 for OpenGL 2.1
    const char* vertexShaderSource = R"(
#version 120
attribute vec3 aPos;
attribute vec3 aNormal;
attribute vec3 aColor;

varying vec3 FragPos;
varying vec3 Normal;
varying vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(model) * aNormal;
    Color = aColor;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

    // Fragment shader source - GLSL 120 for OpenGL 2.1
    const char* fragmentShaderSource = R"(
#version 120
varying vec3 FragPos;
varying vec3 Normal;
varying vec3 Color;

void main() {
    // Use the vertex color passed from vertex shader
    gl_FragColor = vec4(Color, 1.0);
}
)";

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    // Check vertex shader compilation
    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    glGetShaderInfoLog(vertexShader, 1024, nullptr, infoLog);
    if (!success) {
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        return false;
    } else if (strlen(infoLog) > 0) {
        std::cout << "Vertex shader info: " << infoLog << std::endl;
    }
    
    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    glGetShaderInfoLog(fragmentShader, 1024, nullptr, infoLog);
    if (!success) {
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    } else if (strlen(infoLog) > 0) {
        std::cout << "Fragment shader compilation warnings: " << infoLog << std::endl;
    } else {
        std::cout << "Fragment shader compiled successfully" << std::endl;
    }
    
    // Create shader program
    m_voxelShaderProgram = glCreateProgram();
    glAttachShader(m_voxelShaderProgram, vertexShader);
    glAttachShader(m_voxelShaderProgram, fragmentShader);
    glLinkProgram(m_voxelShaderProgram);
    
    // Check linking
    glGetProgramiv(m_voxelShaderProgram, GL_LINK_STATUS, &success);
    glGetProgramInfoLog(m_voxelShaderProgram, 1024, nullptr, infoLog);
    if (!success) {
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    } else if (strlen(infoLog) > 0) {
        std::cout << "Shader program linking warnings: " << infoLog << std::endl;
    } else {
        std::cout << "Shader program linked successfully" << std::endl;
    }
    
    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Validate all uniform locations
    std::cout << "Validating uniform locations..." << std::endl;
    GLint modelLoc = glGetUniformLocation(m_voxelShaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(m_voxelShaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_voxelShaderProgram, "projection");
    GLint lightPosLoc = glGetUniformLocation(m_voxelShaderProgram, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(m_voxelShaderProgram, "lightColor");
    GLint viewPosLoc = glGetUniformLocation(m_voxelShaderProgram, "viewPos");
    
    std::cout << "Uniform locations: " << std::endl;
    std::cout << "  model: " << modelLoc << std::endl;
    std::cout << "  view: " << viewLoc << std::endl;
    std::cout << "  projection: " << projLoc << std::endl;
    std::cout << "  lightPos: " << lightPosLoc << std::endl;
    std::cout << "  lightColor: " << lightColorLoc << std::endl;
    std::cout << "  viewPos: " << viewPosLoc << std::endl;
    
    if (modelLoc == -1 || viewLoc == -1 || projLoc == -1) {
        std::cerr << "ERROR: Critical uniform locations not found!" << std::endl;
        return false;
    }
    
    std::cout << "Shader program created successfully!" << std::endl;
    return true;
}

void Application::updateVoxelMesh() {
    if (m_headless || !m_meshGenerator || !m_voxelManager) {
        return;
    }
    
    // Generate mesh from voxel data
    auto mesh = m_meshGenerator->generateCubeMesh(*m_voxelManager);
    
    // Debug: Print mesh statistics
    std::cout << "Mesh update: " << mesh.vertices.size() << " vertices, " 
              << mesh.indices.size() << " indices" << std::endl;
    
    // Clean up old buffers
    if (m_voxelVAO && glDeleteVertexArrays) {
        glDeleteVertexArrays(1, &m_voxelVAO);
    }
    if (m_voxelVBO) {
        glDeleteBuffers(1, &m_voxelVBO);
    }
    if (m_voxelEBO) {
        glDeleteBuffers(1, &m_voxelEBO);
    }
    m_voxelVAO = 0;
    m_voxelVBO = 0;
    m_voxelEBO = 0;
    
    m_voxelIndexCount = mesh.indices.size();
    
    if (!mesh.vertices.empty()) {
        // Create vertex array object if supported
        if (glGenVertexArrays) {
            // Clear any previous errors
            while (glGetError() != GL_NO_ERROR) {}
            
            glGenVertexArrays(1, &m_voxelVAO);
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cerr << "Error creating VAO: " << err << std::endl;
            }
            
            if (m_voxelVAO != 0) {
                glBindVertexArray(m_voxelVAO);
                err = glGetError();
                if (err != GL_NO_ERROR) {
                    std::cerr << "Error binding VAO: " << err << std::endl;
                }
            }
            
            std::cout << "Created VAO: " << m_voxelVAO << std::endl;
        } else {
            std::cout << "VAOs not supported, using VBOs only" << std::endl;
            m_voxelVAO = 0;
        }
        
        glGenBuffers(1, &m_voxelVBO);
        glGenBuffers(1, &m_voxelEBO);
        
        // Clear any previous errors
        while (glGetError() != GL_NO_ERROR) {}
        
        // Upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, m_voxelVBO);
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "Error after glBindBuffer(VBO): " << err << std::endl;
        }
        
        glBufferData(GL_ARRAY_BUFFER, 
                     sizeof(Rendering::Vertex) * mesh.vertices.size(),
                     mesh.vertices.data(), 
                     GL_STATIC_DRAW);
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "Error after glBufferData(VBO): " << err << std::endl;
        }
        
        // Debug: Print some vertex data
        static int meshUpdateCount = 0;
        if (mesh.vertices.size() > 0 && meshUpdateCount++ < 5) {
            std::cout << "Uploading " << mesh.vertices.size() << " vertices to GPU:" << std::endl;
            for (size_t i = 0; i < std::min(size_t(3), mesh.vertices.size()); ++i) {
                const auto& v = mesh.vertices[i];
                std::cout << "  Vertex " << i << ": pos(" << v.position.x << ", " 
                         << v.position.y << ", " << v.position.z << ") color(" 
                         << v.color.r << ", " << v.color.g << ", " 
                         << v.color.b << ", " << v.color.a << ")" << std::endl;
            }
        }
        
        // Upload index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_voxelEBO);
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "Error after glBindBuffer(EBO): " << err << std::endl;
        }
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     sizeof(uint32_t) * mesh.indices.size(),
                     mesh.indices.data(),
                     GL_STATIC_DRAW);
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "Error after glBufferData(EBO): " << err << std::endl;
        }
        
        // Get attribute locations
        GLint posLoc = glGetAttribLocation(m_voxelShaderProgram, "aPos");
        GLint normalLoc = glGetAttribLocation(m_voxelShaderProgram, "aNormal");
        GLint colorLoc = glGetAttribLocation(m_voxelShaderProgram, "aColor");
        
        // Set vertex attributes
        // Position
        if (posLoc >= 0) {
            glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex),
                                 (void*)offsetof(Rendering::Vertex, position));
            glEnableVertexAttribArray(posLoc);
        }
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "Error after position attribute: " << err << std::endl;
        }
        
        // Normal
        if (normalLoc >= 0) {
            glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex),
                                 (void*)offsetof(Rendering::Vertex, normal));
            glEnableVertexAttribArray(normalLoc);
        }
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "Error after normal attribute: " << err << std::endl;
        }
        
        // Color (vertex has vec4, shader expects vec3)
        if (colorLoc >= 0) {
            glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex),
                                 (void*)offsetof(Rendering::Vertex, color));
            glEnableVertexAttribArray(colorLoc);
        }
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "Error after color attribute: " << err << std::endl;
        }
        
        // Debug: print vertex attribute offsets
        if (meshUpdateCount == 1) {
            std::cout << "Vertex attribute offsets:" << std::endl;
            std::cout << "  sizeof(Vertex): " << sizeof(Rendering::Vertex) << std::endl;
            std::cout << "  position offset: " << offsetof(Rendering::Vertex, position) << std::endl;
            std::cout << "  normal offset: " << offsetof(Rendering::Vertex, normal) << std::endl;
            std::cout << "  texCoords offset: " << offsetof(Rendering::Vertex, texCoords) << std::endl;
            std::cout << "  color offset: " << offsetof(Rendering::Vertex, color) << std::endl;
            
            // Check a transformed vertex position
            auto camera = m_cameraController->getCamera();
            auto viewMatrix = camera->getViewMatrix();
            auto projMatrix = camera->getProjectionMatrix();
            Math::Matrix4f mvp = projMatrix * viewMatrix;
            
            // Debug camera parameters
            std::cout << "\nCamera Debug Info:" << std::endl;
            auto pos = camera->getPosition();
            auto target = camera->getTarget();
            std::cout << "  Camera Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
            std::cout << "  Camera Target: (" << target.x << ", " << target.y << ", " << target.z << ")" << std::endl;
            // Note: FOV getter not available in base class, using default 45°
            std::cout << "  Camera Near: " << camera->getNearPlane() << std::endl;
            std::cout << "  Camera Far: " << camera->getFarPlane() << std::endl;
            std::cout << "  Camera Aspect: " << camera->getAspectRatio() << std::endl;
            
            // Transform all vertices and check if they're in frustum
            int inFrustum = 0;
            for (size_t i = 0; i < mesh.vertices.size(); i++) {
                Math::Vector4f v(mesh.vertices[i].position.x, mesh.vertices[i].position.y, mesh.vertices[i].position.z, 1.0f);
                Math::Vector4f transformed = mvp * v;
                Math::Vector4f ndc = transformed / transformed.w;
                
                if (i == 0) {
                    std::cout << "\nFirst vertex transformation:" << std::endl;
                    std::cout << "  World: (" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
                    std::cout << "  Clip: (" << transformed.x << ", " << transformed.y << ", " << transformed.z << ", " << transformed.w << ")" << std::endl;
                    std::cout << "  NDC: (" << ndc.x << ", " << ndc.y << ", " << ndc.z << ")" << std::endl;
                }
                
                // Check if in frustum (-1 to 1 in NDC)
                if (ndc.x >= -1.0f && ndc.x <= 1.0f &&
                    ndc.y >= -1.0f && ndc.y <= 1.0f &&
                    ndc.z >= -1.0f && ndc.z <= 1.0f) {
                    inFrustum++;
                }
            }
            
            std::cout << "Vertices in frustum: " << inFrustum << "/" << mesh.vertices.size() << std::endl;
        }
        
        glBindVertexArray(0);
    }
}

void Application::cleanupGL() {
    if (m_headless) {
        return;
    }
    
    if (m_voxelVAO && glDeleteVertexArrays) {
        glDeleteVertexArrays(1, &m_voxelVAO);
    }
    if (m_voxelVBO) {
        glDeleteBuffers(1, &m_voxelVBO);
    }
    if (m_voxelEBO) {
        glDeleteBuffers(1, &m_voxelEBO);
    }
    m_voxelVAO = 0;
    m_voxelVBO = 0;
    m_voxelEBO = 0;
    
    if (m_voxelShaderProgram) {
        glDeleteProgram(m_voxelShaderProgram);
    }
}

} // namespace VoxelEditor
