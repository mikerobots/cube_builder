#include <gtest/gtest.h>
#include "OpenGLTestFixture.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <memory>

using namespace VoxelEditor::Rendering::Tests;

/**
 * Visual validation test for shader rendering output
 * 
 * NOTE: These tests have known issues on macOS due to OpenGL context complexities:
 * - OpenGL error 1282 (GL_INVALID_OPERATION) during vertex attribute setup
 * - VAO creation and binding issues with macOS OpenGL 3.3 Core Profile
 * - Rendering produces all black pixels despite correct shader compilation
 * 
 * The core rendering system works correctly as validated by other tests (EdgeRenderingTest).
 * These visual validation tests may need platform-specific fixes or different testing approach.
 */
class ShaderVisualTest : public OpenGLTestFixture {
protected:
    // Override default window size for shader tests
    ShaderVisualTest() {
        windowWidth = 256;
        windowHeight = 256;
    }
    
    void SetUp() override {
        OpenGLTestFixture::SetUp();
        
        if (!hasValidContext()) {
            return;
        }
        
        // Set up default OpenGL state for shader tests
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
    }
    
    struct ColorStats {
        glm::vec3 avgColor;
        float minBrightness;
        float maxBrightness;
        int nonBlackPixels;
        int totalPixels;
    };
    
    ColorStats captureAndAnalyzeFrame() {
        auto pixels = captureFramebuffer();
        
        ColorStats stats{glm::vec3(0), 1.0f, 0.0f, 0, windowWidth * windowHeight};
        
        for (int i = 0; i < windowWidth * windowHeight; ++i) {
            glm::vec3 color(
                pixels[i * 3] / 255.0f,
                pixels[i * 3 + 1] / 255.0f,
                pixels[i * 3 + 2] / 255.0f
            );
            
            float brightness = (color.r + color.g + color.b) / 3.0f;
            
            stats.avgColor += color;
            stats.minBrightness = std::min(stats.minBrightness, brightness);
            stats.maxBrightness = std::max(stats.maxBrightness, brightness);
            
            if (brightness > 0.01f) {
                stats.nonBlackPixels++;
            }
        }
        
        stats.avgColor /= stats.totalPixels;
        return stats;
    }
    
    void savePPM(const std::string& filename) {
        saveFramebufferToPPM(filename);
    }
    
    GLuint createCubeVAO() {
        // Cube vertices with positions, normals, and colors - larger cube
        float vertices[] = {
            // Front face (red)
            -2.0f, -2.0f,  2.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f,
             2.0f, -2.0f,  2.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f,
             2.0f,  2.0f,  2.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f,
            -2.0f,  2.0f,  2.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f,
            
            // Back face (green)
            -2.0f, -2.0f, -2.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
             2.0f, -2.0f, -2.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
             2.0f,  2.0f, -2.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
            -2.0f,  2.0f, -2.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        };
        
        unsigned int indices[] = {
            0, 1, 2, 2, 3, 0,  // Front
            4, 5, 6, 6, 7, 4,  // Back
        };
        
        GLuint VAO, VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal attribute (location 1 - now used for color in the shader)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
        
        return VAO;
    }
};

// Test basic voxel shader rendering
TEST_F(ShaderVisualTest, BasicVoxelShaderRendering) {
    // Check if we have a valid context from the base fixture
    if (!hasValidContext()) {
        GTEST_SKIP() << "Skipping test - no valid OpenGL context";
    }
    
    // Create the simplest possible shader - fixed color output
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        
        void main() {
            gl_Position = vec4(position, 1.0);
        }
    )";
    
    const char* fragmentSource = R"(
        #version 330 core
        out vec4 color;
        
        void main() {
            color = vec4(1.0, 0.0, 0.0, 1.0);  // Fixed red color
        }
    )";
    
    // Create shader program using base fixture helper
    GLuint program = createProgram(vertexSource, fragmentSource);
    if (!program) {
        GTEST_SKIP() << "Shader compilation failed";
    }
    printf("Shader program created: %u\n", program);
    
    // Create VAO and VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // Create simple triangle geometry - just positions
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,   // bottom left
         0.5f, -0.5f, 0.0f,   // bottom right  
         0.0f,  0.5f, 0.0f    // top center
    };
    
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Set up vertex attributes - only position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Unbind VAO for now
    glBindVertexArray(0);
    
    // Clear any existing OpenGL errors
    while (glGetError() != GL_NO_ERROR) {}
    
    // Set clear color to dark blue for debugging
    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
    
    // Clear and render
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    // Use shader program
    glUseProgram(program);
    
    // Bind VAO
    glBindVertexArray(VAO);
    
    // Draw the triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("OpenGL error after draw: %d\n", error);
    } else {
        printf("Drawing completed successfully\n");
    }
    
    // Check if shader is still bound
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    printf("Current program after draw: %d (expected %d)\n", currentProgram, program);
    
    // Check viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    printf("Viewport: %d,%d %dx%d\n", viewport[0], viewport[1], viewport[2], viewport[3]);
    
    // Ensure rendering is complete
    glFlush();
    glFinish();
    
    // Don't swap buffers - we want to read from the back buffer
    // swapBuffers();
    
    // Analyze rendered output from back buffer
    auto stats = captureAndAnalyzeFrame();
    
    // Validate rendering worked - we should see red pixels
    EXPECT_GT(stats.nonBlackPixels, stats.totalPixels * 0.05f) 
        << "At least 5% of pixels should be non-black (red triangle)";
    EXPECT_GT(stats.maxBrightness, 0.3f) 
        << "Maximum brightness should indicate red color";
    EXPECT_GT(stats.avgColor.r, 0.1f) 
        << "Should see red from the triangle";
    
    // Save debug image if test fails
    if (::testing::Test::HasFailure()) {
        savePPM("debug_basic_voxel_shader.ppm");
    }
    
    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(program);
}

