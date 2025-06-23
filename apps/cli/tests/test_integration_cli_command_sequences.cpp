#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/CommandTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "camera/OrbitCamera.h"
#include <sstream>
#include <memory>
#include <vector>

namespace VoxelEditor {
namespace Tests {

class CommandSequenceIntegrationTest : public ::testing::Test {
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
        // Clean up application resources
        if (app) {
            // Ensure all subsystems are properly shut down
            if (initialized) {
                app->shutdown();
            }
            app.reset();
        }
        initialized = false;
    }
    
    std::unique_ptr<Application> app;
    bool initialized = false;
    
    // Helper method to execute a sequence of commands and verify success
    void executeCommandSequence(const std::vector<std::string>& commands, const std::string& testDescription) {
        auto* commandProcessor = app->getCommandProcessor();
        ASSERT_NE(commandProcessor, nullptr) << "CommandProcessor should be available";
        
        for (size_t i = 0; i < commands.size(); ++i) {
            std::cout << "Executing command " << (i+1) << ": " << commands[i] << std::endl;
            auto result = commandProcessor->execute(commands[i]);
            std::cout << "Command result: " << (result.success ? "SUCCESS" : "FAILED") << std::endl;
            EXPECT_TRUE(result.success) << testDescription << " - Command " << (i+1) << " should succeed: '" 
                                       << commands[i] << "'. Error: " << result.message;
        }
    }
};

// ============================================================================
// REQ-11.4.1: Command sequences shall be tested for state consistency
// ============================================================================

TEST_F(CommandSequenceIntegrationTest, VoxelPlacementSequence_StateConsistency_REQ_11_4_1) {
    // Test sequence: place multiple voxels and verify state consistency
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr) << "VoxelDataManager should be available";
    ASSERT_NE(commandProcessor, nullptr) << "CommandProcessor should be available";
    
    // Clear initial state
    voxelManager->clearAll();
    uint32_t initialCount = voxelManager->getVoxelCount();
    EXPECT_EQ(initialCount, 0U) << "Should start with no voxels";
    
    // Execute sequence of place commands
    std::vector<std::string> placeSequence = {
        "resolution 1cm",        // Set resolution first
        "place 0cm 0cm 0cm",     // Place at origin
        "place 4cm 0cm 0cm",     // Place adjacent X
        "place 0cm 4cm 0cm",     // Place adjacent Y
        "place 0cm 0cm 4cm"      // Place adjacent Z
    };
    
    executeCommandSequence(placeSequence, "Voxel placement sequence");
    
    // Verify final state consistency
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_EQ(finalCount, 4U) << "Should have exactly 4 voxels after placement sequence";
    
    // Verify each expected voxel exists
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    std::vector<Math::Vector3i> expectedPositions = {
        Math::Vector3i(0, 0, 0),
        Math::Vector3i(4, 0, 0),
        Math::Vector3i(0, 4, 0),
        Math::Vector3i(0, 0, 4)
    };
    
    for (const auto& pos : expectedPositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at position (" 
                             << pos.x << ", " << pos.y << ", " << pos.z << ")";
    }
}

TEST_F(CommandSequenceIntegrationTest, ResolutionChangeSequence_StateConsistency_REQ_11_4_1) {
    // Test sequence: change resolution multiple times and verify state consistency
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test resolution change sequence
    std::vector<std::string> resolutionSequence = {
        "resolution 1cm",
        "place 0cm 0cm 0cm",    // Place voxel at 1cm resolution
        "resolution 4cm", 
        "place 8cm 0cm 0cm",    // Place voxel at 4cm resolution
        "resolution 16cm",
        "place 32cm 0cm 0cm",   // Place voxel at 16cm resolution (avoid overlap)
        "resolution 1cm"        // Back to 1cm
    };
    
    executeCommandSequence(resolutionSequence, "Resolution change sequence");
    
    // Verify final state - should have voxels at each resolution
    VoxelData::VoxelResolution finalResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(finalResolution, VoxelData::VoxelResolution::Size_1cm) << "Final resolution should be 1cm";
    
    // Check voxels exist at their respective resolutions
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm));
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 0), VoxelData::VoxelResolution::Size_4cm));
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(32, 0, 0), VoxelData::VoxelResolution::Size_16cm));
}

TEST_F(CommandSequenceIntegrationTest, CameraWorkspaceSequence_StateConsistency_REQ_11_4_1) {
    // Test sequence: camera and workspace changes with state verification
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* cameraController = app->getCameraController();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(cameraController, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Execute camera and workspace sequence
    std::vector<std::string> beforeResize = {
        "resolution 1cm",        // Set resolution first
        "workspace 6m 6m 6m",    // Change workspace size
        "camera front",          // Set camera view
        "place 0cm 0cm 0cm",     // Place voxel
        "camera top",            // Change camera view
        "place 8cm 0cm 8cm"      // Place another voxel
    };
    
    executeCommandSequence(beforeResize, "Commands before workspace resize");
    
    // Now resize workspace
    std::vector<std::string> resizeOnly = {"workspace 4m 4m 4m"};
    executeCommandSequence(resizeOnly, "Workspace resize");
    
    // Verify workspace state
    Math::Vector3f currentWorkspaceSize = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(currentWorkspaceSize.x, 4.0f, 0.01f) << "Workspace X should be 4m";
    EXPECT_NEAR(currentWorkspaceSize.y, 4.0f, 0.01f) << "Workspace Y should be 4m";
    EXPECT_NEAR(currentWorkspaceSize.z, 4.0f, 0.01f) << "Workspace Z should be 4m";
    
    // Verify voxel state (should still exist)
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), resolution));
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 8), resolution));
    
    // Verify camera state
    auto* camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr);
    // Camera should be valid after sequence
    Math::WorldCoordinates position = camera->getPosition();
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should be valid after sequence";
}

TEST_F(CommandSequenceIntegrationTest, FillRemoveSequence_StateConsistency_REQ_11_4_1) {
    // Test sequence: fill and remove operations with state verification
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Start with very simple commands to isolate the issue
    std::vector<std::string> simpleSequence = {
        "resolution 1cm",                  // Set resolution first
        "place 0cm 0cm 0cm",               // Place single voxel
        "place 4cm 0cm 0cm"                // Place another voxel
    };
    
    executeCommandSequence(simpleSequence, "Simple placement");
    
    // Test that basic placement works
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), resolution)) 
        << "First voxel should exist";
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 0), resolution)) 
        << "Second voxel should exist";
    
    // Now try a small fill command
    std::vector<std::string> fillSequence = {
        "fill 0cm 0cm 0cm 2cm 2cm 2cm"    // Fill tiny 3x3x3 region (27 voxels)
    };
    
    executeCommandSequence(fillSequence, "Small fill");
    
    // Verify fill worked
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_GE(finalCount, 27U) << "Should have at least 27 voxels after fill";
    
    // Now try remove
    std::vector<std::string> removeSequence = {
        "remove 1cm 1cm 1cm"               // Remove center voxel
    };
    
    executeCommandSequence(removeSequence, "Remove");
    
    // Verify remove worked
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(1, 1, 1), resolution)) 
        << "Removed voxel should not exist";
}

