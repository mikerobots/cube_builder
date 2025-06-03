#include "SphereSelector.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>

namespace VoxelEditor {
namespace Selection {

SphereSelector::SphereSelector(VoxelData::VoxelDataManager* voxelManager)
    : m_voxelManager(voxelManager)
    , m_selectionMode(SelectionMode::Replace)
    , m_includePartial(true)
    , m_useFalloff(false)
    , m_falloffStart(0.8f) {
}

SelectionSet SphereSelector::selectFromSphere(const Math::Vector3f& center,
                                            float radius,
                                            VoxelData::VoxelResolution resolution,
                                            bool checkExistence) {
    SelectionSet result;
    
    float voxelSize = VoxelId(Math::Vector3i::Zero(), resolution).getVoxelSize();
    float radiusSq = radius * radius;
    
    // Calculate bounding box of sphere
    Math::Vector3f radiusVec(radius, radius, radius);
    Math::BoundingBox sphereBounds(center - radiusVec, center + radiusVec);
    
    // Get voxel range
    Math::Vector3i minVoxel(
        static_cast<int>(std::floor(sphereBounds.min.x / voxelSize)),
        static_cast<int>(std::floor(sphereBounds.min.y / voxelSize)),
        static_cast<int>(std::floor(sphereBounds.min.z / voxelSize))
    );
    
    Math::Vector3i maxVoxel(
        static_cast<int>(std::ceil(sphereBounds.max.x / voxelSize)),
        static_cast<int>(std::ceil(sphereBounds.max.y / voxelSize)),
        static_cast<int>(std::ceil(sphereBounds.max.z / voxelSize))
    );
    
    // Check each voxel in range
    for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
        for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
            for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
                VoxelId voxel(Math::Vector3i(x, y, z), resolution);
                
                if (isVoxelInSphere(voxel, center, radius)) {
                    if (!checkExistence || voxelExists(voxel)) {
                        result.add(voxel);
                    }
                }
            }
        }
    }
    
    return result;
}

SelectionSet SphereSelector::selectFromRay(const Math::Ray& ray,
                                         float radius,
                                         float maxDistance,
                                         VoxelData::VoxelResolution resolution) {
    // Get workspace bounds from voxel manager
    Math::BoundingBox workspaceBounds;
    if (m_voxelManager) {
        Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
        workspaceBounds = Math::BoundingBox(
            Math::Vector3f(0.0f, 0.0f, 0.0f),
            workspaceSize
        );
    } else {
        // Default workspace bounds if no manager
        workspaceBounds = Math::BoundingBox(
            Math::Vector3f(-5.0f, -5.0f, -5.0f),
            Math::Vector3f(5.0f, 5.0f, 5.0f)
        );
    }
    
    // Find intersection point with workspace bounds
    float t1, t2;
    Math::Vector3f intersectionPoint;
    
    if (workspaceBounds.intersectRay(ray, t1, t2)) {
        // Use the entry point or maxDistance, whichever is closer
        float t = std::min(t1, maxDistance);
        intersectionPoint = ray.origin + ray.direction * t;
    } else {
        // No intersection with workspace, use a point along the ray
        intersectionPoint = ray.origin + ray.direction * std::min(maxDistance, 10.0f);
    }
    
    return selectFromSphere(intersectionPoint, radius, resolution, true);
}

SelectionSet SphereSelector::selectEllipsoid(const Math::Vector3f& center,
                                           const Math::Vector3f& radii,
                                           const Math::Quaternion& rotation,
                                           VoxelData::VoxelResolution resolution,
                                           bool checkExistence) {
    SelectionSet result;
    
    float voxelSize = VoxelId(Math::Vector3i::Zero(), resolution).getVoxelSize();
    
    // Calculate bounding box of ellipsoid
    float maxRadius = std::max({radii.x, radii.y, radii.z});
    Math::Vector3f radiusVec(maxRadius, maxRadius, maxRadius);
    Math::BoundingBox ellipsoidBounds(center - radiusVec, center + radiusVec);
    
    // Get voxel range
    Math::Vector3i minVoxel(
        static_cast<int>(std::floor(ellipsoidBounds.min.x / voxelSize)),
        static_cast<int>(std::floor(ellipsoidBounds.min.y / voxelSize)),
        static_cast<int>(std::floor(ellipsoidBounds.min.z / voxelSize))
    );
    
    Math::Vector3i maxVoxel(
        static_cast<int>(std::ceil(ellipsoidBounds.max.x / voxelSize)),
        static_cast<int>(std::ceil(ellipsoidBounds.max.y / voxelSize)),
        static_cast<int>(std::ceil(ellipsoidBounds.max.z / voxelSize))
    );
    
    // Check each voxel in range
    for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
        for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
            for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
                VoxelId voxel(Math::Vector3i(x, y, z), resolution);
                
                if (isVoxelInEllipsoid(voxel, center, radii, rotation)) {
                    if (!checkExistence || voxelExists(voxel)) {
                        result.add(voxel);
                    }
                }
            }
        }
    }
    
    return result;
}

