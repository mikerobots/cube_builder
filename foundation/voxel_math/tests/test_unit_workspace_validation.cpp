#include <gtest/gtest.h>
#include "../include/voxel_math/WorkspaceValidation.h"
#include "../include/voxel_math/VoxelGridMath.h"
#include "../../math/CoordinateConverter.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;

class WorkspaceValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a default 5m workspace
        defaultBounds = WorkspaceValidation::createBounds(Vector3f(5.0f, 5.0f, 5.0f));
    }
    
    WorkspaceValidation::WorkspaceBounds defaultBounds;
};

// Test workspace bounds creation
TEST_F(WorkspaceValidationTest, CreateBounds) {
    Vector3f workspaceSize(4.0f, 3.0f, 2.0f);
    auto bounds = WorkspaceValidation::createBounds(workspaceSize);
    
    // Check size
    EXPECT_FLOAT_EQ(bounds.size.x, 4.0f);
    EXPECT_FLOAT_EQ(bounds.size.y, 3.0f);
    EXPECT_FLOAT_EQ(bounds.size.z, 2.0f);
    
    // Check world bounds (centered at origin)
    EXPECT_FLOAT_EQ(bounds.minWorld.value().x, -2.0f);
    EXPECT_FLOAT_EQ(bounds.minWorld.value().y, 0.0f);
    EXPECT_FLOAT_EQ(bounds.minWorld.value().z, -1.0f);
    
    EXPECT_FLOAT_EQ(bounds.maxWorld.value().x, 2.0f);
    EXPECT_FLOAT_EQ(bounds.maxWorld.value().y, 3.0f);
    EXPECT_FLOAT_EQ(bounds.maxWorld.value().z, 1.0f);
    
    // Check increment bounds
    EXPECT_EQ(bounds.minIncrement.x(), -200);
    EXPECT_EQ(bounds.minIncrement.y(), 0);
    EXPECT_EQ(bounds.minIncrement.z(), -100);
    
    EXPECT_EQ(bounds.maxIncrement.x(), 200);
    EXPECT_EQ(bounds.maxIncrement.y(), 300);
    EXPECT_EQ(bounds.maxIncrement.z(), 100);
}

// Test increment coordinate bounds checking
TEST_F(WorkspaceValidationTest, IsInBoundsIncrement) {
    // Test points inside
    EXPECT_TRUE(WorkspaceValidation::isInBounds(IncrementCoordinates(0, 0, 0), defaultBounds));
    EXPECT_TRUE(WorkspaceValidation::isInBounds(IncrementCoordinates(100, 100, 100), defaultBounds));
    EXPECT_TRUE(WorkspaceValidation::isInBounds(IncrementCoordinates(-100, 100, -100), defaultBounds));
    
    // Test points on boundary
    EXPECT_TRUE(WorkspaceValidation::isInBounds(IncrementCoordinates(-250, 0, -250), defaultBounds));
    EXPECT_TRUE(WorkspaceValidation::isInBounds(IncrementCoordinates(250, 500, 250), defaultBounds));
    
    // Test points outside
    EXPECT_FALSE(WorkspaceValidation::isInBounds(IncrementCoordinates(-251, 0, 0), defaultBounds));
    EXPECT_FALSE(WorkspaceValidation::isInBounds(IncrementCoordinates(251, 0, 0), defaultBounds));
    EXPECT_FALSE(WorkspaceValidation::isInBounds(IncrementCoordinates(0, -1, 0), defaultBounds));
    EXPECT_FALSE(WorkspaceValidation::isInBounds(IncrementCoordinates(0, 501, 0), defaultBounds));
}

