#include <gtest/gtest.h>
#include "voxel_math/VoxelPlacementMath.h"
#include "math/CoordinateTypes.h"
#include "math/CoordinateConverter.h"
#include "voxel_data/VoxelTypes.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;

class VoxelPlacementMathTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
};

// Test basic 1cm increment snapping
TEST_F(VoxelPlacementMathTest, SnapToValidIncrement_BasicCases) {
    // Test exact cm positions
    {
        WorldCoordinates world(0.01f, 0.02f, 0.03f);  // 1cm, 2cm, 3cm
        IncrementCoordinates result = VoxelPlacementMath::snapToValidIncrement(world);
        EXPECT_EQ(result.x(), 1);
        EXPECT_EQ(result.y(), 2);
        EXPECT_EQ(result.z(), 3);
    }
    
    // Test rounding
    {
        WorldCoordinates world(0.014f, 0.025f, 0.036f);  // Should round to 1cm, 3cm, 4cm
        IncrementCoordinates result = VoxelPlacementMath::snapToValidIncrement(world);
        EXPECT_EQ(result.x(), 1);
        EXPECT_EQ(result.y(), 3);
        EXPECT_EQ(result.z(), 4);
    }
    
    // Test negative values
    {
        WorldCoordinates world(-0.014f, -0.025f, -0.036f);
        IncrementCoordinates result = VoxelPlacementMath::snapToValidIncrement(world);
        EXPECT_EQ(result.x(), -1);
        EXPECT_EQ(result.y(), -3);
        EXPECT_EQ(result.z(), -4);
    }
}

// Test grid-aligned snapping for different resolutions
TEST_F(VoxelPlacementMathTest, SnapToGridAligned_WithShift) {
    // With shift pressed, should always use 1cm increments
    WorldCoordinates world(0.123f, 0.456f, 0.789f);
    
    IncrementCoordinates result4cm = VoxelPlacementMath::snapToGridAligned(
        world, VoxelData::VoxelResolution::Size_4cm, true);
    IncrementCoordinates result32cm = VoxelPlacementMath::snapToGridAligned(
        world, VoxelData::VoxelResolution::Size_32cm, true);
    
    // Both should give same result (1cm snapping)
    EXPECT_EQ(result4cm.x(), 12);  // 12cm
    EXPECT_EQ(result4cm.y(), 46);  // 46cm
    EXPECT_EQ(result4cm.z(), 79);  // 79cm
    
    EXPECT_EQ(result32cm.x(), 12);
    EXPECT_EQ(result32cm.y(), 46);
    EXPECT_EQ(result32cm.z(), 79);
}

// Test grid-aligned snapping without shift (voxel-size alignment)
TEST_F(VoxelPlacementMathTest, SnapToGridAligned_NoShift_4cm) {
    // Test 4cm voxel alignment
    VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_4cm;
    
    // Position near origin - should snap to (0, 0, 0)
    {
        WorldCoordinates world(0.01f, 0.01f, 0.01f);
        IncrementCoordinates result = VoxelPlacementMath::snapToGridAligned(world, res, false);
        EXPECT_EQ(result.x(), 0);
        EXPECT_EQ(result.y(), 0);
        EXPECT_EQ(result.z(), 0);
    }
    
    // Position that should snap to next grid position
    // For 4cm voxels with bottom-center coords:
    // X/Z snap to centers: ..., -4, 0, 4, 8, ...
    // Y snaps to bottoms: 0, 4, 8, 12, ...
    {
        WorldCoordinates world(0.03f, 0.05f, 0.03f);  // 3cm, 5cm, 3cm
        IncrementCoordinates result = VoxelPlacementMath::snapToGridAligned(world, res, false);
        EXPECT_EQ(result.x(), 4);  // Snaps to center at 4cm
        EXPECT_EQ(result.y(), 4);  // Snaps to 4cm (next voxel bottom)
        EXPECT_EQ(result.z(), 4);  // Snaps to center at 4cm
    }
}

