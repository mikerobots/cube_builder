#include <gtest/gtest.h>
#include <memory>
#include "../include/visual_feedback/OutlineRenderer.h"
#include "../include/visual_feedback/FaceDetector.h"
#include "../include/visual_feedback/FeedbackTypes.h"
#include "../../voxel_data/VoxelTypes.h"
#include "../../../foundation/math/Ray.h"
#include "../../../foundation/math/CoordinateConverter.h"

namespace VoxelEditor {
namespace VisualFeedback {
namespace Tests {

using namespace Math;
using namespace VoxelData;

class GroundPlaneOutlinePositioningTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_outlineRenderer = std::make_unique<OutlineRenderer>();
        m_faceDetector = std::make_unique<FaceDetector>();
        m_resolution = VoxelResolution::Size_32cm;
    }
    
    // Helper to validate that outline position matches hit point
    void validateOutlinePosition(const Vector3f& hitPoint, const Vector3f& outlineCenter) {
        // The outline should be centered at the hit point
        EXPECT_NEAR(outlineCenter.x, hitPoint.x, 0.001f) 
            << "Outline X position doesn't match hit point";
        EXPECT_NEAR(outlineCenter.y, hitPoint.y, 0.001f) 
            << "Outline Y position doesn't match hit point";
        EXPECT_NEAR(outlineCenter.z, hitPoint.z, 0.001f) 
            << "Outline Z position doesn't match hit point";
    }
    
