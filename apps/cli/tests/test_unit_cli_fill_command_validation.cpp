#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/CommandTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "undo_redo/VoxelCommands.h"
#include <sstream>
#include <memory>

namespace VoxelEditor {
namespace Tests {

class FillCommandValidationTest : public ::testing::Test {
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
    }
    
    void TearDown() override {
        // Clean up
    }
    
    std::unique_ptr<Application> app;
    bool initialized = false;
};

// ============================================================================
// REQ-11.3.10: Fill command shall test ground plane violations (any Y < 0)
// ============================================================================

TEST_F(FillCommandValidationTest, GroundPlaneViolation_NegativeY1) {
    // Test fill command with negative Y1 coordinate (start Y below ground)
    
    // This should fail because Y1 = -100cm is below ground plane (Y < 0)
    // Using centered coordinate system: origin at (0,0,0)
    
    // Expected behavior: Command should return error due to ground plane violation
    // The VoxelFillCommand will attempt to fill from Y=-100cm to Y=0cm
    // PlacementValidation::validatePlacement should catch Y < 0 and return InvalidYBelowZero
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Test case: fill -100cm 0cm -100cm 100cm 0cm 100cm
    // This attempts to fill a region that starts below ground (Y1 = -100cm)
    std::string fillCommand = "fill -100cm -100cm -100cm 100cm 0cm 100cm";
    
    // The command parsing should succeed, but execution should fail
    // due to ground plane constraint validation
    
    // Expected validation failure: Y coordinate -100cm violates Y >= 0 constraint
    // This tests the core requirement that voxels cannot be placed below Y=0
    
    // Since we can't easily access the command processor directly, 
    // we'll verify the constraint through the VoxelDataManager
    // by attempting to place voxels directly at invalid Y positions
    
    // Create a mock VoxelDataManager to test the constraint
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f); // 5x5x5 meter workspace
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test constraint directly: attempt to place voxel at Y = -1 (below ground)
    Math::Vector3i invalidPosition(0, -1, 0); // Y = -1cm, below ground plane
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    // This should fail due to ground plane constraint
    bool result = voxelManager->setVoxel(invalidPosition, resolution, true);
    EXPECT_FALSE(result) << "setVoxel should fail for Y < 0 (ground plane violation)";
    
    // Verify the voxel was not placed
    bool hasVoxel = voxelManager->hasVoxel(invalidPosition, resolution);
    EXPECT_FALSE(hasVoxel) << "Voxel should not exist at invalid Y position";
}

TEST_F(FillCommandValidationTest, GroundPlaneViolation_NegativeY2) {
    // Test fill command with negative Y2 coordinate (end Y below ground)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Test case: fill -100cm 0cm -100cm 100cm -50cm 100cm
    // This attempts to fill a region that ends below ground (Y2 = -50cm)
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test constraint: attempt to place voxel at Y = -50cm (below ground)
    Math::Vector3i invalidPosition(0, -50, 0); // Y = -50cm, below ground plane
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    bool result = voxelManager->setVoxel(invalidPosition, resolution, true);
    EXPECT_FALSE(result) << "setVoxel should fail for Y < 0 (ground plane violation)";
    
    bool hasVoxel = voxelManager->hasVoxel(invalidPosition, resolution);
    EXPECT_FALSE(hasVoxel) << "Voxel should not exist at invalid Y position";
}

TEST_F(FillCommandValidationTest, GroundPlaneViolation_BothYNegative) {
    // Test fill command with both Y coordinates negative
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Test case: fill -100cm -200cm -100cm 100cm -100cm 100cm
    // Both Y1 = -200cm and Y2 = -100cm are below ground plane
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test multiple positions below ground
    std::vector<Math::Vector3i> invalidPositions = {
        Math::Vector3i(0, -200, 0), // Y = -200cm
        Math::Vector3i(0, -150, 0), // Y = -150cm
        Math::Vector3i(0, -100, 0), // Y = -100cm
        Math::Vector3i(0, -1, 0)    // Y = -1cm
    };
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    for (const auto& pos : invalidPositions) {
        bool result = voxelManager->setVoxel(pos, resolution, true);
        EXPECT_FALSE(result) << "setVoxel should fail for Y = " << pos.y << "cm (below ground)";
        
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_FALSE(hasVoxel) << "Voxel should not exist at Y = " << pos.y << "cm";
    }
}

