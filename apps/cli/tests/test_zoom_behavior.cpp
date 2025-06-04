#include <gtest/gtest.h>
#include "../../core/camera/CameraController.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Events;

// This test replicates the exact zoom behavior from the CLI command
class ZoomBehaviorTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        controller = std::make_unique<Camera::CameraController>(eventDispatcher.get());
    }
    
    // Simulates the zoom command behavior
    void executeZoomCommand(float factor) {
        float currentDistance = controller->getCamera()->getDistance();
        controller->getCamera()->setDistance(currentDistance / factor);
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<Camera::CameraController> controller;
};

// Test the exact issue: zoom once, then step
TEST_F(ZoomBehaviorTest, ZoomOnceAndStep) {
    float initialDistance = controller->getCamera()->getDistance();
    EXPECT_FLOAT_EQ(initialDistance, 5.0f); // Default distance
    
    // First zoom command
    executeZoomCommand(1.5f);
    float afterFirst = controller->getCamera()->getDistance();
    EXPECT_FLOAT_EQ(afterFirst, initialDistance / 1.5f);
    EXPECT_FLOAT_EQ(afterFirst, 3.333333f);
    
    // Second zoom command - should continue zooming, not step
    executeZoomCommand(1.5f);
    float afterSecond = controller->getCamera()->getDistance();
    EXPECT_FLOAT_EQ(afterSecond, afterFirst / 1.5f);
    EXPECT_FLOAT_EQ(afterSecond, 2.222222f);
    
    // Third zoom - verify it continues
    executeZoomCommand(1.5f);
    float afterThird = controller->getCamera()->getDistance();
    EXPECT_FLOAT_EQ(afterThird, afterSecond / 1.5f);
    EXPECT_NEAR(afterThird, 1.481481f, 0.0001f);
}

// Test multiple sequential zooms
TEST_F(ZoomBehaviorTest, MultipleSequentialZooms) {
    std::vector<float> distances;
    distances.push_back(controller->getCamera()->getDistance());
    
    // Execute multiple zoom commands
    for (int i = 0; i < 5; ++i) {
        executeZoomCommand(1.2f);
        distances.push_back(controller->getCamera()->getDistance());
        
        // Verify each zoom reduces distance
        EXPECT_LT(distances.back(), distances[distances.size() - 2]);
        
        // Verify correct factor application
        EXPECT_NEAR(distances.back(), distances[distances.size() - 2] / 1.2f, 0.0001f);
    }
}

// Test zoom in and out alternation
TEST_F(ZoomBehaviorTest, AlternatingZoomInOut) {
    float startDistance = controller->getCamera()->getDistance();
    
    // Zoom in
    executeZoomCommand(2.0f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getDistance(), startDistance / 2.0f);
    
    // Zoom out
    executeZoomCommand(0.5f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getDistance(), startDistance);
    
    // Another cycle
    executeZoomCommand(1.5f);
    float zoomedIn = controller->getCamera()->getDistance();
    executeZoomCommand(0.666667f); // Approximately 1/1.5
    EXPECT_NEAR(controller->getCamera()->getDistance(), startDistance, 0.0001f);
}

// Test edge case: very small incremental zooms
TEST_F(ZoomBehaviorTest, SmallIncrementalZooms) {
    float distance = controller->getCamera()->getDistance();
    
    // Many small zoom steps
    for (int i = 0; i < 20; ++i) {
        float oldDistance = distance;
        executeZoomCommand(1.05f); // 5% zoom each time
        distance = controller->getCamera()->getDistance();
        
        // Each zoom should change the distance
        EXPECT_NE(distance, oldDistance);
        EXPECT_FLOAT_EQ(distance, oldDistance / 1.05f);
    }
    
    // Verify cumulative effect
    float expectedFinal = 5.0f / std::pow(1.05f, 20);
    EXPECT_NEAR(controller->getCamera()->getDistance(), expectedFinal, 0.0001f);
}

// Test potential issue: zoom after other camera operations
TEST_F(ZoomBehaviorTest, ZoomAfterOtherOperations) {
    // Change view preset
    controller->getCamera()->setViewPreset(Camera::ViewPreset::FRONT);
    float presetDistance = controller->getCamera()->getDistance();
    
    // First zoom should work from preset distance
    executeZoomCommand(1.5f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getDistance(), presetDistance / 1.5f);
    
    // Rotate camera
    controller->getCamera()->orbit(45.0f, 30.0f);
    
    // Zoom should still work correctly
    float beforeRotateZoom = controller->getCamera()->getDistance();
    executeZoomCommand(1.2f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getDistance(), beforeRotateZoom / 1.2f);
}

// Test the specific pattern that might cause stepping
TEST_F(ZoomBehaviorTest, IdenticalConsecutiveZooms) {
    float distance = controller->getCamera()->getDistance();
    const float factor = 1.5f;
    
    // Execute the same zoom factor multiple times
    for (int i = 0; i < 5; ++i) {
        float prevDistance = distance;
        executeZoomCommand(factor);
        distance = controller->getCamera()->getDistance();
        
        // Distance should always decrease by the same factor
        EXPECT_FLOAT_EQ(distance, prevDistance / factor);
        
        // Log for debugging
        std::cout << "Zoom " << i + 1 << ": " << prevDistance 
                  << " -> " << distance 
                  << " (expected: " << prevDistance / factor << ")" << std::endl;
    }
}

// Test zoom with different starting distances
TEST_F(ZoomBehaviorTest, ZoomFromDifferentDistances) {
    std::vector<float> startDistances = {1.0f, 5.0f, 10.0f, 50.0f, 90.0f};
    
    for (float startDist : startDistances) {
        controller->getCamera()->setDistance(startDist);
        
        // Apply same zoom sequence
        executeZoomCommand(1.5f);
        EXPECT_FLOAT_EQ(controller->getCamera()->getDistance(), startDist / 1.5f);
        
        executeZoomCommand(1.5f);
        EXPECT_FLOAT_EQ(controller->getCamera()->getDistance(), startDist / (1.5f * 1.5f));
    }
}

// Test potential floating point precision issues
TEST_F(ZoomBehaviorTest, FloatingPointPrecision) {
    // Start with a distance that might have precision issues
    controller->getCamera()->setDistance(3.333333f);
    
    float distance = controller->getCamera()->getDistance();
    for (int i = 0; i < 10; ++i) {
        float prevDistance = distance;
        executeZoomCommand(1.1f);
        distance = controller->getCamera()->getDistance();
        
        // Use EXPECT_NEAR for floating point comparison
        EXPECT_NEAR(distance, prevDistance / 1.1f, 0.00001f);
    }
}