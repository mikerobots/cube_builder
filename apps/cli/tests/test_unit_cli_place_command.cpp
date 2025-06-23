#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/CommandTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "undo_redo/PlacementCommands.h"
#include "foundation/math/Vector3i.h"
#include <sstream>
#include <memory>

namespace VoxelEditor {
namespace Tests {

class PlaceCommandTest : public ::testing::Test {
protected:
    void SetUp() override {
        app = std::make_unique<Application>();
        
        // Initialize in headless mode
        int argc = 2;
        char arg0[] = "test";
        char arg1[] = "--headless";
        char* argv[] = {arg0, arg1, nullptr};
        
        initialized = app->initialize(argc, argv);
        ASSERT_TRUE(initialized) << "Application should initialize in headless mode";
        
        // Create a VoxelDataManager for testing placement validation
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
        Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f); // 5x5x5 meter workspace
        voxelManager->resizeWorkspace(workspaceSize);
    }
    
    void TearDown() override {
        // Clean up
    }
    
    std::unique_ptr<Application> app;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
    bool initialized = false;
};

// ============================================================================
// REQ-11.3.3: Place command shall test ground plane constraint (Y ≥ 0)
// ============================================================================

TEST_F(PlaceCommandTest, GroundPlaneConstraint_ValidPositions_REQ_11_3_3) {
    // Test that place command accepts valid Y coordinates (Y >= 0)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Test valid positions at ground level and above, using 4cm grid alignment
    // Use different X,Z coordinates to avoid overlaps
    std::vector<std::pair<Math::Vector3i, VoxelData::VoxelResolution>> validPositions = {
        {Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm},      // At ground level (Y = 0)
        {Math::Vector3i(4, 4, 4), VoxelData::VoxelResolution::Size_4cm},      // Above ground (Y = 4cm, aligned to 4cm grid)
        {Math::Vector3i(8, 8, 8), VoxelData::VoxelResolution::Size_4cm},      // Above ground (Y = 8cm, aligned to 4cm grid)
        {Math::Vector3i(12, 100, 12), VoxelData::VoxelResolution::Size_4cm},  // Well above ground (Y = 100cm)
        {Math::Vector3i(-48, 48, -48), VoxelData::VoxelResolution::Size_4cm}, // Valid position with Y = 48cm (negative X,Z are valid)
        {Math::Vector3i(16, 0, 16), VoxelData::VoxelResolution::Size_1cm},    // At ground level with 1cm resolution
    };
    
    for (const auto& [pos, resolution] : validPositions) {
        // Test PlacementCommandFactory validation (don't place voxels to avoid overlaps)
        auto validationResult = UndoRedo::PlacementCommandFactory::validatePlacement(
            voxelManager.get(), pos, resolution);
        
        EXPECT_TRUE(validationResult.valid) 
            << "PlacementCommandFactory should validate position Y=" << pos.y 
            << " as valid. Errors: " 
            << (validationResult.errors.empty() ? "none" : validationResult.errors[0]);
        
        // Test direct voxel placement (this tests the underlying constraint)
        bool result = voxelManager->setVoxel(pos, resolution, true);
        EXPECT_TRUE(result) << "setVoxel should succeed for valid Y position: " << pos.y;
        
        // Clear the voxel for next test
        if (result) {
            voxelManager->setVoxel(pos, resolution, false);
        }
    }
}

TEST_F(PlaceCommandTest, GroundPlaneConstraint_InvalidPositions_NegativeY_REQ_11_3_3) {
    // Test that place command rejects invalid Y coordinates (Y < 0)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Test invalid positions below ground level
    std::vector<Math::Vector3i> invalidPositions = {
        Math::Vector3i(0, -1, 0),     // Just below ground (Y = -1cm)
        Math::Vector3i(0, -4, 0),     // Below ground (Y = -4cm)
        Math::Vector3i(0, -8, 0),     // Below ground (Y = -8cm)
        Math::Vector3i(0, -100, 0),   // Well below ground (Y = -100cm)
        Math::Vector3i(50, -1, 50),   // Valid X,Z but invalid Y = -1cm
        Math::Vector3i(-100, -50, 100), // Valid X,Z but invalid Y = -50cm
    };
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_4cm;
    
    for (const auto& pos : invalidPositions) {
        // Test direct voxel placement (this tests the underlying constraint)
        bool result = voxelManager->setVoxel(pos, resolution, true);
        EXPECT_FALSE(result) << "setVoxel should fail for invalid Y position: " << pos.y 
                           << " (violates ground plane constraint Y >= 0)";
        
        // Verify the voxel was not placed
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_FALSE(hasVoxel) << "Voxel should not exist at invalid Y position: " << pos.y;
        
        // Test PlacementCommandFactory validation
        auto validationResult = UndoRedo::PlacementCommandFactory::validatePlacement(
            voxelManager.get(), pos, resolution);
        
        EXPECT_FALSE(validationResult.valid) 
            << "PlacementCommandFactory should reject position Y=" << pos.y 
            << " as invalid (ground plane violation)";
        
        // Verify the specific error message for ground plane violation
        EXPECT_FALSE(validationResult.errors.empty()) 
            << "Validation should provide error message for Y=" << pos.y;
        
        if (!validationResult.errors.empty()) {
            EXPECT_TRUE(validationResult.errors[0].find("ground plane") != std::string::npos ||
                       validationResult.errors[0].find("Y < 0") != std::string::npos)
                << "Error message should mention ground plane violation. Got: " 
                << validationResult.errors[0];
        }
    }
}

TEST_F(PlaceCommandTest, GroundPlaneConstraint_BoundaryValues_REQ_11_3_3) {
    // Test boundary values around Y = 0 (ground plane)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    // Test Y = 0 (exactly at ground plane) - should be valid
    Math::Vector3i groundPosition(20, 0, 20);  // Use different position to avoid conflicts
    auto groundValidation = UndoRedo::PlacementCommandFactory::validatePlacement(
        voxelManager.get(), groundPosition, resolution);
    
    EXPECT_TRUE(groundValidation.valid) 
        << "PlacementCommandFactory should validate Y=0 as valid (ground plane). Errors: "
        << (groundValidation.errors.empty() ? "none" : groundValidation.errors[0]);
    
    bool groundResult = voxelManager->setVoxel(groundPosition, resolution, true);
    EXPECT_TRUE(groundResult) << "setVoxel should succeed for Y = 0 (ground plane)";
    
    // Clear the voxel
    if (groundResult) {
        voxelManager->setVoxel(groundPosition, resolution, false);
    }
    
    // Test Y = -1 (just below ground plane) - should be invalid
    Math::Vector3i belowGroundPosition(24, -1, 24);  // Use different position
    auto belowValidation = UndoRedo::PlacementCommandFactory::validatePlacement(
        voxelManager.get(), belowGroundPosition, resolution);
    
    EXPECT_FALSE(belowValidation.valid) 
        << "PlacementCommandFactory should reject Y=-1 as invalid (below ground plane)";
    
    bool belowResult = voxelManager->setVoxel(belowGroundPosition, resolution, true);
    EXPECT_FALSE(belowResult) << "setVoxel should fail for Y = -1 (below ground plane)";
}

TEST_F(PlaceCommandTest, GroundPlaneConstraint_AllResolutions_REQ_11_3_3) {
    // Test ground plane constraint across all voxel resolutions that fit in workspace
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Test resolutions that can fit in the 5x5x5m workspace
    // For small/medium voxels, use offset positions. For large voxels, use negative positions to fit.
    std::vector<std::pair<VoxelData::VoxelResolution, int>> resolutionsAndOffsets = {
        {VoxelData::VoxelResolution::Size_1cm, 32},
        {VoxelData::VoxelResolution::Size_4cm, 40},
        {VoxelData::VoxelResolution::Size_16cm, 64},
        {VoxelData::VoxelResolution::Size_64cm, 128},
        {VoxelData::VoxelResolution::Size_256cm, -100}, // Use negative offset to ensure it fits
    };
    
    for (auto [resolution, offset] : resolutionsAndOffsets) {
        // Test valid position (Y = 0) for each resolution with unique X,Z coordinates
        Math::Vector3i validPosition(offset, 0, offset);
        auto validValidation = UndoRedo::PlacementCommandFactory::validatePlacement(
            voxelManager.get(), validPosition, resolution);
        
        EXPECT_TRUE(validValidation.valid) 
            << "Y=0 should be valid for resolution " << static_cast<int>(resolution)
            << " (size: " << VoxelData::getVoxelSize(resolution) << "m)"
            << " at position (" << validPosition.x << "," << validPosition.y << "," << validPosition.z << ")"
            << ". Errors: " << (validValidation.errors.empty() ? "none" : validValidation.errors[0]);
        
        // Test invalid position (Y = -4) for each resolution with unique X,Z coordinates
        Math::Vector3i invalidPosition(offset + 10, -4, offset + 10);
        auto invalidValidation = UndoRedo::PlacementCommandFactory::validatePlacement(
            voxelManager.get(), invalidPosition, resolution);
        
        EXPECT_FALSE(invalidValidation.valid) 
            << "Y=-4 should be invalid for resolution " << static_cast<int>(resolution)
            << " (ground plane constraint)";
    }
}

TEST_F(PlaceCommandTest, AllResolutions_LargeVoxelWorkspaceLimits_REQ_11_3_5) {
    // Test all voxel resolutions including large ones that don't fit in default workspace
    // REQ-11.3.5: Place command shall test all valid voxel resolutions (1cm-512cm)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Test large voxels that may not fit in the default 5x5x5m workspace
    std::vector<VoxelData::VoxelResolution> largeResolutions = {
        VoxelData::VoxelResolution::Size_256cm,  // 2.56m - might not fit depending on bounds logic
        // Note: Size_256cm no longer exists in 5-resolution system
    };
    
    for (auto resolution : largeResolutions) {
        // For 256cm voxel in 5m workspace (-2.5m to 2.5m), place at negative position
        // so voxel doesn't exceed positive boundary
        Math::Vector3i position(-100, 0, -100);  // -1m position, voxel extends to 1.56m
        auto validation = UndoRedo::PlacementCommandFactory::validatePlacement(
            voxelManager.get(), position, resolution);
        
        float voxelSize = VoxelData::getVoxelSize(resolution);
        
        if (voxelSize > 5.0f) {
            // Voxels larger than workspace should be rejected
            EXPECT_FALSE(validation.valid) 
                << "Voxel size " << voxelSize << "m should be rejected in 5x5x5m workspace";
            
            if (!validation.errors.empty()) {
                EXPECT_TRUE(validation.errors[0].find("workspace bounds") != std::string::npos ||
                           validation.errors[0].find("outside") != std::string::npos)
                    << "Error should mention workspace bounds for oversized voxel. Got: " 
                    << validation.errors[0];
            }
        } else {
            // Voxels smaller than workspace should be accepted at origin
            EXPECT_TRUE(validation.valid) 
                << "Voxel size " << voxelSize << "m should fit in 5x5x5m workspace at origin. "
                << "Errors: " << (validation.errors.empty() ? "none" : validation.errors[0]);
        }
        
        // Test ground plane constraint for large voxels too
        Math::Vector3i belowGround(0, -static_cast<int>(voxelSize * 100), 0);
        auto belowValidation = UndoRedo::PlacementCommandFactory::validatePlacement(
            voxelManager.get(), belowGround, resolution);
        
        EXPECT_FALSE(belowValidation.valid) 
            << "Large voxel below ground should be rejected regardless of size";
    }
}

TEST_F(PlaceCommandTest, GroundPlaneConstraint_CommandCreation_REQ_11_3_3) {
    // Test that PlacementCommandFactory refuses to create commands for invalid Y positions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_4cm;
    
    // Test command creation for valid position
    Math::Vector3i validPosition(0, 4, 0);
    auto validCommand = UndoRedo::PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), validPosition, resolution);
    
    EXPECT_NE(validCommand, nullptr) 
        << "PlacementCommandFactory should create command for valid Y position";
    
    // Test command creation for invalid position (below ground)
    Math::Vector3i invalidPosition(0, -4, 0);
    auto invalidCommand = UndoRedo::PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), invalidPosition, resolution);
    
    EXPECT_EQ(invalidCommand, nullptr) 
        << "PlacementCommandFactory should refuse to create command for invalid Y position (below ground)";
}

