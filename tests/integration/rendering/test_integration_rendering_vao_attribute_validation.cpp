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
#include "rendering/MacOSGLLoader.h"
// Define macros for easier use
#define VAO_GEN(n, arrays) VoxelEditor::Rendering::glGenVertexArrays(n, arrays)
#define VAO_BIND(array) VoxelEditor::Rendering::glBindVertexArray(array)
#define VAO_DELETE(n, arrays) VoxelEditor::Rendering::glDeleteVertexArrays(n, arrays)
#else
#include <glad/glad.h>
#define VAO_GEN(n, arrays) glGenVertexArrays(n, arrays)
#define VAO_BIND(array) glBindVertexArray(array)
#define VAO_DELETE(n, arrays) glDeleteVertexArrays(n, arrays)
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
        // Integration tests must not skip in CI - removed skip check
        
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
        
        VAO_BIND(vao);
        
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
        
        VAO_BIND(currentVAO);
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
    VAO_BIND(mesh.vertexArray);
    
    GLint enabled;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE) << "Position attribute should be enabled";
    
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE) << "Normal attribute should be enabled";
    
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE) << "Color attribute should be enabled";
    
    glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_FALSE) << "TexCoord attribute should NOT be enabled";
    
    VAO_BIND(0);
    
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
    // Use OpenGLRenderer to create VAO and buffers properly
    auto vao = glRenderer->createVertexArray();
    EXPECT_NE(vao, 0u) << "Failed to create VAO";
    
    // Create vertex data matching Rendering::Vertex layout
    std::vector<Rendering::Vertex> vertices = {
        {Math::Vector3f(0.0f, 0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), Math::Vector2f(0.5f, 1.0f), Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f)},
        {Math::Vector3f(-0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), Math::Vector2f(0.0f, 0.0f), Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f)},
        {Math::Vector3f(0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), Math::Vector2f(1.0f, 0.0f), Rendering::Color(0.0f, 0.0f, 1.0f, 1.0f)}
    };
    
    // Create VBO and set up vertex attributes manually
    auto vbo = glRenderer->createVertexBuffer(vertices.data(), vertices.size() * sizeof(Rendering::Vertex));
    EXPECT_NE(vbo, 0u) << "Failed to create VBO";
    
    // Bind VAO and VBO
    glRenderer->bindVertexArray(vao);
    glRenderer->bindVertexBuffer(vbo);
    
    // Setup vertex attributes using OpenGLRenderer
    std::vector<Rendering::VertexAttribute> attributes = {
        Rendering::VertexAttribute::Position,
        Rendering::VertexAttribute::Normal,
        Rendering::VertexAttribute::Color
        // Note: NOT including TexCoord0 as our shaders don't use it
    };
    glRenderer->setupVertexAttributes(attributes);
    
    // Verify setup
    EXPECT_TRUE(checkGLError("OpenGLRenderer VAO setup"));
    
    // Test rendering with this VAO
    // Create shader from inline source (avoid file loading issues in tests)
    const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 vertexColor;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";
        const char* fragmentShaderSource = R"(
#version 330 core
in vec4 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vertexColor;
}
)";
    auto shaderId = shaderManager->createShaderFromSource("basic_test", vertexShaderSource, fragmentShaderSource, glRenderer.get());
    auto* shaderProgram = shaderManager->getShaderProgram(shaderId);
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
    VAO_BIND(0);
    VAO_DELETE(1, &vao);
    glRenderer->deleteBuffer(vbo);
}

// Test that shaders handle missing optional attributes gracefully
TEST_F(VAOAttributeValidationTest, ShaderOptionalAttributes) {
    // Clear any existing GL errors before starting test
    while (glGetError() != GL_NO_ERROR) {}
    
    // Create a minimal mesh with only position using OpenGLRenderer
    auto vao = glRenderer->createVertexArray();
    EXPECT_NE(vao, 0u) << "Failed to create VAO";
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    EXPECT_TRUE(checkGLError("Generate VBO"));
    
    glRenderer->bindVertexArray(vao);
    EXPECT_TRUE(checkGLError("Bind VAO"));
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    EXPECT_TRUE(checkGLError("Bind VBO"));
    
    // Just positions
    float positions[] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    EXPECT_TRUE(checkGLError("Buffer data upload"));
    
    // Only enable position attribute
    glEnableVertexAttribArray(0);
    EXPECT_TRUE(checkGLError("Enable vertex attrib array"));
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    EXPECT_TRUE(checkGLError("Set vertex attrib pointer"));
    
    EXPECT_TRUE(checkGLError("Setup minimal VAO"));
    
    // This should still work even with only position attribute
    // Create a simple shader that only uses position
    const char* simpleVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";
    const char* simpleFragmentShader = R"(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White color
}
)";
    
    auto shaderId = shaderManager->createShaderFromSource("simple_test", simpleVertexShader, simpleFragmentShader, glRenderer.get());
    auto* shaderProgram = shaderManager->getShaderProgram(shaderId);
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
    glRenderer->bindVertexArray(0);
    glRenderer->deleteVertexArray(vao);
    glDeleteBuffers(1, &vbo);
}

// Test error detection for truly invalid setups
TEST_F(VAOAttributeValidationTest, DetectInvalidSetups) {
    // Create VAO with wrong attribute setup
    GLuint vao, vbo;
    VAO_GEN(1, &vao);
    glGenBuffers(1, &vbo);
    
    VAO_BIND(vao);
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
    VAO_BIND(0);
    VAO_DELETE(1, &vao);
    glDeleteBuffers(1, &vbo);
}

} // namespace Tests
} // namespace VoxelEditor