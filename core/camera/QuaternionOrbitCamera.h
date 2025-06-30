#pragma once

#include <algorithm>

#include "Camera.h"
#include "../../foundation/math/MathUtils.h"
#include "../../foundation/math/Quaternion.h"
#include "../../foundation/logging/Logger.h"

namespace VoxelEditor {
namespace Camera {

class QuaternionOrbitCamera : public Camera {
public:
    QuaternionOrbitCamera(Events::EventDispatcher* eventDispatcher = nullptr)
        : Camera(eventDispatcher)
        , m_distance(5.0f)
        , m_orientation(Math::Quaternion::identity())
        , m_currentPitch(0.0f)
        , m_currentYaw(0.0f)
        , m_minDistance(0.5f)
        , m_maxDistance(100.0f)
        , m_panSensitivity(1.0f)
        , m_rotateSensitivity(1.0f)
        , m_zoomSensitivity(1.0f)
        , m_smoothing(false)
        , m_smoothFactor(0.1f)
        , m_targetDistance(5.0f)
        , m_targetOrientation(Math::Quaternion::identity())
        , m_targetTarget(Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f))) {
        updateCameraPosition();
    }

    // Orbit controls using quaternions - no gimbal lock!
    void orbit(float deltaYaw, float deltaPitch) {
        // Update stored pitch and yaw
        m_currentYaw += deltaYaw * m_rotateSensitivity;
        m_currentPitch += deltaPitch * m_rotateSensitivity;
        
        // Clamp pitch to avoid over-rotation
        m_currentPitch = Math::clamp(m_currentPitch, -90.0f, 90.0f);
        
        // Create rotation quaternions for yaw and pitch
        Math::Quaternion yawRotation = Math::Quaternion::fromAxisAngle(
            Math::Vector3f::UnitY(), 
            deltaYaw * m_rotateSensitivity * Math::degreesToRadians(1.0f)
        );
        
        // Get the current right vector for pitch rotation
        Math::Vector3f right = getRight();
        Math::Quaternion pitchRotation = Math::Quaternion::fromAxisAngle(
            right, 
            deltaPitch * m_rotateSensitivity * Math::degreesToRadians(1.0f)
        );
        
        // Combine rotations (order matters: yaw * pitch * current)
        Math::Quaternion newOrientation = yawRotation * pitchRotation * m_orientation;
        newOrientation.normalize();
        
        if (m_smoothing) {
            m_targetOrientation = newOrientation;
        } else {
            m_orientation = newOrientation;
            updateCameraPosition();
        }
    }

    void zoom(float delta) {
        float newDistance = m_distance - delta * m_zoomSensitivity;
        newDistance = Math::clamp(newDistance, m_minDistance, m_maxDistance);
        
        if (m_smoothing) {
            m_targetDistance = newDistance;
        } else {
            m_distance = newDistance;
            updateCameraPosition();
        }
    }

    void pan(const Math::Vector3f& delta) {
        Math::Vector3f right = getRight();
        Math::Vector3f up = getActualUp();
        
        Math::WorldCoordinates panOffset((right * delta.x + up * delta.y) * m_panSensitivity);
        
        if (m_smoothing) {
            m_targetTarget = m_target + panOffset;
        } else {
            setTarget(m_target + panOffset);
        }
    }

    // Direct positioning
    void setDistance(float distance) {
        distance = Math::clamp(distance, m_minDistance, m_maxDistance);
        if (m_distance != distance) {
            m_distance = distance;
            updateCameraPosition();
        }
    }

    // Set orientation from Euler angles (for compatibility)
    void setYaw(float yaw) {
        setPitchYawRoll(getPitch(), yaw, 0.0f);
    }

    void setPitch(float pitch) {
        setPitchYawRoll(pitch, getYaw(), 0.0f);
    }

    void setPitchYawRoll(float pitch, float yaw, float roll = 0.0f) {
        // Store pitch and yaw for getters
        m_currentPitch = pitch;
        m_currentYaw = yaw;
        
        // Use standard Euler angle to quaternion conversion
        m_orientation = Math::Quaternion::fromEulerAngles(
            Math::degreesToRadians(pitch),
            Math::degreesToRadians(yaw), 
            Math::degreesToRadians(roll)
        );
        updateCameraPosition();
    }

    // Get Euler angles (stored, not extracted from quaternion to avoid gimbal lock issues)
    float getYaw() const {
        return m_currentYaw;
    }

    float getPitch() const {
        return m_currentPitch;
    }

    float getDistance() const { return m_distance; }

    // Constraints
    void setDistanceConstraints(float minDistance, float maxDistance) {
        m_minDistance = minDistance;
        m_maxDistance = maxDistance;
        m_distance = Math::clamp(m_distance, m_minDistance, m_maxDistance);
    }

    // Smoothing
    void setSmoothingEnabled(bool enabled) { m_smoothing = enabled; }
    bool isSmoothingEnabled() const { return m_smoothing; }
    void setSmoothFactor(float factor) { m_smoothFactor = Math::clamp(factor, 0.01f, 1.0f); }

