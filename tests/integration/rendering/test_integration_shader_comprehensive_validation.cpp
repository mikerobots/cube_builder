#include <gtest/gtest.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include "rendering/MacOSGLLoader.h"
// Define macros for easier use
#define VAO_GEN(n, arrays) VoxelEditor::Rendering::glGenVertexArrays(n, arrays)
#define VAO_BIND(array) VoxelEditor::Rendering::glBindVertexArray(array)
#define VAO_DELETE(n, arrays) VoxelEditor::Rendering::glDeleteVertexArrays(n, arrays)
#else
#define VAO_GEN(n, arrays) glGenVertexArrays(n, arrays)
#define VAO_BIND(array) glBindVertexArray(array)
#define VAO_DELETE(n, arrays) glDeleteVertexArrays(n, arrays)
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <memory>
#include <fstream>

#include "rendering/OpenGLRenderer.h"
#include "rendering/ShaderManager.h"
#include "rendering/RenderState.h"
#include "math/Matrix4f.h"
#include "math/Vector3f.h"
#include "math/Vector4f.h"
#ifdef __APPLE__
#include "rendering/MacOSGLLoader.h"
#endif

using namespace VoxelEditor;

// Test fixture for visual validation of voxel mesh rendering
class VoxelMeshVisualValidation : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Rendering::OpenGLRenderer> renderer;
    std::unique_ptr<Rendering::ShaderManager> shaderManager;
    std::unique_ptr<Rendering::RenderState> renderState;
    
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    
    void SetUp() override {
        // Skip in CI environment
        if (std::getenv("CI") != nullptr) {
            GTEST_SKIP() << "Skipping OpenGL tests in CI environment";
        }
        
        // Initialize GLFW
        if (!glfwInit()) {
            GTEST_SKIP() << "Failed to initialize GLFW";
        }
        
        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing
        #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
        
        // Create window
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Shader Visual Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            GTEST_SKIP() << "Failed to create GLFW window";
        }
        
        glfwMakeContextCurrent(window);
        
        #ifndef __APPLE__
        // Initialize GLAD (not needed on macOS)
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            GTEST_SKIP() << "Failed to initialize GLAD";
        }
        #endif
        
        #ifdef __APPLE__
        // Load OpenGL extensions for macOS compatibility
        ASSERT_TRUE(Rendering::LoadOpenGLExtensions()) << "Failed to load OpenGL extensions on macOS";
        #endif
        
        // Clear any GL errors from initialization
        while (glGetError() != GL_NO_ERROR) {}
        
        // Create renderer components
        renderer = std::make_unique<Rendering::OpenGLRenderer>();
        Rendering::RenderConfig config;
        config.windowWidth = WINDOW_WIDTH;
        config.windowHeight = WINDOW_HEIGHT;
        if (!renderer->initializeContext(config)) {
            GTEST_SKIP() << "Failed to initialize renderer context";
        }
        
        shaderManager = std::make_unique<Rendering::ShaderManager>(renderer.get());
        renderState = std::make_unique<Rendering::RenderState>();
        
        // Set viewport
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    }
    
    void TearDown() override {
        renderState.reset();
        shaderManager.reset();
        renderer.reset();
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    // Create a simple voxel cube mesh
    struct VoxelMesh {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        int indexCount;
        
        ~VoxelMesh() {
            if (vao) VAO_DELETE(1, &vao);
            if (vbo) glDeleteBuffers(1, &vbo);
            if (ebo) glDeleteBuffers(1, &ebo);
        }
    };
    
    std::unique_ptr<VoxelMesh> createVoxelCube(float size, const glm::vec3& color) {
        auto mesh = std::make_unique<VoxelMesh>();
        
        float halfSize = size * 0.5f;
        
        // Vertex data: position (3), normal (3), color (4)
        std::vector<float> vertices = {
            // Front face
            -halfSize, -halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  color.r, color.g, color.b, 1.0f, 1.0f,
             halfSize, -halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  color.r, color.g, color.b, 1.0f, 1.0f,
             halfSize,  halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  color.r, color.g, color.b, 1.0f, 1.0f,
            -halfSize,  halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  color.r, color.g, color.b, 1.0f, 1.0f,
            
            // Back face
            -halfSize, -halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  color.r, color.g, color.b, 1.0f,
             halfSize, -halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  color.r, color.g, color.b, 1.0f,
             halfSize,  halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  color.r, color.g, color.b, 1.0f,
            -halfSize,  halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  color.r, color.g, color.b, 1.0f,
            
            // Top face
            -halfSize,  halfSize, -halfSize,  0.0f,  1.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
             halfSize,  halfSize, -halfSize,  0.0f,  1.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
             halfSize,  halfSize,  halfSize,  0.0f,  1.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
            -halfSize,  halfSize,  halfSize,  0.0f,  1.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
            
            // Bottom face
            -halfSize, -halfSize, -halfSize,  0.0f, -1.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
             halfSize, -halfSize, -halfSize,  0.0f, -1.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
             halfSize, -halfSize,  halfSize,  0.0f, -1.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
            -halfSize, -halfSize,  halfSize,  0.0f, -1.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
            
            // Right face
             halfSize, -halfSize, -halfSize,  1.0f,  0.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
             halfSize, -halfSize,  halfSize,  1.0f,  0.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
             halfSize,  halfSize,  halfSize,  1.0f,  0.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
             halfSize,  halfSize, -halfSize,  1.0f,  0.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
            
            // Left face
            -halfSize, -halfSize, -halfSize, -1.0f,  0.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
            -halfSize, -halfSize,  halfSize, -1.0f,  0.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
            -halfSize,  halfSize,  halfSize, -1.0f,  0.0f,  0.0f,  color.r, color.g, color.b, 1.0f,
            -halfSize,  halfSize, -halfSize, -1.0f,  0.0f,  0.0f,  color.r, color.g, color.b
        };
        
        std::vector<unsigned int> indices = {
            0,  1,  2,  2,  3,  0,   // Front
            4,  7,  6,  6,  5,  4,   // Back
            8,  9, 10, 10, 11,  8,   // Top
           12, 15, 14, 14, 13, 12,   // Bottom
           16, 17, 18, 18, 19, 16,   // Right
           20, 23, 22, 22, 21, 20    // Left
        };
        
        // Create VAO
        VAO_GEN(1, &mesh->vao);
        VAO_BIND(mesh->vao);
        
        // Create VBO
        glGenBuffers(1, &mesh->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        // Create EBO
        glGenBuffers(1, &mesh->ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        // Set vertex attributes
        // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Color (vec4)
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        VAO_BIND(0);
        
        mesh->indexCount = indices.size();
        
        return mesh;
    }
    
    // Capture framebuffer contents
    std::vector<unsigned char> captureFramebuffer() {
        std::vector<unsigned char> pixels(WINDOW_WIDTH * WINDOW_HEIGHT * 4); // RGBA
        glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        return pixels;
    }
    
    // Save pixels to PPM for debugging
    void savePixelsToPPM(const std::vector<unsigned char>& pixels, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return;
        
        file << "P3\n" << WINDOW_WIDTH << " " << WINDOW_HEIGHT << "\n255\n";
        
        // PPM format is top-to-bottom, OpenGL is bottom-to-top
        for (int y = WINDOW_HEIGHT - 1; y >= 0; --y) {
            for (int x = 0; x < WINDOW_WIDTH; ++x) {
                int idx = (y * WINDOW_WIDTH + x) * 4;
                file << (int)pixels[idx] << " " << (int)pixels[idx + 1] << " " << (int)pixels[idx + 2] << " ";
            }
        }
        file.close();
    }
    
    // Count non-background pixels
    int countRenderedPixels(const std::vector<unsigned char>& pixels, 
                          unsigned char bgR = 0, unsigned char bgG = 0, unsigned char bgB = 0) {
        int count = 0;
        for (size_t i = 0; i < pixels.size(); i += 4) {
            if (pixels[i] != bgR || pixels[i+1] != bgG || pixels[i+2] != bgB) {
                count++;
            }
        }
        return count;
    }
    
    // Check if a color is present in the framebuffer
    bool isColorPresent(const std::vector<unsigned char>& pixels, 
                       unsigned char r, unsigned char g, unsigned char b, 
                       int tolerance = 10) {
        for (size_t i = 0; i < pixels.size(); i += 4) {
            if (std::abs(pixels[i] - r) <= tolerance &&
                std::abs(pixels[i+1] - g) <= tolerance &&
                std::abs(pixels[i+2] - b) <= tolerance) {
                return true;
            }
        }
        return false;
    }
    
    // Setup common matrices
    glm::mat4 getProjectionMatrix() {
        return glm::perspective(glm::radians(45.0f), 
                              (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 
                              0.1f, 100.0f);
    }
    
    glm::mat4 getViewMatrix() {
        return glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0f),
                          glm::vec3(0.0f, 0.0f, 0.0f),
                          glm::vec3(0.0f, 1.0f, 0.0f));
    }
    
    // Helper to convert glm matrix to Math::Matrix4f
    Math::Matrix4f glmToMathMatrix(const glm::mat4& mat) {
        Math::Matrix4f result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i * 4 + j] = mat[j][i]; // glm is column-major
            }
        }
        return result;
    }
};

