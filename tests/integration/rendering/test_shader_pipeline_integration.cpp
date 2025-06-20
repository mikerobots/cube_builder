#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

namespace VoxelEditor {
namespace Rendering {

// Vertex structure matching shader expectations
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
};

class ShaderPipelineIntegrationTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::string shaderPath;
    
    // OpenGL objects
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    
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
        window = glfwCreateWindow(800, 600, "Shader Pipeline Integration Test", nullptr, nullptr);
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
        
        // Set shader path
        shaderPath = "/Users/michaelhalloran/cube_edit/build_ninja/bin/core/rendering/shaders/";
        
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        
        // Check for VAO support (OpenGL 3.0+ has core VAO support)
        GLint major, minor;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        
        if (major < 3) {
            std::cerr << "Warning: OpenGL 3.0+ required for VAO support" << std::endl;
        }
    }
    
    void TearDown() override {
        // Clean up OpenGL objects
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (ebo) glDeleteBuffers(1, &ebo);
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    std::string readShaderFile(const std::string& filename) {
        std::string fullPath = shaderPath + filename;
        std::ifstream file(fullPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open shader file: " << fullPath << std::endl;
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    GLuint compileShader(GLenum type, const std::string& source) {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        
        if (!success) {
            GLint logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            
            if (logLength > 0) {
                std::vector<char> log(logLength);
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());
                std::cerr << "Shader compilation error:\n" << log.data() << std::endl;
            }
            
            glDeleteShader(shader);
            return 0;
        }
        
        return shader;
    }
    
    GLuint createProgram(const std::string& vertexSource, const std::string& fragmentSource) {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
        if (!vertexShader) return 0;
        
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (!fragmentShader) {
            glDeleteShader(vertexShader);
            return 0;
        }
        
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        
        // Bind attribute locations before linking
        glBindAttribLocation(program, 0, "a_position");
        glBindAttribLocation(program, 1, "a_normal");
        glBindAttribLocation(program, 2, "a_color");
        
        glLinkProgram(program);
        
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        
        if (!success) {
            GLint logLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            
            if (logLength > 0) {
                std::vector<char> log(logLength);
                glGetProgramInfoLog(program, logLength, nullptr, log.data());
                std::cerr << "Program linking error:\n" << log.data() << std::endl;
            }
            
            glDeleteProgram(program);
            program = 0;
        }
        
        // Clean up shaders
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return program;
    }
    
    void createCubeGeometry(std::vector<Vertex>& vertices, std::vector<GLuint>& indices) {
        // Cube vertices with positions, normals, and colors
        vertices = {
            // Front face (z = 0.5)
            {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
            
            // Back face (z = -0.5)
            {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}},
            
            // Top face (y = 0.5)
            {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.8f, 0.8f, 0.8f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.8f, 0.8f, 0.8f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.8f, 0.8f, 0.8f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.8f, 0.8f, 0.8f, 1.0f}},
            
            // Bottom face (y = -0.5)
            {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.3f, 0.3f, 0.3f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.3f, 0.3f, 0.3f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.3f, 0.3f, 0.3f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.3f, 0.3f, 0.3f, 1.0f}},
            
            // Right face (x = 0.5)
            {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.6f, 0.6f, 0.6f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.6f, 0.6f, 0.6f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.6f, 0.6f, 0.6f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.6f, 0.6f, 0.6f, 1.0f}},
            
            // Left face (x = -0.5)
            {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.4f, 0.4f, 0.4f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.4f, 0.4f, 0.4f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.4f, 0.4f, 0.4f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.4f, 0.4f, 0.4f, 1.0f}}
        };
        
        // Indices for triangles
        indices = {
            // Front face
            0, 1, 2,    2, 3, 0,
            // Back face
            4, 6, 5,    6, 4, 7,
            // Top face
            8, 9, 10,   10, 11, 8,
            // Bottom face
            12, 14, 13, 14, 12, 15,
            // Right face
            16, 17, 18, 18, 19, 16,
            // Left face
            20, 22, 21, 22, 20, 23
        };
    }
    
    void setupVAO(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
        // Check if VAO functions are available
        if (!glGenVertexArrays || !glBindVertexArray) {
            std::cerr << "VAO functions not available" << std::endl;
            return;
        }
        
        // Generate and bind VAO
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        // Generate and bind VBO
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        
        // Generate and bind EBO
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        
        // Setup vertex attributes
        // Position attribute (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);
        
        // Normal attribute (location = 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);
        
        // Color attribute (location = 2)
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(2);
        
        // Unbind VAO
        glBindVertexArray(0);
    }
    
    bool checkGLError(const std::string& context) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in " << context << ": " << error << std::endl;
            return true;
        }
        return false;
    }
};