// Test world coordinate bounds checking
TEST_F(WorkspaceValidationTest, IsInBoundsWorld) {
    // Test points inside
    EXPECT_TRUE(WorkspaceValidation::isInBounds(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)), defaultBounds));
    EXPECT_TRUE(WorkspaceValidation::isInBounds(WorldCoordinates(Vector3f(1.0f, 1.0f, 1.0f)), defaultBounds));
    
    // Test points on boundary
    EXPECT_TRUE(WorkspaceValidation::isInBounds(WorldCoordinates(Vector3f(-2.5f, 0.0f, -2.5f)), defaultBounds));
    EXPECT_TRUE(WorkspaceValidation::isInBounds(WorldCoordinates(Vector3f(2.5f, 5.0f, 2.5f)), defaultBounds));
    
    // Test points outside
    EXPECT_FALSE(WorkspaceValidation::isInBounds(WorldCoordinates(Vector3f(-2.51f, 0.0f, 0.0f)), defaultBounds));
    EXPECT_FALSE(WorkspaceValidation::isInBounds(WorldCoordinates(Vector3f(0.0f, 5.01f, 0.0f)), defaultBounds));
}

// Test voxel fitting in bounds
TEST_F(WorkspaceValidationTest, VoxelFitsInBounds) {
    // Test 32cm voxel at origin - should fit
    EXPECT_TRUE(WorkspaceValidation::voxelFitsInBounds(
        IncrementCoordinates(0, 0, 0), 
        VoxelData::VoxelResolution::Size_32cm, 
        defaultBounds));
    
    // Test voxel near edge - should fit
    EXPECT_TRUE(WorkspaceValidation::voxelFitsInBounds(
        IncrementCoordinates(234, 0, 234),  // 250 - 16 = 234 max for 32cm voxel
        VoxelData::VoxelResolution::Size_32cm, 
        defaultBounds));
    
    // Test voxel extending past edge - should not fit
    EXPECT_FALSE(WorkspaceValidation::voxelFitsInBounds(
        IncrementCoordinates(240, 0, 0),  // Would extend past 250
        VoxelData::VoxelResolution::Size_32cm, 
        defaultBounds));
    
    // Test voxel extending past top - should not fit
    EXPECT_FALSE(WorkspaceValidation::voxelFitsInBounds(
        IncrementCoordinates(0, 490, 0),  // 490 + 32 = 522 > 500
        VoxelData::VoxelResolution::Size_32cm, 
        defaultBounds));
    
    // Test large voxel (512cm = 5.12m) - cannot fit in 5m workspace
    EXPECT_FALSE(WorkspaceValidation::voxelFitsInBounds(
        IncrementCoordinates(0, 0, 0), 
        VoxelData::VoxelResolution::Size_512cm, 
        defaultBounds));
    
    // Test 256cm voxel (2.56m) - should fit at origin
    EXPECT_TRUE(WorkspaceValidation::voxelFitsInBounds(
        IncrementCoordinates(0, 0, 0), 
        VoxelData::VoxelResolution::Size_256cm, 
        defaultBounds));
}

// Test position clamping
TEST_F(WorkspaceValidationTest, ClampToBounds) {
    // Test position already in bounds
    IncrementCoordinates inBounds(100, 100, 100);
    EXPECT_EQ(WorkspaceValidation::clampToBounds(inBounds, defaultBounds), inBounds);
    
    // Test clamping X
    IncrementCoordinates beyondX(300, 100, 100);
    auto clampedX = WorkspaceValidation::clampToBounds(beyondX, defaultBounds);
    EXPECT_EQ(clampedX.x(), 250);
    EXPECT_EQ(clampedX.y(), 100);
    EXPECT_EQ(clampedX.z(), 100);
    
    // Test clamping Y
    IncrementCoordinates beyondY(100, 600, 100);
    auto clampedY = WorkspaceValidation::clampToBounds(beyondY, defaultBounds);
    EXPECT_EQ(clampedY.x(), 100);
    EXPECT_EQ(clampedY.y(), 500);
    EXPECT_EQ(clampedY.z(), 100);
    
    // Test clamping multiple axes
    IncrementCoordinates beyondAll(-300, -100, 300);
    auto clampedAll = WorkspaceValidation::clampToBounds(beyondAll, defaultBounds);
    EXPECT_EQ(clampedAll.x(), -250);
    EXPECT_EQ(clampedAll.y(), 0);
    EXPECT_EQ(clampedAll.z(), 250);
}

