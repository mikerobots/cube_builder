#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/CommandRegistry.h"
#include "cli/SelectCommands.h"
#include "cli/EditCommands.h"
#include "cli/FileCommands.h"
#include "cli/ViewCommands.h"
#include "cli/SystemCommands.h"
#include "cli/MeshCommands.h"
#include "voxel_data/VoxelDataManager.h"
#include "selection/SelectionManager.h"
#include "undo_redo/HistoryManager.h"

namespace VoxelEditor {

class SelectCommandsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Manually register command modules for testing
        CommandRegistry::getInstance().clear();
        CommandRegistry::getInstance().registerModule(std::make_unique<EditCommands>());
        CommandRegistry::getInstance().registerModule(std::make_unique<FileCommands>());
        CommandRegistry::getInstance().registerModule(std::make_unique<ViewCommands>());
        CommandRegistry::getInstance().registerModule(std::make_unique<SelectCommands>());
        CommandRegistry::getInstance().registerModule(std::make_unique<SystemCommands>());
        CommandRegistry::getInstance().registerModule(std::make_unique<MeshCommands>());
        
        app = std::make_unique<Application>();
        app->setHeadless(true);
        ASSERT_TRUE(app->initialize(0, nullptr));
        
        // Set resolution to 1cm for initial voxels
        app->getCommandProcessor()->execute("resolution 1cm");
        
        // Place some test voxels
        app->getCommandProcessor()->execute("place 0cm 0cm 0cm");
        app->getCommandProcessor()->execute("place 100cm 0cm 0cm");
        app->getCommandProcessor()->execute("place 0cm 100cm 0cm");
        app->getCommandProcessor()->execute("place 0cm 0cm 100cm");
        
        // Place some larger voxels
        app->getCommandProcessor()->execute("resolution 4cm");
        app->getCommandProcessor()->execute("place 200cm 0cm 0cm");
        app->getCommandProcessor()->execute("place 0cm 200cm 0cm");
        
    }
    
    void TearDown() override {
        app->shutdown();
    }
    
    std::unique_ptr<Application> app;
};

TEST_F(SelectCommandsTest, SelectSingleVoxel) {
    auto result = app->getCommandProcessor()->execute("select 0 0 0");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 1);
}

TEST_F(SelectCommandsTest, SelectBox) {
    auto result = app->getCommandProcessor()->execute("selectbox -100cm -100cm -100cm 100cm 100cm 100cm");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 4); // Should select the 4 voxels at 1cm resolution
}

TEST_F(SelectCommandsTest, SelectAll) {
    auto result = app->getCommandProcessor()->execute("selectall");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 6); // All 6 voxels
}

TEST_F(SelectCommandsTest, SelectNone) {
    // First select all
    app->getCommandProcessor()->execute("selectall");
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 6);
    
    // Then clear selection
    auto result = app->getCommandProcessor()->execute("selectnone");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 0);
}

TEST_F(SelectCommandsTest, SelectByResolution) {
    auto result = app->getCommandProcessor()->execute("select-resolution 1cm");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 4); // Only the 1cm voxels
}

TEST_F(SelectCommandsTest, InvertSelection) {
    // Select some voxels first
    app->getCommandProcessor()->execute("select-resolution 1cm");
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 4);
    
    // Invert selection
    auto result = app->getCommandProcessor()->execute("invert-selection");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 2); // Should now have the 2 4cm voxels
}

TEST_F(SelectCommandsTest, SelectionInfo) {
    app->getCommandProcessor()->execute("selectall");
    auto result = app->getCommandProcessor()->execute("selection-info");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("Total voxels: 6"), std::string::npos);
}

TEST_F(SelectCommandsTest, DeleteSelected) {
    // Select some voxels
    app->getCommandProcessor()->execute("select-resolution 1cm");
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 4);
    
    // Delete them
    auto result = app->getCommandProcessor()->execute("delete-selected");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 0); // Selection should be cleared
    EXPECT_EQ(app->getVoxelManager()->getVoxelCount(), 2); // Only 2 voxels left
}

TEST_F(SelectCommandsTest, GroupSelected) {
    // Select some voxels
    app->getCommandProcessor()->execute("select-resolution 4cm");
    EXPECT_EQ(app->getSelectionManager()->getSelectionSize(), 2);
    
    // Create group
    auto result = app->getCommandProcessor()->execute("group-selected TestGroup");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("Created group 'TestGroup' with 2 voxels"), std::string::npos);
}

TEST_F(SelectCommandsTest, SelectSphere) {
    auto result = app->getCommandProcessor()->execute("selectsphere 0cm 0cm 0cm 150cm");
    EXPECT_TRUE(result.success);
    // Should select voxels within 150cm radius of origin
    EXPECT_GT(app->getSelectionManager()->getSelectionSize(), 0);
}

} // namespace VoxelEditor