    // View presets implementation
    void setViewPreset(ViewPreset preset) override {
        struct PresetData {
            float yaw, pitch, distance;
        };
        
        static const PresetData presets[] = {
            {0.0f, 0.0f, 10.0f},      // FRONT
            {180.0f, 0.0f, 10.0f},    // BACK
            {-90.0f, 0.0f, 10.0f},    // LEFT
            {90.0f, 0.0f, 10.0f},     // RIGHT
            {0.0f, 90.0f, 10.0f},     // TOP
            {0.0f, -90.0f, 10.0f},    // BOTTOM
            {45.0f, 35.26f, 12.0f}    // ISOMETRIC (classic 3D view)
        };
        
        static const char* presetNames[] = {
            "FRONT", "BACK", "LEFT", "RIGHT", "TOP", "BOTTOM", "ISOMETRIC"
        };
        
        int index = static_cast<int>(preset);
        if (index >= 0 && index < 7) {
            const PresetData& data = presets[index];
            
            if (m_smoothing) {
                m_targetOrientation = Math::Quaternion::fromEulerAngles(
                    Math::degreesToRadians(data.pitch),
                    Math::degreesToRadians(data.yaw),
                    0.0f
                );
                m_targetDistance = data.distance;
            } else {
                setPitchYawRoll(data.pitch, data.yaw, 0.0f);
                setDistance(data.distance);
            }
            
            dispatchCameraChangedEvent(Events::CameraChangedEvent::ChangeType::VIEW_PRESET);
        }
    }

    // Update
    void update(float deltaTime) {
        if (m_smoothing && deltaTime > 0.0f) {
            // Smooth distance
            m_distance = Math::lerp(m_distance, m_targetDistance, m_smoothFactor);
            
            // Smooth orientation using SLERP
            m_orientation = Math::Quaternion::slerp(m_orientation, m_targetOrientation, m_smoothFactor);
            
            // Smooth target position
            Math::Vector3f currentTarget = m_target.value();
            Math::Vector3f targetTarget = m_targetTarget.value();
            Math::Vector3f smoothedTarget = Math::lerp(currentTarget, targetTarget, m_smoothFactor);
            m_target = Math::WorldCoordinates(smoothedTarget);
            
            updateCameraPosition();
        }
    }

    // Set camera to look at target from a specific direction
    void lookAt(const Math::Vector3f& direction, float distance) {
        m_distance = Math::clamp(distance, m_minDistance, m_maxDistance);
        
        // Create orientation quaternion that looks along the negative direction
        // (camera looks along -Z in its local space)
        Math::Vector3f lookDir = -direction.normalized();
        m_orientation = Math::Quaternion::lookRotation(lookDir, Math::Vector3f::UnitY());
        
        updateCameraPosition();
    }

protected:
    Math::Vector3f getRight() const {
        // Transform the right vector (1, 0, 0) by the orientation
        return m_orientation.rotate(Math::Vector3f::UnitX());
    }

    Math::Vector3f getActualUp() const {
        // Transform the up vector (0, 1, 0) by the orientation
        return m_orientation.rotate(Math::Vector3f::UnitY());
    }

private:
    void updateCameraPosition() {
        // Extract the current Euler angles from the quaternion for positioning
        Math::Vector3f euler = m_orientation.getEulerAngles();
        float pitch = euler.y;  // Y component is pitch
        float yaw = euler.z;    // Z component is yaw
        
        // Use the same positioning formula as OrbitCamera
        float cosPitch = std::cos(pitch);
        float sinPitch = std::sin(pitch);
        float cosYaw = std::cos(yaw);
        float sinYaw = std::sin(yaw);
        
        Math::Vector3f offset(
            sinYaw * cosPitch,  // X
            sinPitch,           // Y  
            cosYaw * cosPitch   // Z
        );
        
        Math::WorldCoordinates newPosition = m_target + Math::WorldCoordinates(offset * m_distance);
        setPosition(newPosition);
        
        // Update view matrix to look at target
        Math::Vector3f eye = m_position.value();
        Math::Vector3f center = m_target.value();
        Math::Vector3f up = getActualUp();
        
        m_viewMatrix = Math::Matrix4f::lookAt(eye, center, up);
        m_viewMatrixDirty = false;
        
        // Dispatch position changed event
        dispatchCameraChangedEvent(Events::CameraChangedEvent::ChangeType::POSITION);
    }

    float m_distance;
    Math::Quaternion m_orientation;  // Replaces yaw/pitch with quaternion
    float m_currentPitch;  // Stored pitch value for getters
    float m_currentYaw;    // Stored yaw value for getters
    
    // Constraints
    float m_minDistance;
    float m_maxDistance;
    
    // Sensitivities
    float m_panSensitivity;
    float m_rotateSensitivity;
    float m_zoomSensitivity;
    
    // Smoothing
    bool m_smoothing;
    float m_smoothFactor;
    float m_targetDistance;
    Math::Quaternion m_targetOrientation;
    Math::WorldCoordinates m_targetTarget;
};

} // namespace Camera
} // namespace VoxelEditor