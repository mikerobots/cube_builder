#include <gtest/gtest.h>
#include "../PlacementValidation.h"
#include "../../voxel_data/VoxelTypes.h"
#include "../../voxel_data/VoxelDataManager.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Input;
using namespace VoxelEditor::Math;

class PlacementValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default workspace size
        m_workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
    }
    
    Vector3f m_workspaceSize;
};

// Test position snapping to 1cm increments
TEST_F(PlacementValidationTest, SnapToValidIncrement) {
    // REQ-2.1.1: Voxels shall be placeable only at 1cm increment positions
    // REQ-2.2.2: The preview shall snap to the nearest valid 1cm increment position
    // Test exact positions
    {
        WorldCoordinates worldPos(Vector3f(0.0f, 0.0f, 0.0f));
        IncrementCoordinates snapped = PlacementUtils::snapToValidIncrement(worldPos);
        EXPECT_EQ(snapped.x(), 0);
        EXPECT_EQ(snapped.y(), 0);
        EXPECT_EQ(snapped.z(), 0);
    }
    
    // Test positions that need rounding up
    {
        WorldCoordinates worldPos(Vector3f(0.126f, 0.238f, 0.359f));
        IncrementCoordinates snapped = PlacementUtils::snapToValidIncrement(worldPos);
        EXPECT_EQ(snapped.x(), 13); // 0.126 -> 0.13 -> 13
        EXPECT_EQ(snapped.y(), 24); // 0.238 -> 0.24 -> 24
        EXPECT_EQ(snapped.z(), 36); // 0.359 -> 0.36 -> 36
    }
    
    // Test positions that need rounding down
    {
        WorldCoordinates worldPos(Vector3f(0.123f, 0.234f, 0.345f));
        IncrementCoordinates snapped = PlacementUtils::snapToValidIncrement(worldPos);
        EXPECT_EQ(snapped.x(), 12); // 0.123 -> 0.12 -> 12
        EXPECT_EQ(snapped.y(), 23); // 0.234 -> 0.23 -> 23
        EXPECT_EQ(snapped.z(), 35); // 0.345 -> 0.35 -> 35
    }
    
    // Test negative positions
    {
        WorldCoordinates worldPos(Vector3f(-0.126f, -0.238f, -0.359f));
        IncrementCoordinates snapped = PlacementUtils::snapToValidIncrement(worldPos);
        EXPECT_EQ(snapped.x(), -13);
        EXPECT_EQ(snapped.y(), -24);
        EXPECT_EQ(snapped.z(), -36);
    }
}

// Test grid-aligned snapping behavior (updated for new requirements)
TEST_F(PlacementValidationTest, SnapToGridAligned) {
    // With new requirements: All voxels place at exact 1cm positions without resolution-based snapping
    // The shift key and resolution parameters are maintained for compatibility but don't affect snapping
    
    // Test 32cm voxel without shift (now always snaps to 1cm grid)
    {
        WorldCoordinates worldPos(Vector3f(0.15f, 0.15f, 0.15f));
        IncrementCoordinates snapped = PlacementUtils::snapToGridAligned(worldPos, 
            VoxelData::VoxelResolution::Size_32cm, false);
        EXPECT_EQ(snapped.x(), 15);  // 0.15 -> 15cm (1cm increment)
        EXPECT_EQ(snapped.y(), 15);
        EXPECT_EQ(snapped.z(), 15);
    }
    
    // Test 32cm voxel with shift (same behavior - always 1cm increments)
    {
        WorldCoordinates worldPos(Vector3f(0.15f, 0.15f, 0.15f));
        IncrementCoordinates snapped = PlacementUtils::snapToGridAligned(worldPos, 
            VoxelData::VoxelResolution::Size_32cm, true);
        EXPECT_EQ(snapped.x(), 15);  // 0.15 -> 15cm (1cm increment)
        EXPECT_EQ(snapped.y(), 15);
        EXPECT_EQ(snapped.z(), 15);
    }
    
    // Test 16cm voxel without shift (now always snaps to 1cm grid)
    {
        WorldCoordinates worldPos(Vector3f(0.25f, 0.25f, 0.25f));
        IncrementCoordinates snapped = PlacementUtils::snapToGridAligned(worldPos, 
            VoxelData::VoxelResolution::Size_16cm, false);
        EXPECT_EQ(snapped.x(), 25);  // 0.25 -> 25cm (1cm increment)
        EXPECT_EQ(snapped.y(), 25);
        EXPECT_EQ(snapped.z(), 25);
    }
    
    // Test with arbitrary position - should always snap to 1cm increments
    {
        WorldCoordinates worldPos(Vector3f(0.237f, 0.189f, 0.341f));
        IncrementCoordinates snapped = PlacementUtils::snapToGridAligned(worldPos, 
            VoxelData::VoxelResolution::Size_4cm, false);
        EXPECT_EQ(snapped.x(), 24);  // 0.237 -> 24cm
        EXPECT_EQ(snapped.y(), 19);  // 0.189 -> 19cm 
        EXPECT_EQ(snapped.z(), 34);  // 0.341 -> 34cm
    }
}

