#include <gtest/gtest.h>
#include "../Camera.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor::Camera;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

// Concrete implementation for testing the abstract Camera class
class TestCamera : public Camera {
public:
    TestCamera(VoxelEditor::Events::EventDispatcher* eventDispatcher = nullptr)
        : Camera(eventDispatcher) {}

    void setViewPreset(ViewPreset preset) override {
        // Simple test implementation
        switch (preset) {
            case ViewPreset::FRONT:
                setPosition(WorldCoordinates(Vector3f(0, 0, 5)));
                setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
                break;
            case ViewPreset::TOP:
                setPosition(WorldCoordinates(Vector3f(0, 5, 0)));
                setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
                break;
            default:
                break;
        }
    }
};

// Event handler for testing
class TestCameraChangedHandler : public EventHandler<CameraChangedEvent> {
public:
    void handleEvent(const CameraChangedEvent& event) override {
        eventCount++;
        lastChangeType = event.changeType;
    }
    
    int eventCount = 0;
    CameraChangedEvent::ChangeType lastChangeType = CameraChangedEvent::ChangeType::POSITION;
};

class CameraTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        camera = std::make_unique<TestCamera>(eventDispatcher.get());
        
        eventHandler = std::make_unique<TestCameraChangedHandler>();
        eventDispatcher->subscribe<CameraChangedEvent>(eventHandler.get());
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<TestCamera> camera;
    std::unique_ptr<TestCameraChangedHandler> eventHandler;
};

