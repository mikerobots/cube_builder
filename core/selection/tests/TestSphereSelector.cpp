#include <gtest/gtest.h>
#include "core/selection/SphereSelector.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Selection;

class SphereSelectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        selector = std::make_unique<SphereSelector>();
    }
    
    std::unique_ptr<SphereSelector> selector;
};

// Basic Tests
TEST_F(SphereSelectorTest, DefaultConfiguration) {
    EXPECT_EQ(selector->getSelectionMode(), SelectionMode::Replace);
    EXPECT_TRUE(selector->getIncludePartial());
    EXPECT_FALSE(selector->getFalloff());
    EXPECT_FLOAT_EQ(selector->getFalloffStart(), 0.8f);
}

TEST_F(SphereSelectorTest, SetConfiguration) {
    selector->setSelectionMode(SelectionMode::Add);
    selector->setIncludePartial(false);
    selector->setFalloff(true, 0.6f);
    
    EXPECT_EQ(selector->getSelectionMode(), SelectionMode::Add);
    EXPECT_FALSE(selector->getIncludePartial());
    EXPECT_TRUE(selector->getFalloff());
    EXPECT_FLOAT_EQ(selector->getFalloffStart(), 0.6f);
}

// Sphere Selection Tests
TEST_F(SphereSelectorTest, SelectFromSphere_SmallRadius) {
    // REQ: SphereSelector for different selection methods
    Math::Vector3f center(0.02f, 0.02f, 0.02f);
    float radius = 0.02f;
    
    SelectionSet result = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should contain at least the voxel at origin
    EXPECT_GE(result.size(), 1u);
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(SphereSelectorTest, SelectFromSphere_LargerRadius) {
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    float radius = 0.1f;
    
    SelectionSet result = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should contain multiple voxels
    EXPECT_GT(result.size(), 1u);
    
    // All selected voxels should be within radius
    for (const auto& voxel : result) {
        float distance = (voxel.getWorldPosition() - center).length();
        EXPECT_LE(distance, radius + 0.04f); // Account for voxel size
    }
}

TEST_F(SphereSelectorTest, SelectFromSphere_OffsetCenter) {
    Math::Vector3f center(0.1f, 0.1f, 0.1f);
    float radius = 0.08f;
    
    SelectionSet result = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_4cm, false);
    
    EXPECT_GT(result.size(), 0u);
    
    // Center at (0.1, 0.1, 0.1) with 4cm voxels
    // The voxel at increment position (8,8,8) would have world center at (0.1, 0.1, 0.1)
    // (IncrementCoordinates are in 1cm increments, so 10cm = increment 10)
    // Actually, the voxel containing point (0.1, 0.1, 0.1) would be at increment (8,8,8)
    // since that voxel spans (0.08, 0.08, 0.08) to (0.12, 0.12, 0.12)
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(8, 8, 8)), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(SphereSelectorTest, SelectFromSphere_IncludePartialTrue) {
    selector->setIncludePartial(true);
    
    Math::Vector3f center(0.04f, 0.04f, 0.04f);
    float radius = 0.03f; // Small radius that partially overlaps adjacent voxels
    
    SelectionSet result = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should include voxels that are partially intersected
    EXPECT_GE(result.size(), 2u);
}

TEST_F(SphereSelectorTest, SelectFromSphere_IncludePartialFalse) {
    selector->setIncludePartial(false);
    
    Math::Vector3f center(0.02f, 0.02f, 0.02f);  // Center of voxel at (0,0,0)
    float radius = 0.025f; // Radius large enough to include the voxel center
    
    SelectionSet result = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_4cm, false);
    
    // Should only include voxels whose centers are within radius
    EXPECT_GE(result.size(), 1u);
    
    // Voxel at (0,0,0) should be selected since its center is at (0.02, 0.02, 0.02)
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), VoxelData::VoxelResolution::Size_4cm)));
}

// Ray Selection Tests
TEST_F(SphereSelectorTest, SelectFromRay_Basic) {
    Math::Ray ray(Math::Vector3f(0, 0, -1), Math::Vector3f(0, 0, 1));
    float radius = 0.05f;
    float maxDistance = 2.0f;
    
    SelectionSet result = selector->selectFromRay(ray, radius, maxDistance, VoxelData::VoxelResolution::Size_4cm);
    
    // Should create sphere at intersection point
    EXPECT_GT(result.size(), 0u);
}

// Ellipsoid Selection Tests
TEST_F(SphereSelectorTest, SelectEllipsoid_Basic) {
    selector->setIncludePartial(false); // Only select voxels whose centers are inside
    
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    Math::Vector3f radii(0.1f, 0.05f, 0.08f);
    Math::Quaternion rotation = Math::Quaternion::identity();
    
    SelectionSet result = selector->selectEllipsoid(center, radii, rotation, 
                                                   VoxelData::VoxelResolution::Size_4cm, false);
    
    EXPECT_GT(result.size(), 0u);
    
    // Check that selected voxels satisfy ellipsoid equation
    for (const auto& voxel : result) {
        Math::Vector3f pos = voxel.getWorldPosition() - center;
        float value = (pos.x * pos.x) / (radii.x * radii.x) +
                     (pos.y * pos.y) / (radii.y * radii.y) +
                     (pos.z * pos.z) / (radii.z * radii.z);
        EXPECT_LE(value, 1.1f); // Small tolerance for voxel centers
    }
}

