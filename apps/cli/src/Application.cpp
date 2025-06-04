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

// Platform-specific includes for non-blocking input
#ifdef _WIN32
#include <conio.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
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
    m_renderEngine.reset();
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
        Logging::Logger::getInstance().setLevel(Logging::LogLevel::Debug);
        
        // Clear any existing outputs
        Logging::Logger::getInstance().clearOutputs();
        
        // Add file output for ALL debug info (overwrite mode)
        auto fileOutput = std::make_unique<Logging::FileOutput>("voxel_debug.log", "DebugFile", false);
        Logging::Logger::getInstance().addOutput(std::move(fileOutput));
        
        // Only add console output if not in headless mode
        // Create a custom filtered console output that only shows Info and above
        if (!m_headless) {
            class FilteredConsoleOutput : public Logging::ConsoleOutput {
            public:
                FilteredConsoleOutput() : ConsoleOutput("FilteredConsole") {}
                
                void write(const Logging::LogMessage& message) override {
                    // Only write Info level and above to console
                    if (message.level >= Logging::LogLevel::Info) {
                        ConsoleOutput::write(message);
                    }
                }
            };
            
            Logging::Logger::getInstance().addOutput(
                std::make_unique<FilteredConsoleOutput>());
        }
        
        // Config is a singleton
        Config::ConfigManager::getInstance().setValue("workspace.size", 5.0f);
        Config::ConfigManager::getInstance().setValue("workspace.min", 2.0f);
        Config::ConfigManager::getInstance().setValue("workspace.max", 8.0f);
        Config::ConfigManager::getInstance().setValue("voxel.defaultResolution", static_cast<int>(VoxelData::VoxelResolution::Size_64cm));
        
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
        
        // Set the active resolution from config
        int defaultResolution = Config::ConfigManager::getInstance().getValue<int>("voxel.defaultResolution");
        m_voxelManager->setActiveResolution(static_cast<VoxelData::VoxelResolution>(defaultResolution));
        
        // Camera controller
        m_cameraController = std::make_unique<Camera::CameraController>(m_eventDispatcher.get());
        
        // Set camera to look at center of workspace
        m_cameraController->setViewPreset(Camera::ViewPreset::ISOMETRIC);
        
        // Get workspace size to center camera
        Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
        Math::Vector3f workspaceCenter = workspaceSize * 0.5f;
        
        m_cameraController->getCamera()->setTarget(workspaceCenter);
        
        // Set distance based on workspace size
        float maxDimension = std::max({workspaceSize.x, workspaceSize.y, workspaceSize.z});
        m_cameraController->getCamera()->setDistance(maxDimension * 1.5f);
        
        std::cout << "Camera set to view workspace center at (" 
                  << workspaceCenter.x << ", " << workspaceCenter.y << ", " << workspaceCenter.z << ")" << std::endl;
        
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
        
        // Create render engine
        m_renderEngine = std::make_unique<Rendering::RenderEngine>(m_eventDispatcher.get());
        if (!m_renderEngine->initialize(renderConfig)) {
            return false;
        }
        
        // Set render engine on window
        m_renderWindow->setRenderEngine(m_renderEngine.get());
        
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
            if (m_renderEngine) {
                m_renderEngine->setViewport(0, 0, width, height);
            }
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
            
            // The RenderEngine will handle all OpenGL initialization
            
            // Create initial scene
            createScene();
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
    static std::string input;
    static bool waitingForInput = true;
    
    if (waitingForInput) {
        std::cout << "> " << std::flush;
        waitingForInput = false;
    }
    
    // Non-blocking input check
    bool hasInput = false;
    
#ifdef _WIN32
    // Windows: Use _kbhit() to check for input
    if (_kbhit()) {
        hasInput = true;
    }
#else
    // Unix/Linux/macOS: Use select() to check for input
    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    int result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
    hasInput = (result > 0 && FD_ISSET(STDIN_FILENO, &readfds));
