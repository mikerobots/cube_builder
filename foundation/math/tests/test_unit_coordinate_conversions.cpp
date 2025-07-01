#include <gtest/gtest.h>
#include "math/CoordinateConverter.h"
#include "math/CoordinateTypes.h"
#include "voxel_data/VoxelTypes.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

class CoordinateConversionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default workspace size for testing
        m_workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
    }

    Vector3f m_workspaceSize;
};

// Test that voxel at (0,0,0) has its bottom at Y=0
TEST_F(CoordinateConversionsTest, VoxelAtOriginHasBottomAtY0) {
    // Place a voxel at increment position (0,0,0)
    IncrementCoordinates incPos(0, 0, 0);
    
    // Convert to world coordinates - this should give bottom-center position
    WorldCoordinates worldPos = CoordinateConverter::incrementToWorld(incPos);
    
    // The Y coordinate should be 0 (bottom of voxel at ground plane)
    EXPECT_FLOAT_EQ(worldPos.y(), 0.0f) << "Voxel at (0,0,0) should have bottom at Y=0";
    
    // X and Z should also be 0 (center of voxel)
    EXPECT_FLOAT_EQ(worldPos.x(), 0.0f);
    EXPECT_FLOAT_EQ(worldPos.z(), 0.0f);
}

// Test increment to world conversion for various positions
TEST_F(CoordinateConversionsTest, IncrementToWorldConversion) {
    struct TestCase {
        IncrementCoordinates increment;
        WorldCoordinates expectedWorld;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        { IncrementCoordinates(0, 0, 0), WorldCoordinates(0.0f, 0.0f, 0.0f), "Origin" },
        { IncrementCoordinates(100, 0, 0), WorldCoordinates(1.0f, 0.0f, 0.0f), "1m right" },
        { IncrementCoordinates(-100, 0, 0), WorldCoordinates(-1.0f, 0.0f, 0.0f), "1m left" },
        { IncrementCoordinates(0, 100, 0), WorldCoordinates(0.0f, 1.0f, 0.0f), "1m up" },
        { IncrementCoordinates(0, 0, 100), WorldCoordinates(0.0f, 0.0f, 1.0f), "1m forward" },
        { IncrementCoordinates(0, 0, -100), WorldCoordinates(0.0f, 0.0f, -1.0f), "1m back" },
        { IncrementCoordinates(50, 25, -75), WorldCoordinates(0.5f, 0.25f, -0.75f), "Mixed position" },
    };
    
    for (const auto& tc : testCases) {
        WorldCoordinates result = CoordinateConverter::incrementToWorld(tc.increment);
        EXPECT_FLOAT_EQ(result.x(), tc.expectedWorld.x()) << tc.description;
        EXPECT_FLOAT_EQ(result.y(), tc.expectedWorld.y()) << tc.description;
        EXPECT_FLOAT_EQ(result.z(), tc.expectedWorld.z()) << tc.description;
    }
}

// Test world to increment conversion
TEST_F(CoordinateConversionsTest, WorldToIncrementConversion) {
    struct TestCase {
        WorldCoordinates world;
        IncrementCoordinates expectedIncrement;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        { WorldCoordinates(0.0f, 0.0f, 0.0f), IncrementCoordinates(0, 0, 0), "Origin" },
        { WorldCoordinates(1.0f, 0.0f, 0.0f), IncrementCoordinates(100, 0, 0), "1m right" },
        { WorldCoordinates(-1.0f, 0.0f, 0.0f), IncrementCoordinates(-100, 0, 0), "1m left" },
        { WorldCoordinates(0.0f, 1.0f, 0.0f), IncrementCoordinates(0, 100, 0), "1m up" },
        { WorldCoordinates(0.0f, 0.0f, 1.0f), IncrementCoordinates(0, 0, 100), "1m forward" },
        { WorldCoordinates(0.0f, 0.0f, -1.0f), IncrementCoordinates(0, 0, -100), "1m back" },
        { WorldCoordinates(0.5f, 0.25f, -0.75f), IncrementCoordinates(50, 25, -75), "Mixed position" },
    };
    
    for (const auto& tc : testCases) {
        IncrementCoordinates result = CoordinateConverter::worldToIncrement(tc.world);
        EXPECT_EQ(result.x(), tc.expectedIncrement.x()) << tc.description;
        EXPECT_EQ(result.y(), tc.expectedIncrement.y()) << tc.description;
        EXPECT_EQ(result.z(), tc.expectedIncrement.z()) << tc.description;
    }
}

