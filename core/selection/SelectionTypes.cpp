#include "SelectionTypes.h"

namespace VoxelEditor {
namespace Selection {

Math::Vector3f VoxelId::getWorldPosition() const {
    // Calculate the world position of the voxel center
    // The increment position represents the bottom-center of the voxel
    Math::IncrementCoordinates exact = position;
    
    // Convert to world coordinates (gives us bottom-center position)
    Math::WorldCoordinates bottomCenter = Math::CoordinateConverter::incrementToWorld(exact);
    
    // Add half the voxel size in Y to get the center
    float halfSize = getVoxelSize() * 0.5f;
    Math::Vector3f center = bottomCenter.value() + Math::Vector3f(0, halfSize, 0);
    
    return center;
}

float VoxelId::getVoxelSize() const {
    return VoxelData::getVoxelSize(resolution);
}

Math::BoundingBox VoxelId::getBounds() const {
    // Use exact position without snapping to allow any 1cm position
    Math::IncrementCoordinates exact = position;
    
    // Convert to world coordinates (gives us bottom-center position)
    Math::WorldCoordinates bottomCenter = Math::CoordinateConverter::incrementToWorld(exact);
    float voxelSize = getVoxelSize();
    float halfSize = voxelSize * 0.5f;
    
    // Calculate bounds where bottomCenter is at the bottom face center
    Math::Vector3f min = Math::Vector3f(
        bottomCenter.value().x - halfSize, 
        bottomCenter.value().y,              // Bottom at placement Y
        bottomCenter.value().z - halfSize
    );
    Math::Vector3f max = Math::Vector3f(
        bottomCenter.value().x + halfSize, 
        bottomCenter.value().y + voxelSize,  // Top at placement Y + voxelSize
        bottomCenter.value().z + halfSize
    );
    return Math::BoundingBox(min, max);
}

}
}