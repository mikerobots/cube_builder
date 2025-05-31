#include <gtest/gtest.h>
#include "../CameraController.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor::Camera;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class CameraControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        controller = std::make_unique<CameraController>(eventDispatcher.get());
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<CameraController> controller;
};

TEST_F(CameraControllerTest, DefaultConstruction) {
    EXPECT_NE(controller->getCamera(), nullptr);
    EXPECT_NE(controller->getViewport(), nullptr);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::NONE);
    EXPECT_FALSE(controller->isInteracting());
}

TEST_F(CameraControllerTest, ViewportManagement) {
    // Test viewport size setting
    controller->setViewportSize(1024, 768);
    
    EXPECT_EQ(controller->getViewport()->getWidth(), 1024);
    EXPECT_EQ(controller->getViewport()->getHeight(), 768);
    EXPECT_FLOAT_EQ(controller->getCamera()->getAspectRatio(), 1024.0f / 768.0f);
    
    // Test viewport bounds setting
    controller->setViewportBounds(100, 50, 800, 600);
    
    EXPECT_EQ(controller->getViewport()->getX(), 100);
    EXPECT_EQ(controller->getViewport()->getY(), 50);
    EXPECT_EQ(controller->getViewport()->getWidth(), 800);
    EXPECT_EQ(controller->getViewport()->getHeight(), 600);
    EXPECT_FLOAT_EQ(controller->getCamera()->getAspectRatio(), 800.0f / 600.0f);
}

TEST_F(CameraControllerTest, MouseButtonInteraction) {
    Vector2i mousePos(400, 300);
    
    // Test left button (orbit mode)
    controller->onMouseButtonDown(mousePos, 0);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::ORBIT);
    
    controller->onMouseButtonUp(mousePos, 0);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::NONE);
    
    // Test middle button (pan mode)
    controller->onMouseButtonDown(mousePos, 1);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::PAN);
    
    controller->onMouseButtonUp(mousePos, 1);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::NONE);
    
    // Test right button (zoom mode)
    controller->onMouseButtonDown(mousePos, 2);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::ZOOM);
    
    controller->onMouseButtonUp(mousePos, 2);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::NONE);
}

TEST_F(CameraControllerTest, MouseButtonOutsideViewport) {
    // Mouse outside viewport should not start interaction
    Vector2i outsidePos(1000, 1000);
    
    controller->onMouseButtonDown(outsidePos, 0);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::NONE);
}

TEST_F(CameraControllerTest, MouseDragThreshold) {
    Vector2i startPos(400, 300);
    controller->setMouseDragThreshold(5.0f);
    
    controller->onMouseButtonDown(startPos, 0);
    EXPECT_FALSE(controller->isInteracting()); // Not dragging yet
    
    // Small movement below threshold
    Vector2i smallMove = startPos + Vector2i(2, 2);
    controller->onMouseMove(smallMove);
    EXPECT_FALSE(controller->isInteracting()); // Still not dragging
    
    // Large movement above threshold
    Vector2i largeMove = startPos + Vector2i(10, 10);
    controller->onMouseMove(largeMove);
    EXPECT_TRUE(controller->isInteracting()); // Now dragging
}

TEST_F(CameraControllerTest, OrbitControl) {
    Vector2i startPos(400, 300);
    float initialYaw = controller->getCamera()->getYaw();
    float initialPitch = controller->getCamera()->getPitch();
    
    // Start orbit
    controller->onMouseButtonDown(startPos, 0);
    
    // Move mouse to trigger orbit (above threshold)
    Vector2i newPos = startPos + Vector2i(50, -30);
    controller->onMouseMove(newPos);
    
    // Camera angles should change
    EXPECT_NE(controller->getCamera()->getYaw(), initialYaw);
    EXPECT_NE(controller->getCamera()->getPitch(), initialPitch);
}

TEST_F(CameraControllerTest, PanControl) {
    Vector2i startPos(400, 300);
    Vector3f initialTarget = controller->getCamera()->getTarget();
    
    // Start pan
    controller->onMouseButtonDown(startPos, 1);
    
    // Move mouse to trigger pan
    Vector2i newPos = startPos + Vector2i(30, 20);
    controller->onMouseMove(newPos);
    
    // Camera target should change
    EXPECT_NE(controller->getCamera()->getTarget(), initialTarget);
}

