#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/CommandTypes.h"
#include "surface_gen/MeshSmoother.h"
#include "voxel_data/VoxelDataManager.h"
#include "math/CoordinateTypes.h"
#include <sstream>

namespace VoxelEditor {

class SmoothingCommandTest : public ::testing::Test {
protected:
    void SetUp() override {
        app = std::make_unique<Application>();
        app->initialize(0, nullptr);
        processor = app->getCommandProcessor();
    }

    void TearDown() override {
        app.reset();
    }

    CommandResult executeCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        std::vector<std::string> args;
        
        iss >> cmd;
        std::string arg;
        while (iss >> arg) {
            args.push_back(arg);
        }
        
        CommandContext ctx(app.get(), cmd, args);
        auto cmdDef = processor->getCommand(cmd);
        if (cmdDef) {
            return cmdDef->handler(ctx);
        }
        return CommandResult::Error("Command not found");
    }

    std::unique_ptr<Application> app;
    CommandProcessor* processor;
};

TEST_F(SmoothingCommandTest, SmoothCommandNoArgs_ShowsCurrentSettings) {
    // Default settings
    auto result = executeCommand("smooth");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("Current smoothing settings:"), std::string::npos);
    EXPECT_NE(result.message.find("Level: 0"), std::string::npos);
    EXPECT_NE(result.message.find("Algorithm: None"), std::string::npos);
    EXPECT_NE(result.message.find("Preview: off"), std::string::npos);
}

TEST_F(SmoothingCommandTest, SmoothCommandSetLevel_Success) {
    // Set smoothing level
    auto result = executeCommand("smooth 5");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.message, "Smoothing level set to 5");
    EXPECT_EQ(app->getSmoothingLevel(), 5);
    
    // Verify algorithm was auto-selected (level 5 should use Taubin)
    EXPECT_EQ(app->getSmoothingAlgorithm(), SurfaceGen::MeshSmoother::Algorithm::Taubin);
}

TEST_F(SmoothingCommandTest, SmoothCommandSetMaxLevel_Success) {
    // Set maximum smoothing level
    auto result = executeCommand("smooth 15");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("maximum smoothing"), std::string::npos);
    EXPECT_EQ(app->getSmoothingLevel(), 15);
}

TEST_F(SmoothingCommandTest, SmoothCommandInvalidLevel_Error) {
    // Negative level
    auto result = executeCommand("smooth -1");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Invalid smoothing level"), std::string::npos);
    
    // Non-numeric level
    result = executeCommand("smooth abc");
    EXPECT_FALSE(result.success);
}

TEST_F(SmoothingCommandTest, SmoothCommandPreviewOn_Success) {
    auto result = executeCommand("smooth preview on");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.message, "Smoothing preview enabled");
    EXPECT_TRUE(app->isSmoothPreviewEnabled());
}

TEST_F(SmoothingCommandTest, SmoothCommandPreviewOff_Success) {
    // First enable preview
    app->setSmoothPreviewEnabled(true);
    
    auto result = executeCommand("smooth preview off");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.message, "Smoothing preview disabled");
    EXPECT_FALSE(app->isSmoothPreviewEnabled());
}

TEST_F(SmoothingCommandTest, SmoothCommandPreviewInvalid_Error) {
    auto result = executeCommand("smooth preview invalid");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Invalid option"), std::string::npos);
}

TEST_F(SmoothingCommandTest, SmoothCommandAlgorithmLaplacian_Success) {
    auto result = executeCommand("smooth algorithm laplacian");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.message, "Smoothing algorithm set to Laplacian");
    EXPECT_EQ(app->getSmoothingAlgorithm(), SurfaceGen::MeshSmoother::Algorithm::Laplacian);
}

TEST_F(SmoothingCommandTest, SmoothCommandAlgorithmTaubin_Success) {
    auto result = executeCommand("smooth algorithm taubin");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.message, "Smoothing algorithm set to Taubin");
    EXPECT_EQ(app->getSmoothingAlgorithm(), SurfaceGen::MeshSmoother::Algorithm::Taubin);
}

TEST_F(SmoothingCommandTest, SmoothCommandAlgorithmBiLaplacian_Success) {
    auto result = executeCommand("smooth algorithm bilaplacian");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.message, "Smoothing algorithm set to BiLaplacian");
    EXPECT_EQ(app->getSmoothingAlgorithm(), SurfaceGen::MeshSmoother::Algorithm::BiLaplacian);
}

