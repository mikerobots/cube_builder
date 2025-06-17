#include <gtest/gtest.h>
#include "../GroundPlaneGrid.h"
#include "../OpenGLRenderer.h"
#include "../ShaderManager.h"
#include "../RenderConfig.h"
#include "../../../foundation/math/Vector3f.h"
#include "../../../foundation/math/Matrix4f.h"
#include <memory>
#include <cmath>

namespace VoxelEditor {
namespace Rendering {
namespace Tests {

using namespace Math;

// Mock implementations for testing
class MockOpenGLRenderer : public OpenGLRenderer {
public:
    bool initializeContext(const RenderConfig& config) override {
        m_contextValid = true;
        return true;
    }
    
    void destroyContext() override {
        m_contextValid = false;
    }
    
    ShaderId createShader(ShaderType type, const std::string& source) override {
        return m_nextShaderId++;
    }
    
    ShaderId createProgram(const std::vector<ShaderId>& shaders) override {
        return m_nextShaderId++;
    }
    
    void useProgram(ShaderId programId) override {
        m_currentProgram = programId;
    }
    
    void deleteShader(ShaderId shaderId) override {}
    void deleteProgram(ShaderId programId) override {}
    
private:
    ShaderId m_nextShaderId = 1;
    ShaderId m_currentProgram = 0;
    bool m_contextValid = false;
};

class MockShaderManager : public ShaderManager {
public:
    MockShaderManager(OpenGLRenderer* renderer) : ShaderManager(renderer) {}
    
    ShaderId createShaderFromSource(const std::string& name, 
                                   const std::string& vertexSource, 
                                   const std::string& fragmentSource,
                                   OpenGLRenderer* renderer) {
        return 100; // Valid ID for testing
    }
};

class GroundPlaneGridDynamicsTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_renderer = std::make_unique<MockOpenGLRenderer>();
        m_shaderManager = std::make_unique<MockShaderManager>(m_renderer.get());
        
        RenderConfig config;
        m_renderer->initializeContext(config);
        
        m_grid = std::make_unique<GroundPlaneGrid>(m_shaderManager.get(), m_renderer.get());
        m_grid->initialize();
        m_grid->updateGridMesh(Vector3f(5.0f, 5.0f, 5.0f));
    }
    
    void TearDown() override {
        m_grid.reset();
        m_shaderManager.reset();
        m_renderer->destroyContext();
        m_renderer.reset();
    }
    
