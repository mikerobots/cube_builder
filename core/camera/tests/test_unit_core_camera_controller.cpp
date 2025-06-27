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
    // REQ-CAM-1: Camera system shall provide orbit-style controls
    // REQ-9.2.2: CLI shall support camera commands (rotate, zoom)
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
    WorldCoordinates initialTarget = controller->getCamera()->getTarget();
    
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
    // REQ-CAM-4: Camera shall provide standard view presets
    // REQ-9.2.2: CLI shall support camera commands (view)
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
    
    controller->frameAll(WorldCoordinates(minBounds), WorldCoordinates(maxBounds));
    
    // Camera should focus on center of bounds
    Vector3f expectedCenter = (minBounds + maxBounds) * 0.5f;
    EXPECT_EQ(controller->getCamera()->getTarget().value(), expectedCenter);
}

TEST_F(CameraControllerTest, FocusOn) {
    Vector3f focusPoint(10.0f, 5.0f, -3.0f);
    
    controller->focusOn(WorldCoordinates(focusPoint), 15.0f);
    
    EXPECT_EQ(controller->getCamera()->getTarget().value(), focusPoint);
    EXPECT_FLOAT_EQ(controller->getCamera()->getDistance(), 15.0f);
    
    // Test focus without distance
    controller->focusOn(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)));
    EXPECT_EQ(controller->getCamera()->getTarget().value(), Vector3f(0.0f, 0.0f, 0.0f));
}

TEST_F(CameraControllerTest, MouseRayGeneration) {
    // REQ-5.1.4: Ray-casting shall determine face/position under cursor
    // REQ-CAM-6: Camera shall provide accurate ray generation
    Vector2i mousePos(400, 300); // Center of default viewport
    
    Ray mouseRay = controller->getMouseRay(mousePos);
    
    // Ray should be valid (normalized direction)
    EXPECT_NEAR(mouseRay.direction.length(), 1.0f, 0.001f);
    
    // For center of viewport, ray should point roughly forward
    EXPECT_LT(mouseRay.direction.z, 0.0f); // Negative Z is forward
}

// ===== Comprehensive getMouseRay Tests =====

TEST_F(CameraControllerTest, GetMouseRay_AllCorners) {
    // Test ray generation from all viewport corners
    controller->setViewportSize(800, 600);
    Viewport* viewport = controller->getViewport();
    
    // Test all four corners
    struct CornerTest {
        Vector2i position;
        std::string name;
        bool expectXPositive;
        bool expectYPositive;
    };
    
    CornerTest corners[] = {
        {Vector2i(0, 0), "top-left", false, true},
        {Vector2i(799, 0), "top-right", true, true},
        {Vector2i(0, 599), "bottom-left", false, false},
        {Vector2i(799, 599), "bottom-right", true, false}
    };
    
    for (const auto& corner : corners) {
        Ray ray = controller->getMouseRay(corner.position);
        
        // Ray should be normalized
        EXPECT_NEAR(ray.direction.length(), 1.0f, 0.001f) 
            << "Ray not normalized for " << corner.name;
        
        // Check X direction
        if (corner.expectXPositive) {
            EXPECT_GT(ray.direction.x, 0.0f) 
                << "Ray X should be positive for " << corner.name;
        } else {
            EXPECT_LT(ray.direction.x, 0.0f) 
                << "Ray X should be negative for " << corner.name;
        }
        
        // Check Y direction
        if (corner.expectYPositive) {
            EXPECT_GT(ray.direction.y, 0.0f) 
                << "Ray Y should be positive for " << corner.name;
        } else {
            EXPECT_LT(ray.direction.y, 0.0f) 
                << "Ray Y should be negative for " << corner.name;
        }
        
        // Z should always be negative (forward)
        EXPECT_LT(ray.direction.z, 0.0f) 
            << "Ray Z should be negative for " << corner.name;
    }
}