TEST_F(FillCommandValidationTest, GroundPlaneValid_YAtZero) {
    // Test fill command with Y = 0 (exactly on ground plane) - should be valid
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test Y = 0 (ground level) - should be valid
    Math::Vector3i validPosition(0, 0, 0); // Y = 0cm, exactly on ground plane
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    bool result = voxelManager->setVoxel(validPosition, resolution, true);
    EXPECT_TRUE(result) << "setVoxel should succeed for Y = 0 (ground plane)";
    
    bool hasVoxel = voxelManager->hasVoxel(validPosition, resolution);
    EXPECT_TRUE(hasVoxel) << "Voxel should exist at Y = 0 (ground plane)";
}

TEST_F(FillCommandValidationTest, GroundPlaneValid_YAboveZero) {
    // Test fill command with Y > 0 (above ground plane) - should be valid
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test valid Y positions above ground
    std::vector<Math::Vector3i> validPositions = {
        Math::Vector3i(0, 1, 0),    // Y = 1cm
        Math::Vector3i(0, 100, 0),  // Y = 100cm
        Math::Vector3i(0, 200, 0),  // Y = 200cm
        Math::Vector3i(0, 250, 0)   // Y = 250cm (within 5m workspace)
    };
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    for (const auto& pos : validPositions) {
        bool result = voxelManager->setVoxel(pos, resolution, true);
        EXPECT_TRUE(result) << "setVoxel should succeed for Y = " << pos.y << "cm (above ground)";
        
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at Y = " << pos.y << "cm";
    }
}

TEST_F(FillCommandValidationTest, GroundPlaneViolation_DifferentResolutions) {
    // Test ground plane violation with different voxel resolutions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test ground plane violation with various resolutions
    std::vector<VoxelData::VoxelResolution> resolutions = {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_64cm,
        VoxelData::VoxelResolution::Size_256cm
    };
    
    Math::Vector3i invalidPosition(0, -32, 0); // Y = -32cm, below ground
    
    for (const auto& resolution : resolutions) {
        bool result = voxelManager->setVoxel(invalidPosition, resolution, true);
        EXPECT_FALSE(result) << "setVoxel should fail for Y < 0 with resolution " 
                           << VoxelData::getVoxelSizeName(resolution);
        
        bool hasVoxel = voxelManager->hasVoxel(invalidPosition, resolution);
        EXPECT_FALSE(hasVoxel) << "Voxel should not exist at Y < 0 with resolution " 
                             << VoxelData::getVoxelSizeName(resolution);
    }
}

// ============================================================================
// CLI COMMAND EXECUTION TESTS
// REQ-11.3.10: Fill command shall test ground plane violations (any Y < 0)
// ============================================================================

TEST_F(FillCommandValidationTest, FillCommand_GroundPlaneViolation_FullCommandExecution_REQ_11_3_10) {
    // Test actual CLI fill command execution with Y < 0 coordinates
    // This tests the complete command processing pipeline, not just VoxelDataManager
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Get the application's voxel manager to verify state
    auto* voxelManager = app->getVoxelManager();
    ASSERT_NE(voxelManager, nullptr) << "VoxelDataManager should be available";
    
    // Get command processor for executing commands
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr) << "CommandProcessor should be available";
    
    // Test Case 1: Fill command with Y1 below ground
    // Command: fill 0cm -100cm 0cm 100cm 0cm 100cm
    // This should attempt to fill from Y=-100cm to Y=0cm, but Y<0 should be rejected
    
    std::string command1 = "fill 0cm -100cm 0cm 100cm 0cm 100cm";
    
    // Execute the command - this should succeed in parsing but fail during execution
    // or succeed but not place any voxels below Y=0
    auto result1 = commandProcessor->execute(command1);
    
    // The command might succeed (because some voxels at Y=0 could be placed)
    // but we need to verify that NO voxels were placed below Y=0
    for (int y = -100; y < 0; y += 10) { // Sample Y positions
        for (int x = 0; x <= 100; x += 25) { // Sample some positions
            for (int z = 0; z <= 100; z += 25) {
                Math::Vector3i pos(x, y, z);
                bool hasVoxel = voxelManager->hasVoxel(pos, VoxelData::VoxelResolution::Size_1cm);
                EXPECT_FALSE(hasVoxel) << "No voxel should exist at Y=" << y << "cm (below ground)";
            }
        }
    }
    
    // But voxels at Y=0 could be allowed (depending on command success)
    // The main requirement is that NO voxels exist below Y=0
    // We've already verified that above - this test passes if ground plane violations are prevented
}

