#include <gtest/gtest.h>
extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <memory>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <sstream>

#include "rendering/OpenGLRenderer.h"
#include "rendering/ShaderManager.h"
#include "rendering/RenderState.h"
#include "math/Matrix4f.h"
#include "math/Vector3f.h"
#include "math/Vector4f.h"
#include "PixelValidationHelpers.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Testing;

// Test fixture for shader performance benchmarking
class ShaderPerformanceBenchmark : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Rendering::OpenGLRenderer> renderer;
    std::unique_ptr<Rendering::ShaderManager> shaderManager;
    std::unique_ptr<Rendering::RenderState> renderState;
    
    const int WINDOW_WIDTH = 1920;
    const int WINDOW_HEIGHT = 1080;
    
    // Benchmark configuration
    const int WARMUP_FRAMES = 100;
    const int BENCHMARK_FRAMES = 1000;
    const int MESH_COUNT_SMALL = 100;
    const int MESH_COUNT_MEDIUM = 500;
    const int MESH_COUNT_LARGE = 1000;
    
    void SetUp() override {
        // Initialize GLFW
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        
        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing
        
        // Create window
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Shader Performance Benchmark", nullptr, nullptr);
        ASSERT_NE(window, nullptr) << "Failed to create GLFW window";
        
        glfwMakeContextCurrent(window);
        
        // Initialize GLAD
        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) << "Failed to initialize GLAD";
        
        // Clear any GL errors from initialization
        while (glGetError() != GL_NO_ERROR) {}
        
        // Create renderer components
        renderer = std::make_unique<Rendering::OpenGLRenderer>();
        Rendering::RenderConfig config;
        ASSERT_TRUE(renderer->initializeContext(config)) << "Failed to initialize renderer context";
        
        shaderManager = std::make_unique<Rendering::ShaderManager>();
        renderState = std::make_unique<Rendering::RenderState>();
        
        // Set viewport
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        
        // Load all shader variants
        loadAllShaders();
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
    
    void loadAllShaders() {
        // Try to load all available shader variants
        shaderManager->loadShader("basic_voxel", "basic_voxel_gl33.vert", "basic_voxel_gl33.frag");
        shaderManager->loadShader("enhanced_voxel", "enhanced_voxel.vert", "enhanced_voxel.frag");
        shaderManager->loadShader("flat_voxel", "flat_voxel.vert", "flat_voxel.frag");
    }
    
    // Voxel mesh structure for benchmarking
    struct VoxelMesh {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        int indexCount;
        glm::vec3 position;
        glm::vec3 color;
        float size;
        
        ~VoxelMesh() {
            // Note: VAO/VBO/EBO cleanup handled by renderer when test completes
            // Individual cleanup would require renderer context which isn't available here
        }
    };
    
    std::unique_ptr<VoxelMesh> createVoxelCube(float size, const glm::vec3& color, const glm::vec3& position) {
        auto mesh = std::make_unique<VoxelMesh>();
        mesh->position = position;
        mesh->color = color;
        mesh->size = size;
        
        float halfSize = size * 0.5f;
        
        // Create cube vertices with normals
        std::vector<float> vertices;
        
        // Define all 8 vertices of the cube
        glm::vec3 positions[8] = {
            glm::vec3(-halfSize, -halfSize,  halfSize), // 0
            glm::vec3( halfSize, -halfSize,  halfSize), // 1
            glm::vec3( halfSize,  halfSize,  halfSize), // 2
            glm::vec3(-halfSize,  halfSize,  halfSize), // 3
            glm::vec3(-halfSize, -halfSize, -halfSize), // 4
            glm::vec3( halfSize, -halfSize, -halfSize), // 5
            glm::vec3( halfSize,  halfSize, -halfSize), // 6
            glm::vec3(-halfSize,  halfSize, -halfSize)  // 7
        };
        
        // Define faces with proper normals
        struct Face {
            int indices[4];
            glm::vec3 normal;
        };
        
        Face faces[6] = {
            {{0, 1, 2, 3}, glm::vec3(0, 0, 1)},   // Front
            {{5, 4, 7, 6}, glm::vec3(0, 0, -1)},  // Back
            {{4, 0, 3, 7}, glm::vec3(-1, 0, 0)},  // Left
            {{1, 5, 6, 2}, glm::vec3(1, 0, 0)},   // Right
            {{3, 2, 6, 7}, glm::vec3(0, 1, 0)},   // Top
            {{4, 5, 1, 0}, glm::vec3(0, -1, 0)}   // Bottom
        };
        
        // Build vertex array for each face
        for (const auto& face : faces) {
            for (int i = 0; i < 4; ++i) {
                const glm::vec3& pos = positions[face.indices[i]];
                // Position
                vertices.push_back(pos.x);
                vertices.push_back(pos.y);
                vertices.push_back(pos.z);
                // Normal
                vertices.push_back(face.normal.x);
                vertices.push_back(face.normal.y);
                vertices.push_back(face.normal.z);
                // Color
                vertices.push_back(color.r);
                vertices.push_back(color.g);
                vertices.push_back(color.b);
            }
        }
        
        // Create indices for triangles
        std::vector<unsigned int> indices;
        for (int face = 0; face < 6; ++face) {
            int baseIndex = face * 4;
            // Two triangles per face
            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 3);
        }
        
        mesh->indexCount = indices.size();
        
        // Create OpenGL objects
        mesh->vao = renderer->createVertexArray();
        glGenBuffers(1, &mesh->vbo);
        glGenBuffers(1, &mesh->ebo);
        
        renderer->bindVertexArray(mesh->vao);
        
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
        
        renderer->bindVertexArray(0);
        
        return mesh;
    }
    
    // Create a grid of voxel meshes
    std::vector<std::unique_ptr<VoxelMesh>> createVoxelGrid(int count) {
        std::vector<std::unique_ptr<VoxelMesh>> meshes;
        
        int gridSize = static_cast<int>(std::ceil(std::cbrt(count)));
        float spacing = 2.0f;
        float offset = -gridSize * spacing / 2.0f;
        
        for (int i = 0; i < count; ++i) {
            int x = i % gridSize;
            int y = (i / gridSize) % gridSize;
            int z = i / (gridSize * gridSize);
            
            glm::vec3 position(
                offset + x * spacing,
                offset + y * spacing,
                offset + z * spacing
            );
            
            // Vary colors for visual distinction
            glm::vec3 color(
                0.3f + 0.7f * (float)x / gridSize,
                0.3f + 0.7f * (float)y / gridSize,
                0.3f + 0.7f * (float)z / gridSize
            );
            
            meshes.push_back(createVoxelCube(1.0f, color, position));
        }
        
        return meshes;
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
    
    // Benchmark result structure
    struct BenchmarkResult {
        double averageFPS;
        double minFPS;
        double maxFPS;
        double stdDevFPS;
        double averageFrameTime; // milliseconds
        double percentile95;
        double percentile99;
        std::vector<double> frameTimes;
        
        std::string toString() const {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2);
            ss << "Average FPS: " << averageFPS << "\n";
            ss << "Min FPS: " << minFPS << "\n";
            ss << "Max FPS: " << maxFPS << "\n";
            ss << "Std Dev: " << stdDevFPS << "\n";
            ss << "Avg Frame Time: " << averageFrameTime << " ms\n";
            ss << "95th percentile: " << percentile95 << " ms\n";
            ss << "99th percentile: " << percentile99 << " ms";
            return ss.str();
        }
    };
    
    // Run a benchmark with given parameters  
    BenchmarkResult runBenchmark(const std::string& shaderName, 
                                const std::vector<std::unique_ptr<VoxelMesh>>& meshes,
                                int frameCount = 1000) {
        BenchmarkResult result;
        result.frameTimes.reserve(frameCount);
        
        // Get shader
        auto shaderId = shaderManager->getShader(shaderName);
        if (shaderId == 0) {
            result.averageFPS = 0;
            return result;
        }
        
        auto* shader = shaderManager->getShaderProgram(shaderId);
        if (!shader) {
            result.averageFPS = 0;
            return result;
        }
        
        // Setup rendering state
        glEnable(GL_DEPTH_TEST);
        shader->use();
        
        // Setup view and projection matrices
        glm::mat4 view = glm::lookAt(
            glm::vec3(30, 30, 30),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)WINDOW_WIDTH / WINDOW_HEIGHT,
            0.1f, 200.0f
        );
        
        // Set constant uniforms
        shader->setUniform("view", glmToMathMatrix(view));
        shader->setUniform("projection", glmToMathMatrix(projection));
        shader->setUniform("viewPos", Math::Vector3f(30.0f, 30.0f, 30.0f));
        glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
        shader->setUniform("lightDir", Math::Vector3f(lightDir.x, lightDir.y, lightDir.z));
        
        // Warmup frames
        for (int i = 0; i < WARMUP_FRAMES; ++i) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            for (const auto& mesh : meshes) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), mesh->position);
                shader->setUniform("model", glmToMathMatrix(model));
                
                renderer->bindVertexArray(mesh->vao);
                glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
            }
            
            glfwSwapBuffers(window);
        }
        
        // Benchmark frames
        auto benchmarkStart = std::chrono::high_resolution_clock::now();
        
        for (int frame = 0; frame < frameCount; ++frame) {
            auto frameStart = std::chrono::high_resolution_clock::now();
            
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            // Animate meshes slightly for more realistic benchmark
            float time = frame * 0.01f;
            
            for (const auto& mesh : meshes) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), mesh->position);
                model = glm::rotate(model, time, glm::vec3(0, 1, 0));
                shader->setUniform("model", glmToMathMatrix(model));
                
                renderer->bindVertexArray(mesh->vao);
                glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
            }
            
            glfwSwapBuffers(window);
            
            auto frameEnd = std::chrono::high_resolution_clock::now();
            double frameTime = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
            result.frameTimes.push_back(frameTime);
        }
        
        auto benchmarkEnd = std::chrono::high_resolution_clock::now();
        double totalTime = std::chrono::duration<double>(benchmarkEnd - benchmarkStart).count();
        
        // Calculate statistics
        result.averageFPS = frameCount / totalTime;
        result.averageFrameTime = std::accumulate(result.frameTimes.begin(), result.frameTimes.end(), 0.0) / frameCount;
        
        // Min/Max FPS (from frame times)
        auto minMaxTime = std::minmax_element(result.frameTimes.begin(), result.frameTimes.end());
        result.maxFPS = 1000.0 / *minMaxTime.first;  // Best FPS = lowest frame time
        result.minFPS = 1000.0 / *minMaxTime.second; // Worst FPS = highest frame time
        
        // Standard deviation
        double variance = 0;
        for (double ft : result.frameTimes) {
            variance += std::pow(ft - result.averageFrameTime, 2);
        }
        variance /= frameCount;
        result.stdDevFPS = std::sqrt(variance) * 1000.0 / (result.averageFrameTime * result.averageFrameTime);
        
        // Percentiles
        std::vector<double> sortedTimes = result.frameTimes;
        std::sort(sortedTimes.begin(), sortedTimes.end());
        result.percentile95 = sortedTimes[static_cast<size_t>(frameCount * 0.95)];
        result.percentile99 = sortedTimes[static_cast<size_t>(frameCount * 0.99)];
        
        renderer->bindVertexArray(0);
        
        return result;
    }
};

