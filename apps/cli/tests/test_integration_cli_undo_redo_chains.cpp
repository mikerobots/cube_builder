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

class UndoRedoChainsIntegrationTest : public ::testing::Test {
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
    
    // Helper to capture system state for comparison
    struct SystemState {
        uint32_t voxelCount;
        VoxelData::VoxelResolution activeResolution;
        Math::Vector3f workspaceSize;
        Math::WorldCoordinates cameraPosition;
        Math::WorldCoordinates cameraTarget;
        
        SystemState(UndoRedoChainsIntegrationTest* test) {
            auto* voxelManager = test->app->getVoxelManager();
            auto* cameraController = test->app->getCameraController();
            
            voxelCount = voxelManager->getTotalVoxelCount();
            activeResolution = voxelManager->getActiveResolution();
            workspaceSize = voxelManager->getWorkspaceSize();
            
            auto* camera = cameraController->getCamera();
            cameraPosition = camera->getPosition();
            cameraTarget = camera->getTarget();
        }
        
        bool operator==(const SystemState& other) const {
            const float epsilon = 0.01f;
            return voxelCount == other.voxelCount &&
                   activeResolution == other.activeResolution &&
                   std::abs(workspaceSize.x - other.workspaceSize.x) < epsilon &&
                   std::abs(workspaceSize.y - other.workspaceSize.y) < epsilon &&
                   std::abs(workspaceSize.z - other.workspaceSize.z) < epsilon &&
                   std::abs(cameraPosition.x() - other.cameraPosition.x()) < epsilon &&
                   std::abs(cameraPosition.y() - other.cameraPosition.y()) < epsilon &&
                   std::abs(cameraPosition.z() - other.cameraPosition.z()) < epsilon &&
                   std::abs(cameraTarget.x() - other.cameraTarget.x()) < epsilon &&
                   std::abs(cameraTarget.y() - other.cameraTarget.y()) < epsilon &&
                   std::abs(cameraTarget.z() - other.cameraTarget.z()) < epsilon;
        }
    };
    
    // Helper to execute a command and verify success
    bool executeCommand(const std::string& command) {
        auto* commandProcessor = app->getCommandProcessor();
        auto result = commandProcessor->execute(command);
        return result.success;
    }
};

// ============================================================================
// REQ-11.4.3: Undo/redo chains shall be tested for all commands
// ============================================================================

TEST_F(UndoRedoChainsIntegrationTest, PlaceCommand_UndoRedoChain_REQ_11_4_3) {
    // Test undo/redo chain for place commands
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Set resolution for consistent testing
    EXPECT_TRUE(executeCommand("resolution 1cm"));
    
    // Capture initial state
    SystemState initialState(this);
    
    // Execute place command chain
    EXPECT_TRUE(executeCommand("place 0cm 0cm 0cm"));
    SystemState stateAfterPlace1(this);
    EXPECT_EQ(stateAfterPlace1.voxelCount, initialState.voxelCount + 1);
    
    EXPECT_TRUE(executeCommand("place 4cm 0cm 0cm"));
    SystemState stateAfterPlace2(this);
    EXPECT_EQ(stateAfterPlace2.voxelCount, initialState.voxelCount + 2);
    
    EXPECT_TRUE(executeCommand("place 8cm 0cm 0cm"));
    SystemState stateAfterPlace3(this);
    EXPECT_EQ(stateAfterPlace3.voxelCount, initialState.voxelCount + 3);
    
    // Test undo chain - should reverse in LIFO order
    EXPECT_TRUE(executeCommand("undo"));  // Undo place 8cm
    SystemState stateAfterUndo1(this);
    EXPECT_TRUE(stateAfterUndo1 == stateAfterPlace2) << "State after first undo should match state after second place";
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo place 4cm
    SystemState stateAfterUndo2(this);
    EXPECT_TRUE(stateAfterUndo2 == stateAfterPlace1) << "State after second undo should match state after first place";
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo place 0cm
    SystemState stateAfterUndo3(this);
    EXPECT_TRUE(stateAfterUndo3 == initialState) << "State after third undo should match initial state";
    
    // Test redo chain - should restore in FIFO order
    EXPECT_TRUE(executeCommand("redo"));  // Redo place 0cm
    SystemState stateAfterRedo1(this);
    EXPECT_TRUE(stateAfterRedo1 == stateAfterPlace1) << "State after first redo should match state after first place";
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo place 4cm
    SystemState stateAfterRedo2(this);
    EXPECT_TRUE(stateAfterRedo2 == stateAfterPlace2) << "State after second redo should match state after second place";
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo place 8cm
    SystemState stateAfterRedo3(this);
    EXPECT_TRUE(stateAfterRedo3 == stateAfterPlace3) << "State after third redo should match state after third place";
    
    // Verify final voxel positions
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), resolution));
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 0), resolution));
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 0), resolution));
}