// Test round-trip conversions
TEST_F(CoordinateConversionsTest, RoundTripConversions) {
    // Test increment -> world -> increment
    std::vector<IncrementCoordinates> incrementPositions = {
        IncrementCoordinates(0, 0, 0),
        IncrementCoordinates(10, 20, 30),
        IncrementCoordinates(-50, 100, -25),
        IncrementCoordinates(250, 0, -250),
    };
    
    for (const auto& incPos : incrementPositions) {
        WorldCoordinates worldPos = CoordinateConverter::incrementToWorld(incPos);
        IncrementCoordinates roundTrip = CoordinateConverter::worldToIncrement(worldPos);
        
        EXPECT_EQ(roundTrip.x(), incPos.x()) << "Round-trip X mismatch";
        EXPECT_EQ(roundTrip.y(), incPos.y()) << "Round-trip Y mismatch";
        EXPECT_EQ(roundTrip.z(), incPos.z()) << "Round-trip Z mismatch";
    }
    
    // Test world -> increment -> world (only for values that align to 1cm grid)
    std::vector<WorldCoordinates> worldPositions = {
        WorldCoordinates(0.0f, 0.0f, 0.0f),
        WorldCoordinates(0.1f, 0.2f, 0.3f),   // 10cm, 20cm, 30cm
        WorldCoordinates(-0.5f, 1.0f, -0.25f), // -50cm, 100cm, -25cm
        WorldCoordinates(2.5f, 0.0f, -2.5f),   // 250cm, 0cm, -250cm
    };
    
    for (const auto& worldPos : worldPositions) {
        IncrementCoordinates incPos = CoordinateConverter::worldToIncrement(worldPos);
        WorldCoordinates roundTrip = CoordinateConverter::incrementToWorld(incPos);
        
        EXPECT_NEAR(roundTrip.x(), worldPos.x(), 0.005f) << "Round-trip X mismatch"; // 0.5cm tolerance
        EXPECT_NEAR(roundTrip.y(), worldPos.y(), 0.005f) << "Round-trip Y mismatch";
        EXPECT_NEAR(roundTrip.z(), worldPos.z(), 0.005f) << "Round-trip Z mismatch";
    }
}

// Test voxel bounds calculation to ensure bottom-based positioning
TEST_F(CoordinateConversionsTest, VoxelBoundsBottomBased) {
    // Test with different voxel sizes
    std::vector<VoxelResolution> resolutions = {
        VoxelResolution::Size_1cm,
        VoxelResolution::Size_4cm,
        VoxelResolution::Size_16cm,
        VoxelResolution::Size_64cm,
    };
    
    for (const auto& resolution : resolutions) {
        VoxelPosition voxelPos(IncrementCoordinates(0, 0, 0), resolution);
        Vector3f minBounds, maxBounds;
        voxelPos.getWorldBounds(minBounds, maxBounds);
        
        float voxelSize = getVoxelSize(resolution);
        float halfSize = voxelSize * 0.5f;
        
        // Check that bottom is at Y=0
        EXPECT_FLOAT_EQ(minBounds.y, 0.0f) << "Voxel bottom should be at Y=0";
        EXPECT_FLOAT_EQ(maxBounds.y, voxelSize) << "Voxel top should be at Y=voxelSize";
        
        // Check X and Z bounds are centered
        EXPECT_FLOAT_EQ(minBounds.x, -halfSize);
        EXPECT_FLOAT_EQ(maxBounds.x, halfSize);
        EXPECT_FLOAT_EQ(minBounds.z, -halfSize);
        EXPECT_FLOAT_EQ(maxBounds.z, halfSize);
    }
}

// Test coordinate validation with Y >= 0 constraint
TEST_F(CoordinateConversionsTest, ValidateYGreaterThanOrEqualZero) {
    // Valid positions (Y >= 0)
    EXPECT_TRUE(CoordinateConverter::isValidIncrementCoordinate(
        IncrementCoordinates(0, 0, 0), m_workspaceSize));
    EXPECT_TRUE(CoordinateConverter::isValidIncrementCoordinate(
        IncrementCoordinates(50, 100, -50), m_workspaceSize));
    
    // Invalid positions (Y < 0)
    EXPECT_FALSE(CoordinateConverter::isValidIncrementCoordinate(
        IncrementCoordinates(0, -1, 0), m_workspaceSize));
    EXPECT_FALSE(CoordinateConverter::isValidIncrementCoordinate(
        IncrementCoordinates(0, -100, 0), m_workspaceSize));
    
    // World coordinates validation
    EXPECT_TRUE(CoordinateConverter::isValidWorldCoordinate(
        WorldCoordinates(0.0f, 0.0f, 0.0f), m_workspaceSize));
    EXPECT_TRUE(CoordinateConverter::isValidWorldCoordinate(
        WorldCoordinates(0.5f, 1.0f, -0.5f), m_workspaceSize));
    
    // Invalid world positions (Y < 0)
    EXPECT_FALSE(CoordinateConverter::isValidWorldCoordinate(
        WorldCoordinates(0.0f, -0.01f, 0.0f), m_workspaceSize));
    EXPECT_FALSE(CoordinateConverter::isValidWorldCoordinate(
        WorldCoordinates(0.0f, -1.0f, 0.0f), m_workspaceSize));
}