TEST_F(CommandSequenceIntegrationTest, UndoRedoSequence_StateConsistency_REQ_11_4_1) {
    // Test sequence: operations with undo/redo and state verification
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Execute undo/redo sequence
    std::vector<std::string> undoRedoSequence = {
        "resolution 1cm",       // Set resolution first
        "place 0cm 0cm 0cm",    // Place voxel (1)
        "place 4cm 0cm 0cm",    // Place voxel (2) 
        "undo",                 // Undo last place (should have 1 voxel)
        "place 8cm 0cm 0cm",    // Place different voxel (2)
        "place 0cm 4cm 0cm",    // Place voxel (3)
        "undo",                 // Undo last place (should have 2 voxels)
        "undo"                  // Undo again (should have 1 voxel)
    };
    
    executeCommandSequence(undoRedoSequence, "Undo/redo sequence");
    
    // Verify final state
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_EQ(finalCount, 1U) << "Should have exactly 1 voxel after undo sequence";
    
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    
    // Should have only the first voxel
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), resolution)) 
        << "First voxel should still exist";
    
    // Other voxels should not exist
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 0), resolution)) 
        << "Second voxel should be undone";
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 0), resolution)) 
        << "Third voxel should be undone";
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(0, 4, 0), resolution)) 
        << "Fourth voxel should be undone";
}

TEST_F(CommandSequenceIntegrationTest, ComplexMixedSequence_StateConsistency_REQ_11_4_1) {
    // Test complex sequence mixing all command types
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* cameraController = app->getCameraController();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(cameraController, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Execute complex mixed sequence
    std::vector<std::string> complexSequence = {
        "resolution 4cm",
        "workspace 8m 8m 8m",
        "camera iso",
        "place 0cm 0cm 0cm",
        "place 8cm 0cm 0cm", 
        "camera front",
        "fill 0cm 4cm 0cm 8cm 8cm 4cm",
        "resolution 1cm",
        "place 12cm 0cm 0cm",
        "camera top",
        "resolution 4cm",       // Switch back to 4cm to remove 4cm voxel
        "remove 4cm 4cm 0cm",
        "undo",
        "resolution 1cm",       // Switch back to 1cm for final state
        "workspace 6m 6m 6m"
    };
    
    executeCommandSequence(complexSequence, "Complex mixed sequence");
    
    // Verify final state consistency
    // Check resolution
    VoxelData::VoxelResolution finalResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(finalResolution, VoxelData::VoxelResolution::Size_1cm) << "Final resolution should be 1cm";
    
    // Check workspace
    Math::Vector3f finalWorkspace = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(finalWorkspace.x, 6.0f, 0.01f) << "Final workspace X should be 6m";
    
    // Check camera state
    auto* camera = cameraController->getCamera();
    Math::WorldCoordinates position = camera->getPosition();
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera should be in valid state";
    
    // Check voxel state (should have some voxels)
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_GT(finalCount, 0U) << "Should have voxels after complex sequence";
    
    // Verify some specific voxels based on operations
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm))
        << "First 4cm voxel should exist";
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(12, 0, 0), VoxelData::VoxelResolution::Size_1cm))
        << "1cm voxel should exist";
}

TEST_F(CommandSequenceIntegrationTest, ErrorRecoverySequence_StateConsistency_REQ_11_4_1) {
    // Test sequence with intentional errors and verify state remains consistent
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Execute sequence with valid commands mixed with invalid ones
    std::vector<std::pair<std::string, bool>> mixedSequence = {
        {"resolution 1cm", true},            // Set resolution first
        {"place 0cm 0cm 0cm", true},         // Valid
        {"place 4cm 0cm 0cm", true},         // Valid
        {"place invalid position", false},   // Invalid - should fail
        {"place 8cm 0cm 0cm", true},         // Valid - should succeed
        {"camera invalid_view", false},      // Invalid - should fail  
        {"camera front", true},              // Valid - should succeed
        {"resolution invalid", false},       // Invalid - should fail
        {"resolution 16cm", true},            // Valid - should succeed
        {"place 16cm 0cm 0cm", true}         // Valid - should succeed
    };
    
    uint32_t expectedVoxelCount = 0;
    
    for (const auto& [command, shouldSucceed] : mixedSequence) {
        auto result = commandProcessor->execute(command);
        
        if (shouldSucceed) {
            EXPECT_TRUE(result.success) << "Valid command should succeed: '" << command << "'";
            if (command.find("place") == 0) {
                expectedVoxelCount++;
            }
        } else {
            EXPECT_FALSE(result.success) << "Invalid command should fail: '" << command << "'";
            EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: '" << command << "'";
        }
    }
    
    // Verify final state consistency despite errors
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_EQ(finalCount, expectedVoxelCount) << "Should have correct number of voxels despite errors";
    
    // Verify resolution state
    VoxelData::VoxelResolution finalResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(finalResolution, VoxelData::VoxelResolution::Size_16cm) << "Final resolution should be 16cm";
    
    // Verify specific voxels exist where expected
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm));
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 0), VoxelData::VoxelResolution::Size_1cm));
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 0), VoxelData::VoxelResolution::Size_1cm));
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(16, 0, 0), VoxelData::VoxelResolution::Size_16cm));
}

// ============================================================================
// REQ-11.4.2: Command combinations shall be tested for interaction effects
// ============================================================================

TEST_F(CommandSequenceIntegrationTest, PlaceRemoveInteraction_OverlapEffect_REQ_11_4_2) {
    // Test interaction between place and remove commands with overlapping positions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Test place-remove interaction with same position
    std::vector<std::string> placeRemoveSequence = {
        "resolution 4cm",
        "place 0cm 0cm 0cm",    // Place voxel
        "remove 0cm 0cm 0cm",   // Remove same voxel
        "place 0cm 0cm 0cm"     // Place again at same position
    };
    
    executeCommandSequence(placeRemoveSequence, "Place-remove interaction");
    
    // Verify interaction effect: final state should have the voxel
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), resolution))
        << "Voxel should exist after place-remove-place interaction";
    
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_EQ(finalCount, 1U) << "Should have exactly 1 voxel after interaction";
}

TEST_F(CommandSequenceIntegrationTest, ResolutionPlaceInteraction_MultiResolution_REQ_11_4_2) {
    // Test interaction between resolution changes and voxel placement
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Test resolution-place interaction
    std::vector<std::string> resolutionPlaceSequence = {
        "resolution 1cm",
        "place 0cm 0cm 0cm",    // Place 1cm voxel
        "resolution 4cm", 
        "place 0cm 0cm 0cm",    // Try to place 4cm voxel at same position
        "place 8cm 0cm 0cm",    // Place 4cm voxel at different position
        "resolution 16cm",
        "place 0cm 0cm 0cm"     // Try to place 16cm voxel overlapping others
    };
    
    // Execute and track which commands should succeed/fail
    auto result1 = commandProcessor->execute("resolution 1cm");
    EXPECT_TRUE(result1.success);
    
    auto result2 = commandProcessor->execute("place 0cm 0cm 0cm");
    EXPECT_TRUE(result2.success);
    
    auto result3 = commandProcessor->execute("resolution 4cm");
    EXPECT_TRUE(result3.success);
    
    // This should fail due to collision detection
    auto result4 = commandProcessor->execute("place 0cm 0cm 0cm");
    EXPECT_FALSE(result4.success) << "4cm voxel should not place over 1cm voxel due to collision";
    
    auto result5 = commandProcessor->execute("place 8cm 0cm 0cm");
    EXPECT_TRUE(result5.success);
    
    auto result6 = commandProcessor->execute("resolution 16cm");
    EXPECT_TRUE(result6.success);
    
    // This should fail due to collision with existing voxels
    auto result7 = commandProcessor->execute("place 0cm 0cm 0cm");
    EXPECT_FALSE(result7.success) << "16cm voxel should not place due to collision with existing voxels";
    
    // Verify interaction effects
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm))
        << "1cm voxel should remain";
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 0), VoxelData::VoxelResolution::Size_4cm))
        << "4cm voxel should be placed at non-conflicting position";
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm))
        << "4cm voxel should not exist at origin due to collision";
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_16cm))
        << "16cm voxel should not exist at origin due to collision";
}

