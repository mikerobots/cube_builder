#include <gtest/gtest.h>
#include "camera/OrbitCamera.h"
#include "math/MathUtils.h"
#include <cmath>

using namespace VoxelEditor;
using namespace VoxelEditor::Camera;
using namespace VoxelEditor::Math;

class OrbitCameraTransformationTest : public ::testing::Test {
protected:
    void SetUp() override {
        camera = std::make_unique<OrbitCamera>();
        camera->setAspectRatio(16.0f / 9.0f);
    }
    
    std::unique_ptr<OrbitCamera> camera;
    const float EPSILON = 1e-5f;
};

// Test that camera position is calculated correctly from angles
TEST_F(OrbitCameraTransformationTest, CameraPositionFromAngles) {
    // Test 1: Default position (45° yaw, 45° pitch)
    {
        camera->setYaw(45.0f);
        camera->setPitch(45.0f);
        camera->setDistance(10.0f);
        camera->setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
        
        WorldCoordinates pos = camera->getPosition();
        
        // At 45° yaw, 45° pitch, distance 10:
        // x = distance * cos(pitch) * sin(yaw)
        // y = distance * sin(pitch)
        // z = distance * cos(pitch) * cos(yaw)
        float expectedX = 10.0f * std::cos(Math::toRadians(45.0f)) * std::sin(Math::toRadians(45.0f));
        float expectedY = 10.0f * std::sin(Math::toRadians(45.0f));
        float expectedZ = 10.0f * std::cos(Math::toRadians(45.0f)) * std::cos(Math::toRadians(45.0f));
        
        EXPECT_NEAR(pos.x(), expectedX, EPSILON) << "X position incorrect for 45° yaw, 45° pitch";
        EXPECT_NEAR(pos.y(), expectedY, EPSILON) << "Y position incorrect for 45° yaw, 45° pitch";
        EXPECT_NEAR(pos.z(), expectedZ, EPSILON) << "Z position incorrect for 45° yaw, 45° pitch";
    }
    
    // Test 2: Looking straight down from above (0° yaw, 90° pitch)
    {
        camera->setYaw(0.0f);
        camera->setPitch(90.0f);
        camera->setDistance(5.0f);
        camera->setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
        
        WorldCoordinates pos = camera->getPosition();
        
        EXPECT_NEAR(pos.x(), 0.0f, EPSILON) << "X should be 0 when looking straight down";
        EXPECT_NEAR(pos.y(), 5.0f, EPSILON) << "Y should equal distance when pitch=90°";
        EXPECT_NEAR(pos.z(), 0.0f, EPSILON) << "Z should be 0 when looking straight down";
    }
    
    // Test 3: Looking from the front (0° yaw, 0° pitch)
    {
        camera->setYaw(0.0f);
        camera->setPitch(0.0f);
        camera->setDistance(8.0f);
        camera->setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
        
        WorldCoordinates pos = camera->getPosition();
        
        EXPECT_NEAR(pos.x(), 0.0f, EPSILON) << "X should be 0 at yaw=0°";
        EXPECT_NEAR(pos.y(), 0.0f, EPSILON) << "Y should be 0 at pitch=0°";
        EXPECT_NEAR(pos.z(), 8.0f, EPSILON) << "Z should equal distance when yaw=0°, pitch=0°";
    }
    
    // Test 4: Looking from the side (90° yaw, 0° pitch)
    {
        camera->setYaw(90.0f);
        camera->setPitch(0.0f);
        camera->setDistance(7.0f);
        camera->setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
        
        WorldCoordinates pos = camera->getPosition();
        
        EXPECT_NEAR(pos.x(), 7.0f, EPSILON) << "X should equal distance at yaw=90°";
        EXPECT_NEAR(pos.y(), 0.0f, EPSILON) << "Y should be 0 at pitch=0°";
        EXPECT_NEAR(pos.z(), 0.0f, EPSILON) << "Z should be 0 at yaw=90°";
    }
}

