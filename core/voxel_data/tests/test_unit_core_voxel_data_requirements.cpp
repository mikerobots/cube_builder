#include <gtest/gtest.h>
#include "../VoxelDataManager.h"
#include "../WorkspaceManager.h"
#include "../VoxelGrid.h"
#include "../VoxelTypes.h"

using namespace VoxelEditor::VoxelData;
// Don't import Math namespace to avoid VoxelResolution ambiguity
// using namespace VoxelEditor::Math;

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
    VoxelEditor::Math::Vector3f workspaceSize = workspace->getSize();
    
    // Default workspace is 5m x 5m x 5m
    EXPECT_FLOAT_EQ(workspaceSize.x, 5.0f);
    EXPECT_FLOAT_EQ(workspaceSize.y, 5.0f);
    EXPECT_FLOAT_EQ(workspaceSize.z, 5.0f);
    
    // Origin (0,0,0) should be at center, so bounds are -2.5 to +2.5
    VoxelEditor::Math::Vector3f minBounds = workspace->getMinBounds();
    VoxelEditor::Math::Vector3f maxBounds = workspace->getMaxBounds();
    
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
            VoxelEditor::Math::Vector3i gridDims = grid->getGridDimensions();
            
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
    std::vector<VoxelEditor::Math::Vector3f> validPositions = {
        VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.0f),
        VoxelEditor::Math::Vector3f(0.01f, 0.0f, 0.0f),
        VoxelEditor::Math::Vector3f(-0.01f, 0.0f, 0.0f),
        VoxelEditor::Math::Vector3f(0.1f, 0.2f, 0.3f),
        VoxelEditor::Math::Vector3f(-1.23f, 0.45f, -0.67f),
        VoxelEditor::Math::Vector3f(2.49f, 4.99f, 2.49f)  // Near edge but valid
    };
    
    for (const auto& pos : validPositions) {
        EXPECT_TRUE(manager->isValidIncrementPosition(pos)) 
            << "Position (" << pos.x << ", " << pos.y << ", " << pos.z << ") should be valid";
    }
    
    // Invalid positions (not on 1cm grid)
    std::vector<VoxelEditor::Math::Vector3f> invalidPositions = {
        VoxelEditor::Math::Vector3f(0.001f, 0.0f, 0.0f),
        VoxelEditor::Math::Vector3f(0.0f, 0.015f, 0.0f),
        VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.123f),
        VoxelEditor::Math::Vector3f(0.105f, 0.0f, 0.0f),
        VoxelEditor::Math::Vector3f(1.234f, 0.0f, 0.0f),
    };
    
    for (const auto& pos : invalidPositions) {
        EXPECT_FALSE(manager->isValidIncrementPosition(pos)) 
            << "Position (" << pos.x << ", " << pos.y << ", " << pos.z << ") should be invalid";
    }
}

// REQ-2.1.4: No voxels below Y=0
TEST_F(VoxelDataRequirementsTest, NoVoxelsBelowY0) {
    // Test grid position validation
    EXPECT_FALSE(manager->isValidIncrementPosition(VoxelEditor::Math::Vector3i(0, -1, 0)));
    EXPECT_TRUE(manager->isValidIncrementPosition(VoxelEditor::Math::Vector3i(0, 0, 0)));
    
    // Test world position validation
    EXPECT_FALSE(manager->isValidIncrementPosition(VoxelEditor::Math::Vector3f(0.0f, -0.01f, 0.0f)));
    EXPECT_TRUE(manager->isValidIncrementPosition(VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.0f)));
    
    // Try to place voxels below Y=0
    EXPECT_FALSE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.0f, -0.01f, 0.0f), true));
    EXPECT_FALSE(manager->setVoxel(VoxelEditor::Math::Vector3i(0, -1, 0), VoxelResolution::Size_1cm, true));
}