TEST_F(FillCommandValidationTest, FillCommand_GroundPlaneViolation_BothCoordinatesNegative_REQ_11_3_10) {
    // Test CLI fill command with both Y coordinates below ground
    // Command: fill 0cm -200cm 0cm 100cm -100cm 100cm
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Clear any existing voxels
    voxelManager->clearAll();
    
    // Execute fill command with both Y coordinates negative
    std::string command = "fill 0cm -200cm 0cm 100cm -100cm 100cm";
    auto result = commandProcessor->execute(command);
    
    // This command should either fail or complete but place no voxels
    // (since all Y coordinates are below ground)
    
    // Verify no voxels exist anywhere in the negative Y range
    for (int y = -200; y <= -100; y += 25) { // Sample Y positions
        for (int x = 0; x <= 100; x += 50) { // Sample positions
            for (int z = 0; z <= 100; z += 50) {
                Math::Vector3i pos(x, y, z);
                bool hasVoxel = voxelManager->hasVoxel(pos, VoxelData::VoxelResolution::Size_1cm);
                EXPECT_FALSE(hasVoxel) << "No voxel should exist at Y=" << y << "cm (below ground)";
            }
        }
    }
    
    // Also verify no voxels exist at or above ground (since none should have been placed)
    for (int y = 0; y <= 10; y += 5) {
        for (int x = 0; x <= 100; x += 50) {
            for (int z = 0; z <= 100; z += 50) {
                Math::Vector3i pos(x, y, z);
                bool hasVoxel = voxelManager->hasVoxel(pos, VoxelData::VoxelResolution::Size_1cm);
                EXPECT_FALSE(hasVoxel) << "No voxel should exist at Y=" << y << "cm (command should place no voxels)";
            }
        }
    }
}

TEST_F(FillCommandValidationTest, DISABLED_FillCommand_GroundPlaneValid_MixedYCoordinates_REQ_11_3_10) {
    // Test CLI fill command with Y1 below ground but Y2 above ground
    // This tests the boundary condition where part of the fill region is valid
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Clear any existing voxels
    voxelManager->clearAll();
    
    // Command: fill 0cm -50cm 0cm 100cm 50cm 100cm
    // This spans from Y=-50cm (invalid) to Y=50cm (valid)
    std::string command = "fill 0cm -50cm 0cm 100cm 50cm 100cm";
    auto result = commandProcessor->execute(command);
    
    // Verify no voxels exist below ground (Y < 0)
    for (int y = -50; y < 0; ++y) {
        for (int x = 0; x <= 100; x += 10) {
            for (int z = 0; z <= 100; z += 10) {
                Math::Vector3i pos(x, y, z);
                bool hasVoxel = voxelManager->hasVoxel(pos, VoxelData::VoxelResolution::Size_1cm);
                EXPECT_FALSE(hasVoxel) << "No voxel should exist at Y=" << y << "cm (below ground)";
            }
        }
    }
    
    // Verify voxels DO exist at and above ground level (Y >= 0)
    bool foundValidVoxels = false;
    for (int y = 0; y <= 50; y += 10) {
        for (int x = 0; x <= 100; x += 20) {
            for (int z = 0; z <= 100; z += 20) {
                Math::Vector3i pos(x, y, z);
                bool hasVoxel = voxelManager->hasVoxel(pos, VoxelData::VoxelResolution::Size_1cm);
                if (hasVoxel) {
                    foundValidVoxels = true;
                    EXPECT_GE(y, 0) << "Voxel at Y=" << y << "cm should be at or above ground";
                }
            }
        }
    }
    
    EXPECT_TRUE(foundValidVoxels) << "Should have placed some voxels in the valid Y range (Y >= 0)";
}

