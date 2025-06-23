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

class ResolutionCommandTest : public ::testing::Test {
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
        
        // Create a voxel data manager for direct testing
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
        Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f); // 5x5x5 meter workspace
        voxelManager->resizeWorkspace(workspaceSize);
    }
    
    void TearDown() override {
        // Clean up
    }
    
    // Helper function to verify resolution change
    void verifyResolutionChange(VoxelData::VoxelResolution expectedResolution) {
        VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
        EXPECT_EQ(currentResolution, expectedResolution) 
            << "Active resolution should be " << VoxelData::getVoxelSizeName(expectedResolution);
    }
    
    std::unique_ptr<Application> app;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
    bool initialized = false;
};

// ============================================================================
// REQ-11.3.12: Resolution command shall test all valid resolution values
// ============================================================================

TEST_F(ResolutionCommandTest, AllValidResolutionValues_1cm_REQ_11_3_12) {
    // Test resolution command with 1cm value
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Record state before execution
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    
    // Set resolution to 1cm
    VoxelData::VoxelResolution targetResolution = VoxelData::VoxelResolution::Size_1cm;
    voxelManager->setActiveResolution(targetResolution);
    
    // Verify state change after execution
    verifyResolutionChange(targetResolution);
    
    // Verify the resolution is different from initial if it wasn't already 1cm
    if (initialResolution != targetResolution) {
        EXPECT_NE(voxelManager->getActiveResolution(), initialResolution)
            << "Resolution should have changed from initial state";
    }
}

// Size_4cm test removed - not in 5-resolution system

TEST_F(ResolutionCommandTest, AllValidResolutionValues_4cm_REQ_11_3_12) {
    // Test resolution command with 4cm value
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    VoxelData::VoxelResolution targetResolution = VoxelData::VoxelResolution::Size_4cm;
    
    voxelManager->setActiveResolution(targetResolution);
    
    verifyResolutionChange(targetResolution);
    
    if (initialResolution != targetResolution) {
        EXPECT_NE(voxelManager->getActiveResolution(), initialResolution);
    }
}

// Size_16cm test removed - not in 5-resolution system

TEST_F(ResolutionCommandTest, AllValidResolutionValues_16cm_REQ_11_3_12) {
    // Test resolution command with 16cm value
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    VoxelData::VoxelResolution targetResolution = VoxelData::VoxelResolution::Size_16cm;
    
    voxelManager->setActiveResolution(targetResolution);
    
    verifyResolutionChange(targetResolution);
    
    if (initialResolution != targetResolution) {
        EXPECT_NE(voxelManager->getActiveResolution(), initialResolution);
    }
}

// Size_64cm test removed - not in 5-resolution system

TEST_F(ResolutionCommandTest, AllValidResolutionValues_64cm_REQ_11_3_12) {
    // Test resolution command with 64cm value
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    VoxelData::VoxelResolution targetResolution = VoxelData::VoxelResolution::Size_64cm;
    
    voxelManager->setActiveResolution(targetResolution);
    
    verifyResolutionChange(targetResolution);
    
    if (initialResolution != targetResolution) {
        EXPECT_NE(voxelManager->getActiveResolution(), initialResolution);
    }
}

// Size_256cm test removed - not in 5-resolution system

TEST_F(ResolutionCommandTest, AllValidResolutionValues_256cm_REQ_11_3_12) {
    // Test resolution command with 256cm value
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    VoxelData::VoxelResolution targetResolution = VoxelData::VoxelResolution::Size_256cm;
    
    voxelManager->setActiveResolution(targetResolution);
    
    verifyResolutionChange(targetResolution);
    
    if (initialResolution != targetResolution) {
        EXPECT_NE(voxelManager->getActiveResolution(), initialResolution);
    }
}

// Size_256cm test removed - not in 5-resolution system

// ============================================================================
// Comprehensive Resolution Testing - All Values in One Test
// ============================================================================

