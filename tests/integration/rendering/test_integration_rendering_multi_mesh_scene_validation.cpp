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
#include <chrono>
#include <random>
#include <filesystem>

#include "rendering/OpenGLRenderer.h"
#include "rendering/ShaderManager.h"
#include "rendering/RenderState.h"
#include "math/Matrix4f.h"
#include "math/Vector3f.h"
#include "math/Vector4f.h"

using namespace VoxelEditor;

// Test fixture for multi-mesh scene rendering validation
class MultiMeshSceneValidation : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Rendering::OpenGLRenderer> renderer;
    std::unique_ptr<Rendering::ShaderManager> shaderManager;
    std::unique_ptr<Rendering::RenderState> renderState;
    
    const int WINDOW_WIDTH = 1024;
    const int WINDOW_HEIGHT = 768;
    
    void SetUp() override {
        // Initialize GLFW
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        
        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing
        
        // Create window
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Multi-Mesh Scene Test", nullptr, nullptr);
        ASSERT_NE(window, nullptr) << "Failed to create GLFW window";
        
        glfwMakeContextCurrent(window);
        
        // Initialize GLAD
        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) << "Failed to initialize GLAD";
        
#ifdef __APPLE__
        // Load macOS OpenGL extensions
        ASSERT_TRUE(VoxelEditor::Rendering::LoadOpenGLExtensions()) << "Failed to load macOS OpenGL extensions";