TEST_F(SmoothingCommandTest, SmoothCommandAlgorithmInvalid_Error) {
    auto result = executeCommand("smooth algorithm invalid");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Invalid algorithm"), std::string::npos);
}

TEST_F(SmoothingCommandTest, SmoothCommandAlgorithmAutoSelection) {
    // Level 0 should keep None
    executeCommand("smooth 0");
    EXPECT_EQ(app->getSmoothingAlgorithm(), SurfaceGen::MeshSmoother::Algorithm::None);
    
    // Level 1-3 should select Laplacian
    executeCommand("smooth 2");
    EXPECT_EQ(app->getSmoothingAlgorithm(), SurfaceGen::MeshSmoother::Algorithm::Laplacian);
    
    // Level 4-7 should select Taubin
    executeCommand("smooth 6");
    EXPECT_EQ(app->getSmoothingAlgorithm(), SurfaceGen::MeshSmoother::Algorithm::Taubin);
    
    // Level 8+ should select BiLaplacian
    executeCommand("smooth 9");
    EXPECT_EQ(app->getSmoothingAlgorithm(), SurfaceGen::MeshSmoother::Algorithm::BiLaplacian);
}

TEST_F(SmoothingCommandTest, StatusCommandShowsSmoothingInfo) {
    // Set some smoothing settings
    app->setSmoothingLevel(5);
    app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::Taubin);
    app->setSmoothPreviewEnabled(true);
    
    auto result = executeCommand("status");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("Smoothing Settings:"), std::string::npos);
    EXPECT_NE(result.message.find("Level: 5"), std::string::npos);
    EXPECT_NE(result.message.find("Taubin"), std::string::npos);
    EXPECT_NE(result.message.find("Preview: on"), std::string::npos);
}

// Mesh command tests
class MeshCommandTest : public ::testing::Test {
protected:
    void SetUp() override {
        app = std::make_unique<Application>();
        app->initialize(0, nullptr);
        processor = app->getCommandProcessor();
        
        // Add a test voxel so mesh generation has something to work with
        auto voxelManager = app->getVoxelManager();
        voxelManager->setVoxel(Math::IncrementCoordinates(0, 0, 0), 
                              VoxelData::VoxelResolution::Size_1cm, true);
    }

    void TearDown() override {
        app.reset();
    }

    CommandResult executeCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        std::vector<std::string> args;
        
        iss >> cmd;
        std::string arg;
        while (iss >> arg) {
            args.push_back(arg);
        }
        
        CommandContext ctx(app.get(), cmd, args);
        auto cmdDef = processor->getCommand(cmd);
        if (cmdDef) {
            return cmdDef->handler(ctx);
        }
        return CommandResult::Error("Command not found");
    }

    std::unique_ptr<Application> app;
    CommandProcessor* processor;
};

TEST_F(MeshCommandTest, MeshCommandNoArgs_Error) {
    auto result = executeCommand("mesh");
    EXPECT_FALSE(result.success);
}

TEST_F(MeshCommandTest, MeshCommandInvalidSubcommand_Error) {
    auto result = executeCommand("mesh invalid");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Invalid subcommand"), std::string::npos);
}

TEST_F(MeshCommandTest, MeshValidateCommand_Success) {
    auto result = executeCommand("mesh validate");
    EXPECT_TRUE(result.success);
    // Should contain validation results
    EXPECT_NE(result.message.find("Mesh Validation Results:"), std::string::npos);
}

TEST_F(MeshCommandTest, MeshInfoCommand_Success) {
    auto result = executeCommand("mesh info");
    EXPECT_TRUE(result.success);
    // Should contain mesh information
    EXPECT_NE(result.message.find("Mesh Information:"), std::string::npos);
    EXPECT_NE(result.message.find("Vertices:"), std::string::npos);
    EXPECT_NE(result.message.find("Triangles:"), std::string::npos);
}

TEST_F(MeshCommandTest, MeshInfoWithSmoothing_ShowsSmoothingInfo) {
    // Set smoothing level
    app->setSmoothingLevel(5);
    app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::Taubin);
    
    auto result = executeCommand("mesh info");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("Smoothing applied:"), std::string::npos);
    EXPECT_NE(result.message.find("Level: 5"), std::string::npos);
    EXPECT_NE(result.message.find("Taubin"), std::string::npos);
}

TEST_F(MeshCommandTest, MeshRepairCommand_Placeholder) {
    auto result = executeCommand("mesh repair");
    EXPECT_TRUE(result.success);
    // Currently returns placeholder message
    EXPECT_NE(result.message.find("pending implementation"), std::string::npos);
}

} // namespace VoxelEditor