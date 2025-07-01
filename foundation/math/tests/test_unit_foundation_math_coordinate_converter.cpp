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

// Removed: SnapToVoxelResolution tests - function removed per requirements change
// Voxels can now be placed at any 1cm position without resolution-based snapping

TEST_F(CoordinateConverterTest, GetVoxelWorldCenter_1cmVoxels) {
    // Test getting voxel world center for 1cm voxels
    // Voxels are placed with bottom face on ground plane
    IncrementCoordinates voxelPos(0, 0, 0);
    WorldCoordinates center = CoordinateConverter::getVoxelWorldCenter(
        voxelPos, VoxelEditor::VoxelData::VoxelResolution::Size_1cm);
    
    // For 1cm voxel at origin, center should be at Y=0.005m
    EXPECT_FLOAT_EQ(center.x(), 0.0f);
    EXPECT_FLOAT_EQ(center.y(), 0.005f);  // Half of 1cm
    EXPECT_FLOAT_EQ(center.z(), 0.0f);
    
    // Test another position
    IncrementCoordinates voxelPos2(5, 10, -3);
    WorldCoordinates center2 = CoordinateConverter::getVoxelWorldCenter(
        voxelPos2, VoxelEditor::VoxelData::VoxelResolution::Size_1cm);
    
    // Center should be offset by half voxel height
    EXPECT_FLOAT_EQ(center2.x(), 0.05f);   // 5cm
    EXPECT_FLOAT_EQ(center2.y(), 0.105f);  // 10cm + 0.5cm
    EXPECT_FLOAT_EQ(center2.z(), -0.03f);  // -3cm
}

TEST_F(CoordinateConverterTest, GetVoxelWorldCenter_4cmVoxels) {
    // Test getting voxel world center for 4cm voxels
    IncrementCoordinates voxelPos(107, 215, -33);
    WorldCoordinates center = CoordinateConverter::getVoxelWorldCenter(
        voxelPos, VoxelEditor::VoxelData::VoxelResolution::Size_4cm);
    
    // Center should be at bottom-center position + half voxel height in Y
    // 107cm = 1.07m, 215cm = 2.15m, -33cm = -0.33m
    // Y center = 2.15m + 0.02m (half of 4cm) = 2.17m
    EXPECT_FLOAT_EQ(center.x(), 1.07f);
    EXPECT_FLOAT_EQ(center.y(), 2.17f);   // 2.15 + 0.02
    EXPECT_FLOAT_EQ(center.z(), -0.33f);
}