// Test basic voxel shader renders a cube
TEST_F(VoxelMeshVisualValidation, BasicVoxelShaderRendersCube) {
    // Load basic voxel shader
    ASSERT_TRUE(shaderManager->loadShader("basic_voxel", 
                                         "core/rendering/shaders/basic_voxel_gl33.vert", 
                                         "core/rendering/shaders/basic_voxel_gl33.frag"));
    
    auto shaderId = shaderManager->getShader("basic_voxel");
    ASSERT_NE(shaderId, 0u);
    
    // Create red voxel cube
    auto cube = createVoxelCube(1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    
    // Clear to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    // Use OpenGLRenderer directly to bypass incomplete ShaderProgram wrapper
    renderer->useProgram(shaderId);
    
    // Set uniforms using OpenGLRenderer
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = getViewMatrix();
    glm::mat4 projection = getProjectionMatrix();
    
    // Create UniformValue objects for OpenGLRenderer
    Math::Matrix4f modelMat = glmToMathMatrix(model);
    Math::Matrix4f viewMat = glmToMathMatrix(view);
    Math::Matrix4f projMat = glmToMathMatrix(projection);
    
    renderer->setUniform(shaderId, "model", Rendering::UniformValue(modelMat));
    renderer->setUniform(shaderId, "view", Rendering::UniformValue(viewMat));
    renderer->setUniform(shaderId, "projection", Rendering::UniformValue(projMat));
    renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(3.0f, 3.0f, 3.0f)));
    renderer->setUniform(shaderId, "lightPos", Rendering::UniformValue(Math::Vector3f(10.0f, 10.0f, 10.0f)));
    renderer->setUniform(shaderId, "lightColor", Rendering::UniformValue(Math::Vector3f(1.0f, 1.0f, 1.0f)));
    
    // Draw cube
    VAO_BIND(cube->vao);
    glDrawElements(GL_TRIANGLES, cube->indexCount, GL_UNSIGNED_INT, 0);
    VAO_BIND(0);
    
    // Capture framebuffer
    auto pixels = captureFramebuffer();
    
    // Debug: Save framebuffer for inspection
    savePixelsToPPM(pixels, "test_output/debug_basic_voxel_shader.ppm");
    
    // Validate rendering
    int renderedPixels = countRenderedPixels(pixels);
    EXPECT_GT(renderedPixels, 1000) << "Expected significant number of rendered pixels, got: " << renderedPixels;
    
    // Check if red color is present (with lighting it won't be pure red)
    EXPECT_TRUE(isColorPresent(pixels, 255, 0, 0, 128)) << "Expected red-ish color in rendered output";
}