// Test basic placement validation - comprehensive test for validatePlacement function
TEST_F(PlacementValidationTest, ValidatePlacementBasic) {
    // REQ-2.1.1: Voxels shall be placeable only at 1cm increment positions
    // REQ-2.1.4: No voxels shall be placed below Y=0
    // REQ-5.2.3: Only positions with Y ≥ 0 shall be valid
    
    // Test 1: Valid placement at origin
    {
        IncrementCoordinates gridPos(0, 0, 0);
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::Valid) 
            << "Placement at origin should be valid";
    }
    
    // Test 2: Valid placement at positive coordinates
    {
        IncrementCoordinates gridPos(100, 50, 100);  // 1m, 0.5m, 1m
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_4cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::Valid)
            << "Placement at positive coordinates should be valid";
    }
    
    // Test 3: Invalid placement below ground (Y < 0)
    {
        IncrementCoordinates gridPos(50, -1, 50);
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::InvalidYBelowZero)
            << "Placement below Y=0 should be invalid";
    }
    
    // Test 4: Invalid placement outside workspace bounds (positive)
    {
        IncrementCoordinates gridPos(300, 50, 300);  // 3m, 0.5m, 3m - outside 2.5m bounds
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::InvalidOutOfBounds)
            << "Placement outside positive workspace bounds should be invalid";
    }
    
    // Test 5: Invalid placement outside workspace bounds (negative)
    {
        IncrementCoordinates gridPos(-300, 50, -300);  // -3m, 0.5m, -3m - outside -2.5m bounds
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::InvalidOutOfBounds)
            << "Placement outside negative workspace bounds should be invalid";
    }
    
    // Test 6: Valid placement at workspace edge
    {
        IncrementCoordinates gridPos(249, 50, 249);  // 2.49m - just inside 2.5m bounds
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::Valid)
            << "Placement at workspace edge should be valid";
    }
    
    // Test 7: Large voxel placement near boundary
    {
        // 64cm voxel at 2.2m position - voxel extends to 2.84m (past 2.5m bound)
        // NOTE: Current implementation only checks if placement position is within bounds,
        // not if the full voxel fits. This matches VoxelDataManager behavior.
        IncrementCoordinates gridPos(220, 0, 220);
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_64cm, m_workspaceSize);
        // Since 2.2m is within the 2.5m bound, this is considered valid
        // even though the voxel extends past the boundary
        EXPECT_EQ(result, PlacementValidationResult::Valid)
            << "Placement position within bounds should be valid (current behavior doesn't check full voxel)";
    }
    
    // Test 8: Different resolution voxels at same position
    {
        IncrementCoordinates gridPos(100, 100, 100);
        
        // 1cm voxel should be valid
        auto result1cm = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result1cm, PlacementValidationResult::Valid);
        
        // 32cm voxel should be valid
        auto result32cm = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_32cm, m_workspaceSize);
        EXPECT_EQ(result32cm, PlacementValidationResult::Valid);
        
        // 256cm voxel - current implementation only checks placement position
        auto result256cm = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_256cm, m_workspaceSize);
        // At position 1m,1m,1m placement is within bounds, so it's valid
        // (even though the voxel would extend past bounds)
        EXPECT_EQ(result256cm, PlacementValidationResult::Valid);
    }
}

// Test Y >= 0 constraint validation
TEST_F(PlacementValidationTest, ValidateYBelowZero) {
    // REQ-2.1.4: No voxels shall be placed below Y=0
    // REQ-5.2.3: Only positions with Y ≥ 0 shall be valid
    // Test position below ground
    {
        IncrementCoordinates gridPos(10, -5, 10);
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::InvalidYBelowZero);
    }
    
    // Test position at ground
    {
        IncrementCoordinates gridPos(10, 0, 10);
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::Valid);
    }
    
    // Test position above ground
    {
        IncrementCoordinates gridPos(10, 10, 10);
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::Valid);
    }
}