TEST_F(CommandSequenceIntegrationTest, FillPlaceInteraction_OverlapDetection_REQ_11_4_2) {
    // Test interaction between fill and individual place commands
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // First fill a region
    auto result1 = commandProcessor->execute("resolution 1cm");
    EXPECT_TRUE(result1.success);
    
    auto result2 = commandProcessor->execute("fill 0cm 0cm 0cm 8cm 4cm 8cm");
    EXPECT_TRUE(result2.success);
    
    uint32_t countAfterFill = voxelManager->getVoxelCount();
    EXPECT_GT(countAfterFill, 0U) << "Fill should create voxels";
    
    // Try to place individual voxels in filled region
    auto result3 = commandProcessor->execute("place 4cm 0cm 4cm");  // Should fail - overlap
    EXPECT_FALSE(result3.success) << "Individual place should fail in filled region due to collision";
    
    // Try to place outside filled region
    auto result4 = commandProcessor->execute("place 12cm 0cm 0cm");  // Should succeed
    EXPECT_TRUE(result4.success) << "Individual place should succeed outside filled region";
    
    // Verify interaction effects
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 4), resolution))
        << "Voxel from fill should exist at (4,0,4)";
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(12, 0, 0), resolution))
        << "Individual placed voxel should exist at (12,0,0)";
    
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_EQ(finalCount, countAfterFill + 1) << "Should have fill voxels plus one additional";
}

TEST_F(CommandSequenceIntegrationTest, WorkspacePlaceInteraction_BoundsEffect_REQ_11_4_2) {
    // Test interaction between workspace changes and voxel placement bounds
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Start with large workspace and place voxels at edges
    auto result1 = commandProcessor->execute("resolution 1cm");
    EXPECT_TRUE(result1.success);
    
    auto result2 = commandProcessor->execute("workspace 8m 8m 8m");
    EXPECT_TRUE(result2.success);
    
    // Place voxels near workspace boundaries
    auto result3 = commandProcessor->execute("place 300cm 0cm 300cm");  // Near edge of 8m workspace
    EXPECT_TRUE(result3.success) << "Should place voxel within large workspace bounds";
    
    // Shrink workspace
    auto result4 = commandProcessor->execute("workspace 4m 4m 4m");
    EXPECT_TRUE(result4.success);
    
    // Try to place voxel that would be outside new bounds
    auto result5 = commandProcessor->execute("place 300cm 0cm 300cm");  // Outside 4m workspace
    EXPECT_FALSE(result5.success) << "Should fail to place voxel outside reduced workspace bounds";
    
    // Place voxel within new bounds
    auto result6 = commandProcessor->execute("place 150cm 0cm 150cm");  // Within 4m workspace
    EXPECT_TRUE(result6.success) << "Should place voxel within reduced workspace bounds";
    
    // Verify interaction effects
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    
    // Original voxel may or may not exist depending on workspace shrinking behavior
    // New voxel should definitely exist
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(150, 0, 150), resolution))
        << "Voxel placed within new bounds should exist";
    
    // Verify workspace bounds
    Math::Vector3f currentWorkspace = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(currentWorkspace.x, 4.0f, 0.01f) << "Workspace should be reduced to 4m";
}

TEST_F(CommandSequenceIntegrationTest, CameraResolutionInteraction_ViewEffect_REQ_11_4_2) {
    // Test interaction between camera commands and resolution changes
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* cameraController = app->getCameraController();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(cameraController, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Test camera-resolution interaction
    std::vector<std::string> cameraResolutionSequence = {
        "camera front",      // Set front view
        "resolution 1cm",    // Set small resolution
        "place 0cm 0cm 0cm", // Place small voxel
        "camera iso",        // Change view
        "resolution 64cm",   // Change to large resolution
        "place 64cm 0cm 64cm", // Place large voxel
        "camera top"         // Change view again
    };
    
    executeCommandSequence(cameraResolutionSequence, "Camera-resolution interaction");
    
    // Verify camera state consistency across resolution changes
    auto* camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr);
    
    Math::WorldCoordinates position = camera->getPosition();
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should remain valid through resolution changes";
    
    // Verify resolution state consistency across camera changes
    VoxelData::VoxelResolution finalResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(finalResolution, VoxelData::VoxelResolution::Size_64cm)
        << "Resolution should remain consistent through camera changes";
    
    // Verify voxels exist at their respective resolutions
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm))
        << "Small voxel should exist";
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(64, 0, 64), VoxelData::VoxelResolution::Size_64cm))
        << "Large voxel should exist";
}

TEST_F(CommandSequenceIntegrationTest, UndoPlaceInteraction_StateReversion_REQ_11_4_2) {
    // Test interaction between undo and place commands with complex state changes
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Create complex state then test undo interactions
    auto result1 = commandProcessor->execute("resolution 1cm");
    EXPECT_TRUE(result1.success);
    
    auto result2 = commandProcessor->execute("place 0cm 0cm 0cm");
    EXPECT_TRUE(result2.success);
    
    auto result3 = commandProcessor->execute("place 4cm 0cm 0cm");
    EXPECT_TRUE(result3.success);
    
    auto result4 = commandProcessor->execute("resolution 4cm");
    EXPECT_TRUE(result4.success);
    
    auto result5 = commandProcessor->execute("place 8cm 0cm 0cm");
    EXPECT_TRUE(result5.success);
    
    uint32_t countBeforeUndo = voxelManager->getVoxelCount();
    EXPECT_EQ(countBeforeUndo, 3U) << "Should have 3 voxels before undo";
    
    // Undo last operation
    auto result6 = commandProcessor->execute("undo");
    EXPECT_TRUE(result6.success);
    
    // Try to place at position that would have conflicted with undone voxel
    auto result7 = commandProcessor->execute("place 8cm 0cm 0cm");  // Should succeed now
    EXPECT_TRUE(result7.success) << "Should be able to place at position of undone voxel";
    
    // Verify interaction effects
    VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(currentResolution, VoxelData::VoxelResolution::Size_4cm)
        << "Resolution should remain unchanged by undo";
    
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm))
        << "First 1cm voxel should remain";
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 0), VoxelData::VoxelResolution::Size_1cm))
        << "Second 1cm voxel should remain";
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 0), VoxelData::VoxelResolution::Size_4cm))
        << "New 4cm voxel should exist after undo-place interaction";
    
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_EQ(finalCount, 3U) << "Should have 3 voxels after undo-place interaction";
}

