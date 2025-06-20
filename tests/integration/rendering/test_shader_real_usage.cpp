#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <gtest/gtest.h>
#include "core/rendering/RenderEngine.h"
#include "core/rendering/OpenGLRenderer.h"
#include "core/rendering/ShaderManager.h"
#include "core/rendering/GroundPlaneGrid.h"
#include "core/rendering/RenderTypes.h"
#include "core/camera/Camera.h"
#include "core/camera/OrbitCamera.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Matrix4f.h"
#include "foundation/math/CoordinateTypes.h"
#include "foundation/logging/Logger.h"
#include "foundation/events/EventDispatcher.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <memory>
#include <vector>

using namespace VoxelEditor;
using namespace VoxelEditor::Rendering;
using namespace VoxelEditor::Camera;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Logging;
using namespace VoxelEditor::Events;

class ShaderRealUsageTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<Logger> logger;
    std::unique_ptr<OpenGLRenderer> renderer;
    std::unique_ptr<ShaderManager> shaderManager;
    std::unique_ptr<RenderEngine> renderEngine;
    std::unique_ptr<Camera> camera;
    
    void SetUp() override {
        // Initialize GLFW
        if (!glfwInit()) {
            GTEST_SKIP() << "Failed to initialize GLFW";
        }
        
        // Configure for OpenGL 3.3 Core Profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        // Create window
        window = glfwCreateWindow(800, 600, "Shader Real Usage Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            GTEST_SKIP() << "Failed to create GLFW window";
        }
        
        // Make context current
        glfwMakeContextCurrent(window);
        
        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glfwDestroyWindow(window);
            glfwTerminate();
            GTEST_SKIP() << "Failed to initialize GLAD";
        }
        
        // Create components
        eventDispatcher = std::make_unique<EventDispatcher>();
        logger = std::make_unique<Logger>("ShaderRealUsageTest");
        renderer = std::make_unique<OpenGLRenderer>();
        renderer->initialize();
        
        shaderManager = std::make_unique<ShaderManager>(renderer.get(), logger.get());
        renderEngine = std::make_unique<RenderEngine>();
        renderEngine->initialize();
        
        // Create camera
        camera = std::make_unique<OrbitCamera>(eventDispatcher.get());
        camera->setPosition(WorldCoordinates(5.0f, 5.0f, 5.0f));
        camera->lookAt(WorldCoordinates(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f));
    }
    
    void TearDown() override {
        renderEngine.reset();
        shaderManager.reset();
        renderer.reset();
        camera.reset();
        eventDispatcher.reset();
        logger.reset();
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
};

TEST_F(ShaderRealUsageTest, BasicVoxelShaderWithVAO) {
    // This test validates the actual shader usage with proper VAO setup
    
    // Load basic voxel shader
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_normal;
        layout(location = 2) in vec4 a_color;
        
        out vec3 FragPos;
        out vec3 Normal;
        out vec4 Color;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            FragPos = vec3(model * vec4(a_position, 1.0));
            Normal = mat3(transpose(inverse(model))) * a_normal;
            Color = a_color;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";
    
    const char* fragmentSource = R"(
        #version 330 core
        in vec3 FragPos;
        in vec3 Normal;
        in vec4 Color;
        
        out vec4 FragColor;
        
        void main() {
            vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
            float diff = max(dot(normalize(Normal), lightDir), 0.0);
            vec3 diffuse = diff * Color.rgb;
            FragColor = vec4(diffuse, Color.a);
        }
    )";
    
    // Create shader through ShaderManager
    ShaderId shader = shaderManager->createShaderFromSource("test_basic", vertexSource, fragmentSource);
    ASSERT_NE(shader, InvalidId) << "Failed to create shader";
    
    // Create VAO for a cube
    GLuint vao = renderer->createVertexArray();
    ASSERT_NE(vao, 0) << "Failed to create VAO";
    
    // Vertex data for a cube face
    struct Vertex {
        float position[3];
        float normal[3];
        float color[4];
    };
    
    std::vector<Vertex> vertices = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}}
    };
    
    std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
    
    // Bind VAO
    renderer->bindVertexArray(vao);
    
    // Create and bind VBO
    GLuint vbo = renderer->createVertexBuffer(vertices.data(), vertices.size() * sizeof(Vertex), BufferUsage::Static);
    ASSERT_NE(vbo, 0) << "Failed to create VBO";
    
    // Create and bind IBO
    GLuint ibo = renderer->createIndexBuffer(indices.data(), indices.size(), BufferUsage::Static);
    ASSERT_NE(ibo, 0) << "Failed to create IBO";
    
    // Setup vertex attributes
    std::vector<VertexAttribute> attributes = {
        VertexAttribute::Position,
        VertexAttribute::Normal,
        VertexAttribute::Color
    };
    renderer->setupVertexAttributes(attributes);
    
    // Unbind VAO
    renderer->bindVertexArray(0);
    
    // Render with the shader and VAO
    renderer->clear(ClearFlags::COLOR | ClearFlags::DEPTH);
    renderer->setClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    // Use shader
    renderer->useProgram(shader);
    
    // Set uniforms
    Matrix4f model;
    model.setIdentity();
    Matrix4f view = camera->getViewMatrix();
    Matrix4f projection = Matrix4f::perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    
    renderer->setUniform("model", UniformValue(model));
    renderer->setUniform("view", UniformValue(view));
    renderer->setUniform("projection", UniformValue(projection));
    
    // Bind VAO and draw
    renderer->bindVertexArray(vao);
    renderer->drawElements(PrimitiveType::Triangles, indices.size(), IndexType::UInt32);
    
    // Check for errors
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error during rendering: " << error;
    
    // Cleanup
    renderer->deleteVertexArray(vao);
    renderer->deleteBuffer(vbo);
    renderer->deleteBuffer(ibo);
}

