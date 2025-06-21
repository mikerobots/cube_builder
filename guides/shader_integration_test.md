# Shader Integration Testing Guide

This guide explains the options and approaches for creating comprehensive shader integration tests using C++ and OpenGL in the VoxelEditor project.

## Overview

Shader integration testing validates that shaders correctly process vertex data, apply transformations, handle lighting, and produce expected visual output. Unlike unit tests that test individual components, integration tests verify the entire graphics pipeline from mesh data to framebuffer output.

## Key Components

### 1. OpenGL Context Setup

Every shader integration test requires a proper OpenGL context:

```cpp
// GLFW window setup for headless testing
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden for automated testing

// Initialize GLAD for OpenGL function loading
ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));
```

### 2. Platform-Specific Compatibility

#### macOS VAO Function Loading

macOS requires special handling for Vertex Array Object (VAO) functions:

```cpp
#ifdef __APPLE__
#include "rendering/MacOSGLLoader.h"

// In test setup:
ASSERT_TRUE(Rendering::LoadOpenGLExtensions()) << "Failed to load OpenGL extensions";

// Use compatibility macros:
#define VAO_GEN(n, arrays) VoxelEditor::Rendering::glGenVertexArrays(n, arrays)
#define VAO_BIND(array) VoxelEditor::Rendering::glBindVertexArray(array)
#define VAO_DELETE(n, arrays) VoxelEditor::Rendering::glDeleteVertexArrays(n, arrays)
#else
#define VAO_GEN(n, arrays) glGenVertexArrays(n, arrays)
#define VAO_BIND(array) glBindVertexArray(array)
#define VAO_DELETE(n, arrays) glDeleteVertexArrays(n, arrays)
#endif
```

### 3. Shader API Compatibility

#### Type System Conversion

The VoxelEditor uses a custom Math type system instead of GLM types:

```cpp
// Helper function for matrix conversion
Math::Matrix4f glmToMathMatrix(const glm::mat4& mat) {
    Math::Matrix4f result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i * 4 + j] = mat[j][i]; // glm is column-major
        }
    }
    return result;
}

// Usage in tests:
glm::mat4 model = glm::mat4(1.0f);
renderer->setUniform(shaderId, "model", Rendering::UniformValue(glmToMathMatrix(model)));
renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(3.0f, 3.0f, 3.0f)));
```

#### OpenGLRenderer Direct Usage

When ShaderProgram wrapper is incomplete, use OpenGLRenderer directly:

```cpp
// Instead of: shader->setMat4("model", model);
// Use:
renderer->useProgram(shaderId);
renderer->setUniform(shaderId, "model", Rendering::UniformValue(glmToMathMatrix(model)));
```

## Alternative Implementation Options

When implementing shader integration tests, you have several architectural approaches depending on the maturity of your rendering system:

### Option 1: ShaderProgram Wrapper (Ideal but may be incomplete)

**When to use**: When ShaderProgram class has complete uniform API implementation
**Pros**: Clean, type-safe API with better abstraction
**Cons**: May not be implemented yet or have missing methods

```cpp
// Ideal approach with complete ShaderProgram wrapper
auto* shader = shaderManager->getShaderProgram(shaderId);
shader->use();
shader->setUniform("model", glmToMathMatrix(model));
shader->setUniform("viewPos", Math::Vector3f(3.0f, 3.0f, 3.0f));
```

**Limitations discovered in VoxelEditor**:
- `ShaderProgram::setUniform(string, Matrix4f)` method not implemented
- `ShaderProgram::setUniform(string, Vector3f)` method not implemented  
- `ShaderProgram::use()` method not implemented
- `ShaderManager::getShaderProgram(id)` method not implemented

### Option 2: OpenGLRenderer Direct Usage (Current working solution)

**When to use**: When ShaderProgram wrapper is incomplete or missing methods
**Pros**: Bypasses incomplete abstractions, uses working low-level API
**Cons**: More verbose, requires understanding of UniformValue wrapper

```cpp
// Working approach that bypasses incomplete ShaderProgram wrapper
renderer->useProgram(shaderId);
renderer->setUniform(shaderId, "model", Rendering::UniformValue(glmToMathMatrix(model)));
renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(3.0f, 3.0f, 3.0f)));
```

### Option 3: Raw OpenGL Calls (Fallback option)

**When to use**: When both wrapper systems are incomplete
**Pros**: Always available, direct control
**Cons**: No type safety, requires manual uniform location management, error-prone

```cpp
// Raw OpenGL approach (use only as last resort)
glUseProgram(shaderId);
GLint modelLoc = glGetUniformLocation(shaderId, "model");
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
```

### Option 4: Hybrid Approach (Recommended for development)

**When to use**: During active development when some systems work and others don't
**Strategy**: Use the highest-level working API for each operation

```cpp
// Use ShaderManager for loading (if working)
ASSERT_TRUE(shaderManager->loadShader("basic_voxel", "basic_voxel_gl33.vert", "basic_voxel_gl33.frag"));
auto shaderId = shaderManager->getShader("basic_voxel");

// Use OpenGLRenderer for uniforms (if ShaderProgram incomplete)
renderer->useProgram(shaderId);
renderer->setUniform(shaderId, "model", Rendering::UniformValue(glmToMathMatrix(model)));

// Use raw OpenGL for operations not wrapped yet (if needed)
glEnable(GL_DEPTH_TEST);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

## Testing Approaches

### 1. Visual Validation Testing

Captures framebuffer output and analyzes pixel data:

```cpp
// Capture framebuffer
std::vector<unsigned char> captureFramebuffer() {
    std::vector<unsigned char> pixels(WINDOW_WIDTH * WINDOW_HEIGHT * 4); // RGBA
    glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return pixels;
}

