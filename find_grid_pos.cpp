#include <iostream>
#include "foundation/math/CoordinateConverter.h"
#include "core/voxel_data/VoxelTypes.h"

using namespace VoxelEditor;

int main() {
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    
    std::cout << "Finding grid positions for desired world coordinates..." << std::endl;
    
    // We want 4cm voxel to cover 0.2-0.24m, so center should be at (0.22, 0, 0.22)
    Math::WorldCoordinates desiredWorld(Math::Vector3f(0.22f, 0.02f, 0.22f));
    Math::GridCoordinates grid4cm = Math::CoordinateConverter::worldToGrid(desiredWorld, VoxelData::VoxelResolution::Size_4cm, workspaceSize);
    
    std::cout << "For 4cm voxel center at world (0.22, 0.02, 0.22):" << std::endl;
    std::cout << "  Grid position: (" << grid4cm.value().x << ", " << grid4cm.value().y << ", " << grid4cm.value().z << ")" << std::endl;
    
    // Verify this gives us the expected coverage
    Math::WorldCoordinates actualWorld = Math::CoordinateConverter::gridToWorld(grid4cm, VoxelData::VoxelResolution::Size_4cm, workspaceSize);
    float voxelSize4cm = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_4cm);
    
    std::cout << "  Actual world center: (" << actualWorld.value().x << ", " << actualWorld.value().y << ", " << actualWorld.value().z << ")" << std::endl;
    std::cout << "  Coverage: " << (actualWorld.value().x - voxelSize4cm/2) << " to " << (actualWorld.value().x + voxelSize4cm/2) << " (X)" << std::endl;
    
    // Now find grid positions for 1cm voxels at boundaries
    std::cout << std::endl << "1cm voxel positions:" << std::endl;
    
    // Should overlap: center at (0.205, 0, 0.205) - inside the 4cm voxel
    Math::WorldCoordinates world1cm_inside(Math::Vector3f(0.205f, 0.005f, 0.205f));
    Math::GridCoordinates grid1cm_inside = Math::CoordinateConverter::worldToGrid(world1cm_inside, VoxelData::VoxelResolution::Size_1cm, workspaceSize);
    std::cout << "  Inside (should overlap): grid (" << grid1cm_inside.value().x << ", " << grid1cm_inside.value().y << ", " << grid1cm_inside.value().z << ")" << std::endl;
    
    // Should not overlap: center at (0.245, 0, 0.245) - outside the 4cm voxel
    Math::WorldCoordinates world1cm_outside(Math::Vector3f(0.245f, 0.005f, 0.245f));
    Math::GridCoordinates grid1cm_outside = Math::CoordinateConverter::worldToGrid(world1cm_outside, VoxelData::VoxelResolution::Size_1cm, workspaceSize);
    std::cout << "  Outside (should not overlap): grid (" << grid1cm_outside.value().x << ", " << grid1cm_outside.value().y << ", " << grid1cm_outside.value().z << ")" << std::endl;
    
    return 0;
}