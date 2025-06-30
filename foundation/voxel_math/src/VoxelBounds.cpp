#include "../include/voxel_math/VoxelBounds.h"
#include "../../math/CoordinateConverter.h"
#include <cmath>
#include <algorithm>

namespace VoxelEditor {
namespace Math {

VoxelBounds::VoxelBounds(const WorldCoordinates& bottomCenter, float voxelSizeMeters) 
    : m_size(voxelSizeMeters) {
    calculateBounds(bottomCenter.value(), voxelSizeMeters);
}

VoxelBounds::VoxelBounds(const IncrementCoordinates& bottomCenter, float voxelSizeMeters)
    : m_size(voxelSizeMeters) {
    WorldCoordinates worldPos = CoordinateConverter::incrementToWorld(bottomCenter);
    calculateBounds(worldPos.value(), voxelSizeMeters);
}

void VoxelBounds::calculateBounds(const Vector3f& bottomCenterPos, float voxelSizeMeters) {
    m_bottomCenter = bottomCenterPos;
    
    // Calculate half size for X and Z dimensions
    float halfSize = voxelSizeMeters * 0.5f;
    
    // Min corner: bottom-center minus half size in X and Z, Y stays at bottom
    m_min = Vector3f(
        bottomCenterPos.x - halfSize,
        bottomCenterPos.y,  // Y is already at bottom
        bottomCenterPos.z - halfSize
    );
    
    // Max corner: bottom-center plus half size in X and Z, Y goes up by full size
    m_max = Vector3f(
        bottomCenterPos.x + halfSize,
        bottomCenterPos.y + voxelSizeMeters,
        bottomCenterPos.z + halfSize
    );
    
    // Calculate geometric center
    m_center = Vector3f(
        bottomCenterPos.x,
        bottomCenterPos.y + voxelSizeMeters * 0.5f,
        bottomCenterPos.z
    );
}

BoundingBox VoxelBounds::toBoundingBox() const {
    return BoundingBox(m_min, m_max);
}

bool VoxelBounds::contains(const WorldCoordinates& point) const {
    const Vector3f& p = point.value();
    return p.x >= m_min.x && p.x <= m_max.x &&
           p.y >= m_min.y && p.y <= m_max.y &&
           p.z >= m_min.z && p.z <= m_max.z;
}

bool VoxelBounds::intersects(const VoxelBounds& other) const {
    return m_min.x <= other.m_max.x && m_max.x >= other.m_min.x &&
           m_min.y <= other.m_max.y && m_max.y >= other.m_min.y &&
           m_min.z <= other.m_max.z && m_max.z >= other.m_min.z;
}

bool VoxelBounds::intersectsRay(const Ray& ray, float& tMin, float& tMax) const {
    // Use the BoundingBox ray intersection algorithm
    BoundingBox box(m_min, m_max);
    
    // Avoid division by zero by using a small epsilon
    const float epsilon = 1e-6f;
    Vector3f invDir(
        std::abs(ray.direction.x) > epsilon ? 1.0f / ray.direction.x : 1.0f / epsilon,
        std::abs(ray.direction.y) > epsilon ? 1.0f / ray.direction.y : 1.0f / epsilon,
        std::abs(ray.direction.z) > epsilon ? 1.0f / ray.direction.z : 1.0f / epsilon
    );
    
    Vector3f t1 = (m_min - ray.origin) * invDir;
    Vector3f t2 = (m_max - ray.origin) * invDir;
    
    Vector3f tMinVec(std::min(t1.x, t2.x), std::min(t1.y, t2.y), std::min(t1.z, t2.z));
    Vector3f tMaxVec(std::max(t1.x, t2.x), std::max(t1.y, t2.y), std::max(t1.z, t2.z));
    
    tMin = std::max({tMinVec.x, tMinVec.y, tMinVec.z, 0.0f});
    tMax = std::min({tMaxVec.x, tMaxVec.y, tMaxVec.z});
    
    return tMin <= tMax && tMax >= 0.0f;
}

WorldCoordinates VoxelBounds::getFaceCenter(VoxelData::FaceDirection face) const {
    Vector3f faceCenter = m_center;
    float halfSize = m_size * 0.5f;
    
    switch (face) {
        case VoxelData::FaceDirection::PosX:
            faceCenter.x = m_max.x;
            break;
        case VoxelData::FaceDirection::NegX:
            faceCenter.x = m_min.x;
            break;
        case VoxelData::FaceDirection::PosY:
            faceCenter.y = m_max.y;
            break;
        case VoxelData::FaceDirection::NegY:
            faceCenter.y = m_min.y;
            break;
        case VoxelData::FaceDirection::PosZ:
            faceCenter.z = m_max.z;
            break;
        case VoxelData::FaceDirection::NegZ:
            faceCenter.z = m_min.z;
            break;
    }
    
    return WorldCoordinates(faceCenter);
}

Vector3f VoxelBounds::getFaceNormal(VoxelData::FaceDirection face) const {
    switch (face) {
        case VoxelData::FaceDirection::PosX:
            return Vector3f(1.0f, 0.0f, 0.0f);
        case VoxelData::FaceDirection::NegX:
            return Vector3f(-1.0f, 0.0f, 0.0f);
        case VoxelData::FaceDirection::PosY:
            return Vector3f(0.0f, 1.0f, 0.0f);
        case VoxelData::FaceDirection::NegY:
            return Vector3f(0.0f, -1.0f, 0.0f);
        case VoxelData::FaceDirection::PosZ:
            return Vector3f(0.0f, 0.0f, 1.0f);
        case VoxelData::FaceDirection::NegZ:
            return Vector3f(0.0f, 0.0f, -1.0f);
        default:
            return Vector3f(0.0f, 1.0f, 0.0f);  // Default to up
    }
}

// Note: getFacePlane method removed as Plane.h not available

bool VoxelBounds::operator==(const VoxelBounds& other) const {
    const float epsilon = 1e-6f;
    return std::abs(m_size - other.m_size) < epsilon &&
           std::abs(m_bottomCenter.x - other.m_bottomCenter.x) < epsilon &&
           std::abs(m_bottomCenter.y - other.m_bottomCenter.y) < epsilon &&
           std::abs(m_bottomCenter.z - other.m_bottomCenter.z) < epsilon;
}

} // namespace Math
} // namespace VoxelEditor