TEST_F(CameraControllerTest, GetMouseRay_OutsideViewport) {
    controller->setViewportSize(800, 600);
    
    // Test positions outside viewport bounds
    std::vector<Vector2i> outsidePositions = {
        Vector2i(-100, 300),    // Left of viewport
        Vector2i(900, 300),     // Right of viewport
        Vector2i(400, -100),    // Above viewport
        Vector2i(400, 700),     // Below viewport
        Vector2i(-100, -100),   // Top-left outside
        Vector2i(900, 700)      // Bottom-right outside
    };
    
    for (const auto& pos : outsidePositions) {
        Ray ray = controller->getMouseRay(pos);
        
        // Should still generate valid rays
        EXPECT_NEAR(ray.direction.length(), 1.0f, 0.001f)
            << "Ray not normalized for position (" << pos.x << ", " << pos.y << ")";
        
        // Ray should point forward
        EXPECT_LT(ray.direction.z, 0.0f)
            << "Ray should point forward for position (" << pos.x << ", " << pos.y << ")";
    }
}

TEST_F(CameraControllerTest, GetMouseRay_DifferentViewPresets) {
    controller->setViewportSize(800, 600);
    Vector2i centerPos(400, 300);
    
    // Test ray generation with different view presets
    ViewPreset presets[] = {
        ViewPreset::FRONT,
        ViewPreset::BACK,
        ViewPreset::LEFT,
        ViewPreset::RIGHT,
        ViewPreset::TOP,
        ViewPreset::BOTTOM,
        ViewPreset::ISOMETRIC
    };
    
    for (auto preset : presets) {
        controller->setViewPreset(preset);
        Ray ray = controller->getMouseRay(centerPos);
        
        // Ray should be normalized
        EXPECT_NEAR(ray.direction.length(), 1.0f, 0.001f)
            << "Ray not normalized for preset " << static_cast<int>(preset);
        
        // Verify ray direction makes sense for the view
        switch (preset) {
            case ViewPreset::FRONT:
                EXPECT_LT(ray.direction.z, 0.0f); // Looking -Z
                break;
            case ViewPreset::BACK:
                EXPECT_GT(ray.direction.z, 0.0f); // Looking +Z
                break;
            case ViewPreset::LEFT:
                EXPECT_GT(ray.direction.x, 0.0f); // Looking +X
                break;
            case ViewPreset::RIGHT:
                EXPECT_LT(ray.direction.x, 0.0f); // Looking -X
                break;
            case ViewPreset::TOP:
                EXPECT_LT(ray.direction.y, 0.0f); // Looking -Y
                break;
            case ViewPreset::BOTTOM:
                EXPECT_GT(ray.direction.y, 0.0f); // Looking +Y
                break;
            case ViewPreset::ISOMETRIC:
                // Should have components in all directions
                EXPECT_NE(ray.direction.x, 0.0f);
                EXPECT_NE(ray.direction.y, 0.0f);
                EXPECT_NE(ray.direction.z, 0.0f);
                break;
        }
    }
}

TEST_F(CameraControllerTest, GetMouseRay_ConsistencyWithWorldToScreen) {
    controller->setViewportSize(800, 600);
    
    // Place a point in world space
    Vector3f worldPoint(2.0f, 1.0f, -3.0f);
    
    // Project to screen
    Vector2i screenPos = controller->worldToScreen(worldPoint);
    
    // Generate ray from that screen position
    Ray ray = controller->getMouseRay(screenPos);
    
    // The ray should pass through the world point
    // Calculate parameter t where ray hits the plane containing worldPoint
    // perpendicular to camera forward
    WorldCoordinates cameraPos = controller->getCamera()->getPosition();
    Vector3f toPoint = worldPoint - cameraPos.value();
    float t = toPoint.length();
    
    // Point along ray at distance t
    Vector3f rayPoint = ray.origin + ray.direction * t;
    
    // Should be close to original world point (within projection tolerance)
    EXPECT_NEAR(rayPoint.x, worldPoint.x, 0.1f);
    EXPECT_NEAR(rayPoint.y, worldPoint.y, 0.1f);
    EXPECT_NEAR(rayPoint.z, worldPoint.z, 0.1f);
}

