#include <iostream>
#include "foundation/math/CoordinateConverter.h"
#include "core/voxel_data/VoxelTypes.h"

using namespace VoxelEditor;

int main() {
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    
    std::cout << "=== Systematic Coordinate Test ===" << std::endl;
    
    // Start with a known world position and work backwards to find grid positions
    Math::Vector3f targetWorld(0.0f, 0.5f, 0.0f);  // Near center of workspace
    
    std::cout << "Target world position: (" << targetWorld.x << ", " << targetWorld.y << ", " << targetWorld.z << ")" << std::endl;
    
    // Find grid positions for both resolutions at this world position
    Math::WorldCoordinates worldCoord(targetWorld);
    
    Math::GridCoordinates grid4cm = Math::CoordinateConverter::worldToGrid(worldCoord, VoxelData::VoxelResolution::Size_4cm, workspaceSize);
    Math::GridCoordinates grid1cm = Math::CoordinateConverter::worldToGrid(worldCoord, VoxelData::VoxelResolution::Size_1cm, workspaceSize);
    
    std::cout << "Grid position for 4cm voxel: (" << grid4cm.value().x << ", " << grid4cm.value().y << ", " << grid4cm.value().z << ")" << std::endl;
    std::cout << "Grid position for 1cm voxel: (" << grid1cm.value().x << ", " << grid1cm.value().y << ", " << grid1cm.value().z << ")" << std::endl;
    
    // Convert back to world to verify
    Math::WorldCoordinates world4cm_back = Math::CoordinateConverter::gridToWorld(grid4cm, VoxelData::VoxelResolution::Size_4cm, workspaceSize);
    Math::WorldCoordinates world1cm_back = Math::CoordinateConverter::gridToWorld(grid1cm, VoxelData::VoxelResolution::Size_1cm, workspaceSize);
    
    std::cout << "4cm voxel world center: (" << world4cm_back.value().x << ", " << world4cm_back.value().y << ", " << world4cm_back.value().z << ")" << std::endl;
    std::cout << "1cm voxel world center: (" << world1cm_back.value().x << ", " << world1cm_back.value().y << ", " << world1cm_back.value().z << ")" << std::endl;
    
    // Calculate bounds
    float size4cm = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_4cm);
    float size1cm = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_1cm);
    
    std::cout << std::endl << "4cm voxel bounds:" << std::endl;
    std::cout << "  X: " << (world4cm_back.value().x - size4cm/2) << " to " << (world4cm_back.value().x + size4cm/2) << std::endl;
    std::cout << "  Y: " << (world4cm_back.value().y - size4cm/2) << " to " << (world4cm_back.value().y + size4cm/2) << std::endl;
    std::cout << "  Z: " << (world4cm_back.value().z - size4cm/2) << " to " << (world4cm_back.value().z + size4cm/2) << std::endl;
    
    std::cout << "1cm voxel bounds:" << std::endl;
    std::cout << "  X: " << (world1cm_back.value().x - size1cm/2) << " to " << (world1cm_back.value().x + size1cm/2) << std::endl;
    std::cout << "  Y: " << (world1cm_back.value().y - size1cm/2) << " to " << (world1cm_back.value().y + size1cm/2) << std::endl;
    std::cout << "  Z: " << (world1cm_back.value().z - size1cm/2) << " to " << (world1cm_back.value().z + size1cm/2) << std::endl;
    
    // Check if they overlap
    bool overlaps = (world1cm_back.value().x - size1cm/2) < (world4cm_back.value().x + size4cm/2) &&
                    (world1cm_back.value().x + size1cm/2) > (world4cm_back.value().x - size4cm/2) &&
                    (world1cm_back.value().y - size1cm/2) < (world4cm_back.value().y + size4cm/2) &&
                    (world1cm_back.value().y + size1cm/2) > (world4cm_back.value().y - size4cm/2) &&
                    (world1cm_back.value().z - size1cm/2) < (world4cm_back.value().z + size4cm/2) &&
                    (world1cm_back.value().z + size1cm/2) > (world4cm_back.value().z - size4cm/2);
    
    std::cout << std::endl << "Do they overlap? " << (overlaps ? "YES" : "NO") << std::endl;
    
    if (overlaps) {
        std::cout << std::endl << "FOUND OVERLAPPING COORDINATES:" << std::endl;
        std::cout << "4cm grid: (" << grid4cm.value().x << ", " << grid4cm.value().y << ", " << grid4cm.value().z << ")" << std::endl;
        std::cout << "1cm grid: (" << grid1cm.value().x << ", " << grid1cm.value().y << ", " << grid1cm.value().z << ")" << std::endl;
    }
    
    return 0;
}