TEST_F(UndoRedoChainsIntegrationTest, RemoveCommand_UndoRedoChain_REQ_11_4_3) {
    // Test undo/redo chain for remove commands
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Set resolution and place initial voxels
    EXPECT_TRUE(executeCommand("resolution 1cm"));
    EXPECT_TRUE(executeCommand("place 0cm 0cm 0cm"));
    EXPECT_TRUE(executeCommand("place 4cm 0cm 0cm"));
    EXPECT_TRUE(executeCommand("place 8cm 0cm 0cm"));
    
    SystemState stateAfterSetup(this);
    EXPECT_EQ(stateAfterSetup.voxelCount, 3U);
    
    // Execute remove command chain
    EXPECT_TRUE(executeCommand("remove 0cm 0cm 0cm"));
    SystemState stateAfterRemove1(this);
    EXPECT_EQ(stateAfterRemove1.voxelCount, 2U);
    
    EXPECT_TRUE(executeCommand("remove 4cm 0cm 0cm"));
    SystemState stateAfterRemove2(this);
    EXPECT_EQ(stateAfterRemove2.voxelCount, 1U);
    
    // Test undo chain for remove commands
    EXPECT_TRUE(executeCommand("undo"));  // Undo remove 4cm
    SystemState stateAfterUndo1(this);
    EXPECT_EQ(stateAfterUndo1.voxelCount, 2U);
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 0), resolution)) << "Voxel should be restored after undo";
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo remove 0cm
    SystemState stateAfterUndo2(this);
    EXPECT_EQ(stateAfterUndo2.voxelCount, 3U);
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), resolution)) << "Voxel should be restored after undo";
    
    // Test redo chain for remove commands
    EXPECT_TRUE(executeCommand("redo"));  // Redo remove 0cm
    SystemState stateAfterRedo1(this);
    EXPECT_EQ(stateAfterRedo1.voxelCount, 2U);
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), resolution)) << "Voxel should be removed after redo";
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo remove 4cm
    SystemState stateAfterRedo2(this);
    EXPECT_EQ(stateAfterRedo2.voxelCount, 1U);
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 0), resolution)) << "Voxel should be removed after redo";
    
    // Only 8cm voxel should remain
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 0), resolution));
}