// Test ground plane checks
TEST_F(WorkspaceValidationTest, GroundPlaneOperations) {
    // Test above ground
    EXPECT_TRUE(WorkspaceValidation::isAboveGroundPlane(IncrementCoordinates(0, 0, 0)));
    EXPECT_TRUE(WorkspaceValidation::isAboveGroundPlane(IncrementCoordinates(0, 100, 0)));
    
    // Test below ground
    EXPECT_FALSE(WorkspaceValidation::isAboveGroundPlane(IncrementCoordinates(0, -1, 0)));
    EXPECT_FALSE(WorkspaceValidation::isAboveGroundPlane(IncrementCoordinates(0, -100, 0)));
    
    // Test clamping to ground
    IncrementCoordinates belowGround(100, -50, 100);
    auto clamped = WorkspaceValidation::clampToGroundPlane(belowGround);
    EXPECT_EQ(clamped.x(), 100);
    EXPECT_EQ(clamped.y(), 0);
    EXPECT_EQ(clamped.z(), 100);
    
    IncrementCoordinates aboveGround(100, 50, 100);
    EXPECT_EQ(WorkspaceValidation::clampToGroundPlane(aboveGround), aboveGround);
}

// Test maximum fitting voxel size
TEST_F(WorkspaceValidationTest, GetMaxFittingVoxelSize) {
    // At origin, largest voxel that fits is 256cm (2.56m < 5m workspace)
    auto maxSize = WorkspaceValidation::getMaxFittingVoxelSize(IncrementCoordinates(0, 0, 0), defaultBounds);
    EXPECT_TRUE(maxSize.has_value());
    EXPECT_EQ(maxSize.value(), VoxelData::VoxelResolution::Size_256cm);
    
    // Near edge, only smaller voxels fit
    auto edgeSize = WorkspaceValidation::getMaxFittingVoxelSize(IncrementCoordinates(240, 0, 0), defaultBounds);
    EXPECT_TRUE(edgeSize.has_value());
    EXPECT_EQ(edgeSize.value(), VoxelData::VoxelResolution::Size_16cm);  // 240 + 8 = 248 <= 250
    
    // At the very edge, only 1cm voxels fit
    auto cornerSize = WorkspaceValidation::getMaxFittingVoxelSize(IncrementCoordinates(250, 0, 250), defaultBounds);
    EXPECT_TRUE(cornerSize.has_value());
    EXPECT_EQ(cornerSize.value(), VoxelData::VoxelResolution::Size_1cm);
    
    // Outside bounds, nothing fits
    auto outsideSize = WorkspaceValidation::getMaxFittingVoxelSize(IncrementCoordinates(300, 0, 0), defaultBounds);
    EXPECT_FALSE(outsideSize.has_value());
}

