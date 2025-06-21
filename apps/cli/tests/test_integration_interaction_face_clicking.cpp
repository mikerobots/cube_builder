#include <gtest/gtest.h>
#include <memory>
#include "cli/MouseInteraction.h"
#include "voxel_data/VoxelDataManager.h"
#include "visual_feedback/FaceDetector.h"
#include "visual_feedback/FeedbackTypes.h"
#include "math/Ray.h"
#include "math/Vector3f.h"

namespace VoxelEditor {

// Test fixture that tests face detection and placement calculation directly
class FaceClickingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create event dispatcher
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        
        // Create voxel manager
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>(eventDispatcher.get());
        voxelManager->resizeWorkspace(Math::Vector3f(8.0f, 8.0f, 8.0f));
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_64cm);
        
        // Place initial voxel near origin for testing (centered coordinate system)
        // For 64cm voxels, we need to snap to the 64cm grid
        Math::IncrementCoordinates desiredPos(0, 64, 0); // 64cm up from ground
        Math::IncrementCoordinates snappedPos = Math::CoordinateConverter::snapToVoxelResolution(
            desiredPos, VoxelData::VoxelResolution::Size_64cm);
        
        voxelManager->setVoxel(
            snappedPos.value(), 
            VoxelData::VoxelResolution::Size_64cm, 
            true
        );
    }
    
    void TearDown() override {
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    // Helper to detect face hit by ray
    VisualFeedback::Face detectFace(const Math::Ray& ray) {
        VisualFeedback::FaceDetector detector;
        VisualFeedback::Ray vfRay(ray.origin, ray.direction);
        
        const VoxelData::VoxelGrid* grid = voxelManager->getGrid(
            voxelManager->getActiveResolution()
        );
        
        return detector.detectFace(
            vfRay, 
            *grid, 
            voxelManager->getActiveResolution()
        );
    }
    
    // Helper to test placement position calculation for 64cm voxels
    Math::Vector3i calculatePlacementPosition(const Math::Vector3i& voxelPos, const Math::Vector3f& normal) {
        Math::Vector3i newPos = voxelPos;
        int voxelSize_cm = 64; // 64cm voxels
        if (normal.x > 0.5f) newPos.x += voxelSize_cm;
        else if (normal.x < -0.5f) newPos.x -= voxelSize_cm;
        else if (normal.y > 0.5f) newPos.y += voxelSize_cm;
        else if (normal.y < -0.5f) newPos.y -= voxelSize_cm;
        else if (normal.z > 0.5f) newPos.z += voxelSize_cm;
        else if (normal.z < -0.5f) newPos.z -= voxelSize_cm;
        return newPos;
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
};

// Test clicking on each face of a voxel
TEST_F(FaceClickingTest, TestAllSixFaces) {
    float voxelSize = 0.64f;
    
    // Verify the voxel was actually placed at the snapped position
    Math::IncrementCoordinates snappedPos(0, 64, 0);
    ASSERT_TRUE(voxelManager->hasVoxel(snappedPos.value(), VoxelData::VoxelResolution::Size_64cm))
        << "Voxel should be present at (0,64,0)";
    
    // In centered coordinate system, use coordinate converter to get world position
    Math::WorldCoordinates voxelWorldPos = Math::CoordinateConverter::incrementToWorld(snappedPos);
    Math::Vector3f voxelCenter = voxelWorldPos.value() + Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    
    // Test rays from different directions
    struct TestCase {
        Math::Vector3f rayOrigin;
        Math::Vector3f rayTarget;
        Math::Vector3f expectedNormal;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        // Positive X: ray from right side
        {voxelCenter + Math::Vector3f(2.0f, 0, 0), 
         voxelCenter + Math::Vector3f(0.5f * voxelSize, 0, 0),
         Math::Vector3f(1, 0, 0), "Positive X face"},
        
        // Negative X: ray from left side
        {voxelCenter - Math::Vector3f(2.0f, 0, 0),
         voxelCenter - Math::Vector3f(0.5f * voxelSize, 0, 0),
         Math::Vector3f(-1, 0, 0), "Negative X face"},
        
        // Positive Y: ray from above
        {voxelCenter + Math::Vector3f(0, 2.0f, 0),
         voxelCenter + Math::Vector3f(0, 0.5f * voxelSize, 0),
         Math::Vector3f(0, 1, 0), "Positive Y face"},
        
        // Negative Y: ray from below
        {voxelCenter - Math::Vector3f(0, 2.0f, 0),
         voxelCenter - Math::Vector3f(0, 0.5f * voxelSize, 0),
         Math::Vector3f(0, -1, 0), "Negative Y face"},
        
        // Positive Z: ray from front
        {voxelCenter + Math::Vector3f(0, 0, 2.0f),
         voxelCenter + Math::Vector3f(0, 0, 0.5f * voxelSize),
         Math::Vector3f(0, 0, 1), "Positive Z face"},
        
        // Negative Z: ray from back
        {voxelCenter - Math::Vector3f(0, 0, 2.0f),
         voxelCenter - Math::Vector3f(0, 0, 0.5f * voxelSize),
         Math::Vector3f(0, 0, -1), "Negative Z face"}
    };
    
    for (const auto& tc : testCases) {
        Math::Vector3f direction = (tc.rayTarget - tc.rayOrigin).normalized();
        Math::Ray ray(tc.rayOrigin, direction);
        VisualFeedback::Face face = detectFace(ray);
        
        // Debug logging
        if (!face.isValid()) {
            printf("DEBUG: Face not valid for %s\n", tc.description.c_str());
            printf("  Ray origin: (%.2f, %.2f, %.2f)\n", tc.rayOrigin.x, tc.rayOrigin.y, tc.rayOrigin.z);
            printf("  Ray direction: (%.2f, %.2f, %.2f)\n", direction.x, direction.y, direction.z);
        } else {
            auto voxelPos = face.getVoxelPosition();
            printf("DEBUG: Face detected for %s\n", tc.description.c_str());
            printf("  Voxel position: (%d, %d, %d)\n", voxelPos.x(), voxelPos.y(), voxelPos.z());
            printf("  Expected: (0, 64, 0)\n");
        }
        
        ASSERT_TRUE(face.isValid()) << "Failed to hit face for " << tc.description;
        EXPECT_EQ(face.getVoxelPosition(), Math::IncrementCoordinates(0, 64, 0)) 
            << "Wrong voxel hit for " << tc.description;
        
        Math::Vector3f normal = face.getNormal();
        EXPECT_NEAR(normal.x, tc.expectedNormal.x, 0.01f) << tc.description;
        EXPECT_NEAR(normal.y, tc.expectedNormal.y, 0.01f) << tc.description;
        EXPECT_NEAR(normal.z, tc.expectedNormal.z, 0.01f) << tc.description;
    }
}

// Test voxel placement calculation
TEST_F(FaceClickingTest, TestVoxelPlacementCalculation) {
    struct TestCase {
        Math::Vector3i voxelPos;
        Math::Vector3f normal;
        Math::Vector3i expectedPlacement;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        // Positive X face -> place at X+64 (64cm voxel)
        {Math::Vector3i(0,64,0), Math::Vector3f(1,0,0), Math::Vector3i(64,64,0), "Place on +X"},
        
        // Negative X face -> place at X-64
        {Math::Vector3i(0,64,0), Math::Vector3f(-1,0,0), Math::Vector3i(-64,64,0), "Place on -X"},
        
        // Positive Y face -> place at Y+64
        {Math::Vector3i(0,64,0), Math::Vector3f(0,1,0), Math::Vector3i(0,128,0), "Place on +Y"},
        
        // Negative Y face -> place at Y-64
        {Math::Vector3i(0,64,0), Math::Vector3f(0,-1,0), Math::Vector3i(0,0,0), "Place on -Y"},
        
        // Positive Z face -> place at Z+64
        {Math::Vector3i(0,64,0), Math::Vector3f(0,0,1), Math::Vector3i(0,64,64), "Place on +Z"},
        
        // Negative Z face -> place at Z-64
        {Math::Vector3i(0,64,0), Math::Vector3f(0,0,-1), Math::Vector3i(0,64,-64), "Place on -Z"},
    };
    
    for (const auto& tc : testCases) {
        Math::Vector3i placement = calculatePlacementPosition(tc.voxelPos, tc.normal);
        
        EXPECT_EQ(placement, tc.expectedPlacement) 
            << "Failed placement for " << tc.description;
    }
}

// Test multiple voxel placement in a row
TEST_F(FaceClickingTest, TestSequentialVoxelPlacement) {
    // Start with voxel at (0,64,0)
    ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(0,64,0), VoxelData::VoxelResolution::Size_64cm));
    
    // Simulate clicking on positive X face and placing voxels
    for (int i = 1; i <= 3; ++i) {
        // Get the current rightmost voxel
        Math::Vector3i currentVoxel((i - 1) * 64, 64, 0);
        ASSERT_TRUE(voxelManager->getVoxel(currentVoxel, VoxelData::VoxelResolution::Size_64cm))
            << "Voxel at " << currentVoxel.x << "," << currentVoxel.y << "," << currentVoxel.z << " should exist";
        
        // Calculate placement position for positive X face
        Math::Vector3i placement = calculatePlacementPosition(currentVoxel, Math::Vector3f(1, 0, 0));
        
        // Verify placement is correct
        EXPECT_EQ(placement.x, i * 64);
        EXPECT_EQ(placement.y, 64);
        EXPECT_EQ(placement.z, 0);
        
        // Check if position is valid before placing
        bool isValid = voxelManager->isValidPosition(placement, VoxelData::VoxelResolution::Size_64cm);
        ASSERT_TRUE(isValid) << "Position " << placement.x << "," << placement.y << "," << placement.z 
                             << " is not valid (iteration " << i << ")";
        
        // Place the voxel
        bool placed = voxelManager->setVoxel(
            placement,
            VoxelData::VoxelResolution::Size_64cm,
            true
        );
        
        // Verify it was placed
        ASSERT_TRUE(placed) << "Failed to place voxel at " << placement.x << "," << placement.y 
                            << "," << placement.z << " (iteration " << i << ")";
        ASSERT_TRUE(voxelManager->getVoxel(placement, VoxelData::VoxelResolution::Size_64cm))
            << "Failed to get voxel at iteration " << i;
    }
    
    // Verify we have a row of 4 voxels
    for (int i = 0; i <= 3; ++i) {
        EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(i * 64, 64, 0), VoxelData::VoxelResolution::Size_64cm))
            << "Voxel at " << (i * 64) << ",64,0 should exist";
    }
}

