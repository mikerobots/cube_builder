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
        
        // Place initial voxel at (5,5,5) for testing
        voxelManager->setVoxel(
            Math::Vector3i(5, 5, 5), 
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
    
    // Helper to test placement position calculation
    Math::Vector3i calculatePlacementPosition(const Math::Vector3i& voxelPos, const Math::Vector3f& normal) {
        Math::Vector3i newPos = voxelPos;
        if (normal.x > 0.5f) newPos.x += 1;
        else if (normal.x < -0.5f) newPos.x -= 1;
        else if (normal.y > 0.5f) newPos.y += 1;
        else if (normal.y < -0.5f) newPos.y -= 1;
        else if (normal.z > 0.5f) newPos.z += 1;
        else if (normal.z < -0.5f) newPos.z -= 1;
        return newPos;
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
};

// Test clicking on each face of a voxel
TEST_F(FaceClickingTest, TestAllSixFaces) {
    float voxelSize = 0.64f;
    Math::Vector3f voxelCenter(5.0f * voxelSize + voxelSize * 0.5f,
                               5.0f * voxelSize + voxelSize * 0.5f,
                               5.0f * voxelSize + voxelSize * 0.5f);
    
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
        
        ASSERT_TRUE(face.isValid()) << "Failed to hit face for " << tc.description;
        EXPECT_EQ(face.getVoxelPosition(), Math::Vector3i(5, 5, 5)) 
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
        // Positive X face -> place at X+1
        {Math::Vector3i(5,5,5), Math::Vector3f(1,0,0), Math::Vector3i(6,5,5), "Place on +X"},
        
        // Negative X face -> place at X-1
        {Math::Vector3i(5,5,5), Math::Vector3f(-1,0,0), Math::Vector3i(4,5,5), "Place on -X"},
        
        // Positive Y face -> place at Y+1
        {Math::Vector3i(5,5,5), Math::Vector3f(0,1,0), Math::Vector3i(5,6,5), "Place on +Y"},
        
        // Negative Y face -> place at Y-1
        {Math::Vector3i(5,5,5), Math::Vector3f(0,-1,0), Math::Vector3i(5,4,5), "Place on -Y"},
        
        // Positive Z face -> place at Z+1
        {Math::Vector3i(5,5,5), Math::Vector3f(0,0,1), Math::Vector3i(5,5,6), "Place on +Z"},
        
        // Negative Z face -> place at Z-1
        {Math::Vector3i(5,5,5), Math::Vector3f(0,0,-1), Math::Vector3i(5,5,4), "Place on -Z"},
    };
    
    for (const auto& tc : testCases) {
        Math::Vector3i placement = calculatePlacementPosition(tc.voxelPos, tc.normal);
        
        EXPECT_EQ(placement, tc.expectedPlacement) 
            << "Failed placement for " << tc.description;
    }
}

// Test multiple voxel placement in a row
TEST_F(FaceClickingTest, TestSequentialVoxelPlacement) {
    // Start with voxel at (5,5,5)
    ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(5,5,5), VoxelData::VoxelResolution::Size_64cm));
    
    // Simulate clicking on positive X face and placing voxels
    for (int i = 1; i <= 3; ++i) {
        // Get the current rightmost voxel
        Math::Vector3i currentVoxel(5 + i - 1, 5, 5);
        ASSERT_TRUE(voxelManager->getVoxel(currentVoxel, VoxelData::VoxelResolution::Size_64cm))
            << "Voxel at " << currentVoxel.x << "," << currentVoxel.y << "," << currentVoxel.z << " should exist";
        
        // Calculate placement position for positive X face
        Math::Vector3i placement = calculatePlacementPosition(currentVoxel, Math::Vector3f(1, 0, 0));
        
        // Verify placement is correct
        EXPECT_EQ(placement.x, 5 + i);
        EXPECT_EQ(placement.y, 5);
        EXPECT_EQ(placement.z, 5);
        
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
    for (int x = 5; x <= 8; ++x) {
        EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(x, 5, 5), VoxelData::VoxelResolution::Size_64cm))
            << "Voxel at " << x << ",5,5 should exist";
    }
}

// Test edge cases
TEST_F(FaceClickingTest, TestEdgeCases) {
    // Test clicking on workspace boundaries
    voxelManager->setVoxel(Math::Vector3i(0, 5, 5), VoxelData::VoxelResolution::Size_64cm, true);
    
    // Try to place outside workspace (negative X)
    Math::Vector3i placement = calculatePlacementPosition(Math::Vector3i(0, 5, 5), Math::Vector3f(-1, 0, 0));
    
    // Should calculate position -1,5,5 but voxel manager should reject it
    EXPECT_EQ(placement.x, -1);
    EXPECT_EQ(placement.y, 5);
    EXPECT_EQ(placement.z, 5);
    
    // Verify voxel manager rejects out-of-bounds placement
    bool placed = voxelManager->setVoxel(
        placement,
        VoxelData::VoxelResolution::Size_64cm,
        true
    );
    EXPECT_FALSE(placed) << "Should not be able to place voxel outside workspace";
}

// Test that face detection works correctly with multiple voxels
TEST_F(FaceClickingTest, TestFaceDetectionWithMultipleVoxels) {
    // Place a line of voxels
    for (int x = 3; x <= 7; ++x) {
        voxelManager->setVoxel(Math::Vector3i(x, 5, 5), VoxelData::VoxelResolution::Size_64cm, true);
    }
    
    float voxelSize = 0.64f;
    
    // Test ray hitting the rightmost voxel
    Math::Vector3f rightVoxelCenter(7.0f * voxelSize + voxelSize * 0.5f,
                                   5.0f * voxelSize + voxelSize * 0.5f,
                                   5.0f * voxelSize + voxelSize * 0.5f);
    
    Math::Vector3f rayOrigin = rightVoxelCenter + Math::Vector3f(2.0f, 0, 0);
    Math::Vector3f rayTarget = rightVoxelCenter + Math::Vector3f(0.5f * voxelSize, 0, 0);
    Math::Vector3f direction = (rayTarget - rayOrigin).normalized();
    Math::Ray ray(rayOrigin, direction);
    
    VisualFeedback::Face face = detectFace(ray);
    ASSERT_TRUE(face.isValid());
    EXPECT_EQ(face.getVoxelPosition(), Math::Vector3i(7, 5, 5)) 
        << "Should hit the rightmost voxel";
    
    Math::Vector3f normal = face.getNormal();
    EXPECT_NEAR(normal.x, 1.0f, 0.01f);
    EXPECT_NEAR(normal.y, 0.0f, 0.01f);
    EXPECT_NEAR(normal.z, 0.0f, 0.01f);
}

} // namespace VoxelEditor