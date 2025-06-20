#include <gtest/gtest.h>
#include "rendering/RenderEngine.h"
#include "rendering/RenderTypes.h"
#include "camera/CameraController.h"
#include "math/Matrix4f.h"
#include "math/Vector3f.h"
#include "math/Vector4f.h"
#include <memory>
#include <vector>
#include <string>

// Include OpenGL headers
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>

namespace VoxelEditor {
namespace Tests {

class ShaderRenderingComprehensiveTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Rendering::RenderEngine> renderEngine;
    std::unique_ptr<Camera::CameraController> cameraController;
    
    void SetUp() override {
        // Skip in CI environment
        if (std::getenv("CI") != nullptr) {
            GTEST_SKIP() << "Skipping OpenGL tests in CI environment";
        }
        
        // Initialize GLFW
        if (!glfwInit()) {
            GTEST_SKIP() << "Failed to initialize GLFW";
        }
        
        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
        
        window = glfwCreateWindow(800, 600, "Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            GTEST_SKIP() << "Failed to create GLFW window";
        }
        
        glfwMakeContextCurrent(window);
        
        // Create render engine
        renderEngine = std::make_unique<Rendering::RenderEngine>(nullptr);
        Rendering::RenderConfig config;
        config.windowWidth = 800;
        config.windowHeight = 600;
        
        if (!renderEngine->initialize(config)) {
            GTEST_SKIP() << "Failed to initialize render engine";
        }
        
        // Create camera controller
        cameraController = std::make_unique<Camera::CameraController>(nullptr);
        cameraController->setViewPreset(Camera::ViewPreset::ISOMETRIC);
    }
    
    void TearDown() override {
        cameraController.reset();
        renderEngine.reset();
        
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        glfwTerminate();
    }
    
    // Helper to create a test mesh
    Rendering::Mesh createTestMesh() {
        Rendering::Mesh mesh;
        
        // Simple triangle
        Rendering::Vertex v1(Math::Vector3f(0.0f, 0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                            Math::Vector2f(0.5f, 1.0f), Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f));
        Rendering::Vertex v2(Math::Vector3f(-0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                            Math::Vector2f(0.0f, 0.0f), Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f));
        Rendering::Vertex v3(Math::Vector3f(0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                            Math::Vector2f(1.0f, 0.0f), Rendering::Color(0.0f, 0.0f, 1.0f, 1.0f));
        
        mesh.vertices = {v1, v2, v3};
        mesh.indices = {0, 1, 2};
        
        return mesh;
    }
    
    // Helper to check for OpenGL errors
    bool checkGLError(const std::string& context) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "GL Error in " << context << ": " << error << std::endl;
            return false;
        }
        return true;
    }
};

// Test that all shaders compile and render without GL errors
TEST_F(ShaderRenderingComprehensiveTest, AllShadersRenderWithoutErrors) {
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    
    renderEngine->setCamera(*cameraController->getCamera());
    
    std::vector<std::string> shaderNames = {"basic", "enhanced", "flat"};
    
    for (const auto& shaderName : shaderNames) {
        // Clear any existing errors
        while (glGetError() != GL_NO_ERROR) {}
        
        auto shaderId = renderEngine->getBuiltinShader(shaderName);
        EXPECT_NE(shaderId, Rendering::InvalidId) << "Failed to get " << shaderName << " shader";
        
        Rendering::Transform transform;
        Rendering::Material material;
        material.shader = shaderId;
        material.albedo = Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f);
        
        renderEngine->beginFrame();
        renderEngine->clear();
        renderEngine->renderMesh(mesh, transform, material);
        renderEngine->endFrame();
        
        EXPECT_TRUE(checkGLError("Render with " + shaderName + " shader"));
    }
}

// Test VAO attribute setup
TEST_F(ShaderRenderingComprehensiveTest, VAOAttributesProperlyConfigured) {
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    
    // Verify VAO was created
    EXPECT_NE(mesh.vertexArray, 0u);
    EXPECT_NE(mesh.vertexBuffer, 0u);
    EXPECT_NE(mesh.indexBuffer, 0u);
    
    // Check that only attributes 0, 1, 2 are enabled (not 3 for texcoords)
    glBindVertexArray(mesh.vertexArray);
    
    GLint enabled;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE) << "Position attribute should be enabled";
    
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE) << "Normal attribute should be enabled";
    
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE) << "Color attribute should be enabled";
    
    glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_FALSE) << "TexCoord attribute should NOT be enabled";
    
    glBindVertexArray(0);
    
    EXPECT_TRUE(checkGLError("VAO attribute check"));
}