// Test world to increment coordinate conversion via CoordinateConverter
TEST_F(PlacementValidationTest, WorldToIncrementCoordinate) {
    // Test positive positions
    {
        Vector3f worldPos(1.234f, 2.345f, 3.456f);
        WorldCoordinates worldCoords(worldPos);
        IncrementCoordinates incCoords = Math::CoordinateConverter::worldToIncrement(worldCoords);
        EXPECT_EQ(incCoords.x(), 123); // 1.234m = 123.4cm -> 123
        EXPECT_EQ(incCoords.y(), 235); // 2.345m = 234.5cm -> round to 235
        EXPECT_EQ(incCoords.z(), 346); // 3.456m = 345.6cm -> round to 346
    }
    
    // Test negative positions
    {
        Vector3f worldPos(-1.234f, -2.345f, -3.456f);
        WorldCoordinates worldCoords(worldPos);
        IncrementCoordinates incCoords = Math::CoordinateConverter::worldToIncrement(worldCoords);
        EXPECT_EQ(incCoords.x(), -123); // round(-123.4) = -123
        EXPECT_EQ(incCoords.y(), -235); // round(-234.5) = -235 (already half)
        EXPECT_EQ(incCoords.z(), -346); // round(-345.6) = -346
    }
}

// Test increment coordinate to world conversion via CoordinateConverter
TEST_F(PlacementValidationTest, IncrementToWorldCoordinate) {
    // Test positive increment positions
    {
        IncrementCoordinates incCoords(123, 234, 345);
        WorldCoordinates worldCoords = Math::CoordinateConverter::incrementToWorld(incCoords);
        Vector3f worldPos = worldCoords.value();
        EXPECT_FLOAT_EQ(worldPos.x, 1.23f);
        EXPECT_FLOAT_EQ(worldPos.y, 2.34f);
        EXPECT_FLOAT_EQ(worldPos.z, 3.45f);
    }
    
    // Test negative increment positions
    {
        IncrementCoordinates incCoords(-123, -234, -345);
        WorldCoordinates worldCoords = Math::CoordinateConverter::incrementToWorld(incCoords);
        Vector3f worldPos = worldCoords.value();
        EXPECT_FLOAT_EQ(worldPos.x, -1.23f);
        EXPECT_FLOAT_EQ(worldPos.y, -2.34f);
        EXPECT_FLOAT_EQ(worldPos.z, -3.45f);
    }
}

// Test complete placement context
TEST_F(PlacementValidationTest, GetPlacementContext) {
    // Test valid placement without shift - NEW BEHAVIOR: exact 1cm position placement
    {
        WorldCoordinates worldPos(Vector3f(1.15f, 0.5f, 1.15f));
        PlacementContext context = PlacementUtils::getPlacementContext(
            worldPos, VoxelData::VoxelResolution::Size_32cm, false, m_workspaceSize);
        
        // NEW BEHAVIOR: 1.15m = 115cm (exact position, no snapping to 32cm boundaries)
        EXPECT_EQ(context.snappedIncrementPos.x(), 115);   // 1.15m = 115cm (exact)
        EXPECT_EQ(context.snappedIncrementPos.y(), 50);    // 0.5m = 50cm (exact)
        EXPECT_EQ(context.snappedIncrementPos.z(), 115);   // 1.15m = 115cm (exact)
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid);
        EXPECT_FALSE(context.shiftPressed);
    }
    
    // Test valid placement with shift
    {
        WorldCoordinates worldPos(Vector3f(1.15f, 0.5f, 1.15f));
        PlacementContext context = PlacementUtils::getPlacementContext(
            worldPos, VoxelData::VoxelResolution::Size_32cm, true, m_workspaceSize);
        
        EXPECT_EQ(context.snappedIncrementPos.x(), 115); // 1.15m = 115cm
        EXPECT_EQ(context.snappedIncrementPos.y(), 50);  // 0.5m = 50cm
        EXPECT_EQ(context.snappedIncrementPos.z(), 115);
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid);
        EXPECT_TRUE(context.shiftPressed);
    }
    
    // Test invalid placement (Y < 0)
    {
        WorldCoordinates worldPos(Vector3f(1.0f, -0.5f, 1.0f));
        PlacementContext context = PlacementUtils::getPlacementContext(
            worldPos, VoxelData::VoxelResolution::Size_32cm, false, m_workspaceSize);
        
        EXPECT_EQ(context.validation, PlacementValidationResult::InvalidYBelowZero);
    }
    
    // Test invalid placement (out of bounds)
    {
        WorldCoordinates worldPos(Vector3f(3.0f, 1.0f, 3.0f)); // Outside 2.5m bound
        PlacementContext context = PlacementUtils::getPlacementContext(
            worldPos, VoxelData::VoxelResolution::Size_32cm, false, m_workspaceSize);
        
        EXPECT_EQ(context.validation, PlacementValidationResult::InvalidOutOfBounds);
    }
}

