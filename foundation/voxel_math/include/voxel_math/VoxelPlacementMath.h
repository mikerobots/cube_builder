#pragma once

#include "../../math/CoordinateTypes.h"
#include "../../math/CoordinateConverter.h"
#include "../../../core/voxel_data/VoxelTypes.h"

namespace VoxelEditor {
namespace Math {

/**
 * VoxelPlacementMath provides mathematical algorithms for voxel placement,
 * including snapping, alignment, and validation calculations.
 * 
 * These algorithms handle:
 * - 1cm increment snapping
 * - Grid-aligned placement for voxels
 * - Surface face placement calculations
 * - Bottom-center coordinate system considerations
 */
class VoxelPlacementMath {
public:
    /**
     * Snap a world position to the nearest 1cm increment.
     * This is the most basic snapping - just rounds to nearest cm.
     */
    static IncrementCoordinates snapToValidIncrement(const WorldCoordinates& worldPos);
    
    /**
     * Snap to voxel grid based on resolution.
     * When shiftPressed is true, uses 1cm increments.
     * When false, snaps to voxel-size-aligned positions.
     * 
     * IMPORTANT: This accounts for bottom-center coordinate system.
     * For X/Z axes, it snaps to voxel centers (offset by half voxel size).
     * For Y axis, it snaps to voxel bottoms.
     */
    static IncrementCoordinates snapToGridAligned(const WorldCoordinates& worldPos,
                                                  VoxelData::VoxelResolution resolution,
                                                  bool shiftPressed);
    
    /**
     * Calculate placement position on a surface face with 1cm increment precision.
     * This allows placing smaller voxels at any 1cm position on larger voxel faces.
     * 
     * @param hitPoint The ray hit point on the face
     * @param surfaceFaceVoxelPos Position of the voxel whose face we're placing on
     * @param surfaceFaceVoxelRes Resolution of the surface voxel
     * @param surfaceFaceDir Which face of the surface voxel
     * @param placementResolution Resolution of voxel being placed
     * @param allowOverhang If true, allows smaller voxels to overhang larger ones
     * @param shiftPressed If true, allows 1cm placement even for same-size voxels
     * @return Snapped increment position for placement
     */
    static IncrementCoordinates snapToSurfaceFaceGrid(const WorldCoordinates& hitPoint,
                                                      const IncrementCoordinates& surfaceFaceVoxelPos,
                                                      VoxelData::VoxelResolution surfaceFaceVoxelRes,
                                                      VoxelData::FaceDirection surfaceFaceDir,
                                                      VoxelData::VoxelResolution placementResolution,
                                                      bool allowOverhang = true,
                                                      bool shiftPressed = false);
    
    /**
     * Check if an increment position is valid (Y >= 0).
     */
    static bool isValidIncrementPosition(const IncrementCoordinates& pos);
    
    /**
     * Check if a world position can be converted to valid increments.
     * Validates against NaN, infinity, and extreme values.
     */
    static bool isValidForIncrementPlacement(const WorldCoordinates& worldPos);
    
    /**
     * Calculate the world-space bounds of a voxel given its increment position.
     * Returns min/max corners accounting for bottom-center coordinate system.
     * 
     * @param incrementPos The voxel's position in increment coordinates
     * @param resolution The voxel's resolution
     * @param[out] minCorner Minimum corner of voxel in world space
     * @param[out] maxCorner Maximum corner of voxel in world space
     */
    static void calculateVoxelWorldBounds(const IncrementCoordinates& incrementPos,
                                          VoxelData::VoxelResolution resolution,
                                          Vector3f& minCorner,
                                          Vector3f& maxCorner);
    
    /**
     * Calculate if a placement position is within the bounds of a surface face.
     * Used to validate that placements don't exceed face boundaries.
     * 
     * @param placementPos World position where voxel will be placed
     * @param surfaceVoxelPos Position of surface voxel in increments
     * @param surfaceResolution Resolution of surface voxel
     * @param faceDir Which face we're checking bounds for
     * @param epsilon Tolerance for floating point comparisons
     * @return true if placement is within face bounds
     */
    static bool isWithinFaceBounds(const WorldCoordinates& placementPos,
                                   const IncrementCoordinates& surfaceVoxelPos,
                                   VoxelData::VoxelResolution surfaceResolution,
                                   VoxelData::FaceDirection faceDir,
                                   float epsilon = 0.0001f);
    
private:
    // Maximum reasonable coordinate value to prevent overflow
    static constexpr int MAX_INCREMENT_COORD = 1000000;
};

} // namespace Math
} // namespace VoxelEditor