// Test grid-aligned snapping for 32cm voxels
TEST_F(VoxelPlacementMathTest, SnapToGridAligned_NoShift_32cm) {
    VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_32cm;
    
    // Test snapping near origin
    {
        WorldCoordinates world(0.1f, 0.1f, 0.1f);  // 10cm each
        IncrementCoordinates result = VoxelPlacementMath::snapToGridAligned(world, res, false);
        EXPECT_EQ(result.x(), 0);   // Snaps to center at 0
        EXPECT_EQ(result.y(), 0);   // Snaps to bottom at 0
        EXPECT_EQ(result.z(), 0);   // Snaps to center at 0
    }
    
    // Test snapping to next grid position
    {
        WorldCoordinates world(0.2f, 0.35f, 0.2f);  // 20cm, 35cm, 20cm
        IncrementCoordinates result = VoxelPlacementMath::snapToGridAligned(world, res, false);
        EXPECT_EQ(result.x(), 32);  // Snaps to center at 32cm
        EXPECT_EQ(result.y(), 32);  // Snaps to next voxel bottom at 32cm
        EXPECT_EQ(result.z(), 32);  // Snaps to center at 32cm
    }
}

// Test surface face grid snapping
TEST_F(VoxelPlacementMathTest, SnapToSurfaceFaceGrid_TopFace) {
    // 32cm voxel at origin
    IncrementCoordinates surfacePos(0, 0, 0);
    VoxelData::VoxelResolution surfaceRes = VoxelData::VoxelResolution::Size_32cm;
    VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosY;  // Top face
    
    // Placing a 4cm voxel on top
    VoxelData::VoxelResolution placeRes = VoxelData::VoxelResolution::Size_4cm;
    
    // Hit point near center of top face
    WorldCoordinates hitPoint(0.0f, 0.32f, 0.0f);  // Top of 32cm voxel
    
    IncrementCoordinates result = VoxelPlacementMath::snapToSurfaceFaceGrid(
        hitPoint, surfacePos, surfaceRes, faceDir, placeRes, true, false);
    
    // Should place at Y=32cm (on top of the surface)
    EXPECT_EQ(result.x(), 0);
    EXPECT_EQ(result.y(), 32);
    EXPECT_EQ(result.z(), 0);
}

// Test surface face grid snapping with offset
TEST_F(VoxelPlacementMathTest, SnapToSurfaceFaceGrid_TopFace_Offset) {
    // 32cm voxel at origin
    IncrementCoordinates surfacePos(0, 0, 0);
    VoxelData::VoxelResolution surfaceRes = VoxelData::VoxelResolution::Size_32cm;
    VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosY;
    
    // Placing a 2cm voxel with offset
    VoxelData::VoxelResolution placeRes = VoxelData::VoxelResolution::Size_2cm;
    
    // Hit point offset from center
    WorldCoordinates hitPoint(0.05f, 0.32f, 0.07f);  // 5cm, 32cm, 7cm
    
    IncrementCoordinates result = VoxelPlacementMath::snapToSurfaceFaceGrid(
        hitPoint, surfacePos, surfaceRes, faceDir, placeRes, true, false);
    
    // Should snap to nearest cm
    EXPECT_EQ(result.x(), 5);
    EXPECT_EQ(result.y(), 32);
    EXPECT_EQ(result.z(), 7);
}

// Test small voxel placement on side faces
TEST_F(VoxelPlacementMathTest, SnapToSurfaceFaceGrid_SideFaces_SmallVoxel) {
    // 32cm voxel at origin
    IncrementCoordinates surfacePos(0, 0, 0);
    VoxelData::VoxelResolution surfaceRes = VoxelData::VoxelResolution::Size_32cm;
    
    // Placing a 4cm voxel
    VoxelData::VoxelResolution placeRes = VoxelData::VoxelResolution::Size_4cm;
    
    // Test right face (PosX)
    {
        VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosX;
        WorldCoordinates hitPoint(0.16f, 0.1f, 0.05f);  // On right face
        
        IncrementCoordinates result = VoxelPlacementMath::snapToSurfaceFaceGrid(
            hitPoint, surfacePos, surfaceRes, faceDir, placeRes, true, false);
        
        // Should be placed with its center at 16cm + 2cm = 18cm (face + half voxel)
        EXPECT_EQ(result.x(), 18);  // 16cm (face) + 2cm (half of 4cm voxel)
        EXPECT_EQ(result.y(), 10);  // Y from hit point
        EXPECT_EQ(result.z(), 5);   // Z from hit point
    }
    
    // Test left face (NegX)
    {
        VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::NegX;
        WorldCoordinates hitPoint(-0.16f, 0.08f, 0.03f);  // On left face
        
        IncrementCoordinates result = VoxelPlacementMath::snapToSurfaceFaceGrid(
            hitPoint, surfacePos, surfaceRes, faceDir, placeRes, true, false);
        
        // Should be placed with its center at -16cm - 2cm = -18cm
        EXPECT_EQ(result.x(), -18);  // -16cm (face) - 2cm (half of 4cm voxel)
        EXPECT_EQ(result.y(), 8);
        EXPECT_EQ(result.z(), 3);
    }
    
    // Test front face (NegZ)
    {
        VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::NegZ;
        WorldCoordinates hitPoint(0.05f, 0.12f, -0.16f);  // On front face
        
        IncrementCoordinates result = VoxelPlacementMath::snapToSurfaceFaceGrid(
            hitPoint, surfacePos, surfaceRes, faceDir, placeRes, true, false);
        
        // Should be placed with its center at -16cm - 2cm = -18cm
        EXPECT_EQ(result.x(), 5);
        EXPECT_EQ(result.y(), 12);
        EXPECT_EQ(result.z(), -18);  // -16cm (face) - 2cm (half of 4cm voxel)
    }
}