TEST_F(ResolutionCommandTest, AllValidResolutionValues_Comprehensive_REQ_11_3_12) {
    // Test all valid resolution values in sequence
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Define all valid resolutions as per requirement (5-resolution system)
    std::vector<VoxelData::VoxelResolution> validResolutions = {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_64cm,
        VoxelData::VoxelResolution::Size_256cm
    };
    
    // Verify we have exactly 5 resolutions as per VoxelResolution::COUNT
    EXPECT_EQ(validResolutions.size(), static_cast<size_t>(VoxelData::VoxelResolution::COUNT))
        << "Should test all " << static_cast<int>(VoxelData::VoxelResolution::COUNT) << " resolution values";
    
    for (const auto& resolution : validResolutions) {
        // Record state before execution
        VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
        
        // Execute resolution change
        voxelManager->setActiveResolution(resolution);
        
        // Verify state change after execution
        verifyResolutionChange(resolution);
        
        // Verify the resolution actually changed to the target value
        EXPECT_EQ(voxelManager->getActiveResolution(), resolution)
            << "Active resolution should be exactly " << VoxelData::getVoxelSizeName(resolution);
    }
}

// ============================================================================
// Resolution Parameter Validation Tests
// ============================================================================

TEST_F(ResolutionCommandTest, ParameterValidation_ValidResolutionStrings_REQ_11_3_12) {
    // Test that all valid resolution string values can be converted to enum values
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    struct ResolutionMapping {
        std::string stringValue;
        VoxelData::VoxelResolution enumValue;
    };
    
    std::vector<ResolutionMapping> resolutionMappings = {
        {"1cm", VoxelData::VoxelResolution::Size_1cm},
        {"4cm", VoxelData::VoxelResolution::Size_4cm},
        {"16cm", VoxelData::VoxelResolution::Size_16cm},
        {"64cm", VoxelData::VoxelResolution::Size_64cm},
        {"256cm", VoxelData::VoxelResolution::Size_256cm}
    };
    
    for (const auto& mapping : resolutionMappings) {
        // Test that the enum value can be set
        voxelManager->setActiveResolution(mapping.enumValue);
        
        // Verify the resolution was set correctly
        VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
        EXPECT_EQ(currentResolution, mapping.enumValue)
            << "Resolution should be " << mapping.stringValue;
        
        // Verify the string name matches
        const char* sizeName = VoxelData::getVoxelSizeName(currentResolution);
        EXPECT_STREQ(sizeName, mapping.stringValue.c_str())
            << "getVoxelSizeName should return " << mapping.stringValue;
    }
}

// ============================================================================
// State Consistency Tests
// ============================================================================

TEST_F(ResolutionCommandTest, StateConsistency_ResolutionPersistence_REQ_11_3_12) {
    // Test that resolution changes persist and affect subsequent operations
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Set to a specific resolution
    VoxelData::VoxelResolution testResolution = VoxelData::VoxelResolution::Size_16cm;
    voxelManager->setActiveResolution(testResolution);
    
    // Verify resolution persists
    VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(currentResolution, testResolution) << "Resolution should persist after setting";
    
    // Place a voxel and verify it uses the correct resolution
    Math::Vector3i position(0, 0, 0);
    bool placed = voxelManager->setVoxel(position, testResolution, true);
    EXPECT_TRUE(placed) << "Should be able to place voxel with current resolution";
    
    // Verify the voxel exists at the correct resolution
    bool hasVoxel = voxelManager->hasVoxel(position, testResolution);
    EXPECT_TRUE(hasVoxel) << "Voxel should exist at the specified resolution";
    
    // Verify the voxel doesn't exist at other resolutions
    bool hasVoxelDifferentRes = voxelManager->hasVoxel(position, VoxelData::VoxelResolution::Size_4cm);
    EXPECT_FALSE(hasVoxelDifferentRes) << "Voxel should not exist at different resolution";
}

