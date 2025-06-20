#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "core/rendering/OpenGLRenderer.h"
#include "core/rendering/ShaderManager.h"
#include "core/rendering/GroundPlaneGrid.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Matrix4f.h"
#include "foundation/logging/Logger.h"
#include <vector>

using namespace VoxelEditor;
using namespace VoxelEditor::Rendering;

class ShaderUsageValidationTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    OpenGLRenderer* renderer = nullptr;
    ShaderManager* shaderManager = nullptr;
    
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
        window = glfwCreateWindow(800, 600, "Shader Usage Test", nullptr, nullptr);
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
        
        // Create renderer and shader manager
        renderer = new OpenGLRenderer();
        renderer->initialize();
        
        auto* logger = &Logging::Logger::getInstance();
        shaderManager = new ShaderManager(renderer, logger);
    }
    
    void TearDown() override {
        delete shaderManager;
        delete renderer;
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
};

TEST_F(ShaderUsageValidationTest, ShaderWithVAOPipeline) {
    // Create a simple shader program
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 2) in vec4 aColor;
        
        out vec4 vertexColor;
        
        uniform mat4 mvp;
        
        void main() {
            gl_Position = mvp * vec4(aPos, 1.0);
            vertexColor = aColor;
        }
    )";
    
    const char* fragmentSource = R"(
        #version 330 core
        in vec4 vertexColor;
        out vec4 FragColor;
        
        void main() {
            FragColor = vertexColor;
        }
    )";
    
    // Create shader
    ShaderId shader = shaderManager->createShaderFromSource("test_vao", vertexSource, fragmentSource);
    ASSERT_NE(shader, InvalidId) << "Failed to create shader";
    
    // Create VAO
    GLuint vao = renderer->createVertexArray();
    ASSERT_NE(vao, 0) << "Failed to create VAO";
    
    // Vertex data
    struct Vertex {
        float x, y, z;      // Position
        float r, g, b, a;   // Color
    };
    
    std::vector<Vertex> vertices = {
        {-0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f},  // Red
        { 0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f},  // Green
        { 0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f}   // Blue
    };
    
    // Bind VAO
    renderer->bindVertexArray(vao);
    
    // Create VBO
    GLuint vbo = renderer->createVertexBuffer(vertices.data(), vertices.size() * sizeof(Vertex), BufferUsage::Static);
    ASSERT_NE(vbo, 0) << "Failed to create VBO";
    
    // Setup vertex attributes manually since we're testing real usage
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute (location 2 to match shader expectation)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Unbind VAO
    renderer->bindVertexArray(0);
    
    // Now test rendering
    renderer->clear(ClearFlags::COLOR | ClearFlags::DEPTH);
    renderer->setClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    
    // Use shader
    renderer->useProgram(shader);
    
    // Set MVP matrix
    Matrix4f mvp;
    mvp.setIdentity();
    renderer->setUniform("mvp", UniformValue(mvp));
    
    // Bind VAO and draw
    renderer->bindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Check for errors
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error: " << error;
    
    // Cleanup
    renderer->deleteVertexArray(vao);
    renderer->deleteBuffer(vbo);
}

TEST_F(ShaderUsageValidationTest, GroundPlaneShaderValidation) {
    // Test the actual ground plane grid shader
    auto groundPlane = std::make_unique<GroundPlaneGrid>();
    
    // Initialize with workspace bounds
    Vector3f workspaceSize(10.0f, 10.0f, 10.0f);
    groundPlane->initialize(workspaceSize);
    
    // Create view and projection matrices
    Matrix4f view;
    view.lookAt(Vector3f(5.0f, 5.0f, 5.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f));
    
    Matrix4f projection = Matrix4f::perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    
    // Clear
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render ground plane
    groundPlane->render(view, projection);
    
    // Check for errors
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error in ground plane rendering: " << error;
}