// Test overhang prevention for same-size voxels
TEST_F(VoxelPlacementMathTest, SnapToSurfaceFaceGrid_NoOverhang_SameSize) {
    // 8cm voxel at origin
    IncrementCoordinates surfacePos(0, 0, 0);
    VoxelData::VoxelResolution surfaceRes = VoxelData::VoxelResolution::Size_8cm;
    VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosY;
    
    // Placing another 8cm voxel
    VoxelData::VoxelResolution placeRes = VoxelData::VoxelResolution::Size_8cm;
    
    // Hit point near edge (would cause overhang)
    WorldCoordinates hitPoint(0.06f, 0.08f, 0.0f);  // 6cm from center (beyond 4cm half-size)
    
    IncrementCoordinates result = VoxelPlacementMath::snapToSurfaceFaceGrid(
        hitPoint, surfacePos, surfaceRes, faceDir, placeRes, false, false);  // No overhang allowed, no shift
    
    // Should clamp to valid position (0cm since 8cm voxel extends ±4cm from center)
    EXPECT_EQ(result.x(), 0);  // Clamped to prevent overhang
    EXPECT_EQ(result.y(), 8);  // On top of surface
    EXPECT_EQ(result.z(), 0);
}

// Test same-size voxel alignment without shift
TEST_F(VoxelPlacementMathTest, SameSizeVoxelAlignment_NoShift) {
    // 32cm voxel at origin
    IncrementCoordinates surfacePos(0, 0, 0);
    VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_32cm;
    
    // Test all face directions for same-size voxel placement
    {
        // Top face
        VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosY;
        WorldCoordinates hitPoint(0.1f, 0.32f, 0.1f);  // Anywhere on top face
        
        IncrementCoordinates result = VoxelPlacementMath::snapToSurfaceFaceGrid(
            hitPoint, surfacePos, res, faceDir, res, true, false);  // Same size, no shift
        
        // Should align perfectly on top
        EXPECT_EQ(result.x(), 0);
        EXPECT_EQ(result.y(), 32);  // Exactly one voxel above
        EXPECT_EQ(result.z(), 0);
    }
    
    {
        // Right face (PosX)
        VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosX;
        WorldCoordinates hitPoint(0.16f, 0.1f, 0.0f);  // On right face
        
        IncrementCoordinates result = VoxelPlacementMath::snapToSurfaceFaceGrid(
            hitPoint, surfacePos, res, faceDir, res, true, false);
        
        // Should align perfectly to the right
        EXPECT_EQ(result.x(), 32);  // One voxel to the right
        EXPECT_EQ(result.y(), 0);
        EXPECT_EQ(result.z(), 0);
    }
    
    {
        // Front face (NegZ)
        VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::NegZ;
        WorldCoordinates hitPoint(0.0f, 0.1f, -0.16f);  // On front face
        
        IncrementCoordinates result = VoxelPlacementMath::snapToSurfaceFaceGrid(
            hitPoint, surfacePos, res, faceDir, res, true, false);
        
        // Should align perfectly in front
        EXPECT_EQ(result.x(), 0);
        EXPECT_EQ(result.y(), 0);
        EXPECT_EQ(result.z(), -32);  // One voxel in front
    }
}