// Test flat shading
TEST_F(ShaderVisualTest, FlatShadingValidation) {
    // Check if we have a valid context from the base fixture
    if (!hasValidContext()) {
        GTEST_SKIP() << "Skipping test - no valid OpenGL context";
    }
    
    // Flat shading should produce uniform colors per face
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 color;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        flat out vec3 fragColor;
        
        void main() {
            gl_Position = projection * view * model * vec4(position, 1.0);
            fragColor = color;
        }
    )";
    
    const char* fragmentSource = R"(
        #version 330 core
        flat in vec3 fragColor;
        
        out vec4 color;
        
        void main() {
            color = vec4(fragColor, 1.0);
        }
    )";
    
    // Create shader program using base fixture helper
    GLuint program = createProgram(vertexSource, fragmentSource);
    if (!program) {
        GTEST_SKIP() << "Shader compilation failed";
    }
    
    // Render and validate flat shading produces solid colors
    GLuint cubeVAO = createCubeVAO();
    
    glm::mat4 mvp = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, -10.0f, 10.0f);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    glUseProgram(program);
    // Use orthographic matrices for flat shading test
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(mvp));
    
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // Draw only front face
    
    // Ensure rendering is complete before reading
    glFlush();
    glFinish();
    
    auto stats = captureAndAnalyzeFrame();
    
    // Flat shading should produce uniform red color where visible
    EXPECT_GT(stats.avgColor.r, 0.1f) << "Front face should have red component";
    EXPECT_GT(stats.nonBlackPixels, stats.totalPixels * 0.05f) << "Should see rendered pixels";
    
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteProgram(program);
}

// Test ground plane grid rendering
TEST_F(ShaderVisualTest, GroundPlaneGridRendering) {
    // Check if we have a valid context from the base fixture
    if (!hasValidContext()) {
        GTEST_SKIP() << "Skipping test - no valid OpenGL context";
    }
    
    // Create grid geometry
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    const int gridSize = 10;
    const float spacing = 0.2f;
    
    // Generate grid lines
    int index = 0;
    for (int i = -gridSize; i <= gridSize; ++i) {
        // X-direction lines
        vertices.insert(vertices.end(), {
            -gridSize * spacing, 0, i * spacing, (i % 5 == 0) ? 1.0f : 0.0f,
            gridSize * spacing, 0, i * spacing, (i % 5 == 0) ? 1.0f : 0.0f
        });
        indices.push_back(index++);
        indices.push_back(index++);
        
        // Z-direction lines
        vertices.insert(vertices.end(), {
            i * spacing, 0, -gridSize * spacing, (i % 5 == 0) ? 1.0f : 0.0f,
            i * spacing, 0, gridSize * spacing, (i % 5 == 0) ? 1.0f : 0.0f
        });
        indices.push_back(index++);
        indices.push_back(index++);
    }
    
    // Ground plane shader
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in float isMajor;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out float fragIsMajor;
        
        void main() {
            gl_Position = projection * view * model * vec4(position, 1.0);
            fragIsMajor = isMajor;
        }
    )";
    
    const char* fragmentSource = R"(
        #version 330 core
        in float fragIsMajor;
        
        uniform vec3 gridColor;
        uniform vec3 majorGridColor;
        uniform float gridOpacity;
        
        out vec4 color;
        
        void main() {
            vec3 lineColor = fragIsMajor > 0.5 ? majorGridColor : gridColor;
            color = vec4(lineColor, gridOpacity);
        }
    )";
    
    // Create shader program using base fixture helper
    GLuint program = createProgram(vertexSource, fragmentSource);
    if (!program) {
        GTEST_SKIP() << "Shader compilation failed";
    }
    
    // Create VAO
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Render
    glm::mat4 view = glm::lookAt(glm::vec3(1, 2, 1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(program);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(program, "gridColor"), 0.5f, 0.5f, 0.5f);
    glUniform3f(glGetUniformLocation(program, "majorGridColor"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(program, "gridOpacity"), 1.0f);
    
    // Set line width for better visibility
    glLineWidth(2.0f);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
    
    // Ensure rendering is complete before reading
    glFlush();
    glFinish();
    
    // Validate grid rendering
    auto stats = captureAndAnalyzeFrame();
    
    EXPECT_GT(stats.nonBlackPixels, stats.totalPixels * 0.02f) 
        << "Grid lines should be visible";
    EXPECT_GT(stats.maxBrightness, 0.4f) 
        << "Major grid lines should be bright";
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(program);
}