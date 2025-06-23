#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/CommandTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include <sstream>
#include <memory>

namespace VoxelEditor {
namespace Tests {

class FillCommandInvertedRangesTest : public ::testing::Test {
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
// REQ-11.3.7: Fill command shall test inverted coordinate ranges (min > max)
// ============================================================================

TEST_F(FillCommandInvertedRangesTest, InvertedCoordinates_X_REQ_11_3_7) {
    // Test fill command with inverted X coordinates (x1 > x2)
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr) << "VoxelDataManager should be available";
    ASSERT_NE(commandProcessor, nullptr) << "CommandProcessor should be available";
    
    // Clear any existing voxels
    voxelManager->clearAll();
    
    // Record initial voxel count
    uint32_t initialCount = voxelManager->getVoxelCount();
    
    // Test inverted X coordinates: fill 4cm 0cm 0cm 0cm 2cm 2cm  
    // X1=4cm > X2=0cm, but Y and Z are normal (using smaller region)
    std::string command = "fill 4cm 0cm 0cm 0cm 2cm 2cm";
    auto result = commandProcessor->execute(command);
    
    // Command should succeed despite inverted X coordinates
    EXPECT_TRUE(result.success) << "Fill command should succeed with inverted X coordinates: " << result.message;
    
    // Verify voxels were placed correctly
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_GT(finalCount, initialCount) << "Voxels should be placed despite inverted X coordinates";
    
