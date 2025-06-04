#include <gtest/gtest.h>
#include "../OrbitCamera.h"
#include "../CameraController.h"
#include "../../foundation/events/EventDispatcher.h"
#include <cmath>
#include <vector>

using namespace VoxelEditor::Camera;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class ZoomFunctionalityTest : public ::testing::Test {
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

// Test basic zoom functionality
TEST_F(ZoomFunctionalityTest, BasicZoomIn) {
    float initialDistance = camera->getDistance();
    EXPECT_FLOAT_EQ(initialDistance, 5.0f); // Default distance
    
    // Zoom in by factor of 2
    camera->setDistance(initialDistance / 2.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 2.5f);
}

TEST_F(ZoomFunctionalityTest, BasicZoomOut) {
    float initialDistance = camera->getDistance();
    
    // Zoom out by factor of 0.5
    camera->setDistance(initialDistance / 0.5f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 10.0f);
}

// Test zoom constraints
TEST_F(ZoomFunctionalityTest, ZoomConstraints) {
    // Default constraints: min=0.5, max=100
    camera->setDistance(0.1f); // Below minimum
    EXPECT_FLOAT_EQ(camera->getDistance(), 0.5f);
    
    camera->setDistance(200.0f); // Above maximum
    EXPECT_FLOAT_EQ(camera->getDistance(), 100.0f);
}

// Test multiple zoom operations
TEST_F(ZoomFunctionalityTest, MultipleZoomOperations) {
    float startDistance = 10.0f;
    camera->setDistance(startDistance);
    
    // Multiple zoom factors
    std::vector<float> factors = {1.5f, 1.2f, 0.8f, 2.0f, 0.5f};
    float expectedDistance = startDistance;
    
    for (float factor : factors) {
        expectedDistance = expectedDistance / factor;
        camera->setDistance(expectedDistance);
        EXPECT_FLOAT_EQ(camera->getDistance(), 
                       std::max(0.5f, std::min(100.0f, expectedDistance)));
    }
}

// Test zoom behavior similar to CLI command
TEST_F(ZoomFunctionalityTest, CLIZoomBehavior) {
    // This simulates the CLI zoom command behavior
    float initialDistance = camera->getDistance();
    
    // First zoom
    float factor1 = 1.5f;
    camera->setDistance(initialDistance / factor1);
    float afterFirst = camera->getDistance();
    EXPECT_FLOAT_EQ(afterFirst, initialDistance / factor1);
    
    // Second zoom - should use current distance, not initial
    float factor2 = 1.5f;
    camera->setDistance(afterFirst / factor2);
    float afterSecond = camera->getDistance();
    EXPECT_FLOAT_EQ(afterSecond, afterFirst / factor2);
    
    // Verify cumulative effect
    EXPECT_FLOAT_EQ(afterSecond, initialDistance / (factor1 * factor2));
}

// Test zoom with extreme values
TEST_F(ZoomFunctionalityTest, ExtremeZoomValues) {
    // Very small zoom factor (should zoom out a lot)
    camera->setDistance(5.0f);
    camera->setDistance(5.0f / 0.01f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 100.0f); // Clamped to max
    
    // Very large zoom factor (should zoom in a lot)
    camera->setDistance(5.0f);
    camera->setDistance(5.0f / 100.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 0.5f); // Clamped to min
}

// Test zoom method vs setDistance
TEST_F(ZoomFunctionalityTest, ZoomMethodVsSetDistance) {
    camera->setDistance(10.0f);
    
    // Test zoom method (delta-based)
    float initialDistance = camera->getDistance();
    camera->zoom(2.0f); // Positive delta zooms in
    float afterZoom = camera->getDistance();
    EXPECT_LT(afterZoom, initialDistance);
    
    // Reset and test setDistance (absolute)
    camera->setDistance(10.0f);
    camera->setDistance(10.0f / 1.5f); // Same effect as zoom factor 1.5
    EXPECT_FLOAT_EQ(camera->getDistance(), 10.0f / 1.5f);
}

// Test zoom with custom constraints
TEST_F(ZoomFunctionalityTest, CustomZoomConstraints) {
    camera->setDistanceConstraints(2.0f, 20.0f);
    
    // Test new constraints
    camera->setDistance(1.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 2.0f);
    
    camera->setDistance(25.0f);
    EXPECT_FLOAT_EQ(camera->getDistance(), 20.0f);
    
    // Test zoom within constraints
    camera->setDistance(10.0f);
    camera->setDistance(10.0f / 2.0f); // Zoom to 5.0
    EXPECT_FLOAT_EQ(camera->getDistance(), 5.0f);
}

// Test zoom sensitivity
TEST_F(ZoomFunctionalityTest, ZoomSensitivity) {
    camera->setZoomSensitivity(2.0f);
    float initialDistance = camera->getDistance();
    
    // With higher sensitivity, same delta should produce more change
    camera->zoom(1.0f);
    float newDistance = camera->getDistance();
    
    // Reset and test with lower sensitivity
    camera->setDistance(initialDistance);
    camera->setZoomSensitivity(0.5f);
    camera->zoom(1.0f);
    float newDistanceLowSens = camera->getDistance();
    
    // Higher sensitivity should result in more zoom
    EXPECT_LT(newDistance, newDistanceLowSens);
}

// Test zoom persistence across view changes
TEST_F(ZoomFunctionalityTest, ZoomPersistenceAcrossViews) {
    // Set custom zoom
    camera->setDistance(8.0f);
    float customDistance = camera->getDistance();
    
    // Change view preset (which may change distance)
    camera->setViewPreset(ViewPreset::FRONT);
    
    // For most implementations, view presets set their own distance
    // But we should be able to zoom from the new distance
    float presetDistance = camera->getDistance();
    
    // Apply zoom factor
    camera->setDistance(presetDistance / 1.5f);
    EXPECT_FLOAT_EQ(camera->getDistance(), presetDistance / 1.5f);
}

// Test zoom calculation precision
TEST_F(ZoomFunctionalityTest, ZoomPrecision) {
    // Test that repeated small zooms accumulate correctly
    camera->setDistance(10.0f);
    float expectedDistance = 10.0f;
    
    // Apply many small zoom factors
    for (int i = 0; i < 10; ++i) {
        float factor = 1.05f; // 5% zoom each time
        expectedDistance /= factor;
        camera->setDistance(camera->getDistance() / factor);
    }
    
    // Check accumulated result (with some tolerance for floating point)
    EXPECT_NEAR(camera->getDistance(), expectedDistance, 0.001f);
}

// Test controller zoom behavior
TEST_F(ZoomFunctionalityTest, ControllerZoomBehavior) {
    OrbitCamera* controllerCamera = controller->getCamera();
    float initialDistance = controllerCamera->getDistance();
    
    // Controller's camera should use zoom method
    controllerCamera->zoom(1.0f); // Positive delta zooms in
    EXPECT_LT(controllerCamera->getDistance(), initialDistance);
    
    controllerCamera->zoom(-2.0f); // Negative delta zooms out
    EXPECT_GT(controllerCamera->getDistance(), initialDistance);
}

// Test zoom with smoothing enabled
TEST_F(ZoomFunctionalityTest, SmoothZoom) {
    camera->setSmoothing(true);
    camera->setSmoothFactor(0.1f);
    
    float initialDistance = camera->getDistance();
    
    // Note: setDistance bypasses smoothing and sets immediately
    // To test smoothing, we need to use the zoom() method
    camera->zoom(2.0f); // Zoom in with delta
    
    // After zoom with smoothing, distance changes over time
    camera->update(0.016f); // 60fps frame time
    float afterFirstUpdate = camera->getDistance();
    
    // Distance should have started changing
    EXPECT_LT(afterFirstUpdate, initialDistance);
    
    // Multiple updates should continue the zoom
    float previousDistance = afterFirstUpdate;
    for (int i = 0; i < 10; ++i) {
        camera->update(0.016f);
        float currentDistance = camera->getDistance();
        // Distance should continue decreasing (zooming in)
        EXPECT_LE(currentDistance, previousDistance);
        previousDistance = currentDistance;
    }
}