TEST_F(UndoRedoChainsIntegrationTest, DISABLED_FillCommand_UndoRedoChain_REQ_11_4_3) {
    // Test undo/redo chain for fill commands
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Set resolution
    EXPECT_TRUE(executeCommand("resolution 1cm"));
    
    SystemState initialState(this);
    
    // Execute fill command chain with minimal fills (single voxels)
    EXPECT_TRUE(executeCommand("fill 0cm 0cm 0cm 1cm 1cm 1cm"));
    SystemState stateAfterFill1(this);
    uint32_t countAfterFill1 = stateAfterFill1.voxelCount;
    EXPECT_GT(countAfterFill1, initialState.voxelCount);
    
    EXPECT_TRUE(executeCommand("fill 8cm 0cm 0cm 9cm 1cm 1cm"));
    SystemState stateAfterFill2(this);
    uint32_t countAfterFill2 = stateAfterFill2.voxelCount;
    EXPECT_GT(countAfterFill2, countAfterFill1);
    
    // Test undo chain for fill commands
    EXPECT_TRUE(executeCommand("undo"));  // Undo second fill
    SystemState stateAfterUndo1(this);
    EXPECT_EQ(stateAfterUndo1.voxelCount, countAfterFill1) << "Voxel count should match after undoing second fill";
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo first fill
    SystemState stateAfterUndo2(this);
    EXPECT_TRUE(stateAfterUndo2 == initialState) << "State should match initial after undoing all fills";
    
    // Test redo chain for fill commands
    EXPECT_TRUE(executeCommand("redo"));  // Redo first fill
    SystemState stateAfterRedo1(this);
    EXPECT_EQ(stateAfterRedo1.voxelCount, countAfterFill1) << "Voxel count should match after redoing first fill";
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo second fill
    SystemState stateAfterRedo2(this);
    EXPECT_EQ(stateAfterRedo2.voxelCount, countAfterFill2) << "Voxel count should match after redoing second fill";
    
    // Verify both fill regions exist
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), resolution)) << "First fill region should exist";
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 0), resolution)) << "Second fill region should exist";
}

TEST_F(UndoRedoChainsIntegrationTest, DISABLED_ResolutionCommand_UndoRedoChain_REQ_11_4_3) {
    // Test undo/redo chain for resolution commands
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Get initial resolution
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    
    // Execute resolution command chain
    EXPECT_TRUE(executeCommand("resolution 4cm"));
    EXPECT_EQ(voxelManager->getActiveResolution(), VoxelData::VoxelResolution::Size_4cm);
    
    EXPECT_TRUE(executeCommand("resolution 16cm"));
    EXPECT_EQ(voxelManager->getActiveResolution(), VoxelData::VoxelResolution::Size_16cm);
    
    EXPECT_TRUE(executeCommand("resolution 64cm"));
    EXPECT_EQ(voxelManager->getActiveResolution(), VoxelData::VoxelResolution::Size_64cm);
    
    // Test undo chain for resolution commands
    EXPECT_TRUE(executeCommand("undo"));  // Undo resolution 64cm
    EXPECT_EQ(voxelManager->getActiveResolution(), VoxelData::VoxelResolution::Size_16cm);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo resolution 16cm
    EXPECT_EQ(voxelManager->getActiveResolution(), VoxelData::VoxelResolution::Size_4cm);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo resolution 4cm
    EXPECT_EQ(voxelManager->getActiveResolution(), initialResolution);
    
    // Test redo chain for resolution commands
    EXPECT_TRUE(executeCommand("redo"));  // Redo resolution 4cm
    EXPECT_EQ(voxelManager->getActiveResolution(), VoxelData::VoxelResolution::Size_4cm);
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo resolution 16cm
    EXPECT_EQ(voxelManager->getActiveResolution(), VoxelData::VoxelResolution::Size_16cm);
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo resolution 64cm
    EXPECT_EQ(voxelManager->getActiveResolution(), VoxelData::VoxelResolution::Size_64cm);
}

TEST_F(UndoRedoChainsIntegrationTest, DISABLED_WorkspaceCommand_UndoRedoChain_REQ_11_4_3) {
    // Test undo/redo chain for workspace commands
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Get initial workspace size
    Math::Vector3f initialWorkspace = voxelManager->getWorkspaceSize();
    
    // Execute workspace command chain
    EXPECT_TRUE(executeCommand("workspace 6m 6m 6m"));
    Math::Vector3f workspace1 = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(workspace1.x, 6.0f, 0.01f);
    
    EXPECT_TRUE(executeCommand("workspace 4m 4m 4m"));
    Math::Vector3f workspace2 = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(workspace2.x, 4.0f, 0.01f);
    
    EXPECT_TRUE(executeCommand("workspace 8m 8m 8m"));
    Math::Vector3f workspace3 = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(workspace3.x, 8.0f, 0.01f);
    
    // Test undo chain for workspace commands
    EXPECT_TRUE(executeCommand("undo"));  // Undo workspace 8m
    Math::Vector3f undoWorkspace1 = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(undoWorkspace1.x, 4.0f, 0.01f);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo workspace 4m
    Math::Vector3f undoWorkspace2 = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(undoWorkspace2.x, 6.0f, 0.01f);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo workspace 6m
    Math::Vector3f undoWorkspace3 = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(undoWorkspace3.x, initialWorkspace.x, 0.01f);
    
    // Test redo chain for workspace commands
    EXPECT_TRUE(executeCommand("redo"));  // Redo workspace 6m
    Math::Vector3f redoWorkspace1 = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(redoWorkspace1.x, 6.0f, 0.01f);
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo workspace 4m
    Math::Vector3f redoWorkspace2 = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(redoWorkspace2.x, 4.0f, 0.01f);
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo workspace 8m
    Math::Vector3f redoWorkspace3 = voxelManager->getWorkspaceSize();
    EXPECT_NEAR(redoWorkspace3.x, 8.0f, 0.01f);
}