TEST_F(ResolutionCommandTest, StateConsistency_ResolutionSequence_REQ_11_3_12) {
    // Test multiple resolution changes in sequence maintain consistency
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    std::vector<VoxelData::VoxelResolution> resolutionSequence = {
        VoxelData::VoxelResolution::Size_64cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_256cm,
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_16cm
    };
    
    for (size_t i = 0; i < resolutionSequence.size(); ++i) {
        VoxelData::VoxelResolution targetResolution = resolutionSequence[i];
        
        // Record initial state
        VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
        
        // Execute resolution change
        voxelManager->setActiveResolution(targetResolution);
        
        // Verify state change
        VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
        EXPECT_EQ(currentResolution, targetResolution)
            << "Resolution " << i << " should be " << VoxelData::getVoxelSizeName(targetResolution);
        
        // Verify the change actually occurred (unless it was already the target)
        if (initialResolution != targetResolution) {
            EXPECT_NE(currentResolution, initialResolution)
                << "Resolution should have changed from initial state in step " << i;
        }
    }
}

// ============================================================================
// Resolution Validation Edge Cases
// ============================================================================

TEST_F(ResolutionCommandTest, ResolutionValidation_EnumBounds_REQ_11_3_12) {
    // Test that only valid enum values are accepted
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Test that all valid enum values work
    for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
        VoxelData::VoxelResolution resolution = static_cast<VoxelData::VoxelResolution>(i);
        
        voxelManager->setActiveResolution(resolution);
        
        // Verify it was set correctly
        VoxelData::VoxelResolution current = voxelManager->getActiveResolution();
        EXPECT_EQ(current, resolution) << "Resolution should be set to enum value " << i;
    }
}

TEST_F(ResolutionCommandTest, ResolutionValidation_VoxelSizeCalculation_REQ_11_3_12) {
    // Test that voxel sizes are calculated correctly for all resolutions
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    struct ResolutionSizeMapping {
        VoxelData::VoxelResolution resolution;
        float expectedSize; // in meters
    };
    
    std::vector<ResolutionSizeMapping> sizeMappings = {
        {VoxelData::VoxelResolution::Size_1cm, 0.01f},
        {VoxelData::VoxelResolution::Size_4cm, 0.04f},
        {VoxelData::VoxelResolution::Size_16cm, 0.16f},
        {VoxelData::VoxelResolution::Size_64cm, 0.64f},
        {VoxelData::VoxelResolution::Size_256cm, 2.56f}
    };
    
    for (const auto& mapping : sizeMappings) {
        // Set the resolution
        voxelManager->setActiveResolution(mapping.resolution);
        
        // Get the voxel size
        float actualSize = VoxelData::getVoxelSize(mapping.resolution);
        EXPECT_FLOAT_EQ(actualSize, mapping.expectedSize)
            << "Voxel size for " << VoxelData::getVoxelSizeName(mapping.resolution)
            << " should be " << mapping.expectedSize << " meters";
    }
}

// ============================================================================
// REQ-11.3.14: Resolution command shall test resolution switching with existing voxels
// ============================================================================

