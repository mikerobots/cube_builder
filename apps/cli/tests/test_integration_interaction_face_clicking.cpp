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
        // With new requirements, 64cm voxels can be placed at any 1cm position
        // Test with a non-aligned position (7cm, 23cm up from ground) to validate new requirements
        Math::IncrementCoordinates exactPos(7, 23, 11); // Non-aligned position at 1cm increments
        
        voxelManager->setVoxel(
            exactPos.value(), 
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
    
    // Verify the voxel was actually placed at the exact position (non-aligned)
    Math::IncrementCoordinates exactPos(7, 23, 11);
    bool hasVoxel = voxelManager->hasVoxel(exactPos.value(), VoxelData::VoxelResolution::Size_64cm);
    if (!hasVoxel) {
        // Check if it was placed at a snapped position instead
        for (int x = 0; x <= 64; x += 64) {
            for (int y = 0; y <= 64; y += 64) {
                for (int z = 0; z <= 64; z += 64) {
                    if (voxelManager->hasVoxel(Math::Vector3i(x, y, z), VoxelData::VoxelResolution::Size_64cm)) {
                        printf("DEBUG: Found voxel at aligned position (%d,%d,%d) instead of (7,23,11)\n", x, y, z);
                    }
                }
            }
        }
    }
    ASSERT_TRUE(hasVoxel)
        << "Voxel should be present at exact non-aligned position (7,23,11)";
    
    // In centered coordinate system, use coordinate converter to get world position
    Math::WorldCoordinates voxelWorldPos = Math::CoordinateConverter::incrementToWorld(exactPos);
    Math::Vector3f voxelCenter = voxelWorldPos.value() + Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    
    printf("DEBUG: Voxel at increment pos (7,23,11)\n");
    printf("DEBUG: Voxel world pos: (%.3f, %.3f, %.3f)\n", voxelWorldPos.value().x, voxelWorldPos.value().y, voxelWorldPos.value().z);
    printf("DEBUG: Voxel center: (%.3f, %.3f, %.3f)\n", voxelCenter.x, voxelCenter.y, voxelCenter.z);
    printf("DEBUG: Voxel size: %.3f\n", voxelSize);
    
    // Test rays from different directions
    struct TestCase {
        Math::Vector3f rayOrigin;
        Math::Vector3f rayTarget;
        Math::Vector3f expectedNormal;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        // Positive X: ray from right side hitting positive X face
        {voxelCenter + Math::Vector3f(2.0f, 0, 0), 
         voxelCenter,
         Math::Vector3f(1, 0, 0), "Positive X face"},
        
        // Negative X: ray from left side hitting negative X face
        {voxelCenter - Math::Vector3f(2.0f, 0, 0),
         voxelCenter,
         Math::Vector3f(-1, 0, 0), "Negative X face"},
        
        // Positive Y: ray from above hitting positive Y face
        {voxelCenter + Math::Vector3f(0, 2.0f, 0),
         voxelCenter,
         Math::Vector3f(0, 1, 0), "Positive Y face"},
        
        // Negative Y: ray from below hitting negative Y face
        {voxelCenter - Math::Vector3f(0, 2.0f, 0),
         voxelCenter,
         Math::Vector3f(0, -1, 0), "Negative Y face"},
        
        // Positive Z: ray from front hitting positive Z face
        {voxelCenter + Math::Vector3f(0, 0, 2.0f),
         voxelCenter,
         Math::Vector3f(0, 0, 1), "Positive Z face"},
        
        // Negative Z: ray from back hitting negative Z face
        {voxelCenter - Math::Vector3f(0, 0, 2.0f),
         voxelCenter,
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
            printf("  Expected: (7, 23, 11)\n");
        }
        
        ASSERT_TRUE(face.isValid()) << "Failed to hit face for " << tc.description;
        EXPECT_EQ(face.getVoxelPosition(), Math::IncrementCoordinates(7, 23, 11)) 
            << "Wrong voxel hit for " << tc.description 
            << " - should detect non-aligned voxel at exact position";
        
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
        // Test with non-aligned voxel position (7,23,11) to validate new requirements
        // Positive X face -> place at X+64 (64cm voxel)
        {Math::Vector3i(7,23,11), Math::Vector3f(1,0,0), Math::Vector3i(71,23,11), "Place on +X from non-aligned voxel"},
        
        // Negative X face -> place at X-64
        {Math::Vector3i(7,23,11), Math::Vector3f(-1,0,0), Math::Vector3i(-57,23,11), "Place on -X from non-aligned voxel"},
        
        // Positive Y face -> place at Y+64
        {Math::Vector3i(7,23,11), Math::Vector3f(0,1,0), Math::Vector3i(7,87,11), "Place on +Y from non-aligned voxel"},
        
        // Negative Y face -> place at Y-64
        {Math::Vector3i(7,23,11), Math::Vector3f(0,-1,0), Math::Vector3i(7,-41,11), "Place on -Y from non-aligned voxel"},
        
        // Positive Z face -> place at Z+64
        {Math::Vector3i(7,23,11), Math::Vector3f(0,0,1), Math::Vector3i(7,23,75), "Place on +Z from non-aligned voxel"},
        
        // Negative Z face -> place at Z-64
        {Math::Vector3i(7,23,11), Math::Vector3f(0,0,-1), Math::Vector3i(7,23,-53), "Place on -Z from non-aligned voxel"},
    };
    
    for (const auto& tc : testCases) {
        Math::Vector3i placement = calculatePlacementPosition(tc.voxelPos, tc.normal);
        
        EXPECT_EQ(placement, tc.expectedPlacement) 
            << "Failed placement for " << tc.description;
    }
}

