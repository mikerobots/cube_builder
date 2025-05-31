#pragma once

#include "../../foundation/math/Vector2i.h"
#include "../../foundation/math/Vector2f.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Ray.h"
#include "../../foundation/math/Matrix4f.h"

namespace VoxelEditor {
namespace Camera {

class Viewport {
public:
    Viewport(int x = 0, int y = 0, int width = 800, int height = 600)
        : m_x(x), m_y(y), m_width(width), m_height(height) {
        updateAspectRatio();
    }

    // Position and size
    void setPosition(int x, int y) {
        m_x = x;
        m_y = y;
    }

    void setSize(int width, int height) {
        if (width > 0 && height > 0) {
            m_width = width;
            m_height = height;
            updateAspectRatio();
        }
    }

    void setBounds(int x, int y, int width, int height) {
        setPosition(x, y);
        setSize(width, height);
    }

    // Getters
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getAspectRatio() const { return m_aspectRatio; }

    Math::Vector2i getPosition() const { return Math::Vector2i(m_x, m_y); }
    Math::Vector2i getSize() const { return Math::Vector2i(m_width, m_height); }

    // Coordinate transformations
    Math::Vector2f screenToNormalized(const Math::Vector2i& screenPos) const {
        return Math::Vector2f(
            (screenPos.x - m_x) / static_cast<float>(m_width) * 2.0f - 1.0f,
            1.0f - (screenPos.y - m_y) / static_cast<float>(m_height) * 2.0f
        );
    }

    Math::Vector2i normalizedToScreen(const Math::Vector2f& normalizedPos) const {
        return Math::Vector2i(
            static_cast<int>((normalizedPos.x + 1.0f) * 0.5f * m_width) + m_x,
            static_cast<int>((1.0f - normalizedPos.y) * 0.5f * m_height) + m_y
        );
    }

    // Check if screen coordinates are within viewport
    bool contains(const Math::Vector2i& screenPos) const {
        return screenPos.x >= m_x && screenPos.x < m_x + m_width &&
               screenPos.y >= m_y && screenPos.y < m_y + m_height;
    }

    bool contains(int x, int y) const {
        return contains(Math::Vector2i(x, y));
    }

    // Ray casting for 3D interaction
    Math::Ray screenToWorldRay(const Math::Vector2i& screenPos, 
                              const Math::Matrix4f& viewMatrix,
                              const Math::Matrix4f& projectionMatrix) const {
        
        // Convert screen coordinates to normalized device coordinates
        Math::Vector2f ndc = screenToNormalized(screenPos);
        
        // Create ray in clip space
        Math::Vector3f rayClipNear(ndc.x, ndc.y, -1.0f);
        Math::Vector3f rayClipFar(ndc.x, ndc.y, 1.0f);
        
        // Transform to world space
        Math::Matrix4f invViewProj = (projectionMatrix * viewMatrix).inverse();
        
        Math::Vector3f rayWorldNear = invViewProj.transformPoint(rayClipNear);
        Math::Vector3f rayWorldFar = invViewProj.transformPoint(rayClipFar);
        
        Math::Vector3f rayDirection = (rayWorldFar - rayWorldNear).normalized();
        
        return Math::Ray(rayWorldNear, rayDirection);
    }

    // Project world position to screen coordinates
    Math::Vector2i worldToScreen(const Math::Vector3f& worldPos,
                                const Math::Matrix4f& viewMatrix,
                                const Math::Matrix4f& projectionMatrix) const {
        
        // Transform to clip space
        Math::Matrix4f viewProj = projectionMatrix * viewMatrix;
        Math::Vector3f clipPos = viewProj.transformPoint(worldPos);
        
        // Perspective divide to get NDC
        if (clipPos.z != 0.0f) {
            clipPos.x /= clipPos.z;
            clipPos.y /= clipPos.z;
        }
        
        // Convert NDC to screen coordinates
        return normalizedToScreen(Math::Vector2f(clipPos.x, clipPos.y));
    }

    // Viewport-relative mouse delta for camera controls
    Math::Vector2f getMouseDelta(const Math::Vector2i& currentPos, const Math::Vector2i& lastPos) const {
        Math::Vector2i delta = currentPos - lastPos;
        return Math::Vector2f(
            delta.x / static_cast<float>(m_width),
            delta.y / static_cast<float>(m_height)
        );
    }

    // Calculate appropriate zoom factor based on viewport size
    float getZoomFactor() const {
        return std::min(m_width, m_height) / 800.0f; // Normalized to 800px reference
    }

private:
    void updateAspectRatio() {
        m_aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
    }

    int m_x;
    int m_y;
    int m_width;
    int m_height;
    float m_aspectRatio;
};

}
}