#include <iostream>
#include "core/surface_gen/DualContouring.h"
#include "core/voxel_data/VoxelGrid.h"

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
namespace Math = VoxelEditor::Math;

int main() {
    // Create a grid with a single voxel at origin
    Math::Vector3f workspaceSize(2.0f, 2.0f, 2.0f);
    VoxelGrid grid(VoxelResolution::Size_32cm, workspaceSize);
    
    // Place a single voxel to debug cell positions
    grid.setVoxel(Math::IncrementCoordinates(0, 0, 0), true);
    
    std::cout << "Created grid with single voxel at (0,0,0)" << std::endl;
    
    // Generate mesh using dual contouring  
    DualContouring dc;
    Mesh mesh = dc.generateMesh(grid, SurfaceSettings::Preview());
    
    std::cout << "Generated mesh:" << std::endl;
    std::cout << "  Vertices: " << mesh.vertices.size() << std::endl;
    std::cout << "  Indices: " << mesh.indices.size() << std::endl;
    std::cout << "  Triangles: " << mesh.indices.size() / 3 << std::endl;
    
    // Print vertices
    for (size_t i = 0; i < mesh.vertices.size() && i < 10; ++i) {
        const auto& v = mesh.vertices[i];
        std::cout << "  Vertex " << i << ": (" << v.x() << ", " << v.y() << ", " << v.z() << ")" << std::endl;
    }
    
    return 0;
}