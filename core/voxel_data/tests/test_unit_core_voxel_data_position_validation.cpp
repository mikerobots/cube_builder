#include <gtest/gtest.h>
#include "../VoxelDataManager.h"
#include "../../../foundation/math/CoordinateTypes.h"
#include "../../../foundation/math/CoordinateConverter.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

class VoxelDataPositionValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        voxelManager = std::make_unique<VoxelDataManager>();
        // Set default workspace size to 5m (500cm)
        voxelManager->resizeWorkspace(5.0f);
    }
    
    std::unique_ptr<VoxelDataManager> voxelManager;
};

// Test PositionValidation struct
TEST_F(VoxelDataPositionValidationTest, PositionValidationDefaultConstruction) {
    PositionValidation validation;
    EXPECT_FALSE(validation.valid);
    EXPECT_FALSE(validation.withinBounds);
    EXPECT_FALSE(validation.aboveGroundPlane);
    EXPECT_FALSE(validation.alignedToGrid);
    EXPECT_TRUE(validation.noOverlap);
    EXPECT_TRUE(validation.errorMessage.empty());
}

TEST_F(VoxelDataPositionValidationTest, PositionValidationConstructor) {
    PositionValidation validation(true, "Success");
    EXPECT_TRUE(validation.valid);
    EXPECT_EQ(validation.errorMessage, "Success");
}

// Test validatePosition method
TEST_F(VoxelDataPositionValidationTest, ValidatePosition_ValidPosition) {
    IncrementCoordinates pos(0, 0, 0);
    auto result = voxelManager->validatePosition(pos, VoxelResolution::Size_1cm);
    
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.withinBounds);
    EXPECT_TRUE(result.aboveGroundPlane);
    EXPECT_TRUE(result.alignedToGrid);
    EXPECT_TRUE(result.noOverlap);
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(VoxelDataPositionValidationTest, ValidatePosition_BelowGroundPlane) {
    IncrementCoordinates pos(0, -1, 0);
    auto result = voxelManager->validatePosition(pos, VoxelResolution::Size_1cm);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.aboveGroundPlane);
    EXPECT_EQ(result.errorMessage, "Position is below ground plane (Y must be >= 0)");
}

TEST_F(VoxelDataPositionValidationTest, ValidatePosition_OutsideWorkspace) {
    // Workspace is 5m (500cm), so 300cm is outside
    IncrementCoordinates pos(300, 0, 0);
    auto result = voxelManager->validatePosition(pos, VoxelResolution::Size_1cm);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.withinBounds);
    EXPECT_EQ(result.errorMessage, "Position is outside workspace bounds");
}

TEST_F(VoxelDataPositionValidationTest, ValidatePosition_NotAlignedToGrid) {
    // For 4cm voxels, position must be multiple of 4
    IncrementCoordinates pos(3, 0, 0);  // Not aligned to 4cm grid
    auto result = voxelManager->validatePosition(pos, VoxelResolution::Size_4cm);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.alignedToGrid);
    EXPECT_EQ(result.errorMessage, "Position is not aligned to voxel grid");
}

TEST_F(VoxelDataPositionValidationTest, ValidatePosition_WithOverlap) {
    // Place a voxel first
    IncrementCoordinates pos(0, 0, 0);
    voxelManager->setVoxel(pos, VoxelResolution::Size_1cm, true);
    
    // Try to validate same position - should detect overlap
    auto result = voxelManager->validatePosition(pos, VoxelResolution::Size_1cm);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.noOverlap);
    EXPECT_EQ(result.errorMessage, "Position would overlap with existing voxel");
}

TEST_F(VoxelDataPositionValidationTest, ValidatePosition_SkipOverlapCheck) {
    // Place a voxel first
    IncrementCoordinates pos(0, 0, 0);
    voxelManager->setVoxel(pos, VoxelResolution::Size_1cm, true);
    
    // Validate same position but skip overlap check
    auto result = voxelManager->validatePosition(pos, VoxelResolution::Size_1cm, false);
    
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.noOverlap);  // Should remain true when check is skipped
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(VoxelDataPositionValidationTest, ValidatePosition_WorldCoordinates) {
    WorldCoordinates worldPos(0.0f, 0.0f, 0.0f);
    auto result = voxelManager->validatePosition(worldPos, VoxelResolution::Size_1cm);
    
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.withinBounds);
    EXPECT_TRUE(result.aboveGroundPlane);
    EXPECT_TRUE(result.alignedToGrid);
    EXPECT_TRUE(result.noOverlap);
}

