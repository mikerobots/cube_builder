#include <gtest/gtest.h>
#include "../GroundPlaneGrid.h"
#include "../../../foundation/math/Vector3f.h"
#include <memory>
#include <cmath>

namespace VoxelEditor {
namespace Rendering {
namespace Tests {

using namespace Math;

// Test the non-rendering logic of GroundPlaneGrid
class GroundPlaneGridDynamicsSimpleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a GroundPlaneGrid instance without initializing it
        // This allows us to test the logic without OpenGL dependencies
        m_grid = std::make_unique<GroundPlaneGrid>(nullptr, nullptr);
    }
    
    std::unique_ptr<GroundPlaneGrid> m_grid;
};

// Test opacity parameter setting
TEST_F(GroundPlaneGridDynamicsSimpleTest, SetOpacityParameters) {
    // Test setting opacity parameters
    m_grid->setOpacityParameters(0.2f, 0.8f, 10.0f);
    
    // Parameters are stored correctly (we can't verify directly but no crash is good)
    EXPECT_NO_THROW(m_grid->setOpacityParameters(0.0f, 1.0f, 0.0f));
    EXPECT_NO_THROW(m_grid->setOpacityParameters(1.0f, 0.0f, 100.0f));
}

// Test cursor position setting
TEST_F(GroundPlaneGridDynamicsSimpleTest, SetCursorPosition) {
    // Test setting cursor position
    Vector3f pos1(1.0f, 2.0f, 3.0f);
    EXPECT_NO_THROW(m_grid->setCursorPosition(pos1));
    
    Vector3f pos2(-5.0f, 0.0f, 10.0f);
    EXPECT_NO_THROW(m_grid->setCursorPosition(pos2));
    
    Vector3f pos3(0.0f, 0.0f, 0.0f);
    EXPECT_NO_THROW(m_grid->setCursorPosition(pos3));
}

// Test update method without rendering
TEST_F(GroundPlaneGridDynamicsSimpleTest, UpdateWithoutInit) {
    // Set initial cursor position
    m_grid->setCursorPosition(Vector3f(0.0f, 5.0f, 0.0f));
    
    // Update should work even without initialization
    EXPECT_NO_THROW(m_grid->update(0.016f)); // 60 FPS
    EXPECT_NO_THROW(m_grid->update(1.0f));   // Large time step
    EXPECT_NO_THROW(m_grid->update(0.0f));   // Zero time step
}

// Test visibility toggle
TEST_F(GroundPlaneGridDynamicsSimpleTest, VisibilityToggle) {
    // Default should be visible
    EXPECT_TRUE(m_grid->isVisible());
    
    // Test toggling visibility
    m_grid->setVisible(false);
    EXPECT_FALSE(m_grid->isVisible());
    
    m_grid->setVisible(true);
    EXPECT_TRUE(m_grid->isVisible());
}

// Test grid constants
TEST_F(GroundPlaneGridDynamicsSimpleTest, GridConstants) {
    // Test static grid parameters
    EXPECT_EQ(GroundPlaneGrid::getGridCellSize(), 0.32f);
    EXPECT_EQ(GroundPlaneGrid::getMajorLineInterval(), 1.6f);
}

// Test multiple updates with position changes
TEST_F(GroundPlaneGridDynamicsSimpleTest, MultipleUpdatesWithMovement) {
    m_grid->setOpacityParameters(0.35f, 0.65f, 5.0f);
    
    // Simulate cursor movement over time
    std::vector<Vector3f> positions = {
        Vector3f(0.0f, 5.0f, 0.0f),   // Far above
        Vector3f(0.0f, 2.0f, 0.0f),   // Moving down
        Vector3f(0.0f, 1.0f, 0.0f),   // Getting closer
        Vector3f(0.0f, 0.5f, 0.0f),   // Near grid
        Vector3f(0.0f, 0.0f, 0.0f),   // On grid
        Vector3f(1.0f, 0.0f, 1.0f),   // Moving horizontally
        Vector3f(5.0f, 0.1f, 5.0f),   // Outside grid bounds
    };
    
    for (const auto& pos : positions) {
        m_grid->setCursorPosition(pos);
        EXPECT_NO_THROW(m_grid->update(0.016f));
    }
}

// Test edge cases for update
TEST_F(GroundPlaneGridDynamicsSimpleTest, UpdateEdgeCases) {
    // Test with extreme cursor positions
    m_grid->setCursorPosition(Vector3f(1000.0f, 1000.0f, 1000.0f));
    EXPECT_NO_THROW(m_grid->update(0.016f));
    
    m_grid->setCursorPosition(Vector3f(-1000.0f, -1000.0f, -1000.0f));
    EXPECT_NO_THROW(m_grid->update(0.016f));
    
    // Test with NaN/Inf positions (should handle gracefully)
    m_grid->setCursorPosition(Vector3f(0.0f, std::numeric_limits<float>::infinity(), 0.0f));
    EXPECT_NO_THROW(m_grid->update(0.016f));
    
    // Test with very large time steps
    m_grid->setCursorPosition(Vector3f(0.0f, 0.0f, 0.0f));
    EXPECT_NO_THROW(m_grid->update(1000.0f));
    
    // Test with negative time steps
    EXPECT_NO_THROW(m_grid->update(-1.0f));
}

// Test updateGridMesh without initialization
TEST_F(GroundPlaneGridDynamicsSimpleTest, UpdateGridMeshWithoutInit) {
    // Should handle mesh updates gracefully even without GL context
    EXPECT_NO_THROW(m_grid->updateGridMesh(Vector3f(5.0f, 5.0f, 5.0f)));
    EXPECT_NO_THROW(m_grid->updateGridMesh(Vector3f(10.0f, 10.0f, 10.0f)));
    EXPECT_NO_THROW(m_grid->updateGridMesh(Vector3f(0.0f, 0.0f, 0.0f)));
}

} // namespace Tests
} // namespace Rendering
} // namespace VoxelEditor