#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include "../../core/rendering/ShaderManager.h"
#include "../../core/rendering/OpenGLRenderer.h"
#include "../../core/rendering/RenderTypes.h"
#include "../../foundation/logging/Logger.h"
#include <memory>
#include <string>

#ifdef __APPLE__
#include "../../core/rendering/MacOSGLLoader.h"
#else
#include <glad/glad.h>
#endif

namespace VoxelEditor {
namespace Rendering {

class IntegrationShaderManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for debugging
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::ConsoleOutput>("Test"));
        
        // Initialize GLFW
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        
        // Configure GLFW for OpenGL 3.3 Core Profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden for automated testing
        
        #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
        
        // Create window
        window = glfwCreateWindow(640, 480, "Shader Manager Test", nullptr, nullptr);
        ASSERT_NE(window, nullptr) << "Failed to create GLFW window";
        
        glfwMakeContextCurrent(window);
        
        #ifndef __APPLE__
        // Initialize GLAD (not needed on macOS)
        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) << "Failed to initialize GLAD";
        #else
        // Load OpenGL extensions on macOS
        ASSERT_TRUE(LoadOpenGLExtensions()) << "Failed to load OpenGL extensions on macOS";
        #endif
        
        // Clear any GL errors from initialization
        while (glGetError() != GL_NO_ERROR) {}
        
        // Create real OpenGL renderer
        renderer = std::make_unique<OpenGLRenderer>();
        
        // Initialize with test configuration
        RenderConfig config;
        config.windowWidth = 640;
        config.windowHeight = 480;
        config.vsync = false;
        config.samples = 1;
        
        ASSERT_TRUE(renderer->initializeContext(config)) << "Failed to initialize OpenGL renderer";
        
        shaderManager = std::make_unique<ShaderManager>();
    }
    
    void TearDown() override {
        shaderManager.reset();
        if (renderer) {
            renderer->destroyContext();
        }
        renderer.reset();
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    GLFWwindow* window = nullptr;
    std::unique_ptr<OpenGLRenderer> renderer;
    std::unique_ptr<ShaderManager> shaderManager;
};

// Test creating shader from source with real OpenGL
TEST_F(IntegrationShaderManagerTest, CreateShaderFromSource) {
    const std::string vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )";
    
    const std::string fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource(
        "test_shader", 
        vertexSource, 
        fragmentSource,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId);
    EXPECT_EQ(shaderManager->getShader("test_shader"), shader);
}

// Test creating multiple shaders
TEST_F(IntegrationShaderManagerTest, MultipleShaders) {
    const std::string simpleVertex = "#version 330 core\nlayout(location = 0) in vec3 a_position;\nvoid main() { gl_Position = vec4(a_position, 1.0); }";
    const std::string simpleFragment = "#version 330 core\nout vec4 FragColor;\nvoid main() { FragColor = vec4(1.0); }";
    
    ShaderId shader1 = shaderManager->createShaderFromSource(
        "shader1", simpleVertex, simpleFragment, renderer.get());
    ShaderId shader2 = shaderManager->createShaderFromSource(
        "shader2", simpleVertex, simpleFragment, renderer.get());
    ShaderId shader3 = shaderManager->createShaderFromSource(
        "shader3", simpleVertex, simpleFragment, renderer.get());
    
    EXPECT_NE(shader1, InvalidId);
    EXPECT_NE(shader2, InvalidId);
    EXPECT_NE(shader3, InvalidId);
    EXPECT_NE(shader1, shader2);
    EXPECT_NE(shader2, shader3);
    EXPECT_NE(shader1, shader3);
}

// Test shader replacement
TEST_F(IntegrationShaderManagerTest, ShaderReplacement) {
    const std::string simpleVertex = "#version 330 core\nlayout(location = 0) in vec3 a_position;\nvoid main() { gl_Position = vec4(a_position, 1.0); }";
    const std::string simpleFragment = "#version 330 core\nout vec4 FragColor;\nvoid main() { FragColor = vec4(1.0); }";
    
    ShaderId shader1 = shaderManager->createShaderFromSource(
        "replaceable", simpleVertex, simpleFragment, renderer.get());
    EXPECT_NE(shader1, InvalidId);
    
    // Replace with same name
    ShaderId shader2 = shaderManager->createShaderFromSource(
        "replaceable", simpleVertex, simpleFragment, renderer.get());
    EXPECT_NE(shader2, InvalidId);
    
    // Should get the new shader when querying by name
    EXPECT_EQ(shaderManager->getShader("replaceable"), shader2);
}

// Test shader cleanup
TEST_F(IntegrationShaderManagerTest, ShaderCleanup) {
    const std::string simpleVertex = "#version 330 core\nvoid main() {}";
    const std::string simpleFragment = "#version 330 core\nvoid main() {}";
    
    // Create some shaders
    shaderManager->createShaderFromSource("shader1", simpleVertex, simpleFragment, renderer.get());
    shaderManager->createShaderFromSource("shader2", simpleVertex, simpleFragment, renderer.get());
    shaderManager->createShaderFromSource("shader3", simpleVertex, simpleFragment, renderer.get());
    
    // Clean up
    shaderManager->cleanup();
    
    // All shaders should be gone
    EXPECT_EQ(shaderManager->getShader("shader1"), InvalidId);
    EXPECT_EQ(shaderManager->getShader("shader2"), InvalidId);
    EXPECT_EQ(shaderManager->getShader("shader3"), InvalidId);
}

// Test minimal shader program
TEST_F(IntegrationShaderManagerTest, MinimalShaderProgram) {
    const std::string minimalVertex = R"(
        #version 330 core
        void main() {
            gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        }
    )";
    
    const std::string minimalFragment = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource(
        "minimal_shader",
        minimalVertex,
        minimalFragment,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId) << "Minimal shader should compile and link successfully";
    EXPECT_EQ(shaderManager->getShader("minimal_shader"), shader);
}

// Test shader with attributes
TEST_F(IntegrationShaderManagerTest, BasicAttributesShader) {
    const std::string vertexWithAttributes = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_normal;
        layout(location = 2) in vec2 a_texCoord;
        
        out vec3 v_normal;
        out vec2 v_texCoord;
        
        uniform mat4 u_mvpMatrix;
        
        void main() {
            gl_Position = u_mvpMatrix * vec4(a_position, 1.0);
            v_normal = a_normal;
            v_texCoord = a_texCoord;
        }
    )";
    
    const std::string fragmentWithVaryings = R"(
        #version 330 core
        in vec3 v_normal;
        in vec2 v_texCoord;
        
        out vec4 FragColor;
        
        uniform vec4 u_color;
        
        void main() {
            // Simple lighting calculation
            vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
            float diffuse = max(dot(normalize(v_normal), lightDir), 0.0);
            FragColor = u_color * (0.3 + 0.7 * diffuse);
        }
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource(
        "attributes_shader",
        vertexWithAttributes,
        fragmentWithVaryings,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId) << "Shader with attributes should compile and link successfully";
    EXPECT_EQ(shaderManager->getShader("attributes_shader"), shader);
}

// NOTE: Error handling tests removed due to new "fail hard" assertion policy
// The OpenGLRenderer now asserts on shader compilation/linking failures
// instead of gracefully returning InvalidId. This is intentional to catch
// issues early in development.

} // namespace Rendering
} // namespace VoxelEditor