// Test that view matrix correctly transforms target to origin in view space
TEST_F(OrbitCameraTransformationTest, ViewMatrixTransformsTargetToOrigin) {
    WorldCoordinates target(Vector3f(2.0f, 3.0f, 4.0f));
    camera->setTarget(target);
    camera->setDistance(10.0f);
    camera->setYaw(30.0f);
    camera->setPitch(25.0f);
    
    Matrix4f viewMatrix = camera->getViewMatrix();
    
    // The view matrix transforms world space to view space
    // In view space, the camera looks down the negative Z axis
    // So we need to verify that the target transforms correctly
    
    // For now, let's verify the view matrix is consistent with position
    WorldCoordinates cameraPos = camera->getPosition();
    
    // Transform the target to view space
    Vector4f targetHomogeneous(target.x(), target.y(), target.z(), 1.0f);
    Vector4f targetInView = viewMatrix * targetHomogeneous;
    
    // Transform the camera position to view space (should be at origin)
    Vector4f camPosHomogeneous(cameraPos.x(), cameraPos.y(), cameraPos.z(), 1.0f);
    Vector4f camPosInView = viewMatrix * camPosHomogeneous;
    
    // Camera position should be at origin in view space
    EXPECT_NEAR(camPosInView.x, 0.0f, EPSILON) << "Camera X position in view space";
    EXPECT_NEAR(camPosInView.y, 0.0f, EPSILON) << "Camera Y position in view space";
    EXPECT_NEAR(camPosInView.z, 0.0f, EPSILON) << "Camera Z position in view space";
    
    // The target should be along the negative Z axis in view space
    // The distance from camera to target should be preserved
    float expectedDistance = (target.value() - cameraPos.value()).length();
    EXPECT_NEAR(targetInView.z, -expectedDistance, EPSILON) << "Target should be at -distance along Z in view space";
}

// Test view matrix for objects at various positions
TEST_F(OrbitCameraTransformationTest, ViewMatrixTransformations) {
    camera->setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
    camera->setDistance(5.0f);
    camera->setYaw(0.0f);
    camera->setPitch(0.0f);
    
    Matrix4f viewMatrix = camera->getViewMatrix();
    
    // Camera is at (0, 0, 5) looking at origin
    // Objects at origin should transform to (0, 0, -5) in view space
    {
        Vector4f origin(0, 0, 0, 1);
        Vector4f viewSpace = viewMatrix * origin;
        
        EXPECT_NEAR(viewSpace.x, 0.0f, EPSILON) << "Origin X in view space";
        EXPECT_NEAR(viewSpace.y, 0.0f, EPSILON) << "Origin Y in view space";
        EXPECT_NEAR(viewSpace.z, -5.0f, EPSILON) << "Origin Z in view space should be -distance";
        EXPECT_NEAR(viewSpace.w, 1.0f, EPSILON) << "W component should remain 1";
    }
    
    // Object at camera position should transform to (0, 0, 0)
    {
        WorldCoordinates camPos = camera->getPosition();
        Vector4f atCamera(camPos.x(), camPos.y(), camPos.z(), 1);
        Vector4f viewSpace = viewMatrix * atCamera;
        
        EXPECT_NEAR(viewSpace.x, 0.0f, EPSILON) << "Camera position X in view space";
        EXPECT_NEAR(viewSpace.y, 0.0f, EPSILON) << "Camera position Y in view space";
        EXPECT_NEAR(viewSpace.z, 0.0f, EPSILON) << "Camera position Z in view space";
    }
}

// Test that projection matrix produces correct NDC coordinates
TEST_F(OrbitCameraTransformationTest, ProjectionMatrixNDC) {
    // These are in the base Camera class
    camera->setAspectRatio(1.0f);  // Square for easier calculation
    
    // Get default near/far planes (0.1f and 1000.0f from Camera.h)
    float nearPlane = camera->getNearPlane();
    float farPlane = camera->getFarPlane();
    
    Matrix4f projMatrix = camera->getProjectionMatrix();
    
    // Test near plane center
    {
        Vector4f nearCenter(0, 0, -nearPlane, 1);  // Center of near plane in view space
        Vector4f clip = projMatrix * nearCenter;
        Vector4f ndc = clip / clip.w;
        
        EXPECT_NEAR(ndc.x, 0.0f, EPSILON) << "Near plane center X in NDC";
        EXPECT_NEAR(ndc.y, 0.0f, EPSILON) << "Near plane center Y in NDC";
        EXPECT_NEAR(ndc.z, -1.0f, EPSILON) << "Near plane maps to -1 in NDC";
    }
    
    // Test far plane center
    {
        Vector4f farCenter(0, 0, -farPlane, 1);  // Center of far plane in view space
        Vector4f clip = projMatrix * farCenter;
        Vector4f ndc = clip / clip.w;
        
        EXPECT_NEAR(ndc.x, 0.0f, EPSILON) << "Far plane center X in NDC";
        EXPECT_NEAR(ndc.y, 0.0f, EPSILON) << "Far plane center Y in NDC";
        EXPECT_NEAR(ndc.z, 1.0f, 0.01f) << "Far plane maps to 1 in NDC";  // Slightly less precision due to large range
    }
}