// ============================================================================
// REQ-11.3.4: Place command shall test collision detection with existing voxels
// ============================================================================

TEST_F(PlaceCommandTest, CollisionDetection_SameSize_SamePosition_REQ_11_3_4) {
    // Test collision detection when placing same-size voxels at the same position
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    Math::Vector3i position(0, 0, 0);
    
    // Place first voxel
    bool firstPlacement = voxelManager->setVoxel(position, resolution, true);
    EXPECT_TRUE(firstPlacement) << "First voxel placement should succeed";
    
    // Verify voxel exists
    EXPECT_TRUE(voxelManager->hasVoxel(position, resolution)) 
        << "First voxel should exist at position";
    
    // Test validation - should reject placement at same position
    auto validationResult = UndoRedo::PlacementCommandFactory::validatePlacement(
        voxelManager.get(), position, resolution);
    
    EXPECT_FALSE(validationResult.valid) 
        << "PlacementCommandFactory should reject placement at occupied position";
    
    EXPECT_FALSE(validationResult.errors.empty()) 
        << "Validation should provide error message for collision";
    
    if (!validationResult.errors.empty()) {
        EXPECT_TRUE(validationResult.errors[0].find("overlap") != std::string::npos ||
                   validationResult.errors[0].find("exists") != std::string::npos ||
                   validationResult.errors[0].find("collision") != std::string::npos)
            << "Error message should indicate collision/overlap. Got: " 
            << validationResult.errors[0];
    }
    
    // Test command creation - should refuse to create command
    auto command = UndoRedo::PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), position, resolution);
    
    EXPECT_EQ(command, nullptr) 
        << "PlacementCommandFactory should refuse command creation for occupied position";
    
    // Test direct placement - setVoxel allows overwrites, so we don't test this way
    // Instead, the validation and command creation tests above verify collision detection
}