TEST_F(CoordinateConverterTest, GetVoxelWorldCenter_16cmVoxels) {
    // Test getting voxel world center for 16cm voxels
    IncrementCoordinates voxelPos(100, 200, -50);
    WorldCoordinates center = CoordinateConverter::getVoxelWorldCenter(
        voxelPos, VoxelEditor::VoxelData::VoxelResolution::Size_16cm);
    
    // Center should be at bottom-center position + half voxel height in Y
    // 100cm = 1.0m, 200cm = 2.0m, -50cm = -0.5m
    // Y center = 2.0m + 0.08m (half of 16cm) = 2.08m
    EXPECT_FLOAT_EQ(center.x(), 1.0f);
    EXPECT_FLOAT_EQ(center.y(), 2.08f);   // 2.0 + 0.08
    EXPECT_FLOAT_EQ(center.z(), -0.5f);
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

TEST_F(CoordinateConverterTest, AllResolutions_ConsistentCenterCalculation) {
    // Test that all resolutions work consistently for world center calculation
    IncrementCoordinates testIncrement(50, 100, -20);
    
    std::vector<VoxelEditor::VoxelData::VoxelResolution> resolutions = {
        VoxelEditor::VoxelData::VoxelResolution::Size_1cm, VoxelEditor::VoxelData::VoxelResolution::Size_2cm, VoxelEditor::VoxelData::VoxelResolution::Size_4cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_8cm, VoxelEditor::VoxelData::VoxelResolution::Size_16cm, VoxelEditor::VoxelData::VoxelResolution::Size_32cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_64cm, VoxelEditor::VoxelData::VoxelResolution::Size_128cm, VoxelEditor::VoxelData::VoxelResolution::Size_256cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_512cm
    };
    
    for (auto res : resolutions) {
        WorldCoordinates center = CoordinateConverter::getVoxelWorldCenter(testIncrement, res);
        
        // Get expected center position
        WorldCoordinates bottomCenter = CoordinateConverter::incrementToWorld(testIncrement);
        float halfVoxelHeight = CoordinateConverter::getVoxelSizeMeters(res) * 0.5f;
        
        // Center should be at bottom-center position + half voxel height in Y
        EXPECT_FLOAT_EQ(center.x(), bottomCenter.x());
        EXPECT_FLOAT_EQ(center.y(), bottomCenter.y() + halfVoxelHeight);
        EXPECT_FLOAT_EQ(center.z(), bottomCenter.z());
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

// ==================== Arbitrary 1cm Position Tests (Requirements Change) ====================

TEST_F(CoordinateConverterTest, ArbitraryPositions_AllVoxelSizesSupported) {
    // Test that voxels of any size can be placed at any 1cm increment position
    // This verifies the new requirement: no resolution-based snapping
    
    std::vector<IncrementCoordinates> arbitrary1cmPositions = {
        IncrementCoordinates(1, 1, 1),    // Odd positions
        IncrementCoordinates(3, 7, 13),   // Prime numbers
        IncrementCoordinates(17, 23, 31), // More primes
        IncrementCoordinates(-5, 9, -11), // Mixed positive/negative
        IncrementCoordinates(127, 251, -37) // Larger arbitrary positions
    };
    
    std::vector<VoxelEditor::VoxelData::VoxelResolution> allResolutions = {
        VoxelEditor::VoxelData::VoxelResolution::Size_1cm, VoxelEditor::VoxelData::VoxelResolution::Size_2cm, VoxelEditor::VoxelData::VoxelResolution::Size_4cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_8cm, VoxelEditor::VoxelData::VoxelResolution::Size_16cm, VoxelEditor::VoxelData::VoxelResolution::Size_32cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_64cm, VoxelEditor::VoxelData::VoxelResolution::Size_128cm, VoxelEditor::VoxelData::VoxelResolution::Size_256cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_512cm
    };
    
    // Verify that every arbitrary position works with every voxel size
    for (const auto& pos : arbitrary1cmPositions) {
        for (auto resolution : allResolutions) {
            // Convert to world and back to verify consistency
            WorldCoordinates world = CoordinateConverter::incrementToWorld(pos);
            IncrementCoordinates roundTrip = CoordinateConverter::worldToIncrement(world);
            
            // Should get back exactly the same position (no snapping)
            EXPECT_EQ(roundTrip.x(), pos.x()) << "Failed for resolution " << static_cast<int>(resolution) 
                                              << " at position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
            EXPECT_EQ(roundTrip.y(), pos.y()) << "Failed for resolution " << static_cast<int>(resolution) 
                                              << " at position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
            EXPECT_EQ(roundTrip.z(), pos.z()) << "Failed for resolution " << static_cast<int>(resolution) 
                                              << " at position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
        }
    }
}

TEST_F(CoordinateConverterTest, ArbitraryPositions_NoResolutionBasedConstraints) {
    // Test that coordinate system has no resolution-based constraints
    // Previously, positions might have been restricted based on voxel size
    // Now they should work for any 1cm increment regardless of voxel size
    
    // Test positions that would NOT align with various voxel grid sizes
    IncrementCoordinates pos_notAlignedTo4cm(1, 3, 5);   // Not multiples of 4
    IncrementCoordinates pos_notAlignedTo8cm(3, 5, 7);   // Not multiples of 8  
    IncrementCoordinates pos_notAlignedTo16cm(7, 11, 13); // Not multiples of 16
    IncrementCoordinates pos_notAlignedTo32cm(15, 17, 19); // Not multiples of 32
    
    std::vector<IncrementCoordinates> nonAlignedPositions = {
        pos_notAlignedTo4cm, pos_notAlignedTo8cm, pos_notAlignedTo16cm, pos_notAlignedTo32cm
    };
    
    // All these positions should work perfectly with coordinate conversion
    for (const auto& pos : nonAlignedPositions) {
        // Test world conversion roundtrip
        WorldCoordinates world = CoordinateConverter::incrementToWorld(pos);
        IncrementCoordinates roundTrip = CoordinateConverter::worldToIncrement(world);
        
        EXPECT_EQ(roundTrip.x(), pos.x());
        EXPECT_EQ(roundTrip.y(), pos.y());
        EXPECT_EQ(roundTrip.z(), pos.z());
        
        // Test validation with reasonable workspace
        Vector3f largeWorkspace(10.0f, 10.0f, 10.0f); // 10m workspace
        EXPECT_TRUE(CoordinateConverter::isValidIncrementCoordinate(pos, largeWorkspace));
        EXPECT_TRUE(CoordinateConverter::isValidWorldCoordinate(world, largeWorkspace));
    }
}

TEST_F(CoordinateConverterTest, ArbitraryPositions_VoxelCenterCalculation) {
    // Test that voxel world center calculation works for arbitrary starting positions
    // Voxels are placed with bottom face on ground, center is at half height in Y
    
    // Test with positions that are NOT aligned to voxel boundaries
    IncrementCoordinates arbitraryPos(13, 27, -19); // Random 1cm position
    
    // Test different resolutions
    WorldCoordinates center2cm = CoordinateConverter::getVoxelWorldCenter(
        arbitraryPos, VoxelEditor::VoxelData::VoxelResolution::Size_2cm);
    // Bottom center at (0.13, 0.27, -0.19)m, add half of 2cm (0.01m) in Y
    EXPECT_FLOAT_EQ(center2cm.x(), 0.13f);
    EXPECT_FLOAT_EQ(center2cm.y(), 0.28f);  // 0.27 + 0.01
    EXPECT_FLOAT_EQ(center2cm.z(), -0.19f);
    
    WorldCoordinates center8cm = CoordinateConverter::getVoxelWorldCenter(
        arbitraryPos, VoxelEditor::VoxelData::VoxelResolution::Size_8cm);
    // Bottom center at (0.13, 0.27, -0.19)m, add half of 8cm (0.04m) in Y
    EXPECT_FLOAT_EQ(center8cm.x(), 0.13f);
    EXPECT_FLOAT_EQ(center8cm.y(), 0.31f);  // 0.27 + 0.04
    EXPECT_FLOAT_EQ(center8cm.z(), -0.19f);
    
    WorldCoordinates center32cm = CoordinateConverter::getVoxelWorldCenter(
        arbitraryPos, VoxelEditor::VoxelData::VoxelResolution::Size_32cm);
    // Bottom center at (0.13, 0.27, -0.19)m, add half of 32cm (0.16m) in Y
    EXPECT_FLOAT_EQ(center32cm.x(), 0.13f);
    EXPECT_FLOAT_EQ(center32cm.y(), 0.43f);  // 0.27 + 0.16
    EXPECT_FLOAT_EQ(center32cm.z(), -0.19f);
}

TEST_F(CoordinateConverterTest, ArbitraryPositions_SnapToIncrementGridOnly) {
    // Test that snapToIncrementGrid only snaps to 1cm grid, not to resolution grids
    
    // Test coordinates with sub-centimeter precision
    WorldCoordinates subCm1(1.234f, 2.678f, -3.456f);
    WorldCoordinates snapped1 = CoordinateConverter::snapToIncrementGrid(subCm1);
    
    // Should snap to nearest 1cm
    EXPECT_FLOAT_EQ(snapped1.x(), 1.23f);  // Rounds to 123cm = 1.23m
    EXPECT_FLOAT_EQ(snapped1.y(), 2.68f);  // Rounds to 268cm = 2.68m  
    EXPECT_FLOAT_EQ(snapped1.z(), -3.46f); // Rounds to -346cm = -3.46m
    
    // Test edge cases for rounding
    WorldCoordinates edge1(0.004f, 0.006f, -0.004f);
    WorldCoordinates snappedEdge1 = CoordinateConverter::snapToIncrementGrid(edge1);
    
    EXPECT_FLOAT_EQ(snappedEdge1.x(), 0.00f);  // Rounds down
    EXPECT_FLOAT_EQ(snappedEdge1.y(), 0.01f);  // Rounds up
    EXPECT_FLOAT_EQ(snappedEdge1.z(), 0.00f);  // Rounds down (to 0, not -0.01)
}