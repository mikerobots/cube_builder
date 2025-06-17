#include <gtest/gtest.h>
#include "../VoxelGrid.h"
#include <algorithm>

using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class VoxelGridTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize sparse octree pool
        SparseOctree::initializePool(512);
        
        workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
        resolution = VoxelResolution::Size_1cm;
    }
    
    void TearDown() override {
        SparseOctree::shutdownPool();
    }
    
    Vector3f workspaceSize;
    VoxelResolution resolution;
};

TEST_F(VoxelGridTest, ConstructionWithValidParameters) {
    VoxelGrid grid(resolution, workspaceSize);
    
    EXPECT_EQ(grid.getResolution(), resolution);
    EXPECT_EQ(grid.getWorkspaceSize(), workspaceSize);
    EXPECT_EQ(grid.getVoxelCount(), 0);
    EXPECT_GT(grid.getMemoryUsage(), 0);
    
    // Should be able to handle expected grid dimensions
    Vector3i expectedDims = calculateMaxGridDimensions(resolution, workspaceSize);
    EXPECT_EQ(grid.getGridDimensions(), expectedDims);
}

TEST_F(VoxelGridTest, ConstructionWithDifferentResolutions) {
    for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
        VoxelResolution res = static_cast<VoxelResolution>(i);
        VoxelGrid grid(res, workspaceSize);
        
        EXPECT_EQ(grid.getResolution(), res);
        EXPECT_EQ(grid.getVoxelSize(), getVoxelSize(res));
        EXPECT_EQ(grid.getVoxelCount(), 0);
    }
}

TEST_F(VoxelGridTest, BasicVoxelOperations) {
    VoxelGrid grid(resolution, workspaceSize);
    Vector3i pos(10, 20, 30);
    
    // Initially empty
    EXPECT_FALSE(grid.getVoxel(pos));
    EXPECT_FALSE(grid.getVoxel(pos));
    EXPECT_EQ(grid.getVoxelCount(), 0);
    
    // Set voxel
    EXPECT_TRUE(grid.setVoxel(pos, true));
    EXPECT_TRUE(grid.getVoxel(pos));
    EXPECT_TRUE(grid.getVoxel(pos));
    EXPECT_EQ(grid.getVoxelCount(), 1);
    
    // Clear voxel
    EXPECT_TRUE(grid.setVoxel(pos, false));
    EXPECT_FALSE(grid.getVoxel(pos));
    EXPECT_FALSE(grid.getVoxel(pos));
    EXPECT_EQ(grid.getVoxelCount(), 0);
}

TEST_F(VoxelGridTest, WorldSpaceOperations) {
    VoxelGrid grid(resolution, workspaceSize);
    Vector3f worldPos(0.0f, 0.0f, 0.0f); // Center of workspace
    
    // Set voxel at world position
    EXPECT_TRUE(grid.setVoxelAtWorldPos(worldPos, true));
    EXPECT_TRUE(grid.getVoxelAtWorldPos(worldPos));
    EXPECT_TRUE(grid.getVoxelAtWorldPos(worldPos));
    EXPECT_EQ(grid.getVoxelCount(), 1);
    
    // Clear voxel at world position
    EXPECT_TRUE(grid.setVoxelAtWorldPos(worldPos, false));
    EXPECT_FALSE(grid.getVoxelAtWorldPos(worldPos));
    EXPECT_FALSE(grid.getVoxelAtWorldPos(worldPos));
    EXPECT_EQ(grid.getVoxelCount(), 0);
}

