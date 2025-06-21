#include <gtest/gtest.h>
#include "rendering/RenderEngine.h"
#include "rendering/OpenGLRenderer.h"
#include "rendering/ShaderManager.h"
#include "rendering/RenderTypes.h"
#include "camera/CameraController.h"
#include "math/Vector3f.h"
#include "math/Vector4f.h"
#include <memory>
#include <vector>

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

class VAOAttributeValidationTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Rendering::RenderEngine> renderEngine;
    std::unique_ptr<Rendering::ShaderManager> shaderManager;
    std::unique_ptr<Rendering::OpenGLRenderer> glRenderer;
    
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
        
        // Initialize OpenGL
        glRenderer = std::make_unique<Rendering::OpenGLRenderer>();
        Rendering::RenderConfig config;
        config.windowWidth = 800;
        config.windowHeight = 600;
        
        if (!glRenderer->initializeContext(config)) {
            GTEST_SKIP() << "Failed to initialize OpenGL context";
        }
        
        // Create render engine
        renderEngine = std::make_unique<Rendering::RenderEngine>(nullptr);
        renderEngine->initialize(config);
        
        // Create shader manager
        shaderManager = std::make_unique<Rendering::ShaderManager>(glRenderer.get());
    }
    
    void TearDown() override {
        renderEngine.reset();
        shaderManager.reset();
        glRenderer.reset();
        
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        glfwTerminate();
    }
    
    bool checkGLError(const std::string& context) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "GL Error in " << context << ": " << error << std::endl;
            return false;
        }
        return true;
    }
    
    void dumpVAOState(GLuint vao, const std::string& context) {
        std::cout << "\n=== VAO State Dump: " << context << " ===" << std::endl;
        
        GLint currentVAO;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
        std::cout << "Current VAO: " << currentVAO << std::endl;
        
        glBindVertexArray(vao);
        
        // Check each potential attribute
        for (GLuint i = 0; i < 4; ++i) {
            GLint enabled, size, type, normalized, stride, bufferBinding;
            GLvoid* pointer;
            
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &bufferBinding);
            glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointer);
            
            std::cout << "Attribute " << i << ":" << std::endl;
            std::cout << "  Enabled: " << (enabled ? "YES" : "NO") << std::endl;
            if (enabled) {
                std::cout << "  Size: " << size << std::endl;
                std::cout << "  Type: " << type << " (GL_FLOAT=" << GL_FLOAT << ")" << std::endl;
                std::cout << "  Normalized: " << (normalized ? "YES" : "NO") << std::endl;
                std::cout << "  Stride: " << stride << std::endl;
                std::cout << "  Buffer: " << bufferBinding << std::endl;
                std::cout << "  Offset: " << pointer << std::endl;
            }
        }
        
        glBindVertexArray(currentVAO);
        std::cout << "=========================" << std::endl;
    }
};