// Test snap override with Shift key for all resolutions
TEST_F(PlacementValidationTest, ShiftKeyOverrideAllResolutions) {
    // REQ-3.1.2: Holding Shift shall allow placement at any valid 1cm increment
    // REQ-5.4.1: Shift key shall override auto-snap for same-size voxels
    WorldCoordinates testPos(Vector3f(0.123f, 0.234f, 0.345f));
    
    // Test all resolutions
    for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
        auto resolution = static_cast<VoxelData::VoxelResolution>(i);
        
        // NEW BEHAVIOR: Both shift and no-shift should use 1cm increments
        IncrementCoordinates snappedNoShift = PlacementUtils::snapToGridAligned(testPos, resolution, false);
        
        // With shift - should also snap to 1cm increments (same behavior)
        IncrementCoordinates snappedWithShift = PlacementUtils::snapToGridAligned(testPos, resolution, true);
        
        // Both should give same result (1cm increments) regardless of shift
        EXPECT_EQ(snappedNoShift.x(), 12);
        EXPECT_EQ(snappedNoShift.y(), 23);
        EXPECT_EQ(snappedNoShift.z(), 35);
        
        EXPECT_EQ(snappedWithShift.x(), 12);
        EXPECT_EQ(snappedWithShift.y(), 23);
        EXPECT_EQ(snappedWithShift.z(), 35);
    }
}

// ===== Phase 3 Smart Snapping Tests =====

class SmartSnappingTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_workspaceSize = Vector3f(10.0f, 10.0f, 10.0f);
        m_dataManager = std::make_unique<VoxelData::VoxelDataManager>();
        
        // Set up test workspace
        m_dataManager->getWorkspaceManager()->setSize(m_workspaceSize);
        
        // Add some test voxels for snapping tests
        m_dataManager->setVoxel(IncrementCoordinates(100, 0, 100), VoxelData::VoxelResolution::Size_32cm, true); // 32cm voxel at (1.0m, 0, 1.0m)
        m_dataManager->setVoxel(IncrementCoordinates(200, 0, 100), VoxelData::VoxelResolution::Size_32cm, true); // Adjacent 32cm voxel at (2.0m, 0, 1.0m) 
        m_dataManager->setVoxel(IncrementCoordinates(50, 0, 50), VoxelData::VoxelResolution::Size_16cm, true);   // 16cm voxel at (0.5m, 0, 0.5m)
    }
    
    Vector3f m_workspaceSize;
    std::unique_ptr<VoxelData::VoxelDataManager> m_dataManager;
};

// Test same-size voxel placement - NEW BEHAVIOR: no resolution-based snapping
TEST_F(SmartSnappingTest, SameSizeVoxelSnapping) {
    // REQ-3.1.1: Same-size voxels shall achieve face-to-face alignment but at exact 1cm positions
    // NEW BEHAVIOR: No longer snap to resolution grids, use exact 1cm positions
    using namespace VoxelData;
    
    // Test placement at exact 1cm position when placing 32cm voxel
    WorldCoordinates worldPos(Vector3f(3.35f, 0.0f, 3.35f)); // Exact position should be preserved
    IncrementCoordinates snapped = PlacementUtils::snapToSameSizeVoxel(worldPos, VoxelResolution::Size_32cm, *m_dataManager, false);
    
    // NEW BEHAVIOR: Should use exact 1cm position (3.35m = 335cm)
    IncrementCoordinates expected(335, 0, 335); // Exact 1cm position, no grid snapping
    EXPECT_EQ(snapped.x(), expected.x());
    EXPECT_EQ(snapped.y(), expected.y());
    EXPECT_EQ(snapped.z(), expected.z());
}

// Test same-size snapping with Shift override
TEST_F(SmartSnappingTest, SameSizeSnappingShiftOverride) {
    // REQ-3.1.2: Holding Shift shall allow placement at any valid 1cm increment
    // REQ-5.4.1: Shift key shall override auto-snap for same-size voxels
    using namespace VoxelData;
    
    WorldCoordinates worldPos(Vector3f(3.35f, 0.0f, 3.35f));
    IncrementCoordinates snapped = PlacementUtils::snapToSameSizeVoxel(worldPos, VoxelResolution::Size_32cm, *m_dataManager, true);
    
    // With Shift pressed, should snap to 1cm increments regardless of nearby voxels
    IncrementCoordinates expected(335, 0, 335); // 3.35m rounded to nearest cm
    EXPECT_EQ(snapped.x(), expected.x());
    EXPECT_EQ(snapped.y(), expected.y());
    EXPECT_EQ(snapped.z(), expected.z());
}

