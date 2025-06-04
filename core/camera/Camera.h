#pragma once

#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Matrix4f.h"
#include "../../foundation/math/MathUtils.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/events/CommonEvents.h"
#include "../../foundation/logging/Logger.h"

namespace VoxelEditor {
namespace Camera {

enum class ViewPreset {
    FRONT,
    BACK, 
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    ISOMETRIC
};

class Camera {
public:
    Camera(Events::EventDispatcher* eventDispatcher = nullptr)
        : m_eventDispatcher(eventDispatcher)
        , m_position(0.0f, 0.0f, 5.0f)
        , m_target(0.0f, 0.0f, 0.0f)
        , m_up(0.0f, 1.0f, 0.0f)
        , m_fov(45.0f)
        , m_nearPlane(0.1f)
        , m_farPlane(1000.0f)
        , m_aspectRatio(16.0f / 9.0f)
        , m_viewMatrixDirty(true)
        , m_projectionMatrixDirty(true) {}

    virtual ~Camera() = default;

    // Camera positioning
    virtual void setPosition(const Math::Vector3f& position) {
        if (m_position != position) {
            m_position = position;
            m_viewMatrixDirty = true;
            Logging::Logger::getInstance().debugfc("Camera", "Position changed to (%.3f, %.3f, %.3f)", 
                position.x, position.y, position.z);
            dispatchCameraChangedEvent(Events::CameraChangedEvent::ChangeType::POSITION);
        }
    }

    virtual void setTarget(const Math::Vector3f& target) {
        if (m_target != target) {
            m_target = target;
            m_viewMatrixDirty = true;
            Logging::Logger::getInstance().debugfc("Camera", "Target changed to (%.3f, %.3f, %.3f)", 
                target.x, target.y, target.z);
            dispatchCameraChangedEvent(Events::CameraChangedEvent::ChangeType::POSITION);
        }
    }

    virtual void setUp(const Math::Vector3f& up) {
        if (m_up != up) {
            m_up = up;
            m_viewMatrixDirty = true;
            dispatchCameraChangedEvent(Events::CameraChangedEvent::ChangeType::ROTATION);
        }
    }

    // Projection settings
    void setFieldOfView(float fov) {
        if (m_fov != fov) {
            m_fov = fov;
            m_projectionMatrixDirty = true;
            dispatchCameraChangedEvent(Events::CameraChangedEvent::ChangeType::ZOOM);
        }
    }

    void setAspectRatio(float aspectRatio) {
        if (m_aspectRatio != aspectRatio) {
            m_aspectRatio = aspectRatio;
            m_projectionMatrixDirty = true;
        }
    }

    void setNearFarPlanes(float nearPlane, float farPlane) {
        if (m_nearPlane != nearPlane || m_farPlane != farPlane) {
            m_nearPlane = nearPlane;
            m_farPlane = farPlane;
            m_projectionMatrixDirty = true;
        }
    }

    // Matrix getters with lazy computation
    const Math::Matrix4f& getViewMatrix() const {
        if (m_viewMatrixDirty) {
            updateViewMatrix();
            m_viewMatrixDirty = false;
        }
        return m_viewMatrix;
    }

    const Math::Matrix4f& getProjectionMatrix() const {
        if (m_projectionMatrixDirty) {
            updateProjectionMatrix();
            m_projectionMatrixDirty = false;
        }
        return m_projectionMatrix;
    }

    Math::Matrix4f getViewProjectionMatrix() const {
        return getProjectionMatrix() * getViewMatrix();
    }

    // Getters
    const Math::Vector3f& getPosition() const { return m_position; }
    const Math::Vector3f& getTarget() const { return m_target; }
    const Math::Vector3f& getUp() const { return m_up; }
    float getFieldOfView() const { return m_fov; }
    float getAspectRatio() const { return m_aspectRatio; }
    float getNearPlane() const { return m_nearPlane; }
    float getFarPlane() const { return m_farPlane; }

    // Direction vectors
    Math::Vector3f getForward() const {
        return (m_target - m_position).normalized();
    }

    Math::Vector3f getRight() const {
        return getForward().cross(m_up).normalized();
    }

    Math::Vector3f getActualUp() const {
        return getRight().cross(getForward()).normalized();
    }

    // View presets
    virtual void setViewPreset(ViewPreset preset) = 0;

    // Event dispatcher
    void setEventDispatcher(Events::EventDispatcher* eventDispatcher) {
        m_eventDispatcher = eventDispatcher;
    }

protected:
    void updateViewMatrix() const {
        m_viewMatrix = Math::Matrix4f::lookAt(m_position, m_target, m_up);
        Logging::Logger::getInstance().debug("View matrix updated", "Camera");
    }

    void updateProjectionMatrix() const {
        m_projectionMatrix = Math::Matrix4f::perspective(Math::toRadians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
        Logging::Logger::getInstance().debugfc("Camera", "Projection matrix updated (FOV: %.1f, Aspect: %.3f, Near: %.3f, Far: %.1f)", 
            m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
    }

    void dispatchCameraChangedEvent(Events::CameraChangedEvent::ChangeType changeType) {
        if (m_eventDispatcher) {
            Events::CameraChangedEvent event(changeType);
            m_eventDispatcher->dispatch(event);
        }
    }

    Events::EventDispatcher* m_eventDispatcher;

    Math::Vector3f m_position;
    Math::Vector3f m_target;
    Math::Vector3f m_up;

    float m_fov;
    float m_nearPlane;
    float m_farPlane;
    float m_aspectRatio;

    mutable Math::Matrix4f m_viewMatrix;
    mutable Math::Matrix4f m_projectionMatrix;
    mutable bool m_viewMatrixDirty;
    mutable bool m_projectionMatrixDirty;
};

}
}