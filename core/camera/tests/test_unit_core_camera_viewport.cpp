#include <gtest/gtest.h>
#include "../Viewport.h"

using namespace VoxelEditor::Camera;
using namespace VoxelEditor::Math;

class ViewportTest : public ::testing::Test {
protected:
    void SetUp() override {
        viewport = std::make_unique<Viewport>(100, 50, 800, 600);
    }
    
    std::unique_ptr<Viewport> viewport;
};

TEST_F(ViewportTest, DefaultConstruction) {
    Viewport defaultViewport;
    
    EXPECT_EQ(defaultViewport.getX(), 0);
    EXPECT_EQ(defaultViewport.getY(), 0);
    EXPECT_EQ(defaultViewport.getWidth(), 800);
    EXPECT_EQ(defaultViewport.getHeight(), 600);
    EXPECT_FLOAT_EQ(defaultViewport.getAspectRatio(), 800.0f / 600.0f);
}

TEST_F(ViewportTest, CustomConstruction) {
    EXPECT_EQ(viewport->getX(), 100);
    EXPECT_EQ(viewport->getY(), 50);
    EXPECT_EQ(viewport->getWidth(), 800);
    EXPECT_EQ(viewport->getHeight(), 600);
    EXPECT_FLOAT_EQ(viewport->getAspectRatio(), 800.0f / 600.0f);
}

TEST_F(ViewportTest, PositionManagement) {
    viewport->setPosition(200, 100);
    
    EXPECT_EQ(viewport->getX(), 200);
    EXPECT_EQ(viewport->getY(), 100);
    EXPECT_EQ(viewport->getPosition(), Vector2i(200, 100));
}

TEST_F(ViewportTest, SizeManagement) {
    viewport->setSize(1024, 768);
    
    EXPECT_EQ(viewport->getWidth(), 1024);
    EXPECT_EQ(viewport->getHeight(), 768);
    EXPECT_EQ(viewport->getSize(), Vector2i(1024, 768));
    EXPECT_FLOAT_EQ(viewport->getAspectRatio(), 1024.0f / 768.0f);
    
    // Test invalid sizes are rejected
    viewport->setSize(0, 768);
    EXPECT_EQ(viewport->getWidth(), 1024); // Should remain unchanged
    
    viewport->setSize(1024, 0);
    EXPECT_EQ(viewport->getHeight(), 768); // Should remain unchanged
    
    viewport->setSize(-100, -200);
    EXPECT_EQ(viewport->getWidth(), 1024); // Should remain unchanged
    EXPECT_EQ(viewport->getHeight(), 768); // Should remain unchanged
}

TEST_F(ViewportTest, BoundsManagement) {
    viewport->setBounds(50, 25, 1280, 720);
    
    EXPECT_EQ(viewport->getX(), 50);
    EXPECT_EQ(viewport->getY(), 25);
    EXPECT_EQ(viewport->getWidth(), 1280);
    EXPECT_EQ(viewport->getHeight(), 720);
}

TEST_F(ViewportTest, ScreenToNormalizedCoordinates) {
    // Test center of viewport
    Vector2i center(viewport->getX() + viewport->getWidth() / 2, 
                   viewport->getY() + viewport->getHeight() / 2);
    Vector2f normalized = viewport->screenToNormalized(center);
    
    EXPECT_NEAR(normalized.x, 0.0f, 0.001f);
    EXPECT_NEAR(normalized.y, 0.0f, 0.001f);
    
    // Test top-left corner
    Vector2i topLeft(viewport->getX(), viewport->getY());
    normalized = viewport->screenToNormalized(topLeft);
    
    EXPECT_NEAR(normalized.x, -1.0f, 0.001f);
    EXPECT_NEAR(normalized.y, 1.0f, 0.001f);
    
    // Test bottom-right corner
    Vector2i bottomRight(viewport->getX() + viewport->getWidth(), 
                        viewport->getY() + viewport->getHeight());
    normalized = viewport->screenToNormalized(bottomRight);
    
    EXPECT_NEAR(normalized.x, 1.0f, 0.001f);
    EXPECT_NEAR(normalized.y, -1.0f, 0.001f);
}