// Test multiple voxel placement in a row
TEST_F(FaceClickingTest, TestSequentialVoxelPlacement) {
    // Start with voxel at non-aligned position (7,23,11)
    ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(7,23,11), VoxelData::VoxelResolution::Size_64cm));
    
    // Simulate clicking on positive X face and placing voxels
    for (int i = 1; i <= 3; ++i) {
        // Get the current rightmost voxel starting from non-aligned position
        Math::Vector3i currentVoxel(7 + (i - 1) * 64, 23, 11);
        ASSERT_TRUE(voxelManager->getVoxel(currentVoxel, VoxelData::VoxelResolution::Size_64cm))
            << "Voxel at " << currentVoxel.x << "," << currentVoxel.y << "," << currentVoxel.z << " should exist";
        
        // Calculate placement position for positive X face
        Math::Vector3i placement = calculatePlacementPosition(currentVoxel, Math::Vector3f(1, 0, 0));
        
        // Verify placement is correct (maintains non-aligned Y and Z coordinates)
        EXPECT_EQ(placement.x, 7 + i * 64);
        EXPECT_EQ(placement.y, 23);
        EXPECT_EQ(placement.z, 11);
        
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
    
    // Verify we have a row of 4 voxels in non-aligned positions
    for (int i = 0; i <= 3; ++i) {
        EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(7 + i * 64, 23, 11), VoxelData::VoxelResolution::Size_64cm))
            << "Voxel at " << (7 + i * 64) << ",23,11 should exist";
    }
}

// Test edge cases with non-aligned voxels
TEST_F(FaceClickingTest, TestEdgeCases) {
    // Test placement calculation with negative coordinates and non-aligned positions (valid in centered system)
    // Place at arbitrary non-aligned position: -37cm, 19cm, -23cm
    Math::Vector3i nonAlignedPos(-37, 19, -23);
    voxelManager->setVoxel(nonAlignedPos, VoxelData::VoxelResolution::Size_64cm, true);
    
    // Try to place on negative X face (should work in centered system)
    Math::Vector3i placement = calculatePlacementPosition(nonAlignedPos, Math::Vector3f(-1, 0, 0));
    
    // Should calculate position -37-64, 19, -23 = -101, 19, -23
    EXPECT_EQ(placement.x, -101);
    EXPECT_EQ(placement.y, 19);
    EXPECT_EQ(placement.z, -23);
    
    // Verify this position is within workspace bounds (8x8x8 centered at origin)
    bool isValid = voxelManager->isValidPosition(placement, VoxelData::VoxelResolution::Size_64cm);
    EXPECT_TRUE(isValid) << "Non-aligned position within workspace should be valid in centered coordinate system";
}

