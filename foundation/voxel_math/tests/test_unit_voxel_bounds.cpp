#include <gtest/gtest.h>
#include "../include/voxel_math/VoxelBounds.h"
#include "../../math/CoordinateConverter.h"
#include <cmath>

using namespace VoxelEditor;
using namespace VoxelEditor::Math;

class VoxelBoundsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test data
    }
    
    // Helper function to check if two floats are approximately equal
    bool approxEqual(float a, float b, float epsilon = 1e-5f) {
        return std::abs(a - b) < epsilon;
    }
    
    // Helper function to check if two Vector3f are approximately equal
    bool approxEqual(const Vector3f& a, const Vector3f& b, float epsilon = 1e-5f) {
        return approxEqual(a.x, b.x, epsilon) &&
               approxEqual(a.y, b.y, epsilon) &&
               approxEqual(a.z, b.z, epsilon);
    }
};

// Test construction from world coordinates
TEST_F(VoxelBoundsTest, ConstructFromWorldCoordinates) {
    WorldCoordinates bottomCenter(Vector3f(1.0f, 0.0f, 2.0f));
    float voxelSize = 0.32f;  // 32cm voxel
    
    VoxelBounds bounds(bottomCenter, voxelSize);
    
    // Check size
    EXPECT_FLOAT_EQ(bounds.size(), voxelSize);
    
    // Check bottom center
    EXPECT_TRUE(approxEqual(bounds.bottomCenter().value(), bottomCenter.value()));
    
    // Check min corner
    Vector3f expectedMin(1.0f - 0.16f, 0.0f, 2.0f - 0.16f);
    EXPECT_TRUE(approxEqual(bounds.min().value(), expectedMin));
    
    // Check max corner
    Vector3f expectedMax(1.0f + 0.16f, 0.32f, 2.0f + 0.16f);
    EXPECT_TRUE(approxEqual(bounds.max().value(), expectedMax));
    
    // Check geometric center
    Vector3f expectedCenter(1.0f, 0.16f, 2.0f);
    EXPECT_TRUE(approxEqual(bounds.center().value(), expectedCenter));
}

// Test construction from increment coordinates
TEST_F(VoxelBoundsTest, ConstructFromIncrementCoordinates) {
    IncrementCoordinates bottomCenter(100, 0, 200);  // 1m, 0m, 2m
    float voxelSize = 0.32f;  // 32cm voxel
    
    VoxelBounds bounds(bottomCenter, voxelSize);
    
    // Check that it converts correctly to world coordinates
    WorldCoordinates expectedWorld = CoordinateConverter::incrementToWorld(bottomCenter);
    EXPECT_TRUE(approxEqual(bounds.bottomCenter().value(), expectedWorld.value()));
}

// Test bounds calculation for different voxel sizes
TEST_F(VoxelBoundsTest, BoundsCalculationVariousSizes) {
    WorldCoordinates bottomCenter(Vector3f(0.0f, 0.0f, 0.0f));
    
    // Test 1cm voxel
    {
        VoxelBounds bounds(bottomCenter, 0.01f);
        EXPECT_TRUE(approxEqual(bounds.min().value(), Vector3f(-0.005f, 0.0f, -0.005f)));
        EXPECT_TRUE(approxEqual(bounds.max().value(), Vector3f(0.005f, 0.01f, 0.005f)));
    }
    
    // Test 16cm voxel
    {
        VoxelBounds bounds(bottomCenter, 0.16f);
        EXPECT_TRUE(approxEqual(bounds.min().value(), Vector3f(-0.08f, 0.0f, -0.08f)));
        EXPECT_TRUE(approxEqual(bounds.max().value(), Vector3f(0.08f, 0.16f, 0.08f)));
    }
    
    // Test 512cm (5.12m) voxel
    {
        VoxelBounds bounds(bottomCenter, 5.12f);
        EXPECT_TRUE(approxEqual(bounds.min().value(), Vector3f(-2.56f, 0.0f, -2.56f)));
        EXPECT_TRUE(approxEqual(bounds.max().value(), Vector3f(2.56f, 5.12f, 2.56f)));
    }
}

