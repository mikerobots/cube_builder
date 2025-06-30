#include <gtest/gtest.h>
#include "../include/voxel_math/VoxelRaycast.h"
#include "../include/voxel_math/VoxelGrid.h"
#include "../../../core/voxel_data/VoxelGrid.h"
#include "../../../core/voxel_data/MultiResolutionVoxelGrid.h"
#include "../../math/Ray.h"
#include <memory>

using namespace VoxelEditor;
using namespace VoxelEditor::Math;

class VoxelRaycastTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test grid
        workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
        multiResGrid = std::make_unique<VoxelData::MultiResolutionVoxelGrid>(workspaceSize);
        
        // Get the grid for current resolution
        multiResGrid->setActiveResolution(VoxelData::VoxelResolution::Size_32cm);
        grid = &multiResGrid->getGrid(VoxelData::VoxelResolution::Size_32cm);
    }
    
    Vector3f workspaceSize;
    std::unique_ptr<VoxelData::MultiResolutionVoxelGrid> multiResGrid;
    VoxelData::VoxelGrid* grid;
};

// Test single voxel raycast
TEST_F(VoxelRaycastTest, RaycastSingleVoxel) {
    // Create a voxel at origin
    IncrementCoordinates voxelPos(0, 0, 0);
    
    // Ray hitting the voxel from front
    Ray ray(WorldCoordinates(Vector3f(0.0f, 0.16f, -1.0f)), Vector3f(0.0f, 0.0f, 1.0f));
    
    auto result = VoxelRaycast::raycastVoxel(ray, voxelPos, VoxelData::VoxelResolution::Size_32cm);
    
    EXPECT_TRUE(result.hit);
    EXPECT_FLOAT_EQ(result.distance, 1.0f - 0.16f);  // Distance to front face
    EXPECT_EQ(result.voxelPos, voxelPos);
    EXPECT_EQ(result.hitFace, VoxelData::FaceDirection::NegativeZ);
    
    // Check hit point
    EXPECT_NEAR(result.hitPoint.value().x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.hitPoint.value().y, 0.16f, 1e-5f);
    EXPECT_NEAR(result.hitPoint.value().z, -0.16f, 1e-5f);
}

// Test ray missing voxel
TEST_F(VoxelRaycastTest, RaycastMissVoxel) {
    IncrementCoordinates voxelPos(0, 0, 0);
    
    // Ray missing the voxel
    Ray ray(WorldCoordinates(Vector3f(1.0f, 0.16f, -1.0f)), Vector3f(0.0f, 0.0f, 1.0f));
    
    auto result = VoxelRaycast::raycastVoxel(ray, voxelPos, VoxelData::VoxelResolution::Size_32cm);
    
    EXPECT_FALSE(result.hit);
}

