#pragma once

#include <vector>
#include <memory>
#include "rendering/RenderTypes.h"
#include "voxel_data/VoxelTypes.h"
#include "math/Vector3f.h"

namespace VoxelEditor {
namespace VoxelData {
    class VoxelDataManager;
}

class VoxelMeshGenerator {
public:
    struct Vertex {
        Math::Vector3f position;
        Math::Vector3f normal;
        Math::Vector3f color;
    };
    
    VoxelMeshGenerator();
    ~VoxelMeshGenerator();
    
    // Generate a simple cube mesh for all voxels
    Rendering::Mesh generateCubeMesh(const VoxelData::VoxelDataManager& voxelData);
    
    // Generate edge lines for all voxels (for wireframe overlay)
    Rendering::Mesh generateEdgeMesh(const VoxelData::VoxelDataManager& voxelData);
    
private:
    // Add cube vertices for a single voxel
    void addCube(std::vector<Vertex>& vertices, 
                 std::vector<uint32_t>& indices,
                 const Math::Vector3f& position,
                 float size,
                 const Math::Vector3f& color);
                 
    // Add edge lines for a single voxel
    void addCubeEdges(std::vector<Vertex>& vertices,
                      std::vector<uint32_t>& indices,
                      const Math::Vector3f& position,
                      float size,
                      const Math::Vector3f& color);
    
    // Cube vertex data (8 vertices)
    static const float s_cubeVertices[8][3];
    // Cube face indices (6 faces, 2 triangles each)
    static const uint32_t s_cubeFaces[6][4];
    // Face normals
    static const float s_faceNormals[6][3];
};

} // namespace VoxelEditor