// Test same-size voxel with shift allows 1cm placement
TEST_F(VoxelPlacementMathTest, SameSizeVoxelAlignment_WithShift) {
    // 32cm voxel at origin
    IncrementCoordinates surfacePos(0, 0, 0);
    VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_32cm;
    VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosY;
    
    // Hit point with offset
    WorldCoordinates hitPoint(0.05f, 0.32f, 0.07f);  // 5cm, 32cm, 7cm
    
    IncrementCoordinates result = VoxelPlacementMath::snapToSurfaceFaceGrid(
        hitPoint, surfacePos, res, faceDir, res, true, true);  // Same size, WITH shift
    
    // Should allow 1cm placement
    EXPECT_EQ(result.x(), 5);   // Allows offset
    EXPECT_EQ(result.y(), 32);
    EXPECT_EQ(result.z(), 7);   // Allows offset
}

// Test valid increment position checking
TEST_F(VoxelPlacementMathTest, IsValidIncrementPosition) {
    // Valid positions (Y >= 0)
    EXPECT_TRUE(VoxelPlacementMath::isValidIncrementPosition(IncrementCoordinates(0, 0, 0)));
    EXPECT_TRUE(VoxelPlacementMath::isValidIncrementPosition(IncrementCoordinates(-100, 0, -100)));
    EXPECT_TRUE(VoxelPlacementMath::isValidIncrementPosition(IncrementCoordinates(100, 50, 100)));
    
    // Invalid positions (Y < 0)
    EXPECT_FALSE(VoxelPlacementMath::isValidIncrementPosition(IncrementCoordinates(0, -1, 0)));
    EXPECT_FALSE(VoxelPlacementMath::isValidIncrementPosition(IncrementCoordinates(0, -100, 0)));
}