// Test FPS with different shaders
TEST_F(ShaderPerformanceBenchmark, FPSComparison) {
    // Create test scene
    auto meshes = createVoxelGrid(MESH_COUNT_MEDIUM);
    
    std::vector<std::string> shaderNames = {"basic_voxel", "enhanced_voxel", "flat_voxel"};
    std::map<std::string, BenchmarkResult> results;
    
    for (const auto& shaderName : shaderNames) {
        std::cout << "\nBenchmarking " << shaderName << " shader..." << std::endl;
        results[shaderName] = runBenchmark(shaderName, meshes);
        
        if (results[shaderName].averageFPS > 0) {
            std::cout << results[shaderName].toString() << std::endl;
        } else {
            std::cout << "Shader not available" << std::endl;
        }
    }
    
    // Verify at least one shader achieves target FPS
    bool hasGoodPerformance = false;
    for (const auto& [name, result] : results) {
        if (result.averageFPS >= 60.0) {
            hasGoodPerformance = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasGoodPerformance) << "At least one shader should achieve 60+ FPS with " 
                                   << MESH_COUNT_MEDIUM << " meshes";
}

// Test shader switching overhead
TEST_F(ShaderPerformanceBenchmark, ShaderSwitchingOverhead) {
    // Create small test scene
    auto meshes = createVoxelGrid(MESH_COUNT_SMALL);
    
    std::vector<std::string> shaderNames = {"basic_voxel", "enhanced_voxel", "flat_voxel"};
    
    // Remove any shaders that don't exist
    shaderNames.erase(
        std::remove_if(shaderNames.begin(), shaderNames.end(),
            [this](const std::string& name) {
                return shaderManager->getShader(name) == 0;
            }),
        shaderNames.end()
    );
    
    if (shaderNames.size() < 2) {
        GTEST_SKIP() << "Need at least 2 shaders for switching test";
    }
    
    // Measure rendering without switching
    auto noSwitchStart = std::chrono::high_resolution_clock::now();
    
    glEnable(GL_DEPTH_TEST);
    
    for (int frame = 0; frame < 100; ++frame) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        auto* shader = shaderManager->getShaderProgram(shaderManager->getShader(shaderNames[0]));
        shader->use();
        
        // Setup uniforms
        glm::mat4 view = glm::lookAt(glm::vec3(20, 20, 20), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                              (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);
        shader->setUniform("view", glmToMathMatrix(view));
        shader->setUniform("projection", glmToMathMatrix(projection));
        shader->setUniform("viewPos", Math::Vector3f(20.0f, 20.0f, 20.0f));
        shader->setUniform("lightDir", Math::Vector3f(1.0f, 1.0f, 1.0f));
        
        for (const auto& mesh : meshes) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), mesh->position);
            shader->setUniform("model", glmToMathMatrix(model));
            
            renderer->bindVertexArray(mesh->vao);
            glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
        }
        
        glfwSwapBuffers(window);
    }
    
    auto noSwitchEnd = std::chrono::high_resolution_clock::now();
    double noSwitchTime = std::chrono::duration<double, std::milli>(noSwitchEnd - noSwitchStart).count();
    
    // Measure rendering with shader switching
    auto switchStart = std::chrono::high_resolution_clock::now();
    
    for (int frame = 0; frame < 100; ++frame) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Switch shaders every mesh
        int meshIndex = 0;
        for (const auto& mesh : meshes) {
            // Use different shader for each mesh
            const std::string& shaderName = shaderNames[meshIndex % shaderNames.size()];
            auto* shader = shaderManager->getShaderProgram(shaderManager->getShader(shaderName));
            shader->use();
            
            // Setup uniforms (repeated for each shader switch)
            glm::mat4 view = glm::lookAt(glm::vec3(20, 20, 20), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                                  (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);
            shader->setUniform("view", glmToMathMatrix(view));
            shader->setUniform("projection", glmToMathMatrix(projection));
            shader->setUniform("viewPos", Math::Vector3f(20.0f, 20.0f, 20.0f));
            shader->setUniform("lightDir", Math::Vector3f(1.0f, 1.0f, 1.0f));
            
            glm::mat4 model = glm::translate(glm::mat4(1.0f), mesh->position);
            shader->setUniform("model", glmToMathMatrix(model));
            
            renderer->bindVertexArray(mesh->vao);
            glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
            
            meshIndex++;
        }
        
        glfwSwapBuffers(window);
    }
    
    auto switchEnd = std::chrono::high_resolution_clock::now();
    double switchTime = std::chrono::duration<double, std::milli>(switchEnd - switchStart).count();
    
    // Calculate overhead
    double overhead = (switchTime - noSwitchTime) / noSwitchTime * 100.0;
    
    std::cout << "\nShader Switching Overhead Test:" << std::endl;
    std::cout << "No switching: " << noSwitchTime << " ms" << std::endl;
    std::cout << "With switching: " << switchTime << " ms" << std::endl;
    std::cout << "Overhead: " << overhead << "%" << std::endl;
    
    // Shader switching should not add more than 50% overhead
    EXPECT_LT(overhead, 50.0) << "Shader switching overhead is too high";
}