TEST_F(ShaderRealUsageTest, GroundPlaneGridRendering) {
    // Test the actual ground plane grid rendering with shaders
    
    auto groundPlane = std::make_unique<GroundPlaneGrid>();
    
    // Initialize with workspace bounds
    Vector3f workspaceSize(10.0f, 10.0f, 10.0f);
    groundPlane->initialize(workspaceSize);
    
    // Clear and set viewport
    renderEngine->setViewport(0, 0, 800, 600);
    renderEngine->clear(Color(0.2f, 0.2f, 0.2f, 1.0f));
    
    // Render ground plane
    Matrix4f view = camera->getViewMatrix();
    Matrix4f projection = Matrix4f::perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    
    renderEngine->beginFrame();
    groundPlane->render(view, projection);
    renderEngine->endFrame();
    
    // Verify successful rendering
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error during ground plane rendering: " << error;
}

TEST_F(ShaderRealUsageTest, ShaderProgramValidation) {
    // Test shader program validation which requires VAO to be bound
    
    // Create a simple shader
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 mvp;
        void main() {
            gl_Position = mvp * vec4(aPos, 1.0);
        }
    )";
    
    const char* fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 color;
        void main() {
            FragColor = color;
        }
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource("validation_test", vertexSource, fragmentSource);
    ASSERT_NE(shader, InvalidId);
    
    // Create minimal VAO for validation
    GLuint vao = renderer->createVertexArray();
    GLuint vbo = renderer->createVertexBuffer(nullptr, 3 * sizeof(float), BufferUsage::Dynamic);
    
    renderer->bindVertexArray(vao);
    renderer->bindBuffer(BufferType::Vertex, vbo);
    
    // Enable position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Use shader and validate
    renderer->useProgram(shader);
    
    // Validation should pass with VAO bound
    GLuint programId = shaderManager->getProgramId(shader);
    glValidateProgram(programId);
    
    GLint status;
    glGetProgramiv(programId, GL_VALIDATE_STATUS, &status);
    EXPECT_EQ(status, GL_TRUE) << "Shader program validation failed with VAO bound";
    
    // Unbind VAO and try validation again
    renderer->bindVertexArray(0);
    
    glValidateProgram(programId);
    glGetProgramiv(programId, GL_VALIDATE_STATUS, &status);
    // In Core Profile, validation without VAO should fail
    EXPECT_EQ(status, GL_FALSE) << "Shader validation should fail without VAO in Core Profile";
    
    // Cleanup
    renderer->deleteVertexArray(vao);
    renderer->deleteBuffer(vbo);
}

