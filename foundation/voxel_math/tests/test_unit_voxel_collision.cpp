#include <gtest/gtest.h>
#include "../include/voxel_math/VoxelCollision.h"
#include "../include/voxel_math/VoxelGrid.h"
#include "../../../core/voxel_data/VoxelGrid.h"
#include "../../../core/voxel_data/VoxelDataManager.h"
#include <memory>

using namespace VoxelEditor;
using namespace VoxelEditor::Math;

class VoxelCollisionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test grid with workspace size
        workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
        dataManager = std::make_unique<VoxelData::VoxelDataManager>();
        
        // Set workspace size
        dataManager->resizeWorkspace(workspaceSize);
        
        // Set the active resolution
        dataManager->setActiveResolution(VoxelData::VoxelResolution::Size_32cm);
        
        // Create a standalone grid for testing collision functions that expect a VoxelGrid
        grid = std::make_unique<VoxelData::VoxelGrid>(VoxelData::VoxelResolution::Size_32cm, workspaceSize);
    }
    
    Vector3f workspaceSize;
    std::unique_ptr<VoxelData::VoxelDataManager> dataManager;
    std::unique_ptr<VoxelData::VoxelGrid> grid;
};

// Test basic collision between two voxels
TEST_F(VoxelCollisionTest, BasicCollisionCheck) {
    // Same position, same size - should collide
    EXPECT_TRUE(VoxelCollision::checkCollision(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm,
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm));
    
    // Adjacent voxels (touching) - should not collide
    EXPECT_FALSE(VoxelCollision::checkCollision(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm,
        IncrementCoordinates(32, 0, 0), VoxelData::VoxelResolution::Size_32cm));
    
    // Overlapping voxels - should collide
    EXPECT_TRUE(VoxelCollision::checkCollision(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm,
        IncrementCoordinates(16, 0, 0), VoxelData::VoxelResolution::Size_32cm));
    
    // Far apart - should not collide
    EXPECT_FALSE(VoxelCollision::checkCollision(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm,
        IncrementCoordinates(100, 100, 100), VoxelData::VoxelResolution::Size_32cm));
}

// Test collision with different sized voxels
TEST_F(VoxelCollisionTest, DifferentSizeCollision) {
    // Large voxel containing small voxel - should collide
    EXPECT_TRUE(VoxelCollision::checkCollision(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_64cm,
        IncrementCoordinates(16, 0, 16), VoxelData::VoxelResolution::Size_16cm));
    
    // Small voxel next to large voxel - should not collide
    EXPECT_FALSE(VoxelCollision::checkCollision(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_64cm,
        IncrementCoordinates(40, 0, 0), VoxelData::VoxelResolution::Size_16cm));
    
    // Multiple small voxels can fit where one large can't
    EXPECT_FALSE(VoxelCollision::checkCollision(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_16cm,
        IncrementCoordinates(16, 0, 0), VoxelData::VoxelResolution::Size_16cm));
}