TEST_F(ShaderPipelineIntegrationTest, BasicVoxelShaderPipeline) {
    // Load shaders
    std::string vertexSource = readShaderFile("basic_voxel_gl33.vert");
    std::string fragmentSource = readShaderFile("basic_voxel_gl33.frag");
    
    ASSERT_FALSE(vertexSource.empty()) << "Failed to load vertex shader";
    ASSERT_FALSE(fragmentSource.empty()) << "Failed to load fragment shader";
    
    // Create shader program
    GLuint program = createProgram(vertexSource, fragmentSource);
    ASSERT_NE(program, 0) << "Failed to create shader program";
    
    // Create geometry
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    createCubeGeometry(vertices, indices);
    
    // Setup VAO
    setupVAO(vertices, indices);
    ASSERT_FALSE(checkGLError("VAO setup"));
    
    // Use program
    glUseProgram(program);
    ASSERT_FALSE(checkGLError("glUseProgram"));
    
    // Set up matrices
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), 
                                 glm::vec3(0.0f, 0.0f, 0.0f), 
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    
    // Set uniforms
    GLint modelLoc = glGetUniformLocation(program, "model");
    GLint viewLoc = glGetUniformLocation(program, "view");
    GLint projLoc = glGetUniformLocation(program, "projection");
    
    EXPECT_NE(modelLoc, -1) << "model uniform not found";
    EXPECT_NE(viewLoc, -1) << "view uniform not found";
    EXPECT_NE(projLoc, -1) << "projection uniform not found";
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    ASSERT_FALSE(checkGLError("Set uniforms"));
    
    // Clear and render
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Bind VAO and draw
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    ASSERT_FALSE(checkGLError("Draw call"));
    
    // Swap buffers
    glfwSwapBuffers(window);
    
    // Clean up
    glDeleteProgram(program);
}

TEST_F(ShaderPipelineIntegrationTest, EnhancedVoxelShaderPipeline) {
    // Load enhanced shader
    std::string vertexSource = readShaderFile("basic_voxel.vert");
    std::string fragmentSource = readShaderFile("enhanced_voxel.frag");
    
    ASSERT_FALSE(vertexSource.empty()) << "Failed to load vertex shader";
    ASSERT_FALSE(fragmentSource.empty()) << "Failed to load fragment shader";
    
    // Create shader program
    GLuint program = createProgram(vertexSource, fragmentSource);
    ASSERT_NE(program, 0) << "Failed to create shader program";
    
    // Create geometry
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    createCubeGeometry(vertices, indices);
    
    // Setup VAO
    setupVAO(vertices, indices);
    
    // Use program
    glUseProgram(program);
    
    // Set matrices
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0f), 
                                 glm::vec3(0.0f, 0.0f, 0.0f), 
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    
    // Set matrix uniforms
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Set lighting uniforms (if they exist)
    GLint lightPosLoc = glGetUniformLocation(program, "lightPos");
    GLint viewPosLoc = glGetUniformLocation(program, "viewPos");
    
    if (lightPosLoc != -1) {
        glUniform3f(lightPosLoc, 5.0f, 5.0f, 5.0f);
    }
    if (viewPosLoc != -1) {
        glUniform3f(viewPosLoc, 3.0f, 3.0f, 3.0f);
    }
    
    // Clear and render
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    ASSERT_FALSE(checkGLError("Enhanced shader rendering"));
    
    // Clean up
    glDeleteProgram(program);
}

