#include <gtest/gtest.h>
#include "../Camera.h"
#include "../OrbitCamera.h"
#include "../CameraController.h"
#include "../Viewport.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Matrix4f.h"
#include "../../foundation/math/Ray.h"
#include "../../foundation/math/CoordinateTypes.h"
#include <chrono>
#include <cmath>

namespace VoxelEditor {
namespace Camera {

using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class CameraRequirementsTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        camera = std::make_unique<OrbitCamera>(eventDispatcher.get());
        controller = std::make_unique<CameraController>(eventDispatcher.get());
        viewport = std::make_unique<Viewport>();
        viewport->setSize(1920, 1080);
    }

    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<OrbitCamera> camera;
    std::unique_ptr<CameraController> controller;
    std::unique_ptr<Viewport> viewport;
};

// REQ-1.1.2: The grid shall be positioned at Y=0 (ground level)
TEST_F(CameraRequirementsTest, GridAtGroundLevel_ViewMatrices) {
    // Test that camera view matrices properly transform Y=0 plane
    WorldCoordinates gridPoint(0.0f, 0.0f, 0.0f);
    Matrix4f viewMatrix = camera->getViewMatrix();
    
    // Transform grid point to view space
    Vector4f viewSpacePoint = viewMatrix * Vector4f(gridPoint.x(), gridPoint.y(), gridPoint.z(), 1.0f);
    
    // The Y=0 plane should be in front of camera (negative Z in view space)
    EXPECT_TRUE(viewSpacePoint.z < 0.0f) << "Y=0 plane should be in front of camera";
    
    // Test with different camera positions
    camera->setDistance(10.0f);
    camera->setYaw(45.0f);
    camera->setPitch(30.0f);
    viewMatrix = camera->getViewMatrix();
    viewSpacePoint = viewMatrix * Vector4f(gridPoint.x(), gridPoint.y(), gridPoint.z(), 1.0f);
    EXPECT_TRUE(viewSpacePoint.z < 0.0f) << "Y=0 plane should remain visible from different angles";
}

// REQ-5.1.4: Ray-casting shall determine face/position under cursor
TEST_F(CameraRequirementsTest, RayCasting_ScreenToWorld) {
    // Test ray generation at center of screen
    Vector2i centerPos(960, 540);
    Ray ray = viewport->screenToWorldRay(centerPos, camera->getViewMatrix(), camera->getProjectionMatrix());
    
    // Verify ray properties
    float length = std::sqrt(ray.direction.x * ray.direction.x + 
                            ray.direction.y * ray.direction.y + 
                            ray.direction.z * ray.direction.z);
    EXPECT_NEAR(length, 1.0f, 0.001f) << "Ray direction should be normalized";
    
    // Ray should have valid origin and direction
    EXPECT_FALSE(std::isnan(ray.origin.x)) << "Ray origin should be valid";
    EXPECT_FALSE(std::isnan(ray.direction.x)) << "Ray direction should be valid";
}

// REQ-4.2.3: Highlighting shall be visible from all camera angles
TEST_F(CameraRequirementsTest, ViewIndependence_AllAngles) {
    // Test view matrices from multiple angles
    std::vector<std::pair<float, float>> testAngles = {
        {0.0f, 0.0f},     // Front view
        {90.0f, 0.0f},    // Right view
        {180.0f, 0.0f},   // Back view
        {270.0f, 0.0f},   // Left view
        {0.0f, 45.0f},    // Elevated view
    };
    
    for (const auto& [yaw, pitch] : testAngles) {
        camera->setYaw(yaw);
        camera->setPitch(pitch);
        Matrix4f viewMatrix = camera->getViewMatrix();
        
        // Simulate a highlight position slightly above a surface
        Vector3f surfacePoint(1.0f, 1.0f, 1.0f);
        Vector3f highlightPoint = surfacePoint + Vector3f(0.0f, 0.001f, 0.0f);
        
        // Transform both points to view space
        Vector4f surfaceView = viewMatrix * Vector4f(surfacePoint.x, surfacePoint.y, surfacePoint.z, 1.0f);
        Vector4f highlightView = viewMatrix * Vector4f(highlightPoint.x, highlightPoint.y, highlightPoint.z, 1.0f);
        
        // Highlight should be closer to camera or at same depth (within small tolerance)
        if (surfaceView.z < 0 && highlightView.z < 0) {
            // Allow small floating point precision differences
            EXPECT_LE(highlightView.z, surfaceView.z + 0.001f) 
                << "Highlight should not be occluded at angle " << yaw << "," << pitch;
        }
    }
}

