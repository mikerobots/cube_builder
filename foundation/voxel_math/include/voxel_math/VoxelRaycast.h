#pragma once

#include "../../math/Vector3f.h"
#include "../../math/Vector3i.h"
#include "../../math/CoordinateTypes.h"
#include "../../math/Ray.h"
#include "../../../core/voxel_data/VoxelTypes.h"
#include "VoxelBounds.h"
#include <vector>
#include <optional>

namespace VoxelEditor {
namespace VoxelData {
    class VoxelGrid;
}

namespace Math {

/**
 * Optimized ray-voxel intersection calculations.
 * Provides efficient algorithms for ray casting through voxel grids.
 */
class VoxelRaycast {
public:
    /**
     * Result of a raycast operation
     */
    struct RaycastResult {
        bool hit = false;
        float distance = 0.0f;  // Distance along ray in meters
        IncrementCoordinates voxelPos;  // Voxel position in 1cm units
        VoxelData::FaceDirection hitFace = VoxelData::FaceDirection::PosY;
        WorldCoordinates hitPoint;  // Hit point in world coordinates
        Vector3f hitNormal;  // Normalized direction vector
        
        RaycastResult() = default;
        RaycastResult(bool hit, float dist, const IncrementCoordinates& pos, 
                      VoxelData::FaceDirection face, const WorldCoordinates& point, 
                      const Vector3f& normal)
            : hit(hit), distance(dist), voxelPos(pos), hitFace(face), 
              hitPoint(point), hitNormal(normal) {}
    };
    
    /**
     * Perform raycast against a single voxel
     * @param ray Ray in world coordinates (meters)
     * @param voxelPos Voxel position in increment coordinates (1cm units)
     * @param resolution Voxel resolution
     * @return Raycast result
     */
    static RaycastResult raycastVoxel(const Ray& ray,
                                     const IncrementCoordinates& voxelPos,
                                     VoxelData::VoxelResolution resolution);
    
    /**
     * Perform raycast against voxel grid with early termination
     * @param ray Ray in world coordinates (meters)
     * @param grid Voxel grid to cast against
     * @param resolution Resolution to check (or all if not specified)
     * @param maxDistance Maximum distance to cast in meters
     * @return Raycast result for closest hit
     */
    static RaycastResult raycastGrid(const Ray& ray,
                                   const VoxelData::VoxelGrid& grid,
                                   VoxelData::VoxelResolution resolution,
                                   float maxDistance = 1000.0f);
    
    /**
     * Get all voxel positions along a ray path
     * @param ray Ray in world coordinates
     * @param resolution Voxel resolution to traverse
     * @param maxDistance Maximum distance to traverse in meters
     * @return List of voxel positions along ray
     */
    static std::vector<IncrementCoordinates> getVoxelsAlongRay(const Ray& ray,
                                                              VoxelData::VoxelResolution resolution,
                                                              float maxDistance);
    
    /**
     * Check if ray intersects any voxel in grid (fast boolean check)
     * @param ray Ray in world coordinates
     * @param grid Voxel grid to check
     * @param resolution Resolution to check
     * @param maxDistance Maximum distance to check in meters
     * @return True if ray hits any voxel
     */
    static bool rayIntersectsGrid(const Ray& ray,
                                 const VoxelData::VoxelGrid& grid,
                                 VoxelData::VoxelResolution resolution,
                                 float maxDistance = 1000.0f);
    
    /**
     * Cast ray and return all hits in order of distance
     * @param ray Ray in world coordinates
     * @param grid Voxel grid to cast against
     * @param resolution Resolution to check
     * @param maxDistance Maximum distance to cast in meters
     * @param maxHits Maximum number of hits to return (0 = unlimited)
     * @return List of hits sorted by distance
     */
    static std::vector<RaycastResult> raycastAllHits(const Ray& ray,
                                                    const VoxelData::VoxelGrid& grid,
                                                    VoxelData::VoxelResolution resolution,
                                                    float maxDistance = 1000.0f,
                                                    int maxHits = 0);
    
    /**
     * Cast ray against workspace bounds
     * @param ray Ray in world coordinates
     * @param workspaceSize Workspace dimensions in meters
     * @return Ray intersection with workspace bounds, or nullopt if no hit
     */
    static std::optional<RaycastResult> raycastWorkspace(const Ray& ray,
                                                        const Vector3f& workspaceSize);
    
    /**
     * Calculate the exact entry and exit points for a ray through a voxel
     * @param ray Ray in world coordinates
     * @param voxelBounds Bounds of the voxel
     * @return Pair of entry and exit points, or nullopt if no intersection
     */
    static std::optional<std::pair<WorldCoordinates, WorldCoordinates>> 
    calculateRayVoxelIntersection(const Ray& ray, const VoxelBounds& voxelBounds);
    
private:
    /**
     * DDA (Digital Differential Analyzer) traversal state
     */
    struct TraversalState {
        IncrementCoordinates current;  // Current voxel position in 1cm units
        Vector3f tMax;  // Next intersection times in meters
        Vector3f tDelta;  // Step size in meters
        Vector3i step;  // Step direction in increment units
        bool valid = true;  // Whether traversal is still valid
    };
    
    /**
     * Initialize DDA traversal state
     * @param ray Ray to traverse
     * @param resolution Voxel resolution for traversal
     * @param[out] state Traversal state to initialize
     */
    static void initializeTraversal(const Ray& ray, 
                                   VoxelData::VoxelResolution resolution,
                                   TraversalState& state);
    
    /**
     * Step the DDA traversal to next voxel
     * @param[in,out] state Traversal state to update
     */
    static void stepTraversal(TraversalState& state);
    
    /**
     * Calculate which face of a voxel the ray hits
     * @param ray Ray being cast
     * @param voxelBounds Bounds of the voxel
     * @param t Parameter along ray where intersection occurs
     * @return Face direction that was hit
     */
    static VoxelData::FaceDirection calculateHitFace(const Ray& ray,
                                                    const VoxelBounds& voxelBounds,
                                                    float t);
    
    /**
     * Get the current traversal distance
     * @param state Current traversal state
     * @return Distance traveled so far
     */
    static float getCurrentDistance(const TraversalState& state);
    
    /**
     * Check if position is within workspace bounds
     * @param pos Position in world coordinates
     * @param workspaceSize Workspace dimensions
     * @return True if position is within bounds
     */
    static bool isInWorkspace(const WorldCoordinates& pos, const Vector3f& workspaceSize);
    
    /**
     * Convert traversal direction to face direction
     * @param stepDirection Direction of last step
     * @return Face direction that was crossed
     */
    static VoxelData::FaceDirection stepDirectionToFace(const Vector3i& stepDirection);
    
    /**
     * Optimized ray-box intersection test
     * @param ray Ray to test
     * @param boxMin Minimum corner of box
     * @param boxMax Maximum corner of box
     * @param[out] tMin Entry parameter
     * @param[out] tMax Exit parameter
     * @return True if ray intersects box
     */
    static bool rayBoxIntersection(const Ray& ray,
                                  const Vector3f& boxMin,
                                  const Vector3f& boxMax,
                                  float& tMin,
                                  float& tMax);
    
    // Constants for optimization
    static constexpr float EPSILON = 1e-6f;
    static constexpr int MAX_TRAVERSAL_STEPS = 10000;
};

} // namespace Math
} // namespace VoxelEditor