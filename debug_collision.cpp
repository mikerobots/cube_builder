#include "core/voxel_data/VoxelDataManager.h"
#include "foundation/math/CoordinateConverter.h"
#include <iostream>

using namespace VoxelEditor;

int main() {
    VoxelData::VoxelDataManager manager;
    
    // Debug the coordinate mappings from the test
    Math::Vector3f workspaceSize = manager.getWorkspaceSize();
    std::cout << "Workspace size: " << workspaceSize.x << ", " << workspaceSize.y << ", " << workspaceSize.z << std::endl;
    
    // 4cm voxel at grid position (5,0,5)
    Math::GridCoordinates grid4cm(Math::Vector3i(5, 0, 5));
    Math::WorldCoordinates world4cm = Math::CoordinateConverter::gridToWorld(grid4cm, VoxelData::VoxelResolution::Size_4cm, workspaceSize);
    std::cout << "4cm voxel at grid (5,0,5) = world (" << world4cm.value().x << ", " << world4cm.value().y << ", " << world4cm.value().z << ")" << std::endl;
    
    float voxelSize4cm = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_4cm);
    std::cout << "4cm voxel size: " << voxelSize4cm << "m" << std::endl;
    std::cout << "4cm voxel covers: " << world4cm.value().x << " to " << (world4cm.value().x + voxelSize4cm) << std::endl;
    
    // 1cm voxel at grid position (20,0,20)
    Math::GridCoordinates grid1cm(Math::Vector3i(20, 0, 20));
    Math::WorldCoordinates world1cm = Math::CoordinateConverter::gridToWorld(grid1cm, VoxelData::VoxelResolution::Size_1cm, workspaceSize);
    std::cout << "1cm voxel at grid (20,0,20) = world (" << world1cm.value().x << ", " << world1cm.value().y << ", " << world1cm.value().z << ")" << std::endl;
    
    float voxelSize1cm = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_1cm);
    std::cout << "1cm voxel size: " << voxelSize1cm << "m" << std::endl;
    std::cout << "1cm voxel covers: " << world1cm.value().x << " to " << (world1cm.value().x + voxelSize1cm) << std::endl;
    
    // Test the actual collision
    manager.setVoxel(Math::Vector3i(5, 0, 5), VoxelData::VoxelResolution::Size_4cm, true);
    bool wouldOverlap = manager.wouldOverlap(Math::Vector3i(20, 0, 20), VoxelData::VoxelResolution::Size_1cm);
    std::cout << "wouldOverlap result: " << (wouldOverlap ? "true" : "false") << std::endl;
    
    return 0;
}