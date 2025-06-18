#include <gtest/gtest.h>
#include "../CoordinateConverter.h"
#include <cmath>

using namespace VoxelEditor::Math;

class CoordinateConverterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Standard test workspace: 5m x 5m x 5m centered at origin
        m_workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
    }
    
    void TearDown() override {}
    
    Vector3f m_workspaceSize;
    
    // Helper function to check floating point equality with tolerance
    void ExpectNearVector3f(const Vector3f& expected, const Vector3f& actual, float tolerance = 1e-5f) {
        EXPECT_NEAR(expected.x, actual.x, tolerance);
        EXPECT_NEAR(expected.y, actual.y, tolerance);
        EXPECT_NEAR(expected.z, actual.z, tolerance);
    }
};

// ==================== Resolution and Voxel Size Tests ====================

TEST_F(CoordinateConverterTest, GetVoxelSizeMeters_AllResolutions) {
    EXPECT_FLOAT_EQ(CoordinateConverter::getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution::Size_1cm), 0.01f);
    EXPECT_FLOAT_EQ(CoordinateConverter::getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution::Size_2cm), 0.02f);
    EXPECT_FLOAT_EQ(CoordinateConverter::getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution::Size_4cm), 0.04f);
    EXPECT_FLOAT_EQ(CoordinateConverter::getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution::Size_8cm), 0.08f);
    EXPECT_FLOAT_EQ(CoordinateConverter::getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution::Size_16cm), 0.16f);
    EXPECT_FLOAT_EQ(CoordinateConverter::getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution::Size_32cm), 0.32f);
    EXPECT_FLOAT_EQ(CoordinateConverter::getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution::Size_64cm), 0.64f);
    EXPECT_FLOAT_EQ(CoordinateConverter::getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution::Size_128cm), 1.28f);
    EXPECT_FLOAT_EQ(CoordinateConverter::getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution::Size_256cm), 2.56f);
    EXPECT_FLOAT_EQ(CoordinateConverter::getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution::Size_512cm), 5.12f);
}

TEST_F(CoordinateConverterTest, GetWorkspaceBoundsIncrement_CenteredBounds) {
    // For 5x5x5m workspace
    auto bounds = CoordinateConverter::getWorkspaceBoundsIncrement(m_workspaceSize);
    IncrementCoordinates minBounds = bounds.first;
    IncrementCoordinates maxBounds = bounds.second;
    
    // Expected bounds: X[-250cm, 250cm], Y[0cm, 500cm], Z[-250cm, 250cm]
    EXPECT_EQ(minBounds.x(), -250);
    EXPECT_EQ(minBounds.y(), 0);
    EXPECT_EQ(minBounds.z(), -250);
    
    EXPECT_EQ(maxBounds.x(), 250);
    EXPECT_EQ(maxBounds.y(), 500);
    EXPECT_EQ(maxBounds.z(), 250);
}

// ==================== World ↔ Increment Conversion Tests ====================

TEST_F(CoordinateConverterTest, WorldToIncrement_CenteredConversion) {
    // Test conversion with centered coordinate system
    WorldCoordinates world(1.23f, 4.56f, -2.34f);
    IncrementCoordinates increment = CoordinateConverter::worldToIncrement(world);
    
    // 1.23m = 123cm, 4.56m = 456cm, -2.34m = -234cm
    EXPECT_EQ(increment.x(), 123);
    EXPECT_EQ(increment.y(), 456);
    EXPECT_EQ(increment.z(), -234);
}

TEST_F(CoordinateConverterTest, IncrementToWorld_CenteredConversion) {
    // Test conversion with centered coordinate system
    IncrementCoordinates increment(123, 456, -234);
    WorldCoordinates world = CoordinateConverter::incrementToWorld(increment);
    
    EXPECT_FLOAT_EQ(world.x(), 1.23f);
    EXPECT_FLOAT_EQ(world.y(), 4.56f);
    EXPECT_FLOAT_EQ(world.z(), -2.34f);
}