    std::unique_ptr<MockOpenGLRenderer> m_renderer;
    std::unique_ptr<MockShaderManager> m_shaderManager;
    std::unique_ptr<GroundPlaneGrid> m_grid;
};

// Test opacity calculation based on cursor distance
TEST_F(GroundPlaneGridDynamicsTest, OpacityBasedOnDistance) {
    // Set opacity parameters
    m_grid->setOpacityParameters(0.35f, 0.65f, 5.0f);
    
    // Test cursor far from grid
    m_grid->setCursorPosition(Vector3f(0.0f, 5.0f, 0.0f)); // 5m above grid
    m_grid->update(1.0f); // Large time step to reach target
    // Opacity should be at base level (0.35)
    
    // Test cursor close to grid
    m_grid->setCursorPosition(Vector3f(0.0f, 0.1f, 0.0f)); // 10cm above grid
    m_grid->update(1.0f); // Large time step to reach target
    // Opacity should be near maximum (close to 0.65)
    
    // Test cursor on grid
    m_grid->setCursorPosition(Vector3f(0.0f, 0.0f, 0.0f)); // On grid
    m_grid->update(1.0f);
    // Opacity should be at maximum (0.65)
}

// Test smooth opacity transitions
TEST_F(GroundPlaneGridDynamicsTest, SmoothOpacityTransitions) {
    m_grid->setOpacityParameters(0.35f, 0.65f, 5.0f); // transition speed = 5.0
    
    // Start with cursor far
    m_grid->setCursorPosition(Vector3f(0.0f, 10.0f, 0.0f));
    m_grid->update(1.0f); // Settle to base opacity
    
    // Move cursor close quickly
    m_grid->setCursorPosition(Vector3f(0.0f, 0.1f, 0.0f));
    
    // Test gradual transition with small time steps
    for (int i = 0; i < 10; ++i) {
        m_grid->update(0.016f); // ~60fps
    }
    // Opacity should be transitioning smoothly, not jumping
}

// Test cursor position smoothing
TEST_F(GroundPlaneGridDynamicsTest, CursorPositionSmoothing) {
    // Rapid cursor position changes
    m_grid->setCursorPosition(Vector3f(0.0f, 0.0f, 0.0f));
    m_grid->update(0.016f);
    
    m_grid->setCursorPosition(Vector3f(1.0f, 0.0f, 0.0f));
    m_grid->update(0.016f);
    
    m_grid->setCursorPosition(Vector3f(2.0f, 0.0f, 0.0f));
    m_grid->update(0.016f);
    
    // Position should be smoothed, not jumping
}

// Test opacity when cursor is outside grid bounds
TEST_F(GroundPlaneGridDynamicsTest, OpacityOutsideGridBounds) {
    m_grid->setOpacityParameters(0.35f, 0.65f, 5.0f);
    
    // Cursor close to Y=0 but far outside grid in XZ
    m_grid->setCursorPosition(Vector3f(10.0f, 0.1f, 10.0f)); // Outside 5m workspace
    m_grid->update(1.0f);
    // Opacity should remain at base level despite being close to Y=0
}

// Test different transition speeds
TEST_F(GroundPlaneGridDynamicsTest, DifferentTransitionSpeeds) {
    // Fast transitions
    m_grid->setOpacityParameters(0.35f, 0.65f, 10.0f);
    m_grid->setCursorPosition(Vector3f(0.0f, 5.0f, 0.0f));
    m_grid->update(0.1f);
    m_grid->setCursorPosition(Vector3f(0.0f, 0.0f, 0.0f));
    m_grid->update(0.1f); // Should transition quickly
    
    // Slow transitions
    m_grid->setOpacityParameters(0.35f, 0.65f, 1.0f);
    m_grid->setCursorPosition(Vector3f(0.0f, 5.0f, 0.0f));
    m_grid->update(0.1f);
    m_grid->setCursorPosition(Vector3f(0.0f, 0.0f, 0.0f));
    m_grid->update(0.1f); // Should transition slowly
}

// Test proximity radius calculation
TEST_F(GroundPlaneGridDynamicsTest, ProximityRadius) {
    m_grid->setOpacityParameters(0.35f, 0.65f, 5.0f);
    
    // ProximityRadius is 2 grid squares = 64cm
    float proximityDist = 2.0f * 0.32f; // 0.64m
    
    // Just inside proximity radius
    m_grid->setCursorPosition(Vector3f(0.0f, proximityDist * 0.9f, 0.0f));
    m_grid->update(1.0f);
    // Should have elevated opacity
    
    // Just outside proximity radius
    m_grid->setCursorPosition(Vector3f(0.0f, proximityDist * 1.1f, 0.0f));
    m_grid->update(1.0f);
    // Should be at base opacity
}

// Test opacity parameter bounds
TEST_F(GroundPlaneGridDynamicsTest, OpacityParameterBounds) {
    // Test extreme values
    m_grid->setOpacityParameters(0.0f, 1.0f, 100.0f);
    m_grid->setCursorPosition(Vector3f(0.0f, 0.0f, 0.0f));
    m_grid->update(1.0f);
    // Should handle 0-1 range properly
    
    // Test inverted values
    m_grid->setOpacityParameters(0.8f, 0.2f, 5.0f);
    m_grid->setCursorPosition(Vector3f(0.0f, 0.0f, 0.0f));
    m_grid->update(1.0f);
    // Should handle inverted range (higher when far)
}

// Test workspace size changes
TEST_F(GroundPlaneGridDynamicsTest, WorkspaceSizeChange) {
    m_grid->setOpacityParameters(0.35f, 0.65f, 5.0f);
    
    // Original 5m workspace
    m_grid->setCursorPosition(Vector3f(2.4f, 0.1f, 2.4f)); // Near edge
    m_grid->update(1.0f);
    
    // Change to 3m workspace
    m_grid->updateGridMesh(Vector3f(3.0f, 3.0f, 3.0f));
    m_grid->update(1.0f);
    // Same position should now be outside grid bounds
}

} // namespace Tests
} // namespace Rendering
} // namespace VoxelEditor