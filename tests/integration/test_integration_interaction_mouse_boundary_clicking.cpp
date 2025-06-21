#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include "math/Vector3f.h"
#include "math/Vector2f.h"
#include "math/Vector3i.h"
#include "math/MathUtils.h"
#include "logging/Logger.h"

// Forward declarations to avoid including CLI headers in integration tests
namespace VoxelEditor {
namespace CLI {
    class Application;
    class RenderWindow;
    class MouseInteraction;
}
namespace Camera {
    class OrbitCamera;
}
namespace VoxelData {
    class VoxelDataManager;
}
}

namespace VoxelEditor {
namespace Tests {

// Mock/stub classes for testing mouse interaction without full CLI dependency
class MouseBoundaryClickingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup logging
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::FileOutput>("mouse_boundary_test.log", "TestLog", false));
    }
    
    void TearDown() override {
    }
    
    // Simulate the expected behavior when clicking at workspace boundaries
    bool simulateBoundaryClick(const Math::Vector3f& worldPos, const Math::Vector3f& workspaceSize) {
        // Check if position is within workspace bounds
        Math::Vector3f halfSize = workspaceSize * 0.5f;
        
        if (worldPos.x < -halfSize.x || worldPos.x > halfSize.x ||
            worldPos.y < -halfSize.y || worldPos.y > halfSize.y ||
            worldPos.z < -halfSize.z || worldPos.z > halfSize.z) {
            // Outside bounds - should not place
            return false;
        }
        
        // Inside bounds - placement should succeed
        return true;
    }
    
    // Test helper to validate grid snapping at boundaries
    Math::Vector3i snapToGrid(const Math::Vector3f& worldPos, float resolution) {
        return Math::Vector3i(
            static_cast<int>(std::round(worldPos.x / resolution)),
            static_cast<int>(std::round(worldPos.y / resolution)),
            static_cast<int>(std::round(worldPos.z / resolution))
        );
    }
};

// Test clicking at all workspace corners
TEST_F(MouseBoundaryClickingTest, CornerClickValidation) {
    Math::Vector3f workspaceSize(4.0f, 4.0f, 4.0f);
    float resolution = 0.08f; // 8cm
    
    // Test all 8 corners
    std::vector<Math::Vector3f> corners = {
        {-2.0f, -2.0f, -2.0f},
        {-2.0f, -2.0f,  2.0f},
        {-2.0f,  2.0f, -2.0f},
        {-2.0f,  2.0f,  2.0f},
        { 2.0f, -2.0f, -2.0f},
        { 2.0f, -2.0f,  2.0f},
        { 2.0f,  2.0f, -2.0f},
        { 2.0f,  2.0f,  2.0f}
    };
    
    for (const auto& corner : corners) {
        // Corners are exactly on boundaries, so placement depends on implementation
        // Typically, voxels are placed just inside the boundary
        Math::Vector3f adjustedPos = corner;
        
        // Adjust position slightly inward to be within bounds
        adjustedPos.x = (corner.x > 0) ? corner.x - resolution/2 : corner.x + resolution/2;
        adjustedPos.y = (corner.y > 0) ? corner.y - resolution/2 : corner.y + resolution/2;
        adjustedPos.z = (corner.z > 0) ? corner.z - resolution/2 : corner.z + resolution/2;
        
        bool canPlace = simulateBoundaryClick(adjustedPos, workspaceSize);
        EXPECT_TRUE(canPlace) << "Should be able to place voxel near corner at " 
                             << adjustedPos.x << ", " << adjustedPos.y << ", " << adjustedPos.z;
        
        // Test grid snapping
        Math::Vector3i gridPos = snapToGrid(adjustedPos, resolution);
        EXPECT_NE(gridPos.x, 0) << "Corner grid position should not be at origin";
    }
}

