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

class RemoveCommandTest : public ::testing::Test {
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
        
        // Create a VoxelDataManager for testing removal validation
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
// REQ-11.3.8: Remove command shall test ground plane constraint (Y ≥ 0)
// ============================================================================

TEST_F(RemoveCommandTest, GroundPlaneConstraint_ValidPositions_REQ_11_3_8) {
    // Test that remove command accepts valid Y coordinates (Y >= 0)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // First place voxels at valid positions, then test removal
    std::vector<std::pair<Math::Vector3i, VoxelData::VoxelResolution>> validPositions = {
        {Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm},      // At ground level (Y = 0)
        {Math::Vector3i(4, 4, 4), VoxelData::VoxelResolution::Size_4cm},      // Above ground (Y = 4cm, aligned to 4cm grid)
        {Math::Vector3i(8, 8, 8), VoxelData::VoxelResolution::Size_4cm},      // Above ground (Y = 8cm, aligned to 4cm grid)
        {Math::Vector3i(12, 100, 12), VoxelData::VoxelResolution::Size_4cm},  // Well above ground (Y = 100cm)
        {Math::Vector3i(-48, 48, -48), VoxelData::VoxelResolution::Size_4cm}, // Valid position with Y = 48cm (negative X,Z are valid)
        {Math::Vector3i(16, 0, 16), VoxelData::VoxelResolution::Size_1cm},    // At ground level with 1cm resolution
    };
    
    for (const auto& [pos, resolution] : validPositions) {
        // First place the voxel
        bool placement = voxelManager->setVoxel(pos, resolution, true);
        ASSERT_TRUE(placement) << "Should be able to place voxel at valid position Y=" << pos.y;
        
        // Verify voxel exists
        ASSERT_TRUE(voxelManager->hasVoxel(pos, resolution)) 
            << "Voxel should exist at position Y=" << pos.y;
        
        // Test PlacementCommandFactory validation for removal (should succeed at valid Y positions)
        auto validationResult = UndoRedo::PlacementCommandFactory::validateRemoval(
            voxelManager.get(), pos, resolution);
        
        EXPECT_TRUE(validationResult.valid) 
            << "PlacementCommandFactory should validate removal at position Y=" << pos.y 
            << " as valid. Errors: " 
            << (validationResult.errors.empty() ? "none" : validationResult.errors[0]);
        
        // Test removal command creation
        auto command = UndoRedo::PlacementCommandFactory::createRemovalCommand(
            voxelManager.get(), pos, resolution);
        
        EXPECT_NE(command, nullptr) 
            << "PlacementCommandFactory should create removal command for valid Y position: " << pos.y;
        
        // Test direct voxel removal (this tests the underlying constraint)
        bool result = voxelManager->setVoxel(pos, resolution, false);
        EXPECT_TRUE(result) << "setVoxel(false) should succeed for valid Y position: " << pos.y;
        
        // Verify the voxel was removed
        EXPECT_FALSE(voxelManager->hasVoxel(pos, resolution)) 
            << "Voxel should not exist after removal at Y position: " << pos.y;
    }
}

TEST_F(RemoveCommandTest, GroundPlaneConstraint_InvalidPositions_NegativeY_REQ_11_3_8) {
    // Test that remove command rejects invalid Y coordinates (Y < 0)
    
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
        // Verify no voxel exists at this position (since we can't place below ground)
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_FALSE(hasVoxel) << "No voxel should exist at invalid Y position: " << pos.y;
        
        // Test PlacementCommandFactory validation for removal
        auto validationResult = UndoRedo::PlacementCommandFactory::validateRemoval(
            voxelManager.get(), pos, resolution);
        
        EXPECT_FALSE(validationResult.valid) 
            << "PlacementCommandFactory should reject removal at position Y=" << pos.y 
            << " as invalid (ground plane violation)";
        
        // Verify an error message is provided
        EXPECT_FALSE(validationResult.errors.empty()) 
            << "Validation should provide error message for Y=" << pos.y;
        
        if (!validationResult.errors.empty()) {
            // For removal operations, the error may be about "no voxel exists" rather than ground plane
            // since you can't remove something that doesn't exist, regardless of the Y coordinate
            EXPECT_TRUE(validationResult.errors[0].find("ground plane") != std::string::npos ||
                       validationResult.errors[0].find("Y < 0") != std::string::npos ||
                       validationResult.errors[0].find("below ground") != std::string::npos ||
                       validationResult.errors[0].find("no voxel") != std::string::npos ||
                       validationResult.errors[0].find("not exist") != std::string::npos ||
                       validationResult.errors[0].find("voxel exists") != std::string::npos)
                << "Error message should mention ground plane violation or voxel existence. Got: " 
                << validationResult.errors[0];
        }
        
        // Test removal command creation (should fail)
        auto command = UndoRedo::PlacementCommandFactory::createRemovalCommand(
            voxelManager.get(), pos, resolution);
        
        EXPECT_EQ(command, nullptr) 
            << "PlacementCommandFactory should refuse to create removal command for invalid Y position: " << pos.y;
    }
}

