#pragma once

#include "Vector3f.h"
#include "Ray.h"
#include <algorithm>
#include <array>
#include <vector>

namespace VoxelEditor {
namespace Math {

class BoundingBox {
public:
    Vector3f min;
    Vector3f max;

    BoundingBox() 
        : min(Vector3f(std::numeric_limits<float>::max()))
        , max(Vector3f(std::numeric_limits<float>::lowest())) {}
    
    BoundingBox(const Vector3f& min, const Vector3f& max) 
        : min(min), max(max) {}
    
    BoundingBox(const Vector3f& center, float size) {
        Vector3f halfSize(size * 0.5f);
        min = center - halfSize;
        max = center + halfSize;
    }

    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    void invalidate() {
        min = Vector3f(std::numeric_limits<float>::max());
        max = Vector3f(std::numeric_limits<float>::lowest());
    }

    Vector3f getCenter() const {
        return (min + max) * 0.5f;
    }

    Vector3f getSize() const {
        return isValid() ? (max - min) : Vector3f::Zero();
    }

    Vector3f getExtents() const {
        return getSize() * 0.5f;
    }
    

    float getVolume() const {
        Vector3f size = getSize();
        return size.x * size.y * size.z;
    }

    float getSurfaceArea() const {
        Vector3f size = getSize();
        return 2.0f * (size.x * size.y + size.y * size.z + size.z * size.x);
    }

    float getDiagonalLength() const {
        return getSize().length();
    }
    
    // Alias for backward compatibility
    float getDiagonal() const {
        return getDiagonalLength();
    }

    void expandToInclude(const Vector3f& point) {
        if (!isValid()) {
            min = max = point;
        } else {
            min = Vector3f::min(min, point);
            max = Vector3f::max(max, point);
        }
    }

    void expandToInclude(const BoundingBox& other) {
        if (!other.isValid()) {
            return;
        }
        
        if (!isValid()) {
            *this = other;
        } else {
            min = Vector3f::min(min, other.min);
            max = Vector3f::max(max, other.max);
        }
    }

    void expand(float amount) {
        Vector3f expansion(amount);
        min -= expansion;
        max += expansion;
    }

    void expand(const Vector3f& amount) {
        min -= amount;
        max += amount;
    }

    bool contains(const Vector3f& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    bool contains(const BoundingBox& other) const {
        return contains(other.min) && contains(other.max);
    }

    bool intersects(const BoundingBox& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    BoundingBox intersection(const BoundingBox& other) const {
        if (!intersects(other)) {
            return BoundingBox();
        }
        
        return BoundingBox(
            Vector3f::max(min, other.min),
            Vector3f::min(max, other.max)
        );
    }

    BoundingBox unionWith(const BoundingBox& other) const {
        BoundingBox result = *this;
        result.expandToInclude(other);
        return result;
    }

    Vector3f getCorner(int index) const {
        return Vector3f(
            (index & 1) ? max.x : min.x,
            (index & 2) ? max.y : min.y,
            (index & 4) ? max.z : min.z
        );
    }

    std::array<Vector3f, 8> getCorners() const {
        return {
            Vector3f(min.x, min.y, min.z), // 0
            Vector3f(max.x, min.y, min.z), // 1
            Vector3f(min.x, max.y, min.z), // 2
            Vector3f(max.x, max.y, min.z), // 3
            Vector3f(min.x, min.y, max.z), // 4
            Vector3f(max.x, min.y, max.z), // 5
            Vector3f(min.x, max.y, max.z), // 6
            Vector3f(max.x, max.y, max.z)  // 7
        };
    }

    Vector3f closestPoint(const Vector3f& point) const {
        return Vector3f::clamp(point, min, max);
    }

    float distanceToPoint(const Vector3f& point) const {
        Vector3f closest = closestPoint(point);
        return Vector3f::distance(point, closest);
    }

    float distanceSquaredToPoint(const Vector3f& point) const {
        Vector3f closest = closestPoint(point);
        return Vector3f::distanceSquared(point, closest);
    }

    bool intersectRay(const Ray& ray, float& tMin, float& tMax) const {
        Vector3f invDir = Vector3f(
            1.0f / ray.direction.x,
            1.0f / ray.direction.y,
            1.0f / ray.direction.z
        );
        
        Vector3f t1 = (min - ray.origin) * invDir;
        Vector3f t2 = (max - ray.origin) * invDir;
        
        Vector3f tMinVec = Vector3f::min(t1, t2);
        Vector3f tMaxVec = Vector3f::max(t1, t2);
        
        tMin = std::max({tMinVec.x, tMinVec.y, tMinVec.z});
        tMax = std::min({tMaxVec.x, tMaxVec.y, tMaxVec.z});
        
        return tMax >= 0.0f && tMin <= tMax;
    }

    bool intersectRay(const Ray& ray) const {
        float tMin, tMax;
        return intersectRay(ray, tMin, tMax);
    }

    BoundingBox transformed(const Matrix4f& transform) const;

    static BoundingBox fromPoints(const std::vector<Vector3f>& points) {
        BoundingBox box;
        for (const Vector3f& point : points) {
            box.expandToInclude(point);
        }
        return box;
    }

    static BoundingBox fromCenterAndSize(const Vector3f& center, const Vector3f& size) {
        Vector3f halfSize = size * 0.5f;
        return BoundingBox(center - halfSize, center + halfSize);
    }

    bool operator==(const BoundingBox& other) const {
        return min == other.min && max == other.max;
    }

    bool operator!=(const BoundingBox& other) const {
        return !(*this == other);
    }

    std::string toString() const {
        return "BoundingBox(min: " + min.toString() + ", max: " + max.toString() + ")";
    }
};

}
}