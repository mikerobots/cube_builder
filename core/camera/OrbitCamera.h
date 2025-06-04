#pragma once

#include <algorithm>

#include "Camera.h"
#include "../../foundation/math/MathUtils.h"
#include "../../foundation/logging/Logger.h"

namespace VoxelEditor {
namespace Camera {

class OrbitCamera : public Camera {
public:
    OrbitCamera(Events::EventDispatcher* eventDispatcher = nullptr)
        : Camera(eventDispatcher)
        , m_distance(5.0f)
        , m_yaw(0.0f)
        , m_pitch(0.0f)
        , m_minDistance(0.5f)
        , m_maxDistance(100.0f)
        , m_minPitch(-90.0f)
        , m_maxPitch(90.0f)
        , m_panSensitivity(1.0f)
        , m_rotateSensitivity(1.0f)
        , m_zoomSensitivity(1.0f)
        , m_smoothing(false)  // Default to no smoothing for immediate response
        , m_smoothFactor(0.1f)
        , m_targetDistance(5.0f)
        , m_targetYaw(0.0f)
        , m_targetPitch(0.0f)
        , m_targetTarget(0.0f, 0.0f, 0.0f) {
        updateCameraPosition();
    }

    // Orbit controls
    void orbit(float deltaYaw, float deltaPitch) {
        float newYaw = m_yaw + deltaYaw * m_rotateSensitivity;
        float newPitch = m_pitch + deltaPitch * m_rotateSensitivity;
        
        // Clamp pitch to prevent gimbal lock
        newPitch = Math::clamp(newPitch, m_minPitch, m_maxPitch);
        
        Logging::Logger::getInstance().debugfc("OrbitCamera", "Orbit: yaw=%.1f°, pitch=%.1f°, distance=%.3f", 
            newYaw, newPitch, m_distance);
        
        if (m_smoothing) {
            m_targetYaw = newYaw;
            m_targetPitch = newPitch;
        } else {
            m_yaw = newYaw;
            m_pitch = newPitch;
            updateCameraPosition();
        }
    }

    void zoom(float delta) {
        float newDistance = m_distance - delta * m_zoomSensitivity;
        newDistance = Math::clamp(newDistance, m_minDistance, m_maxDistance);
        
        Logging::Logger::getInstance().debugfc("OrbitCamera", "Zoom: distance %.3f -> %.3f", m_distance, newDistance);
        
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
        
        Math::Vector3f panOffset = (right * delta.x + up * delta.y) * m_panSensitivity;
        
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

    void setYaw(float yaw) {
        if (m_yaw != yaw) {
            m_yaw = yaw;
            updateCameraPosition();
        }
    }

    void setPitch(float pitch) {
        pitch = Math::clamp(pitch, m_minPitch, m_maxPitch);
        if (m_pitch != pitch) {
            m_pitch = pitch;
            updateCameraPosition();
        }
    }

    void setOrbitAngles(float yaw, float pitch) {
        pitch = Math::clamp(pitch, m_minPitch, m_maxPitch);
        if (m_yaw != yaw || m_pitch != pitch) {
            m_yaw = yaw;
            m_pitch = pitch;
            updateCameraPosition();
        }
    }

    // Constraint settings
    void setDistanceConstraints(float minDistance, float maxDistance) {
        m_minDistance = minDistance;
        m_maxDistance = maxDistance;
        m_distance = Math::clamp(m_distance, m_minDistance, m_maxDistance);
    }

    void setPitchConstraints(float minPitch, float maxPitch) {
        m_minPitch = minPitch;
        m_maxPitch = maxPitch;
        m_pitch = Math::clamp(m_pitch, m_minPitch, m_maxPitch);
    }

    // Sensitivity settings
    void setPanSensitivity(float sensitivity) { m_panSensitivity = sensitivity; }
    void setRotateSensitivity(float sensitivity) { m_rotateSensitivity = sensitivity; }
    void setZoomSensitivity(float sensitivity) { m_zoomSensitivity = sensitivity; }

    // Smoothing
    void setSmoothing(bool enabled) { m_smoothing = enabled; }
    void setSmoothFactor(float factor) { m_smoothFactor = Math::clamp(factor, 0.01f, 1.0f); }

    void update(float deltaTime) {
        if (m_smoothing) {
            updateSmoothing(deltaTime);
        }
    }

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
            
            Logging::Logger::getInstance().debugfc("OrbitCamera", "View preset: %s (yaw=%.1f°, pitch=%.1f°, distance=%.1f)", 
                presetNames[index], data.yaw, data.pitch, data.distance);
            
            if (m_smoothing) {
                m_targetYaw = data.yaw;
                m_targetPitch = data.pitch;
                m_targetDistance = data.distance;
            } else {
                setOrbitAngles(data.yaw, data.pitch);
                setDistance(data.distance);
            }
            
            dispatchCameraChangedEvent(Events::CameraChangedEvent::ChangeType::VIEW_PRESET);
        }
    }