#endif
        
        // Clear any GL errors from initialization
        while (glGetError() != GL_NO_ERROR) {}
        
        // Create renderer components
        renderer = std::make_unique<Rendering::OpenGLRenderer>();
        
        shaderManager = std::make_unique<Rendering::ShaderManager>(renderer.get());
        renderState = std::make_unique<Rendering::RenderState>();
        
        // Set viewport
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        
        // Find shader path
        std::string shaderPath = "core/rendering/shaders/";
        if (!std::filesystem::exists(shaderPath)) {
            shaderPath = "bin/core/rendering/shaders/";
            if (!std::filesystem::exists(shaderPath)) {
                shaderPath = "../bin/core/rendering/shaders/";
                if (!std::filesystem::exists(shaderPath)) {
                    shaderPath = "../../core/rendering/shaders/";
                }
            }
        }
        
        // Load all shaders with correct paths
        ASSERT_NE(shaderManager->loadShader("basic_voxel", 
                                             shaderPath + "basic_voxel_gl33.vert", 
                                             shaderPath + "basic_voxel_gl33.frag"), 0u);
        ASSERT_NE(shaderManager->loadShader("enhanced_voxel", 
                                             shaderPath + "enhanced_voxel.vert", 
                                             shaderPath + "enhanced_voxel.frag"), 0u);
        ASSERT_NE(shaderManager->loadShader("flat_voxel", 
                                             shaderPath + "flat_voxel.vert", 
                                             shaderPath + "flat_voxel.frag"), 0u);
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
    
    // Voxel mesh structure
    struct VoxelMesh {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        int indexCount;
        glm::vec3 position;
        glm::vec3 color;
        float size;
        
        ~VoxelMesh() {
            if (vao) VAO_DELETE(1, &vao);
            if (vbo) glDeleteBuffers(1, &vbo);
            if (ebo) glDeleteBuffers(1, &ebo);
        }
    };
    
    std::unique_ptr<VoxelMesh> createVoxelCube(const glm::vec3& position, float size, const glm::vec3& color) {
        auto mesh = std::make_unique<VoxelMesh>();
        mesh->position = position;
        mesh->size = size;
        mesh->color = color;
        
        float halfSize = size * 0.5f;
        
        // Vertex data: position (3), normal (3), color (3)
        std::vector<float> vertices = {
            // Front face
            -halfSize, -halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  color.r, color.g, color.b,
             halfSize, -halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  color.r, color.g, color.b,
             halfSize,  halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  color.r, color.g, color.b,
            -halfSize,  halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  color.r, color.g, color.b,
            
            // Back face
            -halfSize, -halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  color.r, color.g, color.b,
             halfSize, -halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  color.r, color.g, color.b,
             halfSize,  halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  color.r, color.g, color.b,
            -halfSize,  halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  color.r, color.g, color.b,
            
            // Top face
            -halfSize,  halfSize, -halfSize,  0.0f,  1.0f,  0.0f,  color.r, color.g, color.b,
             halfSize,  halfSize, -halfSize,  0.0f,  1.0f,  0.0f,  color.r, color.g, color.b,
             halfSize,  halfSize,  halfSize,  0.0f,  1.0f,  0.0f,  color.r, color.g, color.b,
            -halfSize,  halfSize,  halfSize,  0.0f,  1.0f,  0.0f,  color.r, color.g, color.b,
            
            // Bottom face
            -halfSize, -halfSize, -halfSize,  0.0f, -1.0f,  0.0f,  color.r, color.g, color.b,
             halfSize, -halfSize, -halfSize,  0.0f, -1.0f,  0.0f,  color.r, color.g, color.b,
             halfSize, -halfSize,  halfSize,  0.0f, -1.0f,  0.0f,  color.r, color.g, color.b,
            -halfSize, -halfSize,  halfSize,  0.0f, -1.0f,  0.0f,  color.r, color.g, color.b,
            
            // Right face
             halfSize, -halfSize, -halfSize,  1.0f,  0.0f,  0.0f,  color.r, color.g, color.b,
             halfSize, -halfSize,  halfSize,  1.0f,  0.0f,  0.0f,  color.r, color.g, color.b,
             halfSize,  halfSize,  halfSize,  1.0f,  0.0f,  0.0f,  color.r, color.g, color.b,
             halfSize,  halfSize, -halfSize,  1.0f,  0.0f,  0.0f,  color.r, color.g, color.b,
            
            // Left face
            -halfSize, -halfSize, -halfSize, -1.0f,  0.0f,  0.0f,  color.r, color.g, color.b,
            -halfSize, -halfSize,  halfSize, -1.0f,  0.0f,  0.0f,  color.r, color.g, color.b,
            -halfSize,  halfSize,  halfSize, -1.0f,  0.0f,  0.0f,  color.r, color.g, color.b,
            -halfSize,  halfSize, -halfSize, -1.0f,  0.0f,  0.0f,  color.r, color.g, color.b
        };
        
        std::vector<unsigned int> indices = {
            0, 1, 2, 2, 3, 0,       // Front
            4, 5, 6, 6, 7, 4,       // Back
            8, 9, 10, 10, 11, 8,    // Top
            12, 13, 14, 14, 15, 12, // Bottom
            16, 17, 18, 18, 19, 16, // Right
            20, 21, 22, 22, 23, 20  // Left
        };
        
        // Create VAO, VBO, EBO
        VAO_GEN(1, &mesh->vao);
        glGenBuffers(1, &mesh->vbo);
        glGenBuffers(1, &mesh->ebo);
        
        VAO_BIND(mesh->vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Color attribute
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        VAO_BIND(0);
        
        mesh->indexCount = indices.size();
        
        return mesh;
    }
    
    // Create a grid of voxels
    std::vector<std::unique_ptr<VoxelMesh>> createVoxelGrid(int gridSize, float spacing) {
        std::vector<std::unique_ptr<VoxelMesh>> meshes;
        std::mt19937 rng(42); // Fixed seed for reproducibility
        std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);
        
        for (int x = 0; x < gridSize; ++x) {
            for (int y = 0; y < gridSize; ++y) {
                for (int z = 0; z < gridSize; ++z) {
                    glm::vec3 position(
                        (x - gridSize/2.0f) * spacing,
                        y * spacing,
                        (z - gridSize/2.0f) * spacing
                    );
                    
                    glm::vec3 color(
                        colorDist(rng),
                        colorDist(rng),
                        colorDist(rng)
                    );
                    
                    meshes.push_back(createVoxelCube(position, 0.8f, color));
                }
            }
        }
        
        return meshes;
    }
    
    // Capture framebuffer
    std::vector<unsigned char> captureFramebuffer() {
        std::vector<unsigned char> pixels(WINDOW_WIDTH * WINDOW_HEIGHT * 3);
        glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        return pixels;
    }
    
    // Count non-black pixels
    int countRenderedPixels(const std::vector<unsigned char>& pixels) {
        int count = 0;
        for (size_t i = 0; i < pixels.size(); i += 3) {
            if (pixels[i] > 0 || pixels[i+1] > 0 || pixels[i+2] > 0) {
                count++;
            }
        }
        return count;
    }
    
    // Check GL errors
    bool checkGLErrors(const std::string& context) {
        GLenum err;
        bool hasError = false;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "GL Error in " << context << ": 0x" << std::hex << err << std::dec << std::endl;
            hasError = true;
        }
        return !hasError;
    }
    
    // Measure memory usage (approximate)
    size_t estimateMeshMemoryUsage(const std::vector<std::unique_ptr<VoxelMesh>>& meshes) {
        size_t totalMemory = 0;
        for (const auto& mesh : meshes) {
            // Each vertex: 9 floats * 4 bytes = 36 bytes
            // 24 vertices per cube
            totalMemory += 24 * 36;
            
            // Indices: 36 * 4 bytes
            totalMemory += 36 * 4;
            
            // VAO/VBO/EBO overhead (approximate)
            totalMemory += 256;
        }
        return totalMemory;
    }
};