TEST_F(CameraControllerTest, ZoomControl) {
    Vector2i startPos(400, 300);
    float initialDistance = controller->getCamera()->getDistance();
    
    // Start zoom
    controller->onMouseButtonDown(startPos, 2);
    
    // Move mouse to trigger zoom
    Vector2i newPos = startPos + Vector2i(0, -20); // Move up to zoom in
    controller->onMouseMove(newPos);
    
    // Camera distance should change
    EXPECT_NE(controller->getCamera()->getDistance(), initialDistance);
}

TEST_F(CameraControllerTest, MouseWheelZoom) {
    Vector2i mousePos(400, 300);
    float initialDistance = controller->getCamera()->getDistance();
    
    // Scroll up (zoom in)
    controller->onMouseWheel(mousePos, 1.0f);
    EXPECT_LT(controller->getCamera()->getDistance(), initialDistance);
    
    // Scroll down (zoom out)
    controller->onMouseWheel(mousePos, -2.0f);
    EXPECT_GT(controller->getCamera()->getDistance(), initialDistance);
}

TEST_F(CameraControllerTest, MouseWheelOutsideViewport) {
    Vector2i outsidePos(1000, 1000);
    float initialDistance = controller->getCamera()->getDistance();
    
    controller->onMouseWheel(outsidePos, 1.0f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getDistance(), initialDistance); // Should not change
}

TEST_F(CameraControllerTest, ViewPresets) {
    controller->setViewPreset(ViewPreset::FRONT);
    EXPECT_FLOAT_EQ(controller->getCamera()->getYaw(), 0.0f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getPitch(), 0.0f);
    
    controller->setViewPreset(ViewPreset::TOP);
    EXPECT_FLOAT_EQ(controller->getCamera()->getYaw(), 0.0f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getPitch(), 90.0f);
    
    controller->setViewPreset(ViewPreset::ISOMETRIC);
    EXPECT_FLOAT_EQ(controller->getCamera()->getYaw(), 45.0f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getPitch(), 35.26f);
}

TEST_F(CameraControllerTest, FrameAll) {
    Vector3f minBounds(-5.0f, -3.0f, -2.0f);
    Vector3f maxBounds(5.0f, 3.0f, 2.0f);
    
    controller->frameAll(minBounds, maxBounds);
    
    // Camera should focus on center of bounds
    Vector3f expectedCenter = (minBounds + maxBounds) * 0.5f;
    EXPECT_EQ(controller->getCamera()->getTarget(), expectedCenter);
}

