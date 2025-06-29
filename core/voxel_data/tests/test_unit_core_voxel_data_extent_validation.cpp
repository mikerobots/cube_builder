#include <gtest/gtest.h>
#include "../VoxelDataManager.h"
#include "../../../foundation/math/CoordinateTypes.h"
#include "../../../foundation/math/CoordinateConverter.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

class VoxelExtentValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        voxelManager = std::make_unique<VoxelDataManager>();
        // Set default workspace size to 5m (500cm)
        voxelManager->resizeWorkspace(5.0f);
    }
    
    std::unique_ptr<VoxelDataManager> voxelManager;
};

// Test that small voxels at origin are accepted
TEST_F(VoxelExtentValidationTest, SmallVoxelAtOrigin_Accepted) {
    IncrementCoordinates origin(0, 0, 0);
    
    // Test small resolutions that should fit
    std::vector<VoxelResolution> smallResolutions = {
        VoxelResolution::Size_1cm,
        VoxelResolution::Size_4cm,
        VoxelResolution::Size_16cm,
        VoxelResolution::Size_64cm,
        VoxelResolution::Size_128cm
    };
    
    for (auto resolution : smallResolutions) {
        auto validation = voxelManager->validatePosition(origin, resolution);
        float voxelSize = getVoxelSize(resolution);
        
        EXPECT_TRUE(validation.valid) 
            << "Voxel size " << voxelSize << "m should be accepted at origin in 5m workspace";
        EXPECT_TRUE(validation.withinBounds)
            << "Voxel size " << voxelSize << "m should be within bounds at origin";
    }
}

// Test that 256cm voxel at origin is accepted (fits within 5m workspace)
TEST_F(VoxelExtentValidationTest, Voxel256cm_AtOrigin_Accepted) {
    IncrementCoordinates origin(0, 0, 0);
    VoxelResolution resolution = VoxelResolution::Size_256cm;
    
    auto validation = voxelManager->validatePosition(origin, resolution);
    
    // 256cm voxel extends from -128cm to +128cm, which fits in -250cm to +250cm bounds
    EXPECT_TRUE(validation.valid) 
        << "256cm voxel should be accepted at origin in 5m workspace";
    EXPECT_TRUE(validation.withinBounds)
        << "256cm voxel should be within bounds at origin";
}

// Test that 512cm voxel at origin is rejected (exceeds 5m workspace)
TEST_F(VoxelExtentValidationTest, Voxel512cm_AtOrigin_Rejected) {
    IncrementCoordinates origin(0, 0, 0);
    VoxelResolution resolution = VoxelResolution::Size_512cm;
    
    auto validation = voxelManager->validatePosition(origin, resolution);
    
    // 512cm voxel extends from -256cm to +256cm, which exceeds -250cm to +250cm bounds
    EXPECT_FALSE(validation.valid) 
        << "512cm voxel should be rejected at origin in 5m workspace";
    EXPECT_FALSE(validation.withinBounds)
        << "512cm voxel should be outside bounds at origin";
    EXPECT_EQ(validation.errorMessage, "Voxel would extend outside workspace bounds")
        << "Error message should indicate voxel extent issue";
}

// Test voxels near workspace edge
TEST_F(VoxelExtentValidationTest, VoxelNearEdge_ExtentCheck) {
    // For 5m workspace, bounds are -250cm to +250cm
    
    // Test 64cm voxel near positive X edge
    // At position 220cm, voxel extends from 188cm to 252cm - exceeds bound
    IncrementCoordinates nearEdge(220, 0, 0);
    auto validation = voxelManager->validatePosition(nearEdge, VoxelResolution::Size_64cm);
    
    EXPECT_FALSE(validation.valid) 
        << "64cm voxel at X=220 should be rejected (would extend to 252cm, exceeding 250cm bound)";
    EXPECT_EQ(validation.errorMessage, "Voxel would extend outside workspace bounds");
    
    // Test same voxel further from edge - should fit
    IncrementCoordinates safePosition(180, 0, 0);
    validation = voxelManager->validatePosition(safePosition, VoxelResolution::Size_64cm);
    
    EXPECT_TRUE(validation.valid) 
        << "64cm voxel at X=180 should be accepted (extends to 212cm, within 250cm bound)";
}

// Test voxels at negative bounds
TEST_F(VoxelExtentValidationTest, VoxelAtNegativeBounds) {
    // Test 128cm voxel near negative Z edge
    // At position -190cm, voxel extends from -254cm to -126cm - exceeds bound
    IncrementCoordinates nearNegEdge(0, 0, -190);
    auto validation = voxelManager->validatePosition(nearNegEdge, VoxelResolution::Size_128cm);
    
    EXPECT_FALSE(validation.valid) 
        << "128cm voxel at Z=-190 should be rejected (would extend to -254cm, exceeding -250cm bound)";
    
    // Test position that fits
    IncrementCoordinates safeNegPosition(0, 0, -180);
    validation = voxelManager->validatePosition(safeNegPosition, VoxelResolution::Size_128cm);
    
    EXPECT_TRUE(validation.valid) 
        << "128cm voxel at Z=-180 should be accepted (extends to -244cm, within -250cm bound)";
}