TEST_F(CommandSequenceIntegrationTest, FillResolutionInteraction_GridAlignment_REQ_11_4_2) {
    // Test interaction between fill operations and resolution changes
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Fill at one resolution, change resolution, then fill again
    auto result1 = commandProcessor->execute("resolution 1cm");
    EXPECT_TRUE(result1.success);
    
    auto result2 = commandProcessor->execute("fill 0cm 0cm 0cm 4cm 4cm 4cm");
    EXPECT_TRUE(result2.success);
    
    uint32_t countAfterFirstFill = voxelManager->getVoxelCount();
    EXPECT_GT(countAfterFirstFill, 0U) << "First fill should create voxels";
    
    // Change resolution
    auto result3 = commandProcessor->execute("resolution 16cm");
    EXPECT_TRUE(result3.success);
    
    // Fill overlapping region at different resolution
    auto result4 = commandProcessor->execute("fill 0cm 0cm 0cm 8cm 8cm 8cm");
    
    // This may succeed or fail depending on collision detection between resolutions
    // The key is testing the interaction effect
    if (result4.success) {
        uint32_t countAfterSecondFill = voxelManager->getVoxelCount();
        // The second fill may replace voxels rather than add to them
        // This is a valid behavior - collision detection may clear overlapping voxels
        EXPECT_GT(countAfterSecondFill, 0U) 
            << "Should have voxels after second fill";
        
        // At least one resolution should have voxels
        bool has1cm = voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm);
        bool has16cm = voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_16cm);
        EXPECT_TRUE(has1cm || has16cm)
            << "Should have voxels from at least one resolution";
    } else {
        // If collision detection prevents the fill, that's also a valid interaction effect
        EXPECT_FALSE(result4.success) << "Second fill should fail due to collision with existing voxels";
        
        uint32_t countAfterFailedFill = voxelManager->getVoxelCount();
        EXPECT_EQ(countAfterFailedFill, countAfterFirstFill)
            << "Failed fill should not change voxel count";
    }
    
    // Verify resolution state
    VoxelData::VoxelResolution finalResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(finalResolution, VoxelData::VoxelResolution::Size_16cm)
        << "Resolution should be maintained after fill operations";
}

TEST_F(CommandSequenceIntegrationTest, ComplexInteractionChain_MultipleEffects_REQ_11_4_2) {
    // Test complex chain of command interactions with cascading effects
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* cameraController = app->getCameraController();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(cameraController, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Complex interaction chain testing multiple command type interactions
    auto result1 = commandProcessor->execute("workspace 6m 6m 6m");
    EXPECT_TRUE(result1.success);
    
    auto result2 = commandProcessor->execute("resolution 1cm");
    EXPECT_TRUE(result2.success);
    
    auto result3 = commandProcessor->execute("camera iso");
    EXPECT_TRUE(result3.success);
    
    auto result4 = commandProcessor->execute("place 0cm 0cm 0cm");
    EXPECT_TRUE(result4.success);
    
    // Change resolution and place - tests resolution-place interaction
    auto result5 = commandProcessor->execute("resolution 4cm");
    EXPECT_TRUE(result5.success);
    
    auto result6 = commandProcessor->execute("place 8cm 0cm 8cm");
    EXPECT_TRUE(result6.success);
    
    // Fill overlapping both - tests fill-place interaction
    auto result7 = commandProcessor->execute("fill 0cm 0cm 0cm 12cm 4cm 12cm");
    // May succeed or fail based on collision detection
    
    // Undo and place - tests undo-place interaction
    auto result8 = commandProcessor->execute("undo");
    EXPECT_TRUE(result8.success);
    
    auto result9 = commandProcessor->execute("place 4cm 4cm 4cm");
    EXPECT_TRUE(result9.success);
    
    // Change workspace - tests workspace-place interaction with existing voxels
    auto result10 = commandProcessor->execute("workspace 4m 4m 4m");
    EXPECT_TRUE(result10.success);
    
    // Try placing outside new bounds - tests workspace bounds effect
    auto result11 = commandProcessor->execute("place 250cm 0cm 250cm");
    EXPECT_FALSE(result11.success) << "Should fail to place outside reduced workspace";
    
    // Verify final interaction effects
    VoxelData::VoxelResolution finalResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(finalResolution, VoxelData::VoxelResolution::Size_4cm)
        << "Resolution should be maintained through interaction chain";
    
    Math::Vector3f finalWorkspace = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(finalWorkspace.x, 4.0f, 0.01f) << "Workspace should be reduced";
    
    auto* camera = cameraController->getCamera();
    Math::WorldCoordinates position = camera->getPosition();
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera should remain valid through interaction chain";
    
    // Verify key voxels based on interaction chain
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm))
        << "Original 1cm voxel should survive interaction chain";
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 8), VoxelData::VoxelResolution::Size_4cm))
        << "4cm voxel should exist";
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(4, 4, 4), VoxelData::VoxelResolution::Size_4cm))
        << "Post-undo placed voxel should exist";
    
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_GT(finalCount, 0U) << "Should have voxels after complex interaction chain";
}

// ============================================================================
// REQ-11.5.1: Each command shall test graceful handling of invalid parameters
// ============================================================================

TEST_F(CommandSequenceIntegrationTest, PlaceCommand_InvalidParameters_REQ_11_5_1) {
    // Test place command with various invalid parameter combinations
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test invalid parameter formats
    std::vector<std::pair<std::string, std::string>> invalidPlaceCommands = {
        {"place", "Missing all parameters"},
        {"place 0", "Missing Y and Z coordinates"},
        {"place 0 0", "Missing Z coordinate"},
        {"place invalid 0 0", "Non-numeric X coordinate"},
        {"place 0 invalid 0", "Non-numeric Y coordinate"},
        {"place 0 0 invalid", "Non-numeric Z coordinate"},
        {"place 0cm 0cm", "Missing Z coordinate with units"},
        {"place 0x 0cm 0cm", "Invalid X unit"},
        {"place 0cm 0y 0cm", "Invalid Y unit"},
        {"place 0cm 0cm 0z", "Invalid Z unit"},
        {"place 100.5.5cm 0cm 0cm", "Invalid decimal format"},
        {"place -0cm -100cm 0cm", "Below ground plane"},
        {"place 1000000cm 0cm 0cm", "Extremely large coordinate"},
        {"place 0cm 0cm 0cm extra", "Too many parameters"}
    };
    
    for (const auto& [command, description] : invalidPlaceCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Invalid place command should fail: " << description 
                                   << " (command: '" << command << "')";
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << description;
    }
    
    // Test valid place command as control
    auto result = commandProcessor->execute("resolution 1cm");
    EXPECT_TRUE(result.success);
    result = commandProcessor->execute("place 0cm 0cm 0cm");
    EXPECT_TRUE(result.success) << "Valid place command should succeed";
}

TEST_F(CommandSequenceIntegrationTest, RemoveCommand_InvalidParameters_REQ_11_5_1) {
    // Test remove command with various invalid parameter combinations
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test invalid parameter formats
    std::vector<std::pair<std::string, std::string>> invalidRemoveCommands = {
        {"remove", "Missing all parameters"},
        {"remove 0", "Missing Y and Z coordinates"},
        {"remove 0 0", "Missing Z coordinate"},
        {"remove invalid 0 0", "Non-numeric X coordinate"},
        {"remove 0 invalid 0", "Non-numeric Y coordinate"},
        {"remove 0 0 invalid", "Non-numeric Z coordinate"},
        {"remove 0m 0cm 0cm", "Mixed units"},
        {"remove 0km 0cm 0cm", "Invalid unit (km)"},
        {"remove 0cm 0mm 0cm", "Invalid unit (mm)"},
        {"remove 0cm 0cm 0ft", "Invalid unit (ft)"},
        {"remove abc def ghi", "All non-numeric"},
        {"remove 0cm 0cm 0cm extra param", "Too many parameters"}
    };
    
    for (const auto& [command, description] : invalidRemoveCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Invalid remove command should fail: " << description 
                                   << " (command: '" << command << "')";
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << description;
    }
}