// Test grid raycast
TEST_F(VoxelRaycastTest, RaycastGrid) {
    // Place voxels in grid
    grid->setVoxel(IncrementCoordinates(0, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(32, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(64, 0, 0), true);
    
    // Ray hitting the first voxel
    Ray ray(WorldCoordinates(Vector3f(0.0f, 0.16f, -1.0f)), Vector3f(0.0f, 0.0f, 1.0f));
    
    auto result = VoxelRaycast::raycastGrid(ray, *grid, VoxelData::VoxelResolution::Size_32cm);
    
    EXPECT_TRUE(result.hit);
    EXPECT_EQ(result.voxelPos, IncrementCoordinates(0, 0, 0));
    EXPECT_EQ(result.hitFace, VoxelData::FaceDirection::NegativeZ);
}

// Test ray hitting multiple voxels
TEST_F(VoxelRaycastTest, RaycastMultipleVoxels) {
    // Place voxels in a line
    grid->setVoxel(IncrementCoordinates(0, 0, 0), true);
    grid->setVoxel(IncrementCoordinates(0, 0, 32), true);
    grid->setVoxel(IncrementCoordinates(0, 0, 64), true);
    
    // Ray going through all voxels
    Ray ray(WorldCoordinates(Vector3f(0.0f, 0.16f, -1.0f)), Vector3f(0.0f, 0.0f, 1.0f));
    
    // Get closest hit
    auto result = VoxelRaycast::raycastGrid(ray, *grid, VoxelData::VoxelResolution::Size_32cm);
    
    EXPECT_TRUE(result.hit);
    EXPECT_EQ(result.voxelPos, IncrementCoordinates(0, 0, 0));  // Closest voxel
    
    // Get all hits
    auto allHits = VoxelRaycast::raycastAllHits(ray, *grid, VoxelData::VoxelResolution::Size_32cm);
    
    EXPECT_EQ(allHits.size(), 3);
    EXPECT_LT(allHits[0].distance, allHits[1].distance);
    EXPECT_LT(allHits[1].distance, allHits[2].distance);
}

// Test voxels along ray path
TEST_F(VoxelRaycastTest, GetVoxelsAlongRay) {
    // Ray going in positive X direction
    Ray ray(WorldCoordinates(Vector3f(-1.0f, 0.16f, 0.0f)), Vector3f(1.0f, 0.0f, 0.0f));
    
    auto voxels = VoxelRaycast::getVoxelsAlongRay(ray, VoxelData::VoxelResolution::Size_32cm, 2.0f);
    
    // Should traverse multiple voxels
    EXPECT_GT(voxels.size(), 3);
    
    // Check that voxels are in order along X axis
    for (size_t i = 1; i < voxels.size(); ++i) {
        EXPECT_GE(voxels[i].x(), voxels[i-1].x());
    }
}

// Test ray intersection check (boolean)
TEST_F(VoxelRaycastTest, RayIntersectsGrid) {
    // Empty grid - no intersection
    Ray ray(WorldCoordinates(Vector3f(0.0f, 0.16f, -1.0f)), Vector3f(0.0f, 0.0f, 1.0f));
    EXPECT_FALSE(VoxelRaycast::rayIntersectsGrid(ray, *grid, VoxelData::VoxelResolution::Size_32cm));
    
    // Add voxel - should intersect
    grid->setVoxel(IncrementCoordinates(0, 0, 0), true);
    EXPECT_TRUE(VoxelRaycast::rayIntersectsGrid(ray, *grid, VoxelData::VoxelResolution::Size_32cm));
    
    // Ray missing voxel - no intersection
    Ray missRay(WorldCoordinates(Vector3f(1.0f, 0.16f, -1.0f)), Vector3f(0.0f, 0.0f, 1.0f));
    EXPECT_FALSE(VoxelRaycast::rayIntersectsGrid(missRay, *grid, VoxelData::VoxelResolution::Size_32cm));
}

// Test workspace raycast
TEST_F(VoxelRaycastTest, RaycastWorkspace) {
    Vector3f workspace(2.0f, 2.0f, 2.0f);
    
    // Ray hitting workspace from outside
    Ray ray(WorldCoordinates(Vector3f(0.0f, 1.0f, -2.0f)), Vector3f(0.0f, 0.0f, 1.0f));
    
    auto result = VoxelRaycast::raycastWorkspace(ray, workspace);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->hit);
    EXPECT_FLOAT_EQ(result->distance, 1.0f);  // Distance to front face
    EXPECT_EQ(result->hitFace, VoxelData::FaceDirection::NegativeZ);
    
    // Ray starting inside workspace
    Ray insideRay(WorldCoordinates(Vector3f(0.0f, 1.0f, 0.0f)), Vector3f(0.0f, 0.0f, 1.0f));
    auto insideResult = VoxelRaycast::raycastWorkspace(insideRay, workspace);
    
    EXPECT_TRUE(insideResult.has_value());
    EXPECT_TRUE(insideResult->hit);
    EXPECT_EQ(insideResult->hitFace, VoxelData::FaceDirection::PositiveZ);
}

// Test ray-voxel intersection calculation
TEST_F(VoxelRaycastTest, CalculateRayVoxelIntersection) {
    VoxelBounds voxelBounds(IncrementCoordinates(0, 0, 0), 0.32f);
    
    // Ray going through voxel
    Ray ray(WorldCoordinates(Vector3f(0.0f, 0.16f, -1.0f)), Vector3f(0.0f, 0.0f, 1.0f));
    
    auto intersection = VoxelRaycast::calculateRayVoxelIntersection(ray, voxelBounds);
    
    EXPECT_TRUE(intersection.has_value());
    
    const auto& [entry, exit] = intersection.value();
    
    // Entry point should be on front face
    EXPECT_NEAR(entry.value().z, -0.16f, 1e-5f);
    // Exit point should be on back face
    EXPECT_NEAR(exit.value().z, 0.16f, 1e-5f);
    
    // Both points should be at same X and Y
    EXPECT_NEAR(entry.value().x, exit.value().x, 1e-5f);
    EXPECT_NEAR(entry.value().y, exit.value().y, 1e-5f);
}

// Test face detection for different angles
TEST_F(VoxelRaycastTest, FaceDetectionVariousAngles) {
    IncrementCoordinates voxelPos(0, 0, 0);
    
    // Test each face
    struct TestCase {
        Vector3f rayOrigin;
        Vector3f rayDirection;
        VoxelData::FaceDirection expectedFace;
    };
    
    std::vector<TestCase> testCases = {
        {{-1.0f, 0.16f, 0.0f}, {1.0f, 0.0f, 0.0f}, VoxelData::FaceDirection::NegativeX},
        {{1.0f, 0.16f, 0.0f}, {-1.0f, 0.0f, 0.0f}, VoxelData::FaceDirection::PositiveX},
        {{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, VoxelData::FaceDirection::NegativeY},
        {{0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, VoxelData::FaceDirection::PositiveY},
        {{0.0f, 0.16f, -1.0f}, {0.0f, 0.0f, 1.0f}, VoxelData::FaceDirection::NegativeZ},
        {{0.0f, 0.16f, 1.0f}, {0.0f, 0.0f, -1.0f}, VoxelData::FaceDirection::PositiveZ}
    };
    
    for (const auto& testCase : testCases) {
        Ray ray(WorldCoordinates(testCase.rayOrigin), testCase.rayDirection);
        auto result = VoxelRaycast::raycastVoxel(ray, voxelPos, VoxelData::VoxelResolution::Size_32cm);
        
        EXPECT_TRUE(result.hit) << "Ray should hit voxel from direction: " 
                               << testCase.rayDirection.x << ", " 
                               << testCase.rayDirection.y << ", " 
                               << testCase.rayDirection.z;
        EXPECT_EQ(result.hitFace, testCase.expectedFace);
    }
}

// Test maximum distance limiting
TEST_F(VoxelRaycastTest, MaxDistanceLimiting) {
    // Place voxel far away
    grid->setVoxel(IncrementCoordinates(0, 0, 200), true);  // 2m away
    
    Ray ray(WorldCoordinates(Vector3f(0.0f, 0.16f, -1.0f)), Vector3f(0.0f, 0.0f, 1.0f));
    
    // Test with small max distance - should not hit
    auto result1 = VoxelRaycast::raycastGrid(ray, *grid, VoxelData::VoxelResolution::Size_32cm, 1.0f);
    EXPECT_FALSE(result1.hit);
    
    // Test with large max distance - should hit
    auto result2 = VoxelRaycast::raycastGrid(ray, *grid, VoxelData::VoxelResolution::Size_32cm, 10.0f);
    EXPECT_TRUE(result2.hit);
}

// Test edge cases
TEST_F(VoxelRaycastTest, EdgeCases) {
    // Ray parallel to voxel face
    Ray parallelRay(WorldCoordinates(Vector3f(-1.0f, 0.16f, -1.0f)), 
                   Vector3f(1.0f, 0.0f, 0.0f));
    
    auto result = VoxelRaycast::raycastVoxel(parallelRay, IncrementCoordinates(0, 0, 0), 
                                           VoxelData::VoxelResolution::Size_32cm);
    EXPECT_FALSE(result.hit);  // Should miss
    
    // Very small voxel
    auto smallResult = VoxelRaycast::raycastVoxel(
        Ray(WorldCoordinates(Vector3f(0.0f, 0.005f, -0.1f)), Vector3f(0.0f, 0.0f, 1.0f)),
        IncrementCoordinates(0, 0, 0), 
        VoxelData::VoxelResolution::Size_1cm);
    EXPECT_TRUE(smallResult.hit);
    
    // Very large voxel
    auto largeResult = VoxelRaycast::raycastVoxel(
        Ray(WorldCoordinates(Vector3f(0.0f, 2.56f, -10.0f)), Vector3f(0.0f, 0.0f, 1.0f)),
        IncrementCoordinates(0, 0, 0), 
        VoxelData::VoxelResolution::Size_512cm);
    EXPECT_TRUE(largeResult.hit);
}