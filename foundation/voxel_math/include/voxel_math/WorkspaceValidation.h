#pragma once

#include <optional>
#include "../../../math/Vector3f.h"
#include "../../../math/CoordinateTypes.h"
#include "../../../core/voxel_data/VoxelTypes.h"

namespace VoxelEditor {
namespace Math {

/**
 * Handles all workspace bounds checking and validation.
 * Ensures voxels are placed within valid workspace limits and above ground plane.
 */
class WorkspaceValidation {
public:
    /**
     * Workspace bounds in different coordinate systems
     */
    struct WorkspaceBounds {
        IncrementCoordinates minIncrement;  // In 1cm units
        IncrementCoordinates maxIncrement;  // In 1cm units
        WorldCoordinates minWorld;  // In meters
        WorldCoordinates maxWorld;  // In meters
        Vector3f size;  // Size in meters
        
        WorkspaceBounds() = default;
        WorkspaceBounds(const Vector3f& workspaceSize);
    };
    
    /**
     * Create workspace bounds from size
     * @param workspaceSize Size of workspace in meters (centered at origin)
     * @return Workspace bounds structure
     */
    static WorkspaceBounds createBounds(const Vector3f& workspaceSize);
    
    /**
     * Check if increment position is within bounds
     * @param pos Position in increment coordinates (1cm units)
     * @param bounds Workspace bounds
     * @return True if position is within bounds
     */
    static bool isInBounds(const IncrementCoordinates& pos, const WorkspaceBounds& bounds);
    
    /**
     * Check if world position is within bounds
     * @param pos Position in world coordinates (meters)
     * @param bounds Workspace bounds
     * @return True if position is within bounds
     */
    static bool isInBounds(const WorldCoordinates& pos, const WorkspaceBounds& bounds);
    
    /**
     * Check if a voxel fits entirely within bounds
     * @param pos Bottom-center position in increment coordinates
     * @param resolution Voxel resolution
     * @param bounds Workspace bounds
     * @return True if entire voxel fits within bounds
     */
    static bool voxelFitsInBounds(const IncrementCoordinates& pos, 
                                  VoxelData::VoxelResolution resolution,
                                  const WorkspaceBounds& bounds);
    
    /**
     * Clamp position to workspace bounds
     * @param pos Position in increment coordinates
     * @param bounds Workspace bounds
     * @return Clamped position
     */
    static IncrementCoordinates clampToBounds(const IncrementCoordinates& pos,
                                             const WorkspaceBounds& bounds);
    
    /**
     * Check if position is above or on ground plane (Y >= 0)
     * @param pos Position in increment coordinates
     * @return True if Y >= 0
     */
    static bool isAboveGroundPlane(const IncrementCoordinates& pos);
    
    /**
     * Clamp position to ground plane
     * @param pos Position in increment coordinates
     * @return Position with Y clamped to >= 0
     */
    static IncrementCoordinates clampToGroundPlane(const IncrementCoordinates& pos);
    
    /**
     * Get the maximum voxel size that fits at a position
     * @param pos Position in increment coordinates
     * @param bounds Workspace bounds
     * @return Maximum voxel resolution that fits, or nullopt if none fit
     */
    static std::optional<VoxelData::VoxelResolution> getMaxFittingVoxelSize(
        const IncrementCoordinates& pos,
        const WorkspaceBounds& bounds);
    
    /**
     * Calculate how much of a voxel extends outside bounds
     * @param pos Bottom-center position in increment coordinates
     * @param resolution Voxel resolution
     * @param bounds Workspace bounds
     * @return Overhang distances for each axis (0 if within bounds)
     */
    struct Overhang {
        int minX = 0;  // How far past min X bound
        int maxX = 0;  // How far past max X bound
        int minY = 0;  // How far below ground
        int maxY = 0;  // How far past max Y bound
        int minZ = 0;  // How far past min Z bound
        int maxZ = 0;  // How far past max Z bound
        
        bool hasOverhang() const {
            return minX > 0 || maxX > 0 || minY > 0 || 
                   maxY > 0 || minZ > 0 || maxZ > 0;
        }
    };
    
    static Overhang calculateOverhang(const IncrementCoordinates& pos,
                                     VoxelData::VoxelResolution resolution,
                                     const WorkspaceBounds& bounds);
    
    /**
     * Find nearest valid position for a voxel
     * @param pos Desired position in increment coordinates
     * @param resolution Voxel resolution
     * @param bounds Workspace bounds
     * @return Nearest valid position where voxel fits entirely
     */
    static IncrementCoordinates findNearestValidPosition(const IncrementCoordinates& pos,
                                                        VoxelData::VoxelResolution resolution,
                                                        const WorkspaceBounds& bounds);
    
    /**
     * Check if workspace size is valid
     * @param size Workspace size in meters
     * @return True if size is within acceptable limits
     */
    static bool isValidWorkspaceSize(const Vector3f& size);
    
    // Workspace size limits
    static constexpr float MIN_WORKSPACE_SIZE = 2.0f;   // 2m minimum
    static constexpr float MAX_WORKSPACE_SIZE = 8.0f;   // 8m maximum
    static constexpr float DEFAULT_WORKSPACE_SIZE = 5.0f; // 5m default
};

} // namespace Math
} // namespace VoxelEditor