TEST_F(ResolutionCommandTest, ResolutionSwitchingWithExistingVoxels_BasicSwitch_REQ_11_3_14) {
    // Test switching resolution when there are existing voxels in the workspace
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Set initial resolution and place a voxel
    VoxelData::VoxelResolution initialResolution = VoxelData::VoxelResolution::Size_4cm;
    voxelManager->setActiveResolution(initialResolution);
    
    Math::Vector3i voxelPosition(0, 0, 0);
    bool placed = voxelManager->setVoxel(voxelPosition, initialResolution, true);
    EXPECT_TRUE(placed) << "Should be able to place voxel at initial resolution";
    
    // Verify voxel exists at initial resolution
    bool hasVoxelInitial = voxelManager->hasVoxel(voxelPosition, initialResolution);
    EXPECT_TRUE(hasVoxelInitial) << "Voxel should exist at initial resolution";
    
    // Record state before resolution switch
    VoxelData::VoxelResolution currentRes = voxelManager->getActiveResolution();
    EXPECT_EQ(currentRes, initialResolution) << "Current resolution should match initial";
    
    // Switch to different resolution
    VoxelData::VoxelResolution newResolution = VoxelData::VoxelResolution::Size_16cm;
    voxelManager->setActiveResolution(newResolution);
    
    // Verify state after resolution switch
    VoxelData::VoxelResolution activeResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(activeResolution, newResolution) << "Active resolution should be updated";
    
    // Verify existing voxel still exists at original resolution
    bool hasVoxelAfterSwitch = voxelManager->hasVoxel(voxelPosition, initialResolution);
    EXPECT_TRUE(hasVoxelAfterSwitch) << "Existing voxel should persist after resolution switch";
    
    // Verify existing voxel doesn't appear at new resolution
    bool hasVoxelNewRes = voxelManager->hasVoxel(voxelPosition, newResolution);
    EXPECT_FALSE(hasVoxelNewRes) << "Existing voxel should not appear at new resolution";
}

TEST_F(ResolutionCommandTest, ResolutionSwitchingWithExistingVoxels_MultipleVoxels_REQ_11_3_14) {
    // Test resolution switching with multiple voxels at different resolutions
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Place voxels at different resolutions (spaced far apart to avoid collision)
    std::vector<std::pair<Math::Vector3i, VoxelData::VoxelResolution>> voxelsToPlace = {
        {Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm},
        {Math::Vector3i(100, 0, 0), VoxelData::VoxelResolution::Size_4cm},
        {Math::Vector3i(0, 100, 0), VoxelData::VoxelResolution::Size_16cm},
        {Math::Vector3i(-100, 0, 0), VoxelData::VoxelResolution::Size_64cm}
    };
    
    // Place all voxels
    for (const auto& voxelInfo : voxelsToPlace) {
        voxelManager->setActiveResolution(voxelInfo.second);
        bool placed = voxelManager->setVoxel(voxelInfo.first, voxelInfo.second, true);
        EXPECT_TRUE(placed) << "Should be able to place voxel at " 
                          << VoxelData::getVoxelSizeName(voxelInfo.second);
    }
    
    // Verify all voxels are placed correctly
    for (const auto& voxelInfo : voxelsToPlace) {
        bool hasVoxel = voxelManager->hasVoxel(voxelInfo.first, voxelInfo.second);
        EXPECT_TRUE(hasVoxel) << "Voxel should exist at " 
                            << VoxelData::getVoxelSizeName(voxelInfo.second);
    }
    
    // Switch to a different resolution
    VoxelData::VoxelResolution newActiveResolution = VoxelData::VoxelResolution::Size_64cm;
    voxelManager->setActiveResolution(newActiveResolution);
    
    // Verify active resolution changed
    EXPECT_EQ(voxelManager->getActiveResolution(), newActiveResolution)
        << "Active resolution should be updated to 64cm";
    
    // Verify all existing voxels still exist at their original resolutions
    for (const auto& voxelInfo : voxelsToPlace) {
        bool hasVoxel = voxelManager->hasVoxel(voxelInfo.first, voxelInfo.second);
        EXPECT_TRUE(hasVoxel) << "Existing voxel at " 
                            << VoxelData::getVoxelSizeName(voxelInfo.second)
                            << " should persist after resolution switch";
    }
    
    // Verify existing voxels don't appear at the new active resolution
    // (except for voxels that were already placed at that resolution)
    for (const auto& voxelInfo : voxelsToPlace) {
        if (voxelInfo.second != newActiveResolution) {
            bool hasVoxelNewRes = voxelManager->hasVoxel(voxelInfo.first, newActiveResolution);
            EXPECT_FALSE(hasVoxelNewRes) << "Existing voxel placed at " 
                                        << VoxelData::getVoxelSizeName(voxelInfo.second)
                                        << " should not appear at new active resolution "
                                        << VoxelData::getVoxelSizeName(newActiveResolution);
        }
    }
}

