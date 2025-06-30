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

class WorkspaceCommandValidationTest : public ::testing::Test {
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
// REQ-11.3.15: Workspace command shall test all valid workspace dimensions
// ============================================================================

TEST_F(WorkspaceCommandValidationTest, ValidMinimumDimensions) {
    // Test minimum valid workspace dimensions (2m x 2m x 2m)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Test minimum valid dimensions: 2m in each direction
    Math::Vector3f minSize(2.0f, 2.0f, 2.0f);
    bool result = voxelManager->resizeWorkspace(minSize);
    EXPECT_TRUE(result) << "Workspace should accept minimum valid dimensions 2x2x2 meters";
    
    // Verify the workspace size was set correctly
    Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
    EXPECT_FLOAT_EQ(actualSize.x, 2.0f) << "Width should be set to 2.0m";
    EXPECT_FLOAT_EQ(actualSize.y, 2.0f) << "Height should be set to 2.0m";
    EXPECT_FLOAT_EQ(actualSize.z, 2.0f) << "Depth should be set to 2.0m";
}

TEST_F(WorkspaceCommandValidationTest, ValidMaximumDimensions) {
    // Test maximum valid workspace dimensions (8m x 8m x 8m)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Test maximum valid dimensions: 8m in each direction
    Math::Vector3f maxSize(8.0f, 8.0f, 8.0f);
    bool result = voxelManager->resizeWorkspace(maxSize);
    EXPECT_TRUE(result) << "Workspace should accept maximum valid dimensions 8x8x8 meters";
    
    // Verify the workspace size was set correctly
    Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
    EXPECT_FLOAT_EQ(actualSize.x, 8.0f) << "Width should be set to 8.0m";
    EXPECT_FLOAT_EQ(actualSize.y, 8.0f) << "Height should be set to 8.0m";
    EXPECT_FLOAT_EQ(actualSize.z, 8.0f) << "Depth should be set to 8.0m";
}

TEST_F(WorkspaceCommandValidationTest, ValidMixedDimensions) {
    // Test various valid mixed dimensions within the 2m-8m range
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Test various valid mixed dimensions
    std::vector<Math::Vector3f> validSizes = {
        Math::Vector3f(3.5f, 4.0f, 2.5f),    // Mixed dimensions within range
        Math::Vector3f(6.0f, 3.0f, 7.5f),    // Different valid dimensions
        Math::Vector3f(2.5f, 8.0f, 4.2f),    // Mix of low, high, and mid values
        Math::Vector3f(7.9f, 2.1f, 5.8f),    // Near boundaries but valid
        Math::Vector3f(4.0f, 6.5f, 3.7f)     // All mid-range values
    };
    
    for (const auto& size : validSizes) {
        bool result = voxelManager->resizeWorkspace(size);
        EXPECT_TRUE(result) << "Workspace should accept valid dimensions " 
                           << size.x << "x" << size.y << "x" << size.z << " meters";
        
        // Verify the workspace size was set correctly
        Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
        EXPECT_FLOAT_EQ(actualSize.x, size.x) << "Width should be set correctly";
        EXPECT_FLOAT_EQ(actualSize.y, size.y) << "Height should be set correctly";
        EXPECT_FLOAT_EQ(actualSize.z, size.z) << "Depth should be set correctly";
    }
}

TEST_F(WorkspaceCommandValidationTest, ValidDefaultDimensions) {
    // Test the default workspace dimensions (5m x 5m x 5m)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Default size should be 5x5x5 meters
    Math::Vector3f defaultSize = voxelManager->getWorkspaceSize();
    EXPECT_FLOAT_EQ(defaultSize.x, 5.0f) << "Default width should be 5.0m";
    EXPECT_FLOAT_EQ(defaultSize.y, 5.0f) << "Default height should be 5.0m";
    EXPECT_FLOAT_EQ(defaultSize.z, 5.0f) << "Default depth should be 5.0m";
    
    // Test setting to default explicitly
    Math::Vector3f explicitDefault(5.0f, 5.0f, 5.0f);
    bool result = voxelManager->resizeWorkspace(explicitDefault);
    EXPECT_TRUE(result) << "Workspace should accept default dimensions 5x5x5 meters";
}

TEST_F(WorkspaceCommandValidationTest, ValidBoundaryDimensions) {
    // Test exact boundary values (2.0m and 8.0m)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Test exact boundary values
    std::vector<Math::Vector3f> boundarySizes = {
        Math::Vector3f(2.0f, 5.0f, 5.0f),    // Min width
        Math::Vector3f(5.0f, 2.0f, 5.0f),    // Min height
        Math::Vector3f(5.0f, 5.0f, 2.0f),    // Min depth
        Math::Vector3f(8.0f, 5.0f, 5.0f),    // Max width
        Math::Vector3f(5.0f, 8.0f, 5.0f),    // Max height
        Math::Vector3f(5.0f, 5.0f, 8.0f),    // Max depth
        Math::Vector3f(2.0f, 8.0f, 5.0f),    // Min-max combo
        Math::Vector3f(8.0f, 2.0f, 8.0f)     // Max-min-max combo
    };
    
    for (const auto& size : boundarySizes) {
        bool result = voxelManager->resizeWorkspace(size);
        EXPECT_TRUE(result) << "Workspace should accept boundary dimensions " 
                           << size.x << "x" << size.y << "x" << size.z << " meters";
        
        // Verify the workspace size was set correctly
        Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
        EXPECT_FLOAT_EQ(actualSize.x, size.x) << "Width should be set correctly";
        EXPECT_FLOAT_EQ(actualSize.y, size.y) << "Height should be set correctly";
        EXPECT_FLOAT_EQ(actualSize.z, size.z) << "Depth should be set correctly";
    }
}

TEST_F(WorkspaceCommandValidationTest, InvalidDimensionsTooSmall) {
    // Test invalid dimensions that are too small (< 2m)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Store original size to verify it doesn't change
    Math::Vector3f originalSize = voxelManager->getWorkspaceSize();
    
    // Test dimensions that are too small
    std::vector<Math::Vector3f> tooSmallSizes = {
        Math::Vector3f(1.0f, 5.0f, 5.0f),    // Width too small
        Math::Vector3f(5.0f, 1.0f, 5.0f),    // Height too small
        Math::Vector3f(5.0f, 5.0f, 1.0f),    // Depth too small
        Math::Vector3f(1.9f, 5.0f, 5.0f),    // Width just under minimum
        Math::Vector3f(5.0f, 1.5f, 5.0f),    // Height well under minimum
        Math::Vector3f(0.5f, 0.5f, 0.5f),    // All dimensions too small
        Math::Vector3f(1.999f, 8.0f, 2.0f)   // One dimension just under limit
    };
    
    for (const auto& size : tooSmallSizes) {
        bool result = voxelManager->resizeWorkspace(size);
        EXPECT_FALSE(result) << "Workspace should reject dimensions that are too small: " 
                            << size.x << "x" << size.y << "x" << size.z << " meters";
        
        // Verify the workspace size hasn't changed
        Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
        EXPECT_EQ(actualSize, originalSize) << "Workspace size should not change after invalid resize";
    }
}

TEST_F(WorkspaceCommandValidationTest, InvalidDimensionsTooLarge) {
    // Test invalid dimensions that are too large (> 8m)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Store original size to verify it doesn't change
    Math::Vector3f originalSize = voxelManager->getWorkspaceSize();
    
    // Test dimensions that are too large
    std::vector<Math::Vector3f> tooLargeSizes = {
        Math::Vector3f(9.0f, 5.0f, 5.0f),    // Width too large
        Math::Vector3f(5.0f, 9.0f, 5.0f),    // Height too large
        Math::Vector3f(5.0f, 5.0f, 9.0f),    // Depth too large
        Math::Vector3f(8.1f, 5.0f, 5.0f),    // Width just over maximum
        Math::Vector3f(5.0f, 10.0f, 5.0f),   // Height well over maximum
        Math::Vector3f(12.0f, 12.0f, 12.0f), // All dimensions too large
        Math::Vector3f(8.001f, 2.0f, 8.0f)   // One dimension just over limit
    };
    
    for (const auto& size : tooLargeSizes) {
        bool result = voxelManager->resizeWorkspace(size);
        EXPECT_FALSE(result) << "Workspace should reject dimensions that are too large: " 
                            << size.x << "x" << size.y << "x" << size.z << " meters";
        
        // Verify the workspace size hasn't changed
        Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
        EXPECT_EQ(actualSize, originalSize) << "Workspace size should not change after invalid resize";
    }
}

TEST_F(WorkspaceCommandValidationTest, InvalidZeroAndNegativeDimensions) {
    // Test invalid zero and negative dimensions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Store original size to verify it doesn't change
    Math::Vector3f originalSize = voxelManager->getWorkspaceSize();
    
    // Test zero and negative dimensions
    std::vector<Math::Vector3f> invalidSizes = {
        Math::Vector3f(0.0f, 5.0f, 5.0f),    // Zero width
        Math::Vector3f(5.0f, 0.0f, 5.0f),    // Zero height
        Math::Vector3f(5.0f, 5.0f, 0.0f),    // Zero depth
        Math::Vector3f(-1.0f, 5.0f, 5.0f),   // Negative width
        Math::Vector3f(5.0f, -1.0f, 5.0f),   // Negative height
        Math::Vector3f(5.0f, 5.0f, -1.0f),   // Negative depth
        Math::Vector3f(0.0f, 0.0f, 0.0f),    // All zero
        Math::Vector3f(-2.0f, -3.0f, -1.0f), // All negative
        Math::Vector3f(-0.5f, 0.0f, 2.0f)    // Mix of negative, zero, valid
    };
    
    for (const auto& size : invalidSizes) {
        bool result = voxelManager->resizeWorkspace(size);
        EXPECT_FALSE(result) << "Workspace should reject zero/negative dimensions: " 
                            << size.x << "x" << size.y << "x" << size.z << " meters";
        
        // Verify the workspace size hasn't changed
        Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
        EXPECT_EQ(actualSize, originalSize) << "Workspace size should not change after invalid resize";
    }
}

TEST_F(WorkspaceCommandValidationTest, ValidDimensionsNearBoundaries) {
    // Test valid dimensions very close to but within boundaries
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Test dimensions just inside the valid range
    std::vector<Math::Vector3f> nearBoundarySizes = {
        Math::Vector3f(2.001f, 5.0f, 5.0f),   // Just above minimum width
        Math::Vector3f(5.0f, 2.001f, 5.0f),   // Just above minimum height
        Math::Vector3f(5.0f, 5.0f, 2.001f),   // Just above minimum depth
        Math::Vector3f(7.999f, 5.0f, 5.0f),   // Just below maximum width
        Math::Vector3f(5.0f, 7.999f, 5.0f),   // Just below maximum height
        Math::Vector3f(5.0f, 5.0f, 7.999f),   // Just below maximum depth
        Math::Vector3f(2.1f, 7.9f, 5.0f),     // Near boundaries combo
        Math::Vector3f(7.8f, 2.2f, 7.7f)      // Multiple near-boundary values
    };
    
    for (const auto& size : nearBoundarySizes) {
        bool result = voxelManager->resizeWorkspace(size);
        EXPECT_TRUE(result) << "Workspace should accept near-boundary dimensions: " 
                           << size.x << "x" << size.y << "x" << size.z << " meters";
        
        // Verify the workspace size was set correctly
        Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
        EXPECT_FLOAT_EQ(actualSize.x, size.x) << "Width should be set correctly";
        EXPECT_FLOAT_EQ(actualSize.y, size.y) << "Height should be set correctly";
        EXPECT_FLOAT_EQ(actualSize.z, size.z) << "Depth should be set correctly";
    }
}

// ============================================================================
// REQ-11.3.16: Workspace command shall test minimum and maximum workspace limits
// ============================================================================

TEST_F(WorkspaceCommandValidationTest, MinimumWorkspaceLimits_REQ_11_3_16) {
    // Test enforcement of minimum workspace limits (2m per dimension)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f originalSize = voxelManager->getWorkspaceSize();
    
    // Test each dimension at exact minimum limit (2.0m)
    Math::Vector3f minSize(2.0f, 2.0f, 2.0f);
    bool result = voxelManager->resizeWorkspace(minSize);
    EXPECT_TRUE(result) << "Workspace should accept exact minimum dimensions (2x2x2 meters)";
    
    Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
    EXPECT_FLOAT_EQ(actualSize.x, 2.0f) << "Minimum width limit should be 2.0m";
    EXPECT_FLOAT_EQ(actualSize.y, 2.0f) << "Minimum height limit should be 2.0m";
    EXPECT_FLOAT_EQ(actualSize.z, 2.0f) << "Minimum depth limit should be 2.0m";
    
    // Test dimensions just below minimum limit (should be rejected)
    std::vector<Math::Vector3f> belowMinimum = {
        Math::Vector3f(1.999f, 2.0f, 2.0f),   // Width just below minimum
        Math::Vector3f(2.0f, 1.999f, 2.0f),   // Height just below minimum
        Math::Vector3f(2.0f, 2.0f, 1.999f),   // Depth just below minimum
        Math::Vector3f(1.0f, 2.0f, 2.0f),     // Width well below minimum
        Math::Vector3f(2.0f, 0.5f, 2.0f),     // Height well below minimum
        Math::Vector3f(2.0f, 2.0f, 1.5f)      // Depth well below minimum
    };
    
    for (const auto& size : belowMinimum) {
        voxelManager->resizeWorkspace(minSize); // Reset to valid size
        bool belowResult = voxelManager->resizeWorkspace(size);
        EXPECT_FALSE(belowResult) << "Workspace should reject dimensions below minimum: " 
                                 << size.x << "x" << size.y << "x" << size.z << " meters";
        
        // Verify workspace remains at previous valid size
        Math::Vector3f currentSize = voxelManager->getWorkspaceSize();
        EXPECT_EQ(currentSize, minSize) << "Workspace size should not change after failed resize";
    }
}

TEST_F(WorkspaceCommandValidationTest, MaximumWorkspaceLimits_REQ_11_3_16) {
    // Test enforcement of maximum workspace limits (8m per dimension)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    Math::Vector3f originalSize = voxelManager->getWorkspaceSize();
    
    // Test each dimension at exact maximum limit (8.0m)
    Math::Vector3f maxSize(8.0f, 8.0f, 8.0f);
    bool result = voxelManager->resizeWorkspace(maxSize);
    EXPECT_TRUE(result) << "Workspace should accept exact maximum dimensions (8x8x8 meters)";
    
    Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
    EXPECT_FLOAT_EQ(actualSize.x, 8.0f) << "Maximum width limit should be 8.0m";
    EXPECT_FLOAT_EQ(actualSize.y, 8.0f) << "Maximum height limit should be 8.0m";
    EXPECT_FLOAT_EQ(actualSize.z, 8.0f) << "Maximum depth limit should be 8.0m";
    
    // Test dimensions just above maximum limit (should be rejected)
    std::vector<Math::Vector3f> aboveMaximum = {
        Math::Vector3f(8.001f, 8.0f, 8.0f),   // Width just above maximum
        Math::Vector3f(8.0f, 8.001f, 8.0f),   // Height just above maximum
        Math::Vector3f(8.0f, 8.0f, 8.001f),   // Depth just above maximum
        Math::Vector3f(9.0f, 8.0f, 8.0f),     // Width well above maximum
        Math::Vector3f(8.0f, 10.0f, 8.0f),    // Height well above maximum
        Math::Vector3f(8.0f, 8.0f, 12.0f)     // Depth well above maximum
    };
    
    for (const auto& size : aboveMaximum) {
        voxelManager->resizeWorkspace(maxSize); // Reset to valid size
        bool aboveResult = voxelManager->resizeWorkspace(size);
        EXPECT_FALSE(aboveResult) << "Workspace should reject dimensions above maximum: " 
                                 << size.x << "x" << size.y << "x" << size.z << " meters";
        
        // Verify workspace remains at previous valid size
        Math::Vector3f currentSize = voxelManager->getWorkspaceSize();
        EXPECT_EQ(currentSize, maxSize) << "Workspace size should not change after failed resize";
    }
}

TEST_F(WorkspaceCommandValidationTest, WorkspaceLimitEnforcement_REQ_11_3_16) {
    // Test that workspace limits are consistently enforced across all dimensions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Test mixed boundary conditions
    std::vector<std::pair<Math::Vector3f, bool>> testCases = {
        // Valid boundary combinations
        {Math::Vector3f(2.0f, 8.0f, 5.0f), true},    // Min-max-mid combo
        {Math::Vector3f(8.0f, 2.0f, 8.0f), true},    // Max-min-max combo
        {Math::Vector3f(2.0f, 2.0f, 8.0f), true},    // Min-min-max combo
        {Math::Vector3f(8.0f, 8.0f, 2.0f), true},    // Max-max-min combo
        
        // Invalid boundary combinations
        {Math::Vector3f(1.999f, 8.0f, 5.0f), false}, // One dimension below min
        {Math::Vector3f(8.001f, 2.0f, 8.0f), false}, // One dimension above max
        {Math::Vector3f(1.0f, 9.0f, 5.0f), false},   // One below min, one above max
        {Math::Vector3f(0.5f, 1.5f, 9.5f), false},   // All dimensions invalid
    };
    
    for (const auto& testCase : testCases) {
        const Math::Vector3f& size = testCase.first;
        bool shouldSucceed = testCase.second;
        
        // Reset to default size before each test
        Math::Vector3f defaultSize(5.0f, 5.0f, 5.0f);
        voxelManager->resizeWorkspace(defaultSize);
        
        bool result = voxelManager->resizeWorkspace(size);
        EXPECT_EQ(result, shouldSucceed) 
            << "Workspace resize to " << size.x << "x" << size.y << "x" << size.z 
            << " should " << (shouldSucceed ? "succeed" : "fail");
        
        if (shouldSucceed) {
            // Verify the size was set correctly
            Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
            EXPECT_EQ(actualSize, size) << "Workspace size should match requested size";
        } else {
            // Verify the size remained unchanged
            Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
            EXPECT_EQ(actualSize, defaultSize) << "Workspace size should remain unchanged after failed resize";
        }
    }
}

TEST_F(WorkspaceCommandValidationTest, WorkspaceLimitPrecision_REQ_11_3_16) {
    // Test precision of workspace limit enforcement
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Test very small differences at boundaries
    struct PrecisionTest {
        Math::Vector3f size;
        bool shouldSucceed;
        std::string description;
    };
    
    std::vector<PrecisionTest> precisionTests = {
        // Minimum boundary precision tests
        {Math::Vector3f(2.0000f, 5.0f, 5.0f), true, "Exact minimum (2.0000m)"},
        {Math::Vector3f(2.0001f, 5.0f, 5.0f), true, "Just above minimum (2.0001m)"},
        {Math::Vector3f(1.9999f, 5.0f, 5.0f), false, "Just below minimum (1.9999m)"},
        
        // Maximum boundary precision tests
        {Math::Vector3f(8.0000f, 5.0f, 5.0f), true, "Exact maximum (8.0000m)"},
        {Math::Vector3f(7.9999f, 5.0f, 5.0f), true, "Just below maximum (7.9999m)"},
        {Math::Vector3f(8.0001f, 5.0f, 5.0f), false, "Just above maximum (8.0001m)"},
        
        // Multi-dimension precision tests
        {Math::Vector3f(2.0000f, 8.0000f, 5.0f), true, "Min-max precision combo"},
        {Math::Vector3f(1.9999f, 8.0001f, 5.0f), false, "Both dimensions violate limits"},
    };
    
    for (const auto& test : precisionTests) {
        // Reset to default size
        Math::Vector3f defaultSize(5.0f, 5.0f, 5.0f);
        voxelManager->resizeWorkspace(defaultSize);
        
        bool result = voxelManager->resizeWorkspace(test.size);
        EXPECT_EQ(result, test.shouldSucceed) 
            << test.description << " - Expected " << (test.shouldSucceed ? "success" : "failure")
            << " for size " << test.size.x << "x" << test.size.y << "x" << test.size.z;
        
        if (test.shouldSucceed) {
            Math::Vector3f actualSize = voxelManager->getWorkspaceSize();
            EXPECT_FLOAT_EQ(actualSize.x, test.size.x) << "Width precision should be maintained";
            EXPECT_FLOAT_EQ(actualSize.y, test.size.y) << "Height precision should be maintained";
            EXPECT_FLOAT_EQ(actualSize.z, test.size.z) << "Depth precision should be maintained";
        }
    }
}

TEST_F(WorkspaceCommandValidationTest, WorkspaceVolumeConstraints_REQ_11_3_16) {
    // Test workspace volume constraints and edge cases
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Calculate volume constraints
    float minVolume = 2.0f * 2.0f * 2.0f;  // 8 cubic meters (minimum)
    float maxVolume = 8.0f * 8.0f * 8.0f;  // 512 cubic meters (maximum)
    
    // Test minimum volume configuration
    Math::Vector3f minVolumeSize(2.0f, 2.0f, 2.0f);
    bool minResult = voxelManager->resizeWorkspace(minVolumeSize);
    EXPECT_TRUE(minResult) << "Minimum volume workspace (8 m³) should be accepted";
    
    // Test maximum volume configuration
    Math::Vector3f maxVolumeSize(8.0f, 8.0f, 8.0f);
    bool maxResult = voxelManager->resizeWorkspace(maxVolumeSize);
    EXPECT_TRUE(maxResult) << "Maximum volume workspace (512 m³) should be accepted";
    
    // Test various valid volume configurations within limits
    std::vector<Math::Vector3f> validVolumes = {
        Math::Vector3f(4.0f, 2.0f, 4.0f),    // 32 m³
        Math::Vector3f(2.0f, 8.0f, 4.0f),    // 64 m³  
        Math::Vector3f(8.0f, 4.0f, 4.0f),    // 128 m³
        Math::Vector3f(8.0f, 8.0f, 4.0f),    // 256 m³
    };
    
    for (const auto& size : validVolumes) {
        bool result = voxelManager->resizeWorkspace(size);
        float volume = size.x * size.y * size.z;
        EXPECT_TRUE(result) << "Valid volume configuration (" << volume << " m³) should be accepted";
        EXPECT_GE(volume, minVolume) << "Volume should be at least minimum";
        EXPECT_LE(volume, maxVolume) << "Volume should not exceed maximum";
    }
}

// ============================================================================
// REQ-11.3.17: Workspace command shall test workspace resizing with existing voxels
// ============================================================================

TEST_F(WorkspaceCommandValidationTest, WorkspaceResizeWithVoxels_ShouldSucceedWhenAllVoxelsWithinNewBounds) {
    // REQ-9.3.6: Workspace resize to smaller dimensions shall fail if any voxels would be outside the new bounds
    // This test verifies resize succeeds when all voxels remain within new bounds
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Start with 6x6x6 meters workspace
    ASSERT_TRUE(voxelManager->resizeWorkspace(6.0f));
    
    // Use smaller voxels for more precise placement
    voxelManager->setActiveResolution(VoxelEditor::VoxelData::VoxelResolution::Size_16cm);
    
    // Place voxels that will be within 4x4x4 bounds (4m = 400cm, bounds are -200cm to +200cm)
    // With 16cm voxels, we need to ensure the voxel extent stays within bounds
    std::vector<Math::Vector3i> voxelPositions = {
        Math::Vector3i(0, 0, 0),      // Center
        Math::Vector3i(100, 0, 100),  // 1m from center, well within 4x4x4
        Math::Vector3i(-100, 0, -100),// 1m from center, well within 4x4x4
        Math::Vector3i(150, 0, 0),    // 1.5m from center on X, within 4x4x4
    };
    
    for (const auto& pos : voxelPositions) {
        bool result = voxelManager->setVoxel(pos, VoxelEditor::VoxelData::VoxelResolution::Size_16cm, true);
        ASSERT_TRUE(result) << "Failed to place voxel at (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
    }
    
    // Resize to 4x4x4 - should succeed as all voxels are within bounds
    EXPECT_TRUE(voxelManager->resizeWorkspace(4.0f)) << "Resize should succeed when all voxels within new bounds";
    EXPECT_EQ(voxelManager->getWorkspaceSize(), Math::Vector3f(4.0f, 4.0f, 4.0f));
    
    // Verify all voxels still exist
    for (const auto& pos : voxelPositions) {
        EXPECT_TRUE(voxelManager->hasVoxel(pos, VoxelEditor::VoxelData::VoxelResolution::Size_16cm));
    }
}

TEST_F(WorkspaceCommandValidationTest, WorkspaceResizeWithVoxels_ShouldFailWhenVoxelsOutsideNewBounds) {
    // REQ-9.3.6: Workspace resize to smaller dimensions shall fail if any voxels would be outside the new bounds
    // This test verifies resize fails when voxels would be outside new bounds
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Start with 8x8x8 meters workspace
    ASSERT_TRUE(voxelManager->resizeWorkspace(8.0f));
    Math::Vector3f originalSize = voxelManager->getWorkspaceSize();
    
    // Place voxels - some would be outside 3x3x3 bounds
    voxelManager->setActiveResolution(VoxelEditor::VoxelData::VoxelResolution::Size_64cm);
    std::vector<Math::Vector3i> voxelPositions = {
        Math::Vector3i(0, 0, 0),      // Center - within 3x3x3
        Math::Vector3i(192, 0, 0),    // Outside 3x3x3 bounds
        Math::Vector3i(0, 0, 224),    // Outside 3x3x3 bounds
    };
    
    for (const auto& pos : voxelPositions) {
        ASSERT_TRUE(voxelManager->setVoxel(pos, VoxelEditor::VoxelData::VoxelResolution::Size_64cm, true));
    }
    
    size_t initialVoxelCount = voxelManager->getVoxelCount();
    
    // Try to resize to 3x3x3 - should FAIL because voxels would be outside bounds
    EXPECT_FALSE(voxelManager->resizeWorkspace(3.0f)) << "Resize should fail when voxels would be outside new bounds";
    
    // Verify workspace size unchanged
    EXPECT_EQ(voxelManager->getWorkspaceSize(), originalSize) << "Workspace size should remain unchanged after failed resize";
    
    // Verify all voxels still exist
    EXPECT_EQ(voxelManager->getVoxelCount(), initialVoxelCount) << "All voxels should be preserved after failed resize";
    for (const auto& pos : voxelPositions) {
        EXPECT_TRUE(voxelManager->hasVoxel(pos, VoxelEditor::VoxelData::VoxelResolution::Size_64cm));
    }
}

TEST_F(WorkspaceCommandValidationTest, WorkspaceResizeWithVoxels_EnlargementShouldAlwaysSucceed) {
    // REQ-9.3.3: Workspace resize to larger dimensions shall preserve all existing voxels
    // This test verifies enlargement always succeeds and preserves all voxels
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Start with 3x3x3 meters workspace
    ASSERT_TRUE(voxelManager->resizeWorkspace(3.0f));
    
    // Use 16cm voxels for better control
    voxelManager->setActiveResolution(VoxelEditor::VoxelData::VoxelResolution::Size_16cm);
    
    // Place voxels throughout 3x3x3 workspace (bounds: -150cm to +150cm)
    std::vector<Math::Vector3i> voxelPositions = {
        Math::Vector3i(0, 0, 0),      // Center
        Math::Vector3i(100, 0, 0),    // 1m from center
        Math::Vector3i(-100, 0, 0),   // -1m from center
        Math::Vector3i(0, 50, 100),   // Mixed coordinates
    };
    
    for (const auto& pos : voxelPositions) {
        bool result = voxelManager->setVoxel(pos, VoxelEditor::VoxelData::VoxelResolution::Size_16cm, true);
        ASSERT_TRUE(result) << "Failed to place voxel at (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
    }
    
    size_t initialVoxelCount = voxelManager->getVoxelCount();
    
    // Enlarge to 7x7x7 - should always succeed
    EXPECT_TRUE(voxelManager->resizeWorkspace(7.0f)) << "Workspace enlargement should always succeed";
    EXPECT_EQ(voxelManager->getWorkspaceSize(), Math::Vector3f(7.0f, 7.0f, 7.0f));
    
    // Verify all voxels preserved
    EXPECT_EQ(voxelManager->getVoxelCount(), initialVoxelCount) << "All voxels should be preserved during enlargement";
    for (const auto& pos : voxelPositions) {
        EXPECT_TRUE(voxelManager->hasVoxel(pos, VoxelEditor::VoxelData::VoxelResolution::Size_16cm));
    }
    
    // Verify we can place voxels in expanded area (7x7x7 bounds: -350cm to +350cm)
    Math::Vector3i newPos(300, 0, 300); // Within 7x7x7 but outside original 3x3x3
    EXPECT_TRUE(voxelManager->setVoxel(newPos, VoxelData::VoxelResolution::Size_16cm, true));
}

TEST_F(WorkspaceCommandValidationTest, WorkspaceResizeWithVoxels_MultipleResolutionsShouldFailWhenAnyOutsideBounds) {
    // REQ-9.3.6: Workspace resize shall fail if ANY voxels (regardless of resolution) would be outside bounds
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Start with 6x6x6 meters workspace
    ASSERT_TRUE(voxelManager->resizeWorkspace(6.0f));
    Math::Vector3f originalSize = voxelManager->getWorkspaceSize();
    
    // Place voxels at different resolutions
    struct VoxelTestData {
        Math::Vector3i position;
        VoxelEditor::VoxelData::VoxelResolution resolution;
        bool withinNewBounds; // Within 4x4x4?
    };
    
    std::vector<VoxelTestData> voxels = {
        // Within 4x4x4 bounds
        {Math::Vector3i(0, 0, 0), VoxelEditor::VoxelData::VoxelResolution::Size_16cm, true},
        {Math::Vector3i(32, 0, 32), VoxelEditor::VoxelData::VoxelResolution::Size_16cm, true},
        // Outside 4x4x4 bounds
        {Math::Vector3i(224, 0, 0), VoxelEditor::VoxelData::VoxelResolution::Size_16cm, false},
        {Math::Vector3i(0, 0, 256), VoxelEditor::VoxelData::VoxelResolution::Size_64cm, false}
    };
    
    // Place all voxels
    for (const auto& voxel : voxels) {
        voxelManager->setActiveResolution(voxel.resolution);
        ASSERT_TRUE(voxelManager->setVoxel(voxel.position, voxel.resolution, true));
    }
    
    size_t initialVoxelCount = voxelManager->getTotalVoxelCount();
    
    // Try to resize to 4x4x4 - should FAIL because some voxels are outside
    EXPECT_FALSE(voxelManager->resizeWorkspace(4.0f)) << "Resize should fail when any voxel would be outside bounds";
    
    // Verify nothing changed
    EXPECT_EQ(voxelManager->getWorkspaceSize(), originalSize);
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), initialVoxelCount);
    
    // Verify all voxels still exist
    for (const auto& voxel : voxels) {
        EXPECT_TRUE(voxelManager->hasVoxel(voxel.position, voxel.resolution));
    }
}

