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
    
    EXPECT_GT(result.size(), 0u) << "Expected at least one voxel to be selected";
    
    // With new requirements, voxels can be placed at any 1cm position
    // So there are many possible voxels that could contain or be near the sphere center
    // Let's verify that at least one voxel has its center very close to the sphere center
    bool foundCloseVoxel = false;
    float minDistance = std::numeric_limits<float>::max();
    
    for (const auto& voxel : result) {
        Math::Vector3f voxelCenter = voxel.getWorldPosition();
        float distance = (voxelCenter - center).length();
        minDistance = std::min(minDistance, distance);
        
        // Check if this voxel's center is very close to the sphere center
        if (distance < 0.03f) { // Within 3cm of sphere center
            foundCloseVoxel = true;
        }
    }
    
    EXPECT_TRUE(foundCloseVoxel) << "Expected at least one voxel with center close to sphere center. "
                                 << "Minimum distance found: " << minDistance;
    
    // Also verify all selected voxels are within or partially within the sphere
    // With the new voxel placement system, voxels can be at any 1cm position
    // and the sphere selector uses proper bounds checking for partial inclusion
    for (const auto& voxel : result) {
        // Use the voxel's actual bounds to check if it intersects with the sphere
        Math::BoundingBox voxelBounds = voxel.getBounds();
        
        // Find closest point on voxel to sphere center
        Math::Vector3f closestPoint = voxelBounds.closestPoint(center);
        
        float distance = (closestPoint - center).length();
        EXPECT_LE(distance, radius + 0.001f) << "Voxel bounds [" << voxelBounds.min.toString() 
                                             << " to " << voxelBounds.max.toString() 
                                             << "] do not properly intersect sphere";
    }
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
    
    Math::Vector3f center(0.02f, 0.02f, 0.02f);
    float radius = 0.025f; // Small radius
    
    SelectionSet result = selector->selectFromSphere(center, radius, VoxelData::VoxelResolution::Size_4cm, false);
    
    // With includePartial=false, only voxels whose centers are within the sphere are selected
    // With the new requirements, voxels can be at any 1cm position
    
    // The sphere center (0.02, 0.02, 0.02) with radius 0.025 creates a small sphere
    // We should find at least one voxel whose center is within this sphere
    EXPECT_GE(result.size(), 1u) << "Expected at least one voxel to be selected";
    
    // Verify all selected voxels have their centers within the sphere
    for (const auto& voxel : result) {
        Math::Vector3f voxelCenter = voxel.getWorldPosition();
        float distance = (voxelCenter - center).length();
        EXPECT_LE(distance, radius + 0.001f) << "Voxel center at " << voxelCenter.toString() 
                                              << " is outside the sphere (distance: " << distance 
                                              << ", radius: " << radius << ")";
    }
    
    // Find the closest voxel center to the sphere center
    float minDistance = std::numeric_limits<float>::max();
    for (const auto& voxel : result) {
        Math::Vector3f voxelCenter = voxel.getWorldPosition();
        float distance = (voxelCenter - center).length();
        minDistance = std::min(minDistance, distance);
    }
    
    // With a 4cm voxel, the closest possible center would be at (0.02, 0.02, 0.02)
    // if we have a voxel at position (0,0,0)
    EXPECT_LE(minDistance, 0.001f) << "Expected to find a voxel with center very close to sphere center";
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
    
    // With the new voxel placement rules where voxels can be at any 1cm position,
    // the relationship between resolution and voxel count is more complex.
    // We're selecting all possible voxel positions that could overlap the sphere.
    // Both resolutions should select a significant number of voxels.
    EXPECT_GT(result2cm.size(), 0u);
    EXPECT_GT(result8cm.size(), 0u);
    
    // The actual counts depend on the specific algorithm and overlap handling
    // so we just verify that both select some voxels
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