TEST_F(PlaceCommandTest, CollisionDetection_DifferentSizes_SamePosition_REQ_11_3_4) {
    // Test collision detection when placing different-size voxels at overlapping positions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    Math::Vector3i position(0, 0, 0);
    
    // Place a 4cm voxel first
    VoxelData::VoxelResolution smallResolution = VoxelData::VoxelResolution::Size_4cm;
    bool smallPlacement = voxelManager->setVoxel(position, smallResolution, true);
    EXPECT_TRUE(smallPlacement) << "Small voxel placement should succeed";
    
    // Try to place a larger 8cm voxel at the same position - should fail due to overlap
    VoxelData::VoxelResolution largeResolution = VoxelData::VoxelResolution::Size_16cm;
    auto largeValidation = UndoRedo::PlacementCommandFactory::validatePlacement(
        voxelManager.get(), position, largeResolution);
    
    EXPECT_FALSE(largeValidation.valid) 
        << "Large voxel placement should be rejected due to overlap with small voxel";
    
    bool largePlacement = voxelManager->setVoxel(position, largeResolution, true);
    EXPECT_FALSE(largePlacement) << "Large voxel placement should fail due to collision";
    
    // Clean up and test reverse scenario
    voxelManager->setVoxel(position, smallResolution, false);
    
    // Place large voxel first
    largePlacement = voxelManager->setVoxel(position, largeResolution, true);
    EXPECT_TRUE(largePlacement) << "Large voxel placement should succeed when space is empty";
    
    // Try to place small voxel at same position - should fail
    auto smallValidation = UndoRedo::PlacementCommandFactory::validatePlacement(
        voxelManager.get(), position, smallResolution);
    
    EXPECT_FALSE(smallValidation.valid) 
        << "Small voxel placement should be rejected due to overlap with large voxel";
    
    smallPlacement = voxelManager->setVoxel(position, smallResolution, true);
    EXPECT_FALSE(smallPlacement) << "Small voxel placement should fail due to collision";
}

