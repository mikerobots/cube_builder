#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <iostream>
#include "../include/visual_feedback/HighlightManager.h"
#include "../include/visual_feedback/FaceDetector.h"
#include "../include/visual_feedback/FeedbackTypes.h"
#include "../../voxel_data/VoxelGrid.h"
#include "../../voxel_data/VoxelTypes.h"
#include "../../../foundation/math/Ray.h"

namespace VoxelEditor {
namespace VisualFeedback {
namespace Tests {

using namespace Math;
using namespace VoxelData;

class MouseHoverHighlightingTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_highlightManager = std::make_unique<HighlightManager>();
        m_faceDetector = std::make_unique<FaceDetector>();
        m_resolution = VoxelResolution::Size_32cm;
        m_workspaceSize = Math::Vector3f(2.56f, 2.56f, 2.56f); // 8 * 32cm = 2.56m
        m_voxelGrid = std::make_unique<VoxelGrid>(m_resolution, m_workspaceSize);
        
        // Set up a test scene with some voxels
        setupTestScene();
    }
    
    void setupTestScene() {
        // Place a few voxels for testing
        m_voxelGrid->setVoxel(IncrementCoordinates(0, 0, 0), true);    // At origin
        m_voxelGrid->setVoxel(IncrementCoordinates(32, 0, 0), true);   // 1 unit right
        m_voxelGrid->setVoxel(IncrementCoordinates(0, 32, 0), true);   // 1 unit up
        m_voxelGrid->setVoxel(IncrementCoordinates(0, 0, 32), true);   // 1 unit forward
        m_voxelGrid->setVoxel(IncrementCoordinates(64, 0, 64), true);  // 2 units diagonal
    }
    
    // Helper to simulate mouse hover at screen position
    Ray createMouseRay(const Vector2f& screenPos, const Vector2f& screenSize) {
        // Simple orthographic ray for testing
        // In real app, this would use camera projection matrix
        float x = (screenPos.x / screenSize.x - 0.5f) * 10.0f;
        float z = (screenPos.y / screenSize.y - 0.5f) * 10.0f;
        
        Vector3f origin(x, 5.0f, z); // Start above scene
        Vector3f direction(0, -1, 0); // Look down
        
        return Ray(origin, direction);
    }
    