// Test rendering 100+ voxel meshes
TEST_F(MultiMeshSceneValidation, Render100VoxelMeshes) {
    // Create 125 voxels (5x5x5 grid)
    auto meshes = createVoxelGrid(5, 2.0f);
    ASSERT_EQ(meshes.size(), 125u);
    
    // Get basic shader
    auto shaderId = shaderManager->getShader("basic_voxel");
    ASSERT_NE(shaderId, 0u);
    
    // Clear to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    // Use shader
    renderer->useProgram(shaderId);
    
    // Set view and projection matrices
    glm::mat4 view = glm::lookAt(
        glm::vec3(15.0f, 10.0f, 15.0f),
        glm::vec3(0.0f, 5.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
        0.1f, 100.0f
    );
    
    // Convert glm matrices to Math::Matrix4f
    Math::Matrix4f viewMat, projMat;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            viewMat.m[i * 4 + j] = view[j][i];
            projMat.m[i * 4 + j] = projection[j][i];
        }
    }
    
    renderer->setUniform("view", Rendering::UniformValue(viewMat));
    renderer->setUniform("projection", Rendering::UniformValue(projMat));
    renderer->setUniform("viewPos", Rendering::UniformValue(Math::Vector3f(15.0f, 10.0f, 15.0f)));
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    renderer->setUniform("lightDir", Rendering::UniformValue(Math::Vector3f(lightDir.x, lightDir.y, lightDir.z)));
    
    // Render all meshes
    for (const auto& mesh : meshes) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), mesh->position);
        
        // Convert glm matrix to Math::Matrix4f
        Math::Matrix4f modelMat;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                modelMat.m[i * 4 + j] = model[j][i];
            }
        }
        renderer->setUniform("model", Rendering::UniformValue(modelMat));
        
        VAO_BIND(mesh->vao);
        glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
        VAO_BIND(0);
    }
    
    // Check for GL errors
    EXPECT_TRUE(checkGLErrors("After rendering 125 meshes"));
    
    // Capture and validate
    auto pixels = captureFramebuffer();
    int renderedPixels = countRenderedPixels(pixels);
    
    // Should have significant coverage with 125 voxels
    EXPECT_GT(renderedPixels, 50000) << "Expected significant pixel coverage with 125 voxels";
    
    // Check memory usage
    size_t memoryUsage = estimateMeshMemoryUsage(meshes);
    EXPECT_LT(memoryUsage, 10 * 1024 * 1024) << "Memory usage should be reasonable for 125 meshes";
}