TEST_F(PlaceCommandTest, CollisionDetection_AdjacentVoxels_NoCollision_REQ_11_3_4) {
    // Test that adjacent voxels don't cause false collision detection
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    
    // Place voxel at origin
    Math::Vector3i origin(0, 0, 0);
    bool originPlacement = voxelManager->setVoxel(origin, resolution, true);
    EXPECT_TRUE(originPlacement) << "Origin voxel placement should succeed";
    
    // Test adjacent positions - should not collide
    // For 16cm voxels, need to be at least 16cm apart
    std::vector<Math::Vector3i> adjacentPositions = {
        Math::Vector3i(16, 0, 0),   // Adjacent in +X direction
        Math::Vector3i(-16, 0, 0),  // Adjacent in -X direction
        Math::Vector3i(0, 16, 0),   // Adjacent in +Y direction
        Math::Vector3i(0, 0, 16),   // Adjacent in +Z direction
        Math::Vector3i(0, 0, -16),  // Adjacent in -Z direction
    };
    
    for (const auto& pos : adjacentPositions) {
        auto validation = UndoRedo::PlacementCommandFactory::validatePlacement(
            voxelManager.get(), pos, resolution);
        
        EXPECT_TRUE(validation.valid) 
            << "Adjacent position (" << pos.x << ", " << pos.y << ", " << pos.z 
            << ") should be valid (no collision)";
        
        bool placement = voxelManager->setVoxel(pos, resolution, true);
        EXPECT_TRUE(placement) 
            << "Adjacent voxel placement should succeed at (" 
            << pos.x << ", " << pos.y << ", " << pos.z << ")";
        
        // Clean up for next test
        voxelManager->setVoxel(pos, resolution, false);
    }
}