// Test edge cases
TEST_F(FaceClickingTest, TestEdgeCases) {
    // Test placement calculation with negative coordinates (valid in centered system)
    // Place at -64,64,-64 (one voxel left and back from center)
    voxelManager->setVoxel(Math::Vector3i(-64, 64, -64), VoxelData::VoxelResolution::Size_64cm, true);
    
    // Try to place on negative X face (should work in centered system)
    Math::Vector3i placement = calculatePlacementPosition(Math::Vector3i(-64, 64, -64), Math::Vector3f(-1, 0, 0));
    
    // Should calculate position -128,64,-64
    EXPECT_EQ(placement.x, -128);
    EXPECT_EQ(placement.y, 64);
    EXPECT_EQ(placement.z, -64);
    
    // Verify this position is within workspace bounds (8x8x8 centered at origin)
    bool isValid = voxelManager->isValidPosition(placement, VoxelData::VoxelResolution::Size_64cm);
    EXPECT_TRUE(isValid) << "Position within workspace should be valid in centered coordinate system";
}

// Test that face detection works correctly with multiple voxels
TEST_F(FaceClickingTest, TestFaceDetectionWithMultipleVoxels) {
    // Place a line of voxels centered around origin (using 64cm grid)
    for (int i = -2; i <= 2; ++i) {
        voxelManager->setVoxel(Math::Vector3i(i * 64, 64, 0), VoxelData::VoxelResolution::Size_64cm, true);
    }
    
    float voxelSize = 0.64f;
    
    // Test ray hitting the rightmost voxel (at x=128)
    Math::WorldCoordinates rightVoxelWorldPos = Math::CoordinateConverter::incrementToWorld(Math::IncrementCoordinates(128, 64, 0));
    Math::Vector3f rightVoxelCenter = rightVoxelWorldPos.value() + Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    
    Math::Vector3f rayOrigin = rightVoxelCenter + Math::Vector3f(2.0f, 0, 0);
    Math::Vector3f rayTarget = rightVoxelCenter + Math::Vector3f(0.5f * voxelSize, 0, 0);
    Math::Vector3f direction = (rayTarget - rayOrigin).normalized();
    Math::Ray ray(rayOrigin, direction);
    
    VisualFeedback::Face face = detectFace(ray);
    ASSERT_TRUE(face.isValid());
    EXPECT_EQ(face.getVoxelPosition(), Math::IncrementCoordinates(128, 64, 0)) 
        << "Should hit the rightmost voxel";
    
    Math::Vector3f normal = face.getNormal();
    EXPECT_NEAR(normal.x, 1.0f, 0.01f);
    EXPECT_NEAR(normal.y, 0.0f, 0.01f);
    EXPECT_NEAR(normal.z, 0.0f, 0.01f);
}

} // namespace VoxelEditor