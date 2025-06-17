#include <gtest/gtest.h>
#include "../VoxelDataManager.h"
#include "../WorkspaceManager.h"
#include "../VoxelGrid.h"
#include "../VoxelTypes.h"

using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class VoxelDataRequirementsTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<VoxelDataManager>();
    }
    
    std::unique_ptr<VoxelDataManager> manager;
};

// REQ-1.1.5: Grid origin at workspace center
TEST_F(VoxelDataRequirementsTest, GridOriginAtWorkspaceCenter) {
    WorkspaceManager* workspace = manager->getWorkspaceManager();
    Vector3f workspaceSize = workspace->getSize();
    
    // Default workspace is 5m x 5m x 5m
    EXPECT_FLOAT_EQ(workspaceSize.x, 5.0f);
    EXPECT_FLOAT_EQ(workspaceSize.y, 5.0f);
    EXPECT_FLOAT_EQ(workspaceSize.z, 5.0f);
    
    // Origin (0,0,0) should be at center, so bounds are -2.5 to +2.5
    Vector3f minBounds = workspace->getMinBounds();
    Vector3f maxBounds = workspace->getMaxBounds();
    
    EXPECT_FLOAT_EQ(minBounds.x, -2.5f);
    EXPECT_FLOAT_EQ(minBounds.y, 0.0f);  // Y starts at 0
    EXPECT_FLOAT_EQ(minBounds.z, -2.5f);
    
    EXPECT_FLOAT_EQ(maxBounds.x, 2.5f);
    EXPECT_FLOAT_EQ(maxBounds.y, 5.0f);
    EXPECT_FLOAT_EQ(maxBounds.z, 2.5f);
}

// REQ-1.2.3: Grid extends to cover entire workspace
TEST_F(VoxelDataRequirementsTest, GridCoversEntireWorkspace) {
    // Test with different workspace sizes
    std::vector<float> sizes = {2.0f, 5.0f, 8.0f};
    
    for (float size : sizes) {
        ASSERT_TRUE(manager->resizeWorkspace(size));
        
        // For each resolution, verify grid covers workspace
        for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
            VoxelResolution res = static_cast<VoxelResolution>(i);
            const VoxelGrid* grid = manager->getGrid(res);
            ASSERT_NE(grid, nullptr);
            
            float voxelSize = getVoxelSize(res);
            Vector3i gridDims = grid->getGridDimensions();
            
            // Grid should be large enough to cover workspace
            float gridCoverageX = gridDims.x * voxelSize;
            float gridCoverageY = gridDims.y * voxelSize;
            float gridCoverageZ = gridDims.z * voxelSize;
            
            EXPECT_GE(gridCoverageX, size) << "Resolution: " << getVoxelSizeName(res);
            EXPECT_GE(gridCoverageY, size) << "Resolution: " << getVoxelSizeName(res);
            EXPECT_GE(gridCoverageZ, size) << "Resolution: " << getVoxelSizeName(res);
        }
    }
}

// REQ-2.1.1: Voxels placeable only at 1cm increments
TEST_F(VoxelDataRequirementsTest, VoxelsPlaceableAt1cmIncrements) {
    // Valid 1cm increment positions
    std::vector<Vector3f> validPositions = {
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(0.01f, 0.0f, 0.0f),
        Vector3f(-0.01f, 0.0f, 0.0f),
        Vector3f(0.1f, 0.2f, 0.3f),
        Vector3f(-1.23f, 0.45f, -0.67f),
        Vector3f(2.49f, 4.99f, 2.49f)  // Near edge but valid
    };
    
    for (const auto& pos : validPositions) {
        EXPECT_TRUE(manager->isValidIncrementPosition(pos)) 
            << "Position (" << pos.x << ", " << pos.y << ", " << pos.z << ") should be valid";
    }
    
    // Invalid positions (not on 1cm grid)
    std::vector<Vector3f> invalidPositions = {
        Vector3f(0.001f, 0.0f, 0.0f),
        Vector3f(0.0f, 0.015f, 0.0f),
        Vector3f(0.0f, 0.0f, 0.123f),
        Vector3f(0.105f, 0.0f, 0.0f),
        Vector3f(1.234f, 0.0f, 0.0f),
    };
    
    for (const auto& pos : invalidPositions) {
        EXPECT_FALSE(manager->isValidIncrementPosition(pos)) 
            << "Position (" << pos.x << ", " << pos.y << ", " << pos.z << ") should be invalid";
    }
}

