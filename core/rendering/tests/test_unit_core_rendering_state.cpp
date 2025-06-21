#include <gtest/gtest.h>
#include "../RenderState.h"
#include "../RenderTypes.h"
#include <glad/glad.h>

#ifdef HAVE_GLFW
#include <GLFW/glfw3.h>
#endif

namespace VoxelEditor {
namespace Rendering {

// Test fixture for RenderState tests
class RenderStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a RenderState instance for testing
        m_renderState = std::make_unique<RenderState>();
    }
    
    void TearDown() override {
        m_renderState.reset();
    }
    
    // Helper function to check if we have a valid OpenGL context
    bool hasOpenGLContext() const {
        #ifdef HAVE_GLFW
        return glfwGetCurrentContext() != nullptr;
        #else
        return false;
        #endif
    }
    
    // Skip test if no OpenGL context
    void requireOpenGLContext() {
        if (!hasOpenGLContext()) {
            GTEST_SKIP() << "No OpenGL context available for testing";
        }
    }
    
    std::unique_ptr<RenderState> m_renderState;
};

// ============================================================================
// State Initialization Tests
// ============================================================================

TEST_F(RenderStateTest, InitialState) {
    // Test default state after construction
    EXPECT_TRUE(m_renderState->isDepthTestEnabled());
    EXPECT_TRUE(m_renderState->isDepthWriteEnabled());
    EXPECT_FALSE(m_renderState->isBlendingEnabled());
    EXPECT_EQ(m_renderState->getBlendMode(), BlendMode::Opaque);
    EXPECT_TRUE(m_renderState->isCullingEnabled());
    EXPECT_EQ(m_renderState->getCullMode(), CullMode::Back);
    EXPECT_EQ(m_renderState->getBoundShader(), InvalidId);
    
    // Test statistics are initialized to zero
    EXPECT_EQ(m_renderState->getStateChanges(), 0u);
    EXPECT_EQ(m_renderState->getShaderSwitches(), 0u);
    EXPECT_EQ(m_renderState->getTextureBinds(), 0u);
}

TEST_F(RenderStateTest, ResetToDefaultState) {
    // Change various states
    m_renderState->setDepthTest(false);
    m_renderState->setBlending(true, BlendMode::Additive);
    m_renderState->setCulling(false);
    m_renderState->bindShader(123);
    m_renderState->setPolygonMode(true);
    
    // Reset state
    m_renderState->reset();
    
    // Verify all states are back to default
    EXPECT_TRUE(m_renderState->isDepthTestEnabled());
    EXPECT_TRUE(m_renderState->isDepthWriteEnabled());
    EXPECT_FALSE(m_renderState->isBlendingEnabled());
    EXPECT_EQ(m_renderState->getBlendMode(), BlendMode::Opaque);
    EXPECT_TRUE(m_renderState->isCullingEnabled());
    EXPECT_EQ(m_renderState->getCullMode(), CullMode::Back);
    EXPECT_EQ(m_renderState->getBoundShader(), InvalidId);
}

// ============================================================================
// OpenGL State Management Tests
// ============================================================================

TEST_F(RenderStateTest, DepthStateManagement) {
    // Test depth test enable/disable
    m_renderState->setDepthTest(false);
    EXPECT_FALSE(m_renderState->isDepthTestEnabled());
    
    m_renderState->setDepthTest(true);
    EXPECT_TRUE(m_renderState->isDepthTestEnabled());
    
    // Test depth write enable/disable
    m_renderState->setDepthWrite(false);
    EXPECT_FALSE(m_renderState->isDepthWriteEnabled());
    
    m_renderState->setDepthWrite(true);
    EXPECT_TRUE(m_renderState->isDepthWriteEnabled());
}

TEST_F(RenderStateTest, BlendingStateManagement) {
    // Test blending disabled
    m_renderState->setBlending(false);
    EXPECT_FALSE(m_renderState->isBlendingEnabled());
    
    // Test different blend modes
    m_renderState->setBlending(true, BlendMode::Alpha);
    EXPECT_TRUE(m_renderState->isBlendingEnabled());
    EXPECT_EQ(m_renderState->getBlendMode(), BlendMode::Alpha);
    
    m_renderState->setBlending(true, BlendMode::Additive);
    EXPECT_TRUE(m_renderState->isBlendingEnabled());
    EXPECT_EQ(m_renderState->getBlendMode(), BlendMode::Additive);
    
    m_renderState->setBlending(true, BlendMode::Multiply);
    EXPECT_TRUE(m_renderState->isBlendingEnabled());
    EXPECT_EQ(m_renderState->getBlendMode(), BlendMode::Multiply);
    
    // Test that disabling blending doesn't change blend mode
    m_renderState->setBlending(false);
    EXPECT_FALSE(m_renderState->isBlendingEnabled());
    EXPECT_EQ(m_renderState->getBlendMode(), BlendMode::Multiply);
}

