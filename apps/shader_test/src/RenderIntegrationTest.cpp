#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>

#include "rendering/RenderEngine.h"
#include "rendering/OpenGLRenderer.h"
#include "rendering/ShaderManager.h"
#include "camera/OrbitCamera.h"
#include "shader_test/TestMeshGenerator.h"
#include "math/Matrix4f.h"

// Add GL_SILENCE_DEPRECATION for macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

using namespace VoxelEditor;

class RenderIntegrationTest {
public:
    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        
        // Create window
        m_window = glfwCreateWindow(800, 600, "Render Integration Test", nullptr, nullptr);
        if (!m_window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(m_window);
        
        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            cleanup();
            return false;
        }
        
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        
        // Initialize rendering components
        m_renderEngine = std::make_unique<Rendering::RenderEngine>();
        Rendering::RenderConfig config;
        config.windowWidth = 800;
        config.windowHeight = 600;
        config.vsync = true;
        
        if (!m_renderEngine->initialize(config)) {
            std::cerr << "Failed to initialize RenderEngine" << std::endl;
            cleanup();
            return false;
        }
        
        // Set up camera
        m_camera = std::make_unique<Camera::OrbitCamera>();
        m_camera->setDistance(5.0f);
        m_camera->setPitch(-30.0f);
        m_camera->setYaw(45.0f);
        m_camera->setFieldOfView(45.0f);
        m_camera->setAspectRatio(800.0f / 600.0f);
        
        m_renderEngine->setCamera(*m_camera);
        m_renderEngine->setViewport(0, 0, 800, 600);
        
