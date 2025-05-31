#include <gtest/gtest.h>
#include "../VoxelTypes.h"

using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class VoxelTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
    }
    
    Vector3f workspaceSize;
};

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
    EXPECT_EQ(pos1.gridPos, Vector3i(0, 0, 0));
    EXPECT_EQ(pos1.resolution, VoxelResolution::Size_1cm);
    
    VoxelPosition pos2(Vector3i(10, 20, 30), VoxelResolution::Size_4cm);
    EXPECT_EQ(pos2.gridPos, Vector3i(10, 20, 30));
    EXPECT_EQ(pos2.resolution, VoxelResolution::Size_4cm);
    
    VoxelPosition pos3(5, 10, 15, VoxelResolution::Size_8cm);
    EXPECT_EQ(pos3.gridPos, Vector3i(5, 10, 15));
    EXPECT_EQ(pos3.resolution, VoxelResolution::Size_8cm);
}

TEST_F(VoxelTypesTest, WorldSpaceConversion) {
    VoxelPosition voxelPos(Vector3i(10, 10, 10), VoxelResolution::Size_1cm);
    Vector3f worldPos = voxelPos.toWorldSpace(workspaceSize);
    
    // Grid position (10,10,10) with 1cm voxels = (0.1, 0.1, 0.1) world
    // Centered around origin: (0.1, 0.1, 0.1) - (2.5, 2.5, 2.5) = (-2.4, -2.4, -2.4)
    EXPECT_FLOAT_EQ(worldPos.x, -2.4f);
    EXPECT_FLOAT_EQ(worldPos.y, -2.4f);
    EXPECT_FLOAT_EQ(worldPos.z, -2.4f);
    
    // Test round-trip conversion
    VoxelPosition convertedBack = VoxelPosition::fromWorldSpace(worldPos, VoxelResolution::Size_1cm, workspaceSize);
    EXPECT_EQ(convertedBack.gridPos, voxelPos.gridPos);
    EXPECT_EQ(convertedBack.resolution, voxelPos.resolution);
}

TEST_F(VoxelTypesTest, WorldSpaceConversionLargerVoxels) {
    VoxelPosition voxelPos(Vector3i(5, 5, 5), VoxelResolution::Size_4cm);
    Vector3f worldPos = voxelPos.toWorldSpace(workspaceSize);
    
    // Grid position (5,5,5) with 4cm voxels = (0.2, 0.2, 0.2) world
    // Centered: (0.2, 0.2, 0.2) - (2.5, 2.5, 2.5) = (-2.3, -2.3, -2.3)
    EXPECT_FLOAT_EQ(worldPos.x, -2.3f);
    EXPECT_FLOAT_EQ(worldPos.y, -2.3f);
    EXPECT_FLOAT_EQ(worldPos.z, -2.3f);
}

TEST_F(VoxelTypesTest, VoxelBounds) {
    VoxelPosition voxelPos(Vector3i(0, 0, 0), VoxelResolution::Size_2cm);
    Vector3f minBounds, maxBounds;
    voxelPos.getWorldBounds(workspaceSize, minBounds, maxBounds);
    
    // Grid (0,0,0) with 2cm voxels starts at (-2.5, -2.5, -2.5) and extends 0.02m
    EXPECT_FLOAT_EQ(minBounds.x, -2.5f);
    EXPECT_FLOAT_EQ(minBounds.y, -2.5f);
    EXPECT_FLOAT_EQ(minBounds.z, -2.5f);
    EXPECT_FLOAT_EQ(maxBounds.x, -2.48f);
    EXPECT_FLOAT_EQ(maxBounds.y, -2.48f);
    EXPECT_FLOAT_EQ(maxBounds.z, -2.48f);
}

TEST_F(VoxelTypesTest, VoxelPositionEquality) {
    VoxelPosition pos1(Vector3i(10, 20, 30), VoxelResolution::Size_4cm);
    VoxelPosition pos2(Vector3i(10, 20, 30), VoxelResolution::Size_4cm);
    VoxelPosition pos3(Vector3i(10, 20, 31), VoxelResolution::Size_4cm);
    VoxelPosition pos4(Vector3i(10, 20, 30), VoxelResolution::Size_8cm);
    
    EXPECT_EQ(pos1, pos2);
    EXPECT_NE(pos1, pos3);
    EXPECT_NE(pos1, pos4);
}

TEST_F(VoxelTypesTest, VoxelPositionHash) {
    VoxelPosition pos1(Vector3i(10, 20, 30), VoxelResolution::Size_4cm);
    VoxelPosition pos2(Vector3i(10, 20, 30), VoxelResolution::Size_4cm);
    VoxelPosition pos3(Vector3i(10, 20, 31), VoxelResolution::Size_4cm);
    
    VoxelPosition::Hash hasher;
    
    EXPECT_EQ(hasher(pos1), hasher(pos2));
    EXPECT_NE(hasher(pos1), hasher(pos3));
}

TEST_F(VoxelTypesTest, WorkspaceConstraints) {
    EXPECT_TRUE(WorkspaceConstraints::isValidSize(Vector3f(3.0f, 3.0f, 3.0f)));
    EXPECT_TRUE(WorkspaceConstraints::isValidSize(5.0f));
    EXPECT_FALSE(WorkspaceConstraints::isValidSize(Vector3f(1.0f, 3.0f, 3.0f)));
    EXPECT_FALSE(WorkspaceConstraints::isValidSize(Vector3f(3.0f, 3.0f, 10.0f)));
    EXPECT_FALSE(WorkspaceConstraints::isValidSize(1.0f));
    EXPECT_FALSE(WorkspaceConstraints::isValidSize(10.0f));
    
    Vector3f clamped = WorkspaceConstraints::clampSize(Vector3f(1.0f, 5.0f, 10.0f));
    EXPECT_FLOAT_EQ(clamped.x, 2.0f);
    EXPECT_FLOAT_EQ(clamped.y, 5.0f);
    EXPECT_FLOAT_EQ(clamped.z, 8.0f);
}

TEST_F(VoxelTypesTest, GridDimensionCalculation) {
    Vector3i dims1cm = calculateMaxGridDimensions(VoxelResolution::Size_1cm, workspaceSize);
    EXPECT_EQ(dims1cm.x, 500); // 5.0m / 0.01m = 500
    EXPECT_EQ(dims1cm.y, 500);
    EXPECT_EQ(dims1cm.z, 500);
    
    Vector3i dims4cm = calculateMaxGridDimensions(VoxelResolution::Size_4cm, workspaceSize);
    EXPECT_EQ(dims4cm.x, 125); // 5.0m / 0.04m = 125
    EXPECT_EQ(dims4cm.y, 125);
    EXPECT_EQ(dims4cm.z, 125);
}

TEST_F(VoxelTypesTest, PositionBoundsChecking) {
    Vector3i validPos(10, 10, 10);
    Vector3i invalidPos(1000, 10, 10);
    
    EXPECT_TRUE(isPositionInBounds(validPos, VoxelResolution::Size_1cm, workspaceSize));
    EXPECT_FALSE(isPositionInBounds(invalidPos, VoxelResolution::Size_1cm, workspaceSize));
    
    // Test edge cases
    Vector3i maxValid(499, 499, 499); // Just within bounds for 1cm voxels
    Vector3i justOutside(500, 499, 499); // Just outside bounds
    
    EXPECT_TRUE(isPositionInBounds(maxValid, VoxelResolution::Size_1cm, workspaceSize));
    EXPECT_FALSE(isPositionInBounds(justOutside, VoxelResolution::Size_1cm, workspaceSize));
}