TEST_F(VoxelGridTest, GridWorldCoordinateConversion) {
    VoxelGrid grid(resolution, workspaceSize);
    
    // Test conversion from grid to world with centered coordinate system
    // For a 5m workspace with 1cm voxels, grid coordinates range from (0,0,0) to (500,500,500)
    // Grid center at (250,250,250) should map to world (0,2.5,0)
    Vector3i centerGridPos(250, 250, 250);
    Vector3f centerWorldPos = grid.gridToWorld(centerGridPos);
    
    // Center of workspace: X and Z should be 0 (centered), Y should be 2.5 (250 * 0.01)
    EXPECT_NEAR(centerWorldPos.x, 0.0f, 0.0001f);
    EXPECT_NEAR(centerWorldPos.y, 2.5f, 0.0001f);
    EXPECT_NEAR(centerWorldPos.z, 0.0f, 0.0001f);
    
    // Test edge positions
    Vector3i edgeGridPos(500, 500, 500); // Max edge
    Vector3f edgeWorldPos = grid.gridToWorld(edgeGridPos);
    
    // Should be at positive edge: (2.5, 5.0, 2.5)
    EXPECT_NEAR(edgeWorldPos.x, 2.5f, 0.0001f);
    EXPECT_NEAR(edgeWorldPos.y, 5.0f, 0.0001f);
    EXPECT_NEAR(edgeWorldPos.z, 2.5f, 0.0001f);
    
    // Test conversion from world to grid
    Vector3f testWorldPos(1.0f, 2.0f, 0.5f);
    Vector3i convertedGridPos = grid.worldToGrid(testWorldPos);
    
    // Verify round-trip conversion
    Vector3f roundTripWorldPos = grid.gridToWorld(convertedGridPos);
    
    // Should be close (within voxel size)
    float voxelSize = getVoxelSize(resolution);
    EXPECT_LT(abs(roundTripWorldPos.x - testWorldPos.x), voxelSize);
    EXPECT_LT(abs(roundTripWorldPos.y - testWorldPos.y), voxelSize);
    EXPECT_LT(abs(roundTripWorldPos.z - testWorldPos.z), voxelSize);
}

TEST_F(VoxelGridTest, PositionValidation) {
    VoxelGrid grid(resolution, workspaceSize);
    Vector3i maxDims = grid.getGridDimensions();
    
    // Valid positions
    EXPECT_TRUE(grid.isValidGridPosition(Vector3i(0, 0, 0)));
    EXPECT_TRUE(grid.isValidGridPosition(Vector3i(maxDims.x / 2, maxDims.y / 2, maxDims.z / 2)));
    EXPECT_TRUE(grid.isValidGridPosition(Vector3i(maxDims.x - 1, maxDims.y - 1, maxDims.z - 1)));
    
    // Invalid positions
    EXPECT_FALSE(grid.isValidGridPosition(Vector3i(-1, 0, 0)));
    EXPECT_FALSE(grid.isValidGridPosition(Vector3i(0, -1, 0)));
    EXPECT_FALSE(grid.isValidGridPosition(Vector3i(0, 0, -1)));
    EXPECT_FALSE(grid.isValidGridPosition(Vector3i(maxDims.x, maxDims.y, maxDims.z)));
    
    // World position validation with centered coordinate system
    // For 5m workspace: X,Z range from -2.5 to +2.5, Y ranges from 0 to 5
    EXPECT_TRUE(grid.isValidWorldPosition(Vector3f(0.0f, 0.0f, 0.0f))); // Center of workspace
    EXPECT_TRUE(grid.isValidWorldPosition(Vector3f(2.0f, 2.0f, 2.0f))); // Within bounds
    EXPECT_TRUE(grid.isValidWorldPosition(Vector3f(2.5f, 5.0f, 2.5f))); // At max bounds
    EXPECT_TRUE(grid.isValidWorldPosition(Vector3f(-2.5f, 0.0f, -2.5f))); // At min bounds
    
    // Outside workspace bounds
    EXPECT_FALSE(grid.isValidWorldPosition(Vector3f(-2.6f, 0.0f, 0.0f))); // Beyond min X
    EXPECT_FALSE(grid.isValidWorldPosition(Vector3f(2.6f, 0.0f, 0.0f))); // Beyond max X
    EXPECT_FALSE(grid.isValidWorldPosition(Vector3f(0.0f, -0.1f, 0.0f))); // Below ground
    EXPECT_FALSE(grid.isValidWorldPosition(Vector3f(0.0f, 5.1f, 0.0f))); // Beyond max Y
    EXPECT_FALSE(grid.isValidWorldPosition(Vector3f(0.0f, 0.0f, 2.6f))); // Beyond max Z
}

TEST_F(VoxelGridTest, OutOfBoundsOperations) {
    VoxelGrid grid(resolution, workspaceSize);
    Vector3i maxDims = grid.getGridDimensions();
    
    // Try to set voxels outside bounds
    EXPECT_FALSE(grid.setVoxel(Vector3i(-1, 0, 0), true));
    EXPECT_FALSE(grid.setVoxel(Vector3i(maxDims.x, 0, 0), true));
    EXPECT_FALSE(grid.setVoxel(Vector3i(0, maxDims.y, 0), true));
    EXPECT_FALSE(grid.setVoxel(Vector3i(0, 0, maxDims.z), true));
    
    // Grid should remain empty
    EXPECT_EQ(grid.getVoxelCount(), 0);
    
    // Reading out of bounds should return false
    EXPECT_FALSE(grid.getVoxel(Vector3i(-1, 0, 0)));
    EXPECT_FALSE(grid.getVoxel(Vector3i(maxDims.x, 0, 0)));
}

