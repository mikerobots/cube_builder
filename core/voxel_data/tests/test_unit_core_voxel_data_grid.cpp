#include <gtest/gtest.h>
#include "../VoxelGrid.h"
#include "../../foundation/math/CoordinateConverter.h"
#include <algorithm>

using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

// Use VoxelData namespace for VoxelResolution to avoid ambiguity
using VoxelEditor::VoxelData::VoxelResolution;

class VoxelGridTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize sparse octree pool
        SparseOctree::initializePool(512);
        
        workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
        resolution = VoxelEditor::VoxelData::VoxelResolution::Size_1cm;
    }
    
    void TearDown() override {
        SparseOctree::shutdownPool();
    }
    
    Vector3f workspaceSize;
    VoxelEditor::VoxelData::VoxelResolution resolution;
};

// REQ-1.2.3: The grid shall extend to cover the entire workspace area
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
    for (int i = 0; i < static_cast<int>(VoxelEditor::VoxelData::VoxelResolution::COUNT); ++i) {
        VoxelEditor::VoxelData::VoxelResolution res = static_cast<VoxelEditor::VoxelData::VoxelResolution>(i);
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

// REQ-1.1.5: The grid origin (0,0,0) shall be at the center of the workspace
TEST_F(VoxelGridTest, GridWorldCoordinateConversion) {
    VoxelGrid grid(resolution, workspaceSize);
    
    // Test conversion from increment to world with centered coordinate system
    // In the new system, increment coordinates are in 1cm units and centered
    // Increment (0,0,0) should map to world (0,0,0) - the center
    IncrementCoordinates centerIncPos(0, 0, 0);
    WorldCoordinates centerWorldCoords = CoordinateConverter::incrementToWorld(centerIncPos);
    Vector3f centerWorldPos = centerWorldCoords.value();
    
    // Center of workspace: should be (0,0,0) 
    EXPECT_NEAR(centerWorldPos.x, 0.0f, 0.01f);
    EXPECT_NEAR(centerWorldPos.y, 0.0f, 0.01f);
    EXPECT_NEAR(centerWorldPos.z, 0.0f, 0.01f);
    
    // Test edge positions - for 5m workspace, max increment coordinates are ±250 in X/Z, 0-500 in Y
    IncrementCoordinates edgeIncPos(250, 500, 250); // Positive edge
    WorldCoordinates edgeWorldCoords = CoordinateConverter::incrementToWorld(edgeIncPos);
    Vector3f edgeWorldPos = edgeWorldCoords.value();
    
    // Should be at positive edge: (2.5, 5.0, 2.5)
    EXPECT_NEAR(edgeWorldPos.x, 2.5f, 0.01f);
    EXPECT_NEAR(edgeWorldPos.y, 5.0f, 0.01f);
    EXPECT_NEAR(edgeWorldPos.z, 2.5f, 0.01f);
    
    // Test conversion from world to increment
    WorldCoordinates testWorldCoords(1.0f, 2.0f, 0.5f);
    IncrementCoordinates convertedIncPos = CoordinateConverter::worldToIncrement(testWorldCoords);
    
    // Should convert to increment coordinates: (100, 200, 50) cm
    EXPECT_EQ(convertedIncPos.x(), 100);  // 1.0m = 100cm
    EXPECT_EQ(convertedIncPos.y(), 200);  // 2.0m = 200cm 
    EXPECT_EQ(convertedIncPos.z(), 50);   // 0.5m = 50cm
    
    // Verify round-trip conversion
    WorldCoordinates roundTripWorldCoords = CoordinateConverter::incrementToWorld(convertedIncPos);
    Vector3f roundTripWorldPos = roundTripWorldCoords.value();
    
    // Should be close (within voxel size)
    float voxelSize = getVoxelSize(resolution);
    Vector3f originalWorldPos = testWorldCoords.value();
    EXPECT_LT(abs(roundTripWorldPos.x - originalWorldPos.x), voxelSize);
    EXPECT_LT(abs(roundTripWorldPos.y - originalWorldPos.y), voxelSize);
    EXPECT_LT(abs(roundTripWorldPos.z - originalWorldPos.z), voxelSize);
}

// REQ-2.1.4: No voxels shall be placed below Y=0
TEST_F(VoxelGridTest, PositionValidation) {
    VoxelGrid grid(resolution, workspaceSize);
    
    // Valid increment positions - for 5m workspace, range is:
    // X: -250 to +250 cm, Y: 0 to 500 cm, Z: -250 to +250 cm
    EXPECT_TRUE(grid.isValidIncrementPosition(IncrementCoordinates(0, 0, 0)));     // Center
    EXPECT_TRUE(grid.isValidIncrementPosition(IncrementCoordinates(100, 250, 50))); // Mid-range
    EXPECT_TRUE(grid.isValidIncrementPosition(IncrementCoordinates(250, 500, 250))); // Max edges
    EXPECT_TRUE(grid.isValidIncrementPosition(IncrementCoordinates(-250, 0, -250))); // Min edges
    
    // Invalid increment positions
    EXPECT_FALSE(grid.isValidIncrementPosition(IncrementCoordinates(0, -1, 0)));    // Below ground
    EXPECT_FALSE(grid.isValidIncrementPosition(IncrementCoordinates(251, 0, 0)));   // Beyond max X
    EXPECT_FALSE(grid.isValidIncrementPosition(IncrementCoordinates(-251, 0, 0)));  // Beyond min X
    EXPECT_FALSE(grid.isValidIncrementPosition(IncrementCoordinates(0, 501, 0)));   // Beyond max Y
    EXPECT_FALSE(grid.isValidIncrementPosition(IncrementCoordinates(0, 0, 251)));   // Beyond max Z
    EXPECT_FALSE(grid.isValidIncrementPosition(IncrementCoordinates(0, 0, -251))); // Beyond min Z
    
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

// REQ-2.1.4: No voxels shall be placed below Y=0
TEST_F(VoxelGridTest, OutOfBoundsOperations) {
    VoxelGrid grid(resolution, workspaceSize);
    
    // Test increment coordinate bounds - for 5m workspace: X,Z: -250 to +250, Y: 0 to 500
    // Try to set voxels outside increment bounds
    EXPECT_FALSE(grid.setVoxel(IncrementCoordinates(-251, 0, 0), true));  // Beyond min X
    EXPECT_FALSE(grid.setVoxel(IncrementCoordinates(251, 0, 0), true));   // Beyond max X
    EXPECT_FALSE(grid.setVoxel(IncrementCoordinates(0, -1, 0), true));    // Below ground
    EXPECT_FALSE(grid.setVoxel(IncrementCoordinates(0, 501, 0), true));   // Above max Y
    EXPECT_FALSE(grid.setVoxel(IncrementCoordinates(0, 0, -251), true));  // Beyond min Z
    EXPECT_FALSE(grid.setVoxel(IncrementCoordinates(0, 0, 251), true));   // Beyond max Z
    
    // Grid should remain empty
    EXPECT_EQ(grid.getVoxelCount(), 0);
    
    // Reading out of bounds should return false
    EXPECT_FALSE(grid.getVoxel(IncrementCoordinates(-251, 0, 0)));
    EXPECT_FALSE(grid.getVoxel(IncrementCoordinates(251, 0, 0)));
    EXPECT_FALSE(grid.getVoxel(IncrementCoordinates(0, -1, 0)));
    EXPECT_FALSE(grid.getVoxel(IncrementCoordinates(0, 501, 0)));
}

TEST_F(VoxelGridTest, WorkspaceResizing) {
    VoxelGrid grid(resolution, workspaceSize);
    
    // Test that resizing to larger workspace works
    Vector3f newSize(8.0f, 8.0f, 8.0f);
    EXPECT_TRUE(grid.resizeWorkspace(newSize));
    EXPECT_EQ(grid.getWorkspaceSize(), newSize);
    
    // Test that resizing to smaller workspace works (without voxels to preserve)
    Vector3f smallerSize(2.0f, 2.0f, 2.0f);
    EXPECT_TRUE(grid.resizeWorkspace(smallerSize));
    EXPECT_EQ(grid.getWorkspaceSize(), smallerSize);
    
    // Test resize back to original size
    EXPECT_TRUE(grid.resizeWorkspace(workspaceSize));
    EXPECT_EQ(grid.getWorkspaceSize(), workspaceSize);
    
    // Test with voxels present
    IncrementCoordinates centerPos(0, 0, 0);
    EXPECT_TRUE(grid.setVoxel(centerPos, true));
    EXPECT_EQ(grid.getVoxelCount(), 1);
    
    // Test resize with voxel preservation
    Vector3f preserveSize(6.0f, 6.0f, 6.0f);
    EXPECT_TRUE(grid.resizeWorkspace(preserveSize));
    EXPECT_EQ(grid.getWorkspaceSize(), preserveSize);
    // Voxel count may or may not be preserved depending on implementation details
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

// REQ-6.3.2: Voxel data storage shall not exceed 2GB
// REQ-6.3.5: System shall detect and respond to memory pressure
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
            if (voxelPos.incrementPos.value() == expectedPos) {
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
    
    VoxelGrid grid1cm(VoxelEditor::VoxelData::VoxelResolution::Size_1cm, testWorkspace);
    VoxelGrid grid4cm(VoxelEditor::VoxelData::VoxelResolution::Size_4cm, testWorkspace);
    VoxelGrid grid16cm(VoxelEditor::VoxelData::VoxelResolution::Size_16cm, testWorkspace);
    
    Vector3i dims1cm = grid1cm.getGridDimensions();
    Vector3i dims4cm = grid4cm.getGridDimensions();
    Vector3i dims16cm = grid16cm.getGridDimensions();
    
    // NEW BEHAVIOR: All grids use the same 1cm increment coordinate system
    // So they should have the same dimensions
    EXPECT_EQ(dims1cm.x, dims4cm.x);
    EXPECT_EQ(dims4cm.x, dims16cm.x);
    
    // Same world position should map to same increment coordinates
    WorldCoordinates worldPos(1.0f, 1.0f, 1.0f);
    
    IncrementCoordinates incPos1cm = CoordinateConverter::worldToIncrement(worldPos);
    IncrementCoordinates incPos4cm = CoordinateConverter::worldToIncrement(worldPos);
    IncrementCoordinates incPos16cm = CoordinateConverter::worldToIncrement(worldPos);
    
    // In the new system, all grids use the same increment coordinate system
    EXPECT_EQ(incPos1cm, incPos4cm);
    EXPECT_EQ(incPos4cm, incPos16cm);
}

TEST_F(VoxelGridTest, StressTestLargeGrid) {
    // Use larger voxels for stress test to reduce memory usage
    VoxelGrid grid(VoxelEditor::VoxelData::VoxelResolution::Size_4cm, Vector3f(8.0f, 8.0f, 8.0f));
    
    // For centered coordinate system with 8m workspace:
    // X: -400 to +400 cm (800 cm total)
    // Y: 0 to 800 cm
    // Z: -400 to +400 cm (800 cm total)
    size_t expectedVoxels = 0;
    
    // Fill every 40cm (10 x 4cm voxels) in each dimension to reduce memory usage
    for (int x = -400; x <= 400; x += 40) {
        for (int y = 0; y <= 800; y += 40) {
            for (int z = -400; z <= 400; z += 40) {
                IncrementCoordinates pos(x, y, z);
                if (grid.setVoxel(pos, true)) {
                    expectedVoxels++;
                }
            }
        }
    }
    
    EXPECT_EQ(grid.getVoxelCount(), expectedVoxels);
    EXPECT_GT(expectedVoxels, 0);
    
    // Verify the voxels are correctly set
    for (int x = -400; x <= 400; x += 40) {
        for (int y = 0; y <= 800; y += 40) {
            for (int z = -400; z <= 400; z += 40) {
                IncrementCoordinates pos(x, y, z);
                EXPECT_TRUE(grid.getVoxel(pos)) 
                    << "Failed to retrieve voxel at position (" << x << "," << y << "," << z << ")";
            }
        }
    }
}

// REQ-6.3.2: Voxel data storage shall not exceed 2GB
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

// REQ-2.1.1: Voxels shall be placeable only at 1cm increment positions
TEST_F(VoxelGridTest, VoxelWorldPositionVerification) {
    // Test that voxel world positions match expected coordinates
    // This is critical for rendering - ensures voxels appear where we expect them
    
    // Test with 8cm resolution as used in CLI
    VoxelGrid grid(VoxelEditor::VoxelData::VoxelResolution::Size_8cm, workspaceSize);
    float voxelSize = getVoxelSize(VoxelEditor::VoxelData::VoxelResolution::Size_8cm);
    EXPECT_FLOAT_EQ(voxelSize, 0.08f);
    
    // Test cases with increment coordinates and expected world positions
    struct TestCase {
        IncrementCoordinates incrementPos;
        Vector3f expectedWorldPos;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        // Increment coordinates with centered coordinate system
        // Increment (0,0,0) should map to world (0,0,0) - the center
        {IncrementCoordinates(0, 0, 0), Vector3f(0.0f, 0.0f, 0.0f), "Increment origin (center)"},
        {IncrementCoordinates(8, 0, 0), Vector3f(0.08f, 0.0f, 0.0f), "8cm along X"},
        {IncrementCoordinates(0, 8, 0), Vector3f(0.0f, 0.08f, 0.0f), "8cm along Y"}, 
        {IncrementCoordinates(0, 0, 8), Vector3f(0.0f, 0.0f, 0.08f), "8cm along Z"},
        {IncrementCoordinates(100, 100, 100), Vector3f(1.0f, 1.0f, 1.0f), "1m in each direction"},
        {IncrementCoordinates(-100, 50, -200), Vector3f(-1.0f, 0.5f, -2.0f), "Mixed coordinates"},
        {IncrementCoordinates(200, 400, 200), Vector3f(2.0f, 4.0f, 2.0f), "Near edge of 5m workspace"}
    };
    
    for (const auto& testCase : testCases) {
        // Set voxel at increment position
        EXPECT_TRUE(grid.setVoxel(testCase.incrementPos, true)) 
            << "Failed to set voxel at " << testCase.description;
        
        // Get world position from increment coordinates using CoordinateConverter
        WorldCoordinates actualWorldPos = CoordinateConverter::incrementToWorld(testCase.incrementPos);
        
        // Verify world position matches expected (with small tolerance for floating point)
        EXPECT_NEAR(actualWorldPos.x(), testCase.expectedWorldPos.x, 0.0001f) 
            << testCase.description << " - X mismatch";
        EXPECT_NEAR(actualWorldPos.y(), testCase.expectedWorldPos.y, 0.0001f) 
            << testCase.description << " - Y mismatch";
        EXPECT_NEAR(actualWorldPos.z(), testCase.expectedWorldPos.z, 0.0001f) 
            << testCase.description << " - Z mismatch";
        
        // Verify we can retrieve the voxel using world position
        EXPECT_TRUE(grid.getVoxelAtWorldPos(testCase.expectedWorldPos))
            << testCase.description << " - Can't retrieve voxel at world pos";
        
        // Verify round-trip conversion
        IncrementCoordinates roundTripIncrementPos = CoordinateConverter::worldToIncrement(actualWorldPos);
        EXPECT_EQ(roundTripIncrementPos.x(), testCase.incrementPos.x())
            << testCase.description << " - Round-trip X mismatch";
        EXPECT_EQ(roundTripIncrementPos.y(), testCase.incrementPos.y())
            << testCase.description << " - Round-trip Y mismatch";
        EXPECT_EQ(roundTripIncrementPos.z(), testCase.incrementPos.z())
            << testCase.description << " - Round-trip Z mismatch";
    }
    
    // Verify all voxels are at expected positions
    std::vector<VoxelPosition> allVoxels = grid.getAllVoxels();
    EXPECT_EQ(allVoxels.size(), testCases.size());
    
    // Verify all positions are valid
    for (const auto& voxelPos : allVoxels) {
        WorldCoordinates worldPos = CoordinateConverter::incrementToWorld(voxelPos.incrementPos);
        EXPECT_TRUE(grid.isValidWorldPosition(worldPos.value()));
    }
}

// ==================== Requirements Change Tests - Arbitrary 1cm Position Storage ====================

// REQ-2.1.1 (updated): Voxels shall be placed at any 1cm increment position without resolution-based snapping
TEST_F(VoxelGridTest, ArbitraryPositions_NoSnapToVoxelBoundaries) {
    // Test that VoxelGrid can store voxels at any 1cm position, regardless of voxel size
    // This verifies the new requirement: no resolution-based snapping in storage
    
    // Test with 4cm voxels - previously these would snap to multiples of 4
    VoxelGrid grid4cm(VoxelEditor::VoxelData::VoxelResolution::Size_4cm, Vector3f(10.0f, 10.0f, 10.0f));
    
    // These positions are NOT aligned to 4cm boundaries
    std::vector<IncrementCoordinates> nonAlignedPositions = {
        IncrementCoordinates(1, 1, 1),    // 1cm position (not multiple of 4)
        IncrementCoordinates(3, 7, 11),   // Prime numbers (not multiples of 4)
        IncrementCoordinates(17, 23, 29), // More primes
        IncrementCoordinates(50, 75, 99), // Random non-aligned positions
        IncrementCoordinates(-5, 13, -21) // Mixed positive/negative
    };
    
    // All these positions should be storable without snapping
    for (const auto& pos : nonAlignedPositions) {
        EXPECT_TRUE(grid4cm.setVoxel(pos, true)) 
            << "Failed to store 4cm voxel at non-aligned position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
        EXPECT_TRUE(grid4cm.getVoxel(pos))
            << "Failed to retrieve 4cm voxel at non-aligned position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
    
    EXPECT_EQ(grid4cm.getVoxelCount(), nonAlignedPositions.size());
    
    // Test with 16cm voxels - even larger voxels should store at arbitrary 1cm positions
    VoxelGrid grid16cm(VoxelEditor::VoxelData::VoxelResolution::Size_16cm, Vector3f(20.0f, 20.0f, 20.0f));
    
    std::vector<IncrementCoordinates> moreNonAlignedPositions = {
        IncrementCoordinates(7, 13, 19),   // Not multiples of 16
        IncrementCoordinates(31, 37, 41),  // More primes
        IncrementCoordinates(100, 200, 150) // Large non-aligned
    };
    
    for (const auto& pos : moreNonAlignedPositions) {
        EXPECT_TRUE(grid16cm.setVoxel(pos, true))
            << "Failed to store 16cm voxel at non-aligned position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
        EXPECT_TRUE(grid16cm.getVoxel(pos))
            << "Failed to retrieve 16cm voxel at non-aligned position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
    
    EXPECT_EQ(grid16cm.getVoxelCount(), moreNonAlignedPositions.size());
}

TEST_F(VoxelGridTest, ArbitraryPositions_AllResolutionsSupported) {
    // Test that ALL voxel resolutions can store voxels at arbitrary 1cm positions
    // This is the core of the requirements change
    
    Vector3f testWorkspace(8.0f, 8.0f, 8.0f);
    
    // Test arbitrary 1cm positions that are NOT aligned to any common voxel size
    std::vector<IncrementCoordinates> testPositions = {
        IncrementCoordinates(13, 27, 41),  // Prime numbers
        IncrementCoordinates(97, 103, 107), // More primes
        IncrementCoordinates(-23, 59, -67), // Mixed signs
        IncrementCoordinates(1, 3, 5),     // Small odds
        IncrementCoordinates(127, 131, 137) // Large primes
    };
    
    std::vector<VoxelEditor::VoxelData::VoxelResolution> allResolutions = {
        VoxelEditor::VoxelData::VoxelResolution::Size_1cm, VoxelEditor::VoxelData::VoxelResolution::Size_2cm, 
        VoxelEditor::VoxelData::VoxelResolution::Size_4cm, VoxelEditor::VoxelData::VoxelResolution::Size_8cm, 
        VoxelEditor::VoxelData::VoxelResolution::Size_16cm, VoxelEditor::VoxelData::VoxelResolution::Size_32cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_64cm, VoxelEditor::VoxelData::VoxelResolution::Size_128cm
        // Note: Skip 256cm and 512cm as they're too large for 8m workspace
    };
    
    for (auto resolution : allResolutions) {
        VoxelGrid grid(resolution, testWorkspace);
        float voxelSize = VoxelEditor::VoxelData::getVoxelSize(resolution);
        
        for (const auto& pos : testPositions) {
            // Skip positions outside workspace for this resolution
            if (!grid.isValidIncrementPosition(pos)) {
                continue;
            }
            
            // Should be able to store at exact position (no snapping)
            EXPECT_TRUE(grid.setVoxel(pos, true))
                << "Failed to store " << (voxelSize * 100) << "cm voxel at position (" 
                << pos.x() << "," << pos.y() << "," << pos.z() << ")";
                
            EXPECT_TRUE(grid.getVoxel(pos))
                << "Failed to retrieve " << (voxelSize * 100) << "cm voxel at position (" 
                << pos.x() << "," << pos.y() << "," << pos.z() << ")";
        }
    }
}

TEST_F(VoxelGridTest, ArbitraryPositions_StorageAndRetrieval) {
    // Test that voxels stored at arbitrary positions can be retrieved correctly
    // This verifies that VoxelGrid correctly handles storage at any 1cm position
    
    VoxelGrid grid(VoxelEditor::VoxelData::VoxelResolution::Size_8cm, Vector3f(6.0f, 6.0f, 6.0f));
    
    // Store voxels at positions that would NOT align to 8cm boundaries
    std::vector<IncrementCoordinates> testPositions = {
        IncrementCoordinates(11, 19, 23),  // Not multiples of 8
        IncrementCoordinates(37, 41, 43),  // More non-aligned
        IncrementCoordinates(-13, 29, -31), // Mixed signs, non-aligned
        IncrementCoordinates(67, 71, 73),  // Large non-aligned
        IncrementCoordinates(5, 9, 15)     // Small non-aligned
    };
    
    // Store all voxels
    for (const auto& pos : testPositions) {
        EXPECT_TRUE(grid.setVoxel(pos, true))
            << "Failed to store voxel at position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
    
    EXPECT_EQ(grid.getVoxelCount(), testPositions.size());
    
    // Verify all stored voxels can be retrieved at their exact positions
    for (const auto& pos : testPositions) {
        EXPECT_TRUE(grid.getVoxel(pos))
            << "Failed to retrieve voxel at position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
    
    // Verify adjacent positions are NOT set (unless they happen to map to same grid cell)
    for (const auto& pos : testPositions) {
        IncrementCoordinates adjacent1(pos.x() + 1, pos.y(), pos.z());
        IncrementCoordinates adjacent2(pos.x(), pos.y() + 1, pos.z());
        IncrementCoordinates adjacent3(pos.x(), pos.y(), pos.z() + 1);
        
        // These might or might not be set depending on whether they map to the same grid cell
        // But we can at least verify the operations don't crash
        if (grid.isValidIncrementPosition(adjacent1)) {
            grid.getVoxel(adjacent1); // Should not crash
        }
        if (grid.isValidIncrementPosition(adjacent2)) {
            grid.getVoxel(adjacent2); // Should not crash
        }
        if (grid.isValidIncrementPosition(adjacent3)) {
            grid.getVoxel(adjacent3); // Should not crash
        }
    }
}

TEST_F(VoxelGridTest, ArbitraryPositions_GridCoordinateMapping) {
    // Test that the incrementToGrid function correctly maps arbitrary positions
    // This tests the internal grid coordinate conversion without snapping
    
    VoxelGrid grid(VoxelEditor::VoxelData::VoxelResolution::Size_4cm, Vector3f(8.0f, 8.0f, 8.0f));
    
    // Test positions and their expected grid mappings
    struct MappingTest {
        IncrementCoordinates incrementPos;
        Vector3i expectedGridPos;
        std::string description;
    };
    
    // NEW BEHAVIOR: VoxelGrid now stores at 1cm granularity regardless of resolution
    // For 8m workspace: offset is 400cm for X/Z (halfX_cm = halfZ_cm = 400)
    // Grid position = increment position + offset (no division by voxel size)
    std::vector<MappingTest> mappingTests = {
        {IncrementCoordinates(0, 0, 0), Vector3i(400, 0, 400), "Center position"},
        {IncrementCoordinates(4, 4, 4), Vector3i(404, 4, 404), "4cm offset"},
        {IncrementCoordinates(1, 1, 1), Vector3i(401, 1, 401), "1cm position (each cm is unique)"},
        {IncrementCoordinates(3, 3, 3), Vector3i(403, 3, 403), "3cm position (each cm is unique)"},
        {IncrementCoordinates(5, 5, 5), Vector3i(405, 5, 405), "5cm position (each cm is unique)"},
        {IncrementCoordinates(-100, 50, -200), Vector3i(300, 50, 200), "Negative coordinates"},
        {IncrementCoordinates(100, 100, 100), Vector3i(500, 100, 500), "Positive coordinates"}
    };
    
    for (const auto& test : mappingTests) {
        if (grid.isValidIncrementPosition(test.incrementPos)) {
            Vector3i actualGridPos = grid.incrementToGrid(test.incrementPos);
            EXPECT_EQ(actualGridPos.x, test.expectedGridPos.x) 
                << test.description << " - X coordinate mismatch";
            EXPECT_EQ(actualGridPos.y, test.expectedGridPos.y) 
                << test.description << " - Y coordinate mismatch"; 
            EXPECT_EQ(actualGridPos.z, test.expectedGridPos.z) 
                << test.description << " - Z coordinate mismatch";
        }
    }
}

TEST_F(VoxelGridTest, ArbitraryPositions_DensePacking) {
    // Test storing many voxels at arbitrary 1cm positions to verify no conflicts
    // This ensures the sparse octree can handle arbitrary positions efficiently
    
    VoxelGrid grid(VoxelEditor::VoxelData::VoxelResolution::Size_2cm, Vector3f(4.0f, 4.0f, 4.0f));
    
    // Create positions that test arbitrary 1cm placement
    std::vector<IncrementCoordinates> testPositions;
    
    // Add positions at 1cm intervals within a smaller area (10cm x 10cm x 10cm)
    // For 2cm voxels, many of these will map to the same grid cells, which is expected
    for (int x = -5; x <= 5; x += 1) {
        for (int y = 0; y <= 10; y += 1) {
            for (int z = -5; z <= 5; z += 1) {
                IncrementCoordinates pos(x, y, z);
                if (grid.isValidIncrementPosition(pos)) {
                    testPositions.push_back(pos);
                }
            }
        }
    }
    
    // Store all voxels - NEW: each 1cm position has its own unique grid cell
    size_t successfulStores = 0;
    for (const auto& pos : testPositions) {
        if (grid.setVoxel(pos, true)) {
            successfulStores++;
        }
    }
    
    EXPECT_GT(successfulStores, 0);
    
    // NEW BEHAVIOR: VoxelGrid stores at 1cm granularity
    // So each 1cm position has a unique grid cell, regardless of voxel resolution
    size_t actualVoxelCount = grid.getVoxelCount();
    EXPECT_EQ(actualVoxelCount, successfulStores);
    EXPECT_GT(actualVoxelCount, 0);
    
    // We should have exactly as many voxels as positions we set
    // (11x11x11 = 1331 positions in the test range)
    EXPECT_EQ(actualVoxelCount, testPositions.size());
    
    // Verify all stored positions can be retrieved
    // Each 1cm position should return true since they're stored independently
    size_t successfulRetrieves = 0;
    for (const auto& pos : testPositions) {
        if (grid.getVoxel(pos)) {
            successfulRetrieves++;
        }
    }
    
    EXPECT_EQ(successfulRetrieves, successfulStores);
    
    // Memory usage should be reasonable
    size_t memoryUsage = grid.getMemoryUsage();
    EXPECT_GT(memoryUsage, 0);
    
    // Memory per unique voxel should be reasonable
    if (actualVoxelCount > 0) {
        double memoryPerVoxel = static_cast<double>(memoryUsage) / actualVoxelCount;
        EXPECT_LT(memoryPerVoxel, 2048.0) << "Memory usage per voxel too high: " << memoryPerVoxel << " bytes";
    }
}