// Test enhanced voxel shader
TEST_F(VoxelMeshVisualValidation, EnhancedVoxelShaderRendersCube) {
    // Load enhanced voxel shader
    ASSERT_TRUE(shaderManager->loadShader("enhanced_voxel", 
                                         "core/rendering/shaders/enhanced_voxel.vert", 
                                         "core/rendering/shaders/enhanced_voxel.frag"));
    
    auto shaderId = shaderManager->getShader("enhanced_voxel");
    ASSERT_NE(shaderId, 0u);
    
    // Create green voxel cube
    auto cube = createVoxelCube(1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Clear to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    // Use OpenGLRenderer directly to bypass incomplete ShaderProgram wrapper
    renderer->useProgram(shaderId);
    
    // Set uniforms using OpenGLRenderer
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = getViewMatrix();
    glm::mat4 projection = getProjectionMatrix();
    
    // Create UniformValue objects for OpenGLRenderer
    Math::Matrix4f modelMat = glmToMathMatrix(model);
    Math::Matrix4f viewMat = glmToMathMatrix(view);
    Math::Matrix4f projMat = glmToMathMatrix(projection);
    
    renderer->setUniform(shaderId, "model", Rendering::UniformValue(modelMat));
    renderer->setUniform(shaderId, "view", Rendering::UniformValue(viewMat));
    renderer->setUniform(shaderId, "projection", Rendering::UniformValue(projMat));
    renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(3.0f, 3.0f, 3.0f)));
    renderer->setUniform(shaderId, "lightPos", Rendering::UniformValue(Math::Vector3f(10.0f, 10.0f, 10.0f)));
    renderer->setUniform(shaderId, "lightColor", Rendering::UniformValue(Math::Vector3f(1.0f, 1.0f, 1.0f)));
    
    // Draw cube
    VAO_BIND(cube->vao);
    glDrawElements(GL_TRIANGLES, cube->indexCount, GL_UNSIGNED_INT, 0);
    VAO_BIND(0);
    
    // Capture framebuffer
    auto pixels = captureFramebuffer();
    
    // Validate rendering
    int renderedPixels = countRenderedPixels(pixels);
    EXPECT_GT(renderedPixels, 1000) << "Expected significant number of rendered pixels";
    
    // Check if green color is present
    EXPECT_TRUE(isColorPresent(pixels, 0, 255, 0, 128)) << "Expected green-ish color in rendered output";
}

