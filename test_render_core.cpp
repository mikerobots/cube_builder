// Simple test program to verify core rendering functionality
#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>
#include "core/rendering/RenderEngine.h"
#include "core/rendering/OpenGLRenderer.h"
#include "core/camera/OrbitCamera.h"
#include "foundation/logging/Logger.h"

using namespace VoxelEditor;

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create window with OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Headless

    GLFWwindow* window = glfwCreateWindow(800, 600, "Render Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize rendering
    auto logger = &Logging::Logger::getInstance();
    logger->setLevel(Logging::Logger::Level::Debug);
    
    Rendering::RenderConfig config;
    config.enableDebugOutput = true;
    
    auto renderEngine = std::make_unique<Rendering::RenderEngine>();
    if (!renderEngine->initialize(config)) {
        std::cerr << "Failed to initialize RenderEngine" << std::endl;
        return -1;
    }

    // Create a simple triangle mesh
    Rendering::Mesh triangleMesh;
    
    // Vertices: position (3), normal (3), texcoord (2), color (4)
    triangleMesh.vertices = {
        // Vertex 0: bottom-left, red
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        // Vertex 1: bottom-right, green
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        // Vertex 2: top-center, blue
        {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}
    };
    
    triangleMesh.indices = {0, 1, 2};
    
    // Force mesh buffer setup
    std::cout << "Setting up mesh buffers..." << std::endl;
    
    // Setup camera
    auto camera = std::make_unique<Camera::OrbitCamera>();
    camera->setPosition(Math::Vector3f(0.0f, 0.0f, 3.0f));
    camera->setTarget(Math::Vector3f(0.0f, 0.0f, 0.0f));
    camera->setAspectRatio(800.0f / 600.0f);
    
    renderEngine->setCamera(*camera);
    renderEngine->setViewport(0, 0, 800, 600);

    // Setup transform and material for triangle
    Rendering::Transform transform;
    transform.position = Math::Vector3f(0, 0, 0);
    transform.rotation = Math::Vector3f(0, 0, 0);
    transform.scale = Math::Vector3f(1, 1, 1);
    
    Rendering::Material material = Rendering::Material::createDefault();
    material.albedo = Rendering::Color(1, 1, 1, 1);

    // Render a few frames
    for (int frame = 0; frame < 3; ++frame) {
        std::cout << "\n=== Frame " << frame << " ===" << std::endl;
        
        // Begin frame
        renderEngine->beginFrame();
        
        // Clear to dark blue
        renderEngine->clear(Rendering::ClearFlags::All, 
                           Rendering::Color(0.1f, 0.1f, 0.3f, 1.0f));
        
        // Render triangle
        std::cout << "Rendering triangle mesh..." << std::endl;
        renderEngine->renderMesh(triangleMesh, transform, material);
        
        // End frame
        renderEngine->endFrame();
        
        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Capture screenshot
    std::cout << "\nCapturing screenshot..." << std::endl;
    renderEngine->beginFrame();
    renderEngine->clear(Rendering::ClearFlags::All, 
                       Rendering::Color(0.1f, 0.1f, 0.3f, 1.0f));
    renderEngine->renderMesh(triangleMesh, transform, material);
    renderEngine->captureFrame("test_render_output.ppm");
    renderEngine->endFrame();

    // Get stats
    auto stats = renderEngine->getRenderStats();
    std::cout << "\nRender Stats:" << std::endl;
    std::cout << "  Draw calls: " << stats.drawCalls << std::endl;
    std::cout << "  Triangles: " << stats.trianglesRendered << std::endl;
    std::cout << "  Vertices: " << stats.verticesProcessed << std::endl;

    // Cleanup
    renderEngine->shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
}