TEST_F(ShaderPipelineIntegrationTest, MultipleVAOManagement) {
    // Test proper management of multiple VAOs
    
    // Load shader
    std::string vertexSource = readShaderFile("basic_voxel.vert");
    std::string fragmentSource = readShaderFile("basic_voxel.frag");
    
    GLuint program = createProgram(vertexSource, fragmentSource);
    ASSERT_NE(program, 0);
    
    // Create multiple VAOs
    GLuint vao1, vao2, vbo1, vbo2;
    
    // First cube
    std::vector<Vertex> vertices1;
    std::vector<GLuint> indices1;
    createCubeGeometry(vertices1, indices1);
    
    glGenVertexArrays(1, &vao1);
    glGenBuffers(1, &vbo1);
    
    glBindVertexArray(vao1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo1);
    glBufferData(GL_ARRAY_BUFFER, vertices1.size() * sizeof(Vertex), vertices1.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);
    
    // Second cube with different colors
    std::vector<Vertex> vertices2 = vertices1;
    for (auto& v : vertices2) {
        v.color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
    }
    
    glGenVertexArrays(1, &vao2);
    glGenBuffers(1, &vbo2);
    
    glBindVertexArray(vao2);
    glBindBuffer(GL_ARRAY_BUFFER, vbo2);
    glBufferData(GL_ARRAY_BUFFER, vertices2.size() * sizeof(Vertex), vertices2.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);
    
    // Test switching between VAOs
    glUseProgram(program);
    
    // Set common uniforms
    glm::mat4 view = glm::lookAt(glm::vec3(4.0f, 4.0f, 4.0f), 
                                 glm::vec3(0.0f, 0.0f, 0.0f), 
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Draw first cube
    glm::mat4 model1 = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model1));
    glBindVertexArray(vao1);
    glDrawArrays(GL_TRIANGLES, 0, vertices1.size());
    
    // Draw second cube
    glm::mat4 model2 = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model2));
    glBindVertexArray(vao2);
    glDrawArrays(GL_TRIANGLES, 0, vertices2.size());
    
    ASSERT_FALSE(checkGLError("Multiple VAO rendering"));
    
    // Clean up
    glDeleteVertexArrays(1, &vao1);
    glDeleteVertexArrays(1, &vao2);
    glDeleteBuffers(1, &vbo1);
    glDeleteBuffers(1, &vbo2);
    glDeleteProgram(program);
}

TEST_F(ShaderPipelineIntegrationTest, ShaderUniformValidation) {
    // Test proper uniform handling and validation
    
    std::string vertexSource = readShaderFile("basic_voxel_gl33.vert");
    std::string fragmentSource = readShaderFile("basic_voxel_gl33.frag");
    
    GLuint program = createProgram(vertexSource, fragmentSource);
    ASSERT_NE(program, 0);
    
    glUseProgram(program);
    
    // Test all expected uniforms
    struct UniformTest {
        const char* name;
        bool required;
    };
    
    std::vector<UniformTest> uniforms = {
        {"model", true},
        {"view", true},
        {"projection", true},
        {"lightPos", false},     // May be optimized out
        {"lightColor", false},   // May be optimized out
        {"viewPos", false}       // May be optimized out
    };
    
    for (const auto& uniform : uniforms) {
        GLint location = glGetUniformLocation(program, uniform.name);
        if (uniform.required) {
            EXPECT_NE(location, -1) << "Required uniform '" << uniform.name << "' not found";
        }
        
        if (location != -1) {
            // Test setting the uniform
            if (strcmp(uniform.name, "model") == 0 || 
                strcmp(uniform.name, "view") == 0 || 
                strcmp(uniform.name, "projection") == 0) {
                glm::mat4 matrix(1.0f);
                glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
            } else {
                glUniform3f(location, 1.0f, 1.0f, 1.0f);
            }
            ASSERT_FALSE(checkGLError(std::string("Set uniform ") + uniform.name));
        }
    }
    
    glDeleteProgram(program);
}