// Test valid world position checking
TEST_F(VoxelPlacementMathTest, IsValidForIncrementPlacement) {
    // Valid positions
    EXPECT_TRUE(VoxelPlacementMath::isValidForIncrementPlacement(WorldCoordinates(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(VoxelPlacementMath::isValidForIncrementPlacement(WorldCoordinates(10.0f, 5.0f, -10.0f)));
    
    // Invalid positions (NaN)
    float nan = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(VoxelPlacementMath::isValidForIncrementPlacement(WorldCoordinates(nan, 0.0f, 0.0f)));
    EXPECT_FALSE(VoxelPlacementMath::isValidForIncrementPlacement(WorldCoordinates(0.0f, nan, 0.0f)));
    EXPECT_FALSE(VoxelPlacementMath::isValidForIncrementPlacement(WorldCoordinates(0.0f, 0.0f, nan)));
    
    // Invalid positions (infinity)
    float inf = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(VoxelPlacementMath::isValidForIncrementPlacement(WorldCoordinates(inf, 0.0f, 0.0f)));
    EXPECT_FALSE(VoxelPlacementMath::isValidForIncrementPlacement(WorldCoordinates(0.0f, -inf, 0.0f)));
    
    // Invalid positions (extreme values)
    float extreme = 2000000.0f;  // Would overflow increment coordinates
    EXPECT_FALSE(VoxelPlacementMath::isValidForIncrementPlacement(WorldCoordinates(extreme, 0.0f, 0.0f)));
}

// Test voxel world bounds calculation
TEST_F(VoxelPlacementMathTest, CalculateVoxelWorldBounds) {
    // 4cm voxel at origin
    {
        IncrementCoordinates pos(0, 0, 0);
        VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_4cm;
        Vector3f min, max;
        
        VoxelPlacementMath::calculateVoxelWorldBounds(pos, res, min, max);
        
        // 4cm voxel extends ±2cm in X/Z from center, 0-4cm in Y
        EXPECT_FLOAT_EQ(min.x, -0.02f);
        EXPECT_FLOAT_EQ(min.y, 0.0f);
        EXPECT_FLOAT_EQ(min.z, -0.02f);
        EXPECT_FLOAT_EQ(max.x, 0.02f);
        EXPECT_FLOAT_EQ(max.y, 0.04f);
        EXPECT_FLOAT_EQ(max.z, 0.02f);
    }
    
    // 32cm voxel at (64, 32, -64)
    {
        IncrementCoordinates pos(64, 32, -64);
        VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_32cm;
        Vector3f min, max;
        
        VoxelPlacementMath::calculateVoxelWorldBounds(pos, res, min, max);
        
        // 32cm voxel extends ±16cm in X/Z from center
        EXPECT_FLOAT_EQ(min.x, 0.64f - 0.16f);  // 0.48
        EXPECT_FLOAT_EQ(min.y, 0.32f);           // 0.32
        EXPECT_FLOAT_EQ(min.z, -0.64f - 0.16f);  // -0.80
        EXPECT_FLOAT_EQ(max.x, 0.64f + 0.16f);   // 0.80
        EXPECT_FLOAT_EQ(max.y, 0.32f + 0.32f);   // 0.64
        EXPECT_FLOAT_EQ(max.z, -0.64f + 0.16f);  // -0.48
    }
}

// Test face bounds checking
TEST_F(VoxelPlacementMathTest, IsWithinFaceBounds_TopFace) {
    // 16cm voxel at origin
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_16cm;
    VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosY;
    
    // Points on top face (Y = 16cm)
    // Voxel extends from -8cm to +8cm in X and Z
    
    // Center of face - should be within bounds
    EXPECT_TRUE(VoxelPlacementMath::isWithinFaceBounds(
        WorldCoordinates(0.0f, 0.16f, 0.0f), voxelPos, res, faceDir));
    
    // Near edge but still within
    EXPECT_TRUE(VoxelPlacementMath::isWithinFaceBounds(
        WorldCoordinates(0.07f, 0.16f, 0.07f), voxelPos, res, faceDir));
    
    // Just outside bounds
    EXPECT_FALSE(VoxelPlacementMath::isWithinFaceBounds(
        WorldCoordinates(0.09f, 0.16f, 0.0f), voxelPos, res, faceDir));
    
    // Way outside bounds
    EXPECT_FALSE(VoxelPlacementMath::isWithinFaceBounds(
        WorldCoordinates(0.2f, 0.16f, 0.0f), voxelPos, res, faceDir));
}

// Test face bounds checking for side faces
TEST_F(VoxelPlacementMathTest, IsWithinFaceBounds_SideFaces) {
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_8cm;
    
    // Right face (PosX) - X = 4cm, check YZ bounds
    {
        VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosX;
        
        // Within bounds
        EXPECT_TRUE(VoxelPlacementMath::isWithinFaceBounds(
            WorldCoordinates(0.04f, 0.04f, 0.0f), voxelPos, res, faceDir));
        
        // Outside Y bounds
        EXPECT_FALSE(VoxelPlacementMath::isWithinFaceBounds(
            WorldCoordinates(0.04f, 0.09f, 0.0f), voxelPos, res, faceDir));
        
        // Outside Z bounds
        EXPECT_FALSE(VoxelPlacementMath::isWithinFaceBounds(
            WorldCoordinates(0.04f, 0.04f, 0.05f), voxelPos, res, faceDir));
    }
    
    // Front face (NegZ) - Z = -4cm, check XY bounds
    {
        VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::NegZ;
        
        // Within bounds
        EXPECT_TRUE(VoxelPlacementMath::isWithinFaceBounds(
            WorldCoordinates(0.0f, 0.04f, -0.04f), voxelPos, res, faceDir));
        
        // Outside X bounds
        EXPECT_FALSE(VoxelPlacementMath::isWithinFaceBounds(
            WorldCoordinates(0.05f, 0.04f, -0.04f), voxelPos, res, faceDir));
    }
}

// Test the half-voxel offset fix for grid alignment
TEST_F(VoxelPlacementMathTest, GridAlignment_HalfVoxelOffset) {
    // This tests the specific issue we were fixing
    VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_32cm;
    
    // Position at exact voxel center should stay at center
    {
        WorldCoordinates world(0.0f, 0.32f, 0.0f);  // Exact center of grid cell
        IncrementCoordinates result = VoxelPlacementMath::snapToGridAligned(world, res, false);
        EXPECT_EQ(result.x(), 0);
        EXPECT_EQ(result.y(), 32);
        EXPECT_EQ(result.z(), 0);
    }
    
    // Position slightly offset should still snap to same voxel
    {
        WorldCoordinates world(0.1f, 0.32f, 0.1f);  // 10cm offset in X/Z
        IncrementCoordinates result = VoxelPlacementMath::snapToGridAligned(world, res, false);
        EXPECT_EQ(result.x(), 0);   // Should snap to 0, not 32
        EXPECT_EQ(result.y(), 32);
        EXPECT_EQ(result.z(), 0);   // Should snap to 0, not 32
    }
    
    // Position past half should snap to next voxel
    {
        WorldCoordinates world(0.17f, 0.32f, 0.17f);  // 17cm offset (>16cm half)
        IncrementCoordinates result = VoxelPlacementMath::snapToGridAligned(world, res, false);
        EXPECT_EQ(result.x(), 32);  // Should snap to next voxel
        EXPECT_EQ(result.y(), 32);
        EXPECT_EQ(result.z(), 32);  // Should snap to next voxel
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}