// Test rendering multiple meshes
TEST_F(ShaderRenderingComprehensiveTest, MultipleMeshRendering) {
    renderEngine->setCamera(*cameraController->getCamera());
    
    // Create multiple meshes
    std::vector<Rendering::Mesh> meshes;
    for (int i = 0; i < 5; ++i) {
        auto mesh = createTestMesh();
        renderEngine->setupMeshBuffers(mesh);
        meshes.push_back(mesh);
    }
    
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    
    // Render each mesh with different shaders and positions
    std::vector<std::string> shaderNames = {"basic", "enhanced", "flat", "basic", "enhanced"};
    for (size_t i = 0; i < meshes.size(); ++i) {
        Rendering::Transform transform;
        transform.position = Math::WorldCoordinates(Math::Vector3f(i * 0.5f - 1.0f, 0.0f, 0.0f));
        
        Rendering::Material material;
        material.shader = renderEngine->getBuiltinShader(shaderNames[i % shaderNames.size()]);
        material.albedo = Rendering::Color(1.0f, 0.5f, 0.2f, 1.0f);
        
        renderEngine->renderMesh(meshes[i], transform, material);
    }
    
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("Multiple mesh rendering"));
}

// Test line rendering
TEST_F(ShaderRenderingComprehensiveTest, LineRenderingMode) {
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    
    renderEngine->setCamera(*cameraController->getCamera());
    
    Rendering::Transform transform;
    Rendering::Material material;
    material.shader = renderEngine->getBuiltinShader("basic");
    material.albedo = Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f);
    
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->setLineWidth(2.0f);
    renderEngine->renderMeshAsLines(mesh, transform, material);
    renderEngine->setLineWidth(1.0f);
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("Line rendering"));
}

// Test render state changes
TEST_F(ShaderRenderingComprehensiveTest, RenderStateManagement) {
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    renderEngine->setCamera(*cameraController->getCamera());
    
    // Test different render states
    renderEngine->beginFrame();
    
    // Test depth testing
    renderEngine->setDepthTest(false);
    renderEngine->renderMesh(mesh, Rendering::Transform(), 
                           Rendering::Material::createDefault());
    EXPECT_TRUE(checkGLError("Render without depth test"));
    
    renderEngine->setDepthTest(true);
    renderEngine->renderMesh(mesh, Rendering::Transform(), 
                           Rendering::Material::createDefault());
    EXPECT_TRUE(checkGLError("Render with depth test"));
    
    // Test culling
    renderEngine->setCullMode(Rendering::CullMode::None);
    renderEngine->renderMesh(mesh, Rendering::Transform(), 
                           Rendering::Material::createDefault());
    EXPECT_TRUE(checkGLError("Render without culling"));
    
    renderEngine->setCullMode(Rendering::CullMode::Back);
    renderEngine->renderMesh(mesh, Rendering::Transform(), 
                           Rendering::Material::createDefault());
    EXPECT_TRUE(checkGLError("Render with back culling"));
    
    // Test blending
    renderEngine->setBlendMode(Rendering::BlendMode::Alpha);
    renderEngine->renderMesh(mesh, Rendering::Transform(), 
                           Rendering::Material::createDefault());
    EXPECT_TRUE(checkGLError("Render with alpha blending"));
    
    renderEngine->setBlendMode(Rendering::BlendMode::Opaque);
    renderEngine->renderMesh(mesh, Rendering::Transform(), 
                           Rendering::Material::createDefault());
    EXPECT_TRUE(checkGLError("Render opaque"));
    
    renderEngine->endFrame();
}

// Test ground plane rendering
TEST_F(ShaderRenderingComprehensiveTest, GroundPlaneRendering) {
    renderEngine->setCamera(*cameraController->getCamera());
    
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->setGroundPlaneGridVisible(true);
    renderEngine->updateGroundPlaneGrid(Math::Vector3f(10.0f, 10.0f, 10.0f));
    renderEngine->renderGroundPlaneGrid(Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f)));
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("Ground plane rendering"));
}

// Test empty scene rendering
TEST_F(ShaderRenderingComprehensiveTest, EmptySceneRendering) {
    renderEngine->setCamera(*cameraController->getCamera());
    
    while (glGetError() != GL_NO_ERROR) {}
    
    // Should handle empty scene gracefully
    renderEngine->beginFrame();
    renderEngine->clear(Rendering::ClearFlags::All, Rendering::Color(0.2f, 0.3f, 0.4f, 1.0f));
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("Empty scene rendering"));
}

// Test rapid shader switching
TEST_F(ShaderRenderingComprehensiveTest, RapidShaderSwitching) {
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    renderEngine->setCamera(*cameraController->getCamera());
    
    std::vector<std::string> shaderNames = {"basic", "enhanced", "flat"};
    
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    
    // Rapidly switch between shaders
    for (int i = 0; i < 30; ++i) {
        Rendering::Transform transform;
        transform.position = Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f));
        transform.rotation = Math::Vector3f(0.0f, i * 12.0f, 0.0f);
        
        Rendering::Material material;
        material.shader = renderEngine->getBuiltinShader(shaderNames[i % shaderNames.size()]);
        material.albedo = Rendering::Color(1.0f, 0.5f, 0.2f, 1.0f);
        
        renderEngine->renderMesh(mesh, transform, material);
    }
    
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("Rapid shader switching"));
}

} // namespace Tests
} // namespace VoxelEditor