// REQ-2.2.4: Multi-resolution positioning on ground plane
TEST_F(VoxelDataRequirementsTest, MultiResolutionGroundPlanePositioning) {
    // All voxel sizes should be placeable at grid-aligned positions on ground plane
    // Use larger workspace for very large voxels
    ASSERT_TRUE(manager->resizeWorkspace(8.0f));  // 8m workspace for large voxels
    
    for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
        VoxelResolution res = static_cast<VoxelResolution>(i);
        float voxelSize = getVoxelSize(res);
        
        // Test positions that align to this resolution's grid and fit in workspace
        std::vector<VoxelEditor::Math::Vector3f> alignedPositions;
        
        // Origin always works
        alignedPositions.push_back(VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.0f));
        
        // For smaller voxels, test additional positions
        if (voxelSize < 2.0f) {  // Only test multiple positions for voxels < 2m
            alignedPositions.push_back(VoxelEditor::Math::Vector3f(voxelSize, 0.0f, 0.0f));
            alignedPositions.push_back(VoxelEditor::Math::Vector3f(-voxelSize, 0.0f, 0.0f));
            alignedPositions.push_back(VoxelEditor::Math::Vector3f(0.0f, 0.0f, voxelSize));
        }
        
        for (const auto& pos : alignedPositions) {
            // Clear any previous voxels
            manager->clearAll();
            
            bool result = manager->setVoxelAtWorldPos(pos, res, true);
            EXPECT_TRUE(result) << "Failed to place " << getVoxelSizeName(res) 
                               << " voxel at grid-aligned position (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
            
            if (result) {
                EXPECT_TRUE(manager->getVoxelAtWorldPos(pos, res));
            }
        }
    }
}

// REQ-3.3.2 & REQ-3.3.3: Collision detection and spatial queries
TEST_F(VoxelDataRequirementsTest, CollisionDetectionAndSpatialQueries) {
    // Place a large voxel at origin
    ASSERT_TRUE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.0f), VoxelResolution::Size_32cm, true));
    
    // Test collision detection - small voxel at same position should fail
    EXPECT_FALSE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true));
    
    // Test spatial queries - check if positions are occupied
    EXPECT_TRUE(manager->wouldOverlap(VoxelEditor::Math::Vector3i(0, 0, 0), VoxelResolution::Size_1cm));
    EXPECT_FALSE(manager->wouldOverlap(VoxelEditor::Math::Vector3i(100, 0, 100), VoxelResolution::Size_1cm));
    
    // Test placement far away from existing voxel - should succeed
    EXPECT_TRUE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(1.0f, 0.0f, 1.0f), VoxelResolution::Size_1cm, true));
    EXPECT_TRUE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(-1.0f, 0.0f, -1.0f), VoxelResolution::Size_1cm, true));
}

// REQ-4.1.2, REQ-4.3.1, REQ-4.3.2: Validation for invalid placements
TEST_F(VoxelDataRequirementsTest, PlacementValidation) {
    // Place a voxel at origin
    ASSERT_TRUE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.0f), VoxelResolution::Size_16cm, true));
    
    // Test basic invalid placements
    
    // 1. Y < 0 should always fail
    EXPECT_FALSE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.0f, -0.01f, 0.0f), VoxelResolution::Size_1cm, true))
        << "Placement below Y=0 should fail";
    
    // 2. Overlap with existing voxel should fail
    EXPECT_FALSE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true))
        << "Placement overlapping existing voxel should fail";
    
    // 3. Outside workspace bounds should fail
    EXPECT_FALSE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(10.0f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true))
        << "Placement outside workspace should fail";
    
    // 4. Valid placements should succeed
    EXPECT_TRUE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(1.0f, 0.0f, 1.0f), VoxelResolution::Size_1cm, true))
        << "Valid placement should succeed";
}

// REQ-5.1.1 & REQ-5.1.2: Voxel creation and removal
TEST_F(VoxelDataRequirementsTest, VoxelCreationAndRemoval) {
    // Test creation
    VoxelEditor::Math::Vector3f pos(0.1f, 0.2f, 0.3f);
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
    EXPECT_TRUE(manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.0f), true));
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
    VoxelEditor::Math::Vector3i sourcePos(10, 5, 10);
    VoxelResolution sourceRes = VoxelResolution::Size_16cm;
    
    // Test adjacent positions in all 6 directions
    struct DirectionTest {
        FaceDirection face;
        VoxelEditor::Math::Vector3i expectedOffset;
    };
    
    std::vector<DirectionTest> directions = {
        {FaceDirection::PosX, VoxelEditor::Math::Vector3i(1, 0, 0)},
        {FaceDirection::NegX, VoxelEditor::Math::Vector3i(-1, 0, 0)},
        {FaceDirection::PosY, VoxelEditor::Math::Vector3i(0, 1, 0)},
        {FaceDirection::NegY, VoxelEditor::Math::Vector3i(0, -1, 0)},
        {FaceDirection::PosZ, VoxelEditor::Math::Vector3i(0, 0, 1)},
        {FaceDirection::NegZ, VoxelEditor::Math::Vector3i(0, 0, -1)},
    };
    
    // Same size adjacent
    for (const auto& test : directions) {
        VoxelEditor::Math::Vector3i adjacent = manager->getAdjacentPosition(sourcePos, test.face, sourceRes, sourceRes);
        VoxelEditor::Math::Vector3i expected = sourcePos + test.expectedOffset;
        EXPECT_EQ(adjacent, expected) << "Face: " << static_cast<int>(test.face);
    }
}

