#pragma once

#include <optional>
#include "../../math/Vector3i.h"
#include "../../math/CoordinateTypes.h"
#include "../../../core/voxel_data/VoxelTypes.h"
#include "VoxelBounds.h"
#include "VoxelGrid.h"
#include <vector>
#include <unordered_set>

namespace VoxelEditor {
namespace VoxelData {
    class VoxelGrid;
}

namespace Math {

/**
 * Handles voxel overlap and collision detection.
 * Provides efficient algorithms for checking voxel intersections.
 */
class VoxelCollision {
public:
    /**
     * Information about a voxel for collision queries
     */
    struct VoxelInfo {
        IncrementCoordinates position;
        VoxelData::VoxelResolution resolution;
        VoxelBounds bounds;
        
        VoxelInfo(const IncrementCoordinates& pos, VoxelData::VoxelResolution res)
            : position(pos), resolution(res), bounds(pos, VoxelEditor::Math::VoxelGrid::getVoxelSizeMeters(res)) {}
    };
    
    /**
     * Check if two voxels collide
     * @param pos1 Position of first voxel (bottom-center, 1cm units)
     * @param res1 Resolution of first voxel
     * @param pos2 Position of second voxel (bottom-center, 1cm units)
     * @param res2 Resolution of second voxel
     * @return True if voxels overlap
     */
    static bool checkCollision(const IncrementCoordinates& pos1, VoxelData::VoxelResolution res1,
                              const IncrementCoordinates& pos2, VoxelData::VoxelResolution res2);
    
    /**
     * Check if a voxel collides with any voxel in the grid
     * @param pos Position to check (bottom-center, 1cm units)
     * @param resolution Resolution of voxel to place
     * @param grid Voxel grid to check against
     * @return True if placement would cause collision
     */
    static bool checkCollisionWithGrid(const IncrementCoordinates& pos,
                                      VoxelData::VoxelResolution resolution,
                                      const VoxelData::VoxelGrid& grid);
    
    /**
     * Get all voxels that would collide with a placement
     * @param pos Position to check (bottom-center, 1cm units)
     * @param resolution Resolution of voxel to place
     * @param grid Voxel grid to check against
     * @return List of colliding voxels
     */
    static std::vector<VoxelInfo> getCollidingVoxels(const IncrementCoordinates& pos,
                                                     VoxelData::VoxelResolution resolution,
                                                     const VoxelData::VoxelGrid& grid);
    
    /**
     * Get all voxels within a region
     * @param region Bounding box to search within
     * @param grid Voxel grid to search
     * @return List of voxels in region
     */
    static std::vector<VoxelInfo> getVoxelsInRegion(const VoxelBounds& region,
                                                    const VoxelData::VoxelGrid& grid);
    
    /**
     * Find the nearest non-colliding position for a voxel
     * @param desiredPos Desired position (bottom-center, 1cm units)
     * @param resolution Resolution of voxel to place
     * @param grid Voxel grid to check against
     * @param maxSearchDistance Maximum distance to search (in cm)
     * @return Nearest valid position, or nullopt if none found
     */
    static std::optional<IncrementCoordinates> findNearestFreePosition(
        const IncrementCoordinates& desiredPos,
        VoxelData::VoxelResolution resolution,
        const VoxelData::VoxelGrid& grid,
        int maxSearchDistance = 100);
    
    /**
     * Check if a voxel is completely surrounded by other voxels
     * @param pos Position of voxel (bottom-center, 1cm units)
     * @param resolution Resolution of voxel
     * @param grid Voxel grid to check against
     * @return True if all 6 faces are blocked
     */
    static bool isCompletelySurrounded(const IncrementCoordinates& pos,
                                      VoxelData::VoxelResolution resolution,
                                      const VoxelData::VoxelGrid& grid);
    
    /**
     * Get connected voxels (sharing a face)
     * @param pos Position of voxel (bottom-center, 1cm units)
     * @param resolution Resolution of voxel
     * @param grid Voxel grid to search
     * @return List of adjacent voxels
     */
    static std::vector<VoxelInfo> getConnectedVoxels(const IncrementCoordinates& pos,
                                                     VoxelData::VoxelResolution resolution,
                                                     const VoxelData::VoxelGrid& grid);
    
    /**
     * Calculate the volume of intersection between two voxels
     * @param pos1 Position of first voxel (bottom-center, 1cm units)
     * @param res1 Resolution of first voxel
     * @param pos2 Position of second voxel (bottom-center, 1cm units)
     * @param res2 Resolution of second voxel
     * @return Volume of intersection in cubic meters
     */
    static float calculateIntersectionVolume(const IncrementCoordinates& pos1, 
                                           VoxelData::VoxelResolution res1,
                                           const IncrementCoordinates& pos2, 
                                           VoxelData::VoxelResolution res2);
    
    /**
     * Check if placing a voxel would create a stable structure
     * (has support underneath or is on ground)
     * @param pos Position to check (bottom-center, 1cm units)
     * @param resolution Resolution of voxel to place
     * @param grid Voxel grid to check against
     * @return True if placement would be stable
     */
    static bool checkStability(const IncrementCoordinates& pos,
                              VoxelData::VoxelResolution resolution,
                              const VoxelData::VoxelGrid& grid);
    
private:
    /**
     * Helper to check if two voxel bounds overlap
     * @param bounds1 First voxel bounds
     * @param bounds2 Second voxel bounds
     * @return True if bounds overlap
     */
    static bool boundsOverlap(const VoxelBounds& bounds1, const VoxelBounds& bounds2);
    
    /**
     * Helper to check if a point is inside voxel bounds
     * @param point Point to check (in world coordinates)
     * @param bounds Voxel bounds
     * @return True if point is inside bounds
     */
    static bool pointInBounds(const WorldCoordinates& point, const VoxelBounds& bounds);
    
    /**
     * Get potential collision candidates in a region
     * @param searchBounds Region to search
     * @param grid Voxel grid
     * @return Positions to check for actual collisions
     */
    static std::vector<IncrementCoordinates> getPotentialCollisionCandidates(
        const VoxelBounds& searchBounds,
        const VoxelData::VoxelGrid& grid);
};

} // namespace Math
} // namespace VoxelEditor