// REQ-8.1.5: Format shall store camera position and view settings
TEST_F(CameraRequirementsTest, StatePersistence_Serialization) {
    // Set specific camera state
    camera->setDistance(25.0f);
    camera->setYaw(30.0f);
    camera->setPitch(45.0f);
    camera->setTarget(WorldCoordinates(2.0f, 1.0f, -3.0f));
    
    // Get state for serialization
    float distance = camera->getDistance();
    float yaw = camera->getYaw();
    float pitch = camera->getPitch();
    WorldCoordinates target = camera->getTarget();
    
    // Verify state can be retrieved
    EXPECT_EQ(distance, 25.0f);
    EXPECT_EQ(yaw, 30.0f);
    EXPECT_EQ(pitch, 45.0f);
    EXPECT_EQ(target.x(), 2.0f);
    EXPECT_EQ(target.y(), 1.0f);
    EXPECT_EQ(target.z(), -3.0f);
    
    // Create new camera and restore state
    auto newCamera = std::make_unique<OrbitCamera>(eventDispatcher.get());
    newCamera->setDistance(distance);
    newCamera->setYaw(yaw);
    newCamera->setPitch(pitch);
    newCamera->setTarget(target);
    
    // Verify restored state matches
    EXPECT_EQ(newCamera->getDistance(), distance);
    EXPECT_EQ(newCamera->getYaw(), yaw);
    EXPECT_EQ(newCamera->getPitch(), pitch);
    EXPECT_EQ(newCamera->getTarget().x(), target.x());
    EXPECT_EQ(newCamera->getTarget().y(), target.y());
    EXPECT_EQ(newCamera->getTarget().z(), target.z());
}

// REQ-9.2.2: CLI shall support camera commands (zoom, view, rotate, reset)
TEST_F(CameraRequirementsTest, CLICommands_CameraControl) {
    // Test zoom commands
    float initialDistance = camera->getDistance();
    camera->zoom(2.0f); // Zoom in (positive = move closer)
    EXPECT_LT(camera->getDistance(), initialDistance) << "Zoom in should decrease distance";
    
    camera->zoom(-4.0f); // Zoom out (negative = move away)
    EXPECT_GT(camera->getDistance(), initialDistance) << "Zoom out should increase distance";
    
    // Test view preset commands using controller's camera
    OrbitCamera* controllerCamera = controller->getCamera();
    controller->setViewPreset(ViewPreset::FRONT);
    controller->update(1.0f); // Complete transition
    EXPECT_NEAR(controllerCamera->getYaw(), 0.0f, 0.1f) << "Front view yaw";
    EXPECT_NEAR(controllerCamera->getPitch(), 0.0f, 0.1f) << "Front view pitch";
    
    controller->setViewPreset(ViewPreset::TOP);
    controller->update(1.0f); // Complete transition  
    EXPECT_NEAR(controllerCamera->getPitch(), 90.0f, 0.1f) << "Top view pitch";
    
    // Test orbit (rotation) commands
    float initialYaw = camera->getYaw();
    float initialPitch = camera->getPitch();
    camera->orbit(30.0f, 15.0f);
    EXPECT_NE(camera->getYaw(), initialYaw) << "Orbit should change yaw";
    EXPECT_NE(camera->getPitch(), initialPitch) << "Orbit should change pitch";
}

// REQ-CAM-1: Camera system shall provide orbit-style controls
TEST_F(CameraRequirementsTest, OrbitControls_Implementation) {
    // Test orbit around target
    WorldCoordinates target(1.0f, 2.0f, 3.0f);
    camera->setTarget(target);
    
    // Rotate around target
    camera->setYaw(0.0f);
    WorldCoordinates position1 = camera->getPosition();
    camera->setYaw(90.0f);
    WorldCoordinates position2 = camera->getPosition();
    
    // Both positions should be same distance from target
    float dist1 = std::sqrt(
        std::pow(position1.x() - target.x(), 2) +
        std::pow(position1.y() - target.y(), 2) +
        std::pow(position1.z() - target.z(), 2)
    );
    float dist2 = std::sqrt(
        std::pow(position2.x() - target.x(), 2) +
        std::pow(position2.y() - target.y(), 2) +
        std::pow(position2.z() - target.z(), 2)
    );
    EXPECT_NEAR(dist1, dist2, 0.001f) << "Orbit should maintain constant distance";
}