TEST_F(CameraControllerTest, GetMouseRay_AfterCameraMovement) {
    controller->setViewportSize(800, 600);
    Vector2i centerPos(400, 300);
    
    // Get initial ray
    Ray initialRay = controller->getMouseRay(centerPos);
    
    // Move camera
    controller->getCamera()->setYaw(45.0f);
    controller->getCamera()->setPitch(30.0f);
    controller->getCamera()->setDistance(20.0f);
    controller->getCamera()->setTarget(WorldCoordinates(Vector3f(5.0f, 3.0f, 2.0f)));
    
    // Get ray after movement
    Ray movedRay = controller->getMouseRay(centerPos);
    
    // Rays should be different
    EXPECT_NE(movedRay.origin.x, initialRay.origin.x);
    EXPECT_NE(movedRay.origin.y, initialRay.origin.y);
    EXPECT_NE(movedRay.origin.z, initialRay.origin.z);
    EXPECT_NE(movedRay.direction.x, initialRay.direction.x);
    EXPECT_NE(movedRay.direction.y, initialRay.direction.y);
    EXPECT_NE(movedRay.direction.z, initialRay.direction.z);
    
    // But both should still be normalized
    EXPECT_NEAR(movedRay.direction.length(), 1.0f, 0.001f);
}

TEST_F(CameraControllerTest, GetMouseRay_WithOrthographicProjection) {
    controller->setViewportSize(800, 600);
    
    // Switch to orthographic projection
    controller->getCamera()->setProjectionType(ProjectionType::ORTHOGRAPHIC);
    controller->getCamera()->setOrthographicSize(10.0f);
    
    // Test multiple positions
    Vector2i pos1(200, 150);
    Vector2i pos2(600, 450);
    
    Ray ray1 = controller->getMouseRay(pos1);
    Ray ray2 = controller->getMouseRay(pos2);
    
    // In orthographic projection, all rays should be parallel
    EXPECT_NEAR(ray1.direction.x, ray2.direction.x, 0.001f);
    EXPECT_NEAR(ray1.direction.y, ray2.direction.y, 0.001f);
    EXPECT_NEAR(ray1.direction.z, ray2.direction.z, 0.001f);
    
    // Both should be normalized
    EXPECT_NEAR(ray1.direction.length(), 1.0f, 0.001f);
    EXPECT_NEAR(ray2.direction.length(), 1.0f, 0.001f);
    
    // Origins should be different
    EXPECT_NE(ray1.origin.x, ray2.origin.x);
    EXPECT_NE(ray1.origin.y, ray2.origin.y);
}

TEST_F(CameraControllerTest, GetMouseRay_SubpixelAccuracy) {
    controller->setViewportSize(800, 600);
    
    // Test that nearby pixels generate slightly different rays
    Vector2i pos1(400, 300);
    Vector2i pos2(401, 300); // One pixel to the right
    Vector2i pos3(400, 301); // One pixel down
    
    Ray ray1 = controller->getMouseRay(pos1);
    Ray ray2 = controller->getMouseRay(pos2);
    Ray ray3 = controller->getMouseRay(pos3);
    
    // Rays should be slightly different
    EXPECT_NE(ray1.direction.x, ray2.direction.x);
    EXPECT_NE(ray1.direction.y, ray3.direction.y);
    
    // But differences should be small (subpixel)
    float xDiff = std::abs(ray2.direction.x - ray1.direction.x);
    float yDiff = std::abs(ray3.direction.y - ray1.direction.y);
    
    EXPECT_LT(xDiff, 0.01f); // Small difference
    EXPECT_LT(yDiff, 0.01f); // Small difference
    
    // All should still be normalized
    EXPECT_NEAR(ray1.direction.length(), 1.0f, 0.001f);
    EXPECT_NEAR(ray2.direction.length(), 1.0f, 0.001f);
    EXPECT_NEAR(ray3.direction.length(), 1.0f, 0.001f);
}

TEST_F(CameraControllerTest, WorldToScreen) {
    // Test converting target point to screen coordinates
    Vector3f targetPoint = controller->getCamera()->getTarget().value();
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
    // REQ-CAM-5: View transitions shall be smooth
    // REQ-CAM-7: Camera operations shall be performant (smooth at 60+ FPS)
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