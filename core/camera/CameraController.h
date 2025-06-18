#pragma once

#include <memory>

#include "OrbitCamera.h"
#include "Viewport.h"
#include "../../foundation/math/Vector2i.h"
#include "../../foundation/math/Ray.h"
#include "../../foundation/math/CoordinateTypes.h"

namespace VoxelEditor {
namespace Camera {

enum class InteractionMode {
    NONE,
    ORBIT,
    PAN,
    ZOOM
};

class CameraController {
public:
    // Constants
    static constexpr float DEFAULT_DRAG_THRESHOLD = 3.0f;  // pixels
    static constexpr float ORBIT_SCALE = 180.0f;          // degrees per normalized delta
    static constexpr float PAN_DISTANCE_FACTOR = 10.0f;   // reference distance for pan scaling
    static constexpr float ZOOM_SCALE = 0.5f;             // zoom speed factor

    CameraController(Events::EventDispatcher* eventDispatcher = nullptr)
        : m_camera(std::make_unique<OrbitCamera>(eventDispatcher))
        , m_viewport(std::make_unique<Viewport>())
        , m_interactionMode(InteractionMode::NONE)
        , m_lastMousePos(0, 0)
        , m_mouseDragThreshold(DEFAULT_DRAG_THRESHOLD)
        , m_isDragging(false) {
        
        // Set up camera to use viewport aspect ratio
        updateCameraAspectRatio();
    }

    ~CameraController() = default;

    // Non-copyable but movable
    CameraController(const CameraController&) = delete;
    CameraController& operator=(const CameraController&) = delete;
    CameraController(CameraController&&) = default;
    CameraController& operator=(CameraController&&) = default;

    // Camera access
    OrbitCamera* getCamera() const { return m_camera.get(); }
    Viewport* getViewport() const { return m_viewport.get(); }

    // Viewport management
    void setViewportSize(int width, int height) {
        m_viewport->setSize(width, height);
        updateCameraAspectRatio();
    }

    void setViewportBounds(int x, int y, int width, int height) {
        m_viewport->setBounds(x, y, width, height);
        updateCameraAspectRatio();
    }

    // Mouse interaction
    void onMouseButtonDown(const Math::Vector2i& mousePos, int button) {
        if (!m_viewport->contains(mousePos)) {
            return;
        }

        m_lastMousePos = mousePos;
        m_isDragging = false;

        // Button mapping: 0=left, 1=middle, 2=right
        switch (button) {
            case 0: // Left button - orbit
                m_interactionMode = InteractionMode::ORBIT;
                break;
            case 1: // Middle button - pan
                m_interactionMode = InteractionMode::PAN;
                break;
            case 2: // Right button - zoom
                m_interactionMode = InteractionMode::ZOOM;
                break;
            default:
                m_interactionMode = InteractionMode::NONE;
                break;
        }
    }

    void onMouseButtonUp(const Math::Vector2i& mousePos, int button) {
        m_interactionMode = InteractionMode::NONE;
        m_isDragging = false;
    }

    void onMouseMove(const Math::Vector2i& mousePos) {
        if (!m_viewport->contains(mousePos) || m_interactionMode == InteractionMode::NONE) {
            return;
        }

        Math::Vector2i delta = mousePos - m_lastMousePos;
        
        // Check if we've exceeded drag threshold
        if (!m_isDragging) {
            float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (distance > m_mouseDragThreshold) {
                m_isDragging = true;
            } else {
                return; // Not dragging yet
            }
        }

        // Convert to viewport-normalized coordinates
        Math::Vector2f normalizedDelta = m_viewport->getMouseDelta(mousePos, m_lastMousePos);

        switch (m_interactionMode) {
            case InteractionMode::ORBIT:
                handleOrbit(normalizedDelta);
                break;
            case InteractionMode::PAN:
                handlePan(normalizedDelta);
                break;
            case InteractionMode::ZOOM:
                handleZoom(normalizedDelta);
                break;
            case InteractionMode::NONE:
                break;
        }

        m_lastMousePos = mousePos;
    }

