#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <gtest/gtest.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace VoxelEditor {
namespace Rendering {

class ShaderVAOIntegrationTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    
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
        window = glfwCreateWindow(640, 480, "Shader VAO Test", nullptr, nullptr);
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
    }
    
    void TearDown() override {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
};

TEST_F(ShaderVAOIntegrationTest, BasicVAOFunctionality) {
    // Check OpenGL version
    const char* version = (const char*)glGetString(GL_VERSION);
    std::cout << "OpenGL Version: " << version << std::endl;
    
    // Test VAO creation
    GLuint vao;
    if (GLAD_GL_VERSION_3_0) {
        // Core VAO functions should be available
        ASSERT_TRUE(glGenVertexArrays != nullptr) << "glGenVertexArrays not loaded";
        ASSERT_TRUE(glBindVertexArray != nullptr) << "glBindVertexArray not loaded";
        ASSERT_TRUE(glDeleteVertexArrays != nullptr) << "glDeleteVertexArrays not loaded";
        
        // Create VAO
        glGenVertexArrays(1, &vao);
        EXPECT_NE(vao, 0) << "Failed to generate VAO";
        
        // Bind VAO
        glBindVertexArray(vao);
        
        // Check binding
        GLint currentVAO;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
        EXPECT_EQ(currentVAO, vao) << "VAO not properly bound";
        
        // Unbind
        glBindVertexArray(0);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
        EXPECT_EQ(currentVAO, 0) << "VAO not properly unbound";
        
        // Clean up
        glDeleteVertexArrays(1, &vao);
    } else {
        GTEST_SKIP() << "OpenGL 3.0+ not available for core VAO support";
    }
}

TEST_F(ShaderVAOIntegrationTest, SimpleTriangleWithVAO) {
    if (!GLAD_GL_VERSION_3_0) {
        GTEST_SKIP() << "OpenGL 3.0+ required";
    }
    
    // Simple vertex shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    
    // Simple fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.5, 0.2, 1.0);
        }
    )";
    
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    ASSERT_TRUE(success) << "Vertex shader compilation failed";
    
    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    ASSERT_TRUE(success) << "Fragment shader compilation failed";
    
    // Link program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    ASSERT_TRUE(success) << "Shader program linking failed";
    
    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Triangle vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
    
    // Create VAO and VBO
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    // Bind VAO first
    glBindVertexArray(vao);
    
    // Bind and fill VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Configure vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Unbind VAO
    glBindVertexArray(0);
    
    // Test rendering
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Check for errors
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error during rendering: " << error;
    
    // Clean up
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
}

TEST_F(ShaderVAOIntegrationTest, VAOAttributeState) {
    if (!GLAD_GL_VERSION_3_0) {
        GTEST_SKIP() << "OpenGL 3.0+ required";
    }
    
    // Create two VAOs with different configurations
    GLuint vao1, vao2, vbo1, vbo2;
    glGenVertexArrays(1, &vao1);
    glGenVertexArrays(1, &vao2);
    glGenBuffers(1, &vbo1);
    glGenBuffers(1, &vbo2);
    
    // Configure first VAO
    glBindVertexArray(vao1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo1);
    
    // Enable attribute 0 with specific configuration
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Configure second VAO differently
    glBindVertexArray(vao2);
    glBindBuffer(GL_ARRAY_BUFFER, vbo2);
    
    // Enable attributes 0 and 1
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Test that VAO state is preserved
    glBindVertexArray(vao1);
    GLint enabled;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_TRUE(enabled) << "Attribute 0 should be enabled in VAO1";
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_FALSE(enabled) << "Attribute 1 should be disabled in VAO1";
    
    glBindVertexArray(vao2);
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_TRUE(enabled) << "Attribute 0 should be enabled in VAO2";
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_TRUE(enabled) << "Attribute 1 should be enabled in VAO2";
    
    // Clean up
    glDeleteVertexArrays(1, &vao1);
    glDeleteVertexArrays(1, &vao2);
    glDeleteBuffers(1, &vbo1);
    glDeleteBuffers(1, &vbo2);
}

} // namespace Rendering
} // namespace VoxelEditor