TEST_F(ViewportTest, NormalizedToScreenCoordinates) {
    // Test center
    Vector2f center(0.0f, 0.0f);
    Vector2i screen = viewport->normalizedToScreen(center);
    
    EXPECT_EQ(screen.x, viewport->getX() + viewport->getWidth() / 2);
    EXPECT_EQ(screen.y, viewport->getY() + viewport->getHeight() / 2);
    
    // Test corners
    Vector2f topLeft(-1.0f, 1.0f);
    screen = viewport->normalizedToScreen(topLeft);
    
    EXPECT_EQ(screen.x, viewport->getX());
    EXPECT_EQ(screen.y, viewport->getY());
    
    Vector2f bottomRight(1.0f, -1.0f);
    screen = viewport->normalizedToScreen(bottomRight);
    
    EXPECT_EQ(screen.x, viewport->getX() + viewport->getWidth());
    EXPECT_EQ(screen.y, viewport->getY() + viewport->getHeight());
}

TEST_F(ViewportTest, CoordinateRoundTrip) {
    // Test that screen -> normalized -> screen gives original coordinates
    Vector2i originalScreen(viewport->getX() + 300, viewport->getY() + 200);
    Vector2f normalized = viewport->screenToNormalized(originalScreen);
    Vector2i backToScreen = viewport->normalizedToScreen(normalized);
    
    EXPECT_EQ(originalScreen.x, backToScreen.x);
    EXPECT_EQ(originalScreen.y, backToScreen.y);
}

TEST_F(ViewportTest, ContainsPoint) {
    // Points inside viewport
    EXPECT_TRUE(viewport->contains(Vector2i(400, 300))); // Center
    EXPECT_TRUE(viewport->contains(viewport->getX(), viewport->getY())); // Top-left
    EXPECT_TRUE(viewport->contains(viewport->getX() + viewport->getWidth() - 1, 
                                  viewport->getY() + viewport->getHeight() - 1)); // Bottom-right (inside)
    
    // Points outside viewport
    EXPECT_FALSE(viewport->contains(Vector2i(50, 25))); // Before viewport
    EXPECT_FALSE(viewport->contains(Vector2i(1000, 700))); // After viewport
    EXPECT_FALSE(viewport->contains(viewport->getX() - 1, viewport->getY())); // Just left
    EXPECT_FALSE(viewport->contains(viewport->getX(), viewport->getY() - 1)); // Just above
    EXPECT_FALSE(viewport->contains(viewport->getX() + viewport->getWidth(), 
                                   viewport->getY() + viewport->getHeight())); // Just outside bottom-right
}

TEST_F(ViewportTest, ScreenToWorldRay) {
    // REQ-5.1.4: Ray-casting shall determine face/position under cursor
    // REQ-CAM-6: Camera shall provide accurate ray generation
    // Create test matrices
    Matrix4f viewMatrix = Matrix4f::lookAt(
        Vector3f(0, 0, 5),  // Eye
        Vector3f(0, 0, 0),  // Target
        Vector3f(0, 1, 0)   // Up
    );
    
    Matrix4f projMatrix = Matrix4f::perspective(
        45.0f,                        // FOV
        viewport->getAspectRatio(),   // Aspect ratio
        0.1f,                         // Near plane
        1000.0f                       // Far plane
    );
    
    // Test ray from center of viewport
    Vector2i center(viewport->getX() + viewport->getWidth() / 2, 
                   viewport->getY() + viewport->getHeight() / 2);
    Ray centerRay = viewport->screenToWorldRay(center, viewMatrix, projMatrix);
    
    // Center ray should point forward (negative Z direction)
    EXPECT_LT(centerRay.direction.z, 0.0f);
    EXPECT_NEAR(centerRay.direction.x, 0.0f, 0.1f);
    EXPECT_NEAR(centerRay.direction.y, 0.0f, 0.1f);
    
    // Ray should be normalized
    EXPECT_NEAR(centerRay.direction.length(), 1.0f, 0.001f);
}

