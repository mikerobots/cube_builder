#include <gtest/gtest.h>
#include "../VoxelTypes.h"

using namespace VoxelEditor::VoxelData;
// Don't use Math namespace globally to avoid VoxelResolution ambiguity
namespace Math = VoxelEditor::Math;

class VoxelTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        workspaceSize = Math::Vector3f(5.0f, 5.0f, 5.0f);
    }
    
    Math::Vector3f workspaceSize;
};

// REQ-5.3.3: Available resolutions: 1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm
TEST_F(VoxelTypesTest, VoxelResolutionValues) {
    EXPECT_FLOAT_EQ(getVoxelSize(VoxelResolution::Size_1cm), 0.01f);
    EXPECT_FLOAT_EQ(getVoxelSize(VoxelResolution::Size_2cm), 0.02f);
    EXPECT_FLOAT_EQ(getVoxelSize(VoxelResolution::Size_4cm), 0.04f);
    EXPECT_FLOAT_EQ(getVoxelSize(VoxelResolution::Size_512cm), 5.12f);
}

TEST_F(VoxelTypesTest, VoxelResolutionNames) {
    EXPECT_STREQ(getVoxelSizeName(VoxelResolution::Size_1cm), "1cm");
    EXPECT_STREQ(getVoxelSizeName(VoxelResolution::Size_16cm), "16cm");
    EXPECT_STREQ(getVoxelSizeName(VoxelResolution::Size_512cm), "512cm");
}

TEST_F(VoxelTypesTest, ResolutionValidation) {
    EXPECT_TRUE(isValidResolution(0));
    EXPECT_TRUE(isValidResolution(9));
    EXPECT_FALSE(isValidResolution(-1));
    EXPECT_FALSE(isValidResolution(10));
}

TEST_F(VoxelTypesTest, VoxelPositionConstruction) {
    VoxelPosition pos1;
    EXPECT_EQ(pos1.incrementPos.value(), Math::Vector3i(0, 0, 0));
    EXPECT_EQ(pos1.resolution, VoxelResolution::Size_1cm);
    
    VoxelPosition pos2(Math::Vector3i(10, 20, 30), VoxelResolution::Size_4cm);
    EXPECT_EQ(pos2.incrementPos.value(), Math::Vector3i(10, 20, 30));
    EXPECT_EQ(pos2.resolution, VoxelResolution::Size_4cm);
    
    VoxelPosition pos3(5, 10, 15, VoxelResolution::Size_8cm);
    EXPECT_EQ(pos3.incrementPos.value(), Math::Vector3i(5, 10, 15));
    EXPECT_EQ(pos3.resolution, VoxelResolution::Size_8cm);
}

// REQ-2.2.4: All voxel sizes (1cm to 512cm) shall be placeable at any valid 1cm increment position on the ground plane
TEST_F(VoxelTypesTest, WorldSpaceConversion) {
    VoxelPosition voxelPos(Math::Vector3i(10, 10, 10), VoxelResolution::Size_1cm);
    Math::Vector3f worldPos = voxelPos.toWorldSpace();
    
    // Grid position (10,10,10) with 1cm voxels = (0.1, 0.1, 0.1) world (0-based)
    EXPECT_FLOAT_EQ(worldPos.x, 0.1f);
    EXPECT_FLOAT_EQ(worldPos.y, 0.1f);
    EXPECT_FLOAT_EQ(worldPos.z, 0.1f);
    
    // Test round-trip conversion
    VoxelPosition convertedBack = VoxelPosition::fromWorldSpace(worldPos, VoxelResolution::Size_1cm);
    EXPECT_EQ(convertedBack.incrementPos, voxelPos.incrementPos);
    EXPECT_EQ(convertedBack.resolution, voxelPos.resolution);
}