    void onMouseWheel(const Math::Vector2i& mousePos, float delta) {
        if (!m_viewport->contains(mousePos)) {
            return;
        }

        // Zoom towards cursor position
        float zoomAmount = delta * m_camera->getZoomSensitivity() * 0.1f;
        m_camera->zoom(zoomAmount);
    }

    // Keyboard shortcuts for view presets
    void setViewPreset(ViewPreset preset) {
        m_camera->setViewPreset(preset);
    }

    // Frame objects
    void frameAll(const Math::WorldCoordinates& minBounds, const Math::WorldCoordinates& maxBounds) {
        m_camera->frameBox(minBounds, maxBounds);
    }

    void focusOn(const Math::WorldCoordinates& point, float distance = -1.0f) {
        m_camera->focusOn(point, distance);
    }

    // 3D interaction
    Math::Ray getMouseRay(const Math::Vector2i& mousePos) const {
        return m_viewport->screenToWorldRay(
            mousePos,
            m_camera->getViewMatrix(),
            m_camera->getProjectionMatrix()
        );
    }

    Math::Vector2i worldToScreen(const Math::Vector3f& worldPos) const {
        return m_viewport->worldToScreen(
            worldPos,
            m_camera->getViewMatrix(),
            m_camera->getProjectionMatrix()
        );
    }

    // Update for animation/smoothing
    void update(float deltaTime) {
        m_camera->update(deltaTime);
    }

    // Settings
    void setMouseDragThreshold(float threshold) {
        m_mouseDragThreshold = threshold;
    }

    void setCameraSensitivity(float panSensitivity, float rotateSensitivity, float zoomSensitivity) {
        m_camera->setPanSensitivity(panSensitivity);
        m_camera->setRotateSensitivity(rotateSensitivity);
        m_camera->setZoomSensitivity(zoomSensitivity);
    }

    void setCameraSmoothing(bool enabled, float smoothFactor = 0.1f) {
        m_camera->setSmoothing(enabled);
        if (enabled) {
            m_camera->setSmoothFactor(smoothFactor);
        }
    }

    void setCameraConstraints(float minDistance, float maxDistance, float minPitch, float maxPitch) {
        m_camera->setDistanceConstraints(minDistance, maxDistance);
        m_camera->setPitchConstraints(minPitch, maxPitch);
    }

    // State queries
    bool isInteracting() const {
        return m_interactionMode != InteractionMode::NONE && m_isDragging;
    }

    InteractionMode getInteractionMode() const {
        return m_interactionMode;
    }

    float getMouseDragThreshold() const {
        return m_mouseDragThreshold;
    }

private:
    void updateCameraAspectRatio() {
        m_camera->setAspectRatio(m_viewport->getAspectRatio());
    }

    void handleOrbit(const Math::Vector2f& delta) {
        // Scale rotation based on distance for more intuitive control
        float distanceScale = m_camera->getDistance() / PAN_DISTANCE_FACTOR;
        distanceScale = Math::clamp(distanceScale, 0.1f, 2.0f);
        
        m_camera->orbit(
            -delta.x * ORBIT_SCALE * distanceScale, // Negative for intuitive left/right
            -delta.y * ORBIT_SCALE * distanceScale  // Negative for intuitive up/down
        );
    }

    void handlePan(const Math::Vector2f& delta) {
        // Scale panning based on distance for consistent feel
        float distanceScale = m_camera->getDistance() / PAN_DISTANCE_FACTOR;
        
        Math::Vector3f panDelta(
            -delta.x * distanceScale, // Negative for intuitive left/right
            delta.y * distanceScale,  // Positive for intuitive up/down
            0.0f
        );
        
        m_camera->pan(panDelta);
    }

    void handleZoom(const Math::Vector2f& delta) {
        // Use vertical mouse movement for zoom
        float zoomDelta = delta.y * m_camera->getDistance() * ZOOM_SCALE;
        m_camera->zoom(zoomDelta);
    }

    std::unique_ptr<OrbitCamera> m_camera;
    std::unique_ptr<Viewport> m_viewport;
    
    InteractionMode m_interactionMode;
    Math::Vector2i m_lastMousePos;
    float m_mouseDragThreshold;
    bool m_isDragging;
};

}
}