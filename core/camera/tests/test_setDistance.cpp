#include <gtest/gtest.h>
#include "../OrbitCamera.h"
#include "../CameraController.h"
#include "../../foundation/events/EventDispatcher.h"
#include <cmath>
#include <limits>

using namespace VoxelEditor::Camera;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class SetDistanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        camera = std::make_unique<OrbitCamera>(eventDispatcher.get());
        controller = std::make_unique<CameraController>(eventDispatcher.get());
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<OrbitCamera> camera;
    std::unique_ptr<CameraController> controller;
};

// Test basic setDistance functionality
TEST_F(SetDistanceTest, BasicSetDistance) {
    // Default distance should be 5.0
    EXPECT_FLOAT_EQ(camera->getDistance(), 5.0f);
    
    // Set various distances
    camera->setDistance(10.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 10.0f);
    
    camera->setDistance(2.5f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 2.5f);
    
    camera->setDistance(50.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 50.0f);
}

// Test distance constraints
TEST_F(SetDistanceTest, DistanceConstraints) {
    // Default constraints: min=0.5, max=100
    EXPECT_FLOAT_EQ(camera->getMinDistance(), 0.5f);
    EXPECT_FLOAT_EQ(camera->getMaxDistance(), 100.0f);
    
    // Test clamping to minimum
    camera->setDistance(0.1f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 0.5f);
    
    camera->setDistance(-1.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 0.5f);
    
    // Test clamping to maximum
    camera->setDistance(150.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 100.0f);
    
    camera->setDistance(1000.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 100.0f);
}

// Test custom distance constraints
TEST_F(SetDistanceTest, CustomDistanceConstraints) {
    // Set custom constraints
    camera->setDistanceConstraints(2.0f, 20.0f);
    
    // Verify constraints were set
    EXPECT_FLOAT_EQ(camera->getMinDistance(), 2.0f);
    EXPECT_FLOAT_EQ(camera->getMaxDistance(), 20.0f);
    
    // Test that current distance is clamped if needed
    camera->setDistance(1.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 2.0f);
    
    camera->setDistance(25.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 20.0f);
    
    // Test valid distance within new constraints
    camera->setDistance(10.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 10.0f);
}

// Test edge cases
TEST_F(SetDistanceTest, EdgeCases) {
    // Zero distance (should clamp to minimum)
    camera->setDistance(0.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 0.5f);
    
    // Negative distance (should clamp to minimum)
    camera->setDistance(-10.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 0.5f);
    
    // Very large distance
    camera->setDistance(std::numeric_limits<float>::max());
    EXPECT_FLOAT_EQ(camera->getDistance(), 100.0f);
    
    // NaN (should maintain current distance or clamp to valid range)
    float prevDistance = camera->getDistance();
    camera->setDistance(std::numeric_limits<float>::quiet_NaN());
    // Implementation might handle NaN differently, but distance should be valid
    EXPECT_TRUE(std::isfinite(camera->getDistance()));
    EXPECT_GE(camera->getDistance(), camera->getMinDistance());
    EXPECT_LE(camera->getDistance(), camera->getMaxDistance());
}

// Test that setDistance updates camera position
TEST_F(SetDistanceTest, UpdatesCameraPosition) {
    // Set initial angles
    camera->setYaw(0.0f);
    camera->setPitch(0.0f);
    
    // At yaw=0, pitch=0, camera should be along positive Z axis
    camera->setDistance(10.0f);
    WorldCoordinates pos = camera->getPosition();
    EXPECT_NEAR(pos.x(), 0.0f, 0.001f);
    EXPECT_NEAR(pos.y(), 0.0f, 0.001f);
    EXPECT_NEAR(pos.z(), 10.0f, 0.001f);
    
    // Change distance
    camera->setDistance(5.0f);
    pos = camera->getPosition();
    EXPECT_NEAR(pos.x(), 0.0f, 0.001f);
    EXPECT_NEAR(pos.y(), 0.0f, 0.001f);
    EXPECT_NEAR(pos.z(), 5.0f, 0.001f);
}

// Test setDistance with different camera orientations
TEST_F(SetDistanceTest, WithDifferentOrientations) {
    // Test with yaw=90 degrees (camera on positive X axis)
    camera->setYaw(90.0f);
    camera->setPitch(0.0f);
    camera->setDistance(10.0f);
    
    WorldCoordinates pos = camera->getPosition();
    EXPECT_NEAR(pos.x(), 10.0f, 0.001f);
    EXPECT_NEAR(pos.y(), 0.0f, 0.001f);
    EXPECT_NEAR(pos.z(), 0.0f, 0.001f);
    
    // Change distance with same orientation
    camera->setDistance(5.0f);
    pos = camera->getPosition();
    EXPECT_NEAR(pos.x(), 5.0f, 0.001f);
    EXPECT_NEAR(pos.y(), 0.0f, 0.001f);
    EXPECT_NEAR(pos.z(), 0.0f, 0.001f);
}