TEST_F(ViewportTest, WorldToScreen) {
    // Create test matrices
    Matrix4f viewMatrix = Matrix4f::lookAt(
        Vector3f(0, 0, 5),  // Eye
        Vector3f(0, 0, 0),  // Target
        Vector3f(0, 1, 0)   // Up
    );
    
    Matrix4f projMatrix = Matrix4f::perspective(
        45.0f,
        viewport->getAspectRatio(),
        0.1f,
        1000.0f
    );
    
    // Test point at target (should project to center)
    Vector3f targetPoint(0.0f, 0.0f, 0.0f);
    Vector2i screenPos = viewport->worldToScreen(targetPoint, viewMatrix, projMatrix);
    
    Vector2i expectedCenter(viewport->getX() + viewport->getWidth() / 2,
                           viewport->getY() + viewport->getHeight() / 2);
    
    // Should be close to center (allowing for some projection error)
    EXPECT_NEAR(screenPos.x, expectedCenter.x, 5);
    EXPECT_NEAR(screenPos.y, expectedCenter.y, 5);
}

TEST_F(ViewportTest, MouseDelta) {
    Vector2i currentPos(viewport->getX() + 400, viewport->getY() + 300);
    Vector2i lastPos(viewport->getX() + 350, viewport->getY() + 250);
    
    Vector2f delta = viewport->getMouseDelta(currentPos, lastPos);
    
    // Delta should be normalized to viewport size
    float expectedDeltaX = 50.0f / viewport->getWidth();
    float expectedDeltaY = 50.0f / viewport->getHeight();
    
    EXPECT_FLOAT_EQ(delta.x, expectedDeltaX);
    EXPECT_FLOAT_EQ(delta.y, expectedDeltaY);
}

TEST_F(ViewportTest, ZoomFactor) {
    // Test default size
    float zoomFactor = viewport->getZoomFactor();
    float expectedFactor = std::min(800, 600) / 800.0f; // min(width, height) / 800
    EXPECT_FLOAT_EQ(zoomFactor, expectedFactor);
    
    // Test different size
    viewport->setSize(1600, 1200);
    zoomFactor = viewport->getZoomFactor();
    expectedFactor = std::min(1600, 1200) / 800.0f;
    EXPECT_FLOAT_EQ(zoomFactor, expectedFactor);
    
    // Test small viewport
    viewport->setSize(400, 300);
    zoomFactor = viewport->getZoomFactor();
    expectedFactor = std::min(400, 300) / 800.0f;
    EXPECT_FLOAT_EQ(zoomFactor, expectedFactor);
}

TEST_F(ViewportTest, AspectRatioUpdates) {
    // REQ-CAM-2: Camera shall support multiple view projections (proper aspect ratio handling)
    // Square viewport
    viewport->setSize(600, 600);
    EXPECT_FLOAT_EQ(viewport->getAspectRatio(), 1.0f);
    
    // Wide viewport
    viewport->setSize(1920, 1080);
    EXPECT_FLOAT_EQ(viewport->getAspectRatio(), 1920.0f / 1080.0f);
    
    // Tall viewport
    viewport->setSize(480, 854);
    EXPECT_FLOAT_EQ(viewport->getAspectRatio(), 480.0f / 854.0f);
}

TEST_F(ViewportTest, EdgeCaseCoordinates) {
    // Test coordinates exactly at viewport boundaries
    Vector2i topLeft(viewport->getX(), viewport->getY());
    EXPECT_TRUE(viewport->contains(topLeft));
    
    Vector2i topRight(viewport->getX() + viewport->getWidth() - 1, viewport->getY());
    EXPECT_TRUE(viewport->contains(topRight));
    
    Vector2i bottomLeft(viewport->getX(), viewport->getY() + viewport->getHeight() - 1);
    EXPECT_TRUE(viewport->contains(bottomLeft));
    
    Vector2i bottomRight(viewport->getX() + viewport->getWidth() - 1, 
                        viewport->getY() + viewport->getHeight() - 1);
    EXPECT_TRUE(viewport->contains(bottomRight));
    
    // Test coordinates just outside boundaries
    Vector2i justOutsideRight(viewport->getX() + viewport->getWidth(), viewport->getY());
    EXPECT_FALSE(viewport->contains(justOutsideRight));
    
    Vector2i justOutsideBottom(viewport->getX(), viewport->getY() + viewport->getHeight());
    EXPECT_FALSE(viewport->contains(justOutsideBottom));
}

