#pragma once

#include "CoordinateTypes.h"
#include "Vector3f.h"
#include "Vector3i.h"
#include <cmath>
#include <utility>

// Forward declarations for VoxelData types  
namespace VoxelEditor {
    namespace VoxelData {
        enum class VoxelResolution : uint8_t {
            Size_1cm = 0,   // 1cm voxels (0.01m)
            Size_2cm = 1,   // 2cm voxels (0.02m)
            Size_4cm = 2,   // 4cm voxels (0.04m)
            Size_8cm = 3,   // 8cm voxels (0.08m)
            Size_16cm = 4,  // 16cm voxels (0.16m)
            Size_32cm = 5,  // 32cm voxels (0.32m)
            Size_64cm = 6,  // 64cm voxels (0.64m)
            Size_128cm = 7, // 128cm voxels (1.28m)
            Size_256cm = 8, // 256cm voxels (2.56m)
            Size_512cm = 9, // 512cm voxels (5.12m)
            COUNT = 10
        };
        
        // Implementation for coordinate converter usage
        inline constexpr float getVoxelSize(VoxelResolution resolution) {
            constexpr float sizes[10] = {
                0.01f, 0.02f, 0.04f, 0.08f, 0.16f,
                0.32f, 0.64f, 1.28f, 2.56f, 5.12f
            };
            return sizes[static_cast<uint8_t>(resolution)];
        }
    }
}

namespace VoxelEditor {
namespace Math {

/**
 * Coordinate conversion system for the voxel editor
 * 
 * This class provides type-safe conversions between World and Increment coordinate systems.
 * 
 * Coordinate Systems:
 * - World Coordinates: Positions in meters, centered at origin (0,0,0), Y-up orientation
 * - Increment Coordinates: Positions in 1cm increments, centered at origin (0,0,0), Y-up
 * 
 * Key Properties:
 * - Both coordinate systems are centered at the origin
 * - Y=0 represents the ground plane (no voxels below Y=0)
 * - Voxels are placed with their bottom face on the placement position
 * - All positions snap to 1cm increment boundaries
 * 
 * Example usage:
 * \code
 * // Convert from world to increment
 * WorldCoordinates worldPos(1.5f, 0.0f, -2.3f);
 * IncrementCoordinates incPos = CoordinateConverter::worldToIncrement(worldPos);
 * // Result: incPos = (150, 0, -230)
 * 
 * // Convert from increment to world
 * IncrementCoordinates incPos2(100, 50, -75);
 * WorldCoordinates worldPos2 = CoordinateConverter::incrementToWorld(incPos2);
 * // Result: worldPos2 = (1.0f, 0.5f, -0.75f)
 * \endcode
 */
class CoordinateConverter {
public:
    // Constants for unit conversion
    static constexpr float CM_TO_METERS = 0.01f;   ///< Conversion factor: centimeters to meters
    static constexpr float METERS_TO_CM = 100.0f;  ///< Conversion factor: meters to centimeters

    /**
     * Convert world coordinates to increment coordinates (1cm resolution)
     * Both coordinate systems are centered at origin (0,0,0)
     * @param worldCoord World space position (meters, centered)
     * @return Increment coordinates (1cm units, centered)
     */
    static IncrementCoordinates worldToIncrement(const WorldCoordinates& worldCoord) {
        const Vector3f& world = worldCoord.value();
        
        Vector3i increment(
            static_cast<int>(std::round(world.x * METERS_TO_CM)),
            static_cast<int>(std::round(world.y * METERS_TO_CM)),
            static_cast<int>(std::round(world.z * METERS_TO_CM))
        );
        
        return IncrementCoordinates(increment);
    }

    /**
     * Convert increment coordinates to world coordinates
     * Both coordinate systems are centered at origin (0,0,0)
     * @param incrementCoord Increment position (1cm units, centered)
     * @return World coordinates (meters, centered)
     */
    static WorldCoordinates incrementToWorld(const IncrementCoordinates& incrementCoord) {
        const Vector3i& increment = incrementCoord.value();
        
        Vector3f world(
            increment.x * CM_TO_METERS,
            increment.y * CM_TO_METERS,
            increment.z * CM_TO_METERS
        );
        
        return WorldCoordinates(world);
    }

    /**
     * Check if increment coordinates are valid for a given workspace
     * @param incrementCoord Increment coordinates to validate
     * @param workspaceSize Workspace dimensions in meters
     * @return True if coordinates are within workspace bounds
     */
    static bool isValidIncrementCoordinate(
        const IncrementCoordinates& incrementCoord,
        const Vector3f& workspaceSize
    ) {
        const Vector3i& increment = incrementCoord.value();
        
        // Convert workspace bounds to increment coordinates
        int halfX_cm = static_cast<int>(workspaceSize.x * METERS_TO_CM * 0.5f);
        int halfZ_cm = static_cast<int>(workspaceSize.z * METERS_TO_CM * 0.5f);
        int height_cm = static_cast<int>(workspaceSize.y * METERS_TO_CM);
        
        return increment.x >= -halfX_cm && increment.x <= halfX_cm &&
               increment.y >= 0 && increment.y <= height_cm &&
               increment.z >= -halfZ_cm && increment.z <= halfZ_cm;
    }

