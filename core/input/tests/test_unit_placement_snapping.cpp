#include <gtest/gtest.h>
#include "../PlacementValidation.h"
#include "../../voxel_data/VoxelTypes.h"
#include "../../../foundation/math/CoordinateConverter.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Input;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class PlacementSnappingTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
    }

    Vector3f m_workspaceSize;
};

TEST_F(PlacementSnappingTest, BasicIncrementSnapping) {
    // Test that world positions snap to 1cm increments
    struct TestCase {
        Vector3f worldPos;
        Vector3i expectedIncrement;
        const char* description;
    };
    
    TestCase testCases[] = {
        // Exact positions
        {{0.0f, 0.0f, 0.0f}, {0, 0, 0}, "Origin should stay at origin"},
        {{0.01f, 0.01f, 0.01f}, {1, 1, 1}, "1cm position should map to increment 1"},
        {{1.0f, 1.0f, 1.0f}, {100, 100, 100}, "1m position should map to increment 100"},
        
        // Positions needing rounding
        {{0.004f, 0.004f, 0.004f}, {0, 0, 0}, "0.4cm should round down to 0"},
        {{0.005f, 0.005f, 0.005f}, {1, 1, 1}, "0.5cm should round up to 1cm"},
        {{0.006f, 0.006f, 0.006f}, {1, 1, 1}, "0.6cm should round up to 1cm"},
        {{0.014f, 0.014f, 0.014f}, {1, 1, 1}, "1.4cm should round down to 1cm"},
        {{0.015f, 0.015f, 0.015f}, {2, 2, 2}, "1.5cm should round up to 2cm"},
        
        // Negative positions
        {{-0.01f, 0.0f, -0.01f}, {-1, 0, -1}, "Negative positions should work"},
        {{-0.004f, 0.0f, -0.004f}, {0, 0, 0}, "-0.4cm should round up to 0"},
        {{-0.005f, 0.0f, -0.005f}, {-1, 0, -1}, "-0.5cm should round down to -1cm"},
        
        // Mixed positions
        {{0.123f, 0.456f, 0.789f}, {12, 46, 79}, "Arbitrary position should round correctly"},
        {{-1.234f, 0.567f, -0.891f}, {-123, 57, -89}, "Mixed negative/positive should work"}
    };
    
    for (const auto& tc : testCases) {
        WorldCoordinates worldPos(tc.worldPos);
        IncrementCoordinates result = PlacementUtils::snapToValidIncrement(worldPos);
        
        EXPECT_EQ(result.x(), tc.expectedIncrement.x) << tc.description << " - X mismatch";
        EXPECT_EQ(result.y(), tc.expectedIncrement.y) << tc.description << " - Y mismatch";
        EXPECT_EQ(result.z(), tc.expectedIncrement.z) << tc.description << " - Z mismatch";
    }
}

TEST_F(PlacementSnappingTest, GridAlignedSnappingIgnoresParameters) {
    // Test that snapToGridAligned ignores resolution and shift parameters
    WorldCoordinates worldPos(Vector3f(0.123f, 0.456f, 0.789f));
    
    // Try with different resolutions and shift states
    auto result1 = PlacementUtils::snapToGridAligned(worldPos, VoxelResolution::Size_1cm, false);
    auto result2 = PlacementUtils::snapToGridAligned(worldPos, VoxelResolution::Size_32cm, false);
    auto result3 = PlacementUtils::snapToGridAligned(worldPos, VoxelResolution::Size_1cm, true);
    auto result4 = PlacementUtils::snapToGridAligned(worldPos, VoxelResolution::Size_32cm, true);
    
    // All should produce the same result (1cm snapping)
    EXPECT_EQ(result1.value(), result2.value()) << "Resolution should not affect snapping";
    EXPECT_EQ(result1.value(), result3.value()) << "Shift key should not affect snapping";
    EXPECT_EQ(result1.value(), result4.value()) << "Both parameters should be ignored";
    
    // Verify the actual snapped position
    EXPECT_EQ(result1.x(), 12);  // 0.123m -> 12.3cm -> 12cm
    EXPECT_EQ(result1.y(), 46);  // 0.456m -> 45.6cm -> 46cm
    EXPECT_EQ(result1.z(), 79);  // 0.789m -> 78.9cm -> 79cm
}

TEST_F(PlacementSnappingTest, PlacementContextUsesBasicSnapping) {
    // Test that getPlacementContext uses basic 1cm snapping
    WorldCoordinates worldPos(Vector3f(0.234f, 0.567f, 0.891f));
    
    // Test with different resolutions
    auto context1cm = PlacementUtils::getPlacementContext(worldPos, VoxelResolution::Size_1cm, false, m_workspaceSize);
    auto context32cm = PlacementUtils::getPlacementContext(worldPos, VoxelResolution::Size_32cm, false, m_workspaceSize);
    
    // Both should snap to same 1cm increment
    EXPECT_EQ(context1cm.snappedIncrementPos.value(), context32cm.snappedIncrementPos.value())
        << "Different resolutions should still snap to same 1cm increment";
    
    // Verify the snapped position
    EXPECT_EQ(context1cm.snappedIncrementPos.x(), 23);  // 0.234m -> 23.4cm -> 23cm
    EXPECT_EQ(context1cm.snappedIncrementPos.y(), 57);  // 0.567m -> 56.7cm -> 57cm
    EXPECT_EQ(context1cm.snappedIncrementPos.z(), 89);  // 0.891m -> 89.1cm -> 89cm
}