TEST_F(ViewportTest, SmallViewport) {
    // Test very small viewport
    Viewport smallViewport(0, 0, 1, 1);
    
    EXPECT_EQ(smallViewport.getWidth(), 1);
    EXPECT_EQ(smallViewport.getHeight(), 1);
    EXPECT_FLOAT_EQ(smallViewport.getAspectRatio(), 1.0f);
    
    EXPECT_TRUE(smallViewport.contains(0, 0));
    EXPECT_FALSE(smallViewport.contains(1, 1));
}

// ===== Comprehensive Ray Generation Tests =====

TEST_F(ViewportTest, ScreenToWorldRay_AllCorners) {
    // Test ray generation from all corners and edges
    Matrix4f viewMatrix = Matrix4f::lookAt(
        Vector3f(0, 0, 5),  // Eye at positive Z
        Vector3f(0, 0, 0),  // Looking at origin
        Vector3f(0, 1, 0)   // Y up
    );
    
    Matrix4f projMatrix = Matrix4f::perspective(
        90.0f,                      // 90 degree FOV for easier testing
        viewport->getAspectRatio(), 
        0.1f, 
        1000.0f
    );
    
    // Test top-left corner
    Vector2i topLeft(viewport->getX(), viewport->getY());
    Ray topLeftRay = viewport->screenToWorldRay(topLeft, viewMatrix, projMatrix);
    EXPECT_NEAR(topLeftRay.direction.length(), 1.0f, 0.001f);
    EXPECT_LT(topLeftRay.direction.x, 0.0f); // Should point left
    EXPECT_GT(topLeftRay.direction.y, 0.0f); // Should point up
    EXPECT_LT(topLeftRay.direction.z, 0.0f); // Should point forward
    
    // Test top-right corner
    Vector2i topRight(viewport->getX() + viewport->getWidth() - 1, viewport->getY());
    Ray topRightRay = viewport->screenToWorldRay(topRight, viewMatrix, projMatrix);
    EXPECT_NEAR(topRightRay.direction.length(), 1.0f, 0.001f);
    EXPECT_GT(topRightRay.direction.x, 0.0f); // Should point right
    EXPECT_GT(topRightRay.direction.y, 0.0f); // Should point up
    EXPECT_LT(topRightRay.direction.z, 0.0f); // Should point forward
    
    // Test bottom-left corner
    Vector2i bottomLeft(viewport->getX(), viewport->getY() + viewport->getHeight() - 1);
    Ray bottomLeftRay = viewport->screenToWorldRay(bottomLeft, viewMatrix, projMatrix);
    EXPECT_NEAR(bottomLeftRay.direction.length(), 1.0f, 0.001f);
    EXPECT_LT(bottomLeftRay.direction.x, 0.0f); // Should point left
    EXPECT_LT(bottomLeftRay.direction.y, 0.0f); // Should point down
    EXPECT_LT(bottomLeftRay.direction.z, 0.0f); // Should point forward
    
    // Test bottom-right corner
    Vector2i bottomRight(viewport->getX() + viewport->getWidth() - 1, 
                        viewport->getY() + viewport->getHeight() - 1);
    Ray bottomRightRay = viewport->screenToWorldRay(bottomRight, viewMatrix, projMatrix);
    EXPECT_NEAR(bottomRightRay.direction.length(), 1.0f, 0.001f);
    EXPECT_GT(bottomRightRay.direction.x, 0.0f); // Should point right
    EXPECT_LT(bottomRightRay.direction.y, 0.0f); // Should point down
    EXPECT_LT(bottomRightRay.direction.z, 0.0f); // Should point forward
}

