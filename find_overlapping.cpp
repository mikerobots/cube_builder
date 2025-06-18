#include <iostream>
#include "foundation/math/CoordinateConverter.h"
#include "core/voxel_data/VoxelTypes.h"

using namespace VoxelEditor;

int main() {
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    
    // Start with a simple 4cm voxel at a round position
    Math::Vector3i pos4cm(125, 25, 125); // Y=25 to match 1cm grid positions
    
    Math::GridCoordinates grid4cm(pos4cm);
    Math::WorldCoordinates world4cm = Math::CoordinateConverter::gridToWorld(grid4cm, VoxelData::VoxelResolution::Size_4cm, workspaceSize);
    float size4cm = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_4cm);
    
    std::cout << "4cm voxel at grid (" << pos4cm.x << ", " << pos4cm.y << ", " << pos4cm.z << "):" << std::endl;
    std::cout << "  World center: (" << world4cm.value().x << ", " << world4cm.value().y << ", " << world4cm.value().z << ")" << std::endl;
    std::cout << "  Bounds: (" << (world4cm.value().x - size4cm/2) << ", " << (world4cm.value().y - size4cm/2) << ", " << (world4cm.value().z - size4cm/2) << ")" 
              << " to (" << (world4cm.value().x + size4cm/2) << ", " << (world4cm.value().y + size4cm/2) << ", " << (world4cm.value().z + size4cm/2) << ")" << std::endl;
    
    // Now find 1cm voxel positions that overlap
    // We need 1cm voxels whose centers are within the 4cm voxel bounds
    float minX = world4cm.value().x - size4cm/2;
    float maxX = world4cm.value().x + size4cm/2;
    float minY = world4cm.value().y - size4cm/2;
    float maxY = world4cm.value().y + size4cm/2;
    float minZ = world4cm.value().z - size4cm/2;
    float maxZ = world4cm.value().z + size4cm/2;
    
    std::cout << std::endl << "Looking for 1cm voxels overlapping this region..." << std::endl;
    
    // Try to find 1cm grid positions whose world centers fall within the 4cm bounds
    for (int x = 400; x <= 600; x += 25) {
        for (int y = 25; y <= 25; y++) {  // Keep Y constant
            for (int z = 400; z <= 600; z += 25) {
                Math::Vector3i pos1cm(x, y, z);
                Math::GridCoordinates grid1cm(pos1cm);
                Math::WorldCoordinates world1cm = Math::CoordinateConverter::gridToWorld(grid1cm, VoxelData::VoxelResolution::Size_1cm, workspaceSize);
                
                // Check if the 1cm voxel center is within the 4cm voxel bounds
                if (world1cm.value().x >= minX && world1cm.value().x <= maxX &&
                    world1cm.value().y >= minY && world1cm.value().y <= maxY &&
                    world1cm.value().z >= minZ && world1cm.value().z <= maxZ) {
                    
                    std::cout << "FOUND OVERLAPPING 1cm voxel:" << std::endl;
                    std::cout << "  Grid: (" << pos1cm.x << ", " << pos1cm.y << ", " << pos1cm.z << ")" << std::endl;
                    std::cout << "  World center: (" << world1cm.value().x << ", " << world1cm.value().y << ", " << world1cm.value().z << ")" << std::endl;
                    break;
                }
            }
        }
    }
    
    // Also find a 1cm voxel that should NOT overlap
    Math::Vector3i pos1cm_far(700, 25, 700);
    Math::GridCoordinates grid1cm_far(pos1cm_far);
    Math::WorldCoordinates world1cm_far = Math::CoordinateConverter::gridToWorld(grid1cm_far, VoxelData::VoxelResolution::Size_1cm, workspaceSize);
    
    std::cout << std::endl << "Non-overlapping 1cm voxel:" << std::endl;
    std::cout << "  Grid: (" << pos1cm_far.x << ", " << pos1cm_far.y << ", " << pos1cm_far.z << ")" << std::endl;
    std::cout << "  World center: (" << world1cm_far.value().x << ", " << world1cm_far.value().y << ", " << world1cm_far.value().z << ")" << std::endl;
    
    return 0;
}