// Test voxels at Y bounds (vertical)
TEST_F(VoxelExtentValidationTest, VoxelAtVerticalBounds) {
    // For 5m workspace, Y ranges from 0 to 500cm
    
    // Test 256cm voxel at Y=250 - would extend to Y=506cm, exceeding 500cm
    IncrementCoordinates highPosition(0, 250, 0);
    auto validation = voxelManager->validatePosition(highPosition, VoxelResolution::Size_256cm);
    
    EXPECT_FALSE(validation.valid) 
        << "256cm voxel at Y=250 should be rejected (would extend to 506cm, exceeding 500cm bound)";
    
    // Test position that fits
    IncrementCoordinates safeHighPosition(0, 240, 0);
    validation = voxelManager->validatePosition(safeHighPosition, VoxelResolution::Size_256cm);
    
    EXPECT_TRUE(validation.valid) 
        << "256cm voxel at Y=240 should be accepted (extends to 496cm, within 500cm bound)";
}

// Test different workspace sizes
TEST_F(VoxelExtentValidationTest, DifferentWorkspaceSizes) {
    // Change to minimum workspace (2m = 200cm)
    voxelManager->resizeWorkspace(2.0f);
    
    IncrementCoordinates origin(0, 0, 0);
    
    // 128cm voxel should NOT fit in 2m workspace (extends ±64cm, exceeds ±100cm bounds)
    auto validation = voxelManager->validatePosition(origin, VoxelResolution::Size_128cm);
    EXPECT_TRUE(validation.valid) 
        << "128cm voxel at origin should fit in 2m workspace (extends to ±64cm, within ±100cm)";
    
    // 256cm voxel should definitely not fit
    validation = voxelManager->validatePosition(origin, VoxelResolution::Size_256cm);
    EXPECT_FALSE(validation.valid) 
        << "256cm voxel should not fit in 2m workspace";
    
    // Change to maximum workspace (8m = 800cm)
    voxelManager->resizeWorkspace(8.0f);
    
    // Even 512cm voxel should fit at origin (extends ±256cm, within ±400cm bounds)
    validation = voxelManager->validatePosition(origin, VoxelResolution::Size_512cm);
    EXPECT_TRUE(validation.valid) 
        << "512cm voxel should fit at origin in 8m workspace";
}

// Test fill operation with extent validation
TEST_F(VoxelExtentValidationTest, FillRegion_ExtentValidation) {
    // Try to fill a region with 512cm voxels in 5m workspace
    BoundingBox fillRegion(Vector3f(-1.0f, 0.0f, -1.0f), Vector3f(1.0f, 1.0f, 1.0f));
    
    auto result = voxelManager->fillRegion(fillRegion, VoxelResolution::Size_512cm, true);
    
    // Should fail because 512cm voxels don't fit in 5m workspace
    EXPECT_FALSE(result.success)
        << "Fill with 512cm voxels should fail in 5m workspace";
    EXPECT_GT(result.failedOutOfBounds, 0)
        << "Should report out of bounds failures";
    EXPECT_EQ(result.voxelsFilled, 0)
        << "No voxels should be placed";
}

// Test that placement position check still works
TEST_F(VoxelExtentValidationTest, PlacementPositionValidation) {
    // Test position that's outside bounds regardless of voxel size
    IncrementCoordinates farPosition(300, 0, 300);  // Well outside 250cm bounds
    
    auto validation = voxelManager->validatePosition(farPosition, VoxelResolution::Size_1cm);
    
    EXPECT_FALSE(validation.valid) 
        << "Position outside workspace should be rejected";
    EXPECT_EQ(validation.errorMessage, "Position is outside workspace bounds")
        << "Should report position error, not extent error";
}

// Test edge case: voxel exactly at bounds
TEST_F(VoxelExtentValidationTest, VoxelExactlyAtBounds) {
    // For 5m workspace with 32cm voxel:
    // Voxel extends ±16cm from center
    // Maximum X position where voxel fits: 250 - 16 = 234cm
    
    IncrementCoordinates exactBound(234, 0, 0);
    auto validation = voxelManager->validatePosition(exactBound, VoxelResolution::Size_32cm);
    
    EXPECT_TRUE(validation.valid) 
        << "32cm voxel at X=234 should exactly fit (extends to 250cm)";
    
    // One cm further should fail
    IncrementCoordinates beyondBound(235, 0, 0);
    validation = voxelManager->validatePosition(beyondBound, VoxelResolution::Size_32cm);
    
    EXPECT_FALSE(validation.valid) 
        << "32cm voxel at X=235 should not fit (would extend to 251cm)";
}