TEST_F(PlacementSnappingTest, ShiftKeyBehaviorInBasicContext) {
    // Test that shift key doesn't change snapping in basic context
    WorldCoordinates worldPos(Vector3f(0.345f, 0.678f, 0.912f));
    
    auto contextNoShift = PlacementUtils::getPlacementContext(worldPos, VoxelResolution::Size_4cm, false, m_workspaceSize);
    auto contextWithShift = PlacementUtils::getPlacementContext(worldPos, VoxelResolution::Size_4cm, true, m_workspaceSize);
    
    // Both should produce same result
    EXPECT_EQ(contextNoShift.snappedIncrementPos.value(), contextWithShift.snappedIncrementPos.value())
        << "Shift key should not affect basic snapping";
    
    EXPECT_FALSE(contextNoShift.shiftPressed);
    EXPECT_TRUE(contextWithShift.shiftPressed);
}

TEST_F(PlacementSnappingTest, ValidPositionChecking) {
    // Test isValidIncrementPosition
    EXPECT_TRUE(PlacementUtils::isValidIncrementPosition(IncrementCoordinates(0, 0, 0)));
    EXPECT_TRUE(PlacementUtils::isValidIncrementPosition(IncrementCoordinates(100, 50, -50)));
    EXPECT_FALSE(PlacementUtils::isValidIncrementPosition(IncrementCoordinates(0, -1, 0))) 
        << "Y < 0 should be invalid";
    EXPECT_FALSE(PlacementUtils::isValidIncrementPosition(IncrementCoordinates(0, -100, 0))) 
        << "Y < 0 should be invalid";
}

TEST_F(PlacementSnappingTest, EdgeCaseRounding) {
    // Test edge cases in rounding behavior
    struct EdgeCase {
        float input;
        int expected;
        const char* description;
    };
    
    EdgeCase cases[] = {
        {0.0f, 0, "Zero"},
        {0.004999f, 0, "Just below 0.5cm"},
        {0.005f, 1, "Exactly 0.5cm"},
        {0.005001f, 1, "Just above 0.5cm"},
        {-0.004999f, 0, "Just above -0.5cm"},
        {-0.005f, -1, "Exactly -0.5cm"},
        {-0.005001f, -1, "Just below -0.5cm"},
        {0.994999f, 99, "Just below 99.5cm"},
        {0.995f, 100, "Exactly 99.5cm"},
        {0.995001f, 100, "Just above 99.5cm"}
    };
    
    for (const auto& tc : cases) {
        WorldCoordinates worldPos(Vector3f(tc.input, 0.0f, 0.0f));
        auto result = PlacementUtils::snapToValidIncrement(worldPos);
        EXPECT_EQ(result.x(), tc.expected) << tc.description << ": " << tc.input << " should snap to " << tc.expected;
    }
}

TEST_F(PlacementSnappingTest, PlacementValidationWithSnapping) {
    // Test that validation works correctly with snapped positions
    
    // Position that would be out of bounds without snapping but valid after snapping
    WorldCoordinates edgePos(Vector3f(2.494f, 0.0f, 0.0f));  // 249.4cm
    auto context = PlacementUtils::getPlacementContext(edgePos, VoxelResolution::Size_1cm, false, m_workspaceSize);
    
    // Should snap to 249cm, which is valid for 1cm voxel in 5m workspace (extends to -250cm to +250cm)
    EXPECT_EQ(context.snappedIncrementPos.x(), 249);
    EXPECT_EQ(context.validation, PlacementValidationResult::Valid);
    
    // Position that's still out of bounds after snapping
    WorldCoordinates farPos(Vector3f(2.506f, 0.0f, 0.0f));  // 250.6cm
    auto farContext = PlacementUtils::getPlacementContext(farPos, VoxelResolution::Size_1cm, false, m_workspaceSize);
    
    // Should snap to 251cm, which is out of bounds
    EXPECT_EQ(farContext.snappedIncrementPos.x(), 251);
    EXPECT_EQ(farContext.validation, PlacementValidationResult::InvalidOutOfBounds);
}

TEST_F(PlacementSnappingTest, ConsistencyWithCoordinateConverter) {
    // Verify that PlacementUtils::snapToValidIncrement produces same results as CoordinateConverter
    std::vector<Vector3f> testPositions = {
        {0.0f, 0.0f, 0.0f},
        {1.234f, 5.678f, 9.012f},
        {-1.234f, 0.567f, -8.901f},
        {0.005f, 0.015f, 0.025f},
        {-0.005f, 0.0f, -0.015f}
    };
    
    for (const auto& pos : testPositions) {
        WorldCoordinates worldPos(pos);
        
        auto placementResult = PlacementUtils::snapToValidIncrement(worldPos);
        auto converterResult = CoordinateConverter::worldToIncrement(worldPos);
        
        EXPECT_EQ(placementResult.value(), converterResult.value()) 
            << "PlacementUtils and CoordinateConverter should produce identical results for " 
            << pos.x << ", " << pos.y << ", " << pos.z;
    }
}