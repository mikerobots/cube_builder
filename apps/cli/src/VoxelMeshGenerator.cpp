#include "cli/VoxelMeshGenerator.h"
#include "voxel_data/VoxelDataManager.h"
#include "logging/Logger.h"
#include "math/CoordinateTypes.h"
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
    
    // Create mesh (declare early for error returns)
    Rendering::Mesh mesh;
    
    // Get active resolution
    auto resolution = voxelData.getActiveResolution();
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    Logging::Logger::getInstance().debugfc("VoxelMeshGenerator",
        "Generating mesh for resolution: %d, voxel size: %.2f", static_cast<int>(resolution), voxelSize);
    
    // Get all voxels at current resolution
    auto voxelPositions = voxelData.getAllVoxels(resolution);
    
    Logging::Logger::getInstance().debugfc("VoxelMeshGenerator",
        "Found %zu voxels to render", voxelPositions.size());
    
    // Get the voxel grid for proper coordinate conversion
    const VoxelData::VoxelGrid* grid = voxelData.getGrid(resolution);
    if (!grid) {
        Logging::Logger::getInstance().error("VoxelMeshGenerator", "Failed to get grid for resolution");
        return mesh;
    }
    
    // Generate cube for each voxel
    int voxelCount = 0;
    for (const auto& voxelPos : voxelPositions) {
        // Convert voxel grid coordinates to world position using grid's coordinate system
        Math::WorldCoordinates worldCoords = grid->incrementToWorld(voxelPos.incrementPos);
        Math::Vector3f worldPos(worldCoords.x(), worldCoords.y(), worldCoords.z());
        
        if (voxelCount < 3) {
            Logging::Logger::getInstance().debugfc("VoxelMeshGenerator",
                "  Voxel %d at grid pos (%d, %d, %d) -> world pos (%.3f, %.3f, %.3f)",
                voxelCount, voxelPos.incrementPos.x(), voxelPos.incrementPos.y(), voxelPos.incrementPos.z(),
                worldPos.x, worldPos.y, worldPos.z);
            Logging::Logger::getInstance().debugfc("VoxelMeshGenerator",
                "  VoxelSize: %.3f, Scale: %.3f, Final size: %.3f",
                voxelSize, 0.95f, voxelSize * 0.95f);
        }
        
        // Use red color as expected by tests
        Math::Vector3f color(1.0f, 0.0f, 0.0f);  // Red
        
        addCube(vertices, indices, worldPos, voxelSize * 0.95f, color); // Slight gap between cubes
        voxelCount++;
    }
    
    if (!vertices.empty()) {
        // Convert our vertices to the format expected by Rendering::Mesh
        mesh.vertices.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i) {
            mesh.vertices[i].position = Math::WorldCoordinates(vertices[i].position);
            mesh.vertices[i].normal = vertices[i].normal;
            mesh.vertices[i].color = Rendering::Color(
                vertices[i].color.x,
                vertices[i].color.y,
                vertices[i].color.z,
                1.0f
            );
        }
        
        mesh.indices = std::move(indices);
        
        Logging::Logger::getInstance().debugfc("VoxelMeshGenerator",
            "Generated mesh with %zu vertices and %zu indices", 
            mesh.vertices.size(), mesh.indices.size());
        
        // Print first few vertices for debugging
        for (size_t i = 0; i < std::min(size_t(3), mesh.vertices.size()); ++i) {
            Logging::Logger::getInstance().debugfc("VoxelMeshGenerator",
                "  Vertex %zu: pos(%.3f, %.3f, %.3f)", i,
                mesh.vertices[i].position.x(),
                mesh.vertices[i].position.y(),
                mesh.vertices[i].position.z());
        }
    } else {
        Logging::Logger::getInstance().debug("VoxelMeshGenerator", "No vertices generated (empty mesh)");
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

Rendering::Mesh VoxelMeshGenerator::generateEdgeMesh(const VoxelData::VoxelDataManager& voxelData) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Create mesh (declare early for error returns)
    Rendering::Mesh mesh;
    
    // Get active resolution
    auto resolution = voxelData.getActiveResolution();
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    Logging::Logger::getInstance().debugfc("VoxelMeshGenerator",
        "Generating edge mesh for resolution: %d, voxel size: %.2f", static_cast<int>(resolution), voxelSize);
    
    // Get all voxels at current resolution
    auto voxelPositions = voxelData.getAllVoxels(resolution);
    
    // Get the voxel grid for proper coordinate conversion
    const VoxelData::VoxelGrid* grid = voxelData.getGrid(resolution);
    if (!grid) {
        Logging::Logger::getInstance().error("VoxelMeshGenerator", "Failed to get grid for resolution");
        return mesh;
    }
    
    // Generate edge lines for each voxel
    for (const auto& voxelPos : voxelPositions) {
        // Convert voxel grid coordinates to world position using grid's coordinate system
        Math::WorldCoordinates worldCoords = grid->incrementToWorld(voxelPos.incrementPos);
        Math::Vector3f worldPos(worldCoords.x(), worldCoords.y(), worldCoords.z());
        
        // Use black color for edges
        Math::Vector3f edgeColor(0.1f, 0.1f, 0.1f);  // Very dark gray
        
        addCubeEdges(vertices, indices, worldPos, voxelSize * 0.95f, edgeColor);
    }
    
    if (!vertices.empty()) {
        // Convert our vertices to the format expected by Rendering::Mesh
        mesh.vertices.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i) {
            mesh.vertices[i].position = Math::WorldCoordinates(vertices[i].position);
            mesh.vertices[i].normal = vertices[i].normal;
            mesh.vertices[i].color = Rendering::Color(
                vertices[i].color.x,
                vertices[i].color.y,
                vertices[i].color.z,
                1.0f
            );
        }
        
        mesh.indices = std::move(indices);
        
        Logging::Logger::getInstance().debugfc("VoxelMeshGenerator",
            "Generated edge mesh with %zu vertices and %zu indices", 
            mesh.vertices.size(), mesh.indices.size());
    }
    
    return mesh;
}

void VoxelMeshGenerator::addCubeEdges(std::vector<Vertex>& vertices,
                                     std::vector<uint32_t>& indices,
                                     const Math::Vector3f& position,
                                     float size,
                                     const Math::Vector3f& color) {
    uint32_t baseIndex = vertices.size();
    
    // Add 8 unique vertices for the cube corners
    for (int i = 0; i < 8; ++i) {
        Vertex vertex;
        vertex.position = Math::Vector3f(
            position.x + s_cubeVertices[i][0] * size,
            position.y + s_cubeVertices[i][1] * size,
            position.z + s_cubeVertices[i][2] * size
        );
        vertex.normal = Math::Vector3f(0, 1, 0); // Dummy normal for lines
        vertex.color = color;
        vertices.push_back(vertex);
    }
    
    // Define the 12 edges of a cube
    // Each edge connects two vertices
    static const int edges[12][2] = {
        // Bottom face edges
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        // Top face edges
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        // Vertical edges
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    
    // Add line indices for each edge
    for (int i = 0; i < 12; ++i) {
        indices.push_back(baseIndex + edges[i][0]);
        indices.push_back(baseIndex + edges[i][1]);
    }
}

} // namespace VoxelEditor