TEST_F(VoxelTypesTest, WorldSpaceConversionLargerVoxels) {
    VoxelPosition voxelPos(Math::Vector3i(5, 5, 5), VoxelResolution::Size_4cm);
    Math::Vector3f worldPos = voxelPos.toWorldSpace();
    
    // Increment position (5,5,5) = (0.05, 0.05, 0.05) world (centered coordinate system)
    EXPECT_FLOAT_EQ(worldPos.x, 0.05f);
    EXPECT_FLOAT_EQ(worldPos.y, 0.05f);
    EXPECT_FLOAT_EQ(worldPos.z, 0.05f);
}

TEST_F(VoxelTypesTest, VoxelBounds) {
    VoxelPosition voxelPos(Math::Vector3i(0, 0, 0), VoxelResolution::Size_2cm);
    Math::Vector3f minBounds, maxBounds;
    voxelPos.getWorldBounds(minBounds, maxBounds);
    
    // Grid (0,0,0) with 2cm voxels starts at (0, 0, 0) and extends 0.02m (0-based)
    EXPECT_FLOAT_EQ(minBounds.x, 0.0f);
    EXPECT_FLOAT_EQ(minBounds.y, 0.0f);
    EXPECT_FLOAT_EQ(minBounds.z, 0.0f);
    EXPECT_FLOAT_EQ(maxBounds.x, 0.02f);
    EXPECT_FLOAT_EQ(maxBounds.y, 0.02f);
    EXPECT_FLOAT_EQ(maxBounds.z, 0.02f);
}

TEST_F(VoxelTypesTest, VoxelPositionEquality) {
    VoxelPosition pos1(Math::Vector3i(10, 20, 30), VoxelResolution::Size_4cm);
    VoxelPosition pos2(Math::Vector3i(10, 20, 30), VoxelResolution::Size_4cm);
    VoxelPosition pos3(Math::Vector3i(10, 20, 31), VoxelResolution::Size_4cm);
    VoxelPosition pos4(Math::Vector3i(10, 20, 30), VoxelResolution::Size_8cm);
    
    EXPECT_EQ(pos1, pos2);
    EXPECT_NE(pos1, pos3);
    EXPECT_NE(pos1, pos4);
}

TEST_F(VoxelTypesTest, VoxelPositionHash) {
    VoxelPosition pos1(Math::Vector3i(10, 20, 30), VoxelResolution::Size_4cm);
    VoxelPosition pos2(Math::Vector3i(10, 20, 30), VoxelResolution::Size_4cm);
    VoxelPosition pos3(Math::Vector3i(10, 20, 31), VoxelResolution::Size_4cm);
    
    VoxelPosition::Hash hasher;
    
    EXPECT_EQ(hasher(pos1), hasher(pos2));
    EXPECT_NE(hasher(pos1), hasher(pos3));
}

TEST_F(VoxelTypesTest, WorkspaceConstraints) {
    EXPECT_TRUE(WorkspaceConstraints::isValidSize(Math::Vector3f(3.0f, 3.0f, 3.0f)));
    EXPECT_TRUE(WorkspaceConstraints::isValidSize(5.0f));
    EXPECT_FALSE(WorkspaceConstraints::isValidSize(Math::Vector3f(1.0f, 3.0f, 3.0f)));
    EXPECT_FALSE(WorkspaceConstraints::isValidSize(Math::Vector3f(3.0f, 3.0f, 10.0f)));
    EXPECT_FALSE(WorkspaceConstraints::isValidSize(1.0f));
    EXPECT_FALSE(WorkspaceConstraints::isValidSize(10.0f));
    
    Math::Vector3f clamped = WorkspaceConstraints::clampSize(Math::Vector3f(1.0f, 5.0f, 10.0f));
    EXPECT_FLOAT_EQ(clamped.x, 2.0f);
    EXPECT_FLOAT_EQ(clamped.y, 5.0f);
    EXPECT_FLOAT_EQ(clamped.z, 8.0f);
}

