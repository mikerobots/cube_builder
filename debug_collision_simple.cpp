#include <iostream>
#include <vector>
#include "foundation/math/CoordinateConverter.h"
#include "core/voxel_data/VoxelTypes.h"

using namespace VoxelEditor;

int main() {
    // Test setup from CollisionDetection_DifferentSizeOverlap
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);  // Default workspace
    
    std::cout << "=== Collision Detection Debug ===" << std::endl;
    std::cout << "Workspace size: " << workspaceSize.x << "x" << workspaceSize.y << "x" << workspaceSize.z << std::endl;
    
    // 4cm voxel at grid position (5,0,5)
    Math::GridCoordinates grid4cm(Math::Vector3i(5, 0, 5));
    Math::WorldCoordinates world4cm = Math::CoordinateConverter::gridToWorld(grid4cm, VoxelData::VoxelResolution::Size_4cm, workspaceSize);
    float voxelSize4cm = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_4cm);
    
    std::cout << std::endl << "4cm voxel at grid (5,0,5):" << std::endl;
    std::cout << "  World center: (" << world4cm.value().x << ", " << world4cm.value().y << ", " << world4cm.value().z << ")" << std::endl;
    std::cout << "  Voxel size: " << voxelSize4cm << "m" << std::endl;
    std::cout << "  World bounds: " << (world4cm.value().x - voxelSize4cm/2) << " to " << (world4cm.value().x + voxelSize4cm/2) << " (X)" << std::endl;
    std::cout << "  World bounds: " << (world4cm.value().z - voxelSize4cm/2) << " to " << (world4cm.value().z + voxelSize4cm/2) << " (Z)" << std::endl;
    
    // Test both 1cm voxels
    std::vector<Math::Vector3i> testPositions = {{20, 0, 20}, {24, 0, 24}};
    std::vector<bool> expectedOverlap = {true, false};
    
    for (size_t i = 0; i < testPositions.size(); i++) {
        Math::GridCoordinates grid1cm(testPositions[i]);
        Math::WorldCoordinates world1cm = Math::CoordinateConverter::gridToWorld(grid1cm, VoxelData::VoxelResolution::Size_1cm, workspaceSize);
        float voxelSize1cm = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_1cm);
        
        std::cout << std::endl << "1cm voxel at grid (" << testPositions[i].x << "," << testPositions[i].y << "," << testPositions[i].z << "):" << std::endl;
        std::cout << "  World center: (" << world1cm.value().x << ", " << world1cm.value().y << ", " << world1cm.value().z << ")" << std::endl;
        std::cout << "  Voxel size: " << voxelSize1cm << "m" << std::endl;
        std::cout << "  World bounds: " << (world1cm.value().x - voxelSize1cm/2) << " to " << (world1cm.value().x + voxelSize1cm/2) << " (X)" << std::endl;
        std::cout << "  World bounds: " << (world1cm.value().z - voxelSize1cm/2) << " to " << (world1cm.value().z + voxelSize1cm/2) << " (Z)" << std::endl;
        
        // Check overlap
        bool xOverlap = (world1cm.value().x - voxelSize1cm/2) < (world4cm.value().x + voxelSize4cm/2) &&
                        (world1cm.value().x + voxelSize1cm/2) > (world4cm.value().x - voxelSize4cm/2);
        bool zOverlap = (world1cm.value().z - voxelSize1cm/2) < (world4cm.value().z + voxelSize4cm/2) &&
                        (world1cm.value().z + voxelSize1cm/2) > (world4cm.value().z - voxelSize4cm/2);
        
        std::cout << "  X overlap: " << (xOverlap ? "YES" : "NO") << std::endl;
        std::cout << "  Z overlap: " << (zOverlap ? "YES" : "NO") << std::endl;
        std::cout << "  Should overlap: " << (xOverlap && zOverlap ? "YES" : "NO") << " (expected: " << (expectedOverlap[i] ? "YES" : "NO") << ")" << std::endl;
    }
    
    return 0;
}