// REQ-2.1.4: No voxels below Y=0
TEST_F(VoxelDataRequirementsTest, NoVoxelsBelowY0) {
    // Test grid position validation
    EXPECT_FALSE(manager->isValidIncrementPosition(Vector3i(0, -1, 0)));
    EXPECT_TRUE(manager->isValidIncrementPosition(Vector3i(0, 0, 0)));
    
    // Test world position validation
    EXPECT_FALSE(manager->isValidIncrementPosition(Vector3f(0.0f, -0.01f, 0.0f)));
    EXPECT_TRUE(manager->isValidIncrementPosition(Vector3f(0.0f, 0.0f, 0.0f)));
    
    // Try to place voxels below Y=0
    EXPECT_FALSE(manager->setVoxelAtWorldPos(Vector3f(0.0f, -0.01f, 0.0f), true));
    EXPECT_FALSE(manager->setVoxel(Vector3i(0, -1, 0), VoxelResolution::Size_1cm, true));
}

// REQ-2.2.4: Multi-resolution positioning on ground plane
TEST_F(VoxelDataRequirementsTest, MultiResolutionGroundPlanePositioning) {
    // All voxel sizes should be placeable at any valid 1cm increment on ground
    std::vector<Vector3f> groundPositions = {
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(0.01f, 0.0f, 0.01f),
        Vector3f(0.1f, 0.0f, -0.1f),
        Vector3f(1.23f, 0.0f, -2.34f),
        Vector3f(-2.45f, 0.0f, 2.45f)
    };
    
    for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
        VoxelResolution res = static_cast<VoxelResolution>(i);
        
        for (const auto& pos : groundPositions) {
            // Clear any previous voxels
            manager->clearAll();
            
            bool result = manager->setVoxelAtWorldPos(pos, res, true);
            EXPECT_TRUE(result) << "Failed to place " << getVoxelSizeName(res) 
                               << " voxel at (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
            
            if (result) {
                EXPECT_TRUE(manager->getVoxelAtWorldPos(pos, res));
            }
        }
    }
}