TEST_F(ResolutionCommandTest, ResolutionSwitchingWithExistingVoxels_NewPlacementUsesActiveResolution_REQ_11_3_14) {
    // Test that new voxel placements use the active resolution after switching
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Place voxel at initial resolution
    VoxelData::VoxelResolution initialResolution = VoxelData::VoxelResolution::Size_4cm;
    voxelManager->setActiveResolution(initialResolution);
    
    Math::Vector3i firstPosition(0, 0, 0);
    bool placed1 = voxelManager->setVoxel(firstPosition, initialResolution, true);
    EXPECT_TRUE(placed1) << "Should place first voxel at initial resolution";
    
    // Switch resolution
    VoxelData::VoxelResolution newResolution = VoxelData::VoxelResolution::Size_64cm;
    voxelManager->setActiveResolution(newResolution);
    
    // Place new voxel at different position using active resolution
    Math::Vector3i secondPosition(100, 0, 0);
    bool placed2 = voxelManager->setVoxel(secondPosition, newResolution, true);
    EXPECT_TRUE(placed2) << "Should place second voxel at new active resolution";
    
    // Verify both voxels exist at their respective resolutions
    bool hasFirstVoxel = voxelManager->hasVoxel(firstPosition, initialResolution);
    EXPECT_TRUE(hasFirstVoxel) << "First voxel should exist at initial resolution";
    
    bool hasSecondVoxel = voxelManager->hasVoxel(secondPosition, newResolution);
    EXPECT_TRUE(hasSecondVoxel) << "Second voxel should exist at new resolution";
    
    // Verify voxels don't exist at wrong resolutions
    bool firstAtWrongRes = voxelManager->hasVoxel(firstPosition, newResolution);
    EXPECT_FALSE(firstAtWrongRes) << "First voxel should not exist at new resolution";
    
    bool secondAtWrongRes = voxelManager->hasVoxel(secondPosition, initialResolution);
    EXPECT_FALSE(secondAtWrongRes) << "Second voxel should not exist at initial resolution";
}

TEST_F(ResolutionCommandTest, ResolutionSwitchingWithExistingVoxels_ResolutionConsistency_REQ_11_3_14) {
    // Test that resolution switching maintains data consistency across all operations
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Simplified test with just two resolutions to debug the issue
    VoxelData::VoxelResolution res1 = VoxelData::VoxelResolution::Size_4cm;
    VoxelData::VoxelResolution res2 = VoxelData::VoxelResolution::Size_16cm;
    
    // Place first voxel at res1
    voxelManager->setActiveResolution(res1);
    Math::Vector3i pos1(0, 0, 0);
    bool placed1 = voxelManager->setVoxel(pos1, res1, true);
    EXPECT_TRUE(placed1) << "Should place first voxel at 8cm resolution";
    
    // Verify it exists
    bool hasVoxel1 = voxelManager->hasVoxel(pos1, res1);
    EXPECT_TRUE(hasVoxel1) << "First voxel should exist at 8cm resolution";
    
    // Switch to res2 and place second voxel
    voxelManager->setActiveResolution(res2);
    Math::Vector3i pos2(100, 0, 0);
    bool placed2 = voxelManager->setVoxel(pos2, res2, true);
    EXPECT_TRUE(placed2) << "Should place second voxel at 16cm resolution";
    
    // Verify second voxel exists
    bool hasVoxel2 = voxelManager->hasVoxel(pos2, res2);
    EXPECT_TRUE(hasVoxel2) << "Second voxel should exist at 16cm resolution";
    
    // Verify first voxel still exists after resolution switch
    bool hasVoxel1AfterSwitch = voxelManager->hasVoxel(pos1, res1);
    EXPECT_TRUE(hasVoxel1AfterSwitch) << "First voxel should still exist after resolution switch";
    
    // Switch back to res1 and verify both voxels
    voxelManager->setActiveResolution(res1);
    bool hasVoxel1Final = voxelManager->hasVoxel(pos1, res1);
    EXPECT_TRUE(hasVoxel1Final) << "First voxel should exist when switched back to its resolution";
    
    bool hasVoxel2FromRes1 = voxelManager->hasVoxel(pos2, res2);
    EXPECT_TRUE(hasVoxel2FromRes1) << "Second voxel should still exist even when active resolution is different";
    
    // Note: getVoxelCount() may only return count for active resolution, not total
    // This is implementation-dependent behavior that we need to understand
}

