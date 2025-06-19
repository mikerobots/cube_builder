#include <iostream>
#include "foundation/math/CoordinateConverter.h"
#include "core/voxel_data/VoxelResolution.h"

int main() {
    using namespace VoxelEditor;
    
    Math::IncrementCoordinates inc(Math::Vector3i(1, 2, 3));
    std::cout << "Original: " << inc.x() << ", " << inc.y() << ", " << inc.z() << std::endl;
    
    Math::IncrementCoordinates snapped = Math::CoordinateConverter::snapToVoxelResolution(inc, VoxelData::VoxelResolution::Size_4cm);
    std::cout << "Snapped to 4cm: " << snapped.x() << ", " << snapped.y() << ", " << snapped.z() << std::endl;
    
    Math::WorldCoordinates world = Math::CoordinateConverter::incrementToWorld(snapped);
    std::cout << "World coords: " << world.x() << ", " << world.y() << ", " << world.z() << std::endl;
    
    float voxelSize = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_4cm);
    std::cout << "4cm voxel size: " << voxelSize << std::endl;
    
    return 0;
}