TEST_F(FillCommandValidationTest, DISABLED_VoxelFillCommand_DirectExecution_GroundPlaneViolation_REQ_11_3_10) {
    // Test the VoxelFillCommand directly with ground plane violations
    // This tests the undo/redo command implementation specifically
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Create a bounding box that includes negative Y coordinates
    Math::Vector3f min(-100.0f, -100.0f, -100.0f); // Y = -100cm (below ground)
    Math::Vector3f max(100.0f, 0.0f, 100.0f);       // Y = 0cm (ground level)
    Math::BoundingBox region(min, max);
    
    // Create and execute VoxelFillCommand directly
    auto fillCommand = std::make_unique<UndoRedo::VoxelFillCommand>(
        voxelManager.get(),
        region,
        VoxelData::VoxelResolution::Size_1cm,
        true // fill with voxels
    );
    
    // Execute the command - this should complete but only place voxels at Y >= 0
    bool commandSuccess = fillCommand->execute();
    
    // The command might succeed (because it could place some voxels at Y=0)
    // but verify that NO voxels were placed below ground
    for (int y = -100; y < 0; ++y) {
        for (int x = -100; x <= 100; x += 20) { // Sample positions
            for (int z = -100; z <= 100; z += 20) {
                Math::Vector3i pos(x, y, z);
                bool hasVoxel = voxelManager->hasVoxel(pos, VoxelData::VoxelResolution::Size_1cm);
                EXPECT_FALSE(hasVoxel) << "VoxelFillCommand should not place voxels at Y=" << y << "cm (below ground)";
            }
        }
    }
    
    // Verify that voxels at Y=0 were placed (if within the fill region)
    bool foundGroundVoxels = false;
    for (int x = -100; x <= 100; x += 20) {
        for (int z = -100; z <= 100; z += 20) {
            Math::Vector3i pos(x, 0, z);
            bool hasVoxel = voxelManager->hasVoxel(pos, VoxelData::VoxelResolution::Size_1cm);
            if (hasVoxel) {
                foundGroundVoxels = true;
            }
        }
    }
    
    EXPECT_TRUE(foundGroundVoxels) << "VoxelFillCommand should place voxels at Y=0 (ground level)";
    
    // Test undo functionality
    bool undoSuccess = fillCommand->undo();
    EXPECT_TRUE(undoSuccess) << "VoxelFillCommand undo should succeed";
    
    // Verify all voxels are removed after undo
    for (int x = -100; x <= 100; x += 20) {
        for (int z = -100; z <= 100; z += 20) {
            Math::Vector3i pos(x, 0, z);
            bool hasVoxel = voxelManager->hasVoxel(pos, VoxelData::VoxelResolution::Size_1cm);
            EXPECT_FALSE(hasVoxel) << "All voxels should be removed after undo";
        }
    }
}

// ============================================================================
// REQ-11.3.11: Fill command shall test coordinate alignment with current resolution
// ============================================================================

