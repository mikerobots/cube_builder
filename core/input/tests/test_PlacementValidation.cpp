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
        Vector3f worldPos(0.0f, 0.0f, 0.0f);
        IncrementCoordinates snapped = PlacementUtils::snapToValidIncrement(worldPos);
        EXPECT_EQ(snapped.x(), 0);
        EXPECT_EQ(snapped.y(), 0);
        EXPECT_EQ(snapped.z(), 0);
    }
    
    // Test positions that need rounding up
    {
        Vector3f worldPos(0.126f, 0.238f, 0.359f);
        IncrementCoordinates snapped = PlacementUtils::snapToValidIncrement(worldPos);
        EXPECT_EQ(snapped.x(), 13); // 0.126 -> 0.13 -> 13
        EXPECT_EQ(snapped.y(), 24); // 0.238 -> 0.24 -> 24
        EXPECT_EQ(snapped.z(), 36); // 0.359 -> 0.36 -> 36
    }
    
    // Test positions that need rounding down
    {
        Vector3f worldPos(0.123f, 0.234f, 0.345f);
        IncrementCoordinates snapped = PlacementUtils::snapToValidIncrement(worldPos);
        EXPECT_EQ(snapped.x(), 12); // 0.123 -> 0.12 -> 12
        EXPECT_EQ(snapped.y(), 23); // 0.234 -> 0.23 -> 23
        EXPECT_EQ(snapped.z(), 35); // 0.345 -> 0.35 -> 35
    }
    
    // Test negative positions
    {
        Vector3f worldPos(-0.126f, -0.238f, -0.359f);
        IncrementCoordinates snapped = PlacementUtils::snapToValidIncrement(worldPos);
        EXPECT_EQ(snapped.x(), -13);
        EXPECT_EQ(snapped.y(), -24);
        EXPECT_EQ(snapped.z(), -36);
    }
}

// Test grid-aligned snapping for same-size voxels
TEST_F(PlacementValidationTest, SnapToGridAligned) {
    // REQ-3.1.1: Same-size voxels shall auto-snap to perfect alignment by default
    // REQ-3.1.2: Holding Shift shall allow placement at any valid 1cm increment
    // Test 32cm voxel without shift (should snap to 32cm grid)
    {
        Vector3f worldPos(0.15f, 0.15f, 0.15f);
        IncrementCoordinates snapped = PlacementUtils::snapToGridAligned(worldPos, 
            VoxelData::VoxelResolution::Size_32cm, false);
        EXPECT_EQ(snapped.x(), 0);   // Snaps to 0
        EXPECT_EQ(snapped.y(), 0);   // Snaps to 0
        EXPECT_EQ(snapped.z(), 0);   // Snaps to 0
    }
    
    // Test 32cm voxel with shift (should allow 1cm increments)
    {
        Vector3f worldPos(0.15f, 0.15f, 0.15f);
        IncrementCoordinates snapped = PlacementUtils::snapToGridAligned(worldPos, 
            VoxelData::VoxelResolution::Size_32cm, true);
        EXPECT_EQ(snapped.x(), 15);  // 0.15 -> 15cm
        EXPECT_EQ(snapped.y(), 15);
        EXPECT_EQ(snapped.z(), 15);
    }
    
    // Test 16cm voxel without shift
    {
        Vector3f worldPos(0.25f, 0.25f, 0.25f);
        IncrementCoordinates snapped = PlacementUtils::snapToGridAligned(worldPos, 
            VoxelData::VoxelResolution::Size_16cm, false);
        EXPECT_EQ(snapped.x(), 16);  // Floor(25/16)*16 = 1*16 = 16cm
        EXPECT_EQ(snapped.y(), 16);
        EXPECT_EQ(snapped.z(), 16);
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

// Test workspace bounds validation
TEST_F(PlacementValidationTest, ValidateWorkspaceBounds) {
    // REQ-5.2.2: System shall validate placement before allowing it
    // Test position within bounds (workspace is 5m centered at origin)
    {
        IncrementCoordinates gridPos(100, 100, 100); // 1m, 1m, 1m
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::Valid);
    }
    
    // Test position outside X bounds (negative)
    {
        IncrementCoordinates gridPos(-300, 100, 100); // -3m (outside -2.5m bound)
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::InvalidOutOfBounds);
    }
    
    // Test position outside X bounds (positive)
    {
        IncrementCoordinates gridPos(300, 100, 100); // 3m (outside 2.5m bound)
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::InvalidOutOfBounds);
    }
    
    // Test position outside Y bounds (too high)
    {
        IncrementCoordinates gridPos(100, 600, 100); // 6m (outside 5m height)
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_1cm, m_workspaceSize);
        EXPECT_EQ(result, PlacementValidationResult::InvalidOutOfBounds);
    }
    
    // Test large voxel that would extend outside bounds
    {
        IncrementCoordinates gridPos(200, 100, 200); // 2m, 1m, 2m
        auto result = PlacementUtils::validatePlacement(gridPos, 
            VoxelData::VoxelResolution::Size_64cm, m_workspaceSize);
        // Voxel at 2m + 0.64m size = 2.64m > 2.5m bound
        EXPECT_EQ(result, PlacementValidationResult::InvalidOutOfBounds);
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
    // Test valid placement without shift
    {
        Vector3f worldPos(1.15f, 0.5f, 1.15f);
        PlacementContext context = PlacementUtils::getPlacementContext(
            worldPos, VoxelData::VoxelResolution::Size_32cm, false, m_workspaceSize);
        
        // 1.15m = 115cm, Floor(115/32) = 3, 3*32 = 96cm
        EXPECT_EQ(context.snappedIncrementPos.x(), 96);   // Floor(115/32)*32 = 96cm
        EXPECT_EQ(context.snappedIncrementPos.y(), 32);   // Floor(50/32)*32 = 32cm
        EXPECT_EQ(context.snappedIncrementPos.z(), 96);   // Floor(115/32)*32 = 96cm
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid);
        EXPECT_FALSE(context.shiftPressed);
    }
    
    // Test valid placement with shift
    {
        Vector3f worldPos(1.15f, 0.5f, 1.15f);
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
        Vector3f worldPos(1.0f, -0.5f, 1.0f);
        PlacementContext context = PlacementUtils::getPlacementContext(
            worldPos, VoxelData::VoxelResolution::Size_32cm, false, m_workspaceSize);
        
        EXPECT_EQ(context.validation, PlacementValidationResult::InvalidYBelowZero);
    }
    
    // Test invalid placement (out of bounds)
    {
        Vector3f worldPos(3.0f, 1.0f, 3.0f); // Outside 2.5m bound
        PlacementContext context = PlacementUtils::getPlacementContext(
            worldPos, VoxelData::VoxelResolution::Size_32cm, false, m_workspaceSize);
        
        EXPECT_EQ(context.validation, PlacementValidationResult::InvalidOutOfBounds);
    }
}