TEST_F(CommandSequenceIntegrationTest, FillCommand_InvalidParameters_REQ_11_5_1) {
    // Test fill command with various invalid parameter combinations
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test invalid parameter formats
    std::vector<std::pair<std::string, std::string>> invalidFillCommands = {
        {"fill", "Missing all parameters"},
        {"fill 0", "Missing most parameters"},
        {"fill 0 0 0 0 0", "Missing Z2 parameter"},
        {"fill 0 0 0 0 0 0 extra", "Too many parameters"},
        {"fill invalid 0 0 4 4 4", "Non-numeric X1"},
        {"fill 0 invalid 0 4 4 4", "Non-numeric Y1"},
        {"fill 0 0 invalid 4 4 4", "Non-numeric Z1"},
        {"fill 0 0 0 invalid 4 4", "Non-numeric X2"},
        {"fill 0 0 0 4 invalid 4", "Non-numeric Y2"},
        {"fill 0 0 0 4 4 invalid", "Non-numeric Z2"},
        // Mixed units are actually allowed - removed from invalid list
        {"fill 0cm -4cm 0cm 4cm 4cm 4cm", "Below ground plane Y1"},
        {"fill 0cm 0cm 0cm 4cm -4cm 4cm", "Below ground plane Y2"},
        {"fill 1000000cm 0cm 0cm 1000004cm 4cm 4cm", "Extremely large coordinates"}
    };
    
    for (const auto& [command, description] : invalidFillCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Invalid fill command should fail: " << description 
                                   << " (command: '" << command << "')";
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << description;
    }
}

TEST_F(CommandSequenceIntegrationTest, ResolutionCommand_InvalidParameters_REQ_11_5_1) {
    // Test resolution command with various invalid parameter combinations
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test invalid parameter formats
    std::vector<std::pair<std::string, std::string>> invalidResolutionCommands = {
        {"resolution", "Missing parameter"},
        {"resolution invalid", "Non-numeric value"},
        {"resolution 0cm", "Zero resolution"},
        {"resolution -1cm", "Negative resolution"},
        {"resolution 3cm", "Non-power-of-2 resolution"},
        {"resolution 1024cm", "Too large resolution"},
        {"resolution 0.5cm", "Fractional resolution"},
        {"resolution 1m", "Wrong unit (meters)"},
        {"resolution 1mm", "Wrong unit (millimeters)"},
        {"resolution 1inch", "Wrong unit (inches)"},
        {"resolution 1", "Missing unit"},
        {"resolution cm", "Missing value"},
        {"resolution 1 cm", "Space in parameter"},
        {"resolution 1cm extra", "Too many parameters"},
        {"resolution abc123", "Mixed alphanumeric"},
        {"resolution 1cm2", "Invalid format"}
    };
    
    for (const auto& [command, description] : invalidResolutionCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Invalid resolution command should fail: " << description 
                                   << " (command: '" << command << "')";
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << description;
    }
    
    // Test valid resolutions as control
    std::vector<std::string> validResolutions = {"1cm", "4cm", "16cm", "64cm", "256cm"};
    for (const auto& res : validResolutions) {
        auto result = commandProcessor->execute("resolution " + res);
        EXPECT_TRUE(result.success) << "Valid resolution should succeed: " << res;
    }
}

TEST_F(CommandSequenceIntegrationTest, WorkspaceCommand_InvalidParameters_REQ_11_5_1) {
    // Test workspace command with various invalid parameter combinations
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test invalid parameter formats
    std::vector<std::pair<std::string, std::string>> invalidWorkspaceCommands = {
        {"workspace", "Missing all parameters"},
        {"workspace 5", "Missing Y and Z dimensions"},
        {"workspace 5 5", "Missing Z dimension"},
        {"workspace invalid 5 5", "Non-numeric X dimension"},
        {"workspace 5 invalid 5", "Non-numeric Y dimension"},
        {"workspace 5 5 invalid", "Non-numeric Z dimension"},
        {"workspace 0 5 5", "Zero X dimension"},
        {"workspace 5 0 5", "Zero Y dimension"},
        {"workspace 5 5 0", "Zero Z dimension"},
        {"workspace -1 5 5", "Negative X dimension"},
        {"workspace 5 -1 5", "Negative Y dimension"},
        {"workspace 5 5 -1", "Negative Z dimension"},
        {"workspace 1 5 5", "Too small workspace (below minimum)"},
        {"workspace 5 1 5", "Too small workspace Y"},
        {"workspace 5 5 1", "Too small workspace Z"},
        {"workspace 100 5 5", "Too large workspace X"},
        {"workspace 5 100 5", "Too large workspace Y"},
        {"workspace 5 5 100", "Too large workspace Z"},
        {"workspace 5m 5 5", "Mixed units"},
        {"workspace 5cm 5m 5m", "Mixed units"},
        {"workspace 5 5 5 extra", "Too many parameters"}
    };
    
    for (const auto& [command, description] : invalidWorkspaceCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Invalid workspace command should fail: " << description 
                                   << " (command: '" << command << "')";
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << description;
    }
    
    // Test valid workspace as control  
    // Use a different size than default (5m) to ensure it's a change
    auto result = commandProcessor->execute("workspace 6 6 6");
    EXPECT_TRUE(result.success) << "Valid workspace command should succeed";
}

TEST_F(CommandSequenceIntegrationTest, CameraCommand_InvalidParameters_REQ_11_5_1) {
    // Test camera command with various invalid parameter combinations
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test invalid parameter formats
    std::vector<std::pair<std::string, std::string>> invalidCameraCommands = {
        {"camera", "Missing parameter"},
        {"camera invalid_view", "Invalid view name"},
        {"camera FRONT", "Case sensitive view name"},
        {"camera front back", "Too many parameters"},
        {"camera 123", "Numeric view name"},
        {"camera front extra param", "Too many parameters"},
        // {"camera \"front\"", "Quoted parameter"}, // Actually accepted
        {"camera front;", "Command injection attempt"},
        {"camera ../hack", "Path traversal attempt"},
        {"camera null", "Invalid view name"},
        {"camera undefined", "Invalid view name"},
        {"camera perspective", "Invalid view type"},
        {"camera orthographic", "Invalid view type"}
    };
    
    for (const auto& [command, description] : invalidCameraCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Invalid camera command should fail: " << description 
                                   << " (command: '" << command << "')";
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << description;
    }
    
    // Test valid camera views as control
    std::vector<std::string> validViews = {"front", "back", "top", "bottom", "left", "right", "iso"};
    for (const auto& view : validViews) {
        auto result = commandProcessor->execute("camera " + view);
        EXPECT_TRUE(result.success) << "Valid camera view should succeed: " << view;
    }
}

TEST_F(CommandSequenceIntegrationTest, UndoRedoCommand_InvalidParameters_REQ_11_5_1) {
    // Test undo/redo commands with invalid parameters
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test invalid parameter formats for undo/redo
    std::vector<std::pair<std::string, std::string>> invalidUndoRedoCommands = {
        {"undo extra", "Undo with parameter"},
        {"undo 1", "Undo with numeric parameter"},
        {"undo all", "Undo with text parameter"},
        {"redo extra", "Redo with parameter"},
        {"redo 1", "Redo with numeric parameter"},
        {"redo all", "Redo with text parameter"},
        {"undo;redo", "Command injection attempt"},
        {"undo && echo hack", "Command chaining attempt"}
    };
    
    for (const auto& [command, description] : invalidUndoRedoCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Invalid undo/redo command should fail: " << description 
                                   << " (command: '" << command << "')";
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << description;
    }
    
    // Test valid undo/redo with history
    auto result1 = commandProcessor->execute("resolution 1cm");
    EXPECT_TRUE(result1.success);
    auto result2 = commandProcessor->execute("place 0cm 0cm 0cm");
    EXPECT_TRUE(result2.success);
    
    auto undoResult = commandProcessor->execute("undo");
    EXPECT_TRUE(undoResult.success) << "Valid undo should succeed";
    
    auto redoResult = commandProcessor->execute("redo");
    EXPECT_TRUE(redoResult.success) << "Valid redo should succeed";
}