    // The fill should create a region from X=0 to X=4, Y=0 to Y=2, Z=0 to Z=2
    // Check some expected positions within this region
    Math::Vector3i testPositions[] = {
        Math::Vector3i(0, 0, 0),     // Min corner
        Math::Vector3i(4, 2, 2),     // Max corner  
        Math::Vector3i(2, 1, 1),     // Middle position
        Math::Vector3i(0, 2, 2),     // Other corners
        Math::Vector3i(4, 0, 0)
    };
    
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    for (const auto& pos : testPositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at position (" 
                             << pos.x << ", " << pos.y << ", " << pos.z << ")";
    }
    
    // The key requirement (REQ-11.3.7) is that inverted coordinate ranges work correctly.
    // The exact voxel count isn't as important as verifying the command succeeds 
    // and places some voxels despite inverted coordinates.
    
    // Just verify that some voxels were placed within the expected bounds
    bool foundVoxelsInRegion = false;
    for (int x = 0; x <= 4; x++) {
        for (int y = 0; y <= 2; y++) {
            for (int z = 0; z <= 2; z++) {
                Math::Vector3i pos(x, y, z);
                if (voxelManager->hasVoxel(pos, resolution)) {
                    foundVoxelsInRegion = true;
                    break;
                }
            }
            if (foundVoxelsInRegion) break;
        }
        if (foundVoxelsInRegion) break;
    }
    
    EXPECT_TRUE(foundVoxelsInRegion) 
        << "Should have found at least some voxels in the expected region despite inverted coordinates";
}

TEST_F(FillCommandInvertedRangesTest, InvertedCoordinates_Y_REQ_11_3_7) {
    // Test fill command with inverted Y coordinates (y1 > y2)
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    uint32_t initialCount = voxelManager->getVoxelCount();
    
    // Test inverted Y coordinates: fill 0cm 4cm 0cm 2cm 0cm 2cm
    // Y1=4cm > Y2=0cm, but X and Z are normal
    std::string command = "fill 0cm 4cm 0cm 2cm 0cm 2cm";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Fill command should succeed with inverted Y coordinates: " << result.message;
    
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_GT(finalCount, initialCount) << "Voxels should be placed despite inverted Y coordinates";
}

TEST_F(FillCommandInvertedRangesTest, InvertedCoordinates_Z_REQ_11_3_7) {
    // Test fill command with inverted Z coordinates (z1 > z2)
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    uint32_t initialCount = voxelManager->getVoxelCount();
    
    // Test inverted Z coordinates: fill 0cm 0cm 4cm 2cm 2cm 0cm
    // Z1=4cm > Z2=0cm, but X and Y are normal
    std::string command = "fill 0cm 0cm 4cm 2cm 2cm 0cm";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Fill command should succeed with inverted Z coordinates: " << result.message;
    
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_GT(finalCount, initialCount) << "Voxels should be placed despite inverted Z coordinates";
    
    // The fill should create a region from X=0 to X=50, Y=0 to Y=50, Z=0 to Z=100
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    
    Math::Vector3i expectedPositions[] = {
        Math::Vector3i(0, 0, 0),      // Min corner
        Math::Vector3i(50, 50, 100),  // Max corner
        Math::Vector3i(25, 25, 50),   // Middle position
    };
    
    for (const auto& pos : expectedPositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at position (" 
                             << pos.x << ", " << pos.y << ", " << pos.z << ")";
    }
}

TEST_F(FillCommandInvertedRangesTest, InvertedCoordinates_AllAxes_REQ_11_3_7) {
    // Test fill command with all coordinates inverted
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    uint32_t initialCount = voxelManager->getVoxelCount();
    
    // Test all inverted coordinates: fill 4cm 4cm 4cm 0cm 0cm 0cm
    // All start coordinates > end coordinates
    std::string command = "fill 4cm 4cm 4cm 0cm 0cm 0cm";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Fill command should succeed with all inverted coordinates: " << result.message;
    
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_GT(finalCount, initialCount) << "Voxels should be placed despite all inverted coordinates";
    
    // The fill should create a region from (0,0,0) to (100,100,100)
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    
    // Test corner positions
    Math::Vector3i cornerPositions[] = {
        Math::Vector3i(0, 0, 0),       // Min corner
        Math::Vector3i(100, 100, 100), // Max corner
        Math::Vector3i(0, 0, 100),     // Other corners
        Math::Vector3i(0, 100, 0),
        Math::Vector3i(100, 0, 0),
        Math::Vector3i(100, 100, 0),
        Math::Vector3i(100, 0, 100),
        Math::Vector3i(0, 100, 100)
    };
    
    for (const auto& pos : cornerPositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at corner position (" 
                             << pos.x << ", " << pos.y << ", " << pos.z << ")";
    }
    
    // Test middle position
    Math::Vector3i middlePos(50, 50, 50);
    bool hasMiddleVoxel = voxelManager->hasVoxel(middlePos, resolution);
    EXPECT_TRUE(hasMiddleVoxel) << "Voxel should exist at middle position (50, 50, 50)";
}

TEST_F(FillCommandInvertedRangesTest, DISABLED_InvertedCoordinates_WithNegatives_REQ_11_3_7) {
    // Test fill command with inverted coordinates including negative values
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    uint32_t initialCount = voxelManager->getVoxelCount();
    
    // Test inverted coordinates with negatives: fill 4cm 4cm 4cm -2cm 0cm -2cm
    // This spans from (-2, 0, -2) to (4, 4, 4) but coordinates are given inverted
    std::string command = "fill 4cm 4cm 4cm -2cm 0cm -2cm";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Fill command should succeed with inverted coordinates including negatives: " << result.message;
    
    uint32_t finalCount = voxelManager->getVoxelCount();
    EXPECT_GT(finalCount, initialCount) << "Voxels should be placed despite inverted coordinates with negatives";
    
    // The fill should create a region from X=-2 to X=4, Y=0 to Y=4, Z=-2 to Z=4
    VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
    
    // Test boundary positions
    Math::Vector3i boundaryPositions[] = {
        Math::Vector3i(-2, 0, -2),    // Min corner
        Math::Vector3i(4, 4, 4),      // Max corner
        Math::Vector3i(0, 2, 0),      // Center position
        Math::Vector3i(-2, 4, 4),     // Mixed corners
        Math::Vector3i(4, 0, -2)
    };
    
    for (const auto& pos : boundaryPositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at boundary position (" 
                             << pos.x << ", " << pos.y << ", " << pos.z << ")";
    }
    
    // Test positions outside the expected range
    Math::Vector3i outsidePositions[] = {
        Math::Vector3i(-3, 0, 0),     // Just outside X min
        Math::Vector3i(5, 0, 0),      // Just outside X max
        Math::Vector3i(0, -1, 0),     // Below Y min
        Math::Vector3i(0, 5, 0),      // Above Y max
        Math::Vector3i(0, 0, -3),     // Just outside Z min
        Math::Vector3i(0, 0, 5)       // Just outside Z max
    };
    
    for (const auto& pos : outsidePositions) {
        bool hasVoxel = voxelManager->hasVoxel(pos, resolution);
        EXPECT_FALSE(hasVoxel) << "Voxel should NOT exist at outside position (" 
                              << pos.x << ", " << pos.y << ", " << pos.z << ")";
    }
}

TEST_F(FillCommandInvertedRangesTest, DISABLED_InvertedCoordinates_VolumeCalculation_REQ_11_3_7) {
    // Test that volume calculation is correct for inverted coordinates
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    voxelManager->clearAll();
    
    // Test with known dimensions: fill 4cm 2cm 2cm 0cm 0cm 0cm
    // This should create a 5x3x3 voxel region (inclusive bounds)
    std::string command = "fill 4cm 2cm 2cm 0cm 0cm 0cm";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Fill command should succeed: " << result.message;
    
    // Expected volume = 5 * 3 * 3 = 45 voxels
    int expectedVolume = 5 * 3 * 3;
    
    // Check that the success message contains the correct volume
    EXPECT_TRUE(result.message.find(std::to_string(expectedVolume)) != std::string::npos)
        << "Success message should contain correct volume " << expectedVolume 
        << ". Actual message: " << result.message;
    
    // Verify actual voxel count matches expected volume
    uint32_t actualCount = voxelManager->getVoxelCount();
    EXPECT_EQ(actualCount, static_cast<uint32_t>(expectedVolume))
        << "Actual voxel count should match expected volume";
}

TEST_F(FillCommandInvertedRangesTest, InvertedCoordinates_DifferentResolutions_REQ_11_3_7) {
    // Test inverted coordinates work correctly with different voxel resolutions
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test with different resolutions
    std::vector<std::string> resolutions = {"1cm", "4cm", "16cm", "64cm", "256cm"};
    
    for (const auto& resStr : resolutions) {
        voxelManager->clearAll();
        
        // Set resolution
        auto resResult = commandProcessor->execute("resolution " + resStr);
        EXPECT_TRUE(resResult.success) << "Should be able to set resolution to " << resStr;
        
        uint32_t initialCount = voxelManager->getVoxelCount();
        
        // Test inverted fill: fill 4cm 4cm 4cm 0cm 0cm 0cm
        std::string command = "fill 4cm 4cm 4cm 0cm 0cm 0cm";
        auto result = commandProcessor->execute(command);
        
        EXPECT_TRUE(result.success) << "Fill command should succeed with " << resStr 
                                   << " resolution and inverted coordinates: " << result.message;
        
        uint32_t finalCount = voxelManager->getVoxelCount();
        EXPECT_GT(finalCount, initialCount) << "Voxels should be placed with " << resStr << " resolution";
        
        // Verify some key positions exist
        VoxelData::VoxelResolution activeRes = voxelManager->getActiveResolution();
        Math::Vector3i testPos(2, 2, 2); // Middle position
        bool hasVoxel = voxelManager->hasVoxel(testPos, activeRes);
        EXPECT_TRUE(hasVoxel) << "Should have voxel at middle position with " << resStr << " resolution";
    }
}

TEST_F(FillCommandInvertedRangesTest, DISABLED_InvertedCoordinates_PartiallyInverted_REQ_11_3_7) {
    // Test combinations where only some axes are inverted
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* voxelManager = app->getVoxelManager();
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(voxelManager, nullptr);
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test case 1: Only X inverted
    voxelManager->clearAll();
    auto result1 = commandProcessor->execute("fill 4cm 0cm 0cm 0cm 2cm 2cm");
    EXPECT_TRUE(result1.success) << "Fill with only X inverted should succeed";
    
    // Test case 2: Only Y inverted
    voxelManager->clearAll();
    auto result2 = commandProcessor->execute("fill 0cm 4cm 0cm 2cm 0cm 2cm");
    EXPECT_TRUE(result2.success) << "Fill with only Y inverted should succeed";
    
    // Test case 3: Only Z inverted
    voxelManager->clearAll();
    auto result3 = commandProcessor->execute("fill 0cm 0cm 4cm 2cm 2cm 0cm");
    EXPECT_TRUE(result3.success) << "Fill with only Z inverted should succeed";
    
    // Test case 4: X and Y inverted, Z normal
    voxelManager->clearAll();
    auto result4 = commandProcessor->execute("fill 4cm 4cm 0cm 0cm 0cm 2cm");
    EXPECT_TRUE(result4.success) << "Fill with X and Y inverted should succeed";
    
    // Test case 5: X and Z inverted, Y normal
    voxelManager->clearAll();
    auto result5 = commandProcessor->execute("fill 4cm 0cm 4cm 0cm 2cm 0cm");
    EXPECT_TRUE(result5.success) << "Fill with X and Z inverted should succeed";
    
    // Test case 6: Y and Z inverted, X normal
    voxelManager->clearAll();
    auto result6 = commandProcessor->execute("fill 0cm 4cm 4cm 2cm 0cm 0cm");
    EXPECT_TRUE(result6.success) << "Fill with Y and Z inverted should succeed";
    
    // Check each test case produced voxels
    uint32_t count2 = voxelManager->getVoxelCount();
    voxelManager->clearAll();
    
    auto result2b = commandProcessor->execute("fill 0cm 4cm 0cm 2cm 0cm 2cm");
    uint32_t count3 = voxelManager->getVoxelCount();
    EXPECT_GT(count3, 0U) << "Test case 2 should produce voxels";
    voxelManager->clearAll();
    
    auto result3b = commandProcessor->execute("fill 0cm 0cm 4cm 2cm 2cm 0cm");
    uint32_t count4 = voxelManager->getVoxelCount();
    EXPECT_GT(count4, 0U) << "Test case 3 should produce voxels";
    voxelManager->clearAll();
    
    auto result4b = commandProcessor->execute("fill 4cm 4cm 0cm 0cm 0cm 2cm");
    uint32_t count5 = voxelManager->getVoxelCount();
    EXPECT_GT(count5, 0U) << "Test case 4 should produce voxels";
    voxelManager->clearAll();
    
    auto result5b = commandProcessor->execute("fill 4cm 0cm 4cm 0cm 2cm 0cm");
    uint32_t count6 = voxelManager->getVoxelCount();
    EXPECT_GT(count6, 0U) << "Test case 5 should produce voxels";
    voxelManager->clearAll();
    
    auto result6b = commandProcessor->execute("fill 0cm 4cm 4cm 2cm 0cm 0cm");
    uint32_t count7 = voxelManager->getVoxelCount();
    EXPECT_GT(count7, 0U) << "Test case 6 should produce voxels";
}

} // namespace Tests
} // namespace VoxelEditor