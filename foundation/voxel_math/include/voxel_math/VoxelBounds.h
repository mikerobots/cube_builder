#pragma once

#include "../../math/Vector3f.h"
#include "../../math/CoordinateTypes.h"
#include "../../math/BoundingBox.h"
#include "../../math/Ray.h"
#include "../../../core/voxel_data/VoxelTypes.h"

namespace VoxelEditor {
namespace Math {

/**
 * Encapsulates all bounding box calculations for voxels positioned at bottom-center.
 * This class ensures consistent and correct calculations throughout the codebase.
 */
class VoxelBounds {
public:
    /**
     * Create bounds from bottom-center position and size
     * @param bottomCenter Bottom-center position in world coordinates (meters)
     * @param voxelSizeMeters Size of the voxel in meters (e.g., 0.32f for 32cm voxel)
     */
    VoxelBounds(const WorldCoordinates& bottomCenter, float voxelSizeMeters);
    
    /**
     * Create bounds from bottom-center position and size
     * @param bottomCenter Bottom-center position in increment coordinates (1cm units)
     * @param voxelSizeMeters Size of the voxel in meters
     */
    VoxelBounds(const IncrementCoordinates& bottomCenter, float voxelSizeMeters);
    
    // Accessors (all return world coordinates)
    WorldCoordinates min() const { return WorldCoordinates(m_min); }
    WorldCoordinates max() const { return WorldCoordinates(m_max); }
    WorldCoordinates center() const { return WorldCoordinates(m_center); }
    WorldCoordinates bottomCenter() const { return WorldCoordinates(m_bottomCenter); }
    float size() const { return m_size; }  // Size in meters
    
    // Utility methods
    BoundingBox toBoundingBox() const;
    bool contains(const WorldCoordinates& point) const;
    bool intersects(const VoxelBounds& other) const;
    bool intersectsRay(const Ray& ray, float& tMin, float& tMax) const;
    
    // Face calculations
    WorldCoordinates getFaceCenter(VoxelData::FaceDirection face) const;
    Vector3f getFaceNormal(VoxelData::FaceDirection face) const;
    // Note: Plane functionality removed as Plane.h not available
    
    // Comparison operators
    bool operator==(const VoxelBounds& other) const;
    bool operator!=(const VoxelBounds& other) const { return !(*this == other); }
    
private:
    void calculateBounds(const Vector3f& bottomCenterPos, float voxelSizeMeters);
    
    Vector3f m_min;  // Internal storage in meters
    Vector3f m_max;  // Internal storage in meters
    Vector3f m_center;  // Internal storage in meters
    Vector3f m_bottomCenter;  // Internal storage in meters
    float m_size;  // Size in meters
};

} // namespace Math
} // namespace VoxelEditor