// Test flat voxel shader
TEST_F(VoxelMeshVisualValidation, FlatVoxelShaderRendersCube) {
    // Load flat voxel shader
    ASSERT_TRUE(shaderManager->loadShader("flat_voxel", 
                                         "core/rendering/shaders/flat_voxel.vert", 
                                         "core/rendering/shaders/flat_voxel.frag"));
    
    auto shaderId = shaderManager->getShader("flat_voxel");
    ASSERT_NE(shaderId, 0u);
    
    // Create blue voxel cube
    auto cube = createVoxelCube(1.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Clear to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    // Use OpenGLRenderer directly to bypass incomplete ShaderProgram wrapper
    renderer->useProgram(shaderId);
    
    // Set uniforms using OpenGLRenderer
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = getViewMatrix();
    glm::mat4 projection = getProjectionMatrix();
    
    // Create UniformValue objects for OpenGLRenderer
    Math::Matrix4f modelMat = glmToMathMatrix(model);
    Math::Matrix4f viewMat = glmToMathMatrix(view);
    Math::Matrix4f projMat = glmToMathMatrix(projection);
    
    renderer->setUniform(shaderId, "model", Rendering::UniformValue(modelMat));
    renderer->setUniform(shaderId, "view", Rendering::UniformValue(viewMat));
    renderer->setUniform(shaderId, "projection", Rendering::UniformValue(projMat));
    renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(3.0f, 3.0f, 3.0f)));
    renderer->setUniform(shaderId, "lightPos", Rendering::UniformValue(Math::Vector3f(10.0f, 10.0f, 10.0f)));
    renderer->setUniform(shaderId, "lightColor", Rendering::UniformValue(Math::Vector3f(1.0f, 1.0f, 1.0f)));
    
    // Draw cube
    VAO_BIND(cube->vao);
    glDrawElements(GL_TRIANGLES, cube->indexCount, GL_UNSIGNED_INT, 0);
    VAO_BIND(0);
    
    // Capture framebuffer
    auto pixels = captureFramebuffer();
    
    // Validate rendering
    int renderedPixels = countRenderedPixels(pixels);
    EXPECT_GT(renderedPixels, 1000) << "Expected significant number of rendered pixels";
    
    // Check if blue color is present
    EXPECT_TRUE(isColorPresent(pixels, 0, 0, 255, 128)) << "Expected blue-ish color in rendered output";
}

