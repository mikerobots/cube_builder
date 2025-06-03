#include <gtest/gtest.h>
#include "../ShaderManager.h"
#include "../OpenGLRenderer.h"
#include "../RenderTypes.h"
#include <memory>
#include <string>

namespace VoxelEditor {
namespace Rendering {

class MockOpenGLRenderer : public OpenGLRenderer {
public:
    MockOpenGLRenderer() : OpenGLRenderer() {}
    
    // Override methods to simulate shader creation
    ShaderId createShader(ShaderType type, const std::string& source) {
        (void)type;
        (void)source;
        static ShaderId nextId = 1;
        return nextId++;
    }
    
    ShaderId createProgram(const std::vector<ShaderId>& shaders) {
        (void)shaders;
        static ShaderId nextId = 100;
        return nextId++;
    }
    
    void deleteShader(ShaderId shaderId) {
        // Mock implementation
        (void)shaderId;
    }
    
    void deleteProgram(ShaderId programId) {
        // Mock implementation
        (void)programId;
    }
};

class ShaderManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<ShaderManager> shaderManager;
    std::unique_ptr<MockOpenGLRenderer> renderer;
    
    void SetUp() override {
        shaderManager = std::make_unique<ShaderManager>();
        renderer = std::make_unique<MockOpenGLRenderer>();
    }
    
    void TearDown() override {
        shaderManager.reset();
        renderer.reset();
    }
};

// Test basic functionality
TEST_F(ShaderManagerTest, BasicOperations) {
    // Test getting non-existent shader
    ShaderId shader = shaderManager->getShader("nonexistent");
    EXPECT_EQ(shader, InvalidId);
}

// Test shader creation from source
TEST_F(ShaderManagerTest, CreateShaderFromSource) {
    const std::string vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        void main() {
            gl_Position = vec4(position, 1.0);
        }
    )";
    
    const std::string fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    // Create shader
    ShaderId shader = shaderManager->createShaderFromSource(
        "test_shader", 
        vertexSource, 
        fragmentSource,
        renderer.get()
    );
    
    // Should get a valid ID from our mock
    EXPECT_NE(shader, InvalidId);
    
    // Should be able to retrieve it by name
    ShaderId retrieved = shaderManager->getShader("test_shader");
    EXPECT_EQ(retrieved, shader);
}

// Test multiple shader creation
TEST_F(ShaderManagerTest, MultipleShaders) {
    const std::string simpleVertex = "#version 330 core\nvoid main() {}";
    const std::string simpleFragment = "#version 330 core\nvoid main() {}";
    
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

// Test shader replacement
TEST_F(ShaderManagerTest, ShaderReplacement) {
    const std::string simpleVertex = "#version 330 core\nvoid main() {}";
    const std::string simpleFragment = "#version 330 core\nvoid main() {}";
    
    // Create initial shader
    ShaderId shader1 = shaderManager->createShaderFromSource(
        "replaceable", simpleVertex, simpleFragment, renderer.get());
    EXPECT_NE(shader1, InvalidId);
    
    // Create replacement with same name
    ShaderId shader2 = shaderManager->createShaderFromSource(
        "replaceable", simpleVertex, simpleFragment, renderer.get());
    EXPECT_NE(shader2, InvalidId);
    
    // Should get the new shader when querying by name
    EXPECT_EQ(shaderManager->getShader("replaceable"), shader2);
}

// Test loading from file (will fail as not implemented)
TEST_F(ShaderManagerTest, LoadFromFile) {
    ShaderId shader = shaderManager->loadShaderFromFile(
        "file_shader", 
        "vertex.glsl", 
        "fragment.glsl"
    );
    
    // This is not implemented yet, so should return InvalidId
    EXPECT_EQ(shader, InvalidId);
}

// Test shader cleanup
TEST_F(ShaderManagerTest, Cleanup) {
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

// Test reloadAllShaders (stub implementation)
TEST_F(ShaderManagerTest, ReloadAllShaders) {
    // This is just a stub for now
    shaderManager->reloadAllShaders();
    // No crash is success for a stub
}

// Test hot reload functionality
TEST_F(ShaderManagerTest, HotReloadToggle) {
    // Hot reload is not implemented but we can test the interface
    shaderManager->setHotReloadEnabled(true);
    EXPECT_TRUE(shaderManager->isHotReloadEnabled());
    
    shaderManager->setHotReloadEnabled(false);
    EXPECT_FALSE(shaderManager->isHotReloadEnabled());
}

// Test error handling with null renderer
TEST_F(ShaderManagerTest, NullRendererHandling) {
    const std::string simpleVertex = "#version 330 core\nvoid main() {}";
    const std::string simpleFragment = "#version 330 core\nvoid main() {}";
    
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

} // namespace Rendering
} // namespace VoxelEditor