// Test rapid setDistance calls
TEST_F(SetDistanceTest, RapidSetDistanceCalls) {
    // Simulate rapid distance changes
    std::vector<float> distances = {5.0f, 10.0f, 2.0f, 15.0f, 7.5f, 20.0f, 3.0f};
    
    for (float dist : distances) {
        camera->setDistance(dist);
        EXPECT_FLOAT_EQ(camera->getDistance(), dist);
    }
}

// Test setDistance preserves other camera properties
TEST_F(SetDistanceTest, PreservesOtherProperties) {
    // Set specific camera state
    camera->setYaw(45.0f);
    camera->setPitch(30.0f);
    camera->setTarget(WorldCoordinates(1.0f, 2.0f, 3.0f));
    
    float originalYaw = camera->getYaw();
    float originalPitch = camera->getPitch();
    WorldCoordinates originalTarget = camera->getTarget();
    
    // Change distance
    camera->setDistance(15.0f);
    
    // Verify other properties unchanged
    EXPECT_FLOAT_EQ(camera->getYaw(), originalYaw);
    EXPECT_FLOAT_EQ(camera->getPitch(), originalPitch);
    EXPECT_EQ(camera->getTarget(), originalTarget);
}

// Test setDistance with smoothing enabled
TEST_F(SetDistanceTest, WithSmoothingEnabled) {
    // Enable smoothing
    camera->setSmoothing(true);
    camera->setSmoothFactor(0.1f);
    
    float initialDistance = camera->getDistance();
    EXPECT_FLOAT_EQ(initialDistance, 5.0f);
    
    // Note: setDistance directly sets the distance even with smoothing enabled
    // Smoothing only affects zoom() method and view preset changes
    camera->setDistance(10.0f);
    
    // setDistance should immediately change the distance
    EXPECT_FLOAT_EQ(camera->getDistance(), 10.0f);
    
    // Test zoom method with smoothing
    camera->setDistance(5.0f); // Reset
    camera->zoom(2.0f); // Zoom in with delta
    
    // After zoom with smoothing, distance may not change immediately
    // (depends on implementation)
    camera->update(0.016f); // 60fps frame time
    float afterUpdate = camera->getDistance();
    
    // Distance should have changed after update
    EXPECT_NE(afterUpdate, 5.0f);
}

// Test setDistance through CameraController
TEST_F(SetDistanceTest, ThroughCameraController) {
    OrbitCamera* controllerCamera = controller->getCamera();
    
    // Test basic setDistance through controller's camera
    controllerCamera->setDistance(15.0f);
    EXPECT_FLOAT_EQ(controllerCamera->getDistance(), 15.0f);
    
    // Test constraints still apply
    controllerCamera->setDistance(0.1f);
    EXPECT_FLOAT_EQ(controllerCamera->getDistance(), 0.5f);
    
    controllerCamera->setDistance(200.0f);
    EXPECT_FLOAT_EQ(controllerCamera->getDistance(), 100.0f);
}

// Test setDistance impact on view matrix
TEST_F(SetDistanceTest, ViewMatrixUpdate) {
    // Get initial view matrix
    camera->setDistance(5.0f);
    Matrix4f viewMatrix1 = camera->getViewMatrix();
    
    // Change distance
    camera->setDistance(10.0f);
    Matrix4f viewMatrix2 = camera->getViewMatrix();
    
    // View matrices should be different
    bool matricesDifferent = false;
    for (int i = 0; i < 16; ++i) {
        if (std::abs(viewMatrix1[i] - viewMatrix2[i]) > 0.001f) {
            matricesDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(matricesDifferent);
}

// Test setDistance with very small increments
TEST_F(SetDistanceTest, SmallIncrements) {
    float startDistance = 5.0f;
    camera->setDistance(startDistance);
    
    // Apply many small increments
    for (int i = 0; i < 100; ++i) {
        float currentDistance = camera->getDistance();
        float newDistance = currentDistance + 0.01f;
        camera->setDistance(newDistance);
        EXPECT_FLOAT_EQ(camera->getDistance(), newDistance);
    }
    
    // Should have accumulated to approximately 6.0
    EXPECT_NEAR(camera->getDistance(), 6.0f, 0.001f);
}

// Test setDistance after view preset changes
TEST_F(SetDistanceTest, AfterViewPresetChange) {
    // Set initial distance
    camera->setDistance(7.5f);
    
    // Change to different view preset (which sets its own distance)
    camera->setViewPreset(ViewPreset::FRONT);
    // Front view sets distance to 10.0
    EXPECT_FLOAT_EQ(camera->getDistance(), 10.0f);
    
    // Now test setDistance works after preset change
    camera->setDistance(5.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 5.0f);
    
    // Change to another preset
    camera->setViewPreset(ViewPreset::ISOMETRIC);
    // Isometric sets distance to 12.0
    EXPECT_FLOAT_EQ(camera->getDistance(), 12.0f);
    
    // setDistance should still work
    camera->setDistance(8.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 8.0f);
}