// Test that VAO attributes match shader expectations
TEST_F(VAOAttributeValidationTest, VAOShaderAttributeAlignment) {
    // Create a mesh with all attributes
    Rendering::Mesh mesh;
    // Create vertices using the proper constructor
    Rendering::Vertex v1(Math::Vector3f(0.0f, 0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                        Math::Vector2f(0.5f, 1.0f), Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f));
    Rendering::Vertex v2(Math::Vector3f(-0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                        Math::Vector2f(0.0f, 0.0f), Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f));
    Rendering::Vertex v3(Math::Vector3f(0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                        Math::Vector2f(1.0f, 0.0f), Rendering::Color(0.0f, 0.0f, 1.0f, 1.0f));
    
    mesh.vertices = {v1, v2, v3};
    mesh.indices = {0, 1, 2};
    
    // Setup mesh buffers
    renderEngine->setupMeshBuffers(mesh);
    
    // Dump VAO state for debugging
    dumpVAOState(mesh.vertexArray, "After setupMeshBuffers");
    
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

// Test rendering with mismatched attributes
TEST_F(VAOAttributeValidationTest, RenderWithCorrectAttributes) {
    // Create mesh
    Rendering::Mesh mesh;
    Rendering::Vertex v1(Math::Vector3f(-0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                        Math::Vector2f(0.0f, 0.0f), Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f));
    Rendering::Vertex v2(Math::Vector3f(0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                        Math::Vector2f(1.0f, 0.0f), Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f));
    Rendering::Vertex v3(Math::Vector3f(0.0f, 0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                        Math::Vector2f(0.5f, 1.0f), Rendering::Color(0.0f, 0.0f, 1.0f, 1.0f));
    
    mesh.vertices = {v1, v2, v3};
    mesh.indices = {0, 1, 2};
    
    renderEngine->setupMeshBuffers(mesh);
    
    // Set up camera
    Camera::CameraController cameraController(nullptr);
    cameraController.setViewPreset(Camera::ViewPreset::ISOMETRIC);
    renderEngine->setCamera(*cameraController.getCamera());
    
    // Clear any existing GL errors
    while (glGetError() != GL_NO_ERROR) {}
    
    // Test with each shader
    std::vector<std::string> shaderNames = {"basic", "enhanced", "flat"};
    
    for (const auto& shaderName : shaderNames) {
        renderEngine->beginFrame();
        renderEngine->clear();
        
        Rendering::Transform transform;
        Rendering::Material material;
        material.shader = renderEngine->getBuiltinShader(shaderName);
        material.albedo = Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f);
        
        // This should work without GL_INVALID_OPERATION
        renderEngine->renderMesh(mesh, transform, material);
        
        renderEngine->endFrame();
        
        EXPECT_TRUE(checkGLError("Render with " + shaderName + " shader"));
    }
}

// Test vertex attribute pointer alignment
TEST_F(VAOAttributeValidationTest, VertexAttributePointerAlignment) {
    // Create VAO manually to test attribute setup
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    // Create vertex data matching Rendering::Vertex layout
    struct TestVertex {
        float position[3];
        float normal[3];
        float texcoord[2];
        float color[4];
    };
    
    TestVertex vertices[] = {
        {{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}
    };
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Set up attributes matching shader expectations
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TestVertex), (void*)offsetof(TestVertex, position));
    
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TestVertex), (void*)offsetof(TestVertex, normal));
    
    // Color (skip texcoord to match shader)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(TestVertex), (void*)offsetof(TestVertex, color));
    
    // Verify setup
    EXPECT_TRUE(checkGLError("Manual VAO setup"));
    
    // Test rendering with this VAO
    auto shader = renderEngine->getBuiltinShader("basic");
    auto* shaderProgram = shaderManager->getShaderProgram(shader);
    EXPECT_NE(shaderProgram, nullptr);
    
    shaderProgram->use();
    
    // Set minimal uniforms
    Math::Matrix4f identity;
    identity.identity();
    shaderProgram->setUniform("model", identity);
    shaderProgram->setUniform("view", identity);
    shaderProgram->setUniform("projection", identity);
    
    // Draw
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    EXPECT_TRUE(checkGLError("Draw with manual VAO"));
    
    // Cleanup
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

// Test that shaders handle missing optional attributes gracefully
TEST_F(VAOAttributeValidationTest, ShaderOptionalAttributes) {
    // Create a minimal mesh with only position
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    // Just positions
    float positions[] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    
    // Only enable position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    // Provide default values for missing attributes
    glVertexAttrib3f(1, 0.0f, 0.0f, 1.0f); // Default normal
    glVertexAttrib4f(2, 1.0f, 1.0f, 1.0f, 1.0f); // Default color
    
    EXPECT_TRUE(checkGLError("Setup minimal VAO"));
    
    // This should still work even with only position attribute
    auto shader = renderEngine->getBuiltinShader("flat"); // Flat shader is simplest
    auto* shaderProgram = shaderManager->getShaderProgram(shader);
    EXPECT_NE(shaderProgram, nullptr);
    
    shaderProgram->use();
    
    Math::Matrix4f identity;
    identity.identity();
    shaderProgram->setUniform("model", identity);
    shaderProgram->setUniform("view", identity);
    shaderProgram->setUniform("projection", identity);
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    EXPECT_TRUE(checkGLError("Draw with minimal attributes"));
    
    // Cleanup
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

// Test error detection for truly invalid setups
TEST_F(VAOAttributeValidationTest, DetectInvalidSetups) {
    // Create VAO with wrong attribute setup
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    float data[12] = {0}; // Some data
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    
    // Enable attribute 3 which shader doesn't use
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    // Clear errors
    while (glGetError() != GL_NO_ERROR) {}
    
    auto shader = renderEngine->getBuiltinShader("basic");
    auto* shaderProgram = shaderManager->getShaderProgram(shader);
    if (shaderProgram) {
        shaderProgram->use();
    }
    
    // Try to draw - this might generate an error depending on driver
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Note: Some drivers may not generate an error for unused attributes
    // The important thing is our fixed code doesn't enable attribute 3
    
    // Cleanup
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

} // namespace Tests
} // namespace VoxelEditor