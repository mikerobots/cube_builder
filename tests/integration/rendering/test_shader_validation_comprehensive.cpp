#include <gtest/gtest.h>
#include "rendering/RenderEngine.h"
#include "rendering/ShaderManager.h"
#include "rendering/OpenGLRenderer.h"
#include "rendering/RenderTypes.h"
#include "camera/Camera.h"
#include "camera/CameraController.h"
#include "math/Matrix4f.h"
#include "math/Vector3f.h"
#include "math/Vector4f.h"
#include <memory>
#include <vector>
#include <string>
#include <fstream>

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

class ShaderValidationComprehensiveTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Rendering::RenderEngine> renderEngine;
    std::unique_ptr<Rendering::OpenGLRenderer> glRenderer;
    std::unique_ptr<Rendering::ShaderManager> shaderManager;
    
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
        
        // Create shader manager
        shaderManager = std::make_unique<Rendering::ShaderManager>(glRenderer.get());
        
        // Create render engine
        renderEngine = std::make_unique<Rendering::RenderEngine>(nullptr);
        renderEngine->initialize(config);
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
    
    // Helper to validate shader compilation
    bool validateShaderCompilation(GLuint shader) {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        
        if (!success) {
            GLint logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            
            std::vector<char> log(logLength);
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            
            std::cerr << "Shader compilation failed: " << log.data() << std::endl;
            return false;
        }
        
        return true;
    }
    
    // Helper to validate program linking
    bool validateProgramLinking(GLuint program) {
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        
        if (!success) {
            GLint logLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            
            std::vector<char> log(logLength);
            glGetProgramInfoLog(program, logLength, nullptr, log.data());
            
            std::cerr << "Program linking failed: " << log.data() << std::endl;
            return false;
        }
        
        return true;
    }
    
    // Helper to validate vertex attributes
    bool validateVertexAttributes(GLuint program, const std::vector<std::string>& expectedAttributes) {
        for (const auto& attr : expectedAttributes) {
            GLint location = glGetAttribLocation(program, attr.c_str());
            if (location == -1) {
                std::cerr << "Attribute '" << attr << "' not found in shader" << std::endl;
                return false;
            }
        }
        return true;
    }
    
    // Helper to validate uniforms
    bool validateUniforms(GLuint program, const std::vector<std::string>& expectedUniforms) {
        for (const auto& uniform : expectedUniforms) {
            GLint location = glGetUniformLocation(program, uniform.c_str());
            if (location == -1) {
                std::cerr << "Uniform '" << uniform << "' not found in shader" << std::endl;
                return false;
            }
        }
        return true;
    }
};

// Test basic shader compilation and linking
TEST_F(ShaderValidationComprehensiveTest, BasicShaderValidation) {
    auto basicShader = renderEngine->getBuiltinShader("basic");
    EXPECT_NE(basicShader, Rendering::InvalidId);
    
    // Validate shader was created
    auto* program = shaderManager->getShaderProgram(basicShader);
    EXPECT_NE(program, nullptr);
    
    // Get the GL program ID through the program object
    GLint currentProgram = 0;
    program->use();
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    program->unuse();
    
    EXPECT_NE(currentProgram, 0);
    
    // Validate expected attributes
    std::vector<std::string> expectedAttrs = {"a_position", "a_normal", "a_color"};
    EXPECT_TRUE(validateVertexAttributes(currentProgram, expectedAttrs));
    
    // Validate expected uniforms
    std::vector<std::string> expectedUniforms = {"model", "view", "projection", "lightPos", "lightColor", "viewPos"};
    EXPECT_TRUE(validateUniforms(currentProgram, expectedUniforms));
}

// Test enhanced shader
TEST_F(ShaderValidationComprehensiveTest, EnhancedShaderValidation) {
    auto enhancedShader = renderEngine->getBuiltinShader("enhanced");
    EXPECT_NE(enhancedShader, Rendering::InvalidId);
    
    auto* program = shaderManager->getShaderProgram(enhancedShader);
    EXPECT_NE(program, nullptr);
    
    GLint currentProgram = 0;
    program->use();
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    program->unuse();
    
    EXPECT_NE(currentProgram, 0);
    
    std::vector<std::string> expectedAttrs = {"a_position", "a_normal", "a_color"};
    EXPECT_TRUE(validateVertexAttributes(currentProgram, expectedAttrs));
    
    std::vector<std::string> expectedUniforms = {"model", "view", "projection", "lightPos", "lightColor", "viewPos"};
    EXPECT_TRUE(validateUniforms(currentProgram, expectedUniforms));
}