// Test placement when no same-size voxels nearby - NEW BEHAVIOR: exact 1cm positions
TEST_F(SmartSnappingTest, NoNearbyVoxelsSnapping) {
    using namespace VoxelData;
    
    WorldCoordinates worldPos(Vector3f(7.0f, 2.0f, 7.0f)); // Far from existing voxels
    IncrementCoordinates snapped = PlacementUtils::snapToSameSizeVoxel(worldPos, VoxelResolution::Size_32cm, *m_dataManager, false);
    
    // NEW BEHAVIOR: Should use exact 1cm position even when no nearby voxels
    // 7.0m = 700cm, 2.0m = 200cm (exact positions)
    IncrementCoordinates expected(700, 200, 700); // Exact 1cm positions
    EXPECT_EQ(snapped.x(), expected.x());
    EXPECT_EQ(snapped.y(), expected.y());
    EXPECT_EQ(snapped.z(), expected.z());
}

// Test sub-grid positioning on larger voxel surface faces
TEST_F(SmartSnappingTest, SurfaceFaceGridSnapping) {
    // REQ-3.2.2: Placement shall respect 1cm increment positions on the target face
    // REQ-3.2.3: The preview shall snap to nearest valid position
    using namespace VoxelData;
    
    // Test placing 1cm voxel on positive X surface face of 32cm voxel
    // 32cm voxel at (100, 0, 100) = (1.0m, 0, 1.0m) extends to (1.32m, 0.32m, 1.32m)
    WorldCoordinates hitPoint(Vector3f(1.32f, 0.1f, 1.1f)); // Hit point on the positive X surface face
    Math::IncrementCoordinates surfaceFaceVoxelPos(100, 0, 100); // 32cm voxel position (1.0m, 0, 1.0m)
    VoxelResolution surfaceFaceRes = VoxelResolution::Size_32cm;
    FaceDirection surfaceFaceDir = FaceDirection::PosX;
    VoxelResolution placementRes = VoxelResolution::Size_1cm;
    
    IncrementCoordinates snapped = PlacementUtils::snapToSurfaceFaceGrid(hitPoint, surfaceFaceVoxelPos, surfaceFaceRes, surfaceFaceDir, placementRes);
    
    // Should snap to surface face plane at X=1.32m (132 in 1cm increments)
    EXPECT_EQ(snapped.x(), 132); // On the surface face plane
    EXPECT_GE(snapped.y(), 0);   // Within surface face Y bounds
    EXPECT_LE(snapped.y(), 31);  // Within surface face Y bounds (32cm = 32 increments)
    EXPECT_GE(snapped.z(), 100); // Within surface face Z bounds  
    EXPECT_LE(snapped.z(), 131); // Within surface face Z bounds
}

// Test surface face snapping for all directions
TEST_F(SmartSnappingTest, SurfaceFaceAllDirections) {
    using namespace VoxelData;
    
    Math::IncrementCoordinates voxelPos(100, 100, 100); // 32cm voxel at (1.0m, 1.0m, 1.0m)
    VoxelResolution voxelRes = VoxelResolution::Size_32cm;
    VoxelResolution placementRes = VoxelResolution::Size_4cm;
    
    struct TestCase {
        FaceDirection direction;
        Vector3f hitPoint;
        int expectedConstrainedAxis; // Which axis should be constrained to surface face plane
    };
    
    TestCase testCases[] = {
        {FaceDirection::PosX, Vector3f(1.32f, 1.1f, 1.1f), 0}, // X constrained - positive X surface face
        {FaceDirection::NegX, Vector3f(1.0f, 1.1f, 1.1f), 0},  // X constrained - negative X surface face
        {FaceDirection::PosY, Vector3f(1.1f, 1.32f, 1.1f), 1}, // Y constrained - positive Y surface face
        {FaceDirection::NegY, Vector3f(1.1f, 1.0f, 1.1f), 1},  // Y constrained - negative Y surface face
        {FaceDirection::PosZ, Vector3f(1.1f, 1.1f, 1.32f), 2}, // Z constrained - positive Z surface face
        {FaceDirection::NegZ, Vector3f(1.1f, 1.1f, 1.0f), 2}   // Z constrained - negative Z surface face
    };
    
    for (const auto& testCase : testCases) {
        IncrementCoordinates snapped = PlacementUtils::snapToSurfaceFaceGrid(WorldCoordinates(testCase.hitPoint), voxelPos, voxelRes, testCase.direction, placementRes);
        
        // Verify position is within reasonable bounds
        EXPECT_GE(snapped.x(), 0);
        EXPECT_GE(snapped.y(), 0);
        EXPECT_GE(snapped.z(), 0);
        EXPECT_LT(snapped.x(), 1000);
        EXPECT_LT(snapped.y(), 1000);
        EXPECT_LT(snapped.z(), 1000);
        
        // For positive directions, the constrained axis should be at or beyond the voxel surface
        if (testCase.direction == FaceDirection::PosX) {
            EXPECT_GE(snapped.x(), 132); // At or beyond the positive X surface face (1.32m = 132cm)
        } else if (testCase.direction == FaceDirection::PosY) {
            EXPECT_GE(snapped.y(), 132); // At or beyond the positive Y surface face  
        } else if (testCase.direction == FaceDirection::PosZ) {
            EXPECT_GE(snapped.z(), 132); // At or beyond the positive Z surface face
        }
    }
}