TEST_F(UndoRedoChainsIntegrationTest, DISABLED_CameraCommand_UndoRedoChain_REQ_11_4_3) {
    // Test undo/redo chain for camera commands
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* cameraController = app->getCameraController();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(cameraController, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Set initial camera position
    EXPECT_TRUE(executeCommand("camera iso"));
    auto* camera = cameraController->getCamera();
    Math::WorldCoordinates initialPosition = camera->getPosition();
    Math::WorldCoordinates initialTarget = camera->getTarget();
    
    // Execute camera command chain
    EXPECT_TRUE(executeCommand("camera front"));
    Math::WorldCoordinates position1 = camera->getPosition();
    Math::WorldCoordinates target1 = camera->getTarget();
    
    EXPECT_TRUE(executeCommand("camera top"));
    Math::WorldCoordinates position2 = camera->getPosition();
    Math::WorldCoordinates target2 = camera->getTarget();
    
    EXPECT_TRUE(executeCommand("camera right"));
    Math::WorldCoordinates position3 = camera->getPosition();
    Math::WorldCoordinates target3 = camera->getTarget();
    
    // Test undo chain for camera commands
    EXPECT_TRUE(executeCommand("undo"));  // Undo camera right
    Math::WorldCoordinates undoPosition1 = camera->getPosition();
    Math::WorldCoordinates undoTarget1 = camera->getTarget();
    EXPECT_NEAR(undoPosition1.x(), position2.x(), 0.01f) << "Camera position should match after undo";
    EXPECT_NEAR(undoPosition1.y(), position2.y(), 0.01f);
    EXPECT_NEAR(undoPosition1.z(), position2.z(), 0.01f);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo camera top
    Math::WorldCoordinates undoPosition2 = camera->getPosition();
    EXPECT_NEAR(undoPosition2.x(), position1.x(), 0.01f) << "Camera position should match after second undo";
    EXPECT_NEAR(undoPosition2.y(), position1.y(), 0.01f);
    EXPECT_NEAR(undoPosition2.z(), position1.z(), 0.01f);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo camera front
    Math::WorldCoordinates undoPosition3 = camera->getPosition();
    EXPECT_NEAR(undoPosition3.x(), initialPosition.x(), 0.01f) << "Camera position should match initial after third undo";
    EXPECT_NEAR(undoPosition3.y(), initialPosition.y(), 0.01f);
    EXPECT_NEAR(undoPosition3.z(), initialPosition.z(), 0.01f);
    
    // Test redo chain for camera commands
    EXPECT_TRUE(executeCommand("redo"));  // Redo camera front
    Math::WorldCoordinates redoPosition1 = camera->getPosition();
    EXPECT_NEAR(redoPosition1.x(), position1.x(), 0.01f) << "Camera position should match after redo";
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo camera top
    Math::WorldCoordinates redoPosition2 = camera->getPosition();
    EXPECT_NEAR(redoPosition2.x(), position2.x(), 0.01f) << "Camera position should match after second redo";
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo camera right
    Math::WorldCoordinates redoPosition3 = camera->getPosition();
    EXPECT_NEAR(redoPosition3.x(), position3.x(), 0.01f) << "Camera position should match after third redo";
}

TEST_F(UndoRedoChainsIntegrationTest, DISABLED_MixedCommand_UndoRedoChain_REQ_11_4_3) {
    // Test undo/redo chain for mixed command types
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* cameraController = app->getCameraController();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(cameraController, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Capture initial state
    SystemState initialState(this);
    
    // Execute mixed command chain
    EXPECT_TRUE(executeCommand("resolution 1cm"));
    SystemState stateAfterRes1(this);
    
    EXPECT_TRUE(executeCommand("place 0cm 0cm 0cm"));
    SystemState stateAfterPlace1(this);
    
    EXPECT_TRUE(executeCommand("workspace 6m 6m 6m"));
    SystemState stateAfterWork1(this);
    
    EXPECT_TRUE(executeCommand("camera front"));
    SystemState stateAfterCam1(this);
    
    EXPECT_TRUE(executeCommand("fill 4cm 0cm 0cm 8cm 4cm 4cm"));
    SystemState stateAfterFill1(this);
    
    EXPECT_TRUE(executeCommand("resolution 4cm"));
    SystemState stateAfterRes2(this);
    
    // Test undo chain for mixed commands (LIFO order)
    EXPECT_TRUE(executeCommand("undo"));  // Undo resolution 4cm
    SystemState undoState1(this);
    EXPECT_EQ(undoState1.activeResolution, stateAfterFill1.activeResolution);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo fill
    SystemState undoState2(this);
    EXPECT_EQ(undoState2.voxelCount, stateAfterCam1.voxelCount);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo camera front
    SystemState undoState3(this);
    // Camera state should revert, but other state should match
    EXPECT_EQ(undoState3.voxelCount, stateAfterWork1.voxelCount);
    EXPECT_EQ(undoState3.activeResolution, stateAfterWork1.activeResolution);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo workspace 6m
    SystemState undoState4(this);
    EXPECT_NEAR(undoState4.workspaceSize.x, stateAfterPlace1.workspaceSize.x, 0.01f);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo place 0cm
    SystemState undoState5(this);
    EXPECT_EQ(undoState5.voxelCount, stateAfterRes1.voxelCount);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo resolution 1cm
    SystemState undoState6(this);
    EXPECT_EQ(undoState6.activeResolution, initialState.activeResolution);
    
    // Test redo chain for mixed commands (FIFO order)
    EXPECT_TRUE(executeCommand("redo"));  // Redo resolution 1cm
    SystemState redoState1(this);
    EXPECT_EQ(redoState1.activeResolution, stateAfterRes1.activeResolution);
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo place 0cm
    SystemState redoState2(this);
    EXPECT_EQ(redoState2.voxelCount, stateAfterPlace1.voxelCount);
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo workspace 6m
    SystemState redoState3(this);
    EXPECT_NEAR(redoState3.workspaceSize.x, 6.0f, 0.01f);
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo camera front
    SystemState redoState4(this);
    EXPECT_EQ(redoState4.voxelCount, stateAfterCam1.voxelCount);
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo fill
    SystemState redoState5(this);
    EXPECT_GT(redoState5.voxelCount, redoState4.voxelCount);
    
    EXPECT_TRUE(executeCommand("redo"));  // Redo resolution 4cm
    SystemState redoState6(this);
    EXPECT_EQ(redoState6.activeResolution, VoxelData::VoxelResolution::Size_4cm);
    
    // Verify final state matches expected state after all operations
    EXPECT_EQ(redoState6.activeResolution, VoxelData::VoxelResolution::Size_4cm);
    EXPECT_GT(redoState6.voxelCount, initialState.voxelCount);
    EXPECT_NEAR(redoState6.workspaceSize.x, 6.0f, 0.01f);
}

TEST_F(UndoRedoChainsIntegrationTest, DISABLED_UndoRedoBounds_Testing_REQ_11_4_3) {
    // Test undo/redo bounds and edge cases
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Test undo with no history
    auto undoResult1 = commandProcessor->execute("undo");
    EXPECT_FALSE(undoResult1.success) << "Undo should fail when no history exists";
    EXPECT_FALSE(undoResult1.message.empty()) << "Error message should explain no history";
    
    // Test redo with no history
    auto redoResult1 = commandProcessor->execute("redo");
    EXPECT_FALSE(redoResult1.success) << "Redo should fail when no history exists";
    EXPECT_FALSE(redoResult1.message.empty()) << "Error message should explain no history";
    
    // Create some history
    EXPECT_TRUE(executeCommand("resolution 1cm"));
    EXPECT_TRUE(executeCommand("place 0cm 0cm 0cm"));
    EXPECT_TRUE(executeCommand("place 4cm 0cm 0cm"));
    
    // Test multiple undos beyond history
    EXPECT_TRUE(executeCommand("undo"));  // Valid undo
    EXPECT_TRUE(executeCommand("undo"));  // Valid undo
    EXPECT_TRUE(executeCommand("undo"));  // Valid undo
    
    auto undoResult2 = commandProcessor->execute("undo");
    EXPECT_FALSE(undoResult2.success) << "Undo should fail when history is exhausted";
    
    // Test redo chain
    EXPECT_TRUE(executeCommand("redo"));  // Valid redo
    EXPECT_TRUE(executeCommand("redo"));  // Valid redo
    EXPECT_TRUE(executeCommand("redo"));  // Valid redo
    
    auto redoResult2 = commandProcessor->execute("redo");
    EXPECT_FALSE(redoResult2.success) << "Redo should fail when redo history is exhausted";
    
    // Test that new command clears redo history
    EXPECT_TRUE(executeCommand("undo"));  // Go back one step
    EXPECT_TRUE(executeCommand("place 8cm 0cm 0cm"));  // New command should clear redo history
    
    auto redoResult3 = commandProcessor->execute("redo");
    EXPECT_FALSE(redoResult3.success) << "Redo should fail after new command clears redo history";
    
    // Verify that undo still works
    EXPECT_TRUE(executeCommand("undo")) << "Undo should work for the new command";
}

TEST_F(UndoRedoChainsIntegrationTest, DISABLED_ComplexUndoRedoChain_StateIntegrity_REQ_11_4_3) {
    // Test complex undo/redo chain with state integrity verification
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* cameraController = app->getCameraController();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(cameraController, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Create complex command sequence
    std::vector<std::string> commands = {
        "resolution 1cm",
        "workspace 6m 6m 6m",
        "place 0cm 0cm 0cm",
        "camera front",
        "place 4cm 0cm 0cm",
        "resolution 4cm",
        "fill 8cm 0cm 0cm 16cm 4cm 4cm",
        "camera top",
        "remove 4cm 0cm 0cm",
        "workspace 8m 8m 8m"
    };
    
    // Store state after each command
    std::vector<SystemState> states;
    states.emplace_back(this);  // Initial state
    
    for (const auto& command : commands) {
        EXPECT_TRUE(executeCommand(command)) << "Command should succeed: " << command;
        states.emplace_back(this);
    }
    
    // Undo entire chain and verify state at each step
    for (int i = commands.size() - 1; i >= 0; --i) {
        EXPECT_TRUE(executeCommand("undo")) << "Undo should succeed for command: " << commands[i];
        SystemState currentState(this);
        
        // Verify state matches the state before the undone command
        EXPECT_TRUE(currentState == states[i]) 
            << "State after undoing '" << commands[i] << "' should match state before it was executed";
    }
    
    // Redo entire chain and verify state at each step
    for (size_t i = 0; i < commands.size(); ++i) {
        EXPECT_TRUE(executeCommand("redo")) << "Redo should succeed for command: " << commands[i];
        SystemState currentState(this);
        
        // Verify state matches the state after the redone command
        EXPECT_TRUE(currentState == states[i + 1])
            << "State after redoing '" << commands[i] << "' should match original execution state";
    }
    
    // Verify final state integrity
    SystemState finalState(this);
    EXPECT_TRUE(finalState == states.back()) << "Final state should match original final state";
    
    // Verify specific end state properties
    EXPECT_EQ(finalState.activeResolution, VoxelData::VoxelResolution::Size_4cm);
    EXPECT_NEAR(finalState.workspaceSize.x, 8.0f, 0.01f);
    EXPECT_GT(finalState.voxelCount, 0U) << "Should have voxels after complete sequence";
    
    // Verify specific voxel positions
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm))
        << "Original placed voxel should exist";
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 0), VoxelData::VoxelResolution::Size_1cm))
        << "Removed voxel should not exist";
    
    // Check fill region exists at correct resolution
    bool foundFillVoxel = false;
    for (int x = 8; x <= 16; x += 4) {
        for (int y = 0; y <= 4; y += 4) {
            for (int z = 0; z <= 4; z += 4) {
                if (voxelManager->hasVoxel(Math::Vector3i(x, y, z), VoxelData::VoxelResolution::Size_4cm)) {
                    foundFillVoxel = true;
                    break;
                }
            }
            if (foundFillVoxel) break;
        }
        if (foundFillVoxel) break;
    }
    EXPECT_TRUE(foundFillVoxel) << "Fill region should exist at 4cm resolution";
}