TEST_F(CoordinateConverterTest, WorldIncrement_RoundTripConversion) {
    // Test round-trip conversion preserves values
    WorldCoordinates original(1.23f, 4.56f, -2.34f);
    IncrementCoordinates increment = CoordinateConverter::worldToIncrement(original);
    WorldCoordinates roundTrip = CoordinateConverter::incrementToWorld(increment);
    
    ExpectNearVector3f(original.value(), roundTrip.value(), 1e-5f);
}

TEST_F(CoordinateConverterTest, WorldToIncrement_Rounding) {
    // Test rounding behavior for sub-centimeter values
    WorldCoordinates world1(0.004f, 0.0f, 0.0f);  // Should round to 0
    IncrementCoordinates increment1 = CoordinateConverter::worldToIncrement(world1);
    EXPECT_EQ(increment1.x(), 0);
    
    WorldCoordinates world2(0.006f, 0.0f, 0.0f);  // Should round to 1
    IncrementCoordinates increment2 = CoordinateConverter::worldToIncrement(world2);
    EXPECT_EQ(increment2.x(), 1);
    
    WorldCoordinates world3(-0.006f, 0.0f, 0.0f); // Should round to -1
    IncrementCoordinates increment3 = CoordinateConverter::worldToIncrement(world3);
    EXPECT_EQ(increment3.x(), -1);
}

TEST_F(CoordinateConverterTest, WorldToIncrement_CenterPositions) {
    // Test center of workspace
    WorldCoordinates center(0.0f, 2.5f, 0.0f);
    IncrementCoordinates centerIncrement = CoordinateConverter::worldToIncrement(center);
    
    EXPECT_EQ(centerIncrement.x(), 0);
    EXPECT_EQ(centerIncrement.y(), 250);
    EXPECT_EQ(centerIncrement.z(), 0);
    
    // Test workspace corners
    WorldCoordinates corner1(-2.5f, 0.0f, -2.5f);
    IncrementCoordinates corner1Increment = CoordinateConverter::worldToIncrement(corner1);
    
    EXPECT_EQ(corner1Increment.x(), -250);
    EXPECT_EQ(corner1Increment.y(), 0);
    EXPECT_EQ(corner1Increment.z(), -250);
    
    WorldCoordinates corner2(2.5f, 5.0f, 2.5f);
    IncrementCoordinates corner2Increment = CoordinateConverter::worldToIncrement(corner2);
    
    EXPECT_EQ(corner2Increment.x(), 250);
    EXPECT_EQ(corner2Increment.y(), 500);
    EXPECT_EQ(corner2Increment.z(), 250);
}

// ==================== Validation Tests ====================

TEST_F(CoordinateConverterTest, IsValidIncrementCoordinate_ValidPositions) {
    // Test valid positions within workspace bounds
    IncrementCoordinates center(0, 250, 0);
    EXPECT_TRUE(CoordinateConverter::isValidIncrementCoordinate(center, m_workspaceSize));
    
    IncrementCoordinates corner1(-250, 0, -250);
    EXPECT_TRUE(CoordinateConverter::isValidIncrementCoordinate(corner1, m_workspaceSize));
    
    IncrementCoordinates corner2(250, 500, 250);
    EXPECT_TRUE(CoordinateConverter::isValidIncrementCoordinate(corner2, m_workspaceSize));
    
    IncrementCoordinates nearEdge(249, 499, 249);
    EXPECT_TRUE(CoordinateConverter::isValidIncrementCoordinate(nearEdge, m_workspaceSize));
}

