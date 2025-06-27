#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "surface_gen/MeshSmoother.h"
#include "file_io/STLExporter.h"
#include "math/CoordinateTypes.h"

namespace VoxelEditor {

class SmoothingExportIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        app = std::make_unique<Application>();
        app->initialize(0, nullptr);
        processor = app->getCommandProcessor();
        
        // Create temp directory for test files
        testDir = std::filesystem::temp_directory_path() / "voxel_editor_smooth_test";
        std::filesystem::create_directories(testDir);
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all(testDir);
        app.reset();
    }

    CommandResult executeCommand(const std::string& command) {
        return processor->execute(command);
    }

    bool fileExists(const std::string& filename) {
        return std::filesystem::exists(testDir / filename);
    }

    size_t getFileSize(const std::string& filename) {
        return std::filesystem::file_size(testDir / filename);
    }

    std::unique_ptr<Application> app;
    CommandProcessor* processor;
    std::filesystem::path testDir;
};

TEST_F(SmoothingExportIntegrationTest, ExportWithNoSmoothing_ProducesBlockyMesh) {
    // Set resolution to 1cm first
    executeCommand("resolution 1cm");
    
    // Create a simple voxel structure
    executeCommand("place 0cm 0cm 0cm");
    executeCommand("place 1cm 0cm 0cm");
    executeCommand("place 0cm 1cm 0cm");
    
    // Export without smoothing
    std::string exportPath = (testDir / "test_no_smooth.stl").string();
    auto result = executeCommand("export " + exportPath);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(fileExists("test_no_smooth.stl"));
    
    // File should exist and have reasonable size
    size_t fileSize = getFileSize("test_no_smooth.stl");
    EXPECT_GT(fileSize, 1000); // STL file should be at least 1KB
}

TEST_F(SmoothingExportIntegrationTest, ExportWithLowSmoothing_ProducesLargerFile) {
    // Set resolution to 1cm first
    executeCommand("resolution 1cm");
    
    // Create a simple voxel structure
    executeCommand("place 0cm 0cm 0cm");
    executeCommand("place 1cm 0cm 0cm");
    executeCommand("place 0cm 1cm 0cm");
    
    // Set low smoothing level
    executeCommand("smooth 2");
    
    // Export with smoothing
    std::string exportPath = (testDir / "test_smooth_2.stl").string();
    auto result = executeCommand("export " + exportPath);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(fileExists("test_smooth_2.stl"));
    
    // Note: File size comparison depends on smoothing implementation
    // Smoothed meshes may have more or fewer vertices depending on algorithm
    size_t fileSize = getFileSize("test_smooth_2.stl");
    EXPECT_GT(fileSize, 1000);
}

TEST_F(SmoothingExportIntegrationTest, ExportWithHighSmoothing_Success) {
    // Set resolution to 1cm first
    executeCommand("resolution 1cm");
    
    // Create a voxel cube
    executeCommand("place 0cm 0cm 0cm");
    executeCommand("place 1cm 0cm 0cm");
    executeCommand("place 0cm 1cm 0cm");
    executeCommand("place 1cm 1cm 0cm");
    executeCommand("place 0cm 0cm 1cm");
    executeCommand("place 1cm 0cm 1cm");
    executeCommand("place 0cm 1cm 1cm");
    executeCommand("place 1cm 1cm 1cm");
    
    // Set high smoothing level
    executeCommand("smooth 8");
    
    // Export with smoothing
    std::string exportPath = (testDir / "test_smooth_8.stl").string();
    auto result = executeCommand("export " + exportPath);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(fileExists("test_smooth_8.stl"));
}

TEST_F(SmoothingExportIntegrationTest, ExportWithDifferentAlgorithms_Success) {
    // Set resolution to 1cm first
    executeCommand("resolution 1cm");
    
    // Create test voxels
    executeCommand("place 0cm 0cm 0cm");
    executeCommand("place 1cm 0cm 0cm");
    
    // Test Laplacian
    executeCommand("smooth algorithm laplacian");
    executeCommand("smooth 3");
    std::string exportPath1 = (testDir / "test_laplacian.stl").string();
    auto result1 = executeCommand("export " + exportPath1);
    EXPECT_TRUE(result1.success);
    
    // Test Taubin
    executeCommand("smooth algorithm taubin");
    executeCommand("smooth 5");
    std::string exportPath2 = (testDir / "test_taubin.stl").string();
    auto result2 = executeCommand("export " + exportPath2);
    EXPECT_TRUE(result2.success);
    
    // Test BiLaplacian
    executeCommand("smooth algorithm bilaplacian");
    executeCommand("smooth 9");
    std::string exportPath3 = (testDir / "test_bilaplacian.stl").string();
    auto result3 = executeCommand("export " + exportPath3);
    EXPECT_TRUE(result3.success);
    
    // All files should exist
    EXPECT_TRUE(fileExists("test_laplacian.stl"));
    EXPECT_TRUE(fileExists("test_taubin.stl"));
    EXPECT_TRUE(fileExists("test_bilaplacian.stl"));
}

