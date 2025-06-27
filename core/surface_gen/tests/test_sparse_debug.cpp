#include <iostream>
#include "../DualContouringSparse.h"
#include "../../voxel_data/VoxelGrid.h"

using namespace VoxelEditor;

int main() {
    // Create a simple test grid
    Math::Vector3f workspaceSize(2.0f, 2.0f, 2.0f);
    VoxelData::VoxelGrid grid(VoxelData::VoxelResolution::Size_32cm, workspaceSize);
    
    // Add one voxel
    Math::IncrementCoordinates voxelPos(32, 32, 32);
    std::cout << "Adding voxel at " << voxelPos.value() << std::endl;
    grid.setVoxel(voxelPos, true);
    
    // Get grid info
    auto dims = grid.getGridDimensions();
    std::cout << "Grid dimensions: " << dims << std::endl;
    
    auto voxels = grid.getAllVoxels();
    std::cout << "Number of voxels: " << voxels.size() << std::endl;
    
    for (const auto& v : voxels) {
        std::cout << "Voxel at increment pos: " << v.incrementPos.value() 
                  << ", resolution: " << static_cast<int>(v.resolution) << std::endl;
    }
    
    // Test dual contouring
    SurfaceGen::DualContouringSparse dc;
    auto mesh = dc.generateMesh(grid, SurfaceGen::SurfaceSettings::Preview());
    
    std::cout << "Mesh vertices: " << mesh.vertices.size() 
              << ", triangles: " << mesh.indices.size() / 3 << std::endl;
    
    return 0;
}