// Test flat shader
TEST_F(ShaderValidationComprehensiveTest, FlatShaderValidation) {
    auto flatShader = renderEngine->getBuiltinShader("flat");
    EXPECT_NE(flatShader, Rendering::InvalidId);
    
    auto* program = shaderManager->getShaderProgram(flatShader);
    EXPECT_NE(program, nullptr);
    
    GLint currentProgram = 0;
    program->use();
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    program->unuse();
    
    EXPECT_NE(currentProgram, 0);
    
    std::vector<std::string> expectedAttrs = {"a_position", "a_normal", "a_color"};
    EXPECT_TRUE(validateVertexAttributes(currentProgram, expectedAttrs));
    
    std::vector<std::string> expectedUniforms = {"model", "view", "projection"};
    EXPECT_TRUE(validateUniforms(currentProgram, expectedUniforms));
}

// Test rendering with each shader
TEST_F(ShaderValidationComprehensiveTest, RenderWithBasicShader) {
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    
    Rendering::Transform transform;
    Rendering::Material material;
    material.shader = renderEngine->getBuiltinShader("basic");
    material.albedo = Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Set up camera using controller
    Camera::CameraController cameraController(nullptr);
    cameraController.setViewPreset(Camera::ViewPreset::ISOMETRIC);
    renderEngine->setCamera(*cameraController.getCamera());
    
    // Clear any existing errors
    while (glGetError() != GL_NO_ERROR) {}
    
    // Render
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->renderMesh(mesh, transform, material);
    renderEngine->endFrame();
    
    // Check for errors
    EXPECT_TRUE(checkGLError("RenderWithBasicShader"));
}

TEST_F(ShaderValidationComprehensiveTest, RenderWithEnhancedShader) {
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    
    Rendering::Transform transform;
    Rendering::Material material;
    material.shader = renderEngine->getBuiltinShader("enhanced");
    material.albedo = Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f);
    
    Camera::CameraController cameraController(nullptr);
    cameraController.setViewPreset(Camera::ViewPreset::ISOMETRIC);
    renderEngine->setCamera(*cameraController.getCamera());
    
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->renderMesh(mesh, transform, material);
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("RenderWithEnhancedShader"));
}

TEST_F(ShaderValidationComprehensiveTest, RenderWithFlatShader) {
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    
    Rendering::Transform transform;
    Rendering::Material material;
    material.shader = renderEngine->getBuiltinShader("flat");
    material.albedo = Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f);
    
    Camera::CameraController cameraController(nullptr);
    cameraController.setViewPreset(Camera::ViewPreset::ISOMETRIC);
    renderEngine->setCamera(*cameraController.getCamera());
    
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->renderMesh(mesh, transform, material);
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("RenderWithFlatShader"));
}

// Test vertex attribute setup
TEST_F(ShaderValidationComprehensiveTest, VertexAttributeSetup) {
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    
    // Check that VAO is created
    EXPECT_NE(mesh.vertexArray, 0u);
    
    // Bind VAO and check attributes
    glBindVertexArray(mesh.vertexArray);
    
    // Check attribute 0 (position)
    GLint enabled;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE);
    
    // Check attribute 1 (normal)
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE);
    
    // Check attribute 2 (color)
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE);
    
    // Check that attribute 3 (texcoord) is NOT enabled
    glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_FALSE);
    
    glBindVertexArray(0);
    
    EXPECT_TRUE(checkGLError("VertexAttributeSetup"));
}

// Test line rendering
TEST_F(ShaderValidationComprehensiveTest, LineRendering) {
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    
    Rendering::Transform transform;
    Rendering::Material material;
    material.shader = renderEngine->getBuiltinShader("basic");
    material.albedo = Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f);
    
    Camera::CameraController cameraController(nullptr);
    cameraController.setViewPreset(Camera::ViewPreset::ISOMETRIC);
    renderEngine->setCamera(*cameraController.getCamera());
    
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->setLineWidth(2.0f);
    renderEngine->renderMeshAsLines(mesh, transform, material);
    renderEngine->setLineWidth(1.0f);
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("LineRendering"));
}