TEST_F(ResolutionCommandTest, ResolutionSwitchingWithExistingVoxels_VisualFeedbackConsistency_REQ_11_3_14) {
    // Test that visual feedback updates correctly when switching resolutions with existing voxels
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Place voxels at different resolutions
    VoxelData::VoxelResolution smallRes = VoxelData::VoxelResolution::Size_1cm;
    VoxelData::VoxelResolution largeRes = VoxelData::VoxelResolution::Size_64cm;
    
    // Place small voxel
    voxelManager->setActiveResolution(smallRes);
    Math::Vector3i smallVoxelPos(0, 0, 0);
    bool placedSmall = voxelManager->setVoxel(smallVoxelPos, smallRes, true);
    EXPECT_TRUE(placedSmall) << "Should place small voxel";
    
    // Place large voxel
    voxelManager->setActiveResolution(largeRes);
    Math::Vector3i largeVoxelPos(100, 0, 0);
    bool placedLarge = voxelManager->setVoxel(largeVoxelPos, largeRes, true);
    EXPECT_TRUE(placedLarge) << "Should place large voxel";
    
    // Test resolution switching affects active resolution for feedback
    voxelManager->setActiveResolution(smallRes);
    EXPECT_EQ(voxelManager->getActiveResolution(), smallRes)
        << "Active resolution should be small for visual feedback";
    
    // Verify correct voxel size for visual feedback
    float smallVoxelSize = VoxelData::getVoxelSize(smallRes);
    float activeVoxelSize = VoxelData::getVoxelSize(voxelManager->getActiveResolution());
    EXPECT_FLOAT_EQ(activeVoxelSize, smallVoxelSize)
        << "Active voxel size should match small resolution for visual feedback";
    
    // Switch to large resolution
    voxelManager->setActiveResolution(largeRes);
    EXPECT_EQ(voxelManager->getActiveResolution(), largeRes)
        << "Active resolution should be large for visual feedback";
    
    // Verify visual feedback uses new active resolution
    float largeVoxelSize = VoxelData::getVoxelSize(largeRes);
    float newActiveVoxelSize = VoxelData::getVoxelSize(voxelManager->getActiveResolution());
    EXPECT_FLOAT_EQ(newActiveVoxelSize, largeVoxelSize)
        << "Active voxel size should match large resolution for visual feedback";
    
    // Verify existing voxels remain at their original resolutions
    bool hasSmallVoxel = voxelManager->hasVoxel(smallVoxelPos, smallRes);
    EXPECT_TRUE(hasSmallVoxel) << "Small voxel should persist with original resolution";
    
    bool hasLargeVoxel = voxelManager->hasVoxel(largeVoxelPos, largeRes);
    EXPECT_TRUE(hasLargeVoxel) << "Large voxel should persist with original resolution";
}

// ============================================================================
// REQ-11.3.13: Resolution command shall test invalid resolution values
// ============================================================================