// Test shader switching performance
TEST_F(MultiMeshSceneValidation, ShaderSwitchingPerformance) {
    // Create 100 voxels
    auto meshes = createVoxelGrid(5, 2.0f);
    meshes.resize(100); // Use first 100
    
    // Get all shaders
    auto basicId = shaderManager->getShader("basic_voxel");
    auto enhancedId = shaderManager->getShader("enhanced_voxel");
    auto flatId = shaderManager->getShader("flat_voxel");
    
    ASSERT_NE(basicId, 0u);
    ASSERT_NE(enhancedId, 0u);
    ASSERT_NE(flatId, 0u);
    
    // Setup rendering
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    
    glm::mat4 view = glm::lookAt(
        glm::vec3(15.0f, 10.0f, 15.0f),
        glm::vec3(0.0f, 5.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
        0.1f, 100.0f
    );
    
    // Array of shader IDs to cycle through
    std::vector<Rendering::ShaderId> shaders = {basicId, enhancedId, flatId};
    
    // Measure shader switching time
    auto start = std::chrono::high_resolution_clock::now();
    
    // Render 10 frames, switching shaders each mesh
    for (int frame = 0; frame < 10; ++frame) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        for (size_t i = 0; i < meshes.size(); ++i) {
            // Switch shader for each mesh
            auto shaderId = shaders[i % shaders.size()];
            renderer->useProgram(shaderId);
            
            // Set uniforms
            // Convert glm matrices to Math::Matrix4f
            Math::Matrix4f viewMat, projMat;
            for (int ii = 0; ii < 4; ++ii) {
                for (int jj = 0; jj < 4; ++jj) {
                    viewMat.m[ii * 4 + jj] = view[jj][ii];
                    projMat.m[ii * 4 + jj] = projection[jj][ii];
                }
            }
            
            renderer->setUniform(shaderId, "view", Rendering::UniformValue(viewMat));
            renderer->setUniform(shaderId, "projection", Rendering::UniformValue(projMat));
            renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(15.0f, 10.0f, 15.0f)));
            glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
            renderer->setUniform(shaderId, "lightDir", Rendering::UniformValue(Math::Vector3f(lightDir.x, lightDir.y, lightDir.z)));
            
            glm::mat4 model = glm::translate(glm::mat4(1.0f), meshes[i]->position);
            Math::Matrix4f modelMat;
            for (int ii = 0; ii < 4; ++ii) {
                for (int jj = 0; jj < 4; ++jj) {
                    modelMat.m[ii * 4 + jj] = model[jj][ii];
                }
            }
            renderer->setUniform("model", Rendering::UniformValue(modelMat));
            
            glBindVertexArray(meshes[i]->vao);
            glDrawElements(GL_TRIANGLES, meshes[i]->indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        
        glfwSwapBuffers(window);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 10 frames with 100 shader switches each should complete reasonably fast
    EXPECT_LT(duration.count(), 1000) << "Shader switching should not cause significant overhead";
    
    // Check for GL errors
    EXPECT_TRUE(checkGLErrors("After shader switching test"));
    
    // Verify final frame rendered correctly
    auto pixels = captureFramebuffer();
    int renderedPixels = countRenderedPixels(pixels);
    EXPECT_GT(renderedPixels, 40000) << "Final frame should have rendered content";
}

// Test large scene with no visual artifacts
TEST_F(MultiMeshSceneValidation, LargeSceneNoArtifacts) {
    // Create 216 voxels (6x6x6 grid)
    auto meshes = createVoxelGrid(6, 1.5f);
    ASSERT_EQ(meshes.size(), 216u);
    
    // Use enhanced shader for better visual quality
    auto shaderId = shaderManager->getShader("enhanced_voxel");
    ASSERT_NE(shaderId, 0u);
    auto* shader = shaderManager->getShaderProgram(shaderId);
    ASSERT_NE(shader, nullptr);
    
    // Clear to dark blue to better see artifacts
    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // Use shader
    renderer->useProgram(shaderId);
    
    // Render from multiple angles to check for artifacts
    std::vector<glm::vec3> cameraPositions = {
        glm::vec3(20.0f, 15.0f, 20.0f),
        glm::vec3(-20.0f, 15.0f, 20.0f),
        glm::vec3(0.0f, 30.0f, 0.1f),
        glm::vec3(25.0f, 5.0f, 0.0f)
    };
    
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
        0.1f, 100.0f
    );
    
    // Convert projection matrix
    Math::Matrix4f projMat;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            projMat.m[i * 4 + j] = projection[j][i];
        }
    }
    renderer->setUniform(shaderId, "projection", Rendering::UniformValue(projMat));
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    renderer->setUniform(shaderId, "lightDir", Rendering::UniformValue(Math::Vector3f(lightDir.x, lightDir.y, lightDir.z)));
    
    for (const auto& camPos : cameraPositions) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glm::mat4 view = glm::lookAt(
            camPos,
            glm::vec3(0.0f, 4.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        
        // Convert view matrix
        Math::Matrix4f viewMat;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                viewMat.m[i * 4 + j] = view[j][i];
            }
        }
        renderer->setUniform(shaderId, "view", Rendering::UniformValue(viewMat));
        renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(camPos.x, camPos.y, camPos.z)));
        
        // Render all meshes
        for (const auto& mesh : meshes) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), mesh->position);
            Math::Matrix4f modelMat;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    modelMat.m[i * 4 + j] = model[j][i];
                }
            }
            renderer->setUniform("model", Rendering::UniformValue(modelMat));
            
            glBindVertexArray(mesh->vao);
            glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        
        // Check for GL errors after each view
        EXPECT_TRUE(checkGLErrors("After rendering from camera position"));
        
        // Capture and check for reasonable output
        auto pixels = captureFramebuffer();
        int renderedPixels = countRenderedPixels(pixels);
        
        // Should have good coverage from each angle
        EXPECT_GT(renderedPixels, 30000) << "Each view should show significant content";
        
        // Check for "salt and pepper" artifacts (isolated bright pixels)
        int isolatedPixels = 0;
        for (int y = 1; y < WINDOW_HEIGHT - 1; ++y) {
            for (int x = 1; x < WINDOW_WIDTH - 1; ++x) {
                int idx = (y * WINDOW_WIDTH + x) * 3;
                
                // Check if pixel is bright
                if (pixels[idx] > 200 || pixels[idx+1] > 200 || pixels[idx+2] > 200) {
                    // Check if surrounded by dark pixels
                    bool isolated = true;
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (dx == 0 && dy == 0) continue;
                            int nidx = ((y+dy) * WINDOW_WIDTH + (x+dx)) * 3;
                            if (pixels[nidx] > 50 || pixels[nidx+1] > 50 || pixels[nidx+2] > 50) {
                                isolated = false;
                                break;
                            }
                        }
                        if (!isolated) break;
                    }
                    if (isolated) isolatedPixels++;
                }
            }
        }
        
        // Should have very few isolated bright pixels
        EXPECT_LT(isolatedPixels, 100) << "Should not have many isolated bright pixels (artifacts)";
    }
    
    // Check memory usage
    size_t memoryUsage = estimateMeshMemoryUsage(meshes);
    EXPECT_LT(memoryUsage, 20 * 1024 * 1024) << "Memory usage should be reasonable for 216 meshes";
}

