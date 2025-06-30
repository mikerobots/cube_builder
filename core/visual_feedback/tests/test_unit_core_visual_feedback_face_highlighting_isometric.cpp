#include <gtest/gtest.h>
#include <memory>
#include <cmath>
#include <set>
#include <chrono>
#include "../include/visual_feedback/FaceDetector.h"
#include "../include/visual_feedback/HighlightManager.h"
#include "../include/visual_feedback/FeedbackTypes.h"
#include "../../voxel_data/VoxelGrid.h"
#include "../../../foundation/math/Ray.h"

namespace VoxelEditor {
namespace VisualFeedback {
namespace Tests {

using namespace Math;
using namespace VoxelData;

class FaceHighlightingIsometricTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_faceDetector = std::make_unique<FaceDetector>();
        m_highlightManager = std::make_unique<HighlightManager>();
        m_resolution = VoxelResolution::Size_32cm;
        m_workspaceSize = Math::Vector3f(5.0f, 5.0f, 5.0f);
        m_voxelGrid = std::make_unique<VoxelGrid>(m_resolution, m_workspaceSize);
        
        setupTestScene();
    }
    
    void setupTestScene() {
        // Create a test scene with voxels that would be visible in isometric view
        // In isometric view, we typically see top, right, and front faces
        
        // Single voxel at origin
        m_voxelGrid->setVoxel(IncrementCoordinates(0, 0, 0), true);
        
        // Voxel to the right
        m_voxelGrid->setVoxel(IncrementCoordinates(32, 0, 0), true);
        
        // Voxel above
        m_voxelGrid->setVoxel(IncrementCoordinates(0, 32, 0), true);
        
        // Voxel in front
        m_voxelGrid->setVoxel(IncrementCoordinates(0, 0, 32), true);
        
        // Create a small platform to test multiple faces
        for (int x = 64; x <= 128; x += 32) {
            for (int z = 64; z <= 128; z += 32) {
                m_voxelGrid->setVoxel(IncrementCoordinates(x, 0, z), true);
            }
        }
    }
    
    // Create ray from isometric viewing angle
    Ray createIsometricRay(const Vector2f& screenPos, const Vector2f& screenSize) {
        // Classic isometric angle: camera is positioned at 35.264° elevation, 45° rotation
        const float isoElevation = 35.264f * M_PI / 180.0f;
        const float isoRotation = 45.0f * M_PI / 180.0f;
        
        // Convert screen position to normalized coordinates (-1 to 1)
        float nx = (screenPos.x / screenSize.x) * 2.0f - 1.0f;
        float ny = 1.0f - (screenPos.y / screenSize.y) * 2.0f;
        
        // Camera is looking from upper-right-front towards origin
        // For simplicity, we'll create rays that simulate this view
        Vector3f cameraPos(2.0f, 2.0f, 2.0f);
        
        // Create a ray direction based on screen position
        // This is a simplified version - in real implementation would use proper projection
        Vector3f rayDir(-1.0f + nx * 0.5f, -1.0f + ny * 0.5f, -1.0f);
        rayDir.normalize();
        
        return Ray(cameraPos, rayDir);
    }
    
    // Create ray that simulates different camera angles
    Ray createRayFromAngle(const Vector3f& targetPoint, float azimuth, float elevation) {
        float distance = 3.0f;
        
        float x = targetPoint.x + distance * cos(elevation) * sin(azimuth);
        float y = targetPoint.y + distance * sin(elevation);
        float z = targetPoint.z + distance * cos(elevation) * cos(azimuth);
        
        Vector3f origin(x, y, z);
        Vector3f direction = targetPoint - origin;
        direction.normalize();
        
        return Ray(origin, direction);
    }
    