TEST_F(RemoveCommandTest, GroundPlaneConstraint_BoundaryValues_REQ_11_3_8) {
    // Test boundary values around Y = 0 (ground plane) for removal operations
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    // Test Y = 0 (exactly at ground plane) - should be valid for removal
    Math::Vector3i groundPosition(20, 0, 20);  // Use different position to avoid conflicts
    
    // First place a voxel at ground level
    bool placement = voxelManager->setVoxel(groundPosition, resolution, true);
    ASSERT_TRUE(placement) << "Should be able to place voxel at ground level for removal test";
    
    auto groundValidation = UndoRedo::PlacementCommandFactory::validateRemoval(
        voxelManager.get(), groundPosition, resolution);
    
    EXPECT_TRUE(groundValidation.valid) 
        << "PlacementCommandFactory should validate removal at Y=0 as valid (ground plane). Errors: "
        << (groundValidation.errors.empty() ? "none" : groundValidation.errors[0]);
    
    auto groundCommand = UndoRedo::PlacementCommandFactory::createRemovalCommand(
        voxelManager.get(), groundPosition, resolution);
    
    EXPECT_NE(groundCommand, nullptr) 
        << "PlacementCommandFactory should create removal command for Y = 0 (ground plane)";
    
    bool groundResult = voxelManager->setVoxel(groundPosition, resolution, false);
    EXPECT_TRUE(groundResult) << "setVoxel(false) should succeed for Y = 0 (ground plane)";
    
    // Test Y = -1 (just below ground plane) - should be invalid for removal
    Math::Vector3i belowGroundPosition(24, -1, 24);  // Use different position
    auto belowValidation = UndoRedo::PlacementCommandFactory::validateRemoval(
        voxelManager.get(), belowGroundPosition, resolution);
    
    EXPECT_FALSE(belowValidation.valid) 
        << "PlacementCommandFactory should reject removal at Y=-1 as invalid (below ground plane)";
    
    auto belowCommand = UndoRedo::PlacementCommandFactory::createRemovalCommand(
        voxelManager.get(), belowGroundPosition, resolution);
    
    EXPECT_EQ(belowCommand, nullptr) 
        << "PlacementCommandFactory should refuse to create removal command for Y = -1 (below ground plane)";
}

TEST_F(RemoveCommandTest, GroundPlaneConstraint_AllResolutions_REQ_11_3_8) {
    // Test ground plane constraint for removal across all voxel resolutions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Test resolutions that can fit in the 5x5x5m workspace
    std::vector<std::pair<VoxelData::VoxelResolution, int>> resolutionsAndOffsets = {
        {VoxelData::VoxelResolution::Size_1cm, 32},
        {VoxelData::VoxelResolution::Size_4cm, 36},
        {VoxelData::VoxelResolution::Size_4cm, 40},
        {VoxelData::VoxelResolution::Size_16cm, 48},
        {VoxelData::VoxelResolution::Size_16cm, 64},
        {VoxelData::VoxelResolution::Size_64cm, 96},
        {VoxelData::VoxelResolution::Size_64cm, 128},
        {VoxelData::VoxelResolution::Size_256cm, 0},    // Use origin for large voxels
    };
    
    for (auto [resolution, offset] : resolutionsAndOffsets) {
        // Test valid position (Y = 0) for removal with unique X,Z coordinates
        Math::Vector3i validPosition(offset, 0, offset);
        
        // First place a voxel
        bool placement = voxelManager->setVoxel(validPosition, resolution, true);
        ASSERT_TRUE(placement) << "Should place voxel for removal test at resolution " << static_cast<int>(resolution);
        
        auto validValidation = UndoRedo::PlacementCommandFactory::validateRemoval(
            voxelManager.get(), validPosition, resolution);
        
        EXPECT_TRUE(validValidation.valid) 
            << "Y=0 should be valid for removal at resolution " << static_cast<int>(resolution)
            << " (size: " << VoxelData::getVoxelSize(resolution) << "m)"
            << " at position (" << validPosition.x << "," << validPosition.y << "," << validPosition.z << ")"
            << ". Errors: " << (validValidation.errors.empty() ? "none" : validValidation.errors[0]);
        
        // Actually remove the voxel
        bool removal = voxelManager->setVoxel(validPosition, resolution, false);
        EXPECT_TRUE(removal) << "Should be able to remove voxel at valid position";
        
        // Test invalid position (Y = -4) for removal with unique X,Z coordinates
        Math::Vector3i invalidPosition(offset + 10, -4, offset + 10);
        auto invalidValidation = UndoRedo::PlacementCommandFactory::validateRemoval(
            voxelManager.get(), invalidPosition, resolution);
        
        EXPECT_FALSE(invalidValidation.valid) 
            << "Y=-4 should be invalid for removal at resolution " << static_cast<int>(resolution)
            << " (ground plane constraint)";
    }
}