// Test clicking exactly on boundaries
TEST_F(MouseBoundaryClickingTest, ExactBoundaryClicks) {
    Math::Vector3f workspaceSize(4.0f, 4.0f, 4.0f);
    
    // Test positions exactly on boundaries
    std::vector<Math::Vector3f> boundaryPositions = {
        {-2.0f, 0.0f, 0.0f},  // Left boundary
        { 2.0f, 0.0f, 0.0f},  // Right boundary
        {0.0f, -2.0f, 0.0f},  // Bottom boundary
        {0.0f,  2.0f, 0.0f},  // Top boundary
        {0.0f, 0.0f, -2.0f},  // Front boundary
        {0.0f, 0.0f,  2.0f},  // Back boundary
    };
    
    for (const auto& pos : boundaryPositions) {
        // Exactly on boundary - typically not allowed
        bool canPlace = simulateBoundaryClick(pos, workspaceSize);
        
        // Adjust slightly inward
        Math::Vector3f adjustedPos = pos * 0.95f; // 5% inward
        bool canPlaceAdjusted = simulateBoundaryClick(adjustedPos, workspaceSize);
        
        EXPECT_TRUE(canPlaceAdjusted) << "Should be able to place voxel just inside boundary";
    }
}

// Test clicking outside boundaries
TEST_F(MouseBoundaryClickingTest, OutsideBoundaryClicks) {
    Math::Vector3f workspaceSize(4.0f, 4.0f, 4.0f);
    
    // Test positions clearly outside boundaries
    std::vector<Math::Vector3f> outsidePositions = {
        {-3.0f, 0.0f, 0.0f},
        { 3.0f, 0.0f, 0.0f},
        {0.0f, -3.0f, 0.0f},
        {0.0f,  3.0f, 0.0f},
        {0.0f, 0.0f, -3.0f},
        {0.0f, 0.0f,  3.0f},
        {-5.0f, -5.0f, -5.0f}
    };
    
    for (const auto& pos : outsidePositions) {
        bool canPlace = simulateBoundaryClick(pos, workspaceSize);
        EXPECT_FALSE(canPlace) << "Should not be able to place voxel outside boundary at "
                              << pos.x << ", " << pos.y << ", " << pos.z;
    }
}

// Test boundary behavior with different resolutions
TEST_F(MouseBoundaryClickingTest, ResolutionBoundaryBehavior) {
    Math::Vector3f workspaceSize(4.0f, 4.0f, 4.0f);
    std::vector<float> resolutions = {0.01f, 0.04f, 0.08f, 0.16f, 0.32f};
    
    for (float resolution : resolutions) {
        // Test near corner with different resolutions
        Math::Vector3f cornerPos(-2.0f + resolution, 0.0f, -2.0f + resolution);
        
        bool canPlace = simulateBoundaryClick(cornerPos, workspaceSize);
        EXPECT_TRUE(canPlace) << "Should place voxel with resolution " << resolution;
        
        // Test grid snapping maintains boundary constraints
        Math::Vector3i gridPos = snapToGrid(cornerPos, resolution);
        float worldX = gridPos.x * resolution;
        float worldZ = gridPos.z * resolution;
        
        EXPECT_GE(worldX, -2.0f) << "Snapped position should be within X boundary";
        EXPECT_LE(worldX, 2.0f) << "Snapped position should be within X boundary";
        EXPECT_GE(worldZ, -2.0f) << "Snapped position should be within Z boundary";
        EXPECT_LE(worldZ, 2.0f) << "Snapped position should be within Z boundary";
    }
}

// Test ground plane constraint (Y >= 0)
TEST_F(MouseBoundaryClickingTest, GroundPlaneConstraint) {
    Math::Vector3f workspaceSize(4.0f, 4.0f, 4.0f);
    
    // Test positions below ground plane
    std::vector<Math::Vector3f> belowGroundPositions = {
        {0.0f, -0.1f, 0.0f},
        {1.0f, -1.0f, 1.0f},
        {0.0f, -2.0f, 0.0f}
    };
    
    for (const auto& pos : belowGroundPositions) {
        // In actual implementation, these would be clamped to Y=0
        Math::Vector3f clampedPos = pos;
        clampedPos.y = std::max(0.0f, pos.y);
        
        bool canPlace = simulateBoundaryClick(clampedPos, workspaceSize);
        if (clampedPos.y >= 0.0f && clampedPos.y <= 2.0f) {
            EXPECT_TRUE(canPlace) << "Should place voxel after clamping to ground at Y=0";
        }
    }
}

} // namespace Tests
} // namespace VoxelEditor