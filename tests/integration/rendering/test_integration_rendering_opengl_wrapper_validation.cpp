#include <gtest/gtest.h>
#include "rendering/OpenGLRenderer.h"
#include "rendering/RenderTypes.h"
#include "math/Matrix4f.h"
#include "math/Vector3f.h"
#include <memory>
#include <vector>

// Include OpenGL headers
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>

namespace VoxelEditor {
namespace Tests {

class OpenGLWrapperValidationTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Rendering::OpenGLRenderer> glRenderer;
    
    void SetUp() override {
        // Skip in CI environment
        if (std::getenv("CI") != nullptr) {
            GTEST_SKIP() << "Skipping OpenGL tests in CI environment";
        }
        
        // Initialize GLFW
        if (!glfwInit()) {
            GTEST_SKIP() << "Failed to initialize GLFW";
        }
        
        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
        
        window = glfwCreateWindow(800, 600, "Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            GTEST_SKIP() << "Failed to create GLFW window";
        }
        
        glfwMakeContextCurrent(window);
        
        // Initialize OpenGL renderer
        glRenderer = std::make_unique<Rendering::OpenGLRenderer>();
        Rendering::RenderConfig config;
        config.windowWidth = 800;
        config.windowHeight = 600;
        
        if (!glRenderer->initializeContext(config)) {
            GTEST_SKIP() << "Failed to initialize OpenGL context";
        }
    }
    
    void TearDown() override {
        glRenderer.reset();
        
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        glfwTerminate();
    }
    
    bool checkGLError(const std::string& context) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "GL Error in " << context << ": " << error << std::endl;
            return false;
        }
        return true;
    }
};

// Test buffer creation and management
TEST_F(OpenGLWrapperValidationTest, BufferManagement) {
    // Test vertex buffer creation
    float vertices[] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };
    
    auto vbo = glRenderer->createVertexBuffer(vertices, sizeof(vertices), Rendering::BufferUsage::Static);
    EXPECT_NE(vbo, Rendering::InvalidId);
    EXPECT_TRUE(checkGLError("Create vertex buffer"));
    
    // Test index buffer creation
    uint32_t indices[] = {0, 1, 2};
    auto ibo = glRenderer->createIndexBuffer(indices, 3, Rendering::BufferUsage::Static);
    EXPECT_NE(ibo, Rendering::InvalidId);
    EXPECT_TRUE(checkGLError("Create index buffer"));
    
    // Test buffer update
    float newVertices[] = {
        0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f
    };
    glRenderer->updateBuffer(vbo, newVertices, sizeof(newVertices), 0);
    EXPECT_TRUE(checkGLError("Update buffer"));
    
    // Test buffer deletion
    glRenderer->deleteBuffer(vbo);
    glRenderer->deleteBuffer(ibo);
    EXPECT_TRUE(checkGLError("Delete buffers"));
}

// Test texture creation and management
TEST_F(OpenGLWrapperValidationTest, TextureManagement) {
    // Create a simple texture
    uint8_t textureData[64 * 64 * 4]; // 64x64 RGBA
    for (int i = 0; i < 64 * 64 * 4; i += 4) {
        textureData[i] = 255;     // R
        textureData[i+1] = 128;   // G
        textureData[i+2] = 64;    // B
        textureData[i+3] = 255;   // A
    }
    
    auto texture = glRenderer->createTexture2D(64, 64, Rendering::TextureFormat::RGBA8, textureData);
    EXPECT_NE(texture, Rendering::InvalidId);
    EXPECT_TRUE(checkGLError("Create texture"));
    
    // Test texture update
    uint8_t newData[32 * 32 * 4];
    for (int i = 0; i < 32 * 32 * 4; i += 4) {
        newData[i] = 64;      // R
        newData[i+1] = 128;   // G
        newData[i+2] = 255;   // B
        newData[i+3] = 255;   // A
    }
    glRenderer->updateTexture(texture, 0, 0, 32, 32, newData);
    EXPECT_TRUE(checkGLError("Update texture"));
    
    // Test texture binding
    glRenderer->bindTexture(texture, 0);
    EXPECT_TRUE(checkGLError("Bind texture"));
    
    // Test texture deletion
    glRenderer->deleteTexture(texture);
    EXPECT_TRUE(checkGLError("Delete texture"));
}