// Test validation with smart snapping
TEST_F(SmartSnappingTest, SmartSnappingValidation) {
    using namespace VoxelData;
    
    // Test placement that would be invalid due to Y < 0
    WorldCoordinates invalidPos(Vector3f(3.0f, -0.5f, 3.0f));
    PlacementContext context = PlacementUtils::getSmartPlacementContext(
        invalidPos, VoxelResolution::Size_4cm, false, m_workspaceSize, *m_dataManager, nullptr);
    
    EXPECT_EQ(context.validation, PlacementValidationResult::InvalidYBelowZero);
    
    // Test valid placement
    WorldCoordinates validPos(Vector3f(1.0f, 1.0f, 1.0f));
    PlacementContext validContext = PlacementUtils::getSmartPlacementContext(
        validPos, VoxelResolution::Size_4cm, false, m_workspaceSize, *m_dataManager, nullptr);
    
    EXPECT_EQ(validContext.validation, PlacementValidationResult::Valid);
}

// Test edge cases for surface face snapping
TEST_F(SmartSnappingTest, SurfaceFaceEdgeCases) {
    using namespace VoxelData;
    
    // Test placing large voxel on small surface face (should be constrained)
    // 16cm voxel at (50, 0, 50) = (0.5m, 0, 0.5m) extends to (0.66m, 0.16m, 0.66m)
    WorldCoordinates hitPoint(Vector3f(0.55f, 0.16f, 0.55f)); // Hit point on positive Y surface face
    Math::IncrementCoordinates smallVoxelPos(50, 0, 50); // 16cm voxel at (0.5m, 0, 0.5m)
    VoxelResolution smallVoxelRes = VoxelResolution::Size_16cm;
    FaceDirection faceDir = FaceDirection::PosY;
    VoxelResolution largeVoxelRes = VoxelResolution::Size_32cm; // Larger than surface face!
    
    IncrementCoordinates snapped = PlacementUtils::snapToSurfaceFaceGrid(hitPoint, smallVoxelPos, smallVoxelRes, faceDir, largeVoxelRes);
    
    // Should be constrained to fit within the 16cm surface face bounds
    // The large 32cm voxel (0.32m) can't fit entirely on the 16cm surface face (0.16m wide)
    // So it gets clamped to fit as much as possible: max X = 0.66 - 0.32 = 0.34m
    WorldCoordinates snappedWorldCoords = Math::CoordinateConverter::incrementToWorld(snapped);
    Vector3f snappedWorld = snappedWorldCoords.value();
    EXPECT_GE(snappedWorld.x, 0.34f); // Clamped to fit within surface face
    EXPECT_LE(snappedWorld.x, 0.34f); // Should be exactly at clamped position
    
    // Verify the large voxel would extend properly
    EXPECT_LE(snappedWorld.x + 0.32f, 0.66f); // Large voxel should fit within surface face bounds
}

