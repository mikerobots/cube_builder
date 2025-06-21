# OpenGL Integration Testing Guide

This guide explains the options and approaches for creating comprehensive OpenGL integration tests in the VoxelEditor project, including shader validation, rendering tests, visual feedback, and any component that requires OpenGL context.

## Overview

OpenGL integration testing validates the entire graphics pipeline including shaders, rendering systems, visual feedback components, and framebuffer operations. Unlike unit tests that test individual components, integration tests verify the complete rendering pipeline from input data to visual output.

## Test Classification and Skipping Policy

**Integration Tests**: MUST NOT skip based on environment. These tests validate critical functionality and must pass in all environments including CI.

**Unit Tests**: MAY skip in environments without OpenGL if they test OpenGL-specific functionality in isolation.

**Placeholder Tests**: MAY skip with clear message indicating they are placeholders for future functionality.

**IMPORTANT**: If a test requires OpenGL functionality (rendering, mouse interaction, visual feedback, etc.), it MUST NOT be run in headless mode. The application's headless mode skips OpenGL initialization, so any test using OpenGL features must initialize with a proper OpenGL context.

```cpp
// CORRECT: Placeholder test
TEST_F(FutureFeatureTest, NotYetImplemented) {
    GTEST_SKIP() << "Placeholder for future VR rendering support";
}

// INCORRECT: Integration test skipping
TEST_F(RenderingIntegrationTest, BasicRendering) {
    if (std::getenv("CI") != nullptr) {
        GTEST_SKIP() << "Skipping in CI"; // WRONG! Integration tests must work everywhere
    }
}

// INCORRECT: Using headless mode for OpenGL test
TEST_F(MouseInteractionTest, ClickHandling) {
    app->initialize(argc, {"--headless"}); // WRONG! Mouse interaction needs OpenGL
}

// CORRECT: OpenGL test without headless mode
TEST_F(MouseInteractionTest, ClickHandling) {
    app->initialize(argc, argv); // Initialize with OpenGL context
}
```

## Key Components

### 1. OpenGL Context Setup

Every OpenGL integration test requires a proper OpenGL context with platform-specific handling:

**IMPORTANT**: Integration tests should NEVER skip, even in CI environments. If OpenGL is required for an integration test, the test environment must provide it. Only skip tests if they are placeholders for future functionality.

```cpp
class MyOpenGLTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Rendering::OpenGLRenderer> renderer;
    
    void SetUp() override {
        // IMPORTANT: Do NOT skip integration tests in CI
        // Integration tests must work in all environments
        // Only skip if the test is a placeholder for future functionality
        
        // Initialize GLFW
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        
        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden for automated testing
        
        #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
        
        // Create window
        window = glfwCreateWindow(800, 600, "Test Window", nullptr, nullptr);
        ASSERT_NE(window, nullptr) << "Failed to create GLFW window";
        
        glfwMakeContextCurrent(window);
        
        #ifndef __APPLE__
        // Initialize GLAD (not needed on macOS)
        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) << "Failed to initialize GLAD";
        #endif
        
        // Clear any GL errors from initialization
        while (glGetError() != GL_NO_ERROR) {}
        
        // Initialize OpenGLRenderer
        renderer = std::make_unique<Rendering::OpenGLRenderer>();
        Rendering::RenderConfig config;
        config.windowWidth = 800;
        config.windowHeight = 600;
        ASSERT_TRUE(renderer->initializeContext(config)) << "Failed to initialize renderer";
    }
    
    void TearDown() override {
        renderer.reset();
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
};
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

## Implementation Approaches

When implementing OpenGL integration tests, always prefer using the OpenGLRenderer class which provides a stable, working API:

### Recommended: OpenGLRenderer API (Preferred approach)

**When to use**: Always - this is the stable, tested API for OpenGL operations
**Pros**: Type-safe, consistent API, proper resource management, works across platforms
**Usage**: All rendering operations should go through OpenGLRenderer when possible

```cpp
// Shader management
ASSERT_TRUE(shaderManager->loadShader("basic_voxel", "basic_voxel_gl33.vert", "basic_voxel_gl33.frag"));
auto shaderId = shaderManager->getShader("basic_voxel");

// Use OpenGLRenderer for all shader operations
renderer->useProgram(shaderId);
renderer->setUniform(shaderId, "model", Rendering::UniformValue(glmToMathMatrix(model)));
renderer->setUniform(shaderId, "viewPos", Rendering::UniformValue(Math::Vector3f(3.0f, 3.0f, 3.0f)));
renderer->setUniform(shaderId, "lightDir", Rendering::UniformValue(Math::Vector3f(1.0f, 1.0f, 1.0f)));

// Vertex Array management
auto vao = renderer->createVertexArray();
renderer->bindVertexArray(vao);

// Buffer management
auto vbo = renderer->createBuffer();
renderer->bindBuffer(GL_ARRAY_BUFFER, vbo);
renderer->bufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

