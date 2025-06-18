#include <iostream>
#include <vector>
#include "foundation/math/CoordinateConverter.h"
#include "core/voxel_data/VoxelTypes.h"

using namespace VoxelEditor;

void printVoxelInfo(const std::string& name, const Math::Vector3i& gridPos, VoxelData::VoxelResolution resolution, const Math::Vector3f& workspaceSize) {
    Math::GridCoordinates grid(gridPos);
    Math::WorldCoordinates world = Math::CoordinateConverter::gridToWorld(grid, resolution, workspaceSize);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    std::cout << name << ":" << std::endl;
    std::cout << "  Grid: (" << gridPos.x << ", " << gridPos.y << ", " << gridPos.z << ")" << std::endl;
    std::cout << "  World center: (" << world.value().x << ", " << world.value().y << ", " << world.value().z << ")" << std::endl;
    std::cout << "  Size: " << voxelSize << "m" << std::endl;
    std::cout << "  Bounds: (" << (world.value().x - voxelSize/2) << ", " << (world.value().y - voxelSize/2) << ", " << (world.value().z - voxelSize/2) << ")" 
              << " to (" << (world.value().x + voxelSize/2) << ", " << (world.value().y + voxelSize/2) << ", " << (world.value().z + voxelSize/2) << ")" << std::endl;
}

bool checkOverlap(const Math::Vector3i& grid1, VoxelData::VoxelResolution res1,
                  const Math::Vector3i& grid2, VoxelData::VoxelResolution res2,
                  const Math::Vector3f& workspaceSize) {
    Math::GridCoordinates g1(grid1), g2(grid2);
    Math::WorldCoordinates w1 = Math::CoordinateConverter::gridToWorld(g1, res1, workspaceSize);
    Math::WorldCoordinates w2 = Math::CoordinateConverter::gridToWorld(g2, res2, workspaceSize);
    
    float size1 = VoxelData::getVoxelSize(res1);
    float size2 = VoxelData::getVoxelSize(res2);
    
    // AABB overlap check
    return (std::abs(w1.value().x - w2.value().x) < (size1 + size2) / 2) &&
           (std::abs(w1.value().y - w2.value().y) < (size1 + size2) / 2) &&
           (std::abs(w1.value().z - w2.value().z) < (size1 + size2) / 2);
}

int main() {
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    
    std::cout << "=== Coordinate System Analysis ===" << std::endl;
    std::cout << "Workspace: " << workspaceSize.x << "x" << workspaceSize.y << "x" << workspaceSize.z << std::endl;
    
    // Find some coordinates that should definitely overlap
    Math::Vector3i large4cm(125, 1, 125);
    Math::Vector3i test1cm(500, 1, 500);
    
    printVoxelInfo("4cm voxel", large4cm, VoxelData::VoxelResolution::Size_4cm, workspaceSize);
    printVoxelInfo("1cm voxel test", test1cm, VoxelData::VoxelResolution::Size_1cm, workspaceSize);
    
    bool overlaps = checkOverlap(large4cm, VoxelData::VoxelResolution::Size_4cm,
                                test1cm, VoxelData::VoxelResolution::Size_1cm,
                                workspaceSize);
    
    std::cout << "Manual overlap check: " << (overlaps ? "YES" : "NO") << std::endl;
    
    // Let's try to find coordinates that actually DO overlap
    std::cout << std::endl << "=== Finding overlapping coordinates ===" << std::endl;
    
    // Place a 4cm voxel at a simple position and find 1cm positions that overlap
    Math::Vector3i base4cm(100, 50, 100);  // Try different positions
    printVoxelInfo("Base 4cm voxel", base4cm, VoxelData::VoxelResolution::Size_4cm, workspaceSize);
    
    // Test various 1cm positions around it
    for (int offset = -2; offset <= 2; offset++) {
        Math::Vector3i test1cm_grid(400 + offset*10, 50, 400 + offset*10);  // Scale appropriately
        
        bool overlaps = checkOverlap(base4cm, VoxelData::VoxelResolution::Size_4cm,
                                    test1cm_grid, VoxelData::VoxelResolution::Size_1cm,
                                    workspaceSize);
        
        if (overlaps) {
            std::cout << "FOUND OVERLAP:" << std::endl;
            printVoxelInfo("  1cm voxel", test1cm_grid, VoxelData::VoxelResolution::Size_1cm, workspaceSize);
        }
    }
    
    return 0;
}