// Test contains method
TEST_F(VoxelBoundsTest, ContainsPoint) {
    WorldCoordinates bottomCenter(Vector3f(1.0f, 0.0f, 1.0f));
    VoxelBounds bounds(bottomCenter, 0.32f);
    
    // Test points inside
    EXPECT_TRUE(bounds.contains(WorldCoordinates(Vector3f(1.0f, 0.16f, 1.0f))));  // Center
    EXPECT_TRUE(bounds.contains(WorldCoordinates(Vector3f(0.85f, 0.0f, 0.85f))));  // Near min corner
    EXPECT_TRUE(bounds.contains(WorldCoordinates(Vector3f(1.15f, 0.31f, 1.15f))));  // Near max corner
    
    // Test points on boundary (should be included)
    EXPECT_TRUE(bounds.contains(bounds.min()));
    EXPECT_TRUE(bounds.contains(bounds.max()));
    
    // Test points outside
    EXPECT_FALSE(bounds.contains(WorldCoordinates(Vector3f(1.0f, -0.01f, 1.0f))));  // Below
    EXPECT_FALSE(bounds.contains(WorldCoordinates(Vector3f(1.0f, 0.33f, 1.0f))));   // Above
    EXPECT_FALSE(bounds.contains(WorldCoordinates(Vector3f(1.17f, 0.16f, 1.0f))));  // Outside X
    EXPECT_FALSE(bounds.contains(WorldCoordinates(Vector3f(1.0f, 0.16f, 1.17f))));  // Outside Z
}

// Test intersection with other bounds
TEST_F(VoxelBoundsTest, IntersectsOtherBounds) {
    VoxelBounds bounds1(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)), 0.32f);
    
    // Test overlapping bounds
    VoxelBounds bounds2(WorldCoordinates(Vector3f(0.16f, 0.0f, 0.0f)), 0.32f);
    EXPECT_TRUE(bounds1.intersects(bounds2));
    
    // Test touching bounds (should intersect)
    VoxelBounds bounds3(WorldCoordinates(Vector3f(0.32f, 0.0f, 0.0f)), 0.32f);
    EXPECT_TRUE(bounds1.intersects(bounds3));
    
    // Test non-overlapping bounds
    VoxelBounds bounds4(WorldCoordinates(Vector3f(1.0f, 0.0f, 0.0f)), 0.32f);
    EXPECT_FALSE(bounds1.intersects(bounds4));
    
    // Test bounds at different Y levels
    VoxelBounds bounds5(WorldCoordinates(Vector3f(0.0f, 0.32f, 0.0f)), 0.32f);
    EXPECT_TRUE(bounds1.intersects(bounds5));  // They touch at Y=0.32
    
    VoxelBounds bounds6(WorldCoordinates(Vector3f(0.0f, 0.33f, 0.0f)), 0.32f);
    EXPECT_FALSE(bounds1.intersects(bounds6));  // Gap between them
}

// Test ray intersection
TEST_F(VoxelBoundsTest, RayIntersection) {
    VoxelBounds bounds(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)), 0.32f);
    
    // Test ray hitting from the front
    {
        Ray ray(WorldCoordinates(Vector3f(0.0f, 0.16f, -1.0f)), Vector3f(0.0f, 0.0f, 1.0f));
        float tMin, tMax;
        EXPECT_TRUE(bounds.intersectsRay(ray, tMin, tMax));
        EXPECT_FLOAT_EQ(tMin, 1.0f - 0.16f);  // Distance to front face
        EXPECT_FLOAT_EQ(tMax, 1.0f + 0.16f);  // Distance to back face
    }
    
    // Test ray missing the bounds
    {
        Ray ray(WorldCoordinates(Vector3f(1.0f, 0.16f, -1.0f)), Vector3f(0.0f, 0.0f, 1.0f));
        float tMin, tMax;
        EXPECT_FALSE(bounds.intersectsRay(ray, tMin, tMax));
    }
    
    // Test ray starting inside the bounds
    {
        Ray ray(WorldCoordinates(Vector3f(0.0f, 0.16f, 0.0f)), Vector3f(1.0f, 0.0f, 0.0f));
        float tMin, tMax;
        EXPECT_TRUE(bounds.intersectsRay(ray, tMin, tMax));
        EXPECT_FLOAT_EQ(tMin, 0.0f);  // Already inside
        EXPECT_FLOAT_EQ(tMax, 0.16f);  // Distance to exit face
    }
}

