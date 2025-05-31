#pragma once

#include "Vector3f.h"

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