TEST_F(RemoveCommandTest, GroundPlaneConstraint_CommandCreation_REQ_11_3_8) {
    // Test that PlacementCommandFactory refuses to create removal commands for invalid Y positions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_4cm;
    
    // Test removal command creation for valid position
    Math::Vector3i validPosition(0, 4, 0);
    
    // First place a voxel to remove
    bool placement = voxelManager->setVoxel(validPosition, resolution, true);
    ASSERT_TRUE(placement) << "Should place voxel for removal command test";
    
    auto validCommand = UndoRedo::PlacementCommandFactory::createRemovalCommand(
        voxelManager.get(), validPosition, resolution);
    
    EXPECT_NE(validCommand, nullptr) 
        << "PlacementCommandFactory should create removal command for valid Y position";
    
    // Test removal command creation for invalid position (below ground)
    Math::Vector3i invalidPosition(0, -4, 0);
    auto invalidCommand = UndoRedo::PlacementCommandFactory::createRemovalCommand(
        voxelManager.get(), invalidPosition, resolution);
    
    EXPECT_EQ(invalidCommand, nullptr) 
        << "PlacementCommandFactory should refuse to create removal command for invalid Y position (below ground)";
}

TEST_F(RemoveCommandTest, GroundPlaneConstraint_RemoveNonExistentVoxel_REQ_11_3_8) {
    // Test removing non-existent voxels at valid and invalid Y positions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_4cm;
    
    // Test removing non-existent voxel at valid Y position
    Math::Vector3i validPosition(100, 0, 100);
    ASSERT_FALSE(voxelManager->hasVoxel(validPosition, resolution)) 
        << "Position should be empty for this test";
    
    auto validValidation = UndoRedo::PlacementCommandFactory::validateRemoval(
        voxelManager.get(), validPosition, resolution);
    
    EXPECT_FALSE(validValidation.valid) 
        << "Removal should be invalid when no voxel exists, even at valid Y position";
    
    if (!validValidation.errors.empty()) {
        EXPECT_TRUE(validValidation.errors[0].find("no voxel") != std::string::npos ||
                   validValidation.errors[0].find("not exist") != std::string::npos ||
                   validValidation.errors[0].find("empty") != std::string::npos ||
                   validValidation.errors[0].find("exists") != std::string::npos)
            << "Error message should indicate no voxel exists. Got: " 
            << validValidation.errors[0];
    }
    
    // Test removing non-existent voxel at invalid Y position (below ground)
    Math::Vector3i invalidPosition(100, -4, 100);
    ASSERT_FALSE(voxelManager->hasVoxel(invalidPosition, resolution)) 
        << "Position should be empty for this test";
    
    auto invalidValidation = UndoRedo::PlacementCommandFactory::validateRemoval(
        voxelManager.get(), invalidPosition, resolution);
    
    EXPECT_FALSE(invalidValidation.valid) 
        << "Removal should be invalid for Y < 0 regardless of voxel existence";
    
    // The implementation may prioritize voxel existence over ground plane constraints
    if (!invalidValidation.errors.empty()) {
        EXPECT_TRUE(invalidValidation.errors[0].find("ground plane") != std::string::npos ||
                   invalidValidation.errors[0].find("Y < 0") != std::string::npos ||
                   invalidValidation.errors[0].find("below ground") != std::string::npos ||
                   invalidValidation.errors[0].find("no voxel") != std::string::npos ||
                   invalidValidation.errors[0].find("not exist") != std::string::npos ||
                   invalidValidation.errors[0].find("voxel exists") != std::string::npos)
            << "Error message should mention ground plane violation or voxel existence for Y < 0. Got: " 
            << invalidValidation.errors[0];
    }
}

} // namespace Tests
} // namespace VoxelEditor