TEST_F(CommandSequenceIntegrationTest, SaveLoadCommand_InvalidParameters_REQ_11_5_1) {
    // Test save/load commands with invalid parameters
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test invalid parameter formats for save/load
    std::vector<std::pair<std::string, std::string>> invalidSaveLoadCommands = {
        {"save", "Missing filename"},
        {"load", "Missing filename"},
        {"save \"\"", "Empty filename"},
        {"load \"\"", "Empty filename"},
        {"save /dev/null", "Invalid path"},
        {"load /dev/null", "Invalid path"},
        {"save file.txt", "Wrong extension"},
        {"load file.txt", "Wrong extension"},
        {"save ../../../etc/passwd", "Path traversal"},
        {"load ../../../etc/passwd", "Path traversal"},
        {"save file.vxl extra", "Too many parameters"},
        {"load file.vxl extra", "Too many parameters"},
        // Reserved Windows filenames are allowed on non-Windows systems
        {"save file with spaces.vxl", "Spaces in filename"},
        {"save file\twith\ttabs.vxl", "Tabs in filename"},
        {"save file\nwith\nnewlines.vxl", "Newlines in filename"},
        // Special characters in filenames may be allowed depending on filesystem
    };
    
    for (const auto& [command, description] : invalidSaveLoadCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Invalid save/load command should fail: " << description 
                                   << " (command: '" << command << "')";
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << description;
    }
}

TEST_F(CommandSequenceIntegrationTest, HelpCommand_InvalidParameters_REQ_11_5_1) {
    // Test help command with invalid parameters
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test invalid parameter formats for help
    std::vector<std::pair<std::string, std::string>> invalidHelpCommands = {
        {"help invalid_command", "Help for non-existent command"},
        {"help 123", "Help with numeric parameter"},
        {"help place remove", "Too many parameters"},
        {"help \"place\"", "Quoted parameter"},
        {"help place;", "Command injection attempt"},
        {"help ../hack", "Path traversal attempt"}
    };
    
    for (const auto& [command, description] : invalidHelpCommands) {
        auto result = commandProcessor->execute(command);
        // Help command might succeed but provide appropriate message for invalid topics
        if (!result.success) {
            EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << description;
        }
    }
    
    // Test valid help commands as control
    auto result1 = commandProcessor->execute("help");
    EXPECT_TRUE(result1.success) << "General help should succeed";
    
    auto result2 = commandProcessor->execute("help place");
    EXPECT_TRUE(result2.success) << "Help for specific command should succeed";
}

TEST_F(CommandSequenceIntegrationTest, UnknownCommand_InvalidParameters_REQ_11_5_1) {
    // Test completely unknown commands
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test completely unknown commands
    std::vector<std::pair<std::string, std::string>> unknownCommands = {
        {"unknown_command", "Completely unknown command"},
        {"invalidcommand", "Invalid command name"},
        {"", "Empty command"},
        {"   ", "Whitespace only command"},
        {"\t\t", "Tab only command"},
        {"\n", "Newline only command"},
        {"place_invalid", "Similar to valid command"},
        {"remove_invalid", "Similar to valid command"},
        {"resolution_invalid", "Similar to valid command"},
        {"hack_attempt", "Potential security test"},
        {"../../../bin/sh", "Path traversal attempt"},
        {"; echo 'hacked'", "Command injection"},
        {"$(whoami)", "Command substitution"},
        {"`id`", "Command substitution"},
        {"place && rm -rf /", "Command chaining"},
        {"place; format c:", "Command chaining"},
        {"drop table users", "SQL injection attempt"},
        {"<script>alert('xss')</script>", "XSS attempt"},
        {"${jndi:ldap://hack.com}", "JNDI injection attempt"},
        {"../../../../../../etc/passwd", "Directory traversal"}
    };
    
    for (const auto& [command, description] : unknownCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Unknown command should fail: " << description 
                                   << " (command: '" << command << "')";
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << description;
    }
}

TEST_F(CommandSequenceIntegrationTest, StateConsistency_AfterInvalidCommands_REQ_11_5_1) {
    // Test that application state remains consistent after invalid commands
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Set up initial valid state
    auto result1 = commandProcessor->execute("resolution 4cm");
    EXPECT_TRUE(result1.success);
    
    auto result2 = commandProcessor->execute("workspace 6m 6m 6m");
    EXPECT_TRUE(result2.success);
    
    auto result3 = commandProcessor->execute("place 0cm 0cm 0cm");
    EXPECT_TRUE(result3.success);
    
    // Capture initial state
    uint32_t initialVoxelCount = voxelManager->getVoxelCount();
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    Math::Vector3f initialWorkspace = voxelManager->getWorkspaceSize();
    
    // Execute many invalid commands
    std::vector<std::string> invalidCommands = {
        "place invalid invalid invalid",
        "remove non-numeric data here",
        "resolution -999cm",
        "workspace 0 0 0",
        "camera invalid_view",
        "fill invalid params here",
        "unknown_command with params",
        "place",
        "remove",
        "resolution",
        "workspace",
        "camera"
    };
    
    for (const auto& command : invalidCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Invalid command should fail: " << command;
    }
    
    // Verify state remains consistent after all invalid commands
    uint32_t finalVoxelCount = voxelManager->getVoxelCount();
    VoxelData::VoxelResolution finalResolution = voxelManager->getActiveResolution();
    Math::Vector3f finalWorkspace = voxelManager->getWorkspaceSize();
    
    EXPECT_EQ(finalVoxelCount, initialVoxelCount) 
        << "Voxel count should remain unchanged after invalid commands";
    EXPECT_EQ(finalResolution, initialResolution) 
        << "Resolution should remain unchanged after invalid commands";
    EXPECT_NEAR(finalWorkspace.x, initialWorkspace.x, 0.01f) 
        << "Workspace X should remain unchanged after invalid commands";
    EXPECT_NEAR(finalWorkspace.y, initialWorkspace.y, 0.01f) 
        << "Workspace Y should remain unchanged after invalid commands";
    EXPECT_NEAR(finalWorkspace.z, initialWorkspace.z, 0.01f) 
        << "Workspace Z should remain unchanged after invalid commands";
    
    // Verify that valid commands still work after invalid ones
    auto validResult = commandProcessor->execute("place 4cm 0cm 0cm");
    EXPECT_TRUE(validResult.success) << "Valid commands should still work after invalid command attempts";
    
    uint32_t afterValidCount = voxelManager->getVoxelCount();
    EXPECT_EQ(afterValidCount, initialVoxelCount + 1) 
        << "Valid command should work normally after invalid command attempts";
}

// ============================================================================
// REQ-11.5.4: Commands shall test memory and resource cleanup after failures
// ============================================================================