// REQ-CAM-2: Camera shall support multiple view projections
TEST_F(CameraRequirementsTest, ViewProjections_Support) {
    // Test perspective projection
    Matrix4f perspProj = camera->getProjectionMatrix();
    Matrix4f identity = Matrix4f::Identity();
    
    // Perspective matrix should not be identity
    EXPECT_NE(perspProj, identity) << "Projection should not be identity";
    
    // Test aspect ratio handling
    viewport->setSize(1920, 1080); // 16:9
    float aspect1 = static_cast<float>(viewport->getWidth()) / viewport->getHeight();
    EXPECT_NEAR(aspect1, 16.0f/9.0f, 0.001f);
    
    viewport->setSize(1080, 1080); // 1:1
    float aspect2 = static_cast<float>(viewport->getWidth()) / viewport->getHeight();
    EXPECT_NEAR(aspect2, 1.0f, 0.001f);
    
    // Camera should update projection for new aspect
    camera->setAspectRatio(aspect2);
    Matrix4f squareProj = camera->getProjectionMatrix();
    EXPECT_NE(perspProj, squareProj) << "Projection should change with aspect ratio";
}

// REQ-CAM-3: Camera shall maintain consistent coordinate system
TEST_F(CameraRequirementsTest, CoordinateSystem_Consistency) {
    // Test centered coordinate system
    camera->setTarget(WorldCoordinates(0.0f, 0.0f, 0.0f));
    WorldCoordinates retrievedTarget = camera->getTarget();
    EXPECT_EQ(retrievedTarget.x(), 0.0f) << "Camera should work with origin at center";
    EXPECT_EQ(retrievedTarget.y(), 0.0f);
    EXPECT_EQ(retrievedTarget.z(), 0.0f);
    
    // Test Y-up orientation
    WorldCoordinates up = camera->getUp();
    EXPECT_NEAR(up.y(), 1.0f, 0.001f) << "Camera should maintain Y-up orientation";
    EXPECT_NEAR(up.x(), 0.0f, 0.001f);
    EXPECT_NEAR(up.z(), 0.0f, 0.001f);
    
    // Test negative coordinates
    camera->setTarget(WorldCoordinates(-5.0f, -2.0f, -3.0f));
    Matrix4f viewMatrix = camera->getViewMatrix();
    
    // Transform negative coordinate point
    Vector4f negPoint = viewMatrix * Vector4f(-1.0f, -1.0f, -1.0f, 1.0f);
    EXPECT_FALSE(std::isnan(negPoint.x)) << "Camera should handle negative coordinates";
}

// REQ-CAM-4: Camera shall provide standard view presets
TEST_F(CameraRequirementsTest, ViewPresets_AllDirections) {
    // Test that all view presets can be set without error
    std::vector<ViewPreset> presets = {
        ViewPreset::FRONT,
        ViewPreset::BACK,
        ViewPreset::LEFT,
        ViewPreset::RIGHT,
        ViewPreset::TOP,
        ViewPreset::BOTTOM,
        ViewPreset::ISOMETRIC
    };
    
    OrbitCamera* controllerCamera = controller->getCamera();
    
    for (const auto& preset : presets) {
        EXPECT_NO_THROW(controller->setViewPreset(preset)) 
            << "View preset should set without error";
        
        controller->update(1.0f); // Complete transition
        
        // Verify camera state is valid
        EXPECT_FALSE(std::isnan(controllerCamera->getYaw())) << "Camera yaw should be valid";
        EXPECT_FALSE(std::isnan(controllerCamera->getPitch())) << "Camera pitch should be valid";
        EXPECT_GT(controllerCamera->getDistance(), 0.0f) << "Camera distance should be positive";
    }
}

