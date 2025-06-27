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
 * Simplified coordinate conversion system
 * Provides type-safe conversions between World and Increment coordinate systems
 * Both coordinate systems are centered at origin (0,0,0) with Y-up orientation
 */
class CoordinateConverter {
public:
    // Constants
    static constexpr float CM_TO_METERS = 0.01f;
    static constexpr float METERS_TO_CM = 100.0f;

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
     * @param resolution Voxel resolution for rendering
     * @return Voxel size in meters
     */
    static float getVoxelSizeMeters(VoxelEditor::VoxelData::VoxelResolution resolution) {
        return VoxelEditor::VoxelData::getVoxelSize(resolution);
    }

    /**
     * Get workspace bounds in increment coordinates
     * @param workspaceSize Workspace dimensions in meters
     * @return Min and max bounds in increment coordinates (centered)
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
     * @param worldCoord World coordinates to snap
     * @return Snapped world coordinates aligned to 1cm increment grid
     */
    static WorldCoordinates snapToIncrementGrid(const WorldCoordinates& worldCoord) {
        IncrementCoordinates increment = worldToIncrement(worldCoord);
        return incrementToWorld(increment);
    }


    /**
     * Get the center increment coordinate for a voxel at given resolution
     * @param incrementCoord Bottom-center coordinate of the voxel (placement position)
     * @param resolution Voxel resolution
     * @return Center increment coordinate of the voxel
     * 
     * Note: This function is DEPRECATED and potentially confusing.
     * Since increment coordinates already represent the bottom-center position of voxels,
     * calculating the "center" would mean adding half the voxel height in Y.
     * 
     * For voxels smaller than 2cm, the center cannot be exactly represented
     * in integer increment coordinates. In these cases, the function returns the
     * increment coordinate itself.
     * 
     * WARNING: The implementation incorrectly adds half voxel size in all dimensions,
     * which would move the position from bottom-center to somewhere else entirely.
     * This function should not be used for voxel positioning.
     */
    static IncrementCoordinates getVoxelCenterIncrement(
        const IncrementCoordinates& incrementCoord,
        VoxelEditor::VoxelData::VoxelResolution resolution
    ) {
        const Vector3i& increment = incrementCoord.value();
        float voxelSize = getVoxelSizeMeters(resolution);
        float voxelSize_cm = voxelSize * METERS_TO_CM;
        
        // For 1cm voxels, we can't represent the 0.5cm offset in integer coordinates
        // So we just return the position unchanged
        if (resolution == VoxelEditor::VoxelData::VoxelResolution::Size_1cm) {
            return IncrementCoordinates(increment);
        }
        
        // For larger voxels, calculate the center by adding half the voxel size
        int voxelSize_cm_int = static_cast<int>(voxelSize_cm);
        int halfVoxel_cm = voxelSize_cm_int / 2;
        
        // Simply add half voxel size to get center (no snapping)
        Vector3i center(
            increment.x + halfVoxel_cm,
            increment.y + halfVoxel_cm,
            increment.z + halfVoxel_cm
        );
        
        return IncrementCoordinates(center);
    }
};

} // namespace Math
} // namespace VoxelEditor