    std::unique_ptr<FaceDetector> m_faceDetector;
    std::unique_ptr<HighlightManager> m_highlightManager;
    std::unique_ptr<VoxelGrid> m_voxelGrid;
    VoxelResolution m_resolution;
    Math::Vector3f m_workspaceSize;
};

// REQ-4.2.3: Test that highlighting works from isometric viewing angle
TEST_F(FaceHighlightingIsometricTest, IsometricViewBasicHighlighting) {
    Vector2f screenSize(800, 600);
    
    // Test center of screen
    Ray ray = createIsometricRay(Vector2f(400, 300), screenSize);
    Face detectedFace = m_faceDetector->detectFace(ray, *m_voxelGrid, m_resolution);
    
    EXPECT_TRUE(detectedFace.isValid()) << "Should detect a face from isometric view";
    
    if (detectedFace.isValid()) {
        // In isometric view, we expect to see top, right, or front faces
        FaceDirection dir = detectedFace.getDirection();
        bool isExpectedFace = (dir == FaceDirection::PositiveY ||  // Top
                              dir == FaceDirection::PositiveX ||  // Right  
                              dir == FaceDirection::PositiveZ);   // Front
        
        EXPECT_TRUE(isExpectedFace) 
            << "Face direction " << static_cast<int>(dir) 
            << " is not typically visible in isometric view";
        
        // Test highlighting
        m_highlightManager->setHighlightedFace(detectedFace);
        EXPECT_TRUE(m_highlightManager->hasFaceHighlight());
        EXPECT_EQ(m_highlightManager->getCurrentFace(), detectedFace);
    }
}

// Test face detection from multiple isometric-like angles
TEST_F(FaceHighlightingIsometricTest, MultipleIsometricAngles) {
    // Test variations of isometric angles
    struct AngleTest {
        float azimuth;
        float elevation;
        std::string description;
    };
    
    AngleTest angles[] = {
        {45.0f * M_PI / 180.0f, 35.264f * M_PI / 180.0f, "Classic isometric"},
        {45.0f * M_PI / 180.0f, 30.0f * M_PI / 180.0f, "Lower isometric"},
        {45.0f * M_PI / 180.0f, 40.0f * M_PI / 180.0f, "Higher isometric"},
        {30.0f * M_PI / 180.0f, 35.264f * M_PI / 180.0f, "Rotated left"},
        {60.0f * M_PI / 180.0f, 35.264f * M_PI / 180.0f, "Rotated right"},
    };
    
    Vector3f targetVoxel(0.16f, 0.16f, 0.16f); // Center of voxel at origin
    
    for (const auto& angle : angles) {
        Ray ray = createRayFromAngle(targetVoxel, angle.azimuth, angle.elevation);
        Face face = m_faceDetector->detectFace(ray, *m_voxelGrid, m_resolution);
        
        if (face.isValid()) {
            m_highlightManager->setHighlightedFace(face);
            EXPECT_TRUE(m_highlightManager->hasFaceHighlight()) 
                << "Failed to highlight from " << angle.description;
            
            // The face should remain highlighted
            m_highlightManager->update(0.016f);
            EXPECT_TRUE(m_highlightManager->hasFaceHighlight());
        }
    }
}

// Test highlighting different faces on a platform from isometric view
TEST_F(FaceHighlightingIsometricTest, PlatformFaceHighlighting) {
    Vector2f screenSize(800, 600);
    
    // Test different screen positions to hit different faces of the platform
    struct ScreenTest {
        Vector2f pos;
        std::string description;
    };
    
    ScreenTest positions[] = {
        {{300, 250}, "Upper left - should hit top face"},
        {{500, 250}, "Upper right - might hit right face"},
        {{400, 350}, "Lower center - might hit front face"},
        {{350, 300}, "Left center"},
        {{450, 300}, "Right center"}
    };
    
    int detectedCount = 0;
    std::set<FaceDirection> detectedDirections;
    
    for (const auto& test : positions) {
        Ray ray = createIsometricRay(test.pos, screenSize);
        Face face = m_faceDetector->detectFace(ray, *m_voxelGrid, m_resolution);
        
        if (face.isValid()) {
            detectedCount++;
            detectedDirections.insert(face.getDirection());
            
            m_highlightManager->setHighlightedFace(face);
            EXPECT_TRUE(m_highlightManager->hasFaceHighlight())
                << "Failed to highlight at " << test.description;
        }
    }
    
    // Should detect multiple faces from different positions
    EXPECT_GT(detectedCount, 2) << "Too few faces detected from isometric view";
    
    // Should see variety of face directions
    EXPECT_GT(detectedDirections.size(), 1) 
        << "Should detect different face orientations from isometric view";
}

// REQ-4.2.1: Test yellow highlighting color is maintained in isometric view
TEST_F(FaceHighlightingIsometricTest, HighlightColorInIsometric) {
    Ray ray = createIsometricRay(Vector2f(400, 300), Vector2f(800, 600));
    Face face = m_faceDetector->detectFace(ray, *m_voxelGrid, m_resolution);
    
    if (face.isValid()) {
        Rendering::Color highlightColor = m_faceDetector->getFaceHighlightColor(face);
        
        // Should be yellow
        EXPECT_NEAR(highlightColor.r, 1.0f, 0.1f);
        EXPECT_NEAR(highlightColor.g, 1.0f, 0.1f);
        EXPECT_NEAR(highlightColor.b, 0.0f, 0.1f);
        EXPECT_GT(highlightColor.a, 0.0f);
    }
}

// REQ-4.2.2: Test only one face highlighted at a time in isometric view
TEST_F(FaceHighlightingIsometricTest, SingleFaceHighlightingIsometric) {
    Vector2f screenSize(800, 600);
    
    // Detect first face
    Ray ray1 = createIsometricRay(Vector2f(300, 300), screenSize);
    Face face1 = m_faceDetector->detectFace(ray1, *m_voxelGrid, m_resolution);
    
    // Detect second face from different position
    Ray ray2 = createIsometricRay(Vector2f(500, 300), screenSize);
    Face face2 = m_faceDetector->detectFace(ray2, *m_voxelGrid, m_resolution);
    
    if (face1.isValid() && face2.isValid() && face1 != face2) {
        // Highlight first face
        m_highlightManager->setHighlightedFace(face1);
        EXPECT_EQ(m_highlightManager->getCurrentFace(), face1);
        
        // Highlight second face - first should no longer be highlighted
        m_highlightManager->setHighlightedFace(face2);
        EXPECT_EQ(m_highlightManager->getCurrentFace(), face2);
        EXPECT_NE(m_highlightManager->getCurrentFace(), face1);
    }
}

// Test edge detection at voxel boundaries in isometric view
TEST_F(FaceHighlightingIsometricTest, EdgeDetectionIsometric) {
    // Place a single voxel for precise testing
    m_voxelGrid->clear();
    m_voxelGrid->setVoxel(IncrementCoordinates(100, 100, 100), true);
    
    // In isometric view, voxel edges appear as diagonal lines
    // Test rays near edges
    Vector3f voxelCenter(1.0f, 1.0f, 1.0f);
    float voxelSize = 0.32f;
    
    // Test near top-right edge (visible in isometric)
    Vector3f edgePoint = voxelCenter + Vector3f(voxelSize/2, voxelSize/2, 0);
    Ray edgeRay = createRayFromAngle(edgePoint, 45.0f * M_PI / 180.0f, 35.0f * M_PI / 180.0f);
    
    Face edgeFace = m_faceDetector->detectFace(edgeRay, *m_voxelGrid, m_resolution);
    if (edgeFace.isValid()) {
        m_highlightManager->setHighlightedFace(edgeFace);
        EXPECT_TRUE(m_highlightManager->hasFaceHighlight()) 
            << "Should be able to highlight faces near edges in isometric view";
    }
}

// Performance test for isometric face detection
TEST_F(FaceHighlightingIsometricTest, IsometricPerformance) {
    Vector2f screenSize(800, 600);
    const int numTests = 100;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numTests; i++) {
        // Vary screen position
        float x = 200 + (i % 10) * 40;
        float y = 200 + (i / 10) * 20;
        
        Ray ray = createIsometricRay(Vector2f(x, y), screenSize);
        Face face = m_faceDetector->detectFace(ray, *m_voxelGrid, m_resolution);
        
        if (face.isValid()) {
            m_highlightManager->setHighlightedFace(face);
            m_highlightManager->update(0.001f); // Minimal update
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should complete 100 detections quickly
    EXPECT_LT(duration, 50) << "Isometric face detection too slow: " << duration << "ms for " << numTests << " tests";
}

// Test highlight persistence during simulated camera movement
TEST_F(FaceHighlightingIsometricTest, HighlightPersistenceDuringMovement) {
    // Start with standard isometric angle
    Vector3f target(0.16f, 0.16f, 0.16f);
    Ray initialRay = createRayFromAngle(target, 45.0f * M_PI / 180.0f, 35.264f * M_PI / 180.0f);
    
    Face initialFace = m_faceDetector->detectFace(initialRay, *m_voxelGrid, m_resolution);
    ASSERT_TRUE(initialFace.isValid());
    
    m_highlightManager->setHighlightedFace(initialFace);
    
    // Simulate small camera movements (as would happen with mouse movement)
    for (int i = 0; i < 10; i++) {
        float angleOffset = (i - 5) * 2.0f * M_PI / 180.0f; // ±10 degrees
        
        Ray movedRay = createRayFromAngle(target, 
                                         45.0f * M_PI / 180.0f + angleOffset,
                                         35.264f * M_PI / 180.0f);
        
        Face newFace = m_faceDetector->detectFace(movedRay, *m_voxelGrid, m_resolution);
        
        // Update highlight if we detect a face
        if (newFace.isValid()) {
            m_highlightManager->setHighlightedFace(newFace);
        }
        
        // Highlight should always be active during movement
        EXPECT_TRUE(m_highlightManager->hasFaceHighlight()) 
            << "Lost highlight during camera movement at offset " << angleOffset;
        
        m_highlightManager->update(0.016f);
    }
}

} // namespace Tests
} // namespace VisualFeedback
} // namespace VoxelEditor