TEST_F(ViewportTest, ScreenToWorldRay_DifferentCameraPositions) {
    // Test ray generation with different camera configurations
    Matrix4f projMatrix = Matrix4f::perspective(
        45.0f, viewport->getAspectRatio(), 0.1f, 1000.0f
    );
    
    Vector2i center(viewport->getX() + viewport->getWidth() / 2,
                   viewport->getY() + viewport->getHeight() / 2);
    
    // Camera looking down from above
    {
        Matrix4f viewMatrix = Matrix4f::lookAt(
            Vector3f(0, 10, 0),  // Above origin
            Vector3f(0, 0, 0),   // Looking at origin
            Vector3f(0, 0, -1)   // Z is forward
        );
        
        Ray ray = viewport->screenToWorldRay(center, viewMatrix, projMatrix);
        EXPECT_NEAR(ray.direction.length(), 1.0f, 0.001f);
        EXPECT_NEAR(ray.direction.x, 0.0f, 0.1f);
        EXPECT_LT(ray.direction.y, 0.0f); // Should point down
        EXPECT_NEAR(ray.direction.z, 0.0f, 0.1f);
    }
    
    // Camera at angle
    {
        Matrix4f viewMatrix = Matrix4f::lookAt(
            Vector3f(5, 5, 5),   // At diagonal
            Vector3f(0, 0, 0),   // Looking at origin
            Vector3f(0, 1, 0)    // Y up
        );
        
        Ray ray = viewport->screenToWorldRay(center, viewMatrix, projMatrix);
        EXPECT_NEAR(ray.direction.length(), 1.0f, 0.001f);
        // Should point toward origin from camera position
        Vector3f expectedDir = Vector3f(0, 0, 0) - Vector3f(5, 5, 5);
        expectedDir = expectedDir.normalized();
        EXPECT_NEAR(ray.direction.x, expectedDir.x, 0.1f);
        EXPECT_NEAR(ray.direction.y, expectedDir.y, 0.1f);
        EXPECT_NEAR(ray.direction.z, expectedDir.z, 0.1f);
    }
}

TEST_F(ViewportTest, ScreenToWorldRay_OrthographicProjection) {
    // Test with orthographic projection
    Matrix4f viewMatrix = Matrix4f::lookAt(
        Vector3f(0, 0, 5),
        Vector3f(0, 0, 0),
        Vector3f(0, 1, 0)
    );
    
    float halfWidth = 5.0f;
    float halfHeight = halfWidth / viewport->getAspectRatio();
    Matrix4f orthoMatrix = Matrix4f::orthographic(
        -halfWidth, halfWidth,
        -halfHeight, halfHeight,
        0.1f, 1000.0f
    );
    
    // With orthographic projection, all rays should be parallel
    Vector2i pos1(viewport->getX() + 100, viewport->getY() + 100);
    Vector2i pos2(viewport->getX() + 200, viewport->getY() + 200);
    
    Ray ray1 = viewport->screenToWorldRay(pos1, viewMatrix, orthoMatrix);
    Ray ray2 = viewport->screenToWorldRay(pos2, viewMatrix, orthoMatrix);
    
    // Directions should be parallel (same direction)
    EXPECT_NEAR(ray1.direction.x, ray2.direction.x, 0.001f);
    EXPECT_NEAR(ray1.direction.y, ray2.direction.y, 0.001f);
    EXPECT_NEAR(ray1.direction.z, ray2.direction.z, 0.001f);
    
    // Both should point forward
    EXPECT_NEAR(ray1.direction.x, 0.0f, 0.001f);
    EXPECT_NEAR(ray1.direction.y, 0.0f, 0.001f);
    EXPECT_NEAR(ray1.direction.z, -1.0f, 0.001f);
}

TEST_F(ViewportTest, ScreenToWorldRay_EdgeCases) {
    Matrix4f viewMatrix = Matrix4f::lookAt(
        Vector3f(0, 0, 5),
        Vector3f(0, 0, 0),
        Vector3f(0, 1, 0)
    );
    
    Matrix4f projMatrix = Matrix4f::perspective(
        45.0f, viewport->getAspectRatio(), 0.1f, 1000.0f
    );
    
    // Test positions outside viewport
    {
        Vector2i outsidePos(-100, -100);
        Ray ray = viewport->screenToWorldRay(outsidePos, viewMatrix, projMatrix);
        // Should still generate a valid ray
        EXPECT_NEAR(ray.direction.length(), 1.0f, 0.001f);
    }
    
    // Test with very wide FOV
    {
        Matrix4f wideFovMatrix = Matrix4f::perspective(
            170.0f, viewport->getAspectRatio(), 0.1f, 1000.0f
        );
        
        Vector2i center(viewport->getX() + viewport->getWidth() / 2,
                       viewport->getY() + viewport->getHeight() / 2);
        Ray ray = viewport->screenToWorldRay(center, viewMatrix, wideFovMatrix);
        EXPECT_NEAR(ray.direction.length(), 1.0f, 0.001f);
    }
    
    // Test with very narrow FOV
    {
        Matrix4f narrowFovMatrix = Matrix4f::perspective(
            5.0f, viewport->getAspectRatio(), 0.1f, 1000.0f
        );
        
        Vector2i center(viewport->getX() + viewport->getWidth() / 2,
                       viewport->getY() + viewport->getHeight() / 2);
        Ray ray = viewport->screenToWorldRay(center, viewMatrix, narrowFovMatrix);
        EXPECT_NEAR(ray.direction.length(), 1.0f, 0.001f);
    }
}