// Test workspace bounds calculation
TEST_F(CoordinateConversionsTest, WorkspaceBoundsCalculation) {
    auto [minBounds, maxBounds] = CoordinateConverter::getWorkspaceBoundsIncrement(m_workspaceSize);
    
    // Expected bounds for 5m x 5m x 5m workspace
    // X: -250cm to 250cm
    // Y: 0cm to 500cm  
    // Z: -250cm to 250cm
    EXPECT_EQ(minBounds.x(), -250);
    EXPECT_EQ(minBounds.y(), 0);
    EXPECT_EQ(minBounds.z(), -250);
    
    EXPECT_EQ(maxBounds.x(), 250);
    EXPECT_EQ(maxBounds.y(), 500);
    EXPECT_EQ(maxBounds.z(), 250);
}

// Test snapping to increment grid
TEST_F(CoordinateConversionsTest, SnapToIncrementGrid) {
    struct TestCase {
        WorldCoordinates input;
        WorldCoordinates expectedSnapped;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        { WorldCoordinates(0.0f, 0.0f, 0.0f), WorldCoordinates(0.0f, 0.0f, 0.0f), "Already on grid" },
        { WorldCoordinates(0.004f, 0.0f, 0.0f), WorldCoordinates(0.0f, 0.0f, 0.0f), "Round down" },
        { WorldCoordinates(0.006f, 0.0f, 0.0f), WorldCoordinates(0.01f, 0.0f, 0.0f), "Round up" },
        { WorldCoordinates(1.234f, 2.345f, -3.456f), WorldCoordinates(1.23f, 2.35f, -3.46f), "Complex position" },
    };
    
    for (const auto& tc : testCases) {
        WorldCoordinates result = CoordinateConverter::snapToIncrementGrid(tc.input);
        EXPECT_NEAR(result.x(), tc.expectedSnapped.x(), 0.001f) << tc.description;
        EXPECT_NEAR(result.y(), tc.expectedSnapped.y(), 0.001f) << tc.description;
        EXPECT_NEAR(result.z(), tc.expectedSnapped.z(), 0.001f) << tc.description;
    }
}

// Test voxel center calculation
TEST_F(CoordinateConversionsTest, VoxelCenterCalculation) {
    // Test world-space center calculation for voxels
    // Voxels are placed with their bottom face on the ground plane
    
    // For 1cm voxel at origin, center should be at Y=0.005m (half of 1cm)
    IncrementCoordinates pos1cm(0, 0, 0);
    WorldCoordinates center1cm = CoordinateConverter::getVoxelWorldCenter(
        pos1cm, VoxelResolution::Size_1cm);
    EXPECT_FLOAT_EQ(center1cm.x(), 0.0f);
    EXPECT_FLOAT_EQ(center1cm.y(), 0.005f);  // Half of 1cm = 0.005m
    EXPECT_FLOAT_EQ(center1cm.z(), 0.0f);
    
    // For 32cm voxel at origin, center should be at Y=0.16m (half of 32cm)
    IncrementCoordinates pos32cm(0, 0, 0);
    WorldCoordinates center32cm = CoordinateConverter::getVoxelWorldCenter(
        pos32cm, VoxelResolution::Size_32cm);
    EXPECT_FLOAT_EQ(center32cm.x(), 0.0f);
    EXPECT_FLOAT_EQ(center32cm.y(), 0.16f);  // Half of 32cm = 0.16m
    EXPECT_FLOAT_EQ(center32cm.z(), 0.0f);
    
    // For 64cm voxel at position (100, 50, -200), center calculation
    IncrementCoordinates pos64cm(100, 50, -200);
    WorldCoordinates center64cm = CoordinateConverter::getVoxelWorldCenter(
        pos64cm, VoxelResolution::Size_64cm);
    EXPECT_FLOAT_EQ(center64cm.x(), 1.0f);   // 100cm = 1m
    EXPECT_FLOAT_EQ(center64cm.y(), 0.82f);  // 50cm + 32cm (half of 64cm) = 0.82m
    EXPECT_FLOAT_EQ(center64cm.z(), -2.0f);  // -200cm = -2m
}