TEST_F(CoordinateConverterTest, IsValidIncrementCoordinate_InvalidPositions) {
    // Test invalid positions beyond workspace bounds
    IncrementCoordinates beyondX(251, 250, 0);
    EXPECT_FALSE(CoordinateConverter::isValidIncrementCoordinate(beyondX, m_workspaceSize));
    
    IncrementCoordinates belowX(-251, 250, 0);
    EXPECT_FALSE(CoordinateConverter::isValidIncrementCoordinate(belowX, m_workspaceSize));
    
    IncrementCoordinates belowY(0, -1, 0);
    EXPECT_FALSE(CoordinateConverter::isValidIncrementCoordinate(belowY, m_workspaceSize));
    
    IncrementCoordinates aboveY(0, 501, 0);
    EXPECT_FALSE(CoordinateConverter::isValidIncrementCoordinate(aboveY, m_workspaceSize));
    
    IncrementCoordinates beyondZ(0, 250, 251);
    EXPECT_FALSE(CoordinateConverter::isValidIncrementCoordinate(beyondZ, m_workspaceSize));
    
    IncrementCoordinates belowZ(0, 250, -251);
    EXPECT_FALSE(CoordinateConverter::isValidIncrementCoordinate(belowZ, m_workspaceSize));
}

TEST_F(CoordinateConverterTest, IsValidWorldCoordinate_ValidPositions) {
    // Test valid world positions
    WorldCoordinates center(0.0f, 2.5f, 0.0f);
    EXPECT_TRUE(CoordinateConverter::isValidWorldCoordinate(center, m_workspaceSize));
    
    WorldCoordinates corner1(-2.5f, 0.0f, -2.5f);
    EXPECT_TRUE(CoordinateConverter::isValidWorldCoordinate(corner1, m_workspaceSize));
    
    WorldCoordinates corner2(2.5f, 5.0f, 2.5f);
    EXPECT_TRUE(CoordinateConverter::isValidWorldCoordinate(corner2, m_workspaceSize));
}

TEST_F(CoordinateConverterTest, IsValidWorldCoordinate_InvalidPositions) {
    // Test invalid world positions
    WorldCoordinates beyondX(3.0f, 2.5f, 0.0f);
    EXPECT_FALSE(CoordinateConverter::isValidWorldCoordinate(beyondX, m_workspaceSize));
    
    WorldCoordinates belowX(-3.0f, 2.5f, 0.0f);
    EXPECT_FALSE(CoordinateConverter::isValidWorldCoordinate(belowX, m_workspaceSize));
    
    WorldCoordinates belowY(0.0f, -1.0f, 0.0f);
    EXPECT_FALSE(CoordinateConverter::isValidWorldCoordinate(belowY, m_workspaceSize));
    
    WorldCoordinates aboveY(0.0f, 6.0f, 0.0f);
    EXPECT_FALSE(CoordinateConverter::isValidWorldCoordinate(aboveY, m_workspaceSize));
}

// ==================== Snapping Tests ====================

TEST_F(CoordinateConverterTest, SnapToIncrementGrid_BasicSnapping) {
    // Test snapping to 1cm grid
    WorldCoordinates unaligned(1.234f, 2.567f, -0.891f);
    WorldCoordinates snapped = CoordinateConverter::snapToIncrementGrid(unaligned);
    
    // Should snap to nearest centimeter
    EXPECT_FLOAT_EQ(snapped.x(), 1.23f);
    EXPECT_FLOAT_EQ(snapped.y(), 2.57f);
    EXPECT_FLOAT_EQ(snapped.z(), -0.89f);
}

TEST_F(CoordinateConverterTest, SnapToVoxelResolution_4cmAlignment) {
    // Test snapping increment coordinates to 4cm voxel boundaries
    IncrementCoordinates unaligned(107, 215, -33);
    IncrementCoordinates snapped = CoordinateConverter::snapToVoxelResolution(
        unaligned, VoxelEditor::VoxelData::VoxelResolution::Size_4cm);
    
    // 4cm = 4cm, so should snap to multiples of 4 using floor division
    // floor(107 / 4) = floor(26.75) = 26, so 26 * 4 = 104
    // floor(215 / 4) = floor(53.75) = 53, so 53 * 4 = 212
    // floor(-33 / 4) = floor(-8.25) = -9, so -9 * 4 = -36
    EXPECT_EQ(snapped.x(), 104);
    EXPECT_EQ(snapped.y(), 212);
    EXPECT_EQ(snapped.z(), -36);
}