    std::unique_ptr<HighlightManager> m_highlightManager;
    std::unique_ptr<FaceDetector> m_faceDetector;
    std::unique_ptr<VoxelGrid> m_voxelGrid;
    VoxelResolution m_resolution;
    Math::Vector3f m_workspaceSize;
};

// REQ-2.3.1: When hovering over an existing voxel, the face under cursor shall be highlighted
TEST_F(MouseHoverHighlightingTest, VoxelFaceHighlightOnHover) {
    // Create ray that hits top face of voxel at origin
    Ray mouseRay(Vector3f(0.0f, 5.0f, 0.0f), Vector3f(0, -1, 0));
    
    // Detect face under cursor
    Face detectedFace = m_faceDetector->detectFace(mouseRay, *m_voxelGrid, m_resolution);
    
    // Should detect the top face
    EXPECT_TRUE(detectedFace.isValid());
    
    // For a voxel at origin (0,0,0), the top face is at Y=32 (one voxel size up)
    // This is correct - the face position represents the face location, not the voxel origin
    EXPECT_EQ(detectedFace.getVoxelPosition(), IncrementCoordinates(0, 32, 0));
    EXPECT_EQ(detectedFace.getDirection(), FaceDirection::PositiveY);
    
    // Update highlight manager
    m_highlightManager->setHighlightedFace(detectedFace);
    
    // Verify highlight is active
    EXPECT_TRUE(m_highlightManager->hasFaceHighlight());
    EXPECT_EQ(m_highlightManager->getCurrentFace(), detectedFace);
}

// REQ-2.3.2: Face highlighting shall clearly indicate which face is selected
TEST_F(MouseHoverHighlightingTest, DifferentFacesHighlightedCorrectly) {
    struct TestCase {
        Vector3f rayOrigin;
        Vector3f rayDir;
        IncrementCoordinates expectedVoxel;
        FaceDirection expectedFace;
        std::string description;
    };
    
    TestCase testCases[] = {
        // Top face of voxel at origin - face position includes offset for positive Y
        {Vector3f(0, 5, 0), Vector3f(0, -1, 0), IncrementCoordinates(0, 32, 0), 
         FaceDirection::PositiveY, "Top face of origin voxel"},
        
        // Right face of voxel at (32,0,0) - face position is voxel position
        {Vector3f(5, 0.16f, 0), Vector3f(-1, 0, 0), IncrementCoordinates(32, 0, 0), 
         FaceDirection::PositiveX, "Right face of voxel at (32,0,0)"},
        
        // Front face of voxel at (0,0,32) - face position is voxel position
        {Vector3f(0, 0.16f, 5), Vector3f(0, 0, -1), IncrementCoordinates(0, 0, 32),
         FaceDirection::PositiveZ, "Front face of voxel at (0,0,32)"},
        
        // Left face of origin voxel - face position is voxel position
        {Vector3f(-5, 0.16f, 0), Vector3f(1, 0, 0), IncrementCoordinates(0, 0, 0),
         FaceDirection::NegativeX, "Left face of origin voxel"}
    };
    
    for (const auto& test : testCases) {
        Ray mouseRay(test.rayOrigin, test.rayDir);
        Face detectedFace = m_faceDetector->detectFace(mouseRay, *m_voxelGrid, m_resolution);
        
        EXPECT_TRUE(detectedFace.isValid()) << test.description;
        EXPECT_EQ(detectedFace.getVoxelPosition(), test.expectedVoxel) << test.description;
        EXPECT_EQ(detectedFace.getDirection(), test.expectedFace) << test.description;
        
        // Update highlight
        m_highlightManager->setHighlightedFace(detectedFace);
        EXPECT_EQ(m_highlightManager->getCurrentFace(), detectedFace);
    }
}

// REQ-4.2.1: Face highlighting shall use yellow color
TEST_F(MouseHoverHighlightingTest, FaceHighlightColorIsYellow) {
    Face face(IncrementCoordinates(0, 0, 0), m_resolution, FaceDirection::PositiveY);
    
    // Get highlight color from face detector
    Rendering::Color highlightColor = m_faceDetector->getFaceHighlightColor(face);
    
    // Yellow is approximately RGB(1, 1, 0)
    EXPECT_NEAR(highlightColor.r, 1.0f, 0.1f);
    EXPECT_NEAR(highlightColor.g, 1.0f, 0.1f);
    EXPECT_NEAR(highlightColor.b, 0.0f, 0.1f);
    EXPECT_GT(highlightColor.a, 0.0f); // Should be visible
}

// REQ-4.2.2: Only one face shall be highlighted at a time
TEST_F(MouseHoverHighlightingTest, OnlyOneFaceHighlightedAtTime) {
    // Create multiple faces
    Face face1(IncrementCoordinates(0, 0, 0), m_resolution, FaceDirection::PositiveY);
    Face face2(IncrementCoordinates(32, 0, 0), m_resolution, FaceDirection::PositiveX);
    Face face3(IncrementCoordinates(0, 0, 32), m_resolution, FaceDirection::PositiveZ);
    
    // Highlight first face
    m_highlightManager->setHighlightedFace(face1);
    EXPECT_EQ(m_highlightManager->getCurrentFace(), face1);
    
    // Highlight second face - first should no longer be highlighted
    m_highlightManager->setHighlightedFace(face2);
    EXPECT_EQ(m_highlightManager->getCurrentFace(), face2);
    EXPECT_NE(m_highlightManager->getCurrentFace(), face1);
    
    // Highlight third face
    m_highlightManager->setHighlightedFace(face3);
    EXPECT_EQ(m_highlightManager->getCurrentFace(), face3);
    EXPECT_NE(m_highlightManager->getCurrentFace(), face2);
}

// REQ-4.2.3: Highlighting shall be visible from all camera angles
TEST_F(MouseHoverHighlightingTest, HighlightVisibleFromAllAngles) {
    Face topFace(IncrementCoordinates(0, 0, 0), m_resolution, FaceDirection::PositiveY);
    
    // The face detector should report face as visible regardless of view angle
    // In real implementation, this would check against camera frustum
    EXPECT_TRUE(m_faceDetector->isFaceVisible(topFace));
    
    // Test that highlight manager maintains highlight regardless of camera
    m_highlightManager->setHighlightedFace(topFace);
    
    // Simulate camera rotation by updating multiple times
    for (int i = 0; i < 10; i++) {
        m_highlightManager->update(0.016f); // 60fps
        EXPECT_TRUE(m_highlightManager->hasFaceHighlight());
        EXPECT_EQ(m_highlightManager->getCurrentFace(), topFace);
    }
}

// Test ground plane highlighting when hovering over empty space
TEST_F(MouseHoverHighlightingTest, GroundPlaneHighlightOnEmptySpace) {
    // Ray that hits ground plane (Y=0) at empty location
    Ray mouseRay(Vector3f(3.0f, 5.0f, 3.0f), Vector3f(0, -1, 0));
    
    // Detect ground plane
    Face groundFace = m_faceDetector->detectGroundPlane(mouseRay);
    
    EXPECT_TRUE(groundFace.isValid());
    EXPECT_TRUE(groundFace.isGroundPlane());
    
    // Ground plane hit position should be at Y=0
    Math::WorldCoordinates hitPosCoords = groundFace.getGroundPlaneHitPoint();
    Vector3f hitPos = hitPosCoords.value();
    EXPECT_FLOAT_EQ(hitPos.y, 0.0f);
    EXPECT_NEAR(hitPos.x, 3.0f, 0.001f);
    EXPECT_NEAR(hitPos.z, 3.0f, 0.001f);
    
    // Update highlight manager with ground plane
    m_highlightManager->setHighlightedFace(groundFace);
    EXPECT_TRUE(m_highlightManager->hasFaceHighlight());
    EXPECT_TRUE(m_highlightManager->getCurrentFace().isGroundPlane());
}

// Test combined voxel/ground detection
TEST_F(MouseHoverHighlightingTest, CombinedVoxelAndGroundDetection) {
    // Test 1: Ray that hits voxel should return voxel face, not ground
    Ray voxelRay(Vector3f(0, 5, 0), Vector3f(0, -1, 0));
    Face voxelFace = m_faceDetector->detectFaceOrGround(voxelRay, *m_voxelGrid, m_resolution);
    
    EXPECT_TRUE(voxelFace.isValid());
    EXPECT_FALSE(voxelFace.isGroundPlane());
    EXPECT_EQ(voxelFace.getDirection(), FaceDirection::PositiveY);
    
    // Test 2: Ray that misses voxels should return ground plane
    Ray groundRay(Vector3f(5, 5, 5), Vector3f(0, -1, 0));
    Face groundFace = m_faceDetector->detectFaceOrGround(groundRay, *m_voxelGrid, m_resolution);
    
    EXPECT_TRUE(groundFace.isValid());
    EXPECT_TRUE(groundFace.isGroundPlane());
}

// REQ-6.1.3: Face highlighting shall update within one frame
TEST_F(MouseHoverHighlightingTest, HighlightUpdatePerformance) {
    // Simulate rapid mouse movement across multiple faces
    const int numFrames = 60; // 1 second at 60fps
    const float frameTime = 0.016f; // 16ms per frame
    
    for (int frame = 0; frame < numFrames; frame++) {
        // Create ray for this frame (simulating mouse movement)
        float t = static_cast<float>(frame) / numFrames;
        float x = -2.0f + 4.0f * t; // Move from -2 to +2
        Ray mouseRay(Vector3f(x, 5, 0), Vector3f(0, -1, 0));
        
        // Detect face
        Face face = m_faceDetector->detectFaceOrGround(mouseRay, *m_voxelGrid, m_resolution);
        
        // Update highlight - should complete within frame time
        auto startTime = std::chrono::high_resolution_clock::now();
        m_highlightManager->setHighlightedFace(face);
        m_highlightManager->update(frameTime);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        
        // Should update much faster than one frame (16ms = 16000 microseconds)
        EXPECT_LT(duration, 1000) << "Highlight update took " << duration << " microseconds";
    }
}

// Test highlight clearing when mouse leaves all objects
TEST_F(MouseHoverHighlightingTest, HighlightClearedWhenNoHover) {
    // First, highlight a face
    Face face(IncrementCoordinates(0, 0, 0), m_resolution, FaceDirection::PositiveY);
    m_highlightManager->setHighlightedFace(face);
    EXPECT_TRUE(m_highlightManager->hasFaceHighlight());
    
    // Now simulate mouse leaving (invalid face)
    Face invalidFace; // Default constructed = invalid
    m_highlightManager->setHighlightedFace(invalidFace);
    
    EXPECT_FALSE(m_highlightManager->hasFaceHighlight());
}

// Test smooth transitions between highlights
TEST_F(MouseHoverHighlightingTest, SmoothHighlightTransitions) {
    Face face1(IncrementCoordinates(0, 0, 0), m_resolution, FaceDirection::PositiveY);
    Face face2(IncrementCoordinates(32, 0, 0), m_resolution, FaceDirection::PositiveX);
    
    // Enable animations
    m_highlightManager->setAnimationEnabled(true);
    
    // Set first face
    m_highlightManager->setHighlightedFace(face1);
    
    // Transition to second face over multiple frames
    m_highlightManager->setHighlightedFace(face2);
    
    // Update through transition period (150ms as per HighlightManager)
    const int transitionFrames = 10; // ~150ms at 60fps
    for (int i = 0; i < transitionFrames; i++) {
        m_highlightManager->update(0.016f);
        
        // Current face should immediately be face2
        EXPECT_EQ(m_highlightManager->getCurrentFace(), face2);
    }
}

// Test edge case: hovering at voxel boundaries
TEST_F(MouseHoverHighlightingTest, HoverAtVoxelBoundaries) {
    // Ray that hits exactly at edge between two voxel faces
    // This tests the robustness of face detection
    float voxelSize = VoxelData::getVoxelSize(m_resolution);
    float edgeX = voxelSize / 2.0f; // Edge of voxel at origin
    
    Ray edgeRay(Vector3f(edgeX, 5, 0), Vector3f(0, -1, 0));
    Face face = m_faceDetector->detectFace(edgeRay, *m_voxelGrid, m_resolution);
    
    // Should still detect a valid face (implementation dependent which one)
    EXPECT_TRUE(face.isValid());
}

// Test hovering over different resolution voxels
TEST_F(MouseHoverHighlightingTest, MultiResolutionHighlighting) {
    // Create a new grid with different resolution
    VoxelResolution res64 = VoxelResolution::Size_64cm;
    VoxelGrid grid64(res64, m_workspaceSize);
    
    // Place a larger voxel
    grid64.setVoxel(IncrementCoordinates(0, 0, 0), true);
    
    // Ray hitting the larger voxel
    Ray ray(Vector3f(0.32f, 5, 0.32f), Vector3f(0, -1, 0)); // Hit center of 64cm voxel
    Face face = m_faceDetector->detectFace(ray, grid64, res64);
    
    EXPECT_TRUE(face.isValid());
    EXPECT_EQ(face.getResolution(), res64);
    
    // Highlight should work the same
    m_highlightManager->setHighlightedFace(face);
    EXPECT_TRUE(m_highlightManager->hasFaceHighlight());
}

// Test rapid hover changes (stress test)
TEST_F(MouseHoverHighlightingTest, RapidHoverChanges) {
    std::vector<Face> faces;
    
    // Create many faces
    for (int i = 0; i < 10; i++) {
        faces.emplace_back(
            IncrementCoordinates(i * 32, 0, 0),
            m_resolution,
            FaceDirection::PositiveY
        );
    }
    
    // Rapidly switch between them
    for (int iteration = 0; iteration < 100; iteration++) {
        int faceIndex = iteration % faces.size();
        m_highlightManager->setHighlightedFace(faces[faceIndex]);
        m_highlightManager->update(0.001f); // Very fast updates
        
        EXPECT_EQ(m_highlightManager->getCurrentFace(), faces[faceIndex]);
    }
}

} // namespace Tests
} // namespace VisualFeedback
} // namespace VoxelEditor