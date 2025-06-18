#include <gtest/gtest.h>
#include "../OrbitCamera.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor::Camera;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class OrbitCameraTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        camera = std::make_unique<OrbitCamera>(eventDispatcher.get());
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<OrbitCamera> camera;
};

TEST_F(OrbitCameraTest, DefaultConstruction) {
    EXPECT_FLOAT_EQ(camera->getDistance(), 5.0f);
    EXPECT_FLOAT_EQ(camera->getYaw(), 0.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), 0.0f);
    EXPECT_EQ(camera->getTarget(), WorldCoordinates(0.0f, 0.0f, 0.0f));
    
    // Position should be calculated based on distance and angles
    Vector3f expectedPos(0.0f, 0.0f, 5.0f);
    EXPECT_NEAR(camera->getPosition().x(), expectedPos.x, 0.001f);
    EXPECT_NEAR(camera->getPosition().y(), expectedPos.y, 0.001f);
    EXPECT_NEAR(camera->getPosition().z(), expectedPos.z, 0.001f);
}

TEST_F(OrbitCameraTest, DistanceControl) {
    camera->setDistance(10.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 10.0f);
    
    // Position should update based on new distance
    Vector3f expectedPos(0.0f, 0.0f, 10.0f);
    EXPECT_NEAR(camera->getPosition().x(), expectedPos.x, 0.001f);
    EXPECT_NEAR(camera->getPosition().y(), expectedPos.y, 0.001f);
    EXPECT_NEAR(camera->getPosition().z(), expectedPos.z, 0.001f);
}

TEST_F(OrbitCameraTest, DistanceConstraints) {
    camera->setDistanceConstraints(2.0f, 20.0f);
    
    EXPECT_FLOAT_EQ(camera->getMinDistance(), 2.0f);
    EXPECT_FLOAT_EQ(camera->getMaxDistance(), 20.0f);
    
    // Test constraint enforcement
    camera->setDistance(1.0f); // Below minimum
    EXPECT_FLOAT_EQ(camera->getDistance(), 2.0f);
    
    camera->setDistance(25.0f); // Above maximum
    EXPECT_FLOAT_EQ(camera->getDistance(), 20.0f);
    
    // Valid distance should work
    camera->setDistance(10.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 10.0f);
}

TEST_F(OrbitCameraTest, AngleControl) {
    camera->setYaw(45.0f);
    camera->setPitch(30.0f);
    
    EXPECT_FLOAT_EQ(camera->getYaw(), 45.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), 30.0f);
    
    // Test combined angle setting
    camera->setOrbitAngles(90.0f, -45.0f);
    EXPECT_FLOAT_EQ(camera->getYaw(), 90.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), -45.0f);
}

TEST_F(OrbitCameraTest, PitchConstraints) {
    camera->setPitchConstraints(-60.0f, 60.0f);
    
    EXPECT_FLOAT_EQ(camera->getMinPitch(), -60.0f);
    EXPECT_FLOAT_EQ(camera->getMaxPitch(), 60.0f);
    
    // Test constraint enforcement
    camera->setPitch(-90.0f); // Below minimum
    EXPECT_FLOAT_EQ(camera->getPitch(), -60.0f);
    
    camera->setPitch(90.0f); // Above maximum
    EXPECT_FLOAT_EQ(camera->getPitch(), 60.0f);
}

TEST_F(OrbitCameraTest, OrbitControl) {
    WorldCoordinates initialPos = camera->getPosition();
    
    // Orbit horizontally (yaw)
    camera->orbit(45.0f, 0.0f);
    WorldCoordinates afterYaw = camera->getPosition();
    
    // X should change, Y should stay same, Z might change
    EXPECT_NE(initialPos.x(), afterYaw.x());
    EXPECT_EQ(initialPos.y(), afterYaw.y());
    
    // Orbit vertically (pitch)
    camera->orbit(0.0f, 30.0f);
    WorldCoordinates afterPitch = camera->getPosition();
    
    // Y should change after pitch
    EXPECT_NE(afterYaw.y(), afterPitch.y());
}