// Test individual validation methods
TEST_F(VoxelDataPositionValidationTest, IsWithinWorkspaceBounds) {
    // Within bounds
    EXPECT_TRUE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(0, 0, 0)));
    EXPECT_TRUE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(200, 100, 200)));
    
    // Outside bounds (workspace is 5m = 500cm, centered at origin)
    EXPECT_FALSE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(300, 0, 0)));
    EXPECT_FALSE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(0, 600, 0)));
    EXPECT_FALSE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(0, 0, 300)));
}

TEST_F(VoxelDataPositionValidationTest, IsAboveGroundPlane) {
    EXPECT_TRUE(voxelManager->isAboveGroundPlane(IncrementCoordinates(0, 0, 0)));
    EXPECT_TRUE(voxelManager->isAboveGroundPlane(IncrementCoordinates(0, 100, 0)));
    EXPECT_FALSE(voxelManager->isAboveGroundPlane(IncrementCoordinates(0, -1, 0)));
    EXPECT_FALSE(voxelManager->isAboveGroundPlane(IncrementCoordinates(0, -100, 0)));
}

TEST_F(VoxelDataPositionValidationTest, IsAlignedToGrid) {
    // 1cm resolution - all positions are aligned
    EXPECT_TRUE(voxelManager->isAlignedToGrid(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm));
    EXPECT_TRUE(voxelManager->isAlignedToGrid(IncrementCoordinates(1, 2, 3), VoxelResolution::Size_1cm));
    
    // 4cm resolution - must be multiple of 4
    EXPECT_TRUE(voxelManager->isAlignedToGrid(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_4cm));
    EXPECT_TRUE(voxelManager->isAlignedToGrid(IncrementCoordinates(4, 8, 12), VoxelResolution::Size_4cm));
    EXPECT_FALSE(voxelManager->isAlignedToGrid(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_4cm));
    EXPECT_FALSE(voxelManager->isAlignedToGrid(IncrementCoordinates(0, 2, 0), VoxelResolution::Size_4cm));
    
    // 16cm resolution - must be multiple of 16
    EXPECT_TRUE(voxelManager->isAlignedToGrid(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_16cm));
    EXPECT_TRUE(voxelManager->isAlignedToGrid(IncrementCoordinates(16, 32, 48), VoxelResolution::Size_16cm));
    EXPECT_FALSE(voxelManager->isAlignedToGrid(IncrementCoordinates(15, 0, 0), VoxelResolution::Size_16cm));
}

// Test position utility methods
TEST_F(VoxelDataPositionValidationTest, SnapToGrid_1cm) {
    // 1cm resolution - all positions snap to themselves
    auto snapped = voxelManager->snapToGrid(IncrementCoordinates(3, 5, 7), VoxelResolution::Size_1cm);
    EXPECT_EQ(snapped.x(), 3);
    EXPECT_EQ(snapped.y(), 5);
    EXPECT_EQ(snapped.z(), 7);
}

TEST_F(VoxelDataPositionValidationTest, SnapToGrid_4cm) {
    // Test snapping to 4cm grid
    auto snapped1 = voxelManager->snapToGrid(IncrementCoordinates(3, 5, 7), VoxelResolution::Size_4cm);
    EXPECT_EQ(snapped1.x(), 0);  // 3 -> 0
    EXPECT_EQ(snapped1.y(), 4);  // 5 -> 4
    EXPECT_EQ(snapped1.z(), 4);  // 7 -> 4
    
    auto snapped2 = voxelManager->snapToGrid(IncrementCoordinates(2, 6, 10), VoxelResolution::Size_4cm);
    EXPECT_EQ(snapped2.x(), 0);  // 2 -> 0
    EXPECT_EQ(snapped2.y(), 4);  // 6 -> 4
    EXPECT_EQ(snapped2.z(), 8);  // 10 -> 8
}