TEST_F(ShaderPipelineIntegrationTest, GroundPlaneShaderPipeline) {
    // Test ground plane shader with proper VAO setup
    
    std::string vertexSource = readShaderFile("ground_plane.vert");
    std::string fragmentSource = readShaderFile("ground_plane.frag");
    
    ASSERT_FALSE(vertexSource.empty()) << "Failed to load ground plane vertex shader";
    ASSERT_FALSE(fragmentSource.empty()) << "Failed to load ground plane fragment shader";
    
    GLuint program = createProgram(vertexSource, fragmentSource);
    ASSERT_NE(program, 0);
    
    // Create grid vertices
    std::vector<Vertex> gridVertices;
    std::vector<GLuint> gridIndices;
    
    const float gridSize = 10.0f;
    const int gridLines = 21;
    const float spacing = gridSize / (gridLines - 1);
    
    // Create grid lines
    for (int i = 0; i < gridLines; ++i) {
        float pos = -gridSize / 2.0f + i * spacing;
        
        // X-direction lines
        gridVertices.push_back({{pos, 0.0f, -gridSize / 2.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 0.5f}});
        gridVertices.push_back({{pos, 0.0f,  gridSize / 2.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 0.5f}});
        
        // Z-direction lines
        gridVertices.push_back({{-gridSize / 2.0f, 0.0f, pos}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 0.5f}});
        gridVertices.push_back({{ gridSize / 2.0f, 0.0f, pos}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f, 0.5f}});
    }
    
    // Create indices for line segments
    for (size_t i = 0; i < gridVertices.size(); i += 2) {
        gridIndices.push_back(i);
        gridIndices.push_back(i + 1);
    }
    
    // Setup VAO for grid
    GLuint gridVAO, gridVBO, gridEBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glGenBuffers(1, &gridEBO);
    
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(Vertex), gridVertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, gridIndices.size() * sizeof(GLuint), gridIndices.data(), GL_STATIC_DRAW);
    
    // Setup attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);
    
    // Render
    glUseProgram(program);
    
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), 
                                 glm::vec3(0.0f, 0.0f, 0.0f), 
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Set grid-specific uniforms if they exist
    GLint gridColorLoc = glGetUniformLocation(program, "gridColor");
    if (gridColorLoc != -1) {
        glUniform4f(gridColorLoc, 0.5f, 0.5f, 0.5f, 0.35f);
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(gridVAO);
    glDrawElements(GL_LINES, gridIndices.size(), GL_UNSIGNED_INT, 0);
    
    ASSERT_FALSE(checkGLError("Ground plane rendering"));
    
    // Clean up
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteBuffers(1, &gridEBO);
    glDeleteProgram(program);
}

TEST_F(ShaderPipelineIntegrationTest, VAOStateValidation) {
    // Test that VAO state is properly maintained
    
    std::string vertexSource = readShaderFile("basic_voxel.vert");
    std::string fragmentSource = readShaderFile("flat_voxel.frag");
    
    GLuint program = createProgram(vertexSource, fragmentSource);
    ASSERT_NE(program, 0);
    
    // Create VAO
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    createCubeGeometry(vertices, indices);
    setupVAO(vertices, indices);
    
    // Bind VAO and check state
    glBindVertexArray(vao);
    
    // Check that vertex attributes are enabled
    GLint enabled;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE) << "Position attribute should be enabled";
    
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE) << "Normal attribute should be enabled";
    
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE) << "Color attribute should be enabled";
    
    // Check attribute properties
    GLint size, stride, type;
    GLvoid* pointer;
    
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
    EXPECT_EQ(size, 3) << "Position should have 3 components";
    
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
    EXPECT_EQ(size, 3) << "Normal should have 3 components";
    
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
    EXPECT_EQ(size, 4) << "Color should have 4 components";
    
    // Unbind and verify state is preserved
    glBindVertexArray(0);
    
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_FALSE) << "Attributes should be disabled when VAO is unbound";
    
    // Rebind and verify state is restored
    glBindVertexArray(vao);
    
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE) << "Position attribute should be re-enabled";
    
    glDeleteProgram(program);
}

TEST_F(ShaderPipelineIntegrationTest, RenderingWithoutVAO) {
    // Test that rendering without a VAO properly fails (as expected in Core Profile)
    
    std::string vertexSource = readShaderFile("basic_voxel.vert");
    std::string fragmentSource = readShaderFile("basic_voxel.frag");
    
    GLuint program = createProgram(vertexSource, fragmentSource);
    ASSERT_NE(program, 0);
    
    glUseProgram(program);
    
    // Ensure no VAO is bound
    glBindVertexArray(0);
    
    // Attempt to draw without VAO
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // This should generate an error in Core Profile
    GLenum error = glGetError();
    EXPECT_NE(error, GL_NO_ERROR) << "Drawing without VAO should generate an error in Core Profile";
    
    glDeleteProgram(program);
}

} // namespace Rendering
} // namespace VoxelEditor