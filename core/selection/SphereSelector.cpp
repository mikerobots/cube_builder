#include "SphereSelector.h"
#include "../../foundation/logging/Logger.h"
#include "../../foundation/math/CoordinateConverter.h"
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
    
    // If checking existence, use a more efficient approach
    if (checkExistence && m_voxelManager) {
        // Get all existing voxels of this resolution and filter by sphere
        auto allVoxels = m_voxelManager->getAllVoxels(resolution);
        for (const auto& voxelPos : allVoxels) {
            VoxelId voxel(voxelPos.incrementPos, voxelPos.resolution);
            if (isVoxelInSphere(voxel, center, radius)) {
                result.add(voxel);
            }
        }
        return result;
    }
    
    // Special case: zero radius with includePartial=false should return empty set
    if (radius == 0.0f && !m_includePartial) {
        return result;
    }
    
    float voxelSize = VoxelId(Math::Vector3i::Zero(), resolution).getVoxelSize();
    float radiusSq = radius * radius;
    
    // Calculate bounding box of sphere
    Math::Vector3f radiusVec(radius, radius, radius);
    Math::BoundingBox sphereBounds(center - radiusVec, center + radiusVec);
    
    // Convert world bounds to increment coordinates for iteration
    Math::IncrementCoordinates minIncrement = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(sphereBounds.min)
    );
    Math::IncrementCoordinates maxIncrement = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(sphereBounds.max)
    );
    
    // Get the voxel size in centimeters for stepping
    int voxelSizeCm = static_cast<int>(voxelSize * Math::CoordinateConverter::METERS_TO_CM);
    
    // Use direct coordinates without snapping to allow any 1cm position
    Math::Vector3i minAligned = minIncrement.value();
    Math::Vector3i maxAligned = maxIncrement.value();
    
    // With the new requirements, voxels can be placed at any 1cm position
    // We need to check all positions where a voxel might overlap the sphere
    // Since a voxel extends voxelSize from its origin, we need to expand our search range
    
    // Expand the search range by voxel size to catch voxels that might overlap the sphere
    int expandedMinX = minAligned.x - voxelSizeCm;
    int expandedMinY = minAligned.y - voxelSizeCm;  
    int expandedMinZ = minAligned.z - voxelSizeCm;
    
    // PERFORMANCE FIX: For very large spheres, limit the iteration range to prevent hanging
    // Calculate the volume of positions we'd check to avoid excessive iteration
    long rangeX = maxAligned.x - expandedMinX + 1;
    long rangeY = maxAligned.y - expandedMinY + 1;
    long rangeZ = maxAligned.z - expandedMinZ + 1;
    long totalIterations = rangeX * rangeY * rangeZ;
    
    const long MAX_ITERATIONS = 1000000; // 1M iterations max to prevent hanging
    
    if (totalIterations > MAX_ITERATIONS) {
        // For very large spheres, use a more efficient approach:
        // Instead of checking every 1cm position, sample at a coarser resolution
        // and then check if actual voxels exist in those regions
        int stepSize = static_cast<int>(std::cbrt(totalIterations / MAX_ITERATIONS)) + 1;
        
        for (int x = expandedMinX; x <= maxAligned.x; x += stepSize) {
            for (int y = expandedMinY; y <= maxAligned.y; y += stepSize) {
                for (int z = expandedMinZ; z <= maxAligned.z; z += stepSize) {
                    // Create voxel at this increment position
                    VoxelId voxel(Math::IncrementCoordinates(Math::Vector3i(x, y, z)), resolution);
                    
                    if (isVoxelInSphere(voxel, center, radius)) {
                        if (!checkExistence || voxelExists(voxel)) {
                            result.add(voxel);
                        }
                    }
                }
            }
        }
    } else {
        // For smaller spheres, check all 1cm positions as before
        for (int x = expandedMinX; x <= maxAligned.x; x += 1) {
            for (int y = expandedMinY; y <= maxAligned.y; y += 1) {
                for (int z = expandedMinZ; z <= maxAligned.z; z += 1) {
                    // Create voxel at this increment position
                    VoxelId voxel(Math::IncrementCoordinates(Math::Vector3i(x, y, z)), resolution);
                    
                    if (isVoxelInSphere(voxel, center, radius)) {
                        if (!checkExistence || voxelExists(voxel)) {
                            result.add(voxel);
                        }
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
        // Use centered coordinate system: X,Z centered [-size/2, size/2], Y from [0, size]
        workspaceBounds = Math::BoundingBox(
            Math::Vector3f(-workspaceSize.x/2, 0.0f, -workspaceSize.z/2),
            Math::Vector3f(workspaceSize.x/2, workspaceSize.y, workspaceSize.z/2)
        );
    } else {
        // Default workspace bounds if no manager (5m workspace)
        workspaceBounds = Math::BoundingBox(
            Math::Vector3f(-2.5f, 0.0f, -2.5f),
            Math::Vector3f(2.5f, 5.0f, 2.5f)
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
    
    // Convert world bounds to increment coordinates for iteration
    Math::IncrementCoordinates minIncrement = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(ellipsoidBounds.min)
    );
    Math::IncrementCoordinates maxIncrement = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(ellipsoidBounds.max)
    );
    
    // Get the voxel size in centimeters for expanding search
    int voxelSizeCm = static_cast<int>(voxelSize * Math::CoordinateConverter::METERS_TO_CM);
    
    // Use direct coordinates without snapping to allow any 1cm position
    Math::Vector3i minAligned = minIncrement.value();
    Math::Vector3i maxAligned = maxIncrement.value();
    
    // Expand the search range by voxel size to catch voxels that might overlap
    int expandedMinX = minAligned.x - voxelSizeCm;
    int expandedMinY = minAligned.y - voxelSizeCm;
    int expandedMinZ = minAligned.z - voxelSizeCm;
    
    // PERFORMANCE FIX: For very large ellipsoids, limit iteration to prevent hanging
    long rangeX = maxAligned.x - expandedMinX + 1;
    long rangeY = maxAligned.y - expandedMinY + 1;
    long rangeZ = maxAligned.z - expandedMinZ + 1;
    long totalIterations = rangeX * rangeY * rangeZ;
    
    const long MAX_ITERATIONS = 1000000; // 1M iterations max
    
    if (totalIterations > MAX_ITERATIONS) {
        int stepSize = static_cast<int>(std::cbrt(totalIterations / MAX_ITERATIONS)) + 1;
        
        for (int x = expandedMinX; x <= maxAligned.x; x += stepSize) {
            for (int y = expandedMinY; y <= maxAligned.y; y += stepSize) {
                for (int z = expandedMinZ; z <= maxAligned.z; z += stepSize) {
                    VoxelId voxel(Math::IncrementCoordinates(Math::Vector3i(x, y, z)), resolution);
                    
                    if (isVoxelInEllipsoid(voxel, center, radii, rotation)) {
                        if (!checkExistence || voxelExists(voxel)) {
                            result.add(voxel);
                        }
                    }
                }
            }
        }
    } else {
        // Check each possible voxel position in 1cm increments
        for (int x = expandedMinX; x <= maxAligned.x; x += 1) {
            for (int y = expandedMinY; y <= maxAligned.y; y += 1) {
                for (int z = expandedMinZ; z <= maxAligned.z; z += 1) {
                    VoxelId voxel(Math::IncrementCoordinates(Math::Vector3i(x, y, z)), resolution);
                    
                    if (isVoxelInEllipsoid(voxel, center, radii, rotation)) {
                        if (!checkExistence || voxelExists(voxel)) {
                            result.add(voxel);
                        }
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
    
    // Convert world bounds to increment coordinates for iteration
    Math::IncrementCoordinates minIncrement = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(hemisphereBounds.min)
    );
    Math::IncrementCoordinates maxIncrement = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(hemisphereBounds.max)
    );
    
    // Get the voxel size in centimeters for expanding search
    int voxelSizeCm = static_cast<int>(voxelSize * Math::CoordinateConverter::METERS_TO_CM);
    
    // Use direct coordinates without snapping to allow any 1cm position
    Math::Vector3i minAligned = minIncrement.value();
    Math::Vector3i maxAligned = maxIncrement.value();
    
    // Expand the search range by voxel size to catch voxels that might overlap
    int expandedMinX = minAligned.x - voxelSizeCm;
    int expandedMinY = minAligned.y - voxelSizeCm;
    int expandedMinZ = minAligned.z - voxelSizeCm;
    
    // PERFORMANCE FIX: For very large hemispheres, limit iteration to prevent hanging
    long rangeX = maxAligned.x - expandedMinX + 1;
    long rangeY = maxAligned.y - expandedMinY + 1;
    long rangeZ = maxAligned.z - expandedMinZ + 1;
    long totalIterations = rangeX * rangeY * rangeZ;
    
    const long MAX_ITERATIONS = 1000000; // 1M iterations max
    
    if (totalIterations > MAX_ITERATIONS) {
        int stepSize = static_cast<int>(std::cbrt(totalIterations / MAX_ITERATIONS)) + 1;
        
        for (int x = expandedMinX; x <= maxAligned.x; x += stepSize) {
            for (int y = expandedMinY; y <= maxAligned.y; y += stepSize) {
                for (int z = expandedMinZ; z <= maxAligned.z; z += stepSize) {
                    VoxelId voxel(Math::IncrementCoordinates(Math::Vector3i(x, y, z)), resolution);
                    
                    if (isVoxelInHemisphere(voxel, center, radius, normalizedNormal)) {
                        if (!checkExistence || voxelExists(voxel)) {
                            result.add(voxel);
                        }
                    }
                }
            }
        }
    } else {
        // Check each possible voxel position in 1cm increments
        for (int x = expandedMinX; x <= maxAligned.x; x += 1) {
            for (int y = expandedMinY; y <= maxAligned.y; y += 1) {
                for (int z = expandedMinZ; z <= maxAligned.z; z += 1) {
                    VoxelId voxel(Math::IncrementCoordinates(Math::Vector3i(x, y, z)), resolution);
                    
                    if (isVoxelInHemisphere(voxel, center, radius, normalizedNormal)) {
                        if (!checkExistence || voxelExists(voxel)) {
                            result.add(voxel);
                        }
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
    if (m_includePartial) {
        // Check if voxel bounds intersect ellipsoid (approximation using sphere)
        Math::BoundingBox voxelBounds = voxel.getBounds();
        Math::Vector3f closestPoint = voxelBounds.closestPoint(center);
        
        // Transform closest point to ellipsoid space
        Math::Vector3f localPos = rotation.conjugate().rotate(closestPoint - center);
        
        // Check ellipsoid equation
        float value = (localPos.x * localPos.x) / (radii.x * radii.x) +
                      (localPos.y * localPos.y) / (radii.y * radii.y) +
                      (localPos.z * localPos.z) / (radii.z * radii.z);
        
        return value <= 1.0f;
    } else {
        // Check if voxel center is within ellipsoid
        Math::Vector3f voxelCenter = voxel.getWorldPosition();
        
        // Transform voxel position to ellipsoid space
        Math::Vector3f localPos = rotation.conjugate().rotate(voxelCenter - center);
        
        // Check ellipsoid equation: (x/a)^2 + (y/b)^2 + (z/c)^2 <= 1
        float value = (localPos.x * localPos.x) / (radii.x * radii.x) +
                      (localPos.y * localPos.y) / (radii.y * radii.y) +
                      (localPos.z * localPos.z) / (radii.z * radii.z);
        
        return value <= 1.0f;
    }
}

bool SphereSelector::isVoxelInHemisphere(const VoxelId& voxel, const Math::Vector3f& center, 
                                        float radius, const Math::Vector3f& normal) const {
    if (m_includePartial) {
        // Check if voxel bounds intersect hemisphere
        Math::BoundingBox voxelBounds = voxel.getBounds();
        Math::Vector3f closestPoint = voxelBounds.closestPoint(center);
        
        // Check if closest point is in sphere
        Math::Vector3f toClosest = closestPoint - center;
        if (toClosest.lengthSquared() > radius * radius) {
            return false;
        }
        
        // For partial inclusion, check if any part of voxel is on correct side of plane
        // Check all 8 corners of the voxel
        auto corners = voxelBounds.getCorners();
        
        for (const auto& corner : corners) {
            Math::Vector3f toCorner = corner - center;
            if (toCorner.lengthSquared() <= radius * radius && toCorner.dot(normal) >= 0) {
                return true;
            }
        }
        
        return false;
    } else {
        // Check if voxel center is within hemisphere
        Math::Vector3f voxelCenter = voxel.getWorldPosition();
        
        // Check if in sphere
        Math::Vector3f toVoxel = voxelCenter - center;
        if (toVoxel.lengthSquared() > radius * radius) {
            return false;
        }
        
        // Check if on correct side of plane
        return toVoxel.dot(normal) >= 0;
    }
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
    
    // VoxelId.position is already IncrementCoordinates, so no conversion needed
    return m_voxelManager->hasVoxel(voxel.position, voxel.resolution);
}

}
}