// Test snap override with Shift key for all resolutions
TEST_F(PlacementValidationTest, ShiftKeyOverrideAllResolutions) {
    // REQ-3.1.2: Holding Shift shall allow placement at any valid 1cm increment
    // REQ-5.4.1: Shift key shall override auto-snap for same-size voxels
    Vector3f testPos(0.123f, 0.234f, 0.345f);
    
    // Test all resolutions
    for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
        auto resolution = static_cast<VoxelData::VoxelResolution>(i);
        
        // Without shift - should snap to voxel grid
        IncrementCoordinates snappedNoShift = PlacementUtils::snapToGridAligned(testPos, resolution, false);
        
        // With shift - should snap to 1cm increments
        IncrementCoordinates snappedWithShift = PlacementUtils::snapToGridAligned(testPos, resolution, true);
        
        // With shift, all resolutions should give same result (1cm increments)
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

// Test same-size voxel auto-snapping
TEST_F(SmartSnappingTest, SameSizeVoxelSnapping) {
    // REQ-3.1.1: Same-size voxels shall auto-snap to perfect alignment by default
    using namespace VoxelData;
    
    // Test snapping to 32cm grid when placing 32cm voxel near existing 32cm voxels
    Vector3f worldPos(3.35f, 0.0f, 3.35f); // Slightly off 32cm grid
    IncrementCoordinates snapped = PlacementUtils::snapToSameSizeVoxel(worldPos, VoxelResolution::Size_32cm, *m_dataManager, false);
    
    // Should snap to 32cm grid (32cm = 0.32m = 32 increments)
    // 3.35m / 0.32m = 10.46875 rounds to 10, then 10 * 32 = 320
    IncrementCoordinates expected(320, 0, 320); // Grid-aligned to 32cm voxel size
    EXPECT_EQ(snapped.x(), expected.x());
    EXPECT_EQ(snapped.y(), expected.y());
    EXPECT_EQ(snapped.z(), expected.z());
}

// Test same-size snapping with Shift override
TEST_F(SmartSnappingTest, SameSizeSnappingShiftOverride) {
    // REQ-3.1.2: Holding Shift shall allow placement at any valid 1cm increment
    // REQ-5.4.1: Shift key shall override auto-snap for same-size voxels
    using namespace VoxelData;
    
    Vector3f worldPos(3.35f, 0.0f, 3.35f);
    IncrementCoordinates snapped = PlacementUtils::snapToSameSizeVoxel(worldPos, VoxelResolution::Size_32cm, *m_dataManager, true);
    
    // With Shift pressed, should snap to 1cm increments regardless of nearby voxels
    IncrementCoordinates expected(335, 0, 335); // 3.35m rounded to nearest cm
    EXPECT_EQ(snapped.x(), expected.x());
    EXPECT_EQ(snapped.y(), expected.y());
    EXPECT_EQ(snapped.z(), expected.z());
}

// Test snapping when no same-size voxels nearby
TEST_F(SmartSnappingTest, NoNearbyVoxelsSnapping) {
    using namespace VoxelData;
    
    Vector3f worldPos(7.0f, 2.0f, 7.0f); // Far from existing voxels
    IncrementCoordinates snapped = PlacementUtils::snapToSameSizeVoxel(worldPos, VoxelResolution::Size_32cm, *m_dataManager, false);
    
    // Should still snap to grid (no special behavior when no nearby voxels)
    // 7.0m = 700cm, Floor(700/32) = 21, 21*32 = 672cm
    IncrementCoordinates expected(672, 192, 672); // Grid-aligned for 32cm
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
    Vector3f hitPoint(1.32f, 0.1f, 1.1f); // Hit point on the positive X surface face
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
        IncrementCoordinates snapped = PlacementUtils::snapToSurfaceFaceGrid(testCase.hitPoint, voxelPos, voxelRes, testCase.direction, placementRes);
        
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

// Test smart placement context selection
TEST_F(SmartSnappingTest, SmartPlacementContext) {
    using namespace VoxelData;
    
    Vector3f worldPos(3.35f, 0.1f, 3.35f);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    // Test without surface face (should use same-size snapping)
    PlacementContext contextNoFace = PlacementUtils::getSmartPlacementContext(
        worldPos, resolution, false, m_workspaceSize, *m_dataManager, nullptr);
    
    EXPECT_EQ(contextNoFace.resolution, resolution);
    EXPECT_FALSE(contextNoFace.shiftPressed);
    EXPECT_EQ(contextNoFace.validation, PlacementValidationResult::Valid);
    
    // Test with surface face (should use surface face grid snapping)
    Math::IncrementCoordinates surfaceFacePos(100, 0, 100);
    PlacementContext contextWithFace = PlacementUtils::getSmartPlacementContext(
        worldPos, VoxelResolution::Size_1cm, false, m_workspaceSize, *m_dataManager, 
        &surfaceFacePos, VoxelResolution::Size_32cm, FaceDirection::PosX);
    
    EXPECT_EQ(contextWithFace.resolution, VoxelResolution::Size_1cm);
    EXPECT_FALSE(contextWithFace.shiftPressed);
    EXPECT_EQ(contextWithFace.validation, PlacementValidationResult::Valid);
    
    // The snapped positions should be different due to different snapping methods
    EXPECT_NE(contextNoFace.snappedIncrementPos.x(), contextWithFace.snappedIncrementPos.x());
}

// Test validation with smart snapping
TEST_F(SmartSnappingTest, SmartSnappingValidation) {
    using namespace VoxelData;
    
    // Test placement that would be invalid due to Y < 0
    Vector3f invalidPos(3.0f, -0.5f, 3.0f);
    PlacementContext context = PlacementUtils::getSmartPlacementContext(
        invalidPos, VoxelResolution::Size_4cm, false, m_workspaceSize, *m_dataManager, nullptr);
    
    EXPECT_EQ(context.validation, PlacementValidationResult::InvalidYBelowZero);
    
    // Test valid placement
    Vector3f validPos(1.0f, 1.0f, 1.0f);
    PlacementContext validContext = PlacementUtils::getSmartPlacementContext(
        validPos, VoxelResolution::Size_4cm, false, m_workspaceSize, *m_dataManager, nullptr);
    
    EXPECT_EQ(validContext.validation, PlacementValidationResult::Valid);
}

// Test edge cases for surface face snapping
TEST_F(SmartSnappingTest, SurfaceFaceEdgeCases) {
    using namespace VoxelData;
    
    // Test placing large voxel on small surface face (should be constrained)
    // 16cm voxel at (50, 0, 50) = (0.5m, 0, 0.5m) extends to (0.66m, 0.16m, 0.66m)
    Vector3f hitPoint(0.55f, 0.16f, 0.55f); // Hit point on positive Y surface face
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
                Vector3f worldPos(xPos, 0.0f, zPos);
                
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
        Vector3f testPos(1.234f, 0.0f, 1.234f);
        IncrementCoordinates snappedNoShift = PlacementUtils::snapToGridAligned(testPos, resolution, false);
        IncrementCoordinates snappedWithShift = PlacementUtils::snapToGridAligned(testPos, resolution, true);
        
        // Without shift, should snap to voxel-size grid
        float voxelSize = VoxelData::getVoxelSize(resolution);
        float voxelSizeCm = voxelSize * 100.0f;
        EXPECT_EQ(snappedNoShift.x() % static_cast<int>(voxelSizeCm), 0);
        EXPECT_EQ(snappedNoShift.z() % static_cast<int>(voxelSizeCm), 0);
        
        // With shift, should snap to 1cm increments
        EXPECT_EQ(snappedWithShift.x(), 123);
        EXPECT_EQ(snappedWithShift.z(), 123);
    }
}