// Test complete MVP transformation for a voxel
TEST_F(OrbitCameraTransformationTest, VoxelMVPTransformation) {
    // Set up camera similar to main app
    camera->setViewPreset(ViewPreset::ISOMETRIC);
    camera->setTarget(WorldCoordinates(Vector3f(0.64f, 0.64f, 0.64f)));  // Voxel at (0,0,0) world position
    camera->setDistance(5.0f);
    
    Matrix4f viewMatrix = camera->getViewMatrix();
    Matrix4f projMatrix = camera->getProjectionMatrix();
    Matrix4f mvp = projMatrix * viewMatrix;  // No model matrix (identity)
    
    // Test the voxel center
    {
        Vector4f voxelCenter(0.64f, 0.64f, 0.64f, 1.0f);
        Vector4f clip = mvp * voxelCenter;
        
        // Voxel should be visible if clip.w > 0 and NDC coords are in [-1, 1]
        EXPECT_GT(clip.w, 0.0f) << "Voxel should be in front of camera (positive w)";
        
        Vector4f ndc = clip / clip.w;
        
        // Should be near center of screen since camera targets this position
        EXPECT_GE(ndc.x, -1.0f) << "Voxel X should be within NDC range";
        EXPECT_LE(ndc.x, 1.0f) << "Voxel X should be within NDC range";
        EXPECT_GE(ndc.y, -1.0f) << "Voxel Y should be within NDC range";
        EXPECT_LE(ndc.y, 1.0f) << "Voxel Y should be within NDC range";
        EXPECT_GE(ndc.z, -1.0f) << "Voxel Z should be within NDC range";
        EXPECT_LE(ndc.z, 1.0f) << "Voxel Z should be within NDC range";
        
        // Should be close to center since we're targeting it
        EXPECT_NEAR(ndc.x, 0.0f, 0.1f) << "Targeted voxel should be near screen center X";
        EXPECT_NEAR(ndc.y, 0.0f, 0.1f) << "Targeted voxel should be near screen center Y";
    }
    
    // Test voxel corners
    float halfSize = 0.64f;  // Half of 128cm voxel
    std::vector<Vector4f> corners = {
        Vector4f(0.64f - halfSize, 0.64f - halfSize, 0.64f - halfSize, 1.0f),
        Vector4f(0.64f + halfSize, 0.64f - halfSize, 0.64f - halfSize, 1.0f),
        Vector4f(0.64f - halfSize, 0.64f + halfSize, 0.64f - halfSize, 1.0f),
        Vector4f(0.64f + halfSize, 0.64f + halfSize, 0.64f - halfSize, 1.0f),
        Vector4f(0.64f - halfSize, 0.64f - halfSize, 0.64f + halfSize, 1.0f),
        Vector4f(0.64f + halfSize, 0.64f - halfSize, 0.64f + halfSize, 1.0f),
        Vector4f(0.64f - halfSize, 0.64f + halfSize, 0.64f + halfSize, 1.0f),
        Vector4f(0.64f + halfSize, 0.64f + halfSize, 0.64f + halfSize, 1.0f)
    };
    
    int visibleCorners = 0;
    for (const auto& corner : corners) {
        Vector4f clip = mvp * corner;
        if (clip.w > 0) {
            Vector4f ndc = clip / clip.w;
            if (ndc.x >= -1.0f && ndc.x <= 1.0f &&
                ndc.y >= -1.0f && ndc.y <= 1.0f &&
                ndc.z >= -1.0f && ndc.z <= 1.0f) {
                visibleCorners++;
            }
        }
    }
    
    EXPECT_GT(visibleCorners, 0) << "At least some voxel corners should be visible";
}

// Test view presets produce expected camera positions
TEST_F(OrbitCameraTransformationTest, ViewPresetPositions) {
    camera->setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
    camera->setDistance(10.0f);
    
    // Test FRONT view
    {
        camera->setViewPreset(ViewPreset::FRONT);
        WorldCoordinates pos = camera->getPosition();
        
        EXPECT_NEAR(pos.x(), 0.0f, EPSILON) << "FRONT view X position";
        EXPECT_NEAR(pos.y(), 0.0f, EPSILON) << "FRONT view Y position";
        EXPECT_GT(pos.z(), 0.0f) << "FRONT view should be in +Z";
    }
    
    // Test TOP view
    {
        camera->setViewPreset(ViewPreset::TOP);
        WorldCoordinates pos = camera->getPosition();
        
        EXPECT_NEAR(pos.x(), 0.0f, EPSILON) << "TOP view X position";
        EXPECT_GT(pos.y(), 0.0f) << "TOP view should be in +Y";
        EXPECT_NEAR(pos.z(), 0.0f, EPSILON) << "TOP view Z position";
    }
    
    // Test RIGHT view
    {
        camera->setViewPreset(ViewPreset::RIGHT);
        WorldCoordinates pos = camera->getPosition();
        
        EXPECT_GT(pos.x(), 0.0f) << "RIGHT view should be in +X";
        EXPECT_NEAR(pos.y(), 0.0f, EPSILON) << "RIGHT view Y position";
        EXPECT_NEAR(pos.z(), 0.0f, EPSILON) << "RIGHT view Z position";
    }
}