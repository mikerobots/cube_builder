#include "cli/VoxelMeshGenerator.h"
#include "voxel_data/VoxelDataManager.h"
#include <algorithm>

namespace VoxelEditor {

// Cube vertices (unit cube centered at origin)
const float VoxelMeshGenerator::s_cubeVertices[8][3] = {
    {-0.5f, -0.5f, -0.5f}, // 0
    { 0.5f, -0.5f, -0.5f}, // 1
    { 0.5f,  0.5f, -0.5f}, // 2
    {-0.5f,  0.5f, -0.5f}, // 3
    {-0.5f, -0.5f,  0.5f}, // 4
    { 0.5f, -0.5f,  0.5f}, // 5
    { 0.5f,  0.5f,  0.5f}, // 6
    {-0.5f,  0.5f,  0.5f}  // 7
};

// Face indices (counter-clockwise winding)
const uint32_t VoxelMeshGenerator::s_cubeFaces[6][4] = {
    {0, 1, 2, 3}, // Front (-Z)
    {5, 4, 7, 6}, // Back (+Z)
    {4, 0, 3, 7}, // Left (-X)
    {1, 5, 6, 2}, // Right (+X)
    {4, 5, 1, 0}, // Bottom (-Y)
    {3, 2, 6, 7}  // Top (+Y)
};

// Face normals
const float VoxelMeshGenerator::s_faceNormals[6][3] = {
    { 0.0f,  0.0f, -1.0f}, // Front
    { 0.0f,  0.0f,  1.0f}, // Back
    {-1.0f,  0.0f,  0.0f}, // Left
    { 1.0f,  0.0f,  0.0f}, // Right
    { 0.0f, -1.0f,  0.0f}, // Bottom
    { 0.0f,  1.0f,  0.0f}  // Top
};

VoxelMeshGenerator::VoxelMeshGenerator() {
}

VoxelMeshGenerator::~VoxelMeshGenerator() {
}

Rendering::Mesh VoxelMeshGenerator::generateCubeMesh(const VoxelData::VoxelDataManager& voxelData) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Get active resolution
    auto resolution = voxelData.getActiveResolution();
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Get all voxels at current resolution
    auto voxelPositions = voxelData.getAllVoxels(resolution);
    
    // Generate cube for each voxel
    for (const auto& voxelPos : voxelPositions) {
        // Convert voxel coordinates to world position
        Math::Vector3f worldPos(
            voxelPos.gridPos.x * voxelSize + voxelSize * 0.5f,
            voxelPos.gridPos.y * voxelSize + voxelSize * 0.5f,
            voxelPos.gridPos.z * voxelSize + voxelSize * 0.5f
        );
        
        // Color based on height for now
        float colorIntensity = 0.3f + (voxelPos.gridPos.y / 10.0f) * 0.7f;
        Math::Vector3f color(colorIntensity, colorIntensity * 0.8f, colorIntensity * 0.6f);
        
        addCube(vertices, indices, worldPos, voxelSize * 0.95f, color); // Slight gap between cubes
    }
    
    // Create mesh
    Rendering::Mesh mesh;
    
    if (!vertices.empty()) {
        // Convert our vertices to the format expected by Rendering::Mesh
        mesh.vertices.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i) {
            mesh.vertices[i].position = vertices[i].position;
            mesh.vertices[i].normal = vertices[i].normal;
            mesh.vertices[i].color = Rendering::Color(
                vertices[i].color.x,
                vertices[i].color.y,
                vertices[i].color.z,
                1.0f
            );
        }
        
        mesh.indices = std::move(indices);
    }
    
    return mesh;
}

void VoxelMeshGenerator::addCube(std::vector<Vertex>& vertices,
                                std::vector<uint32_t>& indices,
                                const Math::Vector3f& position,
                                float size,
                                const Math::Vector3f& color) {
    uint32_t baseIndex = vertices.size();
    
    // Add vertices for each face
    for (int face = 0; face < 6; ++face) {
        Math::Vector3f normal(s_faceNormals[face][0], 
                             s_faceNormals[face][1], 
                             s_faceNormals[face][2]);
        
        // Add 4 vertices for this face
        for (int v = 0; v < 4; ++v) {
            int vertexIndex = s_cubeFaces[face][v];
            Vertex vertex;
            vertex.position = Math::Vector3f(
                position.x + s_cubeVertices[vertexIndex][0] * size,
                position.y + s_cubeVertices[vertexIndex][1] * size,
                position.z + s_cubeVertices[vertexIndex][2] * size
            );
            vertex.normal = normal;
            vertex.color = color;
            vertices.push_back(vertex);
        }
        
        // Add indices for 2 triangles
        uint32_t faceBase = baseIndex + face * 4;
        // First triangle
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 1);
        indices.push_back(faceBase + 2);
        // Second triangle
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 3);
    }
}

} // namespace VoxelEditor