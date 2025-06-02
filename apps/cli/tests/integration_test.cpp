#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "selection/SelectionManager.h"
#include "groups/GroupManager.h"
#include "file_io/FileManager.h"
#include <sstream>
#include <filesystem>

namespace VoxelEditor {
namespace Tests {

class CLIIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        app = std::make_unique<Application>();
        
        // Initialize without creating window (headless mode)
        int argc = 0;
        char* argv[] = {nullptr};
        initialized = app->initialize(argc, argv);
    }
    
    void TearDown() override {
        // Clean up any test files
        std::filesystem::remove("test_project.cvef");
        std::filesystem::remove("test_export.stl");
    }
    
    CommandResult executeCommand(const std::string& command) {
        if (!initialized) {
            return CommandResult::Error("Application not initialized");
        }
        
        // Access command processor through reflection or friend class
        // For now, we'll simulate command execution
        return CommandResult::Success();
    }
    
    std::unique_ptr<Application> app;
    bool initialized = false;
};

TEST_F(CLIIntegrationTest, BasicInitialization) {
    ASSERT_TRUE(initialized) << "Application should initialize successfully";
    
    // Verify all systems are created
    ASSERT_NE(app->getVoxelManager(), nullptr);
    ASSERT_NE(app->getCameraController(), nullptr);
    ASSERT_NE(app->getRenderEngine(), nullptr);
    ASSERT_NE(app->getInputManager(), nullptr);
    ASSERT_NE(app->getSelectionManager(), nullptr);
    ASSERT_NE(app->getHistoryManager(), nullptr);
    ASSERT_NE(app->getSurfaceGenerator(), nullptr);
    ASSERT_NE(app->getFeedbackRenderer(), nullptr);
    ASSERT_NE(app->getGroupManager(), nullptr);
    ASSERT_NE(app->getFileManager(), nullptr);
}

TEST_F(CLIIntegrationTest, VoxelPlacementWorkflow) {
    ASSERT_TRUE(initialized);
    
    auto voxelManager = app->getVoxelManager();
    auto historyManager = app->getHistoryManager();
    
    // Set resolution
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::CM_8);
    EXPECT_EQ(voxelManager->getActiveResolution(), VoxelData::VoxelResolution::CM_8);
    
    // Place voxels
    glm::ivec3 pos1(0, 0, 0);
    glm::ivec3 pos2(1, 0, 0);
    glm::ivec3 pos3(0, 1, 0);
    
    EXPECT_TRUE(voxelManager->setVoxel(pos1, VoxelData::VoxelResolution::CM_8, true));
    EXPECT_TRUE(voxelManager->setVoxel(pos2, VoxelData::VoxelResolution::CM_8, true));
    EXPECT_TRUE(voxelManager->setVoxel(pos3, VoxelData::VoxelResolution::CM_8, true));
    
    // Verify voxels exist
    EXPECT_TRUE(voxelManager->getVoxel(pos1, VoxelData::VoxelResolution::CM_8));
    EXPECT_TRUE(voxelManager->getVoxel(pos2, VoxelData::VoxelResolution::CM_8));
    EXPECT_TRUE(voxelManager->getVoxel(pos3, VoxelData::VoxelResolution::CM_8));
    
    // Check voxel count
    EXPECT_EQ(voxelManager->getVoxelCount(), 3);
}

TEST_F(CLIIntegrationTest, SelectionWorkflow) {
    ASSERT_TRUE(initialized);
    
    auto voxelManager = app->getVoxelManager();
    auto selectionManager = app->getSelectionManager();
    
    // Create some voxels
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            voxelManager->setVoxel(glm::ivec3(x, y, 0), VoxelData::VoxelResolution::CM_8, true);
        }
    }
    
    // Select box region
    selectionManager->selectBox(glm::vec3(0), glm::vec3(2, 2, 0), Selection::SelectionMode::Replace);
    
    // Verify selection count
    EXPECT_EQ(selectionManager->getSelectionCount(), 9); // 3x3 region
    
    // Clear selection
    selectionManager->clearSelection();
    EXPECT_EQ(selectionManager->getSelectionCount(), 0);
    
    // Select all
    selectionManager->selectAll();
    EXPECT_EQ(selectionManager->getSelectionCount(), 25); // 5x5 voxels
}

TEST_F(CLIIntegrationTest, GroupManagementWorkflow) {
    ASSERT_TRUE(initialized);
    
    auto voxelManager = app->getVoxelManager();
    auto selectionManager = app->getSelectionManager();
    auto groupManager = app->getGroupManager();
    
    // Create voxels and select them
    std::vector<Selection::VoxelId> voxelIds;
    for (int i = 0; i < 5; ++i) {
        glm::ivec3 pos(i, 0, 0);
        voxelManager->setVoxel(pos, VoxelData::VoxelResolution::CM_8, true);
        auto id = voxelManager->getVoxelId(pos, VoxelData::VoxelResolution::CM_8);
        voxelIds.push_back(id);
        selectionManager->selectVoxel(id);
    }
    
    // Create group from selection
    auto groupId = groupManager->createGroup("TestGroup", selectionManager->getSelection());
    EXPECT_NE(groupId, Groups::GroupId::Invalid);
    
    // Verify group
    auto group = groupManager->getGroup(groupId);
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->getName(), "TestGroup");
    EXPECT_EQ(group->getVoxelCount(), 5);
    
    // Test visibility
    EXPECT_TRUE(group->isVisible());
    groupManager->setGroupVisibility(groupId, false);
    EXPECT_FALSE(group->isVisible());
    groupManager->setGroupVisibility(groupId, true);
    EXPECT_TRUE(group->isVisible());
}