TEST_F(VoxelGridTest, WorkspaceResizing) {
    VoxelGrid grid(resolution, workspaceSize);
    
    // Add some voxels
    Vector3i centerPos = grid.getGridDimensions() / 2;
    EXPECT_TRUE(grid.setVoxel(centerPos, true));
    EXPECT_EQ(grid.getVoxelCount(), 1);
    
    // Resize to larger workspace
    Vector3f newSize(8.0f, 8.0f, 8.0f);
    EXPECT_TRUE(grid.resizeWorkspace(newSize));
    EXPECT_EQ(grid.getWorkspaceSize(), newSize);
    
    // Voxel should still exist
    EXPECT_EQ(grid.getVoxelCount(), 1);
    EXPECT_TRUE(grid.getVoxel(centerPos));
    
    // Try to resize to smaller workspace (might lose voxels)
    Vector3f smallerSize(2.0f, 2.0f, 2.0f);
    bool resizeResult = grid.resizeWorkspace(smallerSize);
    
    if (resizeResult) {
        EXPECT_EQ(grid.getWorkspaceSize(), smallerSize);
        // Check if voxel was preserved or lost based on position
    } else {
        // Resize failed to preserve voxels - workspace should remain unchanged
        EXPECT_EQ(grid.getWorkspaceSize(), newSize);
        EXPECT_EQ(grid.getVoxelCount(), 1);
    }
}

TEST_F(VoxelGridTest, ClearOperation) {
    VoxelGrid grid(resolution, workspaceSize);
    
    // Add multiple voxels
    std::vector<Vector3i> positions = {
        Vector3i(10, 10, 10),
        Vector3i(20, 20, 20),
        Vector3i(30, 30, 30),
        Vector3i(100, 100, 100)
    };
    
    for (const auto& pos : positions) {
        EXPECT_TRUE(grid.setVoxel(pos, true));
    }
    
    EXPECT_EQ(grid.getVoxelCount(), positions.size());
    size_t memoryWithVoxels = grid.getMemoryUsage();
    
    // Clear all voxels
    grid.clear();
    
    EXPECT_EQ(grid.getVoxelCount(), 0);
    EXPECT_LT(grid.getMemoryUsage(), memoryWithVoxels);
    
    // Verify all voxels are gone
    for (const auto& pos : positions) {
        EXPECT_FALSE(grid.getVoxel(pos));
    }
}

TEST_F(VoxelGridTest, MemoryOptimization) {
    VoxelGrid grid(resolution, workspaceSize);
    
    // Add many voxels in a pattern
    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10; ++y) {
            for (int z = 0; z < 10; ++z) {
                grid.setVoxel(Vector3i(x, y, z), true);
            }
        }
    }
    
    size_t voxelCount = grid.getVoxelCount();
    size_t memoryBeforeOptimize = grid.getMemoryUsage();
    
    // Optimize memory
    grid.optimizeMemory();
    
    // Voxel count should be preserved
    EXPECT_EQ(grid.getVoxelCount(), voxelCount);
    
    // All voxels should still be accessible
    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10; ++y) {
            for (int z = 0; z < 10; ++z) {
                EXPECT_TRUE(grid.getVoxel(Vector3i(x, y, z)));
            }
        }
    }
    
    // Memory usage might change
    EXPECT_GT(grid.getMemoryUsage(), 0);
}