SelectionSet SphereSelector::selectHemisphere(const Math::Vector3f& center,
                                            float radius,
                                            const Math::Vector3f& normal,
                                            VoxelData::VoxelResolution resolution,
                                            bool checkExistence) {
    SelectionSet result;
    
    float voxelSize = VoxelId(Math::Vector3i::Zero(), resolution).getVoxelSize();
    Math::Vector3f normalizedNormal = normal.normalized();
    
    // Calculate bounding box of hemisphere
    Math::Vector3f radiusVec(radius, radius, radius);
    Math::BoundingBox hemisphereBounds(center - radiusVec, center + radiusVec);
    
    // Get voxel range
    Math::Vector3i minVoxel(
        static_cast<int>(std::floor(hemisphereBounds.min.x / voxelSize)),
        static_cast<int>(std::floor(hemisphereBounds.min.y / voxelSize)),
        static_cast<int>(std::floor(hemisphereBounds.min.z / voxelSize))
    );
    
    Math::Vector3i maxVoxel(
        static_cast<int>(std::ceil(hemisphereBounds.max.x / voxelSize)),
        static_cast<int>(std::ceil(hemisphereBounds.max.y / voxelSize)),
        static_cast<int>(std::ceil(hemisphereBounds.max.z / voxelSize))
    );
    
    // Check each voxel in range
    for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
        for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
            for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
                VoxelId voxel(Math::Vector3i(x, y, z), resolution);
                
                if (isVoxelInHemisphere(voxel, center, radius, normalizedNormal)) {
                    if (!checkExistence || voxelExists(voxel)) {
                        result.add(voxel);
                    }
                }
            }
        }
    }
    
    return result;
}

bool SphereSelector::isVoxelInSphere(const VoxelId& voxel, const Math::Vector3f& center, float radius) const {
    if (m_includePartial) {
        // Check if voxel bounds intersect sphere
        Math::BoundingBox voxelBounds = voxel.getBounds();
        Math::Vector3f closestPoint = voxelBounds.closestPoint(center);
        float distSq = (closestPoint - center).lengthSquared();
        return distSq <= radius * radius;
    } else {
        // Check if voxel center is within sphere
        Math::Vector3f voxelCenter = voxel.getWorldPosition();
        float distSq = (voxelCenter - center).lengthSquared();
        return distSq <= radius * radius;
    }
}

bool SphereSelector::isVoxelInEllipsoid(const VoxelId& voxel, const Math::Vector3f& center, 
                                       const Math::Vector3f& radii, const Math::Quaternion& rotation) const {
    Math::Vector3f voxelPos = m_includePartial ? voxel.getWorldPosition() : voxel.getWorldPosition();
    
    // Transform voxel position to ellipsoid space
    Math::Vector3f localPos = rotation.conjugate().rotate(voxelPos - center);
    
    // Check ellipsoid equation: (x/a)^2 + (y/b)^2 + (z/c)^2 <= 1
    float value = (localPos.x * localPos.x) / (radii.x * radii.x) +
                  (localPos.y * localPos.y) / (radii.y * radii.y) +
                  (localPos.z * localPos.z) / (radii.z * radii.z);
    
    return value <= 1.0f;
}

bool SphereSelector::isVoxelInHemisphere(const VoxelId& voxel, const Math::Vector3f& center, 
                                        float radius, const Math::Vector3f& normal) const {
    Math::Vector3f voxelPos = m_includePartial ? voxel.getWorldPosition() : voxel.getWorldPosition();
    
    // Check if in sphere
    Math::Vector3f toVoxel = voxelPos - center;
    if (toVoxel.lengthSquared() > radius * radius) {
        return false;
    }
    
    // Check if on correct side of plane
    return toVoxel.dot(normal) >= 0;
}

float SphereSelector::getVoxelWeight(const VoxelId& voxel, const Math::Vector3f& center, float radius) const {
    if (!m_useFalloff) return 1.0f;
    
    Math::Vector3f voxelCenter = voxel.getWorldPosition();
    float distance = (voxelCenter - center).length();
    float normalizedDist = distance / radius;
    
    if (normalizedDist <= m_falloffStart) {
        return 1.0f;
    } else if (normalizedDist >= 1.0f) {
        return 0.0f;
    } else {
        // Linear falloff
        float t = (normalizedDist - m_falloffStart) / (1.0f - m_falloffStart);
        return 1.0f - t;
    }
}

bool SphereSelector::voxelExists(const VoxelId& voxel) const {
    if (!m_voxelManager) return true; // For testing: assume all voxels exist when no manager
    
    return m_voxelManager->hasVoxel(voxel.position, voxel.resolution);
}

}
}