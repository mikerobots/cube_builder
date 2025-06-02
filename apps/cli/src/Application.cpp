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

namespace VoxelEditor {

Application::Application() = default;

Application::~Application() {
    cleanupGL();
}

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
        m_meshGenerator = std::make_unique<VoxelMeshGenerator>();
        
        // Create shader program for voxel rendering
        if (!createShaderProgram()) {
            std::cerr << "Failed to create shader program" << std::endl;
            return false;
        }
        
        // Create initial mesh (empty)
        updateVoxelMesh();
        
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
    if (!m_renderWindow || !m_openGLRenderer) {
        return;
    }
    
    // Make sure we're rendering to the window
    m_renderWindow->makeContextCurrent();
    
    // Clear the screen with a dark gray background
    Rendering::Color clearColor(0.2f, 0.2f, 0.2f, 1.0f);
    m_openGLRenderer->clear(Rendering::ClearFlags::COLOR | Rendering::ClearFlags::DEPTH, clearColor);
    
    // Set viewport to window size
    m_openGLRenderer->setViewport(0, 0, m_renderWindow->getWidth(), m_renderWindow->getHeight());
    
    // Render voxels if we have any
    if (m_voxelShaderProgram && m_voxelIndexCount > 0) {
        glUseProgram(m_voxelShaderProgram);
        
        // Set uniforms
        auto camera = m_cameraController->getCamera();
        auto viewMatrix = camera->getViewMatrix();
        auto projMatrix = camera->getProjectionMatrix();
        
        // Set matrices
        GLint modelLoc = glGetUniformLocation(m_voxelShaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(m_voxelShaderProgram, "view");
        GLint projLoc = glGetUniformLocation(m_voxelShaderProgram, "projection");
        
        Math::Matrix4f identity;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, identity.m);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix.m);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix.m);
        
        // Set light properties
        GLint lightPosLoc = glGetUniformLocation(m_voxelShaderProgram, "lightPos");
        GLint lightColorLoc = glGetUniformLocation(m_voxelShaderProgram, "lightColor");
        GLint viewPosLoc = glGetUniformLocation(m_voxelShaderProgram, "viewPos");
        
        Math::Vector3f lightPos(5.0f, 10.0f, 5.0f);
        Math::Vector3f lightColor(1.0f, 1.0f, 1.0f);
        auto viewPos = camera->getPosition();
        
        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(lightColorLoc, lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(viewPosLoc, viewPos.x, viewPos.y, viewPos.z);
        
        // Draw voxels
        #ifdef __APPLE__
        glBindVertexArrayAPPLE(m_voxelVAO);
        #else
        glBindVertexArray(m_voxelVAO);
        #endif
        
        glDrawElements(GL_TRIANGLES, m_voxelIndexCount, GL_UNSIGNED_INT, 0);
        
        #ifdef __APPLE__
        glBindVertexArrayAPPLE(0);
        #else
        glBindVertexArray(0);
        #endif
        
        glUseProgram(0);
    }
    
    // TODO: Render visual feedback (selection highlights, etc)
    // m_feedbackRenderer->render(*m_cameraController->getCamera(), context);
}

bool Application::createShaderProgram() {
    // Vertex shader source
    const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

    // Fragment shader source
    const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 Color;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine results
    vec3 result = (ambient + diffuse + specular) * Color;
    FragColor = vec4(result, 1.0);
}
)";

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    // Check vertex shader compilation
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        return false;
    }
    
    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }
    
    // Create shader program
    m_voxelShaderProgram = glCreateProgram();
    glAttachShader(m_voxelShaderProgram, vertexShader);
    glAttachShader(m_voxelShaderProgram, fragmentShader);
    glLinkProgram(m_voxelShaderProgram);
    
    // Check linking
    glGetProgramiv(m_voxelShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_voxelShaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    
    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

void Application::updateVoxelMesh() {
    if (!m_meshGenerator || !m_voxelManager) {
        return;
    }
    
    // Generate mesh from voxel data
    auto mesh = m_meshGenerator->generateCubeMesh(*m_voxelManager);
    
    // Clean up old buffers
    if (m_voxelVAO) {
        #ifdef __APPLE__
        glDeleteVertexArraysAPPLE(1, &m_voxelVAO);
        #else
        glDeleteVertexArrays(1, &m_voxelVAO);
        #endif
        glDeleteBuffers(1, &m_voxelVBO);
        glDeleteBuffers(1, &m_voxelEBO);
    }
    
    m_voxelIndexCount = mesh.indices.size();
    
    if (!mesh.vertices.empty()) {
        // Create vertex array object
        #ifdef __APPLE__
        glGenVertexArraysAPPLE(1, &m_voxelVAO);
        glBindVertexArrayAPPLE(m_voxelVAO);
        #else
        glGenVertexArrays(1, &m_voxelVAO);
        glBindVertexArray(m_voxelVAO);
        #endif
        
        glGenBuffers(1, &m_voxelVBO);
        glGenBuffers(1, &m_voxelEBO);
        
        // Upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, m_voxelVBO);
        glBufferData(GL_ARRAY_BUFFER, 
                     sizeof(Rendering::Vertex) * mesh.vertices.size(),
                     mesh.vertices.data(), 
                     GL_STATIC_DRAW);
        
        // Upload index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_voxelEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     sizeof(uint32_t) * mesh.indices.size(),
                     mesh.indices.data(),
                     GL_STATIC_DRAW);
        
        // Set vertex attributes
        // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex),
                             (void*)offsetof(Rendering::Vertex, position));
        glEnableVertexAttribArray(0);
        
        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex),
                             (void*)offsetof(Rendering::Vertex, normal));
        glEnableVertexAttribArray(1);
        
        // Color
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex),
                             (void*)offsetof(Rendering::Vertex, color));
        glEnableVertexAttribArray(2);
        
        #ifdef __APPLE__
        glBindVertexArrayAPPLE(0);
        #else
        glBindVertexArray(0);
        #endif
    }
}

void Application::cleanupGL() {
    if (m_voxelVAO) {
        #ifdef __APPLE__
        glDeleteVertexArraysAPPLE(1, &m_voxelVAO);
        #else
        glDeleteVertexArrays(1, &m_voxelVAO);
        #endif
        glDeleteBuffers(1, &m_voxelVBO);
        glDeleteBuffers(1, &m_voxelEBO);
    }
    
    if (m_voxelShaderProgram) {
        glDeleteProgram(m_voxelShaderProgram);
    }
}

} // namespace VoxelEditor