// Validate rendering occurred
int countRenderedPixels(const std::vector<unsigned char>& pixels) {
    int count = 0;
    for (size_t i = 0; i < pixels.size(); i += 4) {
        if (pixels[i] != 0 || pixels[i+1] != 0 || pixels[i+2] != 0) {
            count++;
        }
    }
    return count;
}
```

### 2. Multi-Resolution Testing

Test all voxel resolutions (1cm to 512cm) with camera distance adjustment:

```cpp
std::vector<float> resolutions = {0.01f, 0.02f, 0.04f, 0.08f, 0.16f, 0.32f, 0.64f, 1.28f, 2.56f, 5.12f};

for (size_t i = 0; i < resolutions.size(); ++i) {
    float size = resolutions[i];
    auto cube = createVoxelCube(size, glm::vec3(0.0f, 0.5f, 1.0f));
    
    // Adjust camera distance for larger voxels
    float cameraDistance = 3.0f + size * 2.0f;
    glm::mat4 view = glm::lookAt(
        glm::vec3(cameraDistance, cameraDistance, cameraDistance),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}
```

### 3. Performance Benchmarking

Test shader performance and uniform update speeds:

```cpp
// FPS testing
auto start = std::chrono::high_resolution_clock::now();
for (int frame = 0; frame < NUM_FRAMES; ++frame) {
    // Render frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glfwSwapBuffers(window);
}
auto end = std::chrono::high_resolution_clock::now();

float fps = NUM_FRAMES / std::chrono::duration<float>(end - start).count();
EXPECT_GT(fps, 60.0f) << "Failed to achieve target 60 FPS";
```

### 4. Reference Image Comparison

Generate and compare against reference images:

```cpp
// Generate reference images
void generateReferenceImage(const std::string& testName) {
    auto pixels = captureFramebuffer();
    std::string filename = "reference_images/" + testName + ".ppm";
    savePPM(filename, pixels, WINDOW_WIDTH, WINDOW_HEIGHT);
}

// Compare against reference
bool compareToReference(const std::string& testName, float tolerance = 5.0f) {
    auto currentPixels = captureFramebuffer();
    auto referencePixels = loadPPM("reference_images/" + testName + ".ppm");
    return calculatePixelDifference(currentPixels, referencePixels) < tolerance;
}
```

## Test Categories

### 1. Basic Shader Functionality
- Single voxel rendering
- Color validation
- Transformation matrices
- Basic lighting

### 2. Multi-Shader Validation
- Shader switching performance
- Different shader outputs (basic, enhanced, flat)
- Uniform compatibility across shaders

### 3. Lighting and Materials
- Phong lighting validation
- Light direction effects
- Material property testing
- Face orientation brightness

### 4. Performance Testing
- FPS benchmarks
- Uniform update performance
- Memory usage scaling
- Large scene rendering

### 5. Edge Cases
- Extreme transformations
- Multiple meshes
- Resolution scaling
- Camera angle variations

## CMake Integration

```cmake
# Add shader test executable
add_executable(test_shader_integration
    test_shader_integration.cpp
)

target_link_libraries(test_shader_integration PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    OpenGL::GL
    glfw
    glad
    glm::glm
    GTest::gtest_main
    ${CMAKE_DL_LIBS}  # Required for macOS dlopen
)

target_compile_features(test_shader_integration PRIVATE cxx_std_20)
gtest_discover_tests(test_shader_integration)
```

## Best Practices

### 1. Error Checking
Always clear and check for OpenGL errors:

```cpp
// Clear existing errors
while (glGetError() != GL_NO_ERROR) {}

// Perform operations
glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

// Check for errors
GLenum error = glGetError();
EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error: " << error;
```

### 2. Resource Management
Use RAII for OpenGL resources:

```cpp
struct VoxelMesh {
    GLuint vao, vbo, ebo;
    int indexCount;
    
    ~VoxelMesh() {
        if (vao) VAO_DELETE(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (ebo) glDeleteBuffers(1, &ebo);
    }
};
```

### 3. Validation Thresholds
Use appropriate tolerances for pixel comparisons:

```cpp
// Color presence check with tolerance
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
```

### 4. Debugging Support
Save framebuffer contents for visual debugging:

```cpp
void savePPM(const std::string& filename, const std::vector<unsigned char>& pixels, 
             int width, int height) {
    std::ofstream file(filename);
    file << "P3\n" << width << " " << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 4;
            file << (int)pixels[idx] << " " << (int)pixels[idx+1] << " " << (int)pixels[idx+2] << "\n";
        }
    }
}
```

## Common Issues and Solutions

### 1. VAO Functions Not Found (macOS)
**Problem**: `glGenVertexArrays` not available through GLAD on macOS
**Solution**: Use MacOSGLLoader.h and VAO compatibility macros

### 2. Shader Uniform Type Mismatch
**Problem**: Shader expects Math types but test uses GLM types
**Solution**: Use conversion helpers like `glmToMathMatrix()`

### 3. Black Screen Output
**Problem**: Nothing renders despite no GL errors
**Solution**: Check depth testing, viewport, matrix calculations, and shader uniform values

### 4. Inconsistent Test Results
**Problem**: Tests pass/fail intermittently
**Solution**: Use appropriate tolerances, clear state between tests, check timing-dependent operations

### 5. Memory Leaks in Tests
**Problem**: OpenGL resources not properly cleaned up
**Solution**: Use RAII resource management, proper test teardown

## Integration with CI/CD

For automated testing environments:
- Use headless rendering (GLFW_VISIBLE = FALSE)
- Set appropriate timeouts for GPU operations
- Consider using software rendering for consistent results
- Save debug images for failed test analysis

This guide provides a comprehensive foundation for creating robust shader integration tests that validate both functional correctness and performance characteristics of the VoxelEditor's graphics pipeline.