TEST_F(WorkspaceCommandValidationTest, WorkspaceResizeWithVoxels_TransactionSafety) {
    // REQ-9.3.4: Workspace resize failure shall leave the workspace and all voxels unchanged
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Start with 5x5x5 meters workspace
    ASSERT_TRUE(voxelManager->resizeWorkspace(5.0f));
    Math::Vector3f originalSize = voxelManager->getWorkspaceSize();
    
    // Place voxels - some would be outside 3x3x3 bounds
    voxelManager->setActiveResolution(VoxelEditor::VoxelData::VoxelResolution::Size_64cm);
    std::vector<Math::Vector3i> voxelPositions = {
        Math::Vector3i(0, 0, 0),      // Center - within 3x3x3
        Math::Vector3i(160, 0, 0),    // Outside 3x3x3 bounds
        Math::Vector3i(0, 0, 180),    // Outside 3x3x3 bounds
    };
    
    for (const auto& pos : voxelPositions) {
        ASSERT_TRUE(voxelManager->setVoxel(pos, VoxelEditor::VoxelData::VoxelResolution::Size_64cm, true));
    }
    
    size_t originalVoxelCount = voxelManager->getVoxelCount();
    
    // Try multiple failed resize attempts
    std::vector<float> failedSizes = {3.0f, 2.5f, 2.0f};
    
    for (float size : failedSizes) {
        // Attempt resize that should fail
        EXPECT_FALSE(voxelManager->resizeWorkspace(size)) << "Resize to " << size << "m should fail";
        
        // Verify nothing changed
        EXPECT_EQ(voxelManager->getWorkspaceSize(), originalSize) << "Workspace size should remain unchanged";
        EXPECT_EQ(voxelManager->getVoxelCount(), originalVoxelCount) << "Voxel count should remain unchanged";
        
        // Verify all voxels still exist
        for (const auto& pos : voxelPositions) {
            EXPECT_TRUE(voxelManager->hasVoxel(pos, VoxelEditor::VoxelData::VoxelResolution::Size_64cm))
                << "Voxel at " << pos.x << "," << pos.y << "," << pos.z << " should still exist";
        }
    }
    
    // Now do a successful resize (enlargement)
    EXPECT_TRUE(voxelManager->resizeWorkspace(7.0f)) << "Enlargement should succeed";
    EXPECT_EQ(voxelManager->getWorkspaceSize(), Math::Vector3f(7.0f, 7.0f, 7.0f));
    EXPECT_EQ(voxelManager->getVoxelCount(), originalVoxelCount) << "All voxels preserved during enlargement";
}

} // namespace Tests
} // namespace VoxelEditor