// Test vertex processing throughput
TEST_F(ShaderPerformanceBenchmark, VertexProcessingThroughput) {
    // Test with increasing vertex counts
    std::vector<int> meshCounts = {10, 50, 100, 200, 500};
    std::map<int, double> verticesPerSecond;
    
    const std::string shaderName = "basic_voxel";
    auto shaderId = shaderManager->getShader(shaderName);
    if (shaderId == 0) {
        GTEST_SKIP() << "Basic voxel shader not available";
    }
    
    for (int count : meshCounts) {
        auto meshes = createVoxelGrid(count);
        
        // Count total vertices
        int totalVertices = 0;
        for (const auto& mesh : meshes) {
            totalVertices += mesh->indexCount; // Each index references a vertex
        }
        
        // Run short benchmark
        auto result = runBenchmark(shaderName, meshes, 100);
        
        // Calculate vertices per second
        double vps = totalVertices * result.averageFPS;
        verticesPerSecond[count] = vps;
        
        std::cout << "\nVertex throughput with " << count << " meshes: " 
                  << std::scientific << vps << " vertices/second" << std::endl;
    }
    
    // Verify vertex throughput scales reasonably
    // Should handle at least 1 million vertices per second on modern hardware
    bool hasGoodThroughput = false;
    for (const auto& [count, vps] : verticesPerSecond) {
        if (vps >= 1e6) {
            hasGoodThroughput = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasGoodThroughput) << "Vertex processing throughput should exceed 1M vertices/second";
}

// Test performance across shader variants
TEST_F(ShaderPerformanceBenchmark, ShaderVariantComparison) {
    // Create different complexity scenes
    struct SceneConfig {
        std::string name;
        int meshCount;
        float expectedMinFPS;
    };
    
    std::vector<SceneConfig> scenes = {
        {"Small Scene", 50, 120.0f},
        {"Medium Scene", 200, 60.0f},
        {"Large Scene", 500, 30.0f}
    };
    
    std::vector<std::string> shaderNames = {"basic_voxel", "enhanced_voxel", "flat_voxel"};
    
    for (const auto& scene : scenes) {
        std::cout << "\n=== " << scene.name << " (" << scene.meshCount << " meshes) ===" << std::endl;
        
        auto meshes = createVoxelGrid(scene.meshCount);
        
        for (const auto& shaderName : shaderNames) {
            auto result = runBenchmark(shaderName, meshes, 500);
            
            if (result.averageFPS > 0) {
                std::cout << shaderName << ": " << result.averageFPS << " FPS" << std::endl;
                
                // Check if performance meets expectations
                if (shaderName == "basic_voxel") {
                    EXPECT_GE(result.averageFPS, scene.expectedMinFPS) 
                        << "Basic shader should achieve at least " << scene.expectedMinFPS 
                        << " FPS in " << scene.name;
                }
            }
        }
    }
}

// Test frame time consistency
TEST_F(ShaderPerformanceBenchmark, FrameTimeConsistency) {
    auto meshes = createVoxelGrid(MESH_COUNT_MEDIUM);
    
    const std::string shaderName = "basic_voxel";
    auto result = runBenchmark(shaderName, meshes, 500);
    
    if (result.averageFPS == 0) {
        GTEST_SKIP() << "Basic voxel shader not available";
    }
    
    // Calculate frame time variance metrics
    double maxDeviation = 0;
    int spikes = 0;
    const double spikeThreshold = result.averageFrameTime * 2.0; // 2x average is a spike
    
    for (double frameTime : result.frameTimes) {
        double deviation = std::abs(frameTime - result.averageFrameTime);
        maxDeviation = std::max(maxDeviation, deviation);
        
        if (frameTime > spikeThreshold) {
            spikes++;
        }
    }
    
    double spikePercentage = (double)spikes / result.frameTimes.size() * 100.0;
    
    std::cout << "\nFrame Time Consistency Analysis:" << std::endl;
    std::cout << "Average frame time: " << result.averageFrameTime << " ms" << std::endl;
    std::cout << "Max deviation: " << maxDeviation << " ms" << std::endl;
    std::cout << "95th percentile: " << result.percentile95 << " ms" << std::endl;
    std::cout << "99th percentile: " << result.percentile99 << " ms" << std::endl;
    std::cout << "Frame spikes (>2x avg): " << spikePercentage << "%" << std::endl;
    
    // Verify frame times are consistent
    EXPECT_LT(result.percentile95, result.averageFrameTime * 1.5) 
        << "95% of frames should be within 1.5x average frame time";
    
    EXPECT_LT(result.percentile99, result.averageFrameTime * 2.0) 
        << "99% of frames should be within 2x average frame time";
    
    EXPECT_LT(spikePercentage, 1.0) 
        << "Less than 1% of frames should have significant spikes";
}

// Memory usage benchmark
TEST_F(ShaderPerformanceBenchmark, MemoryUsageBenchmark) {
    // Get initial GPU memory state (if available)
    GLint totalMemory = 0;
    GLint availableMemory = 0;
    
    // Try to get GPU memory info (NVIDIA extension)
    // Note: This is vendor-specific and may not be available on all GPUs
    const GLint GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX = 0x9048;
    const GLint GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX = 0x9049;
    
    glGetIntegerv(GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalMemory);
    glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availableMemory);
    
    struct MemoryTest {
        int meshCount;
        double fps;
        int memoryUsed; // KB
    };
    
    std::vector<MemoryTest> memoryTests;
    
    // Test with increasing mesh counts
    std::vector<int> meshCounts = {100, 500, 1000, 2000};
    
    for (int count : meshCounts) {
        auto meshes = createVoxelGrid(count);
        
        // Estimate memory usage (rough calculation)
        int verticesPerMesh = 24; // 6 faces * 4 vertices
        int floatsPerVertex = 9; // position(3) + normal(3) + color(3)
        int indicesPerMesh = 36; // 6 faces * 2 triangles * 3 indices
        
        int meshMemory = (verticesPerMesh * floatsPerVertex * sizeof(float) + 
                         indicesPerMesh * sizeof(unsigned int)) / 1024; // KB
        int totalMemory = meshMemory * count;
        
        // Run performance test
        auto result = runBenchmark("basic_voxel", meshes, 200);
        
        memoryTests.push_back({count, result.averageFPS, totalMemory});
        
        std::cout << "\nMemory test with " << count << " meshes:" << std::endl;
        std::cout << "Estimated VRAM usage: " << totalMemory << " KB" << std::endl;
        std::cout << "Performance: " << result.averageFPS << " FPS" << std::endl;
        
        // Clean up meshes before next iteration
        meshes.clear();
    }
    
    // Verify memory scaling is reasonable
    for (const auto& test : memoryTests) {
        // Memory usage should be proportional to mesh count
        double memoryPerMesh = (double)test.memoryUsed / test.meshCount;
        EXPECT_LT(memoryPerMesh, 10.0) << "Memory per mesh should be reasonable (<10KB)";
        
        // Performance should degrade gracefully with memory usage
        if (test.memoryUsed < 100000) { // Less than 100MB
            EXPECT_GT(test.fps, 30.0) << "Should maintain 30+ FPS with reasonable memory usage";
        }
    }
}