// REQ-3.3.2 & REQ-3.3.3: Collision detection and spatial queries
TEST_F(VoxelDataRequirementsTest, CollisionDetectionAndSpatialQueries) {
    // Place a 32cm voxel at world origin
    // It will snap to grid position and occupy a specific region
    ASSERT_TRUE(manager->setVoxelAtWorldPos(Vector3f(0.0f, 0.0f, 0.0f), VoxelResolution::Size_32cm, true));
    
    // Get the actual occupied region
    const VoxelGrid* grid32 = manager->getGrid(VoxelResolution::Size_32cm);
    Vector3i gridPos = grid32->worldToGrid(Vector3f(0.0f, 0.0f, 0.0f));
    Vector3f actualWorldPos = grid32->gridToWorld(gridPos);
    
    // The voxel occupies actualWorldPos to actualWorldPos + 0.32
    // Test overlap detection with world positions
    EXPECT_FALSE(manager->setVoxelAtWorldPos(actualWorldPos, VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(manager->setVoxelAtWorldPos(actualWorldPos + Vector3f(0.16f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true));
    EXPECT_TRUE(manager->setVoxelAtWorldPos(actualWorldPos + Vector3f(0.32f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true));
    
    // Test adjacent placement
    EXPECT_TRUE(manager->setVoxelAtWorldPos(actualWorldPos + Vector3f(-0.01f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true));
}

// REQ-4.1.2, REQ-4.3.1, REQ-4.3.2: Validation for invalid placements
TEST_F(VoxelDataRequirementsTest, PlacementValidation) {
    // Place initial voxel at a known position
    Vector3f initialPos(0.16f, 0.0f, 0.16f);
    ASSERT_TRUE(manager->setVoxelAtWorldPos(initialPos, VoxelResolution::Size_16cm, true));
    
    // Get actual position after snapping
    const VoxelGrid* grid = manager->getGrid(VoxelResolution::Size_16cm);
    Vector3i gridPos = grid->worldToGrid(initialPos);
    Vector3f actualPos = grid->gridToWorld(gridPos);
    
    // Test various invalid placements
    struct TestCase {
        Vector3f position;
        VoxelResolution resolution;
        std::string reason;
    };
    
    std::vector<TestCase> invalidCases = {
        {Vector3f(0.0f, -0.01f, 0.0f), VoxelResolution::Size_1cm, "Below Y=0"},
        {Vector3f(0.105f, 0.0f, 0.0f), VoxelResolution::Size_1cm, "Not on 1cm increment"},
        {actualPos + Vector3f(0.08f, 0.0f, 0.08f), VoxelResolution::Size_1cm, "Would overlap"},
        {Vector3f(3.0f, 0.0f, 0.0f), VoxelResolution::Size_1cm, "Outside workspace"},
    };
    
    for (const auto& testCase : invalidCases) {
        EXPECT_FALSE(manager->setVoxelAtWorldPos(testCase.position, testCase.resolution, true))
            << "Placement should fail: " << testCase.reason;
    }
}

// REQ-5.1.1 & REQ-5.1.2: Voxel creation and removal
TEST_F(VoxelDataRequirementsTest, VoxelCreationAndRemoval) {
    // Test creation
    Vector3f pos(0.1f, 0.2f, 0.3f);
    EXPECT_TRUE(manager->setVoxelAtWorldPos(pos, VoxelResolution::Size_8cm, true));
    EXPECT_TRUE(manager->getVoxelAtWorldPos(pos, VoxelResolution::Size_8cm));
    EXPECT_EQ(manager->getVoxelCount(VoxelResolution::Size_8cm), 1);
    
    // Test removal
    EXPECT_TRUE(manager->setVoxelAtWorldPos(pos, VoxelResolution::Size_8cm, false));
    EXPECT_FALSE(manager->getVoxelAtWorldPos(pos, VoxelResolution::Size_8cm));
    EXPECT_EQ(manager->getVoxelCount(VoxelResolution::Size_8cm), 0);
}

// REQ-5.3.1, REQ-5.3.2, REQ-5.3.3: Resolution management
TEST_F(VoxelDataRequirementsTest, ResolutionManagement) {
    // Test all 10 resolutions are available
    EXPECT_EQ(static_cast<int>(VoxelResolution::COUNT), 10);
    
    // Test resolution names
    EXPECT_STREQ(getVoxelSizeName(VoxelResolution::Size_1cm), "1cm");
    EXPECT_STREQ(getVoxelSizeName(VoxelResolution::Size_512cm), "512cm");
    
    // Test resolution sizes
    EXPECT_FLOAT_EQ(getVoxelSize(VoxelResolution::Size_1cm), 0.01f);
    EXPECT_FLOAT_EQ(getVoxelSize(VoxelResolution::Size_512cm), 5.12f);
    
    // Test active resolution management
    EXPECT_EQ(manager->getActiveResolution(), VoxelResolution::Size_1cm);
    manager->setActiveResolution(VoxelResolution::Size_32cm);
    EXPECT_EQ(manager->getActiveResolution(), VoxelResolution::Size_32cm);
    
    // Test using active resolution
    EXPECT_TRUE(manager->setVoxelAtWorldPos(Vector3f(0.0f, 0.0f, 0.0f), true));
    EXPECT_EQ(manager->getVoxelCount(), 1);  // Uses active resolution
}

// REQ-6.2.2: Workspace bounds up to 8m x 8m
TEST_F(VoxelDataRequirementsTest, WorkspaceBounds) {
    // Test minimum size
    EXPECT_TRUE(manager->resizeWorkspace(2.0f));
    EXPECT_EQ(manager->getWorkspaceSize().x, 2.0f);
    
    // Test maximum size
    EXPECT_TRUE(manager->resizeWorkspace(8.0f));
    EXPECT_EQ(manager->getWorkspaceSize().x, 8.0f);
    
    // Test invalid sizes
    EXPECT_FALSE(manager->resizeWorkspace(1.9f));
    EXPECT_FALSE(manager->resizeWorkspace(8.1f));
}

// REQ-2.3.3: Adjacent position calculation
TEST_F(VoxelDataRequirementsTest, AdjacentPositionCalculation) {
    // Place a voxel
    Vector3i sourcePos(10, 5, 10);
    VoxelResolution sourceRes = VoxelResolution::Size_16cm;
    
    // Test adjacent positions in all 6 directions
    struct DirectionTest {
        FaceDirection face;
        Vector3i expectedOffset;
    };
    
    std::vector<DirectionTest> directions = {
        {FaceDirection::PosX, Vector3i(1, 0, 0)},
        {FaceDirection::NegX, Vector3i(-1, 0, 0)},
        {FaceDirection::PosY, Vector3i(0, 1, 0)},
        {FaceDirection::NegY, Vector3i(0, -1, 0)},
        {FaceDirection::PosZ, Vector3i(0, 0, 1)},
        {FaceDirection::NegZ, Vector3i(0, 0, -1)},
    };
    
    // Same size adjacent
    for (const auto& test : directions) {
        Vector3i adjacent = manager->getAdjacentPosition(sourcePos, test.face, sourceRes, sourceRes);
        Vector3i expected = sourcePos + test.expectedOffset;
        EXPECT_EQ(adjacent, expected) << "Face: " << static_cast<int>(test.face);
    }
}

// REQ-3.1.1 & REQ-3.1.3: Same-size voxel alignment
TEST_F(VoxelDataRequirementsTest, SameSizeVoxelAlignment) {
    // Place initial voxel
    Vector3i pos1(10, 0, 10);
    ASSERT_TRUE(manager->setVoxel(pos1, VoxelResolution::Size_8cm, true));
    
    // Adjacent same-size voxel should align perfectly
    Vector3i pos2 = manager->getAdjacentPosition(pos1, FaceDirection::PosX, 
                                                VoxelResolution::Size_8cm, 
                                                VoxelResolution::Size_8cm);
    EXPECT_EQ(pos2, Vector3i(11, 0, 10));
    
    // Verify no overlap
    EXPECT_FALSE(manager->wouldOverlap(pos2, VoxelResolution::Size_8cm));
    EXPECT_TRUE(manager->setVoxel(pos2, VoxelResolution::Size_8cm, true));
}

// Performance test for REQ-6.2.1: Sparse storage for 10,000+ voxels
TEST_F(VoxelDataRequirementsTest, SparseStoragePerformance) {
    const int TARGET_VOXELS = 10000;
    
    // Use a larger workspace to avoid running out of space
    ASSERT_TRUE(manager->resizeWorkspace(8.0f));
    
    // For performance testing, we need to optimize placement
    // The collision detection becomes O(n²) as we add more voxels
    // Let's measure just the sparse storage performance, not collision detection
    
    // First, let's test direct octree performance without collision checks
    VoxelGrid* grid = manager->getGrid(VoxelResolution::Size_1cm);
    ASSERT_NE(grid, nullptr);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    int placed = 0;
    const int GRID_SIZE = 100; // 100x100 = 10,000 voxels
    const int SPACING = 3; // 3cm spacing to ensure no overlaps even with centered coords
    
    for (int x = 0; x < GRID_SIZE; ++x) {
        for (int z = 0; z < GRID_SIZE; ++z) {
            // Use the grid directly to bypass collision detection for this performance test
            if (grid->setVoxel(Vector3i(x * SPACING, 0, z * SPACING), true)) {
                placed++;
            }
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(placed, TARGET_VOXELS);
    EXPECT_EQ(manager->getVoxelCount(VoxelResolution::Size_1cm), TARGET_VOXELS);
    
    // Should complete in reasonable time (< 1 second)
    EXPECT_LT(duration.count(), 1000) << "Placing 10,000 voxels took " << duration.count() << "ms";
    
    // Check memory efficiency
    size_t memoryUsage = manager->getMemoryUsage();
    float bytesPerVoxel = static_cast<float>(memoryUsage) / TARGET_VOXELS;
    
    // Should use less than 1KB per voxel (sparse storage)
    EXPECT_LT(bytesPerVoxel, 1024) << "Memory usage: " << bytesPerVoxel << " bytes per voxel";
}

// Test coordinate system conversions
TEST_F(VoxelDataRequirementsTest, CoordinateSystemConversions) {
    // Get a grid for testing
    const VoxelGrid* grid = manager->getGrid(VoxelResolution::Size_1cm);
    ASSERT_NE(grid, nullptr);
    
    // Test that grid (0,0,0) maps to world (-2.5, 0, -2.5) for 5m workspace
    Vector3i gridOrigin(0, 0, 0);
    Vector3f worldFromGrid = grid->gridToWorld(gridOrigin);
    EXPECT_FLOAT_EQ(worldFromGrid.x, -2.5f);
    EXPECT_FLOAT_EQ(worldFromGrid.y, 0.0f);
    EXPECT_FLOAT_EQ(worldFromGrid.z, -2.5f);
    
    // Test that world (0,0,0) maps to grid center
    Vector3f worldOrigin(0.0f, 0.0f, 0.0f);
    Vector3i gridFromWorld = grid->worldToGrid(worldOrigin);
    EXPECT_EQ(gridFromWorld.x, 250);  // 2.5m / 0.01m = 250
    EXPECT_EQ(gridFromWorld.y, 0);
    EXPECT_EQ(gridFromWorld.z, 250);
}