TEST_F(PlaceCommandTest, CollisionDetection_MultipleResolutions_REQ_11_3_4) {
    // Test collision detection across different voxel resolutions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Test positions that should be far enough apart to avoid collisions
    std::vector<std::pair<VoxelData::VoxelResolution, Math::Vector3i>> testVoxels = {
        {VoxelData::VoxelResolution::Size_1cm, Math::Vector3i(32, 0, 32)},
        {VoxelData::VoxelResolution::Size_4cm, Math::Vector3i(64, 0, 64)},
        {VoxelData::VoxelResolution::Size_4cm, Math::Vector3i(96, 0, 96)},
        {VoxelData::VoxelResolution::Size_16cm, Math::Vector3i(128, 0, 128)},
        {VoxelData::VoxelResolution::Size_16cm, Math::Vector3i(160, 0, 160)},
    };
    
    // Place all voxels first
    for (const auto& [resolution, pos] : testVoxels) {
        bool placement = voxelManager->setVoxel(pos, resolution, true);
        EXPECT_TRUE(placement) 
            << "Voxel placement should succeed at unique position (" 
            << pos.x << ", " << pos.y << ", " << pos.z << ")";
    }
    
    // Test collision detection - try to place voxels at already occupied positions
    for (const auto& [resolution, pos] : testVoxels) {
        auto validation = UndoRedo::PlacementCommandFactory::validatePlacement(
            voxelManager.get(), pos, resolution);
        
        EXPECT_FALSE(validation.valid) 
            << "Voxel placement should be rejected at occupied position (" 
            << pos.x << ", " << pos.y << ", " << pos.z << ")";
        
        auto command = UndoRedo::PlacementCommandFactory::createPlacementCommand(
            voxelManager.get(), pos, resolution);
        
        EXPECT_EQ(command, nullptr) 
            << "Command creation should fail at occupied position (" 
            << pos.x << ", " << pos.y << ", " << pos.z << ")";
    }
}

