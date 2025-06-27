#include <gtest/gtest.h>
#include "../ShaderManager.h"
#include "../RenderTypes.h"
#include <memory>
#include <string>

namespace VoxelEditor {
namespace Rendering {

class ShaderManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<ShaderManager> shaderManager;
    
    void SetUp() override {
        shaderManager = std::make_unique<ShaderManager>();
    }
    
    void TearDown() override {
        shaderManager.reset();
    }
};

// Test basic functionality that doesn't require OpenGL
TEST_F(ShaderManagerTest, BasicOperations) {
    // Test getting non-existent shader
    ShaderId shader = shaderManager->getShader("nonexistent");
    EXPECT_EQ(shader, InvalidId);
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

// Test null renderer handling
TEST_F(ShaderManagerTest, NullRendererHandling) {
    const std::string vertexSource = "#version 330 core\nvoid main() {}";
    const std::string fragmentSource = "#version 330 core\nvoid main() {}";
    
    // Should handle null renderer gracefully
    ShaderId shader = shaderManager->createShaderFromSource(
        "null_test",
        vertexSource,
        fragmentSource,
        nullptr
    );
    
    EXPECT_EQ(shader, InvalidId);
}

// Test cleanup without any shaders
TEST_F(ShaderManagerTest, CleanupEmpty) {
    // Should not crash when cleaning up empty manager
    shaderManager->cleanup();
}

// Test getting shader names
TEST_F(ShaderManagerTest, GetShaderNames) {
    // Without any shaders loaded, should return empty or handle gracefully
    EXPECT_EQ(shaderManager->getShader(""), InvalidId);
    EXPECT_EQ(shaderManager->getShader("any_name"), InvalidId);
}

} // namespace Rendering
} // namespace VoxelEditor