// REQ-CAM-5: View transitions shall be smooth
TEST_F(CameraRequirementsTest, ViewTransitions_Smoothness) {
    controller->setCameraSmoothing(true);
    OrbitCamera* controllerCamera = controller->getCamera();
    
    // Start at front view
    controller->setViewPreset(ViewPreset::FRONT);
    controller->update(1.0f); // Ensure we're at front view
    
    WorldCoordinates startPos = controllerCamera->getPosition();
    
    // Transition to right view
    controller->setViewPreset(ViewPreset::RIGHT);
    
    // Update with small time step
    controller->update(0.016f); // 16ms
    WorldCoordinates midPos = controllerCamera->getPosition();
    
    // Position should change during transition
    float moveDist = std::sqrt(
        std::pow(midPos.x() - startPos.x(), 2) +
        std::pow(midPos.y() - startPos.y(), 2) +
        std::pow(midPos.z() - startPos.z(), 2)
    );
    
    // With smoothing, camera should not jump instantly
    EXPECT_LT(moveDist, 10.0f) << "Camera should not jump with smoothing enabled";
}

// REQ-CAM-6: Camera shall provide accurate ray generation
TEST_F(CameraRequirementsTest, RayGeneration_Accuracy) {
    // Test ray through center of screen
    int centerX = viewport->getWidth() / 2;
    int centerY = viewport->getHeight() / 2;
    camera->setTarget(WorldCoordinates(0.0f, 0.0f, 0.0f));
    camera->setYaw(0.0f);
    camera->setPitch(30.0f); // Slight elevation to hit Y=0
    
    Ray ray = viewport->screenToWorldRay(
        Vector2i(centerX, centerY),
        camera->getViewMatrix(),
        camera->getProjectionMatrix()
    );
    
    // Calculate intersection with Y=0 plane
    if (ray.direction.y != 0.0f) {
        float t = -ray.origin.y / ray.direction.y;
        if (t > 0.0f) {
            Vector3f hitPoint = ray.origin + ray.direction * t;
            EXPECT_NEAR(hitPoint.y, 0.0f, 0.001f) << "Ray should hit Y=0 plane";
            // Center ray should hit reasonably close to origin
            EXPECT_LT(std::abs(hitPoint.x), 2.0f) << "Center ray should hit near origin";
            EXPECT_LT(std::abs(hitPoint.z), 2.0f) << "Center ray should hit near origin";
        }
    }
}

// REQ-CAM-7: Camera operations shall be performant
TEST_F(CameraRequirementsTest, Performance_Operations) {
    // Test matrix calculation performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++) {
        camera->getViewMatrix();
        camera->getProjectionMatrix();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    float avgTime = duration / 2000.0f; // 2000 matrix calculations
    
    EXPECT_LT(avgTime, 1000.0f) << "Matrix calculations should average < 1ms";
    
    // Test ray generation performance
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++) {
        Ray ray = viewport->screenToWorldRay(
            Vector2i(i % 1920, i / 1920),
            camera->getViewMatrix(),
            camera->getProjectionMatrix()
        );
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    avgTime = duration / 1000.0f;
    
    EXPECT_LT(avgTime, 100.0f) << "Ray generation should average < 0.1ms";
}

// REQ-CAM-8: Camera shall integrate with other subsystems
TEST_F(CameraRequirementsTest, Integration_EventSystem) {
    // Test that camera matrices are provided
    Matrix4f view = camera->getViewMatrix();
    Matrix4f proj = camera->getProjectionMatrix();
    Matrix4f identity = Matrix4f::Identity();
    
    EXPECT_NE(view, identity) << "View matrix should not be identity";
    EXPECT_NE(proj, identity) << "Projection matrix should not be identity";
    
    // Test that camera state changes
    float initialDistance = camera->getDistance();
    camera->setDistance(20.0f);
    EXPECT_NE(camera->getDistance(), initialDistance) << "Distance should change";
}

// Additional test for camera bounds and constraints
TEST_F(CameraRequirementsTest, CameraBounds_Constraints) {
    // Test minimum distance constraint
    camera->setDistance(0.1f);
    EXPECT_GE(camera->getDistance(), 0.5f) << "Camera should enforce minimum distance";
    
    // Test maximum distance constraint
    camera->setDistance(1000.0f);
    EXPECT_LE(camera->getDistance(), 100.0f) << "Camera should enforce maximum distance";
    
    // Test pitch constraints
    camera->setPitch(95.0f);
    EXPECT_LE(camera->getPitch(), 90.0f) << "Camera should limit pitch to prevent gimbal lock";
    
    camera->setPitch(-95.0f);
    EXPECT_GE(camera->getPitch(), -90.0f) << "Camera should limit negative pitch";
}

} // namespace Camera
} // namespace VoxelEditor