// Test REQ-2.2.4: All voxel sizes placeable at 1cm increments on ground plane
TEST_F(PlacementValidationTest, AllVoxelSizesOnGroundPlane1cmIncrements) {
    // REQ-2.2.4: All voxel sizes (1cm to 512cm) shall be placeable at any valid 1cm increment position on the ground plane
    // Test all resolutions from 1cm to 512cm
    std::vector<VoxelData::VoxelResolution> resolutions = {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_2cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_8cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_32cm,
        VoxelData::VoxelResolution::Size_64cm,
        VoxelData::VoxelResolution::Size_128cm,
        VoxelData::VoxelResolution::Size_256cm,
        VoxelData::VoxelResolution::Size_512cm
    };
    
    // The key requirement is that with Shift pressed, all voxel sizes 
    // can be placed at 1cm increments on the ground plane
    for (auto resolution : resolutions) {
        // Test various 1cm increment positions
        std::vector<float> testPositions = {0.0f, 0.01f, 0.05f, 0.13f, 0.27f, 0.99f, 1.23f};
        
        for (float xPos : testPositions) {
            for (float zPos : testPositions) {
                WorldCoordinates worldPos(Vector3f(xPos, 0.0f, zPos));
                
                // With shift key, should snap to exact 1cm increments
                IncrementCoordinates snapped = PlacementUtils::snapToValidIncrement(worldPos);
                
                // Verify snapping is correct
                EXPECT_EQ(snapped.x(), static_cast<int>(std::round(xPos * 100)));
                EXPECT_EQ(snapped.y(), 0);
                EXPECT_EQ(snapped.z(), static_cast<int>(std::round(zPos * 100)));
                
                // Also verify through the full placement context with shift
                // Use a workspace size appropriate for the voxel
                float voxelSize = VoxelData::getVoxelSize(resolution);
                Vector3f workspaceForVoxel(
                    std::max(8.0f, voxelSize * 2 + 1.0f),
                    std::max(8.0f, voxelSize * 2 + 1.0f),
                    std::max(8.0f, voxelSize * 2 + 1.0f)
                );
                
                PlacementContext context = PlacementUtils::getPlacementContext(
                    worldPos, resolution, true, workspaceForVoxel);
                
                // With shift, should snap to 1cm increments regardless of voxel size
                EXPECT_EQ(context.snappedIncrementPos.x(), static_cast<int>(std::round(xPos * 100)));
                EXPECT_EQ(context.snappedIncrementPos.y(), 0);
                EXPECT_EQ(context.snappedIncrementPos.z(), static_cast<int>(std::round(zPos * 100)));
            }
        }
    }
    
    // Also verify that without shift, voxels snap to their natural grid
    for (auto resolution : resolutions) {
        WorldCoordinates testPos(Vector3f(1.234f, 0.0f, 1.234f));
        IncrementCoordinates snappedNoShift = PlacementUtils::snapToGridAligned(testPos, resolution, false);
        IncrementCoordinates snappedWithShift = PlacementUtils::snapToGridAligned(testPos, resolution, true);
        
        // NEW BEHAVIOR: Both shift and no-shift should place at exact 1cm positions
        // No longer snap to voxel-size grid - all placements use 1cm increments
        EXPECT_EQ(snappedNoShift.x(), 123);   // 1.23m = 123cm (exact)
        EXPECT_EQ(snappedNoShift.z(), 123);   // 1.23m = 123cm (exact)
        
        // With shift, should also snap to 1cm increments (same as no-shift)
        EXPECT_EQ(snappedWithShift.x(), 123);
        EXPECT_EQ(snappedWithShift.z(), 123);
    }
}

// ===== New Tests for 1cm Increment Placement Validation (UNIT-005) =====

TEST_F(PlacementValidationTest, Arbitrary1cmPositions_AllVoxelSizes) {
    // Test that all voxel sizes can be placed at arbitrary 1cm positions
    // This validates the new requirements where voxels aren't constrained to resolution-based grids
    
    std::vector<VoxelData::VoxelResolution> testResolutions = {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_32cm,
        VoxelData::VoxelResolution::Size_64cm
    };
    
    // Test various arbitrary 1cm positions
    std::vector<Math::IncrementCoordinates> testPositions = {
        Math::IncrementCoordinates(13, 0, 17),   // 0.13m, 0, 0.17m
        Math::IncrementCoordinates(25, 0, 39),   // 0.25m, 0, 0.39m
        Math::IncrementCoordinates(41, 0, 53),   // 0.41m, 0, 0.53m
        Math::IncrementCoordinates(67, 0, 71),   // 0.67m, 0, 0.71m
        Math::IncrementCoordinates(83, 0, 97),   // 0.83m, 0, 0.97m
    };
    
    for (auto resolution : testResolutions) {
        for (auto position : testPositions) {
            // Convert to world coordinates
            Math::WorldCoordinates worldCoords = Math::CoordinateConverter::incrementToWorld(position);
            
            // Test placement context validation
            PlacementContext context = PlacementUtils::getPlacementContext(
                worldCoords, resolution, true, Vector3f(8.0f, 8.0f, 8.0f));
            
            // Should be valid at exact 1cm positions
            EXPECT_EQ(context.validation, PlacementValidationResult::Valid) 
                << "Failed for " << static_cast<int>(resolution) << "cm voxel at position (" 
                << position.x() << ", " << position.y() << ", " << position.z() << ")";
            
            // Should maintain exact position (with shift key allowing 1cm increments)
            EXPECT_EQ(context.snappedIncrementPos.x(), position.x());
            EXPECT_EQ(context.snappedIncrementPos.y(), position.y()); 
            EXPECT_EQ(context.snappedIncrementPos.z(), position.z());
        }
    }
}