TEST_F(VoxelGridTest, VoxelExport) {
    VoxelGrid grid(resolution, workspaceSize);
    
    std::vector<Vector3i> expectedPositions = {
        Vector3i(5, 10, 15),
        Vector3i(25, 30, 35),
        Vector3i(50, 60, 70)
    };
    
    // Set voxels
    for (const auto& pos : expectedPositions) {
        EXPECT_TRUE(grid.setVoxel(pos, true));
    }
    
    // Export all voxels
    std::vector<VoxelPosition> exportedVoxels = grid.getAllVoxels();
    
    EXPECT_EQ(exportedVoxels.size(), expectedPositions.size());
    
    // Verify exported voxels
    for (const auto& voxelPos : exportedVoxels) {
        EXPECT_EQ(voxelPos.resolution, resolution);
        
        bool found = false;
        for (const auto& expectedPos : expectedPositions) {
            if (voxelPos.gridPos == expectedPos) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }
}

TEST_F(VoxelGridTest, DifferentResolutionBehavior) {
    // Test different resolutions have different grid dimensions
    Vector3f testWorkspace(4.0f, 4.0f, 4.0f);
    
    VoxelGrid grid1cm(VoxelResolution::Size_1cm, testWorkspace);
    VoxelGrid grid4cm(VoxelResolution::Size_4cm, testWorkspace);
    VoxelGrid grid16cm(VoxelResolution::Size_16cm, testWorkspace);
    
    Vector3i dims1cm = grid1cm.getGridDimensions();
    Vector3i dims4cm = grid4cm.getGridDimensions();
    Vector3i dims16cm = grid16cm.getGridDimensions();
    
    // Higher resolution = more grid cells
    EXPECT_GT(dims1cm.x, dims4cm.x);
    EXPECT_GT(dims4cm.x, dims16cm.x);
    
    // Same world position should map to different grid positions
    Vector3f worldPos(1.0f, 1.0f, 1.0f);
    
    Vector3i gridPos1cm = grid1cm.worldToGrid(worldPos);
    Vector3i gridPos4cm = grid4cm.worldToGrid(worldPos);
    Vector3i gridPos16cm = grid16cm.worldToGrid(worldPos);
    
    EXPECT_NE(gridPos1cm, gridPos4cm);
    EXPECT_NE(gridPos4cm, gridPos16cm);
}

TEST_F(VoxelGridTest, StressTestLargeGrid) {
    // Use larger voxels for stress test to reduce memory usage
    VoxelGrid grid(VoxelResolution::Size_4cm, Vector3f(8.0f, 8.0f, 8.0f));
    
    Vector3i maxDims = grid.getGridDimensions();
    size_t expectedVoxels = 0;
    
    // Fill every 4th voxel in each dimension
    for (int x = 0; x < maxDims.x; x += 4) {
        for (int y = 0; y < maxDims.y; y += 4) {
            for (int z = 0; z < maxDims.z; z += 4) {
                if (grid.setVoxel(Vector3i(x, y, z), true)) {
                    expectedVoxels++;
                }
            }
        }
    }
    
    EXPECT_EQ(grid.getVoxelCount(), expectedVoxels);
    EXPECT_GT(expectedVoxels, 0);
    
    // Verify the voxels are correctly set
    for (int x = 0; x < maxDims.x; x += 4) {
        for (int y = 0; y < maxDims.y; y += 4) {
            for (int z = 0; z < maxDims.z; z += 4) {
                EXPECT_TRUE(grid.getVoxel(Vector3i(x, y, z)));
            }
        }
    }
}

TEST_F(VoxelGridTest, MemoryUsageScaling) {
    VoxelGrid grid(resolution, workspaceSize);
    
    size_t baseMemory = grid.getMemoryUsage();
    
    // Add voxels and track memory growth
    std::vector<size_t> memoryCheckpoints;
    
    for (int i = 1; i <= 10; ++i) {
        grid.setVoxel(Vector3i(i * 10, i * 10, i * 10), true);
        memoryCheckpoints.push_back(grid.getMemoryUsage());
    }
    
    // Memory should generally increase (though octree might have steps)
    EXPECT_GE(memoryCheckpoints.back(), baseMemory);
    
    // Clear and verify memory decreases
    grid.clear();
    EXPECT_LT(grid.getMemoryUsage(), memoryCheckpoints.back());
}

TEST_F(VoxelGridTest, ThreadSafetyPreparation) {
    // This test verifies the grid works correctly when accessed from different contexts
    // (preparing for multi-threaded usage in VoxelDataManager)
    
    VoxelGrid grid(resolution, workspaceSize);
    
    // Simulate rapid operations that might occur in multi-threaded context
    std::vector<Vector3i> positions;
    for (int i = 0; i < 100; ++i) {
        positions.push_back(Vector3i(i, i % 10, (i * 2) % 20));
    }
    
    // Set all voxels
    for (const auto& pos : positions) {
        EXPECT_TRUE(grid.setVoxel(pos, true));
    }
    
    EXPECT_EQ(grid.getVoxelCount(), positions.size());
    
    // Verify all voxels
    for (const auto& pos : positions) {
        EXPECT_TRUE(grid.getVoxel(pos));
    }
    
    // Clear half the voxels
    for (size_t i = 0; i < positions.size() / 2; ++i) {
        EXPECT_TRUE(grid.setVoxel(positions[i], false));
    }
    
    EXPECT_EQ(grid.getVoxelCount(), positions.size() - positions.size() / 2);
    
    // Verify correct voxels remain
    for (size_t i = 0; i < positions.size(); ++i) {
        bool shouldExist = i >= positions.size() / 2;
        EXPECT_EQ(grid.getVoxel(positions[i]), shouldExist);
    }
}

TEST_F(VoxelGridTest, VoxelWorldPositionVerification) {
    // Test that voxel world positions match expected coordinates
    // This is critical for rendering - ensures voxels appear where we expect them
    
    // Test with 8cm resolution as used in CLI
    VoxelGrid grid(VoxelResolution::Size_8cm, workspaceSize);
    float voxelSize = getVoxelSize(VoxelResolution::Size_8cm);
    EXPECT_FLOAT_EQ(voxelSize, 0.08f);
    
    // Test cases with expected world positions
    struct TestCase {
        Vector3i gridPos;
        Vector3f expectedWorldPos;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        // Grid coordinates with centered coordinate system
        // For 5m workspace: grid (0,0,0) maps to world (-2.5, 0, -2.5)
        {Vector3i(0, 0, 0), Vector3f(-2.5f, 0.0f, -2.5f), "Grid origin (corner)"},
        {Vector3i(1, 0, 0), Vector3f(-2.42f, 0.0f, -2.5f), "One voxel along X"},
        {Vector3i(0, 1, 0), Vector3f(-2.5f, 0.08f, -2.5f), "One voxel along Y"},
        {Vector3i(0, 0, 1), Vector3f(-2.5f, 0.0f, -2.42f), "One voxel along Z"},
        {Vector3i(10, 10, 10), Vector3f(-1.7f, 0.8f, -1.7f), "10 voxels in each direction"},
        {Vector3i(25, 25, 25), Vector3f(-0.5f, 2.0f, -0.5f), "25 voxels = 2m"},
        {Vector3i(31, 31, 31), Vector3f(-0.02f, 2.48f, -0.02f), "Near center of 5m workspace"}
    };
    
    for (const auto& testCase : testCases) {
        // Set voxel at grid position
        EXPECT_TRUE(grid.setVoxel(testCase.gridPos, true)) 
            << "Failed to set voxel at " << testCase.description;
        
        // Get world position from grid
        Vector3f actualWorldPos = grid.gridToWorld(testCase.gridPos);
        
        // Verify world position matches expected (with small tolerance for floating point)
        EXPECT_NEAR(actualWorldPos.x, testCase.expectedWorldPos.x, 0.0001f) 
            << testCase.description << " - X mismatch";
        EXPECT_NEAR(actualWorldPos.y, testCase.expectedWorldPos.y, 0.0001f) 
            << testCase.description << " - Y mismatch";
        EXPECT_NEAR(actualWorldPos.z, testCase.expectedWorldPos.z, 0.0001f) 
            << testCase.description << " - Z mismatch";
        
        // Verify we can retrieve the voxel using world position
        EXPECT_TRUE(grid.getVoxelAtWorldPos(testCase.expectedWorldPos))
            << testCase.description << " - Can't retrieve voxel at world pos";
        
        // Verify round-trip conversion (allowing for 1 cell difference due to floating point)
        Vector3i roundTripGridPos = grid.worldToGrid(actualWorldPos);
        int maxDiff = std::max({
            std::abs(roundTripGridPos.x - testCase.gridPos.x),
            std::abs(roundTripGridPos.y - testCase.gridPos.y),
            std::abs(roundTripGridPos.z - testCase.gridPos.z)
        });
        EXPECT_LE(maxDiff, 1)
            << testCase.description << " - Round-trip conversion off by more than 1 cell";
    }
    
    // Verify all voxels are at expected positions
    std::vector<VoxelPosition> allVoxels = grid.getAllVoxels();
    EXPECT_EQ(allVoxels.size(), testCases.size());
    
    // Log actual voxel positions for debugging
    for (const auto& voxelPos : allVoxels) {
        Vector3f worldPos = grid.gridToWorld(voxelPos.gridPos);
        // This would be where we'd add logging in the actual implementation
        // For now, just verify the position is valid
        EXPECT_TRUE(grid.isValidWorldPosition(worldPos));
    }
}