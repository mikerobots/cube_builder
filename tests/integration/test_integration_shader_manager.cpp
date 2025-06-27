#include <gtest/gtest.h>
#include "../../core/rendering/ShaderManager.h"
#include "../../core/rendering/OpenGLRenderer.h"
#include "../../core/rendering/RenderTypes.h"
#include "../../foundation/logging/Logger.h"
#include <memory>
#include <string>

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
        
        // Create real OpenGL renderer - will have proper context in integration environment
        renderer = std::make_unique<OpenGLRenderer>();
        
        // Initialize with test configuration
        RenderConfig config;
        config.windowWidth = 640;
        config.windowHeight = 480;
        config.windowTitle = "ShaderManager Integration Test";
        config.vsync = false;
        config.samples = 1;
        config.openGLMajor = 3;
        config.openGLMinor = 3;
        config.useCompatProfile = false;
        
        if (!renderer->initializeContext(config)) {
            GTEST_SKIP() << "OpenGL context initialization failed - skipping test";
        }
        
        shaderManager = std::make_unique<ShaderManager>();
    }
    
    void TearDown() override {
        shaderManager.reset();
        if (renderer) {
            renderer->destroyContext();
        }
        renderer.reset();
    }
    
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

// Test shader compilation error handling
TEST_F(IntegrationShaderManagerTest, ShaderCompilationErrorHandling) {
    // Vertex shader with syntax error
    const std::string errorVertex = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position
        // Missing semicolon above
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )";
    
    const std::string validFragment = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0);
        }
    )";
    
    // This should fail compilation
    ShaderId shader = shaderManager->createShaderFromSource(
        "error_shader",
        errorVertex,
        validFragment,
        renderer.get()
    );
    
    EXPECT_EQ(shader, InvalidId) << "Shader with syntax error should fail to compile";
}

// Test shader linking error handling
TEST_F(IntegrationShaderManagerTest, ShaderLinkingErrorHandling) {
    // Vertex shader with output varying
    const std::string vertexWithVarying = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        out vec4 v_color;
        
        void main() {
            gl_Position = vec4(a_position, 1.0);
            v_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    // Fragment shader expecting different varying
    const std::string fragmentWrongVarying = R"(
        #version 330 core
        in vec3 v_normal; // Mismatched varying
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(v_normal, 1.0);
        }
    )";
    
    // This should fail linking
    ShaderId shader = shaderManager->createShaderFromSource(
        "link_error_shader",
        vertexWithVarying,
        fragmentWrongVarying,
        renderer.get()
    );
    
    EXPECT_EQ(shader, InvalidId) << "Shader with mismatched varyings should fail to link";
}

} // namespace Rendering
} // namespace VoxelEditor