// REQ-6.2.2: Grid size shall scale with workspace (up to 8m x 8m)
TEST_F(VoxelTypesTest, GridDimensionCalculation) {
    Math::Vector3i dims1cm = calculateMaxGridDimensions(VoxelResolution::Size_1cm, workspaceSize);
    EXPECT_EQ(dims1cm.x, 500); // 5.0m / 0.01m = 500
    EXPECT_EQ(dims1cm.y, 500);
    EXPECT_EQ(dims1cm.z, 500);
    
    Math::Vector3i dims4cm = calculateMaxGridDimensions(VoxelResolution::Size_4cm, workspaceSize);
    EXPECT_EQ(dims4cm.x, 125); // 5.0m / 0.04m = 125
    EXPECT_EQ(dims4cm.y, 125);
    EXPECT_EQ(dims4cm.z, 125);
}

TEST_F(VoxelTypesTest, Resolution8cmValidation) {
    // Specifically test 8cm voxels as mentioned in TODO
    EXPECT_FLOAT_EQ(getVoxelSize(VoxelResolution::Size_8cm), 0.08f);
    
    // Test grid dimensions for 8cm voxels
    Math::Vector3i dims8cm = calculateMaxGridDimensions(VoxelResolution::Size_8cm, workspaceSize);
    EXPECT_EQ(dims8cm.x, 63); // 5.0m / 0.08m = 62.5, rounded up to 63
    EXPECT_EQ(dims8cm.y, 63);
    EXPECT_EQ(dims8cm.z, 63);
    
    // Test world position conversion for 8cm voxels
    VoxelPosition voxelPos(Math::Vector3i(2, 2, 2), VoxelResolution::Size_8cm);
    Math::Vector3f worldPos = voxelPos.toWorldSpace();
    EXPECT_FLOAT_EQ(worldPos.x, 0.02f); // 2cm in increment coords = 0.02m world
    EXPECT_FLOAT_EQ(worldPos.y, 0.02f);
    EXPECT_FLOAT_EQ(worldPos.z, 0.02f);
    
    // Test bounds for 8cm voxel
    Math::Vector3f minBounds, maxBounds;
    voxelPos.getWorldBounds(minBounds, maxBounds);
    EXPECT_FLOAT_EQ(minBounds.x, 0.02f);
    EXPECT_FLOAT_EQ(minBounds.y, 0.02f);
    EXPECT_FLOAT_EQ(minBounds.z, 0.02f);
    EXPECT_FLOAT_EQ(maxBounds.x, 0.10f); // 0.02 + 0.08
    EXPECT_FLOAT_EQ(maxBounds.y, 0.10f);
    EXPECT_FLOAT_EQ(maxBounds.z, 0.10f);
}

TEST_F(VoxelTypesTest, PositionBoundsChecking) {
    Math::Vector3i validPos(10, 10, 10);  // 10cm from origin
    Math::Vector3i invalidPos(260, 10, 10);  // 2.6m from origin, outside 2.5m X boundary in 5m workspace
    
    EXPECT_TRUE(isPositionInBounds(validPos, workspaceSize));
    EXPECT_FALSE(isPositionInBounds(invalidPos, workspaceSize));
    
    // Test edge cases for 5m workspace (centered at origin)
    // Workspace: X[-2.5m, 2.5m], Y[0m, 5m], Z[-2.5m, 2.5m]
    // In centimeters: X[-250cm, 250cm], Y[0cm, 500cm], Z[-250cm, 250cm]
    Math::Vector3i maxValidPos(250, 500, 250);     // At boundary
    Math::Vector3i justOutsideX(251, 100, 100);   // Just outside X boundary
    Math::Vector3i justOutsideY(100, 501, 100);   // Just outside Y boundary
    Math::Vector3i belowGround(100, -1, 100);     // Below ground plane
    
    EXPECT_TRUE(isPositionInBounds(maxValidPos, workspaceSize));
    EXPECT_FALSE(isPositionInBounds(justOutsideX, workspaceSize));
    EXPECT_FALSE(isPositionInBounds(justOutsideY, workspaceSize));
    EXPECT_FALSE(isPositionInBounds(belowGround, workspaceSize));
}