// Test face center calculations
TEST_F(VoxelBoundsTest, FaceCenters) {
    WorldCoordinates bottomCenter(Vector3f(1.0f, 0.0f, 2.0f));
    VoxelBounds bounds(bottomCenter, 0.32f);
    
    // Test each face center
    EXPECT_TRUE(approxEqual(bounds.getFaceCenter(VoxelData::FaceDirection::PositiveX).value(), 
                           Vector3f(1.16f, 0.16f, 2.0f)));
    EXPECT_TRUE(approxEqual(bounds.getFaceCenter(VoxelData::FaceDirection::NegativeX).value(), 
                           Vector3f(0.84f, 0.16f, 2.0f)));
    EXPECT_TRUE(approxEqual(bounds.getFaceCenter(VoxelData::FaceDirection::PositiveY).value(), 
                           Vector3f(1.0f, 0.32f, 2.0f)));
    EXPECT_TRUE(approxEqual(bounds.getFaceCenter(VoxelData::FaceDirection::NegativeY).value(), 
                           Vector3f(1.0f, 0.0f, 2.0f)));
    EXPECT_TRUE(approxEqual(bounds.getFaceCenter(VoxelData::FaceDirection::PositiveZ).value(), 
                           Vector3f(1.0f, 0.16f, 2.16f)));
    EXPECT_TRUE(approxEqual(bounds.getFaceCenter(VoxelData::FaceDirection::NegativeZ).value(), 
                           Vector3f(1.0f, 0.16f, 1.84f)));
}

// Test face normals
TEST_F(VoxelBoundsTest, FaceNormals) {
    VoxelBounds bounds(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)), 0.32f);
    
    EXPECT_TRUE(approxEqual(bounds.getFaceNormal(VoxelData::FaceDirection::PositiveX), 
                           Vector3f(1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(bounds.getFaceNormal(VoxelData::FaceDirection::NegativeX), 
                           Vector3f(-1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(bounds.getFaceNormal(VoxelData::FaceDirection::PositiveY), 
                           Vector3f(0.0f, 1.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(bounds.getFaceNormal(VoxelData::FaceDirection::NegativeY), 
                           Vector3f(0.0f, -1.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(bounds.getFaceNormal(VoxelData::FaceDirection::PositiveZ), 
                           Vector3f(0.0f, 0.0f, 1.0f)));
    EXPECT_TRUE(approxEqual(bounds.getFaceNormal(VoxelData::FaceDirection::NegativeZ), 
                           Vector3f(0.0f, 0.0f, -1.0f)));
}

// Test face planes
TEST_F(VoxelBoundsTest, FacePlanes) {
    WorldCoordinates bottomCenter(Vector3f(1.0f, 0.0f, 2.0f));
    VoxelBounds bounds(bottomCenter, 0.32f);
    
    // Test positive X face plane
    Plane xPlane = bounds.getFacePlane(VoxelData::FaceDirection::PositiveX);
    // The plane should pass through the face center and have the correct normal
    WorldCoordinates faceCenter = bounds.getFaceCenter(VoxelData::FaceDirection::PositiveX);
    float distance = xPlane.distanceToPoint(faceCenter.value());
    EXPECT_NEAR(distance, 0.0f, 1e-5f);
}

// Test equality operator
TEST_F(VoxelBoundsTest, EqualityOperator) {
    VoxelBounds bounds1(WorldCoordinates(Vector3f(1.0f, 0.0f, 2.0f)), 0.32f);
    VoxelBounds bounds2(WorldCoordinates(Vector3f(1.0f, 0.0f, 2.0f)), 0.32f);
    VoxelBounds bounds3(WorldCoordinates(Vector3f(1.0f, 0.0f, 2.0f)), 0.16f);
    VoxelBounds bounds4(WorldCoordinates(Vector3f(1.1f, 0.0f, 2.0f)), 0.32f);
    
    EXPECT_EQ(bounds1, bounds2);
    EXPECT_NE(bounds1, bounds3);  // Different size
    EXPECT_NE(bounds1, bounds4);  // Different position
}

// Test toBoundingBox conversion
TEST_F(VoxelBoundsTest, ToBoundingBox) {
    WorldCoordinates bottomCenter(Vector3f(1.0f, 0.0f, 2.0f));
    VoxelBounds bounds(bottomCenter, 0.32f);
    
    BoundingBox box = bounds.toBoundingBox();
    EXPECT_TRUE(approxEqual(box.min, bounds.min().value()));
    EXPECT_TRUE(approxEqual(box.max, bounds.max().value()));
}

// Test edge cases
TEST_F(VoxelBoundsTest, EdgeCases) {
    // Test very small voxel
    VoxelBounds tinyBounds(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)), 0.001f);
    EXPECT_FLOAT_EQ(tinyBounds.size(), 0.001f);
    
    // Test very large voxel
    VoxelBounds hugeBounds(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)), 10.0f);
    EXPECT_FLOAT_EQ(hugeBounds.size(), 10.0f);
    
    // Test negative Y position (should still work correctly)
    VoxelBounds negativeBounds(WorldCoordinates(Vector3f(0.0f, -1.0f, 0.0f)), 0.32f);
    EXPECT_FLOAT_EQ(negativeBounds.min().value().y, -1.0f);
    EXPECT_FLOAT_EQ(negativeBounds.max().value().y, -0.68f);
}