// Test overhang calculation
TEST_F(WorkspaceValidationTest, CalculateOverhang) {
    // No overhang
    auto noOverhang = WorkspaceValidation::calculateOverhang(
        IncrementCoordinates(0, 0, 0),
        VoxelData::VoxelResolution::Size_32cm,
        defaultBounds);
    EXPECT_FALSE(noOverhang.hasOverhang());
    
    // X axis overhang
    auto xOverhang = WorkspaceValidation::calculateOverhang(
        IncrementCoordinates(240, 0, 0),  // 240 + 16 = 256 > 250
        VoxelData::VoxelResolution::Size_32cm,
        defaultBounds);
    EXPECT_TRUE(xOverhang.hasOverhang());
    EXPECT_EQ(xOverhang.maxX, 6);  // 256 - 250 = 6
    EXPECT_EQ(xOverhang.minX, 0);
    
    // Y axis overhang (top)
    auto yOverhang = WorkspaceValidation::calculateOverhang(
        IncrementCoordinates(0, 490, 0),  // 490 + 32 = 522 > 500
        VoxelData::VoxelResolution::Size_32cm,
        defaultBounds);
    EXPECT_TRUE(yOverhang.hasOverhang());
    EXPECT_EQ(yOverhang.maxY, 22);  // 522 - 500 = 22
    
    // Below ground overhang
    auto belowGround = WorkspaceValidation::calculateOverhang(
        IncrementCoordinates(0, -10, 0),
        VoxelData::VoxelResolution::Size_32cm,
        defaultBounds);
    EXPECT_TRUE(belowGround.hasOverhang());
    EXPECT_EQ(belowGround.minY, 10);
}

// Test finding nearest valid position
TEST_F(WorkspaceValidationTest, FindNearestValidPosition) {
    // Already valid position
    IncrementCoordinates validPos(0, 0, 0);
    auto nearest = WorkspaceValidation::findNearestValidPosition(
        validPos, VoxelData::VoxelResolution::Size_32cm, defaultBounds);
    EXPECT_EQ(nearest, validPos);
    
    // Position with X overhang
    IncrementCoordinates xOver(245, 0, 0);
    auto xAdjusted = WorkspaceValidation::findNearestValidPosition(
        xOver, VoxelData::VoxelResolution::Size_32cm, defaultBounds);
    EXPECT_EQ(xAdjusted.x(), 234);  // 250 - 16 = 234
    
    // Position below ground
    IncrementCoordinates belowGround(0, -50, 0);
    auto groundAdjusted = WorkspaceValidation::findNearestValidPosition(
        belowGround, VoxelData::VoxelResolution::Size_32cm, defaultBounds);
    EXPECT_EQ(groundAdjusted.y(), 0);
    
    // Position with multiple overhangs
    IncrementCoordinates multiOver(245, 490, 245);
    auto multiAdjusted = WorkspaceValidation::findNearestValidPosition(
        multiOver, VoxelData::VoxelResolution::Size_32cm, defaultBounds);
    EXPECT_EQ(multiAdjusted.x(), 234);  // Adjusted for X overhang
    EXPECT_EQ(multiAdjusted.y(), 468);  // 500 - 32 = 468
    EXPECT_EQ(multiAdjusted.z(), 234);  // Adjusted for Z overhang
}

// Test workspace size validation
TEST_F(WorkspaceValidationTest, IsValidWorkspaceSize) {
    // Valid sizes
    EXPECT_TRUE(WorkspaceValidation::isValidWorkspaceSize(Vector3f(2.0f, 2.0f, 2.0f)));
    EXPECT_TRUE(WorkspaceValidation::isValidWorkspaceSize(Vector3f(5.0f, 5.0f, 5.0f)));
    EXPECT_TRUE(WorkspaceValidation::isValidWorkspaceSize(Vector3f(8.0f, 8.0f, 8.0f)));
    
    // Too small
    EXPECT_FALSE(WorkspaceValidation::isValidWorkspaceSize(Vector3f(1.9f, 2.0f, 2.0f)));
    EXPECT_FALSE(WorkspaceValidation::isValidWorkspaceSize(Vector3f(2.0f, 1.9f, 2.0f)));
    
    // Too large
    EXPECT_FALSE(WorkspaceValidation::isValidWorkspaceSize(Vector3f(8.1f, 8.0f, 8.0f)));
    EXPECT_FALSE(WorkspaceValidation::isValidWorkspaceSize(Vector3f(8.0f, 8.0f, 8.1f)));
    
    // Mixed valid/invalid
    EXPECT_FALSE(WorkspaceValidation::isValidWorkspaceSize(Vector3f(5.0f, 1.0f, 5.0f)));
}