TEST_F(UndoRedoChainsIntegrationTest, PartialUndoRedo_BranchingHistory_REQ_11_4_3) {
    // Test branching undo/redo history when new commands are executed mid-chain
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Create initial command sequence
    EXPECT_TRUE(executeCommand("resolution 1cm"));
    EXPECT_TRUE(executeCommand("place 0cm 0cm 0cm"));
    EXPECT_TRUE(executeCommand("place 4cm 0cm 0cm"));
    EXPECT_TRUE(executeCommand("place 8cm 0cm 0cm"));
    
    SystemState stateAfterInitial(this);
    EXPECT_EQ(stateAfterInitial.voxelCount, 3U);
    
    // Undo partially
    EXPECT_TRUE(executeCommand("undo"));  // Undo place 8cm
    EXPECT_TRUE(executeCommand("undo"));  // Undo place 4cm
    
    SystemState stateAfterPartialUndo(this);
    EXPECT_EQ(stateAfterPartialUndo.voxelCount, 1U);
    
    // Execute new command - this should clear the redo history
    EXPECT_TRUE(executeCommand("place 12cm 0cm 0cm"));
    
    SystemState stateAfterBranch(this);
    EXPECT_EQ(stateAfterBranch.voxelCount, 2U);
    
    // Test that redo fails (history was cleared by new command)
    auto redoResult = commandProcessor->execute("redo");
    EXPECT_FALSE(redoResult.success) << "Redo should fail after branching history with new command";
    
    // Test that undo still works for the new path
    EXPECT_TRUE(executeCommand("undo"));  // Undo place 12cm
    SystemState stateAfterBranchUndo(this);
    EXPECT_EQ(stateAfterBranchUndo.voxelCount, 1U);
    
    EXPECT_TRUE(executeCommand("undo"));  // Undo place 0cm
    SystemState stateAfterFullUndo(this);
    EXPECT_EQ(stateAfterFullUndo.voxelCount, 0U);
    
    // Verify specific voxel states
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), resolution));
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 0), resolution));
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 0), resolution));
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(12, 0, 0), resolution));
    
    // Test redo for the new branch
    EXPECT_TRUE(executeCommand("redo"));  // Redo place 0cm
    EXPECT_TRUE(executeCommand("redo"));  // Redo place 12cm
    
    SystemState finalBranchState(this);
    EXPECT_EQ(finalBranchState.voxelCount, 2U);
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(0, 0, 0), resolution));
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(12, 0, 0), resolution));
    
    // Original voxels from the abandoned branch should not exist
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(4, 0, 0), resolution));
    EXPECT_FALSE(voxelManager->hasVoxel(Math::Vector3i(8, 0, 0), resolution));
}

} // namespace Tests
} // namespace VoxelEditor