// REQ-3.1.1 & REQ-3.1.3: Same-size voxel alignment
TEST_F(VoxelDataRequirementsTest, SameSizeVoxelAlignment) {
    // Test that same-size voxels can be placed adjacent to each other without overlap
    // Use positions that are definitely non-overlapping for 8cm voxels
    
    // Place first voxel at origin
    EXPECT_TRUE(manager->setVoxel(VoxelEditor::Math::Vector3i(0, 0, 0), VoxelResolution::Size_8cm, true));
    
    // Place second voxel 8cm away (8 increment units) - should not overlap
    EXPECT_TRUE(manager->setVoxel(VoxelEditor::Math::Vector3i(8, 0, 0), VoxelResolution::Size_8cm, true));
    
    // Place third voxel in Z direction - should not overlap  
    EXPECT_TRUE(manager->setVoxel(VoxelEditor::Math::Vector3i(0, 0, 8), VoxelResolution::Size_8cm, true));
    
    // Verify all voxels are placed
    EXPECT_TRUE(manager->getVoxel(VoxelEditor::Math::Vector3i(0, 0, 0), VoxelResolution::Size_8cm));
    EXPECT_TRUE(manager->getVoxel(VoxelEditor::Math::Vector3i(8, 0, 0), VoxelResolution::Size_8cm));
    EXPECT_TRUE(manager->getVoxel(VoxelEditor::Math::Vector3i(0, 0, 8), VoxelResolution::Size_8cm));
    
    // Test adjacent position calculation if available
    VoxelEditor::Math::Vector3i adjacentPos = manager->getAdjacentPosition(
        VoxelEditor::Math::Vector3i(0, 0, 0), 
        FaceDirection::PosX,
        VoxelResolution::Size_8cm, 
        VoxelResolution::Size_8cm);
    
    // Adjacent position should be different from original and not cause overlap
    EXPECT_NE(adjacentPos, VoxelEditor::Math::Vector3i(0, 0, 0));
}

// Performance test for REQ-6.2.1: Sparse storage for 10,000+ voxels  
// TEST_F(VoxelDataRequirementsTest, SparseStoragePerformance) {
//     const int TARGET_VOXELS = 10000;
//     
//     // Use a larger workspace to avoid running out of space
//     ASSERT_TRUE(manager->resizeWorkspace(8.0f));
//     
//     // Clear any existing voxels
//     manager->clearAll();
//     
//     auto start = std::chrono::high_resolution_clock::now();
//     
//     int placed = 0;
//     // Place voxels in a 3D grid using world coordinates
//     // 8m workspace gives us -4m to +4m range
//     const float SPACING = 0.05f; // 5cm spacing in world coordinates
//     const int DIM = 22; // 22^3 = 10,648 > 10,000
//     
//     for (int x = 0; x < DIM && placed < TARGET_VOXELS; ++x) {
//         for (int y = 0; y < DIM && placed < TARGET_VOXELS; ++y) {
//             for (int z = 0; z < DIM && placed < TARGET_VOXELS; ++z) {
//                 // Calculate world position starting from near origin
//                 float worldX = -0.5f + (x * SPACING);
//                 float worldY = 0.0f + (y * SPACING);
//                 float worldZ = -0.5f + (z * SPACING);
//                 
//                 // Place voxel bypassing collision detection
//                 VoxelEditor::Math::Vector3f worldPos(worldX, worldY, worldZ);
//                 VoxelGrid* grid = manager->getGrid(VoxelResolution::Size_1cm);
//                 if (grid && grid->setVoxelAtWorldPos(worldPos, true)) {
//                     placed++;
//                 }
//             }
//         }
//     }
//     
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//     
//     EXPECT_EQ(placed, TARGET_VOXELS);
//     EXPECT_EQ(manager->getVoxelCount(VoxelResolution::Size_1cm), TARGET_VOXELS);
//     
//     // Should complete in reasonable time (< 1 second)
//     EXPECT_LT(duration.count(), 1000) << "Placing 10,000 voxels took " << duration.count() << "ms";
//     
//     // Check memory efficiency
//     size_t memoryUsage = manager->getMemoryUsage();
//     float bytesPerVoxel = static_cast<float>(memoryUsage) / TARGET_VOXELS;
//     
//     // Should use less than 1KB per voxel (sparse storage)
//     EXPECT_LT(bytesPerVoxel, 1024) << "Memory usage: " << bytesPerVoxel << " bytes per voxel";
// }