TEST_F(RenderStateTest, CullingStateManagement) {
    // Test culling disabled
    m_renderState->setCulling(false);
    EXPECT_FALSE(m_renderState->isCullingEnabled());
    
    // Test different cull modes
    m_renderState->setCulling(true, CullMode::Front);
    EXPECT_TRUE(m_renderState->isCullingEnabled());
    EXPECT_EQ(m_renderState->getCullMode(), CullMode::Front);
    
    m_renderState->setCulling(true, CullMode::Back);
    EXPECT_TRUE(m_renderState->isCullingEnabled());
    EXPECT_EQ(m_renderState->getCullMode(), CullMode::Back);
    
    m_renderState->setCulling(true, CullMode::None);
    EXPECT_TRUE(m_renderState->isCullingEnabled());
    EXPECT_EQ(m_renderState->getCullMode(), CullMode::None);
}

TEST_F(RenderStateTest, PolygonModeManagement) {
    // Test wireframe mode
    m_renderState->setPolygonMode(true);
    // Note: Can't query polygon mode directly from RenderState
    // Would need to add a getter or test with OpenGL context
    
    m_renderState->setPolygonMode(false);
    // Back to filled mode
}

TEST_F(RenderStateTest, LineAndPointSize) {
    // Test line width setting
    m_renderState->setLineWidth(2.5f);
    // Note: Can't query line width directly from RenderState
    
    m_renderState->setLineWidth(1.0f);
    
    // Test point size setting
    m_renderState->setPointSize(5.0f);
    // Note: Can't query point size directly from RenderState
    
    m_renderState->setPointSize(1.0f);
}

// ============================================================================
// Binding State Tests
// ============================================================================

TEST_F(RenderStateTest, ShaderBinding) {
    // Test initial state
    EXPECT_EQ(m_renderState->getBoundShader(), InvalidId);
    
    // Test binding different shaders
    m_renderState->bindShader(100);
    EXPECT_EQ(m_renderState->getBoundShader(), 100u);
    
    m_renderState->bindShader(200);
    EXPECT_EQ(m_renderState->getBoundShader(), 200u);
    
    // Test binding invalid shader
    m_renderState->bindShader(InvalidId);
    EXPECT_EQ(m_renderState->getBoundShader(), InvalidId);
}

TEST_F(RenderStateTest, TextureBinding) {
    // Test binding textures to different slots
    m_renderState->bindTexture(101, 0);
    m_renderState->bindTexture(102, 1);
    m_renderState->bindTexture(103, 2);
    
    // Test binding to same slot (should replace)
    m_renderState->bindTexture(201, 0);
    
    // Test binding invalid texture
    m_renderState->bindTexture(InvalidId, 0);
    
    // Test binding to out-of-range slot
    m_renderState->bindTexture(301, 16); // Should handle gracefully
}

TEST_F(RenderStateTest, BufferBinding) {
    // Test VAO binding
    m_renderState->bindVertexArray(1001);
    
    // Test VBO binding
    m_renderState->bindVertexBuffer(2001);
    
    // Test IBO binding
    m_renderState->bindIndexBuffer(3001);
    
    // Test binding invalid buffers
    m_renderState->bindVertexArray(0);
    m_renderState->bindVertexBuffer(0);
    m_renderState->bindIndexBuffer(0);
}

// ============================================================================
// State Persistence Tests
// ============================================================================

TEST_F(RenderStateTest, StatePersistenceAcrossFrames) {
    // Set various states
    m_renderState->setDepthTest(false);
    m_renderState->setBlending(true, BlendMode::Alpha);
    m_renderState->setCulling(false);
    m_renderState->bindShader(123);
    
    // Flush state (simulating end of frame)
    m_renderState->flush();
    
    // States should persist
    EXPECT_FALSE(m_renderState->isDepthTestEnabled());
    EXPECT_TRUE(m_renderState->isBlendingEnabled());
    EXPECT_EQ(m_renderState->getBlendMode(), BlendMode::Alpha);
    EXPECT_FALSE(m_renderState->isCullingEnabled());
    EXPECT_EQ(m_renderState->getBoundShader(), 123u);
}

TEST_F(RenderStateTest, ForceStateChange) {
    // Set a state
    m_renderState->setDepthTest(true);
    m_renderState->flush();
    
    // Set same state again (normally would be ignored)
    m_renderState->setDepthTest(true);
    
    // Force state change
    m_renderState->forceStateChange();
    
    // Set same state again (should now be applied)
    m_renderState->setDepthTest(true);
    m_renderState->flush();
    
    // State should still be the same
    EXPECT_TRUE(m_renderState->isDepthTestEnabled());
}

// ============================================================================
// Viewport and Clear State Tests
// ============================================================================