TEST_F(VoxelDataPositionValidationTest, SnapToGrid_16cm) {
    // Test snapping to 16cm grid
    auto snapped = voxelManager->snapToGrid(IncrementCoordinates(15, 20, 35), VoxelResolution::Size_16cm);
    EXPECT_EQ(snapped.x(), 0);   // 15 -> 0
    EXPECT_EQ(snapped.y(), 16);  // 20 -> 16
    EXPECT_EQ(snapped.z(), 32);  // 35 -> 32
}

TEST_F(VoxelDataPositionValidationTest, ClampToWorkspace) {
    // Workspace is 5m (500cm), centered at origin
    // X and Z range: -250 to 250
    // Y range: 0 to 500
    
    // Within bounds - should not change
    auto clamped1 = voxelManager->clampToWorkspace(IncrementCoordinates(100, 200, -100));
    EXPECT_EQ(clamped1.x(), 100);
    EXPECT_EQ(clamped1.y(), 200);
    EXPECT_EQ(clamped1.z(), -100);
    
    // Outside bounds - should clamp
    auto clamped2 = voxelManager->clampToWorkspace(IncrementCoordinates(300, 600, -300));
    EXPECT_EQ(clamped2.x(), 250);   // Clamped to max X
    EXPECT_EQ(clamped2.y(), 500);   // Clamped to max Y
    EXPECT_EQ(clamped2.z(), -250);  // Clamped to min Z
    
    // Below ground - should clamp Y to 0
    auto clamped3 = voxelManager->clampToWorkspace(IncrementCoordinates(0, -50, 0));
    EXPECT_EQ(clamped3.x(), 0);
    EXPECT_EQ(clamped3.y(), 0);  // Clamped to ground
    EXPECT_EQ(clamped3.z(), 0);
}

// Test edge cases
TEST_F(VoxelDataPositionValidationTest, EdgeCase_WorkspaceBoundary) {
    // Test positions exactly at workspace boundaries
    IncrementCoordinates maxX(250, 0, 0);
    IncrementCoordinates maxY(0, 500, 0);
    IncrementCoordinates maxZ(0, 0, 250);
    
    EXPECT_TRUE(voxelManager->isWithinWorkspaceBounds(maxX));
    EXPECT_TRUE(voxelManager->isWithinWorkspaceBounds(maxY));
    EXPECT_TRUE(voxelManager->isWithinWorkspaceBounds(maxZ));
    
    // Just outside boundaries
    IncrementCoordinates beyondX(251, 0, 0);
    IncrementCoordinates beyondY(0, 501, 0);
    IncrementCoordinates beyondZ(0, 0, 251);
    
    EXPECT_FALSE(voxelManager->isWithinWorkspaceBounds(beyondX));
    EXPECT_FALSE(voxelManager->isWithinWorkspaceBounds(beyondY));
    EXPECT_FALSE(voxelManager->isWithinWorkspaceBounds(beyondZ));
}

TEST_F(VoxelDataPositionValidationTest, EdgeCase_LargeVoxelAlignment) {
    // Test alignment for largest voxel size (512cm)
    EXPECT_TRUE(voxelManager->isAlignedToGrid(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_512cm));
    EXPECT_TRUE(voxelManager->isAlignedToGrid(IncrementCoordinates(512, 1024, -512), VoxelResolution::Size_512cm));
    EXPECT_FALSE(voxelManager->isAlignedToGrid(IncrementCoordinates(256, 0, 0), VoxelResolution::Size_512cm));
    EXPECT_FALSE(voxelManager->isAlignedToGrid(IncrementCoordinates(0, 511, 0), VoxelResolution::Size_512cm));
}

// Test different workspace sizes
TEST_F(VoxelDataPositionValidationTest, DifferentWorkspaceSizes) {
    // Change to minimum workspace (2m = 200cm)
    voxelManager->resizeWorkspace(2.0f);
    
    // X and Z range: -100 to 100
    EXPECT_TRUE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(50, 0, 50)));
    EXPECT_TRUE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(100, 0, -100)));
    EXPECT_FALSE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(150, 0, 0)));
    
    // Change to maximum workspace (8m = 800cm)
    voxelManager->resizeWorkspace(8.0f);
    
    // X and Z range: -400 to 400
    EXPECT_TRUE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(300, 0, -300)));
    EXPECT_TRUE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(400, 0, 400)));
    EXPECT_FALSE(voxelManager->isWithinWorkspaceBounds(IncrementCoordinates(500, 0, 0)));
}