// Drawing
renderer->drawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
```

### Alternative: Raw OpenGL (Only when necessary)

**When to use**: Only for operations not yet wrapped by OpenGLRenderer
**Examples**: Specific GL state queries, specialized extensions, debugging

```cpp
// Use raw OpenGL only for operations not in OpenGLRenderer API
glEnable(GL_DEPTH_TEST);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glViewport(0, 0, width, height);

// Always prefer renderer->drawElements() over glDrawElements()
```

### Complete Example Using OpenGLRenderer

```cpp
class VisualFeedbackTest : public ::testing::Test {
protected:
    void renderTestScene() {
        // Clear using raw GL (if not in renderer API)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // All shader operations through renderer
        auto shaderId = shaderManager->getShader("feedback_shader");
        renderer->useProgram(shaderId);
        
        // Set uniforms using renderer
        renderer->setUniform(shaderId, "projection", Rendering::UniformValue(projMatrix));
        renderer->setUniform(shaderId, "view", Rendering::UniformValue(viewMatrix));
        renderer->setUniform(shaderId, "model", Rendering::UniformValue(modelMatrix));
        
        // Bind and draw using renderer
        renderer->bindVertexArray(meshVAO);
        renderer->drawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
        
        // Swap buffers through GLFW (window management)
        glfwSwapBuffers(window);
    }
};
```

### Type Compatibility Notes

Some components expect `RenderEngine*` interface. For integration testing, many components can be tested with nullptr if they don't actively use the render engine:

```cpp
// For components that don't use RenderEngine in the test scenarios
auto feedbackRenderer = std::make_unique<FeedbackRenderer>(nullptr);

// If actual rendering is needed, ensure proper inheritance or adapters are in place
// Note: OpenGLRenderer may not directly inherit from RenderEngine in all cases
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

### 1. Shader Tests
- Shader compilation and linking
- Uniform setting and retrieval
- Vertex attribute processing
- Fragment shader output validation
- Multi-shader switching

### 2. Rendering Tests
- Basic primitive rendering
- Mesh rendering with indices
- Multiple draw calls
- Framebuffer operations
- Depth and stencil testing

### 3. Visual Feedback Tests
- Overlay rendering
- Text rendering
- UI element rendering
- Grid and guide rendering
- Selection highlighting

### 4. Camera and Transformation Tests
- View matrix calculations
- Projection matrix handling
- Model transformations
- Viewport management
- Coordinate system conversions

### 5. Performance Tests
- FPS benchmarks
- Draw call optimization
- State change minimization
- Memory usage profiling
- Large scene handling

### 6. Integration Tests
- Complete render pipeline validation
- Multi-component interaction
- Event-driven rendering updates
- Resource management
- Error handling and recovery

## CMake Integration

```cmake
# Add OpenGL integration test executable
add_executable(test_integration_opengl_component
    test_integration_opengl_component.cpp
)

# IMPORTANT: Must link against glfw, glad, and OpenGL::GL for any test using OpenGL
target_link_libraries(test_integration_opengl_component PRIVATE
    VoxelEditor_Rendering      # For OpenGLRenderer
    VoxelEditor_VisualFeedback # If testing visual feedback
    VoxelEditor_Camera         # If testing camera integration
    VoxelEditor_Math
    VoxelEditor_Logging
    glfw                       # REQUIRED for GLFW window management
    glad                       # REQUIRED for OpenGL function loading (except macOS)
    OpenGL::GL                 # REQUIRED for OpenGL
    glm::glm                   # For math operations
    GTest::gtest_main          # Google Test framework
    ${CMAKE_DL_LIBS}          # Required for macOS dlopen
)

target_compile_features(test_integration_opengl_component PRIVATE cxx_std_20)

# Add compile definitions if needed
target_compile_definitions(test_integration_opengl_component PRIVATE
    $<$<PLATFORM_ID:Darwin>:GL_SILENCE_DEPRECATION>
)

gtest_discover_tests(test_integration_opengl_component)

# Note: Order matters! glfw should come before glad in the link libraries
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

**CRITICAL**: Integration tests MUST NOT skip in CI environments. The CI environment must be configured to support OpenGL testing.

For automated testing environments:
- Use headless rendering (GLFW_VISIBLE = FALSE)
- Set appropriate timeouts for GPU operations
- Consider using software rendering for consistent results (e.g., Mesa3D software renderer, Xvfb)
- Save debug images for failed test analysis
- Configure CI to provide OpenGL context (e.g., using xvfb-run on Linux)

Example CI configuration for OpenGL support:
```bash
# Linux CI setup
xvfb-run -a ./run_integration_tests.sh

# Or with specific display settings
export DISPLAY=:99.0
Xvfb :99 -screen 0 1024x768x24 > /dev/null 2>&1 &
./run_integration_tests.sh
```

This guide provides a comprehensive foundation for creating robust shader integration tests that validate both functional correctness and performance characteristics of the VoxelEditor's graphics pipeline.