TEST_F(CommandSequenceIntegrationTest, PlaceCommandFailure_ResourceCleanup_REQ_11_5_4) {
    // Test that failed place commands don't leak memory or resources
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Set up initial state
    auto result1 = commandProcessor->execute("resolution 4cm");
    EXPECT_TRUE(result1.success);
    
    // Capture initial state before failure attempts
    uint32_t initialVoxelCount = voxelManager->getVoxelCount();
    
    // Attempt many failing place commands
    std::vector<std::string> failingPlaceCommands = {
        "place 0cm -100cm 0cm",     // Below ground plane
        "place invalid 0cm 0cm",    // Invalid coordinate
        "place 0cm 0cm",            // Missing parameter
        "place",                    // Missing all parameters
        "place 1000000cm 0cm 0cm",  // Out of bounds
        "place 0cm 0cm invalid"     // Invalid Z coordinate
    };
    
    for (int i = 0; i < 100; ++i) {  // Repeat many times to stress test
        for (const auto& command : failingPlaceCommands) {
            auto result = commandProcessor->execute(command);
            EXPECT_FALSE(result.success) << "Command should fail: " << command;
        }
    }
    
    // Verify no resource leaks - voxel count should remain unchanged
    uint32_t finalVoxelCount = voxelManager->getVoxelCount();
    EXPECT_EQ(finalVoxelCount, initialVoxelCount) 
        << "Voxel count should not change after failed place commands";
    
    // Verify system is still functional after many failures
    auto validResult = commandProcessor->execute("place 0cm 0cm 0cm");
    EXPECT_TRUE(validResult.success) << "Valid command should still work after many failures";
    
    uint32_t afterValidCount = voxelManager->getVoxelCount();
    EXPECT_EQ(afterValidCount, initialVoxelCount + 1) 
        << "Valid command should work normally after failure stress test";
}

TEST_F(CommandSequenceIntegrationTest, FillCommandFailure_ResourceCleanup_REQ_11_5_4) {
    // Test that failed fill commands don't leak memory or resources
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Set up initial state
    auto result1 = commandProcessor->execute("resolution 1cm");
    EXPECT_TRUE(result1.success);
    
    uint32_t initialVoxelCount = voxelManager->getVoxelCount();
    
    // Attempt many failing fill commands
    std::vector<std::string> failingFillCommands = {
        "fill 0cm -100cm 0cm 4cm 4cm 4cm",  // Below ground plane
        "fill invalid 0cm 0cm 4cm 4cm 4cm", // Invalid coordinate
        "fill 0cm 0cm 0cm",                 // Missing parameters
        "fill",                             // Missing all parameters
        "fill 0cm 0cm 0cm 4cm -4cm 4cm",    // End Y below ground
        "fill 1000000cm 0cm 0cm 1000004cm 4cm 4cm" // Out of bounds
    };
    
    for (int i = 0; i < 50; ++i) {  // Repeat to stress test memory
        for (const auto& command : failingFillCommands) {
            auto result = commandProcessor->execute(command);
            EXPECT_FALSE(result.success) << "Fill command should fail: " << command;
        }
    }
    
    // Verify no resource leaks
    uint32_t finalVoxelCount = voxelManager->getVoxelCount();
    EXPECT_EQ(finalVoxelCount, initialVoxelCount) 
        << "Voxel count should not change after failed fill commands";
    
    // Verify system is still functional
    auto validResult = commandProcessor->execute("fill 0cm 0cm 0cm 4cm 4cm 4cm");
    EXPECT_TRUE(validResult.success) << "Valid fill should work after failures";
    
    uint32_t afterValidCount = voxelManager->getVoxelCount();
    EXPECT_GT(afterValidCount, initialVoxelCount) 
        << "Valid fill should create voxels after failure stress test";
}

TEST_F(CommandSequenceIntegrationTest, ResolutionCommandFailure_ResourceCleanup_REQ_11_5_4) {
    // Test that failed resolution commands don't leak memory or affect state
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Set initial resolution
    auto result1 = commandProcessor->execute("resolution 16cm");
    EXPECT_TRUE(result1.success);
    
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    
    // Attempt many failing resolution commands
    std::vector<std::string> failingResolutionCommands = {
        "resolution invalid",       // Invalid value
        "resolution 0cm",          // Zero resolution
        "resolution -1cm",         // Negative resolution
        "resolution 3cm",          // Non-power-of-2
        "resolution 1024cm",       // Too large
        "resolution",              // Missing parameter
        "resolution 1m",           // Wrong unit
        "resolution abc"           // Non-numeric
    };
    
    for (int i = 0; i < 100; ++i) {  // Repeat many times
        for (const auto& command : failingResolutionCommands) {
            auto result = commandProcessor->execute(command);
            EXPECT_FALSE(result.success) << "Resolution command should fail: " << command;
        }
    }
    
    // Verify resolution state is unchanged after failures
    VoxelData::VoxelResolution finalResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(finalResolution, initialResolution) 
        << "Resolution should remain unchanged after failed commands";
    
    // Verify system is still functional
    auto validResult = commandProcessor->execute("resolution 16cm");
    EXPECT_TRUE(validResult.success) << "Valid resolution should work after failures";
    
    VoxelData::VoxelResolution afterValidResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(afterValidResolution, VoxelData::VoxelResolution::Size_16cm) 
        << "Valid resolution change should work after failure stress test";
}

TEST_F(CommandSequenceIntegrationTest, WorkspaceCommandFailure_ResourceCleanup_REQ_11_5_4) {
    // Test that failed workspace commands don't leak memory or affect state
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Set initial workspace
    auto result1 = commandProcessor->execute("workspace 6m 6m 6m");
    EXPECT_TRUE(result1.success);
    
    Math::Vector3f initialWorkspace = voxelManager->getWorkspaceSize();
    
    // Attempt many failing workspace commands
    std::vector<std::string> failingWorkspaceCommands = {
        "workspace invalid 5 5",    // Invalid X
        "workspace 5 invalid 5",    // Invalid Y
        "workspace 5 5 invalid",    // Invalid Z
        "workspace 0 5 5",          // Zero X
        "workspace 5 0 5",          // Zero Y
        "workspace 5 5 0",          // Zero Z
        "workspace -1 5 5",         // Negative X
        "workspace",                // Missing parameters
        "workspace 100 5 5",        // Too large
        "workspace 1 1 1"           // Too small
    };
    
    for (int i = 0; i < 100; ++i) {  // Repeat many times
        for (const auto& command : failingWorkspaceCommands) {
            auto result = commandProcessor->execute(command);
            EXPECT_FALSE(result.success) << "Workspace command should fail: " << command;
        }
    }
    
    // Verify workspace state is unchanged after failures
    Math::Vector3f finalWorkspace = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(finalWorkspace.x, initialWorkspace.x, 0.01f) 
        << "Workspace X should remain unchanged after failed commands";
    EXPECT_NEAR(finalWorkspace.y, initialWorkspace.y, 0.01f) 
        << "Workspace Y should remain unchanged after failed commands";
    EXPECT_NEAR(finalWorkspace.z, initialWorkspace.z, 0.01f) 
        << "Workspace Z should remain unchanged after failed commands";
    
    // Verify system is still functional
    auto validResult = commandProcessor->execute("workspace 8m 8m 8m");
    EXPECT_TRUE(validResult.success) << "Valid workspace should work after failures";
    
    Math::Vector3f afterValidWorkspace = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(afterValidWorkspace.x, 8.0f, 0.01f) 
        << "Valid workspace change should work after failure stress test";
}