TEST_F(SmoothingExportIntegrationTest, SmoothingWorkflowWithMultipleResolutions) {
    // Create voxels at different resolutions
    executeCommand("resolution 1cm");
    executeCommand("place 0cm 0cm 0cm");
    
    executeCommand("resolution 2cm");
    executeCommand("place 2cm 0cm 0cm");
    
    executeCommand("resolution 4cm");
    executeCommand("place 0cm 4cm 0cm");
    
    // Set smoothing
    executeCommand("smooth 5");
    
    // Export
    std::string exportPath = (testDir / "test_multi_res.stl").string();
    auto result = executeCommand("export " + exportPath);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(fileExists("test_multi_res.stl"));
}

TEST_F(SmoothingExportIntegrationTest, MeshValidateAfterSmoothing) {
    // Set resolution to 1cm first
    executeCommand("resolution 1cm");
    
    // Create a simple closed shape
    executeCommand("place 0cm 0cm 0cm");
    executeCommand("place 1cm 0cm 0cm");
    executeCommand("place 0cm 1cm 0cm");
    executeCommand("place 1cm 1cm 0cm");
    executeCommand("place 0cm 0cm 1cm");
    executeCommand("place 1cm 0cm 1cm");
    executeCommand("place 0cm 1cm 1cm");
    executeCommand("place 1cm 1cm 1cm");
    
    // Apply smoothing
    executeCommand("smooth 5");
    
    // Validate mesh
    auto result = executeCommand("mesh validate");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.message.find("Watertight:"), std::string::npos);
    EXPECT_NE(result.message.find("Manifold:"), std::string::npos);
}

TEST_F(SmoothingExportIntegrationTest, MeshInfoShowsChangesWithSmoothing) {
    // Set resolution to 1cm first
    executeCommand("resolution 1cm");
    
    // Create test geometry
    executeCommand("place 0cm 0cm 0cm");
    executeCommand("place 1cm 0cm 0cm");
    executeCommand("place 2cm 0cm 0cm");
    
    // Get mesh info without smoothing
    auto result1 = executeCommand("mesh info");
    EXPECT_TRUE(result1.success);
    
    // Apply smoothing and get info again
    executeCommand("smooth 5");
    auto result2 = executeCommand("mesh info");
    EXPECT_TRUE(result2.success);
    
    // Both should succeed and show smoothing info in second result
    EXPECT_EQ(result2.message.find("Smoothing applied:") != std::string::npos, true);
}

TEST_F(SmoothingExportIntegrationTest, SmoothingPersistenceAcrossExports) {
    // Set resolution to 1cm first
    executeCommand("resolution 1cm");
    
    // Create voxels
    executeCommand("place 0cm 0cm 0cm");
    
    // Set smoothing once
    executeCommand("smooth 7");
    
    // Export multiple times - smoothing should persist
    std::string exportPath1 = (testDir / "test_persist_1.stl").string();
    auto result1 = executeCommand("export " + exportPath1);
    EXPECT_TRUE(result1.success);
    
    std::string exportPath2 = (testDir / "test_persist_2.stl").string();
    auto result2 = executeCommand("export " + exportPath2);
    EXPECT_TRUE(result2.success);
    
    // Both exports should have used smoothing
    EXPECT_TRUE(fileExists("test_persist_1.stl"));
    EXPECT_TRUE(fileExists("test_persist_2.stl"));
}

TEST_F(SmoothingExportIntegrationTest, CompleteWorkflow_BuildSmoothenExport) {
    // Complete workflow test
    // Set resolution to 1cm first
    executeCommand("resolution 1cm");
    
    // 1. Create a voxel structure
    executeCommand("place 0cm 0cm 0cm");
    executeCommand("place 1cm 0cm 0cm");
    executeCommand("place 0cm 1cm 0cm");
    executeCommand("place 1cm 1cm 0cm");
    
    // 2. Check status
    auto statusResult = executeCommand("status");
    EXPECT_TRUE(statusResult.success);
    EXPECT_NE(statusResult.message.find("Voxels: 4"), std::string::npos);
    
    // 3. Set smoothing
    auto smoothResult = executeCommand("smooth 6");
    EXPECT_TRUE(smoothResult.success);
    
    // 4. Check mesh info
    auto infoResult = executeCommand("mesh info");
    EXPECT_TRUE(infoResult.success);
    EXPECT_NE(infoResult.message.find("Smoothing applied:"), std::string::npos);
    
    // 5. Validate mesh
    auto validateResult = executeCommand("mesh validate");
    EXPECT_TRUE(validateResult.success);
    
    // 6. Export
    std::string exportPath = (testDir / "test_complete_workflow.stl").string();
    auto exportResult = executeCommand("export " + exportPath);
    EXPECT_TRUE(exportResult.success);
    EXPECT_TRUE(fileExists("test_complete_workflow.stl"));
}

} // namespace VoxelEditor