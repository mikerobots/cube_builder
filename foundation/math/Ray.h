#pragma once

#include "Vector3f.h"
#include "Vector2f.h"
#include "Vector2i.h"
#include "Vector4f.h"

// Forward declaration to avoid circular dependencies
namespace VoxelEditor { namespace Math { class Matrix4f; }}

namespace VoxelEditor {
namespace Math {

class Ray {
public:
    Vector3f origin;
    Vector3f direction;

    Ray() : origin(Vector3f::Zero()), direction(Vector3f::UnitZ()) {}
    
    Ray(const Vector3f& origin, const Vector3f& direction) 
        : origin(origin), direction(direction.normalized()) {}

    Vector3f getPoint(float t) const {
        return origin + direction * t;
    }

    Vector3f closestPoint(const Vector3f& point) const {
        Vector3f toPoint = point - origin;
        float t = toPoint.dot(direction);
        return getPoint(std::max(0.0f, t));
    }

    float distanceToPoint(const Vector3f& point) const {
        Vector3f closest = closestPoint(point);
        return Vector3f::distance(point, closest);
    }

    bool intersectPlane(const Vector3f& planePoint, const Vector3f& planeNormal, float& t) const {
        float denominator = direction.dot(planeNormal);
        
        if (std::abs(denominator) < 1e-6f) {
            return false;
        }
        
        Vector3f toPlane = planePoint - origin;
        t = toPlane.dot(planeNormal) / denominator;
        
        return t >= 0.0f;
    }

    bool intersectSphere(const Vector3f& center, float radius, float& t1, float& t2) const {
        Vector3f toCenter = origin - center;
        
        float a = direction.dot(direction);
        float b = 2.0f * toCenter.dot(direction);
        float c = toCenter.dot(toCenter) - radius * radius;
        
        float discriminant = b * b - 4.0f * a * c;
        
        if (discriminant < 0.0f) {
            return false;
        }
        
        float sqrtDiscriminant = std::sqrt(discriminant);
        t1 = (-b - sqrtDiscriminant) / (2.0f * a);
        t2 = (-b + sqrtDiscriminant) / (2.0f * a);
        
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        
        return t2 >= 0.0f;
    }

    bool intersectTriangle(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, 
                          float& t, float& u, float& v) const {
        Vector3f edge1 = v1 - v0;
        Vector3f edge2 = v2 - v0;
        Vector3f h = direction.cross(edge2);
        float a = edge1.dot(h);
        
        if (std::abs(a) < 1e-6f) {
            return false;
        }
        
        float f = 1.0f / a;
        Vector3f s = origin - v0;
        u = f * s.dot(h);
        
        if (u < 0.0f || u > 1.0f) {
            return false;
        }
        
        Vector3f q = s.cross(edge1);
        v = f * direction.dot(q);
        
        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }
        
        t = f * edge2.dot(q);
        
        return t > 1e-6f;
    }

    Ray transformed(const Matrix4f& transform) const;

    static Ray fromTwoPoints(const Vector3f& start, const Vector3f& end) {
        return Ray(start, (end - start).normalized());
    }

    static Ray screenToWorld(const Vector2f& screenPos, const Vector2i& screenSize, 
                            const Matrix4f& viewMatrix, const Matrix4f& projectionMatrix);

    std::string toString() const {
        return "Ray(origin: " + origin.toString() + ", direction: " + direction.toString() + ")";
    }
};

}
}

// Include Matrix4f after the class declaration to avoid circular dependencies
#include "Matrix4f.h"

namespace VoxelEditor {
namespace Math {

// Inline implementations of methods that depend on Matrix4f

inline Ray Ray::transformed(const Matrix4f& transform) const {
    // Transform origin as a point (w=1)
    Vector3f transformedOrigin = transform * origin;
    
    // Transform direction as a vector (w=0, no translation)
    Vector3f transformedDirection = transform.transformDirection(direction);
    
    return Ray(transformedOrigin, transformedDirection);
}

inline Ray Ray::screenToWorld(const Vector2f& screenPos, const Vector2i& screenSize, 
                             const Matrix4f& viewMatrix, const Matrix4f& projectionMatrix) {
    // Convert screen coordinates to normalized device coordinates [-1, 1]
    float x = (2.0f * screenPos.x) / screenSize.x - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y) / screenSize.y;  // Flip Y for screen space
    
    // Create ray in clip space (homogeneous coordinates)
    Vector4f rayClipNear(x, y, -1.0f, 1.0f);  // Near plane
    Vector4f rayClipFar(x, y, 1.0f, 1.0f);    // Far plane
    
    // Transform to world space
    Matrix4f viewProj = projectionMatrix * viewMatrix;
    Matrix4f invViewProj = viewProj.inverted();
    
    // Transform clip space points to world space
    Vector4f worldNear4 = invViewProj * rayClipNear;
    Vector4f worldFar4 = invViewProj * rayClipFar;
    
    // Perspective divide
    Vector3f rayWorldNear(worldNear4.x / worldNear4.w, 
                          worldNear4.y / worldNear4.w, 
                          worldNear4.z / worldNear4.w);
    Vector3f rayWorldFar(worldFar4.x / worldFar4.w, 
                         worldFar4.y / worldFar4.w, 
                         worldFar4.z / worldFar4.w);
    
    // Create ray from near to far
    Vector3f rayDirection = (rayWorldFar - rayWorldNear).normalized();
    
    return Ray(rayWorldNear, rayDirection);
}

}
}