TEST_F(PlacementValidationTest, NonAlignedPositions_NoResolutionConstraints) {
    // Test that voxels can be placed at positions that don't align with their resolution grid
    // This validates removal of snapToVoxelResolution behavior
    
    // Test a 32cm voxel at positions that don't align with 32cm grid
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    std::vector<Math::IncrementCoordinates> nonAlignedPositions = {
        Math::IncrementCoordinates(1, 0, 1),     // 0.01m, 0, 0.01m (not aligned to 32cm)
        Math::IncrementCoordinates(15, 0, 15),   // 0.15m, 0, 0.15m (not aligned to 32cm)
        Math::IncrementCoordinates(23, 0, 23),   // 0.23m, 0, 0.23m (not aligned to 32cm)
        Math::IncrementCoordinates(31, 0, 31),   // 0.31m, 0, 0.31m (not aligned to 32cm)
        Math::IncrementCoordinates(33, 0, 33),   // 0.33m, 0, 0.33m (not aligned to 32cm)
        Math::IncrementCoordinates(47, 0, 47),   // 0.47m, 0, 0.47m (not aligned to 32cm)
    };
    
    for (auto position : nonAlignedPositions) {
        Math::WorldCoordinates worldCoords = Math::CoordinateConverter::incrementToWorld(position);
        
        // With shift key (allowing 1cm increments), should be valid
        PlacementContext contextWithShift = PlacementUtils::getPlacementContext(
            worldCoords, resolution, true, Vector3f(8.0f, 8.0f, 8.0f));
        
        EXPECT_EQ(contextWithShift.validation, PlacementValidationResult::Valid)
            << "32cm voxel should be placeable at 1cm position (" 
            << position.x() << ", " << position.y() << ", " << position.z() << ") with shift";
        
        // Should maintain exact position
        EXPECT_EQ(contextWithShift.snappedIncrementPos.x(), position.x());
        EXPECT_EQ(contextWithShift.snappedIncrementPos.y(), position.y());
        EXPECT_EQ(contextWithShift.snappedIncrementPos.z(), position.z());
        
        // Verify that old behavior would have snapped these to 32cm grid
        // (i.e., positions like 1, 15, 23, 31, 33, 47 are NOT multiples of 32)
        EXPECT_NE(position.x() % 32, 0) << "Test position should not be aligned to 32cm grid";
        EXPECT_NE(position.z() % 32, 0) << "Test position should not be aligned to 32cm grid";
    }
}

TEST_F(PlacementValidationTest, PlacementContext_ExactPositions) {
    // Test that the complete placement context system works with exact 1cm positions
    // This validates that all components work together without snapping
    
    struct TestCase {
        Vector3f worldPos;
        VoxelData::VoxelResolution resolution;
        bool shiftPressed;
        Math::IncrementCoordinates expectedPosition;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        // 1cm voxel at arbitrary positions
        {Vector3f(0.07f, 0.0f, 0.13f), VoxelData::VoxelResolution::Size_1cm, true, 
         Math::IncrementCoordinates(7, 0, 13), "1cm voxel at arbitrary position"},
        
        // 4cm voxel at non-4cm-aligned position
        {Vector3f(0.18f, 0.0f, 0.22f), VoxelData::VoxelResolution::Size_4cm, true,
         Math::IncrementCoordinates(18, 0, 22), "4cm voxel at non-aligned position"},
        
        // 16cm voxel at non-16cm-aligned position  
        {Vector3f(0.33f, 0.0f, 0.41f), VoxelData::VoxelResolution::Size_16cm, true,
         Math::IncrementCoordinates(33, 0, 41), "16cm voxel at non-aligned position"},
        
        // 32cm voxel at non-32cm-aligned position
        {Vector3f(0.19f, 0.0f, 0.27f), VoxelData::VoxelResolution::Size_32cm, true,
         Math::IncrementCoordinates(19, 0, 27), "32cm voxel at non-aligned position"},
        
        // Test rounding behavior
        {Vector3f(0.126f, 0.0f, 0.234f), VoxelData::VoxelResolution::Size_8cm, true,
         Math::IncrementCoordinates(13, 0, 23), "8cm voxel with rounding"},
    };
    
    for (const auto& testCase : testCases) {
        PlacementContext context = PlacementUtils::getPlacementContext(
            WorldCoordinates(testCase.worldPos), testCase.resolution, testCase.shiftPressed, Vector3f(8.0f, 8.0f, 8.0f));
        
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid)
            << "Failed validation for: " << testCase.description;
        
        EXPECT_EQ(context.snappedIncrementPos.x(), testCase.expectedPosition.x())
            << "X position mismatch for: " << testCase.description;
        EXPECT_EQ(context.snappedIncrementPos.y(), testCase.expectedPosition.y())
            << "Y position mismatch for: " << testCase.description;
        EXPECT_EQ(context.snappedIncrementPos.z(), testCase.expectedPosition.z())
            << "Z position mismatch for: " << testCase.description;
        
        // Verify the resolution is preserved
        EXPECT_EQ(context.resolution, testCase.resolution)
            << "Resolution mismatch for: " << testCase.description;
        
        // Verify shift state is preserved
        EXPECT_EQ(context.shiftPressed, testCase.shiftPressed)
            << "Shift state mismatch for: " << testCase.description;
    }
}