// Test ground plane shader
TEST_F(ShaderValidationComprehensiveTest, GroundPlaneShaderValidation) {
    // The ground plane grid has its own shader
    Camera::CameraController cameraController(nullptr);
    cameraController.setViewPreset(Camera::ViewPreset::ISOMETRIC);
    renderEngine->setCamera(*cameraController.getCamera());
    
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->setGroundPlaneGridVisible(true);
    renderEngine->updateGroundPlaneGrid(Math::Vector3f(10.0f, 10.0f, 10.0f));
    renderEngine->renderGroundPlaneGrid(Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f)));
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("GroundPlaneShaderValidation"));
}

// Test shader uniform updates
TEST_F(ShaderValidationComprehensiveTest, ShaderUniformUpdates) {
    auto shader = renderEngine->getBuiltinShader("basic");
    EXPECT_NE(shader, Rendering::InvalidId);
    
    // Create test matrices
    Math::Matrix4f model;
    model.identity();
    Math::Matrix4f view = Math::Matrix4f::lookAt(
        Math::Vector3f(0.0f, 0.0f, 5.0f),
        Math::Vector3f(0.0f, 0.0f, 0.0f),
        Math::Vector3f(0.0f, 1.0f, 0.0f)
    );
    Math::Matrix4f projection = Math::Matrix4f::perspective(45.0f, 1.0f, 0.1f, 100.0f);
    
    // Get shader program and use it
    auto* program = shaderManager->getShaderProgram(shader);
    EXPECT_NE(program, nullptr);
    
    program->use();
    
    while (glGetError() != GL_NO_ERROR) {}
    
    program->setUniform("model", model);
    EXPECT_TRUE(checkGLError("Set model uniform"));
    
    program->setUniform("view", view);
    EXPECT_TRUE(checkGLError("Set view uniform"));
    
    program->setUniform("projection", projection);
    EXPECT_TRUE(checkGLError("Set projection uniform"));
    
    program->setUniform("lightPos", Math::Vector3f(1.0f, 1.0f, 1.0f));
    EXPECT_TRUE(checkGLError("Set lightPos uniform"));
    
    program->setUniform("lightColor", Math::Vector3f(1.0f, 1.0f, 1.0f));
    EXPECT_TRUE(checkGLError("Set lightColor uniform"));
    
    program->setUniform("viewPos", Math::Vector3f(0.0f, 0.0f, 5.0f));
    EXPECT_TRUE(checkGLError("Set viewPos uniform"));
    
    program->unuse();
}

// Test error conditions
TEST_F(ShaderValidationComprehensiveTest, ErrorConditions) {
    // Test rendering without setting camera
    auto mesh = createTestMesh();
    renderEngine->setupMeshBuffers(mesh);
    
    Rendering::Transform transform;
    Rendering::Material material;
    material.shader = renderEngine->getBuiltinShader("basic");
    
    // Don't set camera - should handle gracefully
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->renderMesh(mesh, transform, material);
    renderEngine->endFrame();
    
    // Should not crash, but might have GL errors
    glGetError(); // Clear any errors
}

// Test multiple mesh rendering
TEST_F(ShaderValidationComprehensiveTest, MultipleMeshRendering) {
    // Create multiple meshes
    std::vector<Rendering::Mesh> meshes;
    for (int i = 0; i < 5; ++i) {
        auto mesh = createTestMesh();
        renderEngine->setupMeshBuffers(mesh);
        meshes.push_back(mesh);
    }
    
    Camera::CameraController cameraController(nullptr);
    cameraController.setViewPreset(Camera::ViewPreset::ISOMETRIC);
    renderEngine->setCamera(*cameraController.getCamera());
    
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    
    // Render each mesh with different shaders
    std::vector<std::string> shaderNames = {"basic", "enhanced", "flat", "basic", "enhanced"};
    for (size_t i = 0; i < meshes.size(); ++i) {
        Rendering::Transform transform;
        transform.position = Math::WorldCoordinates(Math::Vector3f(i * 0.5f - 1.0f, 0.0f, 0.0f));
        
        Rendering::Material material;
        material.shader = renderEngine->getBuiltinShader(shaderNames[i]);
        material.albedo = Rendering::Color(1.0f, 0.5f, 0.2f, 1.0f);
        
        renderEngine->renderMesh(meshes[i], transform, material);
    }
    
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("MultipleMeshRendering"));
}

} // namespace Tests
} // namespace VoxelEditor