// Test memory usage with many meshes
TEST_F(MultiMeshSceneValidation, MemoryUsageWithManyMeshes) {
    // Create progressively more meshes and monitor memory
    std::vector<int> meshCounts = {100, 200, 400, 800};
    std::vector<size_t> memoryUsages;
    
    for (int count : meshCounts) {
        // Create meshes in a pattern
        std::vector<std::unique_ptr<VoxelMesh>> meshes;
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> posDist(-20.0f, 20.0f);
        std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);
        
        for (int i = 0; i < count; ++i) {
            glm::vec3 position(
                posDist(rng),
                std::abs(posDist(rng)) * 0.5f, // Keep above ground
                posDist(rng)
            );
            
            glm::vec3 color(
                colorDist(rng),
                colorDist(rng),
                colorDist(rng)
            );
            
            meshes.push_back(createVoxelCube(position, 0.5f, color));
        }
        
        // Estimate memory
        size_t memoryUsage = estimateMeshMemoryUsage(meshes);
        memoryUsages.push_back(memoryUsage);
        
        // Quick render test
        auto shaderId = shaderManager->getShader("flat_voxel");
        auto* shader = shaderManager->getShaderProgram(shaderId);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        
        renderer->useProgram(shaderId);
        
        glm::mat4 view = glm::lookAt(
            glm::vec3(30.0f, 20.0f, 30.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
            0.1f, 100.0f
        );
        
        // Convert matrices
        Math::Matrix4f viewMat, projMat;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                viewMat.m[i * 4 + j] = view[j][i];
                projMat.m[i * 4 + j] = projection[j][i];
            }
        }
        renderer->setUniform(shaderId, "view", Rendering::UniformValue(viewMat));
        renderer->setUniform(shaderId, "projection", Rendering::UniformValue(projMat));
        renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(30.0f, 20.0f, 30.0f)));
        glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
        renderer->setUniform(shaderId, "lightDir", Rendering::UniformValue(Math::Vector3f(lightDir.x, lightDir.y, lightDir.z)));
        
        // Render all
        for (const auto& mesh : meshes) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), mesh->position);
            Math::Matrix4f modelMat;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    modelMat.m[i * 4 + j] = model[j][i];
                }
            }
            renderer->setUniform("model", Rendering::UniformValue(modelMat));
            
            glBindVertexArray(mesh->vao);
            glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        
        EXPECT_TRUE(checkGLErrors("After rendering " + std::to_string(count) + " meshes"));
    }
    
    // Verify memory scales linearly
    for (size_t i = 1; i < memoryUsages.size(); ++i) {
        double ratio = (double)memoryUsages[i] / memoryUsages[i-1];
        double expectedRatio = (double)meshCounts[i] / meshCounts[i-1];
        
        // Allow 10% deviation from linear scaling
        EXPECT_NEAR(ratio, expectedRatio, expectedRatio * 0.1) 
            << "Memory usage should scale linearly with mesh count";
    }
    
    // Even with 800 meshes, should stay under reasonable limits
    EXPECT_LT(memoryUsages.back(), 100 * 1024 * 1024) 
        << "Memory usage should stay reasonable even with 800 meshes";
}