// Test coordinate system conversions
TEST_F(VoxelDataRequirementsTest, CoordinateSystemConversions) {
    // Get a grid for testing
    const VoxelGrid* grid = manager->getGrid(VoxelResolution::Size_1cm);
    ASSERT_NE(grid, nullptr);
    
    // Test that increment (0,0,0) maps to world (0,0,0) in centered coordinate system
    VoxelEditor::Math::Vector3i incrementOrigin(0, 0, 0);
    VoxelEditor::Math::Vector3f worldFromIncrement = grid->incrementToWorld(incrementOrigin);
    EXPECT_NEAR(worldFromIncrement.x, 0.0f, 0.01f);
    EXPECT_NEAR(worldFromIncrement.y, 0.0f, 0.01f);
    EXPECT_NEAR(worldFromIncrement.z, 0.0f, 0.01f);
    
    // Test that world (0,0,0) maps to grid center
    VoxelEditor::Math::Vector3f worldOrigin(0.0f, 0.0f, 0.0f);
    VoxelEditor::Math::Vector3i incrementFromWorld = grid->worldToIncrement(worldOrigin);
    EXPECT_EQ(incrementFromWorld.x, 0);  // World origin maps to increment origin
    EXPECT_EQ(incrementFromWorld.y, 0);
    EXPECT_EQ(incrementFromWorld.z, 0);
}

// REQ-2.1.2: 32 valid positions per axis in 32cm cell
TEST_F(VoxelDataRequirementsTest, ValidPositionsIn32cmCell_REQ_2_1_2) {
    // In a 32cm cell, there should be exactly 32 valid 1cm increment positions per axis
    // Test that positions 0-31cm are valid, but 32cm starts a new cell
    
    // Test X axis positions within a 32cm cell
    for (int i = 0; i < 32; ++i) {
        float xPos = i * 0.01f; // Convert to meters
        VoxelEditor::Math::Vector3f pos(xPos, 0.0f, 0.0f);
        EXPECT_TRUE(manager->isValidIncrementPosition(pos)) 
            << "Position " << xPos << "m should be valid within 32cm cell";
    }
    
    // Verify the pattern repeats in the next cell
    for (int i = 32; i < 64; ++i) {
        float xPos = i * 0.01f;
        VoxelEditor::Math::Vector3f pos(xPos, 0.0f, 0.0f);
        EXPECT_TRUE(manager->isValidIncrementPosition(pos)) 
            << "Position " << xPos << "m should be valid in next 32cm cell";
    }
}