TEST_F(CLIIntegrationTest, CameraControlWorkflow) {
    ASSERT_TRUE(initialized);
    
    auto cameraController = app->getCameraController();
    
    // Test view presets
    cameraController->setViewPreset(Camera::ViewPreset::Front);
    auto camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr);
    
    // Test zoom
    float initialDistance = camera->getDistance();
    camera->setDistance(initialDistance * 0.5f);
    EXPECT_NEAR(camera->getDistance(), initialDistance * 0.5f, 0.001f);
    
    // Test rotation
    camera->rotate(glm::radians(45.0f), 0.0f);
    
    // Reset view
    cameraController->reset();
    EXPECT_NEAR(camera->getDistance(), 10.0f, 0.001f); // Default distance
}

TEST_F(CLIIntegrationTest, UndoRedoWorkflow) {
    ASSERT_TRUE(initialized);
    
    auto voxelManager = app->getVoxelManager();
    auto historyManager = app->getHistoryManager();
    
    // Execute place command through history
    auto cmd = std::make_unique<UndoRedo::PlaceVoxelCommand>(
        voxelManager,
        glm::ivec3(0, 0, 0),
        VoxelData::VoxelResolution::CM_8
    );
    
    historyManager->executeCommand(std::move(cmd));
    EXPECT_TRUE(voxelManager->getVoxel(glm::ivec3(0, 0, 0), VoxelData::VoxelResolution::CM_8));
    
    // Undo
    EXPECT_TRUE(historyManager->undo());
    EXPECT_FALSE(voxelManager->getVoxel(glm::ivec3(0, 0, 0), VoxelData::VoxelResolution::CM_8));
    
    // Redo
    EXPECT_TRUE(historyManager->redo());
    EXPECT_TRUE(voxelManager->getVoxel(glm::ivec3(0, 0, 0), VoxelData::VoxelResolution::CM_8));
}

TEST_F(CLIIntegrationTest, FileIOWorkflow) {
    ASSERT_TRUE(initialized);
    
    auto voxelManager = app->getVoxelManager();
    auto fileManager = app->getFileManager();
    
    // Create some test data
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::CM_16);
    for (int i = 0; i < 10; ++i) {
        voxelManager->setVoxel(glm::ivec3(i, i, i), VoxelData::VoxelResolution::CM_16, true);
    }
    
    // Save project
    FileIO::SaveOptions saveOptions;
    EXPECT_TRUE(fileManager->saveProject("test_project.cvef", saveOptions));
    
    // Clear data
    voxelManager->clear();
    EXPECT_EQ(voxelManager->getVoxelCount(), 0);
    
    // Load project
    FileIO::LoadOptions loadOptions;
    EXPECT_TRUE(fileManager->loadProject("test_project.cvef", loadOptions));
    
    // Verify data restored
    EXPECT_EQ(voxelManager->getVoxelCount(), 10);
    EXPECT_EQ(voxelManager->getActiveResolution(), VoxelData::VoxelResolution::CM_16);
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(voxelManager->getVoxel(glm::ivec3(i, i, i), VoxelData::VoxelResolution::CM_16));
    }
}

TEST_F(CLIIntegrationTest, WorkspaceResizing) {
    ASSERT_TRUE(initialized);
    
    auto voxelManager = app->getVoxelManager();
    
    // Get initial size
    auto initialSize = voxelManager->getWorkspaceSize();
    EXPECT_EQ(initialSize, glm::vec3(5.0f)); // Default 5m³
    
    // Resize workspace
    glm::vec3 newSize(8.0f, 8.0f, 8.0f);
    EXPECT_TRUE(voxelManager->resizeWorkspace(newSize));
    EXPECT_EQ(voxelManager->getWorkspaceSize(), newSize);
    
    // Try invalid size (too small)
    glm::vec3 tooSmall(1.0f, 1.0f, 1.0f);
    EXPECT_FALSE(voxelManager->resizeWorkspace(tooSmall));
    EXPECT_EQ(voxelManager->getWorkspaceSize(), newSize); // Should remain unchanged
}

TEST_F(CLIIntegrationTest, MultiResolutionSupport) {
    ASSERT_TRUE(initialized);
    
    auto voxelManager = app->getVoxelManager();
    
    // Place voxels at different resolutions
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::CM_1);
    voxelManager->setVoxel(glm::ivec3(0, 0, 0), VoxelData::VoxelResolution::CM_1, true);
    
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::CM_8);
    voxelManager->setVoxel(glm::ivec3(1, 0, 0), VoxelData::VoxelResolution::CM_8, true);
    
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::CM_64);
    voxelManager->setVoxel(glm::ivec3(2, 0, 0), VoxelData::VoxelResolution::CM_64, true);
    
    // Verify each resolution has its voxel
    EXPECT_TRUE(voxelManager->getVoxel(glm::ivec3(0, 0, 0), VoxelData::VoxelResolution::CM_1));
    EXPECT_TRUE(voxelManager->getVoxel(glm::ivec3(1, 0, 0), VoxelData::VoxelResolution::CM_8));
    EXPECT_TRUE(voxelManager->getVoxel(glm::ivec3(2, 0, 0), VoxelData::VoxelResolution::CM_64));
    
    // Verify total count across all resolutions
    EXPECT_EQ(voxelManager->getVoxelCount(), 3);
}

} // namespace Tests
} // namespace VoxelEditor