TEST_F(ResolutionCommandTest, InvalidResolutionValues_InvalidNumbers_REQ_11_3_13) {
    // Test resolution command with invalid numeric values
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    // Get the command processor from the application
    auto* cmdProcessor = app->getCommandProcessor();
    ASSERT_NE(cmdProcessor, nullptr) << "Command processor should be available";
    
    // Record initial resolution state
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    
    // Test invalid numeric values that are not powers of 2 or not in valid range
    std::vector<std::string> invalidNumbers = {
        "3cm",      // Not a power of 2
        "5cm",      // Not in valid list
        "10cm",     // Not in valid list
        "0cm",      // Zero size
        "1024cm",   // Too large
        "15cm",     // Not valid
        "24cm",     // Not valid
        "7cm"       // Not valid
    };
    
    for (const std::string& invalidValue : invalidNumbers) {
        // Execute the invalid resolution command
        CommandResult result = cmdProcessor->execute("resolution " + invalidValue);
        
        // Verify the command failed
        EXPECT_FALSE(result.success) 
            << "Command 'resolution " << invalidValue << "' should fail";
        
        // Verify error message contains expected text
        EXPECT_TRUE(result.message.find("Invalid resolution") != std::string::npos)
            << "Error message should mention 'Invalid resolution' for value: " << invalidValue;
        
        // Verify resolution state unchanged
        VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
        EXPECT_EQ(currentResolution, initialResolution)
            << "Resolution should remain unchanged after invalid command: " << invalidValue;
    }
}

TEST_F(ResolutionCommandTest, InvalidResolutionValues_WrongUnits_REQ_11_3_13) {
    // Test resolution command with wrong units
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* cmdProcessor = app->getCommandProcessor();
    ASSERT_NE(cmdProcessor, nullptr) << "Command processor should be available";
    
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    
    // Test wrong unit formats
    std::vector<std::string> wrongUnits = {
        "1",        // Missing units
        "1m",       // Wrong units - meters instead of cm
        "10mm",     // Wrong units - millimeters
        "16inches", // Wrong units - imperial
        "32ft",     // Wrong units - feet
        "64meters"  // Wrong units - meters spelled out
    };
    
    for (const std::string& invalidValue : wrongUnits) {
        CommandResult result = cmdProcessor->execute("resolution " + invalidValue);
        
        EXPECT_FALSE(result.success) 
            << "Command 'resolution " << invalidValue << "' should fail";
        
        EXPECT_TRUE(result.message.find("Invalid resolution") != std::string::npos)
            << "Error message should mention 'Invalid resolution' for value: " << invalidValue;
        
        VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
        EXPECT_EQ(currentResolution, initialResolution)
            << "Resolution should remain unchanged after invalid command: " << invalidValue;
    }
}

TEST_F(ResolutionCommandTest, InvalidResolutionValues_FormatIssues_REQ_11_3_13) {
    // Test resolution command with format issues
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* cmdProcessor = app->getCommandProcessor();
    ASSERT_NE(cmdProcessor, nullptr) << "Command processor should be available";
    
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    
    // Test format issues (case sensitivity, spacing, etc.)
    std::vector<std::string> formatIssues = {
        "1CM",      // Uppercase
        "1 cm",     // Space between number and unit
        "cm1",      // Reversed format
        "1.5cm",    // Decimal values
        "-1cm",     // Negative values
        "+1cm"      // Plus sign
    };
    
    for (const std::string& invalidValue : formatIssues) {
        CommandResult result = cmdProcessor->execute("resolution " + invalidValue);
        
        EXPECT_FALSE(result.success) 
            << "Command 'resolution " << invalidValue << "' should fail";
        
        EXPECT_TRUE(result.message.find("Invalid resolution") != std::string::npos)
            << "Error message should mention 'Invalid resolution' for value: " << invalidValue;
        
        VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
        EXPECT_EQ(currentResolution, initialResolution)
            << "Resolution should remain unchanged after invalid command: " << invalidValue;
    }
}