TEST_F(OrbitCameraTest, ZoomControl) {
    float initialDistance = camera->getDistance();
    
    camera->zoom(1.0f); // Zoom in
    EXPECT_LT(camera->getDistance(), initialDistance);
    
    camera->zoom(-2.0f); // Zoom out
    EXPECT_GT(camera->getDistance(), initialDistance);
}

TEST_F(OrbitCameraTest, PanControl) {
    WorldCoordinates initialTarget = camera->getTarget();
    
    // Pan right and up
    camera->pan(Vector3f(1.0f, 1.0f, 0.0f));
    WorldCoordinates newTarget = camera->getTarget();
    
    EXPECT_NE(initialTarget, newTarget);
    
    // Camera should maintain same relative position to target
    EXPECT_FLOAT_EQ(camera->getDistance(), 5.0f);
}

TEST_F(OrbitCameraTest, ViewPresets) {
    // Test each view preset
    camera->setViewPreset(ViewPreset::FRONT);
    EXPECT_FLOAT_EQ(camera->getYaw(), 0.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), 0.0f);
    
    camera->setViewPreset(ViewPreset::BACK);
    EXPECT_FLOAT_EQ(camera->getYaw(), 180.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), 0.0f);
    
    camera->setViewPreset(ViewPreset::LEFT);
    EXPECT_FLOAT_EQ(camera->getYaw(), -90.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), 0.0f);
    
    camera->setViewPreset(ViewPreset::RIGHT);
    EXPECT_FLOAT_EQ(camera->getYaw(), 90.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), 0.0f);
    
    camera->setViewPreset(ViewPreset::TOP);
    EXPECT_FLOAT_EQ(camera->getYaw(), 0.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), 90.0f);
    
    camera->setViewPreset(ViewPreset::BOTTOM);
    EXPECT_FLOAT_EQ(camera->getYaw(), 0.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), -90.0f);
    
    camera->setViewPreset(ViewPreset::ISOMETRIC);
    EXPECT_FLOAT_EQ(camera->getYaw(), 45.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), 35.26f);
}

TEST_F(OrbitCameraTest, IsometricViewMatrixValidation) {
    // Set isometric view preset
    camera->setViewPreset(ViewPreset::ISOMETRIC);
    camera->setTarget(WorldCoordinates(0, 0, 0));
    camera->setDistance(10.0f);
    
    // Get the view matrix
    Matrix4f view = camera->getViewMatrix();
    
    // In isometric view:
    // - Yaw = 45 degrees (π/4 radians)
    // - Pitch = 35.26 degrees (atan(1/sqrt(2)) radians) 
    // This creates the classic isometric angle
    
    // Calculate expected camera position
    float yawRad = 45.0f * M_PI / 180.0f;
    float pitchRad = 35.26f * M_PI / 180.0f;
    float distance = 10.0f;
    
    Vector3f expectedPos(
        distance * cosf(pitchRad) * sinf(yawRad),
        distance * sinf(pitchRad),
        distance * cosf(pitchRad) * cosf(yawRad)
    );
    
    WorldCoordinates actualPos = camera->getPosition();
    EXPECT_NEAR(actualPos.x(), expectedPos.x, 0.01f);
    EXPECT_NEAR(actualPos.y(), expectedPos.y, 0.01f);
    EXPECT_NEAR(actualPos.z(), expectedPos.z, 0.01f);
    
    // Test that parallel lines remain parallel (orthographic property)
    // Transform two parallel edges of a cube
    Vector3f cubeEdge1Start(0, 0, 0);
    Vector3f cubeEdge1End(1, 0, 0);
    Vector3f cubeEdge2Start(0, 1, 0);
    Vector3f cubeEdge2End(1, 1, 0);
    
    // Transform to view space
    Vector3f viewEdge1Start = view * cubeEdge1Start;
    Vector3f viewEdge1End = view * cubeEdge1End;
    Vector3f viewEdge2Start = view * cubeEdge2Start;
    Vector3f viewEdge2End = view * cubeEdge2End;
    
    // Direction vectors in view space
    Vector3f dir1 = (viewEdge1End - viewEdge1Start).normalized();
    Vector3f dir2 = (viewEdge2End - viewEdge2Start).normalized();
    
    // Parallel lines should have same direction (or opposite)
    float dot = dir1.dot(dir2);
    EXPECT_NEAR(std::abs(dot), 1.0f, 0.001f);
    
    // Verify the view matrix maintains right-handed coordinate system
    // Extract basis vectors from view matrix (rows for row-major)
    Vector3f right(view.m[0], view.m[1], view.m[2]);
    Vector3f up(view.m[4], view.m[5], view.m[6]);
    Vector3f forward(view.m[8], view.m[9], view.m[10]);
    
    // Should be orthonormal
    EXPECT_NEAR(right.length(), 1.0f, 0.001f);
    EXPECT_NEAR(up.length(), 1.0f, 0.001f);
    EXPECT_NEAR(forward.length(), 1.0f, 0.001f);
    
    // Should be orthogonal
    EXPECT_NEAR(right.dot(up), 0.0f, 0.001f);
    EXPECT_NEAR(right.dot(forward), 0.0f, 0.001f);
    EXPECT_NEAR(up.dot(forward), 0.0f, 0.001f);
    
    // Should form right-handed system
    Vector3f cross = right.cross(up);
    EXPECT_NEAR(cross.dot(forward), 1.0f, 0.001f);
}

