#include "../include/voxel_math/VoxelGrid.h"
#include "../../math/CoordinateConverter.h"
#include <cmath>

namespace VoxelEditor {
namespace Math {

// Define static constexpr members
constexpr float VoxelGrid::VOXEL_SIZES_METERS[10];
constexpr int VoxelGrid::VOXEL_SIZES_CM[10];

IncrementCoordinates VoxelGrid::snapToIncrementGrid(const WorldCoordinates& world) {
    // Use the coordinate converter which already implements proper rounding
    return CoordinateConverter::worldToIncrement(world);
}

IncrementCoordinates VoxelGrid::snapToVoxelGrid(const WorldCoordinates& world, 
                                               VoxelData::VoxelResolution resolution) {
    // First convert to increment coordinates
    IncrementCoordinates increment = snapToIncrementGrid(world);
    
    // Then snap to voxel grid
    return snapToVoxelGrid(increment, resolution);
}

IncrementCoordinates VoxelGrid::snapToVoxelGrid(const IncrementCoordinates& increment, 
                                               VoxelData::VoxelResolution resolution) {
    int voxelSizeCm = getVoxelSizeCm(resolution);
    const Vector3i& pos = increment.value();
    
    // Snap X and Z to floor (snap down for positive, towards -infinity for negative)
    int snappedX = (pos.x / voxelSizeCm) * voxelSizeCm;
    int snappedZ = (pos.z / voxelSizeCm) * voxelSizeCm;
    
    // Handle negative X and Z coordinates correctly
    if (pos.x < 0 && pos.x % voxelSizeCm != 0) {
        snappedX -= voxelSizeCm;
    }
    if (pos.z < 0 && pos.z % voxelSizeCm != 0) {
        snappedZ -= voxelSizeCm;
    }
    
    // For Y-axis: Always round UP to ensure voxels stay above ground
    // This is because voxels are positioned with their bottom at the Y coordinate
    int snappedY;
    if (pos.y >= 0) {
        // For positive Y, round up if not already aligned
        snappedY = ((pos.y + voxelSizeCm - 1) / voxelSizeCm) * voxelSizeCm;
    } else {
        // For negative Y, still use floor division
        snappedY = (pos.y / voxelSizeCm) * voxelSizeCm;
        if (pos.y < 0 && pos.y % voxelSizeCm != 0) {
            snappedY -= voxelSizeCm;
        }
    }
    
    return IncrementCoordinates(snappedX, snappedY, snappedZ);
}

bool VoxelGrid::isAlignedToGrid(const IncrementCoordinates& pos, 
                                VoxelData::VoxelResolution resolution) {
    int voxelSizeCm = getVoxelSizeCm(resolution);
    const Vector3i& p = pos.value();
    
    // Check if each component is divisible by the voxel size
    return (p.x % voxelSizeCm == 0) && 
           (p.y % voxelSizeCm == 0) && 
           (p.z % voxelSizeCm == 0);
}

bool VoxelGrid::isOnIncrementGrid(const WorldCoordinates& world) {
    const Vector3f& w = world.value();
    
    // Convert to centimeters and check if the result is close to an integer
    float xCm = w.x * METERS_TO_CM;
    float yCm = w.y * METERS_TO_CM;
    float zCm = w.z * METERS_TO_CM;
    
    const float epsilon = 1e-5f;  // Small tolerance for floating point comparison
    
    return (std::abs(xCm - std::round(xCm)) < epsilon) &&
           (std::abs(yCm - std::round(yCm)) < epsilon) &&
           (std::abs(zCm - std::round(zCm)) < epsilon);
}

float VoxelGrid::getVoxelSizeMeters(VoxelData::VoxelResolution resolution) {
    // Use cached values for performance
    int index = static_cast<int>(resolution);
    if (index >= 0 && index < 10) {
        return VOXEL_SIZES_METERS[index];
    }
    // Fallback (should never happen with valid resolution)
    return 0.01f;
}

int VoxelGrid::getVoxelSizeCm(VoxelData::VoxelResolution resolution) {
    // Use cached values for performance
    int index = static_cast<int>(resolution);
    if (index >= 0 && index < 10) {
        return VOXEL_SIZES_CM[index];
    }
    // Fallback (should never happen with valid resolution)
    return 1;
}

Vector3i VoxelGrid::getFaceDirectionOffset(VoxelData::FaceDirection direction, int voxelSizeCm) {
    switch (direction) {
        case VoxelData::FaceDirection::PosX:
            return Vector3i(voxelSizeCm, 0, 0);
        case VoxelData::FaceDirection::NegX:
            return Vector3i(-voxelSizeCm, 0, 0);
        case VoxelData::FaceDirection::PosY:
            return Vector3i(0, voxelSizeCm, 0);
        case VoxelData::FaceDirection::NegY:
            return Vector3i(0, -voxelSizeCm, 0);
        case VoxelData::FaceDirection::PosZ:
            return Vector3i(0, 0, voxelSizeCm);
        case VoxelData::FaceDirection::NegZ:
            return Vector3i(0, 0, -voxelSizeCm);
        default:
            return Vector3i(0, 0, 0);
    }
}

IncrementCoordinates VoxelGrid::getAdjacentPosition(const IncrementCoordinates& pos, 
                                                   VoxelData::FaceDirection direction, 
                                                   VoxelData::VoxelResolution resolution) {
    int voxelSizeCm = getVoxelSizeCm(resolution);
    Vector3i offset = getFaceDirectionOffset(direction, voxelSizeCm);
    
    const Vector3i& p = pos.value();
    return IncrementCoordinates(p.x + offset.x, p.y + offset.y, p.z + offset.z);
}

void VoxelGrid::getAdjacentPositions(const IncrementCoordinates& pos,
                                    VoxelData::VoxelResolution resolution,
                                    IncrementCoordinates adjacentPositions[6]) {
    int voxelSizeCm = getVoxelSizeCm(resolution);
    const Vector3i& p = pos.value();
    
    // Calculate all 6 adjacent positions efficiently
    adjacentPositions[0] = IncrementCoordinates(p.x + voxelSizeCm, p.y, p.z);  // +X
    adjacentPositions[1] = IncrementCoordinates(p.x - voxelSizeCm, p.y, p.z);  // -X
    adjacentPositions[2] = IncrementCoordinates(p.x, p.y + voxelSizeCm, p.z);  // +Y
    adjacentPositions[3] = IncrementCoordinates(p.x, p.y - voxelSizeCm, p.z);  // -Y
    adjacentPositions[4] = IncrementCoordinates(p.x, p.y, p.z + voxelSizeCm);  // +Z
    adjacentPositions[5] = IncrementCoordinates(p.x, p.y, p.z - voxelSizeCm);  // -Z
}

} // namespace Math
} // namespace VoxelEditor