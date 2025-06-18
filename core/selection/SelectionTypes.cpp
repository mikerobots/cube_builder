#include "SelectionTypes.h"

namespace VoxelEditor {
namespace Selection {

Math::Vector3f VoxelId::getWorldPosition() const {
    // Calculate the world position of the voxel center
    // First, snap to the voxel grid for the given resolution
    Math::IncrementCoordinates snapped = Math::CoordinateConverter::snapToVoxelResolution(position, resolution);
    
    // Convert snapped corner position to world coordinates
    Math::WorldCoordinates cornerWorld = Math::CoordinateConverter::incrementToWorld(snapped);
    
    // Add half the voxel size to get the center
    float halfSize = getVoxelSize() * 0.5f;
    Math::Vector3f center = cornerWorld.value() + Math::Vector3f(halfSize, halfSize, halfSize);
    
    return center;
}

float VoxelId::getVoxelSize() const {
    return VoxelData::getVoxelSize(resolution);
}

Math::BoundingBox VoxelId::getBounds() const {
    // Snap to the voxel grid for the given resolution
    Math::IncrementCoordinates snapped = Math::CoordinateConverter::snapToVoxelResolution(position, resolution);
    
    // Convert snapped position to world coordinates
    Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(snapped);
    float size = getVoxelSize();
    
    // The world position is the corner of the voxel
    Math::Vector3f min = worldPos.value();
    Math::Vector3f max = min + Math::Vector3f(size, size, size);
    return Math::BoundingBox(min, max);
}

}
}