#include <gtest/gtest.h>
#include "voxel_data/VoxelDataManager.h"
#include "visual_feedback/FaceDetector.h"
#include "visual_feedback/FeedbackTypes.h"
#include "foundation/logging/Logger.h"

namespace VoxelEditor {
namespace Tests {

// Direct test of the FaceDetector fix without full application
TEST(FaceDetectorTest, SelectsClosestVoxelToCamera) {
    // Initialize logging (silent)
    Logging::Logger::getInstance().setLevel(Logging::Logger::Level::Error);
    
    // Create voxel manager (it creates its own workspace manager)
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Set up test voxels using increment coordinates
    // For 64cm voxels, increment coordinates should be 64cm apart (64 * 1cm = 64 increments)
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_64cm);
    bool success1 = voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);      // World pos (0.0, 0.0, 0.0)
    bool success2 = voxelManager->setVoxel(Math::Vector3i(64, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);   // World pos (0.64, 0.0, 0.0)
    
    // Verify voxels were placed successfully
    ASSERT_TRUE(success1) << "First voxel should be placed successfully";
    ASSERT_TRUE(success2) << "Second voxel should be placed successfully";
    EXPECT_EQ(voxelManager->getVoxelCount(), 2) << "Should have 2 voxels placed";
    
    // Get grid reference
    auto* grid = voxelManager->getGrid(VoxelData::VoxelResolution::Size_64cm);
    ASSERT_NE(grid, nullptr) << "Grid should exist";
    
    // Debug: Check where voxels are in the grid
    std::cout << "Grid dimensions: " << grid->getGridDimensions().x << "x" 
              << grid->getGridDimensions().y << "x" << grid->getGridDimensions().z << std::endl;
    for (int x = 0; x < std::min(20, grid->getGridDimensions().x); x++) {
        for (int y = 0; y < std::min(5, grid->getGridDimensions().y); y++) {
            for (int z = 0; z < std::min(20, grid->getGridDimensions().z); z++) {
                if (grid->getVoxel(Math::Vector3i(x, y, z))) {
                    Math::Vector3f worldPos = grid->gridToWorld(Math::Vector3i(x, y, z));
                    std::cout << "Found voxel at grid(" << x << "," << y << "," << z 
                              << ") world(" << worldPos.x << "," << worldPos.y << "," << worldPos.z << ")" << std::endl;
                }
            }
        }
    }
    
    // Create face detector
    VisualFeedback::FaceDetector detector;
    
    // Create a ray from camera position looking at the voxels
    // Need to account for voxels being at z=-0.58 in world space
    VisualFeedback::Ray ray;
    ray.origin = Math::Vector3f(6.83f, 0.32f, -0.58f);  // Align with voxel Z position
    ray.direction = Math::Vector3f(-1.0f, 0.0f, 0.0f);  // Looking in negative X direction
    ray.direction.normalize();
    
    // Perform raycast
    auto hit = detector.detectFace(ray, *grid, VoxelData::VoxelResolution::Size_64cm);
    
    // Should hit voxel at grid position (4,0,3) which is closer to camera
    ASSERT_TRUE(hit.isValid()) << "Ray should hit a voxel";
    EXPECT_EQ(hit.getVoxelPosition().x, 4) << "Should hit voxel at grid x=4 (closer to camera)";
    EXPECT_EQ(hit.getVoxelPosition().y, 0);
    EXPECT_EQ(hit.getVoxelPosition().z, 3);
}

TEST(FaceDetectorTest, HandlesMultipleVoxelsAlongRay) {
    // Initialize logging (silent)
    Logging::Logger::getInstance().setLevel(Logging::Logger::Level::Error);
    
    // Create voxel manager
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Set up a line of voxels using increment coordinates (64cm apart)
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_64cm);
    for (int x = 0; x < 5; x++) {
        voxelManager->setVoxel(Math::Vector3i(x * 64, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    }
    
    // Create face detector
    VisualFeedback::FaceDetector detector;
    
    // Ray from the right side looking left
    // Need to account for voxels being at z=-0.58 in world space
    VisualFeedback::Ray ray;
    ray.origin = Math::Vector3f(10.0f, 0.32f, -0.58f);  // Align with voxel Z position
    ray.direction = Math::Vector3f(-1.0f, 0.0f, 0.0f);
    ray.direction.normalize();
    
    // Get grid and perform raycast
    auto* grid = voxelManager->getGrid(VoxelData::VoxelResolution::Size_64cm);
    ASSERT_NE(grid, nullptr) << "Grid should exist";
    auto hit = detector.detectFace(ray, *grid, VoxelData::VoxelResolution::Size_64cm);
    
    // Should hit voxel at rightmost position (grid x varies, z=3)
    ASSERT_TRUE(hit.isValid()) << "Ray should hit a voxel";
    // The rightmost voxel (increment 4*64) should be at higher grid X
    EXPECT_GE(hit.getVoxelPosition().x, 4) << "Should hit one of the rightmost voxels";
    EXPECT_EQ(hit.getVoxelPosition().z, 3);
}

TEST(FaceDetectorTest, PlacementBugScenario) {
    // Test the specific bug scenario: voxel at (0,5,0)
    Logging::Logger::getInstance().setLevel(Logging::Logger::Level::Error);
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_64cm);
    voxelManager->setVoxel(Math::Vector3i(0, 5 * 64, 0), VoxelData::VoxelResolution::Size_64cm, true);  // 5 voxels up
    
    VisualFeedback::FaceDetector detector;
    
    // Ray from default camera position toward the voxel
    VisualFeedback::Ray ray;
    ray.origin = Math::Vector3f(6.83f, 6.83f, 6.83f);  // Default iso camera position
    // The voxel at increment (0, 5*64, 0) should be at grid (3, 5, 3) with world center around (-0.26, 3.52, -0.26)
    Math::Vector3f voxelCenter(-0.26f, 3.52f, -0.26f);   // Approximate center of voxel
    ray.direction = voxelCenter - ray.origin;
    ray.direction.normalize();
    
    auto* grid = voxelManager->getGrid(VoxelData::VoxelResolution::Size_64cm);
    ASSERT_NE(grid, nullptr) << "Grid should exist";
    auto hit = detector.detectFace(ray, *grid, VoxelData::VoxelResolution::Size_64cm);
    
    // Should successfully detect the voxel (grid coordinates should be (3,5,3))
    ASSERT_TRUE(hit.isValid()) << "Ray should hit the voxel at grid position (3,5,3)";
    EXPECT_EQ(hit.getVoxelPosition(), Math::Vector3i(3, 5, 3));
}

} // namespace Tests
} // namespace VoxelEditor