// Test multiple colored voxels
TEST_F(VoxelMeshVisualValidation, MultipleColoredVoxelsRender) {
    // Load basic voxel shader
    ASSERT_TRUE(shaderManager->loadShader("basic_voxel", 
                                         "core/rendering/shaders/basic_voxel_gl33.vert", 
                                         "core/rendering/shaders/basic_voxel_gl33.frag"));
    
    auto shaderId = shaderManager->getShader("basic_voxel");
    ASSERT_NE(shaderId, 0u);
    
    // Create multiple colored cubes
    auto redCube = createVoxelCube(0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
    auto greenCube = createVoxelCube(0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
    auto blueCube = createVoxelCube(0.5f, glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Clear to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    // Use OpenGLRenderer directly to bypass incomplete ShaderProgram wrapper
    renderer->useProgram(shaderId);
    
    // Set common uniforms using OpenGLRenderer
    glm::mat4 view = getViewMatrix();
    glm::mat4 projection = getProjectionMatrix();
    Math::Matrix4f viewMat = glmToMathMatrix(view);
    Math::Matrix4f projMat = glmToMathMatrix(projection);
    
    renderer->setUniform(shaderId, "view", Rendering::UniformValue(viewMat));
    renderer->setUniform(shaderId, "projection", Rendering::UniformValue(projMat));
    renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(3.0f, 3.0f, 3.0f)));
    renderer->setUniform(shaderId, "lightPos", Rendering::UniformValue(Math::Vector3f(10.0f, 10.0f, 10.0f)));
    renderer->setUniform(shaderId, "lightColor", Rendering::UniformValue(Math::Vector3f(1.0f, 1.0f, 1.0f)));
    
    // Draw red cube
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
    renderer->setUniform(shaderId, "model", Rendering::UniformValue(glmToMathMatrix(model)));
    VAO_BIND(redCube->vao);
    glDrawElements(GL_TRIANGLES, redCube->indexCount, GL_UNSIGNED_INT, 0);
    
    // Draw green cube
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    renderer->setUniform(shaderId, "model", Rendering::UniformValue(glmToMathMatrix(model)));
    VAO_BIND(greenCube->vao);
    glDrawElements(GL_TRIANGLES, greenCube->indexCount, GL_UNSIGNED_INT, 0);
    
    // Draw blue cube
    model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    renderer->setUniform(shaderId, "model", Rendering::UniformValue(glmToMathMatrix(model)));
    VAO_BIND(blueCube->vao);
    glDrawElements(GL_TRIANGLES, blueCube->indexCount, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
    
    // Capture framebuffer
    auto pixels = captureFramebuffer();
    
    // Validate all three colors are present
    EXPECT_TRUE(isColorPresent(pixels, 255, 0, 0, 128)) << "Expected red color in output";
    EXPECT_TRUE(isColorPresent(pixels, 0, 255, 0, 128)) << "Expected green color in output";
    EXPECT_TRUE(isColorPresent(pixels, 0, 0, 255, 128)) << "Expected blue color in output";
}

// Test shader switching
TEST_F(VoxelMeshVisualValidation, ShaderSwitchingWorks) {
    // Load all shaders
    ASSERT_TRUE(shaderManager->loadShader("basic_voxel", 
                                         "core/rendering/shaders/basic_voxel_gl33.vert", 
                                         "core/rendering/shaders/basic_voxel_gl33.frag"));
    ASSERT_TRUE(shaderManager->loadShader("enhanced_voxel", 
                                         "core/rendering/shaders/enhanced_voxel.vert", 
                                         "core/rendering/shaders/enhanced_voxel.frag"));
    ASSERT_TRUE(shaderManager->loadShader("flat_voxel", 
                                         "core/rendering/shaders/flat_voxel.vert", 
                                         "core/rendering/shaders/flat_voxel.frag"));
    
    // Create white cube to see lighting differences
    auto cube = createVoxelCube(1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    
    // Common matrices
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = getViewMatrix();
    glm::mat4 projection = getProjectionMatrix();
    
    // Test each shader
    std::vector<std::string> shaderNames = {"basic_voxel", "enhanced_voxel", "flat_voxel"};
    
    for (const auto& shaderName : shaderNames) {
        auto shaderId = shaderManager->getShader(shaderName);
        ASSERT_NE(shaderId, 0u) << "Failed to get shader ID: " << shaderName;
        
        // Clear and render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        
        // Use OpenGLRenderer directly to bypass incomplete ShaderProgram wrapper
        renderer->useProgram(shaderId);
        
        // Set uniforms using OpenGLRenderer
        Math::Matrix4f modelMat = glmToMathMatrix(model);
        Math::Matrix4f viewMat = glmToMathMatrix(view);
        Math::Matrix4f projMat = glmToMathMatrix(projection);
        
        renderer->setUniform(shaderId, "model", Rendering::UniformValue(modelMat));
        renderer->setUniform(shaderId, "view", Rendering::UniformValue(viewMat));
        renderer->setUniform(shaderId, "projection", Rendering::UniformValue(projMat));
        renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(3.0f, 3.0f, 3.0f)));
        renderer->setUniform(shaderId, "lightPos", Rendering::UniformValue(Math::Vector3f(10.0f, 10.0f, 10.0f)));
        renderer->setUniform(shaderId, "lightColor", Rendering::UniformValue(Math::Vector3f(1.0f, 1.0f, 1.0f)));
        
        VAO_BIND(cube->vao);
        glDrawElements(GL_TRIANGLES, cube->indexCount, GL_UNSIGNED_INT, 0);
        VAO_BIND(0);
        
        // Capture and validate
        auto pixels = captureFramebuffer();
        int renderedPixels = countRenderedPixels(pixels);
        EXPECT_GT(renderedPixels, 1000) << "Shader " << shaderName << " failed to render pixels";
    }
}

// Test multi-resolution voxel rendering
TEST_F(VoxelMeshVisualValidation, MultiResolutionVoxelRendering) {
    // Load basic voxel shader for consistent testing
    ASSERT_TRUE(shaderManager->loadShader("basic_voxel", 
                                         "core/rendering/shaders/basic_voxel_gl33.vert", 
                                         "core/rendering/shaders/basic_voxel_gl33.frag"));
    
    auto shaderId = shaderManager->getShader("basic_voxel");
    ASSERT_NE(shaderId, 0u);
    
    // Test all 10 voxel resolutions
    std::vector<float> resolutions = {
        0.01f,  // 1cm
        0.02f,  // 2cm
        0.04f,  // 4cm
        0.08f,  // 8cm
        0.16f,  // 16cm
        0.32f,  // 32cm
        0.64f,  // 64cm
        1.28f,  // 128cm
        2.56f,  // 256cm
        5.12f   // 512cm
    };
    
    // Common setup
    glEnable(GL_DEPTH_TEST);
    // Use OpenGLRenderer directly to bypass incomplete ShaderProgram wrapper
    renderer->useProgram(shaderId);
    
    // Track pixel counts for each resolution
    std::vector<int> pixelCounts;
    int previousPixelCount = 0;
    
    for (size_t i = 0; i < resolutions.size(); ++i) {
        float size = resolutions[i];
        
        // Create cube at this resolution
        auto cube = createVoxelCube(size, glm::vec3(0.0f, 0.5f, 1.0f)); // Blue gradient
        
        // Clear framebuffer
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set up matrices - keep camera distance fixed to see size differences
        float cameraDistance = 5.0f; // Fixed distance to observe actual size differences
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(
            glm::vec3(cameraDistance, cameraDistance, cameraDistance),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 projection = getProjectionMatrix();
        
        // Set uniforms using OpenGLRenderer
        renderer->setUniform(shaderId, "model", Rendering::UniformValue(glmToMathMatrix(model)));
        renderer->setUniform(shaderId, "view", Rendering::UniformValue(glmToMathMatrix(view)));
        renderer->setUniform(shaderId, "projection", Rendering::UniformValue(glmToMathMatrix(projection)));
        renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(cameraDistance, cameraDistance, cameraDistance)));
        renderer->setUniform(shaderId, "lightPos", Rendering::UniformValue(Math::Vector3f(10.0f, 10.0f, 10.0f)));
        renderer->setUniform(shaderId, "lightColor", Rendering::UniformValue(Math::Vector3f(1.0f, 1.0f, 1.0f)));
        
        // Render
        VAO_BIND(cube->vao);
        glDrawElements(GL_TRIANGLES, cube->indexCount, GL_UNSIGNED_INT, 0);
        VAO_BIND(0);
        
        // Capture and count pixels
        auto pixels = captureFramebuffer();
        int pixelCount = countRenderedPixels(pixels);
        pixelCounts.push_back(pixelCount);
        
        // Validate rendering occurred
        EXPECT_GT(pixelCount, 0) << "Resolution " << size << "m (" << (size * 100) << "cm) failed to render";
        
        // For reasonable voxel sizes, expect some consistent rendering
        // (Very small voxels may not be visible from this distance)
        if (size >= 0.08f) { // 8cm and larger should be visible
            EXPECT_GT(pixelCount, 1000) << "Resolution " << size << "m should render enough pixels to be visible";
        }
        
        previousPixelCount = pixelCount;
        
        // Verify the blue gradient color is present
        EXPECT_TRUE(isColorPresent(pixels, 0, 128, 255, 128)) 
            << "Expected blue color not found at resolution " << size << "m";
    }
    
    // Log pixel counts for debugging
    std::cout << "Multi-resolution pixel counts:\n";
    for (size_t i = 0; i < resolutions.size(); ++i) {
        std::cout << "  " << (resolutions[i] * 100) << "cm: " << pixelCounts[i] << " pixels\n";
    }
    
    // Verify we have a general trend of increasing pixels for first few resolutions
    bool increasingTrend = true;
    for (size_t i = 1; i < std::min(size_t(5), pixelCounts.size()); ++i) {
        if (pixelCounts[i] < pixelCounts[i-1] * 0.8f) { // Allow 20% variance
            increasingTrend = false;
            break;
        }
    }
    EXPECT_TRUE(increasingTrend) << "Expected generally increasing pixel counts for larger voxels";
}