TEST_F(CameraControllerTest, FocusOn) {
    Vector3f focusPoint(10.0f, 5.0f, -3.0f);
    
    controller->focusOn(focusPoint, 15.0f);
    
    EXPECT_EQ(controller->getCamera()->getTarget(), focusPoint);
    EXPECT_FLOAT_EQ(controller->getCamera()->getDistance(), 15.0f);
    
    // Test focus without distance
    controller->focusOn(Vector3f(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(controller->getCamera()->getTarget(), Vector3f(0.0f, 0.0f, 0.0f));
}

TEST_F(CameraControllerTest, MouseRayGeneration) {
    Vector2i mousePos(400, 300); // Center of default viewport
    
    Ray mouseRay = controller->getMouseRay(mousePos);
    
    // Ray should be valid (normalized direction)
    EXPECT_NEAR(mouseRay.direction.length(), 1.0f, 0.001f);
    
    // For center of viewport, ray should point roughly forward
    EXPECT_LT(mouseRay.direction.z, 0.0f); // Negative Z is forward
}

TEST_F(CameraControllerTest, WorldToScreen) {
    // Test converting target point to screen coordinates
    Vector3f targetPoint = controller->getCamera()->getTarget();
    Vector2i screenPos = controller->worldToScreen(targetPoint);
    
    // Target should project near center of viewport
    Viewport* viewport = controller->getViewport();
    Vector2i center(viewport->getX() + viewport->getWidth() / 2,
                   viewport->getY() + viewport->getHeight() / 2);
    
    // Allow some tolerance for projection accuracy
    EXPECT_NEAR(screenPos.x, center.x, 10);
    EXPECT_NEAR(screenPos.y, center.y, 10);
}

TEST_F(CameraControllerTest, UpdateAnimation) {
    // Enable smoothing
    controller->setCameraSmoothing(true, 0.1f);
    
    // Make a change that should be smoothed
    controller->getCamera()->zoom(5.0f);
    
    float initialDistance = controller->getCamera()->getDistance();
    
    // Update should move towards target
    controller->update(0.016f); // 60fps delta
    
    // Distance should change during smoothing
    EXPECT_NE(controller->getCamera()->getDistance(), initialDistance);
}

TEST_F(CameraControllerTest, SensitivitySettings) {
    controller->setCameraSensitivity(0.5f, 2.0f, 1.5f);
    
    EXPECT_FLOAT_EQ(controller->getCamera()->getPanSensitivity(), 0.5f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getRotateSensitivity(), 2.0f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getZoomSensitivity(), 1.5f);
}

TEST_F(CameraControllerTest, SmoothingSettings) {
    controller->setCameraSmoothing(true, 0.3f);
    
    EXPECT_TRUE(controller->getCamera()->isSmoothing());
    EXPECT_FLOAT_EQ(controller->getCamera()->getSmoothFactor(), 0.3f);
    
    controller->setCameraSmoothing(false);
    EXPECT_FALSE(controller->getCamera()->isSmoothing());
}

TEST_F(CameraControllerTest, ConstraintSettings) {
    controller->setCameraConstraints(1.0f, 50.0f, -45.0f, 45.0f);
    
    EXPECT_FLOAT_EQ(controller->getCamera()->getMinDistance(), 1.0f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getMaxDistance(), 50.0f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getMinPitch(), -45.0f);
    EXPECT_FLOAT_EQ(controller->getCamera()->getMaxPitch(), 45.0f);
}

TEST_F(CameraControllerTest, DragThresholdSettings) {
    controller->setMouseDragThreshold(10.0f);
    EXPECT_FLOAT_EQ(controller->getMouseDragThreshold(), 10.0f);
}

TEST_F(CameraControllerTest, MouseMoveOutsideViewport) {
    Vector2i insidePos(400, 300);
    Vector2i outsidePos(1000, 1000);
    
    // Start interaction inside viewport
    controller->onMouseButtonDown(insidePos, 0);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::ORBIT);
    
    // Move outside viewport
    controller->onMouseMove(outsidePos);
    
    // Should still be in orbit mode but not interacting
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::ORBIT);
    EXPECT_FALSE(controller->isInteracting());
}

TEST_F(CameraControllerTest, MultipleMouseButtons) {
    Vector2i mousePos(400, 300);
    
    // Press multiple buttons
    controller->onMouseButtonDown(mousePos, 0); // Orbit
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::ORBIT);
    
    controller->onMouseButtonDown(mousePos, 1); // Pan (should override)
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::PAN);
    
    // Release first button
    controller->onMouseButtonUp(mousePos, 0);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::NONE); // Should stop all interaction
}

TEST_F(CameraControllerTest, InvalidMouseButton) {
    Vector2i mousePos(400, 300);
    
    // Test invalid button number
    controller->onMouseButtonDown(mousePos, 99);
    EXPECT_EQ(controller->getInteractionMode(), InteractionMode::NONE);
}

TEST_F(CameraControllerTest, ContinuousMouseMovement) {
    Vector2i startPos(400, 300);
    float initialYaw = controller->getCamera()->getYaw();
    
    // Start orbit
    controller->onMouseButtonDown(startPos, 0);
    
    // Multiple mouse movements
    for (int i = 1; i <= 5; ++i) {
        Vector2i newPos = startPos + Vector2i(i * 10, 0);
        controller->onMouseMove(newPos);
    }
    
    // Yaw should have changed significantly
    float finalYaw = controller->getCamera()->getYaw();
    EXPECT_GT(std::abs(finalYaw - initialYaw), 5.0f); // Significant change
}

TEST_F(CameraControllerTest, ZeroMouseMovement) {
    Vector2i mousePos(400, 300);
    float initialYaw = controller->getCamera()->getYaw();
    float initialPitch = controller->getCamera()->getPitch();
    
    // Start orbit
    controller->onMouseButtonDown(mousePos, 0);
    
    // Move to same position (no movement)
    controller->onMouseMove(mousePos);
    
    // Camera should not change
    EXPECT_FLOAT_EQ(controller->getCamera()->getYaw(), initialYaw);
    EXPECT_FLOAT_EQ(controller->getCamera()->getPitch(), initialPitch);
    EXPECT_FALSE(controller->isInteracting()); // Below threshold
}