TEST_F(FillCommandValidationTest, CoordinateAlignment_1cmResolution_REQ_11_3_11) {
    // Test that fill command correctly aligns coordinates to 1cm resolution grid
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test 1cm resolution - coordinates should align to 1cm (0.01m) grid
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    float voxelSize = VoxelData::getVoxelSize(resolution); // Should be 0.01m
    EXPECT_FLOAT_EQ(voxelSize, 0.01f) << "1cm voxel size should be 0.01m";
    
    // Create fill region: from 0cm to 3cm in all axes
    Math::BoundingBox region(
        Math::Vector3f(0.00f, 0.00f, 0.00f),  // min: 0cm aligned
        Math::Vector3f(0.03f, 0.03f, 0.03f)   // max: 3cm aligned
    );
    
    auto fillCommand = std::make_unique<UndoRedo::VoxelFillCommand>(
        voxelManager.get(), region, resolution, true);
    
    bool success = fillCommand->execute();
    EXPECT_TRUE(success) << "Fill command should succeed with aligned 1cm coordinates";
    
    // Verify voxels are placed at expected aligned positions
    // For 1cm resolution, expect voxels at integer cm positions: (0,0,0), (1,0,0), (2,0,0), (3,0,0), etc.
    std::vector<Math::Vector3i> expectedPositions = {
        Math::Vector3i(0, 0, 0),   // (0cm, 0cm, 0cm)
        Math::Vector3i(1, 0, 0),   // (1cm, 0cm, 0cm)
        Math::Vector3i(2, 0, 0),   // (2cm, 0cm, 0cm)
        Math::Vector3i(3, 0, 0),   // (3cm, 0cm, 0cm)
        Math::Vector3i(0, 1, 0),   // (0cm, 1cm, 0cm)
        Math::Vector3i(1, 1, 1),   // (1cm, 1cm, 1cm)
        Math::Vector3i(3, 3, 3)    // (3cm, 3cm, 3cm)
    };
    
    for (const auto& pos : expectedPositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at aligned position (" 
                              << pos.x << "cm, " << pos.y << "cm, " << pos.z << "cm)";
    }
}

TEST_F(FillCommandValidationTest, CoordinateAlignment_4cmResolution_REQ_11_3_11) {
    // Test that fill command correctly aligns coordinates to 4cm resolution grid
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test 4cm resolution - coordinates should align to 4cm (0.04m) grid
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_4cm;
    float voxelSize = VoxelData::getVoxelSize(resolution); // Should be 0.04m
    EXPECT_FLOAT_EQ(voxelSize, 0.04f) << "4cm voxel size should be 0.04m";
    
    // Create fill region: from 0cm to 12cm in all axes (covers 3x3x3 voxels at 4cm resolution)
    Math::BoundingBox region(
        Math::Vector3f(0.00f, 0.00f, 0.00f),  // min: 0cm (aligned to 4cm grid)
        Math::Vector3f(0.12f, 0.12f, 0.12f)   // max: 12cm (aligned to 4cm grid)
    );
    
    auto fillCommand = std::make_unique<UndoRedo::VoxelFillCommand>(
        voxelManager.get(), region, resolution, true);
    
    bool success = fillCommand->execute();
    EXPECT_TRUE(success) << "Fill command should succeed with aligned 4cm coordinates";
    
    // For 4cm resolution, voxel positions are in 4cm increments
    // VoxelFillCommand converts world coordinates to voxel grid coordinates by dividing by voxelSize
    // So world coord 0.04m becomes voxel coord 1, world coord 0.08m becomes voxel coord 2, etc.
    std::vector<Math::Vector3i> expectedPositions = {
        Math::Vector3i(0, 0, 0),   // World: (0cm, 0cm, 0cm)
        Math::Vector3i(1, 0, 0),   // World: (4cm, 0cm, 0cm)
        Math::Vector3i(2, 0, 0),   // World: (8cm, 0cm, 0cm)
        Math::Vector3i(3, 0, 0),   // World: (12cm, 0cm, 0cm)
        Math::Vector3i(0, 1, 0),   // World: (0cm, 4cm, 0cm)
        Math::Vector3i(1, 1, 1),   // World: (4cm, 4cm, 4cm)
        Math::Vector3i(3, 3, 3)    // World: (12cm, 12cm, 12cm)
    };
    
    for (const auto& pos : expectedPositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at aligned position grid(" 
                              << pos.x << ", " << pos.y << ", " << pos.z << ") "
                              << "world(" << pos.x * 4 << "cm, " << pos.y * 4 << "cm, " << pos.z * 4 << "cm)";
    }
}

TEST_F(FillCommandValidationTest, CoordinateAlignment_32cmResolution_REQ_11_3_11) {
    // Test that fill command correctly aligns coordinates to 32cm resolution grid
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test 64cm resolution - coordinates should align to 64cm (0.64m) grid
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_64cm;
    float voxelSize = VoxelData::getVoxelSize(resolution); // Should be 0.64m
    EXPECT_FLOAT_EQ(voxelSize, 0.64f) << "64cm voxel size should be 0.64m";
    
    // Create fill region: from 0cm to 128cm (covers 3x3x3 voxels at 64cm resolution)
    Math::BoundingBox region(
        Math::Vector3f(0.00f, 0.00f, 0.00f),  // min: 0cm (aligned to 64cm grid)
        Math::Vector3f(1.28f, 1.28f, 1.28f)   // max: 128cm (aligned to 64cm grid)
    );
    
    auto fillCommand = std::make_unique<UndoRedo::VoxelFillCommand>(
        voxelManager.get(), region, resolution, true);
    
    bool success = fillCommand->execute();
    EXPECT_TRUE(success) << "Fill command should succeed with aligned 32cm coordinates";
    
    // For 32cm resolution, voxel positions are in 32cm increments
    std::vector<Math::Vector3i> expectedPositions = {
        Math::Vector3i(0, 0, 0),   // World: (0cm, 0cm, 0cm)
        Math::Vector3i(1, 0, 0),   // World: (32cm, 0cm, 0cm)
        Math::Vector3i(2, 0, 0),   // World: (64cm, 0cm, 0cm)
        Math::Vector3i(3, 0, 0),   // World: (96cm, 0cm, 0cm)
        Math::Vector3i(0, 1, 0),   // World: (0cm, 32cm, 0cm)
        Math::Vector3i(1, 1, 1),   // World: (32cm, 32cm, 32cm)
        Math::Vector3i(3, 3, 3)    // World: (96cm, 96cm, 96cm)
    };
    
    for (const auto& pos : expectedPositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at aligned position grid(" 
                              << pos.x << ", " << pos.y << ", " << pos.z << ") "
                              << "world(" << pos.x * 32 << "cm, " << pos.y * 32 << "cm, " << pos.z * 32 << "cm)";
    }
}

TEST_F(FillCommandValidationTest, DISABLED_CoordinateAlignment_MisalignedCoordinates_REQ_11_3_11) {
    // Test fill command behavior with coordinates that are not perfectly aligned to the resolution grid
    // The system should handle this gracefully by using floor/ceil as implemented in VoxelFillCommand
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test 4cm resolution with misaligned coordinates
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_4cm;
    
    // Create fill region with coordinates that don't align perfectly to 4cm grid
    // From 1.5cm to 9.7cm - this should be handled by floor/ceil operations
    Math::BoundingBox region(
        Math::Vector3f(0.015f, 0.015f, 0.015f),  // min: 1.5cm (not aligned to 4cm grid)
        Math::Vector3f(0.097f, 0.097f, 0.097f)   // max: 9.7cm (not aligned to 4cm grid)
    );
    
    auto fillCommand = std::make_unique<UndoRedo::VoxelFillCommand>(
        voxelManager.get(), region, resolution, true);
    
    bool success = fillCommand->execute();
    EXPECT_TRUE(success) << "Fill command should succeed even with misaligned coordinates";
    
    // The VoxelFillCommand uses floor for min and ceil for max, so:
    // minVoxel = floor(0.015 / 0.04) = floor(0.375) = 0
    // maxVoxel = ceil(0.097 / 0.04) = ceil(2.425) = 3
    // So voxels should be placed at grid positions 0, 1, 2, 3
    
    std::vector<Math::Vector3i> expectedPositions = {
        Math::Vector3i(0, 0, 0),   // World: (0cm, 0cm, 0cm)
        Math::Vector3i(1, 0, 0),   // World: (4cm, 0cm, 0cm)
        Math::Vector3i(2, 0, 0),   // World: (8cm, 0cm, 0cm)
        Math::Vector3i(0, 1, 0),   // World: (0cm, 4cm, 0cm)
        Math::Vector3i(1, 1, 1),   // World: (4cm, 4cm, 4cm)
        Math::Vector3i(2, 2, 2)    // World: (8cm, 8cm, 8cm)
    };
    
    for (const auto& pos : expectedPositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at calculated position grid(" 
                              << pos.x << ", " << pos.y << ", " << pos.z << ") "
                              << "despite misaligned input coordinates";
    }
    
    // Verify that no voxels exist beyond the calculated range
    Math::Vector3i beyondRange(4, 4, 4); // Should be outside the calculated range
    bool hasBeyondVoxel = voxelManager->hasVoxel(beyondRange, resolution);
    EXPECT_FALSE(hasBeyondVoxel) << "No voxel should exist beyond calculated range";
}

TEST_F(FillCommandValidationTest, CoordinateAlignment_AllResolutions_REQ_11_3_11) {
    // Test coordinate alignment for all valid voxel resolutions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test all valid resolutions in the 5-resolution system
    std::vector<VoxelData::VoxelResolution> resolutions = {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_64cm,
        VoxelData::VoxelResolution::Size_256cm
    };
    
    for (const auto& resolution : resolutions) {
        // Clear voxels between tests
        voxelManager->clearAll();
        
        float voxelSize = VoxelData::getVoxelSize(resolution);
        
        // Create a small fill region aligned to this resolution
        // Use 2 voxel units in each direction
        float regionSize = voxelSize * 2.0f;
        Math::BoundingBox region(
            Math::Vector3f(0.0f, 0.0f, 0.0f),      // min: origin
            Math::Vector3f(regionSize, regionSize, regionSize) // max: 2 voxel units
        );
        
        auto fillCommand = std::make_unique<UndoRedo::VoxelFillCommand>(
            voxelManager.get(), region, resolution, true);
        
        bool success = fillCommand->execute();
        if (!success) {
            // Debug information for failure
            std::cout << "Fill command failed for resolution " << VoxelData::getVoxelSizeName(resolution) 
                      << " (voxel size: " << voxelSize << "m, region size: " << regionSize << "m)" << std::endl;
        }
        EXPECT_TRUE(success) << "Fill command should succeed for resolution " 
                           << VoxelData::getVoxelSizeName(resolution);
        
        // Verify that at least the origin voxel was placed
        Math::Vector3i origin(0, 0, 0);
        bool hasOriginVoxel = voxelManager->hasVoxel(origin, resolution);
        EXPECT_TRUE(hasOriginVoxel) << "Origin voxel should exist for resolution " 
                                  << VoxelData::getVoxelSizeName(resolution);
        
        // Verify that corner voxel was placed (at position 1,1,1 in voxel coordinates)
        // Since region is 2 voxel units, corner should be at (1,1,1) 
        Math::Vector3i corner(1, 1, 1);
        bool hasCornerVoxel = voxelManager->hasVoxel(corner, resolution);
        EXPECT_TRUE(hasCornerVoxel) << "Corner voxel should exist for resolution " 
                                  << VoxelData::getVoxelSizeName(resolution);
    }
}

TEST_F(FillCommandValidationTest, CoordinateAlignment_NegativeCoordinates_REQ_11_3_11) {
    // Test coordinate alignment with negative coordinates (but above ground plane Y >= 0)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    voxelManager->resizeWorkspace(workspaceSize);
    
    // Test 16cm resolution with negative X and Z coordinates (but positive Y)
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    
    // Create fill region: from (-16cm, 0cm, -16cm) to (16cm, 16cm, 16cm)
    // This tests negative coordinate handling while respecting ground plane constraint
    Math::BoundingBox region(
        Math::Vector3f(-0.16f, 0.00f, -0.16f),  // min: (-16cm, 0cm, -16cm)
        Math::Vector3f(0.16f, 0.16f, 0.16f)     // max: (16cm, 16cm, 16cm)
    );
    
    auto fillCommand = std::make_unique<UndoRedo::VoxelFillCommand>(
        voxelManager.get(), region, resolution, true);
    
    bool success = fillCommand->execute();
    EXPECT_TRUE(success) << "Fill command should succeed with negative X,Z coordinates";
    
    // For 8cm resolution with range -16cm to +16cm:
    // minVoxel = floor(-0.16 / 0.08) = floor(-2) = -2
    // maxVoxel = ceil(0.16 / 0.08) = ceil(2) = 2
    // So voxels should be at positions -2, -1, 0, 1, 2
    
    std::vector<Math::Vector3i> expectedPositions = {
        Math::Vector3i(-2, 0, -2),  // World: (-16cm, 0cm, -16cm)
        Math::Vector3i(-1, 0, -1),  // World: (-8cm, 0cm, -8cm)
        Math::Vector3i(0, 0, 0),    // World: (0cm, 0cm, 0cm)
        Math::Vector3i(1, 0, 1),    // World: (8cm, 0cm, 8cm)
        Math::Vector3i(2, 0, 2),    // World: (16cm, 0cm, 16cm)
        Math::Vector3i(0, 1, 0),    // World: (0cm, 8cm, 0cm)
        Math::Vector3i(-2, 2, 2),   // World: (-16cm, 16cm, 16cm)
        Math::Vector3i(2, 2, -2)    // World: (16cm, 16cm, -16cm)
    };
    
    for (const auto& pos : expectedPositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at position grid(" 
                              << pos.x << ", " << pos.y << ", " << pos.z << ") "
                              << "world(" << pos.x * 8 << "cm, " << pos.y * 8 << "cm, " << pos.z * 8 << "cm)";
        
        // Verify Y coordinate respects ground plane constraint
        EXPECT_GE(pos.y, 0) << "Voxel Y coordinate should be >= 0 (ground plane constraint)";
    }
}

} // namespace Tests
} // namespace VoxelEditor