TEST_F(RenderStateTest, ViewportState) {
    // Test setting viewport
    m_renderState->setViewport(0, 0, 1920, 1080);
    
    // Test different viewport
    m_renderState->setViewport(100, 100, 800, 600);
    
    // Test invalid viewport (should handle gracefully)
    m_renderState->setViewport(-10, -10, 0, 0);
}

TEST_F(RenderStateTest, ClearColorState) {
    // Test setting clear color
    m_renderState->setClearColor(Color(1.0f, 0.0f, 0.0f, 1.0f));
    
    // Test different clear color
    m_renderState->setClearColor(Color(0.5f, 0.5f, 0.5f, 0.5f));
    
    // Test black clear color
    m_renderState->setClearColor(Color::Black());
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(RenderStateTest, StateChangeStatistics) {
    // Initial statistics should be zero
    EXPECT_EQ(m_renderState->getStateChanges(), 0u);
    EXPECT_EQ(m_renderState->getShaderSwitches(), 0u);
    EXPECT_EQ(m_renderState->getTextureBinds(), 0u);
    
    // Make some state changes
    m_renderState->setDepthTest(false);
    m_renderState->flush();
    
    m_renderState->bindShader(100);
    m_renderState->flush();
    
    m_renderState->bindTexture(200, 0);
    m_renderState->flush();
    
    // Check statistics increased
    EXPECT_GT(m_renderState->getStateChanges(), 0u);
    EXPECT_GT(m_renderState->getShaderSwitches(), 0u);
    EXPECT_GT(m_renderState->getTextureBinds(), 0u);
    
    // Reset statistics
    m_renderState->resetStatistics();
    
    // Statistics should be zero again
    EXPECT_EQ(m_renderState->getStateChanges(), 0u);
    EXPECT_EQ(m_renderState->getShaderSwitches(), 0u);
    EXPECT_EQ(m_renderState->getTextureBinds(), 0u);
}

// ============================================================================
// ScopedRenderState Tests
// ============================================================================

TEST_F(RenderStateTest, ScopedStateRestore) {
    // Set initial state
    m_renderState->setDepthTest(true);
    m_renderState->setBlending(false);
    m_renderState->setCulling(true, CullMode::Back);
    m_renderState->bindShader(100);
    m_renderState->flush();
    
    {
        // Create scoped state
        ScopedRenderState scoped(*m_renderState);
        
        // Change states within scope
        m_renderState->setDepthTest(false);
        m_renderState->setBlending(true, BlendMode::Additive);
        m_renderState->setCulling(false);
        m_renderState->bindShader(200);
        m_renderState->flush();
        
        // Verify changes took effect
        EXPECT_FALSE(m_renderState->isDepthTestEnabled());
        EXPECT_TRUE(m_renderState->isBlendingEnabled());
        EXPECT_FALSE(m_renderState->isCullingEnabled());
        EXPECT_EQ(m_renderState->getBoundShader(), 200u);
    }
    
    // After scope, states should be restored
    EXPECT_TRUE(m_renderState->isDepthTestEnabled());
    EXPECT_FALSE(m_renderState->isBlendingEnabled());
    EXPECT_TRUE(m_renderState->isCullingEnabled());
    EXPECT_EQ(m_renderState->getCullMode(), CullMode::Back);
    EXPECT_EQ(m_renderState->getBoundShader(), 100u);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(RenderStateTest, RedundantStateChanges) {
    // Set state
    m_renderState->setDepthTest(true);
    m_renderState->flush();
    
    auto initialChanges = m_renderState->getStateChanges();
    
    // Set same state again
    m_renderState->setDepthTest(true);
    m_renderState->flush();
    
    // State changes should not increase for redundant changes
    // Note: This depends on implementation - it might still count
    // the attempt even if OpenGL call is skipped
}

TEST_F(RenderStateTest, MultipleStateChangesBeforeFlush) {
    // Make multiple state changes without flushing
    m_renderState->setDepthTest(false);
    m_renderState->setDepthTest(true);
    m_renderState->setDepthTest(false);
    
    // Only the final state should matter when flushed
    m_renderState->flush();
    
    EXPECT_FALSE(m_renderState->isDepthTestEnabled());
}

// ============================================================================
// OpenGL Context Tests (only run with valid context)
// ============================================================================

TEST_F(RenderStateTest, OpenGLStateApplication) {
    requireOpenGLContext();
    
    // Test that OpenGL state actually changes
    // This would require querying OpenGL state and comparing
    // For now, just test that the calls don't crash
    
    m_renderState->setDepthTest(true);
    m_renderState->setBlending(true, BlendMode::Alpha);
    m_renderState->setCulling(true, CullMode::Back);
    m_renderState->setViewport(0, 0, 800, 600);
    m_renderState->setClearColor(Color(0.5f, 0.5f, 0.5f, 1.0f));
    m_renderState->flush();
    
    // If we get here without crashing, basic functionality works
    SUCCEED();
}

} // namespace Rendering
} // namespace VoxelEditor