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

TEST_F(VoxelDataPositionValidationTest, ValidatePosition_AllPositionsAligned) {
    // With new requirements, all 1cm increment positions are valid regardless of resolution
    IncrementCoordinates pos1(3, 0, 0);  // Any 1cm position is valid
    auto result1 = voxelManager->validatePosition(pos1, VoxelResolution::Size_4cm);
    
    EXPECT_TRUE(result1.valid);
    EXPECT_TRUE(result1.alignedToGrid);  // All 1cm positions are considered aligned
    
    // Test with different resolutions and positions
    IncrementCoordinates pos2(7, 0, 5);
    auto result2 = voxelManager->validatePosition(pos2, VoxelResolution::Size_16cm);
    
    EXPECT_TRUE(result2.valid);
    EXPECT_TRUE(result2.alignedToGrid);
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


// Test position utility methods

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