TEST_F(CoordinateConverterTest, SnapToVoxelResolution_16cmAlignment) {
    // Test snapping to 16cm voxel boundaries
    IncrementCoordinates unaligned(100, 200, -50);
    IncrementCoordinates snapped = CoordinateConverter::snapToVoxelResolution(
        unaligned, VoxelEditor::VoxelData::VoxelResolution::Size_16cm);
    
    // 16cm = 16cm, so should snap to multiples of 16 using floor division
    // floor(100 / 16) = floor(6.25) = 6, so 6 * 16 = 96
    // floor(200 / 16) = floor(12.5) = 12, so 12 * 16 = 192
    // floor(-50 / 16) = floor(-3.125) = -4, so -4 * 16 = -64
    EXPECT_EQ(snapped.x(), 96);
    EXPECT_EQ(snapped.y(), 192);
    EXPECT_EQ(snapped.z(), -64);
}

TEST_F(CoordinateConverterTest, GetVoxelCenterIncrement_4cmVoxels) {
    // Test getting voxel center for 4cm voxels
    IncrementCoordinates voxelPos(107, 215, -33);
    IncrementCoordinates center = CoordinateConverter::getVoxelCenterIncrement(
        voxelPos, VoxelEditor::VoxelData::VoxelResolution::Size_4cm);
    
    // 4cm voxels have 2cm centers
    // 107: floor(107/4) = 26, so voxel at 26*4 = 104, center at 104 + 2 = 106
    // 215: floor(215/4) = 53, so voxel at 53*4 = 212, center at 212 + 2 = 214
    // -33: floor(-33/4) = -9, so voxel at -9*4 = -36, center at -36 + 2 = -34
    EXPECT_EQ(center.x(), 106);
    EXPECT_EQ(center.y(), 214);
    EXPECT_EQ(center.z(), -34);
}

TEST_F(CoordinateConverterTest, GetVoxelCenterIncrement_16cmVoxels) {
    // Test getting voxel center for 16cm voxels
    IncrementCoordinates voxelPos(100, 200, -50);
    IncrementCoordinates center = CoordinateConverter::getVoxelCenterIncrement(
        voxelPos, VoxelEditor::VoxelData::VoxelResolution::Size_16cm);
    
    // 16cm voxels have 8cm centers
    // 100: floor(100/16) = 6, so voxel at 6*16 = 96, center at 96 + 8 = 104
    // 200: floor(200/16) = 12, so voxel at 12*16 = 192, center at 192 + 8 = 200
    // -50: floor(-50/16) = -4, so voxel at -4*16 = -64, center at -64 + 8 = -56
    EXPECT_EQ(center.x(), 104);
    EXPECT_EQ(center.y(), 200);
    EXPECT_EQ(center.z(), -56);
}

// ==================== Edge Cases and Error Conditions ====================

TEST_F(CoordinateConverterTest, ZeroWorkspace_HandledGracefully) {
    Vector3f zeroWorkspace(0.0f, 0.0f, 0.0f);
    
    // These operations should not crash
    WorldCoordinates world(0.0f, 0.0f, 0.0f);
    IncrementCoordinates increment = CoordinateConverter::worldToIncrement(world);
    
    // Should convert to origin
    EXPECT_EQ(increment.x(), 0);
    EXPECT_EQ(increment.y(), 0);
    EXPECT_EQ(increment.z(), 0);
    
    // Validation should work
    bool isValid = CoordinateConverter::isValidIncrementCoordinate(increment, zeroWorkspace);
    EXPECT_TRUE(isValid); // Origin should be valid even for zero workspace
}