        return true;
    }
    
    void cleanup() {
        m_renderEngine.reset();
        m_camera.reset();
        
        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
    }
    
    bool testFileShaderRendering() {
        std::cout << "\n=== Testing File-based Shader Rendering ===" << std::endl;
        
        // Create test meshes
        auto cubeMesh = ShaderTest::TestMeshGenerator::createCube(1.0f, Math::Vector3f(0.7f, 0.7f, 0.7f));
        auto sphereMesh = ShaderTest::TestMeshGenerator::createSphere(0.8f, 32, 32);
        auto gridMesh = ShaderTest::TestMeshGenerator::createGrid(20, 0.5f, 5);
        
        // Convert to rendering meshes
        Rendering::Mesh renderCube = convertMesh(cubeMesh);
        Rendering::Mesh renderSphere = convertMesh(sphereMesh);
        Rendering::Mesh renderGrid = convertMesh(gridMesh);
        
        // Test different shaders
        std::vector<std::string> shaderNames = {"basic", "enhanced", "flat"};
        
        for (const auto& shaderName : shaderNames) {
            std::cout << "\nTesting " << shaderName << " shader..." << std::endl;
            
            // Clear screen
            m_renderEngine->clear(Rendering::ClearFlags::All);
            
            // Set shader
            Rendering::ShaderId shaderId = m_renderEngine->getBuiltinShader(shaderName);
            if (shaderId == Rendering::InvalidId) {
                std::cerr << "Failed to get " << shaderName << " shader" << std::endl;
                continue;
            }
            
            // Render cube
            Rendering::Transform cubeTransform;
            cubeTransform.position = Math::Vector3f(-1.5f, 0.0f, 0.0f);
            
            Rendering::Material cubeMaterial;
            cubeMaterial.shader = shaderId;
            cubeMaterial.albedo = Rendering::Color(0.8f, 0.2f, 0.2f, 1.0f);
            
            m_renderEngine->renderMesh(renderCube, cubeTransform, cubeMaterial);
            
            // Render sphere
            Rendering::Transform sphereTransform;
            sphereTransform.position = Math::Vector3f(1.5f, 0.0f, 0.0f);
            
            Rendering::Material sphereMaterial;
            sphereMaterial.shader = shaderId;
            sphereMaterial.albedo = Rendering::Color(0.2f, 0.8f, 0.2f, 1.0f);
            
            m_renderEngine->renderMesh(renderSphere, sphereTransform, sphereMaterial);
            
            // Render grid (only with basic shader)
            if (shaderName == "basic") {
                Rendering::Transform gridTransform;
                gridTransform.position = Math::Vector3f(0.0f, -1.0f, 0.0f);
                
                Rendering::Material gridMaterial;
                gridMaterial.shader = shaderId;
                gridMaterial.albedo = Rendering::Color(0.5f, 0.5f, 0.5f, 0.5f);
                
                // Note: Grid should be rendered as lines, but renderMesh uses triangles
                // For proper grid rendering, we'd need renderMeshAsLines
                m_renderEngine->renderMeshAsLines(renderGrid, gridTransform, gridMaterial);
            }
            
            // Capture screenshot
            std::string filename = "test_output/integration_" + shaderName + ".ppm";
            m_renderEngine->captureFrame(filename);
            std::cout << "Captured: " << filename << std::endl;
            
            // Swap buffers to display
            glfwSwapBuffers(m_window);
            glfwPollEvents();
            
            // Wait a bit to see the result
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        // Test error handling - try to use non-existent shader
        std::cout << "\nTesting error handling with non-existent shader..." << std::endl;
        Rendering::ShaderId invalidShaderId = m_renderEngine->getBuiltinShader("non_existent");
        if (invalidShaderId == Rendering::InvalidId) {
            std::cout << "✅ Correctly returned InvalidId for non-existent shader" << std::endl;
        } else {
            std::cout << "❌ Error: Got valid ID for non-existent shader" << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool testAnimatedRendering() {
        std::cout << "\n=== Testing Animated Rendering ===" << std::endl;
        
        // Create a cluster of voxels
        auto voxelCluster = ShaderTest::TestMeshGenerator::createVoxelCluster(3, 3, 3, 0.3f, 0.1f);
        Rendering::Mesh renderCluster = convertMesh(voxelCluster);
        
        // Get enhanced shader
        Rendering::ShaderId shaderId = m_renderEngine->getBuiltinShader("enhanced");
        if (shaderId == Rendering::InvalidId) {
            std::cerr << "Failed to get enhanced shader" << std::endl;
            return false;
        }
        
        // Animate for 3 seconds
        auto startTime = std::chrono::high_resolution_clock::now();
        float animationDuration = 3.0f;
        
        std::cout << "Animating for " << animationDuration << " seconds..." << std::endl;
        
        while (true) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(currentTime - startTime).count();
            
            if (elapsed > animationDuration) break;
            
            // Clear screen
            m_renderEngine->clear(Rendering::ClearFlags::All);
            
            // Update camera rotation
            float yaw = 45.0f + elapsed * 30.0f; // 30 degrees per second
            m_camera->setYaw(yaw);
            m_renderEngine->setCamera(*m_camera);
            
            // Render voxel cluster
            Rendering::Transform transform;
            transform.rotation = Math::Vector3f(0.0f, elapsed * 45.0f, 0.0f); // Rotate on Y axis
            
            Rendering::Material material;
            material.shader = shaderId;
            material.albedo = Rendering::Color(0.7f, 0.7f, 0.7f, 1.0f);
            
            m_renderEngine->renderMesh(renderCluster, transform, material);
            
            // Swap buffers
            glfwSwapBuffers(m_window);
            glfwPollEvents();
            
            // Check for ESC key
            if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                std::cout << "Animation cancelled by user" << std::endl;
                break;
            }
        }
        
        // Capture final frame
        m_renderEngine->captureFrame("test_output/integration_animated_final.ppm");
        std::cout << "Captured final frame" << std::endl;
        
        return true;
    }
    
private:
    GLFWwindow* m_window = nullptr;
    std::unique_ptr<Rendering::RenderEngine> m_renderEngine;
    std::unique_ptr<Camera::OrbitCamera> m_camera;
    
    // Convert test mesh to rendering mesh
    Rendering::Mesh convertMesh(const Rendering::Mesh& testMesh) {
        Rendering::Mesh mesh;
        mesh.vertices = testMesh.vertices;
        mesh.indices = testMesh.indices;
        return mesh;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "=== Render Integration Test ===" << std::endl;
    std::cout << "This test validates shader loading from files and rendering with the full pipeline" << std::endl;
    
    RenderIntegrationTest test;
    
    if (!test.initialize()) {
        std::cerr << "Failed to initialize test" << std::endl;
        return 1;
    }
    
    // Create output directory
    system("mkdir -p test_output");
    
    bool success = true;
    
    // Run tests
    if (!test.testFileShaderRendering()) {
        std::cerr << "File shader rendering test failed" << std::endl;
        success = false;
    }
    
    if (!test.testAnimatedRendering()) {
        std::cerr << "Animated rendering test failed" << std::endl;
        success = false;
    }
    
    test.cleanup();
    
    if (success) {
        std::cout << "\n✅ All integration tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some integration tests failed!" << std::endl;
        return 1;
    }
}