TEST_F(ShaderRealUsageTest, MultipleVAOWithDifferentShaders) {
    // Test switching between multiple VAOs and shaders
    
    // Create two different shaders
    ShaderId basicShader = renderEngine->getBuiltinShader("basic");
    ShaderId flatShader = renderEngine->getBuiltinShader("flat");
    
    ASSERT_NE(basicShader, InvalidId);
    ASSERT_NE(flatShader, InvalidId);
    
    // Create two VAOs with different vertex layouts
    struct BasicVertex {
        float position[3];
        float normal[3];
        float color[4];
    };
    
    // VAO 1 - Red triangle
    GLuint vao1 = renderer->createVertexArray();
    std::vector<BasicVertex> vertices1 = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}
    };
    
    renderer->bindVertexArray(vao1);
    GLuint vbo1 = renderer->createVertexBuffer(vertices1.data(), vertices1.size() * sizeof(BasicVertex), BufferUsage::Static);
    
    std::vector<VertexAttribute> attributes = {
        VertexAttribute::Position,
        VertexAttribute::Normal,
        VertexAttribute::Color
    };
    renderer->setupVertexAttributes(attributes);
    
    // VAO 2 - Blue triangle
    GLuint vao2 = renderer->createVertexArray();
    std::vector<BasicVertex> vertices2 = vertices1;
    for (auto& v : vertices2) {
        v.color[0] = 0.0f; // Red = 0
        v.color[2] = 1.0f; // Blue = 1
        v.position[1] += 1.0f; // Offset Y
    }
    
    renderer->bindVertexArray(vao2);
    GLuint vbo2 = renderer->createVertexBuffer(vertices2.data(), vertices2.size() * sizeof(BasicVertex), BufferUsage::Static);
    renderer->setupVertexAttributes(attributes);
    
    // Render both with different shaders
    renderer->clear(ClearFlags::COLOR | ClearFlags::DEPTH);
    
    Matrix4f view = camera->getViewMatrix();
    Matrix4f projection = Matrix4f::perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    Matrix4f model;
    model.setIdentity();
    
    // Draw first triangle with basic shader
    renderer->useProgram(basicShader);
    renderer->setUniform("model", UniformValue(model));
    renderer->setUniform("view", UniformValue(view));
    renderer->setUniform("projection", UniformValue(projection));
    
    renderer->bindVertexArray(vao1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Draw second triangle with flat shader
    renderer->useProgram(flatShader);
    renderer->setUniform("model", UniformValue(model));
    renderer->setUniform("view", UniformValue(view));
    renderer->setUniform("projection", UniformValue(projection));
    
    renderer->bindVertexArray(vao2);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Verify no errors
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error with multiple VAOs: " << error;
    
    // Cleanup
    renderer->deleteVertexArray(vao1);
    renderer->deleteVertexArray(vao2);
    renderer->deleteBuffer(vbo1);
    renderer->deleteBuffer(vbo2);
}

TEST_F(ShaderRealUsageTest, FileBasedShaderLoading) {
    // Test loading shaders from files as done in production
    
    std::string shaderPath = "core/rendering/shaders/";
    
    // Try to load file-based shaders
    ShaderId fileShader = shaderManager->loadFromFile(
        "basic_voxel_gl33",
        shaderPath + "basic_voxel_gl33.vert",
        shaderPath + "basic_voxel_gl33.frag"
    );
    
    if (fileShader != InvalidId) {
        // Successfully loaded from file
        EXPECT_TRUE(shaderManager->isValid(fileShader)) << "File shader should be valid";
        
        // Test using the shader
        renderer->useProgram(fileShader);
        
        // Check that expected uniforms exist
        GLuint programId = shaderManager->getProgramId(fileShader);
        GLint modelLoc = glGetUniformLocation(programId, "model");
        GLint viewLoc = glGetUniformLocation(programId, "view");
        GLint projLoc = glGetUniformLocation(programId, "projection");
        
        EXPECT_NE(modelLoc, -1) << "model uniform not found";
        EXPECT_NE(viewLoc, -1) << "view uniform not found";
        EXPECT_NE(projLoc, -1) << "projection uniform not found";
        
        // Verify shader attributes match expected layout
        GLint posLoc = glGetAttribLocation(programId, "a_position");
        GLint normalLoc = glGetAttribLocation(programId, "a_normal");
        GLint colorLoc = glGetAttribLocation(programId, "a_color");
        
        EXPECT_EQ(posLoc, 0) << "Position attribute should be at location 0";
        EXPECT_EQ(normalLoc, 1) << "Normal attribute should be at location 1";
        EXPECT_EQ(colorLoc, 2) << "Color attribute should be at location 2";
    } else {
        std::cout << "File-based shader loading not available, using built-in shaders" << std::endl;
    }
}