// Test that face detection works correctly with multiple non-aligned voxels
TEST_F(FaceClickingTest, TestFaceDetectionWithMultipleVoxels) {
    // Place a line of voxels at non-aligned positions to test new requirements
    for (int i = -2; i <= 2; ++i) {
        // Use non-aligned positions: varying X and slightly offset Y/Z
        Math::Vector3i nonAlignedPos(13 + i * 67, 29, 5);  // Arbitrary 1cm positions
        voxelManager->setVoxel(nonAlignedPos, VoxelData::VoxelResolution::Size_64cm, true);
    }
    
    float voxelSize = 0.64f;
    
    // Test ray hitting the rightmost voxel (at x=13+2*67=147)
    Math::IncrementCoordinates rightVoxelPos(13 + 2 * 67, 29, 5);
    Math::WorldCoordinates rightVoxelWorldPos = Math::CoordinateConverter::incrementToWorld(rightVoxelPos);
    Math::Vector3f rightVoxelCenter = rightVoxelWorldPos.value() + Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    
    Math::Vector3f rayOrigin = rightVoxelCenter + Math::Vector3f(2.0f, 0, 0);
    Math::Vector3f rayTarget = rightVoxelCenter + Math::Vector3f(0.5f * voxelSize, 0, 0);
    Math::Vector3f direction = (rayTarget - rayOrigin).normalized();
    Math::Ray ray(rayOrigin, direction);
    
    VisualFeedback::Face face = detectFace(ray);
    ASSERT_TRUE(face.isValid()) << "Should detect face on non-aligned voxel";
    EXPECT_EQ(face.getVoxelPosition(), rightVoxelPos) 
        << "Should hit the rightmost non-aligned voxel at exact position";
    
    Math::Vector3f normal = face.getNormal();
    EXPECT_NEAR(normal.x, 1.0f, 0.01f);
    EXPECT_NEAR(normal.y, 0.0f, 0.01f);
    EXPECT_NEAR(normal.z, 0.0f, 0.01f);
}

