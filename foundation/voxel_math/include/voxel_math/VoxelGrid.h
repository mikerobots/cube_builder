#pragma once

#include "../../math/Vector3f.h"
#include "../../math/Vector3i.h"
#include "../../math/CoordinateTypes.h"
#include "../../../core/voxel_data/VoxelTypes.h"
#include <array>

namespace VoxelEditor {
namespace Math {

/**
 * Handles all grid-related calculations and snapping operations.
 * Provides consistent grid alignment and coordinate conversions.
 */
class VoxelGrid {
public:
    // Constants
    static constexpr float CM_TO_METERS = 0.01f;
    static constexpr float METERS_TO_CM = 100.0f;
    static constexpr int MIN_INCREMENT_CM = 1;
    
    /**
     * Snap world coordinates to the nearest 1cm increment grid position
     * @param world Position in world coordinates (meters)
     * @return Snapped position in increment coordinates (1cm units)
     */
    static IncrementCoordinates snapToIncrementGrid(const WorldCoordinates& world);
    
    /**
     * Snap world coordinates to the voxel grid for a given resolution
     * @param world Position in world coordinates (meters)
     * @param resolution Voxel resolution to snap to
     * @return Snapped position in increment coordinates (1cm units)
     */
    static IncrementCoordinates snapToVoxelGrid(const WorldCoordinates& world, 
                                               VoxelData::VoxelResolution resolution);
    
    /**
     * Snap increment coordinates to the voxel grid for a given resolution
     * @param increment Position in increment coordinates (1cm units)
     * @param resolution Voxel resolution to snap to
     * @return Snapped position in increment coordinates (1cm units)
     */
    static IncrementCoordinates snapToVoxelGrid(const IncrementCoordinates& increment, 
                                               VoxelData::VoxelResolution resolution);
    
    /**
     * Check if an increment position is aligned to the voxel grid
     * @param pos Position in increment coordinates (1cm units)
     * @param resolution Voxel resolution to check against
     * @return True if the position is aligned to the voxel grid
     */
    static bool isAlignedToGrid(const IncrementCoordinates& pos, 
                                VoxelData::VoxelResolution resolution);
    
    /**
     * Check if a world position is on the 1cm increment grid
     * @param world Position in world coordinates (meters)
     * @return True if the position is on the increment grid
     */
    static bool isOnIncrementGrid(const WorldCoordinates& world);
    
    /**
     * Get voxel size in meters for a given resolution
     * @param resolution Voxel resolution
     * @return Size in meters
     */
    static float getVoxelSizeMeters(VoxelData::VoxelResolution resolution);
    
    /**
     * Get voxel size in centimeters for a given resolution
     * @param resolution Voxel resolution
     * @return Size in centimeters (integer)
     */
    static int getVoxelSizeCm(VoxelData::VoxelResolution resolution);
    
    /**
     * Calculate adjacent position in a given face direction
     * @param pos Current position in increment coordinates (1cm units)
     * @param direction Face direction to move towards
     * @param resolution Voxel resolution (determines offset distance)
     * @return Adjacent position in increment coordinates
     */
    static IncrementCoordinates getAdjacentPosition(const IncrementCoordinates& pos, 
                                                   VoxelData::FaceDirection direction, 
                                                   VoxelData::VoxelResolution resolution);
    
    /**
     * Get all 6 adjacent positions for performance-critical operations
     * @param pos Current position in increment coordinates (1cm units)
     * @param resolution Voxel resolution (determines offset distance)
     * @param[out] adjacentPositions Array of 6 adjacent positions
     */
    static void getAdjacentPositions(const IncrementCoordinates& pos,
                                    VoxelData::VoxelResolution resolution,
                                    IncrementCoordinates adjacentPositions[6]);
    
    /**
     * Get the face direction offset in increment coordinates
     * @param direction Face direction
     * @param voxelSizeCm Voxel size in centimeters
     * @return Offset vector in increment coordinates
     */
    static Vector3i getFaceDirectionOffset(VoxelData::FaceDirection direction, int voxelSizeCm);
    
private:
    // Cached voxel sizes for performance (in meters)
    static constexpr float VOXEL_SIZES_METERS[10] = {
        0.01f, 0.02f, 0.04f, 0.08f, 0.16f,
        0.32f, 0.64f, 1.28f, 2.56f, 5.12f
    };
    
    // Cached voxel sizes for performance (in centimeters)
    static constexpr int VOXEL_SIZES_CM[10] = {
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512
    };
};

} // namespace Math
} // namespace VoxelEditor