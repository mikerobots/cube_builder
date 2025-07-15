#pragma once

#include "../../../math/Vector3f.h"
#include "../../../math/Vector3i.h"
#include "../../../math/CoordinateTypes.h"
#include "../../../core/voxel_data/VoxelTypes.h"
#include "VoxelBounds.h"
#include <array>

namespace VoxelEditor {
namespace Math {

// Forward declaration

/**
 * Centralizes all face-related calculations and operations.
 * Provides consistent face direction handling throughout the codebase.
 */
class FaceOperations {
public:
    /**
     * Get the normalized normal vector for a face direction
     * @param direction Face direction
     * @return Normalized direction vector
     */
    static Vector3f getFaceNormal(VoxelData::FaceDirection direction);
    
    /**
     * Get the offset in increment coordinates for a face direction
     * @param direction Face direction
     * @param voxelSizeCm Voxel size in centimeters
     * @return Offset vector in increment units
     */
    static Vector3i getFaceOffset(VoxelData::FaceDirection direction, int voxelSizeCm);
    
    /**
     * Get the opposite face direction
     * @param direction Face direction
     * @return Opposite face direction
     */
    static VoxelData::FaceDirection getOppositeFace(VoxelData::FaceDirection direction);
    
    /**
     * Determine which face of a voxel was hit based on hit point
     * @param hitPoint Hit point in world coordinates
     * @param voxelBounds Bounds of the voxel that was hit
     * @param epsilon Tolerance for face detection (in meters)
     * @return Face direction that was hit
     */
    static VoxelData::FaceDirection determineFaceFromHit(const WorldCoordinates& hitPoint,
                                                        const VoxelBounds& voxelBounds,
                                                        float epsilon = 0.01f);
    
    /**
     * Determine face direction from ray direction (useful for exit faces)
     * @param rayDirection Normalized ray direction vector
     * @return Face direction most aligned with ray
     */
    static VoxelData::FaceDirection determineFaceFromRayDirection(const Vector3f& rayDirection);
    
    /**
     * Calculate placement position for a new voxel on a face
     * @param voxelPos Position of existing voxel in increment coordinates
     * @param face Face to place new voxel on
     * @param resolution Resolution of the new voxel
     * @return Position for new voxel in increment coordinates
     */
    static IncrementCoordinates calculatePlacementPosition(const IncrementCoordinates& voxelPos,
                                                         VoxelData::FaceDirection face,
                                                         VoxelData::VoxelResolution resolution);
    
    // Note: isFaceExposed method commented out due to VoxelGrid namespace conflicts
    // Will be implemented later when VoxelGrid dependencies are resolved
    
    /**
     * Get all face normals for bulk operations
     * @param[out] normals Array of 6 normalized face normals
     */
    static void getAllFaceNormals(Vector3f normals[6]);
    
    /**
     * Get all face offsets for bulk operations
     * @param voxelSizeCm Voxel size in centimeters
     * @param[out] offsets Array of 6 face offsets in increment units
     */
    static void getAllFaceOffsets(int voxelSizeCm, Vector3i offsets[6]);
    
    /**
     * Convert face direction to array index (0-5)
     * @param direction Face direction
     * @return Index 0-5 for array access
     */
    static int faceDirectionToIndex(VoxelData::FaceDirection direction);
    
    /**
     * Convert array index to face direction
     * @param index Index 0-5
     * @return Face direction
     */
    static VoxelData::FaceDirection indexToFaceDirection(int index);
    
    /**
     * Get human-readable name for face direction
     * @param direction Face direction
     * @return String name (e.g., "PositiveX", "NegativeY")
     */
    static const char* getFaceDirectionName(VoxelData::FaceDirection direction);
    
private:
    // Cached face normals for performance
    static const Vector3f FACE_NORMALS[6];
    
    // Face direction names for debugging
    static const char* const FACE_NAMES[6];
};

} // namespace Math
} // namespace VoxelEditor