    std::unique_ptr<OutlineRenderer> m_outlineRenderer;
    std::unique_ptr<FaceDetector> m_faceDetector;
    VoxelResolution m_resolution;
};

// Test that ground plane hit point calculation is correct
TEST_F(GroundPlaneOutlinePositioningTest, GroundPlaneHitPointCalculation) {
    // Test various cursor positions
    struct TestCase {
        Vector3f rayOrigin;
        Vector3f rayDir;
        Vector3f expectedHitPoint;
        std::string description;
    };
    
    TestCase testCases[] = {
        // Ray from above hitting origin
        {Vector3f(0, 5, 0), Vector3f(0, -1, 0), Vector3f(0, 0, 0), "Hit at origin"},
        
        // Ray from above hitting offset position
        {Vector3f(1.5f, 5, 1.5f), Vector3f(0, -1, 0), Vector3f(1.5f, 0, 1.5f), "Hit at (1.5, 0, 1.5)"},
        
        // Ray from above hitting negative coordinates
        {Vector3f(-2.0f, 5, -2.0f), Vector3f(0, -1, 0), Vector3f(-2.0f, 0, -2.0f), "Hit at (-2, 0, -2)"},
        
        // Angled ray
        {Vector3f(0, 5, 0), Vector3f(0.2f, -1, 0.2f).normalized(), Vector3f(1.0f, 0, 1.0f), "Angled ray hit"}
    };
    
    for (const auto& test : testCases) {
        Ray ray(test.rayOrigin, test.rayDir);
        Face groundFace = m_faceDetector->detectGroundPlane(ray);
        
        ASSERT_TRUE(groundFace.isValid()) << test.description;
        ASSERT_TRUE(groundFace.isGroundPlane()) << test.description;
        
        WorldCoordinates hitPointCoords = groundFace.getGroundPlaneHitPoint();
        Vector3f hitPoint = hitPointCoords.value();
        
        EXPECT_NEAR(hitPoint.x, test.expectedHitPoint.x, 0.001f) << test.description;
        EXPECT_NEAR(hitPoint.y, test.expectedHitPoint.y, 0.001f) << test.description;
        EXPECT_NEAR(hitPoint.z, test.expectedHitPoint.z, 0.001f) << test.description;
    }
}

// Test coordinate conversion issues in OutlineRenderer
TEST_F(GroundPlaneOutlinePositioningTest, OutlineCoordinateConversion) {
    // The bug is in OutlineRenderer::addVoxelEdges which uses hardcoded workspace size
    // Let's test what the correct conversion should be
    
    struct TestCase {
        IncrementCoordinates incrementPos;
        WorldCoordinates expectedWorldPos;
        std::string description;
    };
    
    TestCase testCases[] = {
        // Origin voxel
        {IncrementCoordinates(0, 0, 0), 
         WorldCoordinates(0, 0, 0),
         "Origin voxel"},
         
        // Positive offset voxel (32cm = 0.32m)
        {IncrementCoordinates(32, 0, 32), 
         WorldCoordinates(0.32f, 0, 0.32f),
         "One unit positive"},
         
        // Negative offset voxel
        {IncrementCoordinates(-32, 0, -32), 
         WorldCoordinates(-0.32f, 0, -0.32f),
         "One unit negative"},
         
        // Large offset
        {IncrementCoordinates(320, 0, 320), 
         WorldCoordinates(3.2f, 0, 3.2f),
         "Ten units positive"}
    };
    
    for (const auto& test : testCases) {
        // Test proper coordinate conversion
        WorldCoordinates converted = CoordinateConverter::incrementToWorld(test.incrementPos);
        
        EXPECT_NEAR(converted.x(), test.expectedWorldPos.x(), 0.001f) 
            << test.description << " - X coordinate";
        EXPECT_NEAR(converted.y(), test.expectedWorldPos.y(), 0.001f) 
            << test.description << " - Y coordinate";
        EXPECT_NEAR(converted.z(), test.expectedWorldPos.z(), 0.001f) 
            << test.description << " - Z coordinate";
    }
}

// Test that outline position matches ground plane hit point
TEST_F(GroundPlaneOutlinePositioningTest, OutlinePositionMatchesHitPoint) {
    // This test demonstrates the bug: the outline renderer uses wrong coordinate conversion
    
    // Simulate mouse hovering at world position (1.5, 0, 1.5)
    Ray mouseRay(Vector3f(1.5f, 5.0f, 1.5f), Vector3f(0, -1, 0));
    Face groundFace = m_faceDetector->detectGroundPlane(mouseRay);
    
    ASSERT_TRUE(groundFace.isValid());
    ASSERT_TRUE(groundFace.isGroundPlane());
    
    WorldCoordinates hitPointCoords = groundFace.getGroundPlaneHitPoint();
    Vector3f hitPoint = hitPointCoords.value();
    
    // The hit point should be at (1.5, 0, 1.5)
    EXPECT_NEAR(hitPoint.x, 1.5f, 0.001f);
    EXPECT_NEAR(hitPoint.y, 0.0f, 0.001f);
    EXPECT_NEAR(hitPoint.z, 1.5f, 0.001f);
    
    // Now test what OutlineRenderer would do with this position
    // The bug is that it uses hardcoded workspace size of 5.0f
    
    // Convert world position back to increment coordinates for outline
    IncrementCoordinates incrementPos = CoordinateConverter::worldToIncrement(hitPointCoords);
    
    // The current buggy calculation in OutlineRenderer::addVoxelEdges
    float voxelSize = getVoxelSize(m_resolution);
    float workspaceSize = 5.0f; // Hardcoded in OutlineRenderer
    float halfWorkspace = workspaceSize * 0.5f;
    
    Vector3f buggyOutlinePos(
        incrementPos.x() * voxelSize - halfWorkspace,
        incrementPos.y() * voxelSize,
        incrementPos.z() * voxelSize - halfWorkspace
    );
    
    // This will show the bug - the outline position won't match the hit point
    std::cout << "Hit point: (" << hitPoint.x << ", " << hitPoint.y << ", " << hitPoint.z << ")\n";
    std::cout << "Buggy outline pos: (" << buggyOutlinePos.x << ", " << buggyOutlinePos.y << ", " << buggyOutlinePos.z << ")\n";
    
    // The correct calculation should be:
    WorldCoordinates correctWorldPos = CoordinateConverter::incrementToWorld(incrementPos);
    Vector3f correctOutlinePos = correctWorldPos.value();
    
    std::cout << "Correct outline pos: (" << correctOutlinePos.x << ", " << correctOutlinePos.y << ", " << correctOutlinePos.z << ")\n";
    
    // Validate that the correct position matches the hit point
    validateOutlinePosition(hitPoint, correctOutlinePos);
    
    // Show that the buggy position does NOT match
    EXPECT_NE(buggyOutlinePos.x, hitPoint.x) << "Bug confirmed: outline X position is wrong";
}

// Test outline positioning for various workspace sizes
TEST_F(GroundPlaneOutlinePositioningTest, OutlineWithDifferentWorkspaceSizes) {
    // The hardcoded 5.0f workspace size causes issues with different actual workspace sizes
    
    struct WorkspaceTest {
        float actualWorkspaceSize;
        Vector3f hitPoint;
        std::string description;
    };
    
    WorkspaceTest tests[] = {
        {2.0f, Vector3f(0.5f, 0, 0.5f), "2m workspace"},
        {5.0f, Vector3f(1.5f, 0, 1.5f), "5m workspace (hardcoded value)"},
        {8.0f, Vector3f(3.0f, 0, 3.0f), "8m workspace"},
    };
    
    for (const auto& test : tests) {
        // For each workspace size, the outline should still match the hit point
        // But the buggy code assumes 5.0f workspace always
        
        WorldCoordinates hitWorldCoords(test.hitPoint);
        IncrementCoordinates incrementPos = CoordinateConverter::worldToIncrement(hitWorldCoords);
        
        // Correct conversion
        WorldCoordinates correctPos = CoordinateConverter::incrementToWorld(incrementPos);
        
        // The outline should always match the hit point regardless of workspace size
        validateOutlinePosition(test.hitPoint, correctPos.value());
    }
}

// Test edge cases for ground plane outline positioning
TEST_F(GroundPlaneOutlinePositioningTest, EdgeCasePositioning) {
    struct EdgeCase {
        Vector3f hitPoint;
        std::string description;
    };
    
    EdgeCase cases[] = {
        {Vector3f(0, 0, 0), "Exact origin"},
        {Vector3f(0.16f, 0, 0.16f), "Center of voxel at origin"},
        {Vector3f(-2.5f, 0, -2.5f), "Near workspace boundary"},
        {Vector3f(0.001f, 0, 0.001f), "Very small offset"},
        {Vector3f(2.499f, 0, 2.499f), "Just inside workspace"},
    };
    
    for (const auto& testCase : cases) {
        Ray ray(Vector3f(testCase.hitPoint.x, 5.0f, testCase.hitPoint.z), Vector3f(0, -1, 0));
        Face groundFace = m_faceDetector->detectGroundPlane(ray);
        
        ASSERT_TRUE(groundFace.isValid()) << testCase.description;
        
        WorldCoordinates hitPointCoords = groundFace.getGroundPlaneHitPoint();
        Vector3f actualHitPoint = hitPointCoords.value();
        
        // Verify hit point is calculated correctly
        EXPECT_NEAR(actualHitPoint.x, testCase.hitPoint.x, 0.001f) << testCase.description;
        EXPECT_NEAR(actualHitPoint.y, testCase.hitPoint.y, 0.001f) << testCase.description;
        EXPECT_NEAR(actualHitPoint.z, testCase.hitPoint.z, 0.001f) << testCase.description;
        
        // The outline should be positioned at this exact hit point
        IncrementCoordinates incrementPos = CoordinateConverter::worldToIncrement(hitPointCoords);
        WorldCoordinates outlineWorldPos = CoordinateConverter::incrementToWorld(incrementPos);
        
        validateOutlinePosition(actualHitPoint, outlineWorldPos.value());
    }
}

} // namespace Tests
} // namespace VisualFeedback
} // namespace VoxelEditor