#include <gtest/gtest.h>
#include "../ShaderManagerSafe.h"
#include "../OpenGLRenderer.h"
#include "../RenderTypes.h"
#include <memory>
#include <string>

using namespace VoxelEditor::Rendering;

class MockOpenGLRenderer : public OpenGLRenderer {
public:
    MockOpenGLRenderer() : OpenGLRenderer() {}
    
    ShaderId createShader(ShaderType type, const std::string& source) override {
        (void)type;
        (void)source;
        static ShaderId nextId = 1;
        return nextId++;
    }
    
    ShaderId createProgram(const std::vector<ShaderId>& shaders) override {
        (void)shaders;
        static ShaderId nextId = 100;
        return nextId++;
    }
    
    void deleteShader(ShaderId shaderId) override {
        (void)shaderId;
    }
    
    void deleteProgram(ShaderId programId) override {
        (void)programId;
    }
};

class ShaderManagerSafeTest : public ::testing::Test {
protected:
    void SetUp() override {
        renderer = std::make_unique<MockOpenGLRenderer>();
    }
    
    void TearDown() override {
        renderer.reset();
    }
    
    std::unique_ptr<MockOpenGLRenderer> renderer;
};

// Test with null/silent logger
TEST_F(ShaderManagerSafeTest, BasicFunctionalityWithSilentLogger) {
    auto shaderManager = ShaderManagerSafe::createSilent();
    
    // Test getting non-existent shader
    ShaderId shader = shaderManager->getShader("nonexistent");
    EXPECT_EQ(shader, InvalidId);
}

// Test with test logger (safe logging)
TEST_F(ShaderManagerSafeTest, BasicFunctionalityWithTestLogger) {
    auto shaderManager = ShaderManagerSafe::createForTesting();
    
    const std::string vertexSource = R"(
        #version 120
        attribute vec3 a_position;
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )";
    
    const std::string fragmentSource = R"(
        #version 120
        void main() {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    // This should work without crashing
    ShaderId shader = shaderManager->createShaderFromSource(
        "test_shader", 
        vertexSource, 
        fragmentSource,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId);
    EXPECT_EQ(shaderManager->getShader("test_shader"), shader);
}

// Test with production logger (should be safe now)
TEST_F(ShaderManagerSafeTest, BasicFunctionalityWithProductionLogger) {
    auto shaderManager = ShaderManagerSafe::createForProduction();
    
    const std::string vertexSource = R"(
        #version 120
        attribute vec3 a_position;
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )";
    
    const std::string fragmentSource = R"(
        #version 120
        void main() {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    // This should work even with the production logger
    ShaderId shader = shaderManager->createShaderFromSource(
        "production_test_shader", 
        vertexSource, 
        fragmentSource,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId);
    EXPECT_EQ(shaderManager->getShader("production_test_shader"), shader);
}

// Test multiple shaders with logging
TEST_F(ShaderManagerSafeTest, MultipleShaders) {
    auto shaderManager = ShaderManagerSafe::createForTesting();
    
    const std::string simpleVertex = "#version 120\nvoid main() {}";
    const std::string simpleFragment = "#version 120\nvoid main() {}";
    
    // Create multiple shaders
    ShaderId shader1 = shaderManager->createShaderFromSource(
        "shader1", simpleVertex, simpleFragment, renderer.get());
    ShaderId shader2 = shaderManager->createShaderFromSource(
        "shader2", simpleVertex, simpleFragment, renderer.get());
    ShaderId shader3 = shaderManager->createShaderFromSource(
        "shader3", simpleVertex, simpleFragment, renderer.get());
    
    // All should be valid and different
    EXPECT_NE(shader1, InvalidId);
    EXPECT_NE(shader2, InvalidId);
    EXPECT_NE(shader3, InvalidId);
    EXPECT_NE(shader1, shader2);
    EXPECT_NE(shader2, shader3);
    EXPECT_NE(shader1, shader3);
    
    // Should be able to retrieve all by name
    EXPECT_EQ(shaderManager->getShader("shader1"), shader1);
    EXPECT_EQ(shaderManager->getShader("shader2"), shader2);
    EXPECT_EQ(shaderManager->getShader("shader3"), shader3);
}

// Test error handling with null renderer
TEST_F(ShaderManagerSafeTest, NullRendererHandling) {
    auto shaderManager = ShaderManagerSafe::createForTesting();
    
    const std::string simpleVertex = "#version 120\nvoid main() {}";
    const std::string simpleFragment = "#version 120\nvoid main() {}";
    
    // Create shader with null renderer
    ShaderId shader = shaderManager->createShaderFromSource(
        "null_test", 
        simpleVertex, 
        simpleFragment,
        nullptr
    );
    
    // Should return InvalidId
    EXPECT_EQ(shader, InvalidId);
}

// Test the complex shader that matches what the original application uses
TEST_F(ShaderManagerSafeTest, ComplexShaderFromRenderEngine) {
    auto shaderManager = ShaderManagerSafe::createForTesting();
    
    // This is the exact shader from RenderEngine.cpp
    const std::string basicVertex = R"(
#version 120
attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec4 a_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

varying vec4 v_color;
varying vec3 v_normal;

void main() {
    // Transform through MVP
    vec4 worldPos = model * vec4(a_position, 1.0);
    vec4 viewPos = view * worldPos;
    vec4 clipPos = projection * viewPos;
    
    // Output the clip position
    gl_Position = clipPos;
    
    // Pass through color and normal
    v_color = a_color;
    v_normal = mat3(model) * a_normal;
}
    )";
    
    const std::string basicFragment = R"(
#version 120

varying vec4 v_color;
varying vec3 v_normal;

void main() {
    // Simple directional lighting with high ambient
    vec3 lightDir = normalize(vec3(0.5, -1.0, 0.3));
    vec3 normal = normalize(v_normal);
    
    float NdotL = max(dot(normal, -lightDir), 0.0);
    float lighting = 0.7 + 0.3 * NdotL;  // High ambient (0.7) to ensure visibility
    
    // Output lit color
    gl_FragColor = vec4(v_color.rgb * lighting, v_color.a);
}
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource(
        "basic_lit_shader",
        basicVertex,
        basicFragment,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId);
    EXPECT_EQ(shaderManager->getShader("basic_lit_shader"), shader);
}

// Stress test - create many shaders to ensure no memory issues
TEST_F(ShaderManagerSafeTest, StressTestManyShaders) {
    auto shaderManager = ShaderManagerSafe::createSilent(); // Use silent to avoid too much output
    
    const std::string simpleVertex = "#version 120\nvoid main() {}";
    const std::string simpleFragment = "#version 120\nvoid main() {}";
    
    std::vector<ShaderId> shaders;
    const int numShaders = 50;
    
    for (int i = 0; i < numShaders; ++i) {
        std::string name = "stress_shader_" + std::to_string(i);
        ShaderId shader = shaderManager->createShaderFromSource(
            name, simpleVertex, simpleFragment, renderer.get());
        
        EXPECT_NE(shader, InvalidId);
        shaders.push_back(shader);
        
        // Verify we can retrieve it
        EXPECT_EQ(shaderManager->getShader(name), shader);
    }
    
    // All shaders should be unique
    for (int i = 0; i < numShaders; ++i) {
        for (int j = i + 1; j < numShaders; ++j) {
            EXPECT_NE(shaders[i], shaders[j]);
        }
    }
}