// REQ-5.2.1 & REQ-5.2.2: Overlap prevention and validation
TEST_F(VoxelDataRequirementsTest, OverlapPreventionAndValidation_REQ_5_2_1_REQ_5_2_2) {
    // Place a voxel
    VoxelEditor::Math::Vector3f pos1(0.0f, 0.0f, 0.0f);
    ASSERT_TRUE(manager->setVoxelAtWorldPos(pos1, VoxelResolution::Size_16cm, true));
    
    // REQ-5.2.2: System shall validate placement before allowing it
    // Check validation detects overlap
    EXPECT_TRUE(manager->wouldOverlap(VoxelEditor::Math::Vector3i(0, 0, 0), VoxelResolution::Size_1cm));
    
    // REQ-5.2.1: Voxels shall not overlap with existing voxels
    // Try to place overlapping voxel - should fail
    EXPECT_FALSE(manager->setVoxelAtWorldPos(pos1, VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(manager->setVoxelAtWorldPos(pos1, VoxelResolution::Size_16cm, true));
    
    // Place non-overlapping voxel - should succeed
    VoxelEditor::Math::Vector3f pos2(0.5f, 0.0f, 0.0f);
    EXPECT_TRUE(manager->setVoxelAtWorldPos(pos2, VoxelResolution::Size_1cm, true));
}

// REQ-6.1.4: Resolution switching performance
// TEST_F(VoxelDataRequirementsTest, ResolutionSwitchingPerformance_REQ_6_1_4) {
//     // Place some voxels at different resolutions
//     manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(0.0f, 0.0f, 0.0f), VoxelResolution::Size_1cm, true);
//     manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(1.0f, 0.0f, 0.0f), VoxelResolution::Size_32cm, true);
//     
//     // Test resolution switching performance
//     auto start = std::chrono::high_resolution_clock::now();
//     
//     // Switch through all resolutions
//     for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
//         VoxelResolution res = static_cast<VoxelResolution>(i);
//         manager->setActiveResolution(res);
//         EXPECT_EQ(manager->getActiveResolution(), res);
//     }
//     
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//     
//     // Should complete within 100ms
//     EXPECT_LT(duration.count(), 100) << "Resolution switching took " << duration.count() << "ms";
// }

// REQ-6.3.2: Voxel data storage limit
TEST_F(VoxelDataRequirementsTest, VoxelDataStorageLimit_REQ_6_3_2) {
    // This test verifies memory usage stays under 2GB
    // Note: This is a representative test, not exhaustive
    
    // Get baseline memory
    size_t baselineMemory = manager->getMemoryUsage();
    
    // Place a reasonable number of voxels
    const int VOXEL_COUNT = 1000;
    for (int i = 0; i < VOXEL_COUNT; ++i) {
        float x = (i % 100) * 0.05f;
        float y = ((i / 100) % 10) * 0.05f;
        float z = (i / 1000) * 0.05f;
        manager->setVoxelAtWorldPos(VoxelEditor::Math::Vector3f(x, y, z), VoxelResolution::Size_1cm, true);
    }
    
    size_t currentMemory = manager->getMemoryUsage();
    size_t memoryUsed = currentMemory - baselineMemory;
    
    // Verify memory usage is reasonable
    EXPECT_LT(memoryUsed, 10 * 1024 * 1024) << "1000 voxels should use less than 10MB";
    
    // Extrapolate to verify 2GB limit won't be exceeded
    // With sparse storage, we should be able to store millions of voxels
    size_t bytesPerVoxel = memoryUsed / VOXEL_COUNT;
    size_t maxVoxelsIn2GB = (2ULL * 1024 * 1024 * 1024) / bytesPerVoxel;
    EXPECT_GT(maxVoxelsIn2GB, 1000000) << "Should support at least 1M voxels in 2GB";
}

// REQ-6.3.5: Memory pressure detection
TEST_F(VoxelDataRequirementsTest, MemoryPressureDetection_REQ_6_3_5) {
    // Test basic memory reporting
    size_t baselineMemory = manager->getMemoryUsage();
    EXPECT_GT(baselineMemory, 0) << "Should report some baseline memory usage";
    
    // Place voxels and verify memory usage increases
    const int VOXEL_COUNT = 100;
    for (int i = 0; i < VOXEL_COUNT; ++i) {
        manager->setVoxelAtWorldPos(
            VoxelEditor::Math::Vector3f(i * 0.01f, 0.0f, 0.0f), 
            VoxelResolution::Size_1cm, true);
    }
    
    size_t afterMemory = manager->getMemoryUsage();
    EXPECT_GT(afterMemory, baselineMemory) << "Memory usage should increase after placing voxels";
    
    // Clear voxels and verify memory usage decreases
    manager->clearAll();
    size_t clearedMemory = manager->getMemoryUsage();
    EXPECT_LT(clearedMemory, afterMemory) << "Memory usage should decrease after clearing voxels";
    
    // Note: Full memory pressure detection and response would require
    // integration with the Memory subsystem's MemoryPool, which tracks
    // system-wide memory pressure and triggers cleanup callbacks
}

// Additional requirement coverage notes:
// REQ-2.1.3: Voxels always axis-aligned - no rotation support in system, so always true
// REQ-3.1.3: Aligned placement edges match - tested via adjacent position calculations
// REQ-3.2.2: Placement respects 1cm increments on target face - covered by increment validation
// REQ-6.3.1: Total memory < 4GB - tested in SparseOctree memory tests
// REQ-8.1.x: File format requirements - belong in file_io subsystem tests
// REQ-9.2.3: CLI commands - belong in CLI application tests
// UI-related requirements (REQ-4.1.2, REQ-4.3.2, REQ-4.3.3) - belong in input/visual_feedback subsystems