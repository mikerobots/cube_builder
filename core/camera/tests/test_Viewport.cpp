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