#endif
    
    // Only read input if available
    if (hasInput && std::getline(std::cin, input)) {
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
    if (m_headless || !m_renderWindow || !m_renderEngine) {
        return;
    }
    
    // Make sure we're rendering to the window
    m_renderWindow->makeContextCurrent();
    
    // Set camera for the render engine
    m_renderEngine->setCamera(*m_cameraController->getCamera());
    
    // Begin frame
    m_renderEngine->beginFrame();
    
    // Clear with grey background
    m_renderEngine->clear(Rendering::ClearFlags::All, Rendering::Color(0.3f, 0.3f, 0.3f, 1.0f));
    
    // Render all voxel meshes
    static int frameCount = 0;
    if (frameCount < 5) {
        Logging::Logger::getInstance().debugfc("Application", 
            "Rendering frame %d, mesh count: %zu", frameCount, m_voxelMeshes.size());
    }
    
    for (size_t i = 0; i < m_voxelMeshes.size(); ++i) {
        const auto& mesh = m_voxelMeshes[i];
        if (!mesh.vertices.empty()) {
            if (frameCount < 5) {
                Logging::Logger::getInstance().debugfc("Application",
                    "  Rendering mesh %zu with %zu vertices, %zu indices", 
                    i, mesh.vertices.size(), mesh.indices.size());
                
                // Show first few vertex positions to verify they're different
                for (size_t v = 0; v < std::min(size_t(3), mesh.vertices.size()); v += 24) {
                    Logging::Logger::getInstance().debugfc("Application",
                        "    Vertex %zu: pos(%.3f, %.3f, %.3f)", v,
                        mesh.vertices[v].position.x,
                        mesh.vertices[v].position.y,
                        mesh.vertices[v].position.z);
                }
            }
            
            // Create identity transform and basic material
            Rendering::Transform transform;
            Rendering::Material material;
            material.albedo = Rendering::Color(0.8f, 0.8f, 0.8f, 1.0f);
            
            // Use configured shader or default to enhanced
            if (m_defaultShaderId == Rendering::InvalidId) {
                m_defaultShaderId = m_renderEngine->getBuiltinShader("enhanced");
            }
            material.shader = m_defaultShaderId;
            
            if (frameCount < 5) {
                Logging::Logger::getInstance().debugfc("Application", "  Shader ID: %u", material.shader);
            }
            
            m_renderEngine->renderMesh(mesh, transform, material);
        }
    }
    
    // Render edge meshes as lines if enabled
    if (m_showEdges && !m_edgeMeshes.empty()) {
        // Set up for line rendering
        m_renderEngine->setLineWidth(2.0f);  // Thicker lines for visibility
        
        for (const auto& edgeMesh : m_edgeMeshes) {
            if (edgeMesh.vertices.empty()) continue;
            
            // Create material for edges
            Rendering::Transform transform;
            Rendering::Material edgeMaterial;
            edgeMaterial.albedo = Rendering::Color(0.1f, 0.1f, 0.1f, 1.0f);  // Dark edges
            edgeMaterial.shader = m_renderEngine->getBuiltinShader("basic");  // Use basic shader for lines
            edgeMaterial.doubleSided = true;
            
            // Use the proper render method for lines
            m_renderEngine->renderMeshAsLines(edgeMesh, transform, edgeMaterial);
        }
        
        // Reset line width
        m_renderEngine->setLineWidth(1.0f);
    }
    
    frameCount++;
    
    // Render visual feedback (highlights, outlines, previews)
    if (m_feedbackRenderer) {
        // The FeedbackRenderer will handle its own rendering calls internally
        // No need to explicitly call render methods since they're called from MouseInteraction
    }
    
    // End frame and present
    m_renderEngine->endFrame();
    
    m_renderEngine->present();
}

void Application::createScene() {
    std::cout << "Creating scene..." << std::endl;
    
    // Initialize empty voxel meshes
    updateVoxelMesh();
    
    std::cout << "Scene created successfully!" << std::endl;
}

void Application::updateVoxelMesh() {
    if (m_headless || !m_meshGenerator || !m_voxelManager) {
        return;
    }
    
    // Generate mesh from voxel data using the VoxelMeshGenerator
    auto generatedMesh = m_meshGenerator->generateCubeMesh(*m_voxelManager);
    
    // Debug: Print mesh statistics
    Logging::Logger::getInstance().debugfc("Application",
        "Mesh update: %zu vertices, %zu indices", 
        generatedMesh.vertices.size(), generatedMesh.indices.size());
    
    // Clear existing meshes
    m_voxelMeshes.clear();
    m_edgeMeshes.clear();
    
    // Convert VoxelMeshGenerator::Mesh to Rendering::Mesh if we have data
    if (!generatedMesh.vertices.empty()) {
        Rendering::Mesh renderMesh;
        
        // Convert vertices from generator format to rendering format
        renderMesh.vertices.reserve(generatedMesh.vertices.size());
        for (const auto& v : generatedMesh.vertices) {
            Rendering::Vertex renderVertex;
            renderVertex.position = v.position;
            renderVertex.normal = v.normal;
            renderVertex.texCoords = Math::Vector2f(0.0f, 0.0f); // No tex coords for now
            renderVertex.color = v.color;
            renderMesh.vertices.push_back(renderVertex);
        }
        
        // Copy indices
        renderMesh.indices = generatedMesh.indices;
        
        // Set up mesh for rendering (this will create GPU buffers)
        if (m_renderEngine) {
            m_renderEngine->setupMeshBuffers(renderMesh);
        }
        
        // Add to our mesh collection
        size_t vertexCount = renderMesh.vertices.size();
        m_voxelMeshes.push_back(std::move(renderMesh));
        
        Logging::Logger::getInstance().debugfc("Application",
            "Created render mesh with %zu vertices", vertexCount);
            
        // Generate edge mesh for wireframe overlay
        auto edgeMesh = m_meshGenerator->generateEdgeMesh(*m_voxelManager);
        if (!edgeMesh.vertices.empty()) {
            // Set up edge mesh for rendering
            if (m_renderEngine) {
                m_renderEngine->setupMeshBuffers(edgeMesh);
            }
            m_edgeMeshes.push_back(std::move(edgeMesh));
            
            Logging::Logger::getInstance().debugfc("Application",
                "Created edge mesh with %zu vertices", m_edgeMeshes.back().vertices.size());
        }
    }
}


} // namespace VoxelEditor