TEST_F(ViewportTest, ScreenToWorldRay_ConsistencyCheck) {
    // Verify that ray generation is consistent and reversible
    Matrix4f viewMatrix = Matrix4f::lookAt(
        Vector3f(3, 4, 5),
        Vector3f(0, 0, 0),
        Vector3f(0, 1, 0)
    );
    
    Matrix4f projMatrix = Matrix4f::perspective(
        60.0f, viewport->getAspectRatio(), 0.1f, 1000.0f
    );
    
    // Test multiple screen positions
    std::vector<Vector2i> testPositions = {
        Vector2i(viewport->getX() + 100, viewport->getY() + 100),
        Vector2i(viewport->getX() + 400, viewport->getY() + 300),
        Vector2i(viewport->getX() + 700, viewport->getY() + 500)
    };
    
    for (const auto& screenPos : testPositions) {
        Ray ray = viewport->screenToWorldRay(screenPos, viewMatrix, projMatrix);
        
        // Ray should originate near camera position
        float distToCamera = (ray.origin - Vector3f(3, 4, 5)).length();
        EXPECT_LT(distToCamera, 1.0f) << "Ray origin should be near camera";
        
        // Project a point along the ray back to screen
        Vector3f worldPoint = ray.origin + ray.direction * 10.0f;
        Vector2i projectedBack = viewport->worldToScreen(worldPoint, viewMatrix, projMatrix);
        
        // Should be close to original screen position (within a few pixels)
        EXPECT_NEAR(projectedBack.x, screenPos.x, 5);
        EXPECT_NEAR(projectedBack.y, screenPos.y, 5);
    }
}

TEST_F(ViewportTest, ScreenToWorldRay_RayPlaneIntersection) {
    // Test that generated rays can correctly intersect with planes
    Matrix4f viewMatrix = Matrix4f::lookAt(
        Vector3f(0, 5, 5),
        Vector3f(0, 0, 0),
        Vector3f(0, 1, 0)
    );
    
    Matrix4f projMatrix = Matrix4f::perspective(
        45.0f, viewport->getAspectRatio(), 0.1f, 1000.0f
    );
    
    // Generate ray toward ground plane (Y=0)
    Vector2i center(viewport->getX() + viewport->getWidth() / 2,
                   viewport->getY() + viewport->getHeight() / 2);
    Ray ray = viewport->screenToWorldRay(center, viewMatrix, projMatrix);
    
    // Calculate intersection with Y=0 plane
    if (ray.direction.y < 0) { // Ray points down
        float t = -ray.origin.y / ray.direction.y;
        Vector3f intersection = ray.origin + ray.direction * t;
        
        // Intersection should be near origin since camera looks at origin
        EXPECT_NEAR(intersection.x, 0.0f, 1.0f);
        EXPECT_NEAR(intersection.y, 0.0f, 0.001f);
        EXPECT_NEAR(intersection.z, 0.0f, 1.0f);
    }
}

TEST_F(ViewportTest, ScreenToWorldRay_NonInvertibleMatrix) {
    // Test handling of non-invertible matrices
    Matrix4f viewMatrix = Matrix4f::Identity();
    Matrix4f singularMatrix = Matrix4f::zero(); // Non-invertible
    
    Vector2i center(viewport->getX() + viewport->getWidth() / 2,
                   viewport->getY() + viewport->getHeight() / 2);
    
    // Should return a default ray without crashing
    Ray ray = viewport->screenToWorldRay(center, viewMatrix, singularMatrix);
    
    // Default ray should point down negative Z
    EXPECT_NEAR(ray.direction.x, 0.0f, 0.001f);
    EXPECT_NEAR(ray.direction.y, 0.0f, 0.001f);
    EXPECT_NEAR(ray.direction.z, -1.0f, 0.001f);
}