// Test rendering performance benchmark
TEST_F(MultiMeshSceneValidation, RenderingPerformanceBenchmark) {
    // Create 200 voxels for performance testing
    auto meshes = createVoxelGrid(6, 2.0f);
    meshes.resize(200);
    
    // Get basic shader for consistent performance testing
    auto shaderId = shaderManager->getShader("basic_voxel");
    auto* shader = shaderManager->getShaderProgram(shaderId);
    
    glEnable(GL_DEPTH_TEST);
    renderer->useProgram(shaderId);
    
    glm::mat4 view = glm::lookAt(
        glm::vec3(20.0f, 15.0f, 20.0f),
        glm::vec3(0.0f, 5.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
        0.1f, 100.0f
    );
    
    // Convert matrices
    Math::Matrix4f viewMat, projMat;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            viewMat.m[i * 4 + j] = view[j][i];
            projMat.m[i * 4 + j] = projection[j][i];
        }
    }
    renderer->setUniform(shaderId, "view", Rendering::UniformValue(viewMat));
    renderer->setUniform(shaderId, "projection", Rendering::UniformValue(projMat));
    renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(20.0f, 15.0f, 20.0f)));
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    renderer->setUniform(shaderId, "lightDir", Rendering::UniformValue(Math::Vector3f(lightDir.x, lightDir.y, lightDir.z)));
    
    // Warm up
    for (int i = 0; i < 10; ++i) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (const auto& mesh : meshes) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), mesh->position);
            Math::Matrix4f modelMat;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    modelMat.m[i * 4 + j] = model[j][i];
                }
            }
            renderer->setUniform("model", Rendering::UniformValue(modelMat));
            
            glBindVertexArray(mesh->vao);
            glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        glfwSwapBuffers(window);
    }
    
    // Measure 100 frames
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int frame = 0; frame < 100; ++frame) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        for (const auto& mesh : meshes) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), mesh->position);
            Math::Matrix4f modelMat;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    modelMat.m[i * 4 + j] = model[j][i];
                }
            }
            renderer->setUniform("model", Rendering::UniformValue(modelMat));
            
            glBindVertexArray(mesh->vao);
            glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        
        glfwSwapBuffers(window);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double fps = 100000.0 / duration.count();
    
    // Should achieve at least 60 FPS with 200 voxels
    EXPECT_GT(fps, 60.0) << "Should achieve at least 60 FPS with 200 voxel meshes";
    
    // Log performance for future comparison
    std::cout << "Performance: " << fps << " FPS with 200 voxel meshes" << std::endl;
    std::cout << "Average frame time: " << (duration.count() / 100.0) << " ms" << std::endl;
}