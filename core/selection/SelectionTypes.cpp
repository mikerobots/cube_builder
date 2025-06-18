#include "SelectionTypes.h"

namespace VoxelEditor {
namespace Selection {

Math::Vector3f VoxelId::getWorldPosition() const {
    // Use the centralized coordinate converter
    // With the new coordinate system, IncrementCoordinates can be directly converted to world coordinates
    Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(position);
    return worldPos.value();
}

float VoxelId::getVoxelSize() const {
    return VoxelData::getVoxelSize(resolution);
}

Math::BoundingBox VoxelId::getBounds() const {
    // Use the centralized coordinate converter for consistent world position calculation
    Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(position);
    float size = getVoxelSize();
    
    // The world position is the corner of the voxel, so calculate bounds from it
    Math::Vector3f corner = worldPos.value();
    Math::Vector3f min = corner;
    Math::Vector3f max = corner + Math::Vector3f(size, size, size);
    return Math::BoundingBox(min, max);
}

}
}