// Test that face detection works with different voxel sizes at non-aligned positions
TEST_F(FaceClickingTest, TestNonAlignedVoxelMixedSizes) {
    // Place voxels of different sizes at non-aligned 1cm positions
    // This tests the requirements that all voxel sizes work at arbitrary 1cm positions
    
    // 16cm voxel at non-aligned position
    Math::IncrementCoordinates pos16cm(11, 0, 17);
    bool placed16 = voxelManager->setVoxel(pos16cm.value(), VoxelData::VoxelResolution::Size_16cm, true);
    std::cout << "DEBUG: 16cm voxel placement at (11,0,17): " << (placed16 ? "success" : "failed") << std::endl;
    ASSERT_TRUE(placed16) << "16cm voxel should be placeable at non-aligned position (11,0,17)";
    
    // 32cm voxel at non-aligned position (moved to avoid overlap with 64cm voxel at (7,23,11))
    // 64cm voxel occupies roughly (-25 to 39, -9 to 55, -21 to 43)
    // Place 32cm voxel at (80, 0, 29) which is well clear of the 64cm voxel
    Math::IncrementCoordinates pos32cm(80, 0, 29);
    bool placed32 = voxelManager->setVoxel(pos32cm.value(), VoxelData::VoxelResolution::Size_32cm, true);
    std::cout << "DEBUG: 32cm voxel placement at (80,0,29): " << (placed32 ? "success" : "failed") << std::endl;
    ASSERT_TRUE(placed32) << "32cm voxel should be placeable at non-aligned position (80,0,29)";
    
    // Test face detection on 16cm voxel
    Math::WorldCoordinates world16cm = Math::CoordinateConverter::incrementToWorld(pos16cm);
    Math::Vector3f center16cm = world16cm.value() + Math::Vector3f(0.08f, 0.08f, 0.08f); // 16cm/2 = 8cm
    
    Math::Vector3f rayOrigin16 = center16cm + Math::Vector3f(1.0f, 0, 0);
    Math::Vector3f rayTarget16 = center16cm + Math::Vector3f(0.08f, 0, 0); // Hit positive X face
    Math::Vector3f direction16 = (rayTarget16 - rayOrigin16).normalized();
    Math::Ray ray16(rayOrigin16, direction16);
    
    VisualFeedback::FaceDetector detector16;
    VisualFeedback::Ray vfRay16(ray16.origin, ray16.direction);
    const VoxelData::VoxelGrid* grid16 = voxelManager->getGrid(VoxelData::VoxelResolution::Size_16cm);
    VisualFeedback::Face face16 = detector16.detectFace(vfRay16, *grid16, VoxelData::VoxelResolution::Size_16cm);
    
    ASSERT_TRUE(face16.isValid()) << "Should detect 16cm voxel face at non-aligned position";
    EXPECT_EQ(face16.getVoxelPosition(), pos16cm) << "Should detect exact non-aligned 16cm voxel position";
    
    // Test face detection on 32cm voxel
    Math::WorldCoordinates world32cm = Math::CoordinateConverter::incrementToWorld(pos32cm);
    Math::Vector3f center32cm = world32cm.value() + Math::Vector3f(0.16f, 0.16f, 0.16f); // 32cm/2 = 16cm
    
    Math::Vector3f rayOrigin32 = center32cm + Math::Vector3f(1.0f, 0, 0);
    Math::Vector3f rayTarget32 = center32cm + Math::Vector3f(0.16f, 0, 0); // Hit positive X face
    Math::Vector3f direction32 = (rayTarget32 - rayOrigin32).normalized();
    Math::Ray ray32(rayOrigin32, direction32);
    
    // Debug output
    std::cout << "DEBUG: 32cm voxel test:" << std::endl;
    std::cout << "  Voxel pos: (" << pos32cm.x() << "," << pos32cm.y() << "," << pos32cm.z() << ")" << std::endl;
    std::cout << "  World pos: (" << world32cm.value().x << "," << world32cm.value().y << "," << world32cm.value().z << ")" << std::endl;
    std::cout << "  Center: (" << center32cm.x << "," << center32cm.y << "," << center32cm.z << ")" << std::endl;
    std::cout << "  Ray origin: (" << rayOrigin32.x << "," << rayOrigin32.y << "," << rayOrigin32.z << ")" << std::endl;
    std::cout << "  Ray direction: (" << direction32.x << "," << direction32.y << "," << direction32.z << ")" << std::endl;
    
    VisualFeedback::FaceDetector detector32;
    VisualFeedback::Ray vfRay32(ray32.origin, ray32.direction);
    const VoxelData::VoxelGrid* grid32 = voxelManager->getGrid(VoxelData::VoxelResolution::Size_32cm);
    
    std::cout << "  Grid has voxel: " << (grid32->getVoxel(pos32cm) ? "true" : "false") << std::endl;
    
    VisualFeedback::Face face32 = detector32.detectFace(vfRay32, *grid32, VoxelData::VoxelResolution::Size_32cm);
    
    std::cout << "  Face valid: " << (face32.isValid() ? "true" : "false") << std::endl;
    
    ASSERT_TRUE(face32.isValid()) << "Should detect 32cm voxel face at non-aligned position";
    EXPECT_EQ(face32.getVoxelPosition(), pos32cm) << "Should detect exact non-aligned 32cm voxel position";
}

} // namespace VoxelEditor