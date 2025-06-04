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
    
    // Set up test voxels
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_64cm);
    voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    voxelManager->setVoxel(Math::Vector3i(1, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    
    // Create face detector
    VisualFeedback::FaceDetector detector;
    
    // Create a ray from camera position looking at the voxels
    // Camera at (6.83, 0.32, 0.32) looking toward origin
    VisualFeedback::Ray ray;
    ray.origin = Math::Vector3f(6.83f, 0.32f, 0.32f);
    ray.direction = Math::Vector3f(-1.0f, 0.0f, 0.0f);  // Looking in negative X direction
    ray.direction.normalize();
    
    // Perform raycast
    auto* grid = voxelManager->getGrid(VoxelData::VoxelResolution::Size_64cm);
    ASSERT_NE(grid, nullptr) << "Grid should exist";
    auto hit = detector.detectFace(ray, *grid, VoxelData::VoxelResolution::Size_64cm);
    
    // Should hit voxel (1,0,0) which is closer to camera
    ASSERT_TRUE(hit.isValid()) << "Ray should hit a voxel";
    EXPECT_EQ(hit.getVoxelPosition().x, 1) << "Should hit voxel at x=1 (closer to camera)";
    EXPECT_EQ(hit.getVoxelPosition().y, 0);
    EXPECT_EQ(hit.getVoxelPosition().z, 0);
}

TEST(FaceDetectorTest, HandlesMultipleVoxelsAlongRay) {
    // Initialize logging (silent)
    Logging::Logger::getInstance().setLevel(Logging::Logger::Level::Error);
    
    // Create voxel manager
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Set up a line of voxels
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_64cm);
    for (int x = 0; x < 5; x++) {
        voxelManager->setVoxel(Math::Vector3i(x, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    }
    
    // Create face detector
    VisualFeedback::FaceDetector detector;
    
    // Ray from the right side looking left
    VisualFeedback::Ray ray;
    ray.origin = Math::Vector3f(10.0f, 0.32f, 0.32f);
    ray.direction = Math::Vector3f(-1.0f, 0.0f, 0.0f);
    ray.direction.normalize();
    
    // Perform raycast
    auto* grid = voxelManager->getGrid(VoxelData::VoxelResolution::Size_64cm);
    ASSERT_NE(grid, nullptr) << "Grid should exist";
    auto hit = detector.detectFace(ray, *grid, VoxelData::VoxelResolution::Size_64cm);
    
    // Should hit voxel (4,0,0) which is closest to ray origin
    ASSERT_TRUE(hit.isValid()) << "Ray should hit a voxel";
    EXPECT_EQ(hit.getVoxelPosition().x, 4) << "Should hit voxel at x=4 (closest to ray origin)";
}

TEST(FaceDetectorTest, PlacementBugScenario) {
    // Test the specific bug scenario: voxel at (0,5,0)
    Logging::Logger::getInstance().setLevel(Logging::Logger::Level::Error);
    
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_64cm);
    voxelManager->setVoxel(Math::Vector3i(0, 5, 0), VoxelData::VoxelResolution::Size_64cm, true);
    
    VisualFeedback::FaceDetector detector;
    
    // Ray from default camera position toward the voxel
    VisualFeedback::Ray ray;
    ray.origin = Math::Vector3f(6.83f, 6.83f, 6.83f);  // Default iso camera position
    Math::Vector3f voxelCenter(0.32f, 3.52f, 0.32f);   // Center of voxel at (0,5,0)
    ray.direction = voxelCenter - ray.origin;
    ray.direction.normalize();
    
    auto* grid = voxelManager->getGrid(VoxelData::VoxelResolution::Size_64cm);
    ASSERT_NE(grid, nullptr) << "Grid should exist";
    auto hit = detector.detectFace(ray, *grid, VoxelData::VoxelResolution::Size_64cm);
    
    // Should successfully detect the voxel
    ASSERT_TRUE(hit.isValid()) << "Ray should hit the voxel at (0,5,0)";
    EXPECT_EQ(hit.getVoxelPosition(), Math::Vector3i(0, 5, 0));
}

} // namespace Tests
} // namespace VoxelEditor