    /**
     * Check if world coordinates are within workspace bounds
     * @param worldCoord World coordinates to validate
     * @param workspaceSize Workspace dimensions in meters
     * @return True if coordinates are within workspace
     */
    static bool isValidWorldCoordinate(
        const WorldCoordinates& worldCoord,
        const Vector3f& workspaceSize
    ) {
        const Vector3f& world = worldCoord.value();
        float halfX = workspaceSize.x * 0.5f;
        float halfZ = workspaceSize.z * 0.5f;
        
        return world.x >= -halfX && world.x <= halfX &&
               world.y >= 0.0f && world.y <= workspaceSize.y &&
               world.z >= -halfZ && world.z <= halfZ;
    }

    /**
     * Get the voxel size in meters for a given rendering resolution
     * @param resolution Voxel resolution enum value
     * @return Voxel size in meters (e.g., 0.01f for 1cm, 0.32f for 32cm)
     * 
     * Example usage:
     * \code
     * float size = getVoxelSizeMeters(VoxelResolution::Size_32cm); // Returns 0.32f
     * \endcode
     */
    static float getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution resolution) {
        return VoxelEditor::VoxelData::getVoxelSize(resolution);
    }

    /**
     * Get workspace bounds in increment coordinates
     * 
     * Calculates the minimum and maximum bounds of the workspace in increment coordinates.
     * The workspace is centered on the X and Z axes, with Y starting at 0 (ground plane).
     * 
     * @param workspaceSize Workspace dimensions in meters (width, height, depth)
     * @return Pair of (min, max) bounds in increment coordinates
     * 
     * Example:
     * \code
     * Vector3f workspace(5.0f, 3.0f, 5.0f);  // 5m x 3m x 5m
     * auto [min, max] = getWorkspaceBoundsIncrement(workspace);
     * // min = (-250, 0, -250)    // Centered on X/Z, Y starts at 0
     * // max = (250, 300, 250)    // Half-size on X/Z, full height on Y
     * \endcode
     */
    static std::pair<IncrementCoordinates, IncrementCoordinates> getWorkspaceBoundsIncrement(
        const Vector3f& workspaceSize
    ) {
        int halfX_cm = static_cast<int>(workspaceSize.x * METERS_TO_CM * 0.5f);
        int halfZ_cm = static_cast<int>(workspaceSize.z * METERS_TO_CM * 0.5f);
        int height_cm = static_cast<int>(workspaceSize.y * METERS_TO_CM);
        
        IncrementCoordinates minBounds(-halfX_cm, 0, -halfZ_cm);
        IncrementCoordinates maxBounds(halfX_cm, height_cm, halfZ_cm);
        
        return {minBounds, maxBounds};
    }

    /**
     * Snap world coordinates to the nearest increment grid position (1cm)
     * 
     * This function rounds world coordinates to the nearest 1cm boundary.
     * It's useful for ensuring positions align with the voxel placement grid.
     * 
     * @param worldCoord World coordinates to snap (in meters)
     * @return Snapped world coordinates aligned to 1cm increment grid
     * 
     * Rounding behavior:
     * - Values round to nearest 1cm (0.01m)
     * - 0.004m rounds down to 0.00m
     * - 0.006m rounds up to 0.01m
     * - Negative values follow standard rounding rules
     * 
     * Example:
     * \code
     * WorldCoordinates unaligned(1.234f, 2.567f, -0.891f);
     * WorldCoordinates snapped = snapToIncrementGrid(unaligned);
     * // Result: (1.23f, 2.57f, -0.89f)
     * \endcode
     */
    static WorldCoordinates snapToIncrementGrid(const WorldCoordinates& worldCoord) {
        IncrementCoordinates increment = worldToIncrement(worldCoord);
        return incrementToWorld(increment);
    }


    /**
     * Calculate the world-space center position of a voxel
     * @param bottomCenterIncrement The bottom-center increment coordinate of the voxel
     * @param resolution Voxel resolution
     * @return World-space center position of the voxel (in meters)
     * 
     * Note: Voxels are placed with their bottom face on the ground plane.
     * This function calculates the true 3D center by adding half the voxel
     * height in the Y direction.
     * 
     * Example usage:
     * \code
     * IncrementCoordinates voxelPos(0, 0, 0); // Voxel at origin
     * WorldCoordinates center = getVoxelWorldCenter(voxelPos, VoxelResolution::Size_32cm);
     * // center.y will be 0.16f (half of 32cm)
     * \endcode
     */
    static WorldCoordinates getVoxelWorldCenter(
        const IncrementCoordinates& bottomCenterIncrement,
        VoxelEditor::VoxelData::VoxelResolution resolution
    ) {
        // Convert bottom-center to world coordinates
        WorldCoordinates bottomCenterWorld = incrementToWorld(bottomCenterIncrement);
        const Vector3f& bottomCenter = bottomCenterWorld.value();
        
        // Get voxel size in meters
        float voxelSize = getVoxelSizeMeters(resolution);
        float halfVoxelSize = voxelSize * 0.5f;
        
        // Calculate true center by adding half voxel height in Y
        // (voxels are placed with bottom face on ground)
        Vector3f center(
            bottomCenter.x,
            bottomCenter.y + halfVoxelSize,  // Add half height to get center
            bottomCenter.z
        );
        
        return WorldCoordinates(center);
    }
};

} // namespace Math
} // namespace VoxelEditor