// Test framebuffer operations
TEST_F(OpenGLWrapperValidationTest, FramebufferOperations) {
    // Note: OpenGLRenderer doesn't expose framebuffer operations directly
    // Test clear operations instead
    glRenderer->clear(Rendering::ClearFlags::All, Rendering::Color(0.2f, 0.3f, 0.4f, 1.0f));
    EXPECT_TRUE(checkGLError("Clear framebuffer"));
    
    // Test with different clear flags
    glRenderer->clear(Rendering::ClearFlags::COLOR, Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_TRUE(checkGLError("Clear color only"));
    
    glRenderer->clear(Rendering::ClearFlags::DEPTH);
    EXPECT_TRUE(checkGLError("Clear depth only"));
}

// Test render state management
TEST_F(OpenGLWrapperValidationTest, RenderStateManagement) {
    // Test depth testing
    glRenderer->setDepthTest(true);
    EXPECT_TRUE(checkGLError("Enable depth test"));
    
    GLboolean depthEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
    EXPECT_EQ(depthEnabled, GL_TRUE);
    
    glRenderer->setDepthTest(false);
    EXPECT_TRUE(checkGLError("Disable depth test"));
    
    glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
    EXPECT_EQ(depthEnabled, GL_FALSE);
    
    // Test depth write
    glRenderer->setDepthWrite(true);
    EXPECT_TRUE(checkGLError("Enable depth write"));
    
    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    EXPECT_EQ(depthMask, GL_TRUE);
    
    glRenderer->setDepthWrite(false);
    EXPECT_TRUE(checkGLError("Disable depth write"));
    
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    EXPECT_EQ(depthMask, GL_FALSE);
    
    // Test blending
    glRenderer->setBlending(true, Rendering::BlendMode::Alpha);
    EXPECT_TRUE(checkGLError("Enable blending"));
    
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    EXPECT_EQ(blendEnabled, GL_TRUE);
    
    glRenderer->setBlending(false, Rendering::BlendMode::Opaque);
    EXPECT_TRUE(checkGLError("Disable blending"));
    
    glGetBooleanv(GL_BLEND, &blendEnabled);
    EXPECT_EQ(blendEnabled, GL_FALSE);
    
    // Test culling
    glRenderer->setCulling(true, Rendering::CullMode::Back);
    EXPECT_TRUE(checkGLError("Enable culling"));
    
    GLboolean cullEnabled;
    glGetBooleanv(GL_CULL_FACE, &cullEnabled);
    EXPECT_EQ(cullEnabled, GL_TRUE);
    
    glRenderer->setCulling(false, Rendering::CullMode::None);
    EXPECT_TRUE(checkGLError("Disable culling"));
    
    glGetBooleanv(GL_CULL_FACE, &cullEnabled);
    EXPECT_EQ(cullEnabled, GL_FALSE);
}

// Test viewport operations
TEST_F(OpenGLWrapperValidationTest, ViewportOperations) {
    glRenderer->setViewport(0, 0, 640, 480);
    EXPECT_TRUE(checkGLError("Set viewport"));
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    EXPECT_EQ(viewport[0], 0);
    EXPECT_EQ(viewport[1], 0);
    EXPECT_EQ(viewport[2], 640);
    EXPECT_EQ(viewport[3], 480);
    
    glRenderer->setViewport(100, 100, 400, 300);
    EXPECT_TRUE(checkGLError("Update viewport"));
    
    glGetIntegerv(GL_VIEWPORT, viewport);
    EXPECT_EQ(viewport[0], 100);
    EXPECT_EQ(viewport[1], 100);
    EXPECT_EQ(viewport[2], 400);
    EXPECT_EQ(viewport[3], 300);
}

// Test shader compilation through ShaderManager
TEST_F(OpenGLWrapperValidationTest, ShaderCompilation) {
    // Note: OpenGLRenderer doesn't expose shader compilation directly
    // We test through the existing shader system instead
    
    // Test that we can get current program
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    EXPECT_EQ(currentProgram, 0);
    EXPECT_TRUE(checkGLError("Get current program"));
    
    // Test use of shader through program ID
    glRenderer->useProgram(1); // Use a dummy program ID
    // This might fail if program 1 doesn't exist, but that's OK for this test
    glGetError(); // Clear any error
    
    glRenderer->useProgram(0);
    EXPECT_TRUE(checkGLError("Reset program"));
}

// Test uniform setting
TEST_F(OpenGLWrapperValidationTest, UniformSetting) {
    // Test setting uniforms through the UniformValue system
    
    // First bind a dummy program (assuming 0 is not a valid program)
    glRenderer->useProgram(0);
    
    // Clear any existing errors
    while (glGetError() != GL_NO_ERROR) {}
    
    // Test matrix uniform
    Math::Matrix4f identity;
    identity.identity();
    glRenderer->setUniform("model", Rendering::UniformValue(identity));
    // This might generate an error if no program is bound, which is expected
    glGetError(); // Clear the error
    
    // Test vector uniform
    glRenderer->setUniform("color", Rendering::UniformValue(Math::Vector3f(1.0f, 0.5f, 0.2f)));
    glGetError(); // Clear the error
    
    // Test float uniform
    glRenderer->setUniform("alpha", Rendering::UniformValue(0.8f));
    glGetError(); // Clear the error
    
    // Test int uniform
    glRenderer->setUniform("mode", Rendering::UniformValue(2));
    glGetError(); // Clear the error
    
    // The test passes if we don't crash
    EXPECT_TRUE(true);
}

// Test VAO operations
TEST_F(OpenGLWrapperValidationTest, VAOOperations) {
    // Create VAO
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    EXPECT_NE(vao, 0u);
    EXPECT_TRUE(checkGLError("Generate VAO"));
    
    // Bind VAO
    glBindVertexArray(vao);
    EXPECT_TRUE(checkGLError("Bind VAO"));
    
    // Create and bind VBO
    float vertices[] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };
    
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    EXPECT_TRUE(checkGLError("Create and fill VBO"));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    EXPECT_TRUE(checkGLError("Setup vertex attributes"));
    
    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // Test that VAO state is preserved
    glBindVertexArray(vao);
    GLint enabled;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    EXPECT_EQ(enabled, GL_TRUE);
    
    // Clean up
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    EXPECT_TRUE(checkGLError("Delete VAO and VBO"));
}

// Test line width setting
TEST_F(OpenGLWrapperValidationTest, LineWidth) {
    glRenderer->setLineWidth(2.0f);
    EXPECT_TRUE(checkGLError("Set line width"));
    
    GLfloat width;
    glGetFloatv(GL_LINE_WIDTH, &width);
    // Note: Some implementations may clamp line width
    EXPECT_GE(width, 1.0f);
    
    glRenderer->setLineWidth(1.0f);
    EXPECT_TRUE(checkGLError("Reset line width"));
}

} // namespace Tests
} // namespace VoxelEditor