TEST_F(CameraTest, DefaultConstruction) {
    // REQ-CAM-1: Camera system shall provide orbit-style controls
    // REQ-CAM-3: Camera shall maintain consistent coordinate system
    TestCamera defaultCamera;
    
    EXPECT_EQ(defaultCamera.getPosition().value(), Vector3f(0.0f, 0.0f, 5.0f));
    EXPECT_EQ(defaultCamera.getTarget().value(), Vector3f(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(defaultCamera.getUp().value(), Vector3f(0.0f, 1.0f, 0.0f));
    EXPECT_FLOAT_EQ(defaultCamera.getFieldOfView(), 45.0f);
    EXPECT_FLOAT_EQ(defaultCamera.getAspectRatio(), 16.0f / 9.0f);
    EXPECT_FLOAT_EQ(defaultCamera.getNearPlane(), 0.1f);
    EXPECT_FLOAT_EQ(defaultCamera.getFarPlane(), 1000.0f);
}

TEST_F(CameraTest, PositionManagement) {
    WorldCoordinates newPosition(Vector3f(10.0f, 5.0f, 15.0f));
    camera->setPosition(newPosition);
    
    EXPECT_EQ(camera->getPosition().value(), newPosition.value());
    EXPECT_EQ(eventHandler->eventCount, 1);
    EXPECT_EQ(eventHandler->lastChangeType, CameraChangedEvent::ChangeType::POSITION);
    
    // Setting same position should not trigger event
    camera->setPosition(newPosition);
    EXPECT_EQ(eventHandler->eventCount, 1);
}

TEST_F(CameraTest, TargetManagement) {
    WorldCoordinates newTarget(Vector3f(5.0f, 2.0f, 3.0f));
    camera->setTarget(newTarget);
    
    EXPECT_EQ(camera->getTarget().value(), newTarget.value());
    EXPECT_EQ(eventHandler->eventCount, 1);
    EXPECT_EQ(eventHandler->lastChangeType, CameraChangedEvent::ChangeType::POSITION);
}

TEST_F(CameraTest, UpVectorManagement) {
    WorldCoordinates newUp(Vector3f(0.0f, 0.0f, 1.0f));
    camera->setUp(newUp);
    
    EXPECT_EQ(camera->getUp().value(), newUp.value());
    EXPECT_EQ(eventHandler->eventCount, 1);
    EXPECT_EQ(eventHandler->lastChangeType, CameraChangedEvent::ChangeType::ROTATION);
}

TEST_F(CameraTest, ProjectionSettings) {
    // REQ-CAM-2: Camera shall support multiple view projections
    // Test field of view
    camera->setFieldOfView(60.0f);
    EXPECT_FLOAT_EQ(camera->getFieldOfView(), 60.0f);
    EXPECT_EQ(eventHandler->eventCount, 1);
    EXPECT_EQ(eventHandler->lastChangeType, CameraChangedEvent::ChangeType::ZOOM);
    
    // Test aspect ratio
    camera->setAspectRatio(4.0f / 3.0f);
    EXPECT_FLOAT_EQ(camera->getAspectRatio(), 4.0f / 3.0f);
    
    // Test near/far planes
    camera->setNearFarPlanes(0.5f, 500.0f);
    EXPECT_FLOAT_EQ(camera->getNearPlane(), 0.5f);
    EXPECT_FLOAT_EQ(camera->getFarPlane(), 500.0f);
}

TEST_F(CameraTest, DirectionVectors) {
    // REQ-CAM-3: Camera shall maintain consistent coordinate system (Y-up orientation)
    camera->setPosition(WorldCoordinates(Vector3f(0, 0, 5)));
    camera->setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
    camera->setUp(WorldCoordinates(Vector3f(0, 1, 0)));
    
    Vector3f forward = camera->getForward();
    Vector3f right = camera->getRight();
    Vector3f up = camera->getActualUp();
    
    // Forward should point towards target
    EXPECT_NEAR(forward.x, 0.0f, 0.001f);
    EXPECT_NEAR(forward.y, 0.0f, 0.001f);
    EXPECT_NEAR(forward.z, -1.0f, 0.001f);
    
    // Right should be perpendicular to forward and up
    EXPECT_NEAR(right.x, 1.0f, 0.001f);
    EXPECT_NEAR(right.y, 0.0f, 0.001f);
    EXPECT_NEAR(right.z, 0.0f, 0.001f);
    
    // Up should be the actual up vector
    EXPECT_NEAR(up.x, 0.0f, 0.001f);
    EXPECT_NEAR(up.y, 1.0f, 0.001f);
    EXPECT_NEAR(up.z, 0.0f, 0.001f);
}

TEST_F(CameraTest, ViewMatrix) {
    camera->setPosition(WorldCoordinates(Vector3f(0, 0, 5)));
    camera->setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
    camera->setUp(WorldCoordinates(Vector3f(0, 1, 0)));
    
    Matrix4f viewMatrix = camera->getViewMatrix();
    
    // View matrix should be valid (non-zero determinant)
    EXPECT_NE(viewMatrix.determinant(), 0.0f);
    
    // Multiple calls should return same matrix (cached)
    Matrix4f viewMatrix2 = camera->getViewMatrix();
    EXPECT_EQ(viewMatrix, viewMatrix2);
}

TEST_F(CameraTest, ProjectionMatrix) {
    camera->setFieldOfView(45.0f);
    camera->setAspectRatio(16.0f / 9.0f);
    camera->setNearFarPlanes(0.1f, 1000.0f);
    
    Matrix4f projMatrix = camera->getProjectionMatrix();
    
    // Projection matrix should be valid
    EXPECT_NE(projMatrix.determinant(), 0.0f);
    
    // Multiple calls should return same matrix (cached)
    Matrix4f projMatrix2 = camera->getProjectionMatrix();
    EXPECT_EQ(projMatrix, projMatrix2);
}

TEST_F(CameraTest, ViewProjectionMatrix) {
    Matrix4f viewProjMatrix = camera->getViewProjectionMatrix();
    Matrix4f expectedMatrix = camera->getProjectionMatrix() * camera->getViewMatrix();
    
    EXPECT_EQ(viewProjMatrix, expectedMatrix);
}

TEST_F(CameraTest, MatrixCaching) {
    // Initially, matrices should be dirty and computed
    Matrix4f viewMatrix1 = camera->getViewMatrix();
    Matrix4f projMatrix1 = camera->getProjectionMatrix();
    
    // Getting matrices again should return cached versions
    Matrix4f viewMatrix2 = camera->getViewMatrix();
    Matrix4f projMatrix2 = camera->getProjectionMatrix();
    
    EXPECT_EQ(viewMatrix1, viewMatrix2);
    EXPECT_EQ(projMatrix1, projMatrix2);
    
    // Changing position should invalidate view matrix
    camera->setPosition(WorldCoordinates(Vector3f(1, 1, 1)));
    Matrix4f viewMatrix3 = camera->getViewMatrix();
    EXPECT_NE(viewMatrix1, viewMatrix3);
    
    // Changing FOV should invalidate projection matrix
    camera->setFieldOfView(60.0f);
    Matrix4f projMatrix3 = camera->getProjectionMatrix();
    EXPECT_NE(projMatrix1, projMatrix3);
}

TEST_F(CameraTest, ViewPresets) {
    // REQ-CAM-4: Camera shall provide standard view presets
    // REQ-9.2.2: CLI shall support camera commands (view)
    camera->setViewPreset(ViewPreset::FRONT);
    EXPECT_EQ(camera->getPosition().value(), Vector3f(0, 0, 5));
    EXPECT_EQ(camera->getTarget().value(), Vector3f(0, 0, 0));
    
    camera->setViewPreset(ViewPreset::TOP);
    EXPECT_EQ(camera->getPosition().value(), Vector3f(0, 5, 0));
    EXPECT_EQ(camera->getTarget().value(), Vector3f(0, 0, 0));
}

TEST_F(CameraTest, EventDispatcherManagement) {
    // REQ-CAM-8: Camera shall integrate with other subsystems (event system)
    // Test with null event dispatcher
    TestCamera cameraNoEvents;
    cameraNoEvents.setPosition(WorldCoordinates(Vector3f(1, 2, 3)));
    // Should not crash
    
    // Test changing event dispatcher
    camera->setEventDispatcher(nullptr);
    int eventCountBefore = eventHandler->eventCount;
    camera->setPosition(WorldCoordinates(Vector3f(5, 5, 5)));
    EXPECT_EQ(eventHandler->eventCount, eventCountBefore); // No new events
    
    // Restore event dispatcher
    camera->setEventDispatcher(eventDispatcher.get());
    camera->setPosition(WorldCoordinates(Vector3f(6, 6, 6)));
    EXPECT_EQ(eventHandler->eventCount, eventCountBefore + 1); // New event dispatched
}

TEST_F(CameraTest, EdgeCases) {
    // Test very small FOV
    camera->setFieldOfView(1.0f);
    EXPECT_FLOAT_EQ(camera->getFieldOfView(), 1.0f);
    
    // Test very large FOV
    camera->setFieldOfView(179.0f);
    EXPECT_FLOAT_EQ(camera->getFieldOfView(), 179.0f);
    
    // Test extreme aspect ratios
    camera->setAspectRatio(0.1f);
    EXPECT_FLOAT_EQ(camera->getAspectRatio(), 0.1f);
    
    camera->setAspectRatio(10.0f);
    EXPECT_FLOAT_EQ(camera->getAspectRatio(), 10.0f);
    
    // Test near/far plane edge cases
    camera->setNearFarPlanes(0.001f, 100000.0f);
    EXPECT_FLOAT_EQ(camera->getNearPlane(), 0.001f);
    EXPECT_FLOAT_EQ(camera->getFarPlane(), 100000.0f);
}

TEST_F(CameraTest, ProjectionMatrixWithAspectRatio) {
    // Test projection matrix generation with different aspect ratios
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    
    camera->setFieldOfView(fov);
    camera->setNearFarPlanes(nearPlane, farPlane);
    
    // Test 1: Square aspect ratio (1:1)
    {
        camera->setAspectRatio(1.0f);
        Matrix4f proj = camera->getProjectionMatrix();
        
        // For square aspect, horizontal and vertical FOV should be same
        float fovRad = fov * M_PI / 180.0f;
        float expectedDiagonal = 1.0f / tanf(fovRad / 2.0f);
        
        EXPECT_NEAR(proj.m[0], expectedDiagonal, 0.001f); // X scaling
        EXPECT_NEAR(proj.m[5], expectedDiagonal, 0.001f); // Y scaling (should be same)
    }
    
    // Test 2: Wide aspect ratio (16:9)
    {
        float aspect = 16.0f / 9.0f;
        camera->setAspectRatio(aspect);
        Matrix4f proj = camera->getProjectionMatrix();
        
        float fovRad = fov * M_PI / 180.0f;
        float yScale = 1.0f / tanf(fovRad / 2.0f);
        float xScale = yScale / aspect;
        
        EXPECT_NEAR(proj.m[0], xScale, 0.001f);
        EXPECT_NEAR(proj.m[5], yScale, 0.001f);
        
        // X scale should be less than Y scale for wide aspect
        EXPECT_LT(proj.m[0], proj.m[5]);
    }
    
    // Test 3: Tall aspect ratio (9:16)
    {
        float aspect = 9.0f / 16.0f;
        camera->setAspectRatio(aspect);
        Matrix4f proj = camera->getProjectionMatrix();
        
        float fovRad = fov * M_PI / 180.0f;
        float yScale = 1.0f / tanf(fovRad / 2.0f);
        float xScale = yScale / aspect;
        
        EXPECT_NEAR(proj.m[0], xScale, 0.001f);
        EXPECT_NEAR(proj.m[5], yScale, 0.001f);
        
        // X scale should be greater than Y scale for tall aspect
        EXPECT_GT(proj.m[0], proj.m[5]);
    }
    
    // Test 4: Verify frustum shape changes with aspect ratio
    {
        // Create test points at the edges of the near plane
        camera->setAspectRatio(2.0f); // Wide aspect
        Matrix4f projWide = camera->getProjectionMatrix();
        
        camera->setAspectRatio(0.5f); // Tall aspect
        Matrix4f projTall = camera->getProjectionMatrix();
        
        // Point at right edge of near plane
        Vector4f rightEdge(nearPlane * tanf(fov * M_PI / 360.0f) * 2.0f, 0, -nearPlane, 1);
        Vector4f rightWide = projWide * rightEdge;
        Vector4f rightTall = projTall * rightEdge;
        
        // Wide aspect should map the right edge differently than tall aspect
        float ndcXWide = rightWide.x / rightWide.w;
        float ndcXTall = rightTall.x / rightTall.w;
        
        EXPECT_NE(ndcXWide, ndcXTall);
        EXPECT_LT(std::abs(ndcXWide), std::abs(ndcXTall)); // Wide aspect compresses X more
    }
}

TEST_F(CameraTest, VectorNormalization) {
    // Test that direction vectors are properly normalized
    camera->setPosition(WorldCoordinates(Vector3f(10, 20, 30)));
    camera->setTarget(WorldCoordinates(Vector3f(5, 15, 25)));
    
    Vector3f forward = camera->getForward();
    Vector3f right = camera->getRight();
    Vector3f up = camera->getActualUp();
    
    // All direction vectors should be unit length
    EXPECT_NEAR(forward.length(), 1.0f, 0.001f);
    EXPECT_NEAR(right.length(), 1.0f, 0.001f);
    EXPECT_NEAR(up.length(), 1.0f, 0.001f);
    
    // Vectors should be orthogonal
    EXPECT_NEAR(forward.dot(right), 0.0f, 0.001f);
    EXPECT_NEAR(forward.dot(up), 0.0f, 0.001f);
    EXPECT_NEAR(right.dot(up), 0.0f, 0.001f);
}