// Test collision with grid
TEST_F(VoxelCollisionTest, CollisionWithGrid) {
    // Place some voxels in the grid
    grid->setVoxel(IncrementCoordinates(0, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(64, 0, 0), true);
    
    // Test collision at occupied position
    EXPECT_TRUE(VoxelCollision::checkCollisionWithGrid(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm, *grid));
    
    // Test no collision at free position
    EXPECT_FALSE(VoxelCollision::checkCollisionWithGrid(
        IncrementCoordinates(32, 0, 0), VoxelData::VoxelResolution::Size_32cm, *grid));
    
    // Test collision with partial overlap
    EXPECT_TRUE(VoxelCollision::checkCollisionWithGrid(
        IncrementCoordinates(16, 0, 0), VoxelData::VoxelResolution::Size_32cm, *grid));
}

// Test getting colliding voxels
TEST_F(VoxelCollisionTest, GetCollidingVoxels) {
    // Place multiple voxels
    grid->setVoxel(IncrementCoordinates(0, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(16, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(32, 0, 0), true);
    
    // Check for collisions with a voxel that overlaps all three existing voxels
    auto colliding = VoxelCollision::getCollidingVoxels(
        IncrementCoordinates(8, 0, 0), VoxelData::VoxelResolution::Size_32cm, *grid);
    
    EXPECT_EQ(colliding.size(), 3);  // Should collide with all three voxels
}

// Test getting voxels in region
TEST_F(VoxelCollisionTest, GetVoxelsInRegion) {
    // Place voxels at various positions
    grid->setVoxel(IncrementCoordinates(0, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(32, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(64, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(100, 0, 0), true);
    
    // Create a search region - VoxelBounds uses bottom-center position and size
    VoxelBounds region(IncrementCoordinates(50, 0, 50), 1.0f);  // 1m cube centered at (0.5, 0.5, 0.5)
    
    auto voxelsInRegion = VoxelCollision::getVoxelsInRegion(region, *grid);
    
    // Should find all 4 voxels (0, 32, 64, 100 cm positions) - all are within the 100cm region
    EXPECT_EQ(voxelsInRegion.size(), 4);
}

// Test finding nearest free position
TEST_F(VoxelCollisionTest, FindNearestFreePosition) {
    // Fill a small area with voxels
    grid->setVoxel(IncrementCoordinates(0, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(32, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(0, 0, 32), true);
    
    // Try to find free position near occupied area
    auto freePos = VoxelCollision::findNearestFreePosition(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm, *grid);
    
    EXPECT_TRUE(freePos.has_value());
    // Should find a nearby free position
    EXPECT_FALSE(VoxelCollision::checkCollisionWithGrid(
        freePos.value(), VoxelData::VoxelResolution::Size_32cm, *grid));
}

// Test surrounded check
TEST_F(VoxelCollisionTest, IsCompletelySurrounded) {
    // Place a voxel and surround it
    IncrementCoordinates center(32, 32, 32);
    grid->setVoxel(center, true);
    
    // Not surrounded yet
    EXPECT_FALSE(VoxelCollision::isCompletelySurrounded(center, VoxelData::VoxelResolution::Size_32cm, *grid));
    
    // Surround it on all 6 faces
    grid->setVoxel(IncrementCoordinates(64, 32, 32), true);  // +X
    grid->setVoxel(IncrementCoordinates(0, 32, 32), true);   // -X
    grid->setVoxel(IncrementCoordinates(32, 64, 32), true);  // +Y
    grid->setVoxel(IncrementCoordinates(32, 0, 32), true);   // -Y
    grid->setVoxel(IncrementCoordinates(32, 32, 64), true);  // +Z
    grid->setVoxel(IncrementCoordinates(32, 32, 0), true);   // -Z
    
    // Now it should be surrounded
    EXPECT_TRUE(VoxelCollision::isCompletelySurrounded(center, VoxelData::VoxelResolution::Size_32cm, *grid));
}

// Test intersection volume calculation
TEST_F(VoxelCollisionTest, CalculateIntersectionVolume) {
    // Same position, same size - full intersection
    float volume1 = VoxelCollision::calculateIntersectionVolume(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm,
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm);
    
    float expectedVolume = 0.32f * 0.32f * 0.32f;  // 32cm cube
    EXPECT_FLOAT_EQ(volume1, expectedVolume);
    
    // Half overlap
    float volume2 = VoxelCollision::calculateIntersectionVolume(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm,
        IncrementCoordinates(16, 0, 0), VoxelData::VoxelResolution::Size_32cm);
    
    float expectedVolume2 = 0.16f * 0.32f * 0.32f;  // Half width overlap
    EXPECT_FLOAT_EQ(volume2, expectedVolume2);
    
    // No overlap
    float volume3 = VoxelCollision::calculateIntersectionVolume(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm,
        IncrementCoordinates(100, 0, 0), VoxelData::VoxelResolution::Size_32cm);
    
    EXPECT_FLOAT_EQ(volume3, 0.0f);
}

// Test stability check
TEST_F(VoxelCollisionTest, CheckStability) {
    // Voxel on ground is stable
    EXPECT_TRUE(VoxelCollision::checkStability(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm, *grid));
    
    // Floating voxel is not stable
    EXPECT_FALSE(VoxelCollision::checkStability(
        IncrementCoordinates(0, 32, 0), VoxelData::VoxelResolution::Size_32cm, *grid));
    
    // Place support voxel
    grid->setVoxel(IncrementCoordinates(0, 0, 0), true);
    
    // Now voxel above should be stable
    EXPECT_TRUE(VoxelCollision::checkStability(
        IncrementCoordinates(0, 32, 0), VoxelData::VoxelResolution::Size_32cm, *grid));
    
    // Voxel offset from support should not be stable
    EXPECT_FALSE(VoxelCollision::checkStability(
        IncrementCoordinates(64, 32, 0), VoxelData::VoxelResolution::Size_32cm, *grid));
}

// Test edge cases
TEST_F(VoxelCollisionTest, EdgeCases) {
    // Very small voxels
    EXPECT_TRUE(VoxelCollision::checkCollision(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_1cm,
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_1cm));
    
    EXPECT_FALSE(VoxelCollision::checkCollision(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_1cm,
        IncrementCoordinates(1, 0, 0), VoxelData::VoxelResolution::Size_1cm));
    
    // Very large voxels
    EXPECT_TRUE(VoxelCollision::checkCollision(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_512cm,
        IncrementCoordinates(200, 0, 200), VoxelData::VoxelResolution::Size_512cm));
    
    // Empty grid operations
    VoxelData::VoxelGrid emptyGrid(VoxelData::VoxelResolution::Size_32cm, workspaceSize);
    EXPECT_FALSE(VoxelCollision::checkCollisionWithGrid(
        IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_32cm, emptyGrid));
    
    auto emptyRegionVoxels = VoxelCollision::getVoxelsInRegion(
        VoxelBounds(IncrementCoordinates(50, 0, 50), 1.0f), emptyGrid);
    EXPECT_TRUE(emptyRegionVoxels.empty());
}