TEST_F(OrbitCameraTest, FocusOnPoint) {
    WorldCoordinates focusPoint(10.0f, 5.0f, 15.0f);
    camera->focusOn(focusPoint, 8.0f);
    
    EXPECT_EQ(camera->getTarget(), focusPoint);
    EXPECT_FLOAT_EQ(camera->getDistance(), 8.0f);
    
    // Test focus without specifying distance
    camera->focusOn(WorldCoordinates(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(camera->getTarget(), WorldCoordinates(0.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(camera->getDistance(), 8.0f); // Should maintain current distance
}

TEST_F(OrbitCameraTest, FrameBox) {
    Vector3f minBounds(-5.0f, -3.0f, -2.0f);
    Vector3f maxBounds(5.0f, 3.0f, 2.0f);
    
    camera->frameBox(WorldCoordinates(minBounds.x, minBounds.y, minBounds.z), 
                     WorldCoordinates(maxBounds.x, maxBounds.y, maxBounds.z));
    
    // Target should be at center of box
    Vector3f expectedCenter = (minBounds + maxBounds) * 0.5f;
    EXPECT_EQ(camera->getTarget(), WorldCoordinates(expectedCenter.x, expectedCenter.y, expectedCenter.z));
    
    // Distance should be calculated to frame the entire box
    EXPECT_GT(camera->getDistance(), 0.0f);
}

TEST_F(OrbitCameraTest, SensitivitySettings) {
    camera->setPanSensitivity(0.5f);
    camera->setRotateSensitivity(2.0f);
    camera->setZoomSensitivity(1.5f);
    
    EXPECT_FLOAT_EQ(camera->getPanSensitivity(), 0.5f);
    EXPECT_FLOAT_EQ(camera->getRotateSensitivity(), 2.0f);
    EXPECT_FLOAT_EQ(camera->getZoomSensitivity(), 1.5f);
    
    // Test that sensitivity affects operations
    WorldCoordinates initialTarget = camera->getTarget();
    camera->pan(Vector3f(1.0f, 0.0f, 0.0f));
    WorldCoordinates targetAfterPan = camera->getTarget();
    
    // Pan amount should be scaled by sensitivity
    Vector3f panDelta = Vector3f(targetAfterPan.x() - initialTarget.x(), 
                                  targetAfterPan.y() - initialTarget.y(), 
                                  targetAfterPan.z() - initialTarget.z());
    EXPECT_LT(panDelta.length(), 1.0f); // Should be less than 1.0 due to 0.5 sensitivity
}

TEST_F(OrbitCameraTest, SmoothingSystem) {
    camera->setSmoothing(true);
    camera->setSmoothFactor(0.5f);
    
    EXPECT_TRUE(camera->isSmoothing());
    EXPECT_FLOAT_EQ(camera->getSmoothFactor(), 0.5f);
    
    // Test smoothing clamps factor
    camera->setSmoothFactor(2.0f);
    EXPECT_FLOAT_EQ(camera->getSmoothFactor(), 1.0f);
    
    camera->setSmoothFactor(-0.5f);
    EXPECT_FLOAT_EQ(camera->getSmoothFactor(), 0.01f);
}

TEST_F(OrbitCameraTest, SmoothingBehavior) {
    camera->setSmoothing(true);
    camera->setSmoothFactor(0.1f); // Slow smoothing
    
    float initialDistance = camera->getDistance();
    camera->zoom(5.0f); // Large zoom
    
    // Distance should not change immediately with smoothing
    EXPECT_FLOAT_EQ(camera->getDistance(), initialDistance);
    
    // After update, should move towards target
    camera->update(0.016f); // 60fps delta
    EXPECT_NE(camera->getDistance(), initialDistance);
    
    // Should not reach target immediately (should still be closer to initial than target)
    EXPECT_GT(camera->getDistance(), initialDistance * 0.5f); // Should be more than halfway between initial and target
}

TEST_F(OrbitCameraTest, UpdateWithoutSmoothing) {
    camera->setSmoothing(false);
    
    float initialDistance = camera->getDistance();
    camera->zoom(2.0f);
    
    // Should change immediately without smoothing
    EXPECT_NE(camera->getDistance(), initialDistance);
    
    // Update should not change anything
    float distanceAfterZoom = camera->getDistance();
    camera->update(0.016f);
    EXPECT_FLOAT_EQ(camera->getDistance(), distanceAfterZoom);
}

TEST_F(OrbitCameraTest, PositionCalculation) {
    // Test that position is correctly calculated from angles and distance
    camera->setOrbitAngles(90.0f, 0.0f); // Looking from +X axis
    camera->setDistance(10.0f);
    
    WorldCoordinates pos = camera->getPosition();
    EXPECT_NEAR(pos.x(), 10.0f, 0.001f);
    EXPECT_NEAR(pos.y(), 0.0f, 0.001f);
    EXPECT_NEAR(pos.z(), 0.0f, 0.001f);
    
    camera->setOrbitAngles(0.0f, 90.0f); // Looking from +Y axis
    pos = camera->getPosition();
    EXPECT_NEAR(pos.x(), 0.0f, 0.001f);
    EXPECT_NEAR(pos.y(), 10.0f, 0.001f);
    EXPECT_NEAR(pos.z(), 0.0f, 0.001f);
}

TEST_F(OrbitCameraTest, TargetOverride) {
    WorldCoordinates originalTarget = camera->getTarget();
    WorldCoordinates newTarget(5.0f, 5.0f, 5.0f);
    
    camera->setTarget(newTarget);
    
    EXPECT_EQ(camera->getTarget(), newTarget);
    
    // Position should be recalculated relative to new target
    WorldCoordinates pos = camera->getPosition();
    Vector3f offset(pos.x() - newTarget.x(), pos.y() - newTarget.y(), pos.z() - newTarget.z());
    EXPECT_NEAR(offset.length(), camera->getDistance(), 0.001f);
}

TEST_F(OrbitCameraTest, EdgeCaseAngles) {
    // Test wrap-around for yaw
    camera->setYaw(720.0f); // Multiple rotations
    EXPECT_FLOAT_EQ(camera->getYaw(), 720.0f); // Should store actual value
    
    // Test extreme pitch values get clamped
    camera->setPitch(180.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), camera->getMaxPitch());
    
    camera->setPitch(-180.0f);
    EXPECT_FLOAT_EQ(camera->getPitch(), camera->getMinPitch());
}

TEST_F(OrbitCameraTest, ZeroDistance) {
    camera->setDistanceConstraints(0.0f, 100.0f);
    camera->setDistance(0.0f);
    
    // Should handle zero distance gracefully
    EXPECT_FLOAT_EQ(camera->getDistance(), 0.0f);
    EXPECT_EQ(camera->getPosition(), camera->getTarget());
}