TEST_F(PlaceCommandTest, CollisionDetection_LargeVoxelOverlap_REQ_11_3_4) {
    // Test collision detection when large voxels would overlap with smaller ones
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Place a small 1cm voxel
    Math::Vector3i smallPos(0, 0, 0);
    VoxelData::VoxelResolution smallRes = VoxelData::VoxelResolution::Size_1cm;
    bool smallPlacement = voxelManager->setVoxel(smallPos, smallRes, true);
    EXPECT_TRUE(smallPlacement) << "Small voxel placement should succeed";
    
    // Try to place a large 32cm voxel that would overlap
    // The 32cm voxel centered at (0,0,0) would occupy a 32x32x32cm cube
    Math::Vector3i largePos(0, 0, 0);
    VoxelData::VoxelResolution largeRes = VoxelData::VoxelResolution::Size_64cm;
    
    auto validation = UndoRedo::PlacementCommandFactory::validatePlacement(
        voxelManager.get(), largePos, largeRes);
    
    EXPECT_FALSE(validation.valid) 
        << "Large voxel should be rejected due to overlap with small voxel";
    
    bool largePlacement = voxelManager->setVoxel(largePos, largeRes, true);
    EXPECT_FALSE(largePlacement) 
        << "Large voxel placement should fail due to collision";
    
    // Test that we can place the large voxel in a different location
    Math::Vector3i noOverlapPos(64, 0, 64);  // Far enough away
    auto noOverlapValidation = UndoRedo::PlacementCommandFactory::validatePlacement(
        voxelManager.get(), noOverlapPos, largeRes);
    
    EXPECT_TRUE(noOverlapValidation.valid) 
        << "Large voxel should be accepted at non-overlapping position";
    
    bool noOverlapPlacement = voxelManager->setVoxel(noOverlapPos, largeRes, true);
    EXPECT_TRUE(noOverlapPlacement) 
        << "Large voxel placement should succeed at non-overlapping position";
}

TEST_F(PlaceCommandTest, CollisionDetection_EdgeCases_REQ_11_3_4) {
    // Test edge cases in collision detection
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_4cm;
    
    // Test 1: Place and remove voxel, then place again - should succeed
    Math::Vector3i position(40, 0, 40);
    
    bool firstPlacement = voxelManager->setVoxel(position, resolution, true);
    EXPECT_TRUE(firstPlacement) << "First placement should succeed";
    
    bool removal = voxelManager->setVoxel(position, resolution, false);
    EXPECT_TRUE(removal) << "Removal should succeed";
    
    EXPECT_FALSE(voxelManager->hasVoxel(position, resolution)) 
        << "Voxel should not exist after removal";
    
    auto validation = UndoRedo::PlacementCommandFactory::validatePlacement(
        voxelManager.get(), position, resolution);
    
    EXPECT_TRUE(validation.valid) 
        << "Placement should be valid after voxel removal";
    
    bool secondPlacement = voxelManager->setVoxel(position, resolution, true);
    EXPECT_TRUE(secondPlacement) << "Second placement should succeed after removal";
    
    // Test 2: Collision detection with null voxel manager
    auto nullValidation = UndoRedo::PlacementCommandFactory::validatePlacement(
        nullptr, position, resolution);
    
    EXPECT_FALSE(nullValidation.valid) 
        << "Validation should fail with null voxel manager";
    
    auto nullCommand = UndoRedo::PlacementCommandFactory::createPlacementCommand(
        nullptr, position, resolution);
    
    EXPECT_EQ(nullCommand, nullptr) 
        << "Command creation should fail with null voxel manager";
}

TEST_F(PlaceCommandTest, GroundPlaneConstraint_VoxelsTooLargeForWorkspace_REQ_11_3_3) {
    // Test that voxels larger than the workspace are properly rejected
    // 512cm (5.12m) voxels don't fit in the 5x5x5m default workspace
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    Math::Vector3i position(0, 0, 0);  // Even at origin
    VoxelData::VoxelResolution tooLargeResolution = VoxelData::VoxelResolution::Size_256cm;
    
    auto validation = UndoRedo::PlacementCommandFactory::validatePlacement(
        voxelManager.get(), position, tooLargeResolution);
    
    EXPECT_FALSE(validation.valid) 
        << "512cm voxels (5.12m) should be rejected in 5x5x5m workspace, even at origin";
    
    EXPECT_FALSE(validation.errors.empty()) 
        << "Validation should provide error message for oversized voxel";
    
    if (!validation.errors.empty()) {
        EXPECT_TRUE(validation.errors[0].find("workspace bounds") != std::string::npos ||
                   validation.errors[0].find("outside") != std::string::npos)
            << "Error message should mention workspace bounds. Got: " 
            << validation.errors[0];
    }
}

} // namespace Tests
} // namespace VoxelEditor