    // Focus on a point
    void focusOn(const Math::Vector3f& point, float optimalDistance = -1.0f) {
        if (optimalDistance > 0.0f) {
            optimalDistance = Math::clamp(optimalDistance, m_minDistance, m_maxDistance);
        } else {
            optimalDistance = m_distance;
        }
        
        if (m_smoothing) {
            m_targetTarget = point;
            m_targetDistance = optimalDistance;
        } else {
            setTarget(point);
            setDistance(optimalDistance);
        }
    }

    // Frame a bounding box
    void frameBox(const Math::Vector3f& minBounds, const Math::Vector3f& maxBounds) {
        Math::Vector3f center = (minBounds + maxBounds) * 0.5f;
        Math::Vector3f size = maxBounds - minBounds;
        float maxDimension = std::max({size.x, size.y, size.z});
        
        // Calculate distance to frame the entire box
        float distance = maxDimension / (2.0f * std::tan(Math::toRadians(m_fov) * 0.5f));
        distance = Math::clamp(distance * 1.2f, m_minDistance, m_maxDistance); // Add 20% padding
        
        focusOn(center, distance);
    }

    // Getters
    float getDistance() const { return m_distance; }
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }
    
    float getMinDistance() const { return m_minDistance; }
    float getMaxDistance() const { return m_maxDistance; }
    float getMinPitch() const { return m_minPitch; }
    float getMaxPitch() const { return m_maxPitch; }
    
    float getPanSensitivity() const { return m_panSensitivity; }
    float getRotateSensitivity() const { return m_rotateSensitivity; }
    float getZoomSensitivity() const { return m_zoomSensitivity; }
    
    bool isSmoothing() const { return m_smoothing; }
    float getSmoothFactor() const { return m_smoothFactor; }

    // Override setTarget to update internal state
    void setTarget(const Math::Vector3f& target) override {
        Camera::setTarget(target);
        updateCameraPosition();
    }

private:
    void updateCameraPosition() {
        float yawRad = Math::toRadians(m_yaw);
        float pitchRad = Math::toRadians(m_pitch);
        
        float cosYaw = std::cos(yawRad);
        float sinYaw = std::sin(yawRad);
        float cosPitch = std::cos(pitchRad);
        float sinPitch = std::sin(pitchRad);
        
        Math::Vector3f offset(
            sinYaw * cosPitch,
            sinPitch,
            cosYaw * cosPitch
        );
        
        Math::Vector3f newPosition = m_target + offset * m_distance;
        Camera::setPosition(newPosition);
    }

    void updateSmoothing(float deltaTime) {
        bool changed = false;
        float lerpFactor = 1.0f - std::pow(1.0f - m_smoothFactor, deltaTime * 60.0f); // 60fps reference
        
        // Smooth distance
        if (std::abs(m_distance - m_targetDistance) > 0.001f) {
            m_distance = Math::lerp(m_distance, m_targetDistance, lerpFactor);
            changed = true;
        }
        
        // Smooth angles
        if (std::abs(m_yaw - m_targetYaw) > 0.01f) {
            m_yaw = Math::lerp(m_yaw, m_targetYaw, lerpFactor);
            changed = true;
        }
        
        if (std::abs(m_pitch - m_targetPitch) > 0.01f) {
            m_pitch = Math::lerp(m_pitch, m_targetPitch, lerpFactor);
            changed = true;
        }
        
        // Smooth target position
        Math::Vector3f targetDelta = m_targetTarget - m_target;
        if (targetDelta.length() > 0.001f) {
            Math::Vector3f newTarget = m_target + targetDelta * lerpFactor;
            Camera::setTarget(newTarget);
            changed = true;
        }
        
        if (changed) {
            updateCameraPosition();
        }
    }

    float m_distance;
    float m_yaw;
    float m_pitch;
    
    float m_minDistance;
    float m_maxDistance;
    float m_minPitch;
    float m_maxPitch;
    
    float m_panSensitivity;
    float m_rotateSensitivity;
    float m_zoomSensitivity;
    
    bool m_smoothing;
    float m_smoothFactor;
    
    // Smoothing targets
    float m_targetDistance = 0.0f;
    float m_targetYaw = 0.0f;
    float m_targetPitch = 0.0f;
    Math::Vector3f m_targetTarget{0.0f, 0.0f, 0.0f};
};

}
}