TEST_F(ResolutionCommandTest, InvalidResolutionValues_InvalidStrings_REQ_11_3_13) {
    // Test resolution command with completely invalid strings
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* cmdProcessor = app->getCommandProcessor();
    ASSERT_NE(cmdProcessor, nullptr) << "Command processor should be available";
    
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    
    // Test completely invalid strings
    std::vector<std::string> invalidStrings = {
        "invalid",  // Completely invalid
        "abc",      // Non-numeric
        "small",    // Descriptive word
        "large",    // Descriptive word
        "medium",   // Descriptive word
        "x",        // Single character
        "resolution", // Command name itself
        "123xyz"    // Mixed invalid
    };
    
    for (const std::string& invalidValue : invalidStrings) {
        CommandResult result = cmdProcessor->execute("resolution " + invalidValue);
        
        EXPECT_FALSE(result.success) 
            << "Command 'resolution " << invalidValue << "' should fail";
        
        EXPECT_TRUE(result.message.find("Invalid resolution") != std::string::npos)
            << "Error message should mention 'Invalid resolution' for value: " << invalidValue;
        
        VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
        EXPECT_EQ(currentResolution, initialResolution)
            << "Resolution should remain unchanged after invalid command: " << invalidValue;
    }
}

TEST_F(ResolutionCommandTest, InvalidResolutionValues_ErrorMessageContent_REQ_11_3_13) {
    // Test that error messages provide helpful guidance
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* cmdProcessor = app->getCommandProcessor();
    ASSERT_NE(cmdProcessor, nullptr) << "Command processor should be available";
    
    // Test with one invalid value to check error message content
    CommandResult result = cmdProcessor->execute("resolution invalid");
    
    EXPECT_FALSE(result.success) 
        << "Invalid resolution command should fail";
    
    // Verify error message contains all valid resolution values
    std::vector<std::string> expectedValues = {
        "1cm", "4cm", "16cm", "64cm", "256cm"
    };
    
    for (const std::string& expectedValue : expectedValues) {
        EXPECT_TRUE(result.message.find(expectedValue) != std::string::npos)
            << "Error message should list valid value: " << expectedValue;
    }
    
    // Verify error message structure
    EXPECT_TRUE(result.message.find("Invalid resolution") != std::string::npos)
        << "Error message should start with 'Invalid resolution'";
    
    EXPECT_TRUE(result.message.find("Use:") != std::string::npos)
        << "Error message should provide guidance with 'Use:'";
}

TEST_F(ResolutionCommandTest, InvalidResolutionValues_NoParameters_REQ_11_3_13) {
    // Test resolution command with no parameters
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* cmdProcessor = app->getCommandProcessor();
    ASSERT_NE(cmdProcessor, nullptr) << "Command processor should be available";
    
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    
    // Test resolution command with no parameters
    CommandResult result = cmdProcessor->execute("resolution");
    
    EXPECT_FALSE(result.success) 
        << "Resolution command without parameters should fail";
    
    // Verify resolution state unchanged
    VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
    EXPECT_EQ(currentResolution, initialResolution)
        << "Resolution should remain unchanged after command with no parameters";
}

TEST_F(ResolutionCommandTest, InvalidResolutionValues_MultipleParameters_REQ_11_3_13) {
    // Test resolution command with multiple parameters - CLI accepts first parameter
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* cmdProcessor = app->getCommandProcessor();
    ASSERT_NE(cmdProcessor, nullptr) << "Command processor should be available";
    
    VoxelData::VoxelResolution initialResolution = voxelManager->getActiveResolution();
    
    // Test with invalid first parameter and multiple parameters
    std::vector<std::string> invalidMultipleParams = {
        "resolution invalid extra",
        "resolution 3cm 4cm",
        "resolution 0cm invalid",
        "resolution xyz abc def"
    };
    
    for (const std::string& command : invalidMultipleParams) {
        CommandResult result = cmdProcessor->execute(command);
        
        EXPECT_FALSE(result.success) 
            << "Command '" << command << "' should fail due to invalid first parameter";
        
        VoxelData::VoxelResolution currentResolution = voxelManager->getActiveResolution();
        EXPECT_EQ(currentResolution, initialResolution)
            << "Resolution should remain unchanged after command: " << command;
    }
}

} // namespace Tests
} // namespace VoxelEditor