TEST_F(SphereSelectorTest, SelectEllipsoid_Rotated) {
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    Math::Vector3f radii(0.1f, 0.05f, 0.05f); // Elongated along X
    Math::Quaternion rotation = Math::Quaternion::fromAxisAngle(Math::Vector3f(0, 0, 1), Math::PI / 4); // 45 degrees around Z
    
    SelectionSet result = selector->selectEllipsoid(center, radii, rotation, 
                                                   VoxelData::VoxelResolution::Size_4cm, false);
    
    EXPECT_GT(result.size(), 0u);
}

// Hemisphere Selection Tests
TEST_F(SphereSelectorTest, SelectHemisphere_UpwardFacing) {
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    float radius = 0.1f;
    Math::Vector3f normal(0, 1, 0); // Upward facing
    
    SelectionSet result = selector->selectHemisphere(center, radius, normal, 
                                                    VoxelData::VoxelResolution::Size_4cm, false);
    
    EXPECT_GT(result.size(), 0u);
    
    // All selected voxels should be in upper hemisphere
    for (const auto& voxel : result) {
        Math::Vector3f toVoxel = voxel.getWorldPosition() - center;
        if (toVoxel.length() <= radius) {
            EXPECT_GE(toVoxel.dot(normal), -0.02f); // Small tolerance
        }
    }
    
    // Should not contain voxels significantly below the plane
    // With 4cm voxels, a voxel at (0, -8, 0) would be centered at (0, -0.08, 0) which is below the hemisphere
    EXPECT_FALSE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, -8, 0)), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(SphereSelectorTest, SelectHemisphere_SidewaysFacing) {
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    float radius = 0.08f;
    Math::Vector3f normal(1, 0, 0); // Facing positive X
    
    SelectionSet result = selector->selectHemisphere(center, radius, normal, 
                                                    VoxelData::VoxelResolution::Size_4cm, false);
    
    EXPECT_GT(result.size(), 0u);
    
    // Should contain voxels in positive X direction
    // With 4cm voxels, increment position (4, 0, 0) is at world (0.04, 0, 0)
    EXPECT_TRUE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(4, 0, 0)), VoxelData::VoxelResolution::Size_4cm)));
    
    // Should not contain voxels in negative X direction (beyond center)
    // Voxel at (-8, 0, 0) would be centered at (-0.08, 0, 0) which is in the negative hemisphere
    EXPECT_FALSE(result.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(-8, 0, 0)), VoxelData::VoxelResolution::Size_4cm)));
}

// Different Resolution Tests
TEST_F(SphereSelectorTest, SelectFromSphere_DifferentResolutions) {
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    float radius = 0.2f;
    
    // Test with 2cm resolution
    SelectionSet result2cm = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_2cm, false);
    
    // Test with 8cm resolution
    SelectionSet result8cm = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_8cm, false);
    
    // Higher resolution should have more voxels
    EXPECT_GT(result2cm.size(), result8cm.size());
}

// Edge Cases
TEST_F(SphereSelectorTest, SelectFromSphere_ZeroRadius) {
    // Ensure includePartial is false for this test
    selector->setIncludePartial(false);
    
    Math::Vector3f center(0.02f, 0.02f, 0.02f);
    float radius = 0.0f;
    
    SelectionSet result = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_4cm, false);
    
    // With zero radius and includePartial=false, should select no voxels
    // (voxel centers are not exactly at the sphere center)
    EXPECT_EQ(result.size(), 0u);
}

TEST_F(SphereSelectorTest, SelectFromSphere_VeryLargeRadius) {
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    float radius = 10.0f;
    
    SelectionSet result = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_64cm, false);
    
    // Should select many voxels
    EXPECT_GT(result.size(), 100u);
}

TEST_F(SphereSelectorTest, SelectEllipsoid_Sphere) {
    // When all radii are equal, ellipsoid should behave like sphere
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    float radius = 0.1f;
    Math::Vector3f radii(radius, radius, radius);
    Math::Quaternion rotation = Math::Quaternion::identity();
    
    SelectionSet ellipsoidResult = selector->selectEllipsoid(center, radii, rotation, 
                                                            VoxelData::VoxelResolution::Size_4cm, false);
    SelectionSet sphereResult = selector->selectFromSphere(center, radius, 
                                                          VoxelData::VoxelResolution::Size_4cm, false);
    
    // Results should be identical
    EXPECT_EQ(ellipsoidResult, sphereResult);
}

TEST_F(SphereSelectorTest, SelectHemisphere_FullSphere) {
    // Compare hemisphere with full sphere - use larger radius to ensure difference
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    float radius = 0.12f;  // Larger radius to ensure voxels exist on both sides
    Math::Vector3f normal(0, 1, 0);
    
    SelectionSet hemisphereResult = selector->selectHemisphere(center, radius, normal, 
                                                              VoxelData::VoxelResolution::Size_4cm, false);
    SelectionSet sphereResult = selector->selectFromSphere(center, radius, 
                                                          VoxelData::VoxelResolution::Size_4cm, false);
    
    // Hemisphere should have fewer voxels than full sphere (or equal if sphere is very small)
    EXPECT_LE(hemisphereResult.size(), sphereResult.size());
    
    // All hemisphere voxels should be in sphere
    for (const auto& voxel : hemisphereResult) {
        EXPECT_TRUE(sphereResult.contains(voxel));
    }
    
    // If we have a reasonable number of voxels, hemisphere should be smaller
    if (sphereResult.size() > 10) {
        EXPECT_LT(hemisphereResult.size(), sphereResult.size());
    }
}

// Voxel Manager Tests
TEST_F(SphereSelectorTest, SetVoxelManager) {
    selector->setVoxelManager(nullptr);
    
    // Selection should still work without manager (assumes all voxels exist)
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    float radius = 0.1f;
    
    SelectionSet result = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_4cm, true);
    EXPECT_GT(result.size(), 0u);
}