TEST_F(CoordinateConverterTest, LargeValues_NoOverflow) {
    // Test with large but reasonable values
    WorldCoordinates large(100.0f, 100.0f, 100.0f);
    Vector3f largeWorkspace(200.0f, 200.0f, 200.0f);
    
    IncrementCoordinates increment = CoordinateConverter::worldToIncrement(large);
    EXPECT_EQ(increment.x(), 10000); // 100m = 10000cm
    EXPECT_EQ(increment.y(), 10000);
    EXPECT_EQ(increment.z(), 10000);
    
    // Should round-trip correctly
    WorldCoordinates roundTrip = CoordinateConverter::incrementToWorld(increment);
    ExpectNearVector3f(large.value(), roundTrip.value(), 1e-5f);
}

TEST_F(CoordinateConverterTest, AllResolutions_ConsistentSnapping) {
    // Test that all resolutions work consistently for snapping
    // Use a coordinate close to origin that should stay within bounds for all resolutions
    IncrementCoordinates testIncrement(50, 100, -20);
    
    std::vector<VoxelEditor::VoxelData::VoxelResolution> resolutions = {
        VoxelEditor::VoxelData::VoxelResolution::Size_1cm, VoxelEditor::VoxelData::VoxelResolution::Size_2cm, VoxelEditor::VoxelData::VoxelResolution::Size_4cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_8cm, VoxelEditor::VoxelData::VoxelResolution::Size_16cm, VoxelEditor::VoxelData::VoxelResolution::Size_32cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_64cm, VoxelEditor::VoxelData::VoxelResolution::Size_128cm, VoxelEditor::VoxelData::VoxelResolution::Size_256cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_512cm
    };
    
    for (auto res : resolutions) {
        IncrementCoordinates snapped = CoordinateConverter::snapToVoxelResolution(testIncrement, res);
        IncrementCoordinates center = CoordinateConverter::getVoxelCenterIncrement(testIncrement, res);
        
        // Snapped coordinates should work (may be outside bounds for large resolutions, which is expected)
        // Just verify the operations don't crash and produce reasonable results
        
        // Center should be within the voxel size of the original position
        float voxelSize_cm = CoordinateConverter::getVoxelSizeMeters(res) * 100.0f;
        EXPECT_LE(std::abs(center.x() - testIncrement.x()), voxelSize_cm);
        EXPECT_LE(std::abs(center.y() - testIncrement.y()), voxelSize_cm);
        EXPECT_LE(std::abs(center.z() - testIncrement.z()), voxelSize_cm);
    }
}

// ==================== Coordinate System Consistency Tests ====================

TEST_F(CoordinateConverterTest, CoordinateSystemConsistency_CenteredOrigin) {
    // Test that both world and increment coordinates are centered at origin
    WorldCoordinates worldOrigin(0.0f, 0.0f, 0.0f);
    IncrementCoordinates incrementOrigin = CoordinateConverter::worldToIncrement(worldOrigin);
    
    EXPECT_EQ(incrementOrigin.x(), 0);
    EXPECT_EQ(incrementOrigin.y(), 0);
    EXPECT_EQ(incrementOrigin.z(), 0);
    
    // Convert back
    WorldCoordinates worldBack = CoordinateConverter::incrementToWorld(incrementOrigin);
    EXPECT_FLOAT_EQ(worldBack.x(), 0.0f);
    EXPECT_FLOAT_EQ(worldBack.y(), 0.0f);
    EXPECT_FLOAT_EQ(worldBack.z(), 0.0f);
}

TEST_F(CoordinateConverterTest, CoordinateSystemConsistency_NegativeValues) {
    // Test that negative coordinates work correctly in both systems
    WorldCoordinates worldNeg(-1.5f, 2.0f, -0.5f);
    IncrementCoordinates incrementNeg = CoordinateConverter::worldToIncrement(worldNeg);
    
    EXPECT_EQ(incrementNeg.x(), -150);
    EXPECT_EQ(incrementNeg.y(), 200);
    EXPECT_EQ(incrementNeg.z(), -50);
    
    // Verify round-trip
    WorldCoordinates worldBack = CoordinateConverter::incrementToWorld(incrementNeg);
    ExpectNearVector3f(worldNeg.value(), worldBack.value(), 1e-5f);
}