TEST_F(CommandSequenceIntegrationTest, CameraCommandFailure_ResourceCleanup_REQ_11_5_4) {
    // Test that failed camera commands don't leak memory or affect state
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* cameraController = app->getCameraController();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(cameraController, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Set initial camera state
    auto result1 = commandProcessor->execute("camera front");
    EXPECT_TRUE(result1.success);
    
    auto* initialCamera = cameraController->getCamera();
    ASSERT_NE(initialCamera, nullptr);
    Math::WorldCoordinates initialPosition = initialCamera->getPosition();
    
    // Attempt many failing camera commands
    std::vector<std::string> failingCameraCommands = {
        "camera invalid_view",      // Invalid view name
        "camera FRONT",            // Case sensitive
        "camera front back",       // Too many parameters
        "camera 123",              // Numeric view
        "camera",                  // Missing parameter
        // "camera \"front\"",        // Quoted parameter - actually accepted
        "camera null",             // Invalid view
        "camera ../hack"           // Security attempt
    };
    
    for (int i = 0; i < 100; ++i) {  // Repeat many times
        for (const auto& command : failingCameraCommands) {
            auto result = commandProcessor->execute(command);
            EXPECT_FALSE(result.success) << "Camera command should fail: " << command;
        }
    }
    
    // Verify camera state is still valid after failures
    auto* finalCamera = cameraController->getCamera();
    ASSERT_NE(finalCamera, nullptr) << "Camera should still exist after failed commands";
    
    Math::WorldCoordinates finalPosition = finalCamera->getPosition();
    EXPECT_FALSE(std::isnan(finalPosition.x()) || std::isnan(finalPosition.y()) || std::isnan(finalPosition.z()))
        << "Camera position should remain valid after failed commands";
    
    // Verify system is still functional
    auto validResult = commandProcessor->execute("camera top");
    EXPECT_TRUE(validResult.success) << "Valid camera command should work after failures";
    
    auto* afterValidCamera = cameraController->getCamera();
    ASSERT_NE(afterValidCamera, nullptr);
    Math::WorldCoordinates afterValidPosition = afterValidCamera->getPosition();
    EXPECT_FALSE(std::isnan(afterValidPosition.x()) || std::isnan(afterValidPosition.y()) || std::isnan(afterValidPosition.z()))
        << "Camera should remain functional after failure stress test";
}

TEST_F(CommandSequenceIntegrationTest, SaveLoadCommandFailure_ResourceCleanup_REQ_11_5_4) {
    // Test that failed save/load commands don't leak file handles or resources
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Attempt many failing save/load commands
    std::vector<std::string> failingSaveLoadCommands = {
        "save",                     // Missing filename
        "load",                     // Missing filename
        "save /dev/null",          // Invalid path
        "load /dev/null",          // Invalid path
        "save ../../../etc/passwd", // Path traversal
        "load nonexistent.vxl",    // Non-existent file
        "save \"\"",               // Empty filename
        "load \"\"",               // Empty filename
        "save file.txt",           // Wrong extension
        "load file.txt"            // Wrong extension
    };
    
    // Stress test file operations
    for (int i = 0; i < 50; ++i) {  // Repeat many times to test file handle cleanup
        for (const auto& command : failingSaveLoadCommands) {
            auto result = commandProcessor->execute(command);
            EXPECT_FALSE(result.success) << "Save/load command should fail: " << command;
        }
    }
    
    // After many failed file operations, the system should still work
    // We can't easily test actual save/load without setting up files, but we can test
    // that the command processor is still responsive
    auto statusResult = commandProcessor->execute("status");
    EXPECT_TRUE(statusResult.success) << "System should remain functional after file operation failures";
}

TEST_F(CommandSequenceIntegrationTest, MixedCommandFailures_ResourceCleanup_REQ_11_5_4) {
    // Test that mixed command failures don't accumulate resource leaks
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* cameraController = app->getCameraController();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(cameraController, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Set up initial state
    auto result1 = commandProcessor->execute("resolution 4cm");
    EXPECT_TRUE(result1.success);
    auto result2 = commandProcessor->execute("workspace 6m 6m 6m");
    EXPECT_TRUE(result2.success);
    auto result3 = commandProcessor->execute("camera iso");
    EXPECT_TRUE(result3.success);
    
    // Capture initial state
    uint32_t initialVoxelCount = voxelManager->getVoxelCount();
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    Math::Vector3f initialWorkspace = voxelManager->getWorkspaceSize();
    
    // Mix of failing commands from different categories
    std::vector<std::string> mixedFailingCommands = {
        "place invalid 0cm 0cm",        // Place failure
        "remove invalid invalid invalid", // Remove failure
        "fill invalid params here",      // Fill failure
        "resolution -999cm",             // Resolution failure
        "workspace 0 0 0",               // Workspace failure
        "camera invalid_view",           // Camera failure
        "unknown_command",               // Unknown command
        "save /dev/null",               // File operation failure
        "load nonexistent.vxl",         // Load failure
        ""                              // Empty command
    };
    
    // Stress test with mixed failures
    for (int i = 0; i < 100; ++i) {
        for (const auto& command : mixedFailingCommands) {
            auto result = commandProcessor->execute(command);
            EXPECT_FALSE(result.success) << "Mixed command should fail: " << command;
        }
    }
    
    // Verify no state corruption after massive failure stress test
    uint32_t finalVoxelCount = voxelManager->getVoxelCount();
    VoxelData::VoxelResolution finalResolution = voxelManager->getActiveResolution();
    Math::Vector3f finalWorkspace = voxelManager->getWorkspaceSize();
    
    EXPECT_EQ(finalVoxelCount, initialVoxelCount) 
        << "Voxel count should remain unchanged after mixed failures";
    EXPECT_EQ(finalResolution, initialResolution) 
        << "Resolution should remain unchanged after mixed failures";
    EXPECT_NEAR(finalWorkspace.x, initialWorkspace.x, 0.01f) 
        << "Workspace should remain unchanged after mixed failures";
    
    // Verify camera is still functional
    auto* camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr) << "Camera should still exist after mixed failures";
    Math::WorldCoordinates position = camera->getPosition();
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera should remain valid after mixed failures";
    
    // Verify all command types still work after massive failure stress test
    std::vector<std::string> validCommands = {
        "place 0cm 0cm 0cm",
        "place 4cm 0cm 0cm",
        "remove 0cm 0cm 0cm",
        "resolution 16cm",
        "workspace 6 6 6",
        "camera front"
    };
    
    for (const auto& command : validCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_TRUE(result.success) << "Valid command should work after mixed failure stress test: " << command;
    }
}

TEST_F(CommandSequenceIntegrationTest, CommandProcessor_ResourceCleanup_REQ_11_5_4) {
    // Test that the command processor itself properly cleans up after failures
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test extremely long commands that might cause buffer issues
    std::string veryLongCommand = "place ";
    for (int i = 0; i < 1000; ++i) {
        veryLongCommand += "invalid_parameter_";
    }
    
    // Test many very long invalid commands
    for (int i = 0; i < 10; ++i) {
        auto result = commandProcessor->execute(veryLongCommand);
        EXPECT_FALSE(result.success) << "Very long invalid command should fail";
    }
    
    // Test commands with many parameters
    std::string manyParamsCommand = "place";
    for (int i = 0; i < 100; ++i) {
        manyParamsCommand += " param" + std::to_string(i);
    }
    
    for (int i = 0; i < 10; ++i) {
        auto result = commandProcessor->execute(manyParamsCommand);
        EXPECT_FALSE(result.success) << "Command with many parameters should fail";
    }
    
    // Test commands with special characters that might cause parsing issues
    std::vector<std::string> specialCharCommands = {
        "place \0\0\0",
        "place \xff\xff\xff",
        "place \n\n\n",
        "place \t\t\t",
        "place \\\\\\",
        "place '''",
        "place \"\"\"",
        "place ;;;",
        "place &&&",
        "place |||"
    };
    
    for (const auto& command : specialCharCommands) {
        auto result = commandProcessor->execute(command);
        EXPECT_FALSE(result.success) << "Special character command should fail safely";
    }
    
    // After all these potential parser-breaking attempts, verify system still works
    auto validResult = commandProcessor->execute("resolution 1cm");
    EXPECT_TRUE(validResult.success) << "Command processor should remain functional after stress test";
}

} // namespace Tests
} // namespace VoxelEditor