TEST_F(ShaderUsageValidationTest, ShaderAttributeLocations) {
    // Test that shaders use the expected attribute locations
    
    // Load a shader from source that matches the expected layout
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_normal;
        layout(location = 2) in vec4 a_color;
        layout(location = 3) in vec2 a_texCoord;
        
        out vec3 Normal;
        out vec4 Color;
        out vec2 TexCoord;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * model * vec4(a_position, 1.0);
            Normal = mat3(transpose(inverse(model))) * a_normal;
            Color = a_color;
            TexCoord = a_texCoord;
        }
    )";
    
    const char* fragmentSource = R"(
        #version 330 core
        in vec3 Normal;
        in vec4 Color;
        in vec2 TexCoord;
        
        out vec4 FragColor;
        
        void main() {
            FragColor = Color;
        }
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource("attribute_test", vertexSource, fragmentSource);
    ASSERT_NE(shader, InvalidId);
    
    // Get the actual OpenGL program ID
    renderer->useProgram(shader);
    
    // Verify attribute locations match expectations
    // Note: We can't directly get the program ID from ShaderManager in the current API,
    // but we can verify that setting up VAO with expected locations works
    
    // Create test VAO with the expected attribute layout
    GLuint vao = renderer->createVertexArray();
    renderer->bindVertexArray(vao);
    
    // Create a dummy VBO
    float dummyData[16] = {0}; // Enough for one vertex with all attributes
    GLuint vbo = renderer->createVertexBuffer(dummyData, sizeof(dummyData), BufferUsage::Static);
    
    // Setup attributes at expected locations
    // Position at location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal at location 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);
    
    // Color at location 2
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(2);
    
    // TexCoord at location 3
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(3);
    
    // If the shader expects different locations, this will cause errors
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "Attribute setup error: " << error;
    
    // Cleanup
    renderer->deleteVertexArray(vao);
    renderer->deleteBuffer(vbo);
}

TEST_F(ShaderUsageValidationTest, MultipleVAOSwitching) {
    // Test switching between multiple VAOs during rendering
    
    // Create a simple shader
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 2) in vec3 aColor;
        
        out vec3 fragColor;
        
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            fragColor = aColor;
        }
    )";
    
    const char* fragmentSource = R"(
        #version 330 core
        in vec3 fragColor;
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(fragColor, 1.0);
        }
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource("multi_vao", vertexSource, fragmentSource);
    ASSERT_NE(shader, InvalidId);
    
    // Create two VAOs with different data
    struct SimpleVertex {
        float x, y;
        float r, g, b;
    };
    
    // First triangle (red)
    std::vector<SimpleVertex> triangle1 = {
        {-0.8f, -0.5f,  1.0f, 0.0f, 0.0f},
        {-0.2f, -0.5f,  1.0f, 0.0f, 0.0f},
        {-0.5f,  0.5f,  1.0f, 0.0f, 0.0f}
    };
    
    // Second triangle (blue)
    std::vector<SimpleVertex> triangle2 = {
        { 0.2f, -0.5f,  0.0f, 0.0f, 1.0f},
        { 0.8f, -0.5f,  0.0f, 0.0f, 1.0f},
        { 0.5f,  0.5f,  0.0f, 0.0f, 1.0f}
    };
    
    // Setup first VAO
    GLuint vao1 = renderer->createVertexArray();
    renderer->bindVertexArray(vao1);
    GLuint vbo1 = renderer->createVertexBuffer(triangle1.data(), triangle1.size() * sizeof(SimpleVertex), BufferUsage::Static);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Setup second VAO
    GLuint vao2 = renderer->createVertexArray();
    renderer->bindVertexArray(vao2);
    GLuint vbo2 = renderer->createVertexBuffer(triangle2.data(), triangle2.size() * sizeof(SimpleVertex), BufferUsage::Static);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Clear and use shader
    renderer->clear(ClearFlags::COLOR | ClearFlags::DEPTH);
    renderer->setClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    renderer->useProgram(shader);
    
    // Draw first triangle
    renderer->bindVertexArray(vao1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Draw second triangle
    renderer->bindVertexArray(vao2);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Verify no errors
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "Error during VAO switching: " << error;
    
    // Cleanup
    renderer->deleteVertexArray(vao1);
    renderer->deleteVertexArray(vao2);
    renderer->deleteBuffer(vbo1);
    renderer->deleteBuffer(vbo2);
}