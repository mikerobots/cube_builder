#include "shader_test/TestMeshGenerator.h"
#include <glad/glad.h>
#include <cmath>

namespace VoxelEditor {
namespace ShaderTest {

void TestMeshGenerator::addCubeFace(std::vector<Rendering::Vertex>& vertices,
                                   std::vector<unsigned int>& indices,
                                   const Math::Vector3f& center,
                                   const Math::Vector3f& normal,
                                   const Math::Vector3f& up,
                                   const Math::Vector3f& right,
                                   float size,
                                   const Math::Vector3f& color) {
    unsigned int baseIndex = vertices.size();
    
    // Calculate half size
    float halfSize = size * 0.5f;
    
    // Create 4 vertices for the face
    for (int i = 0; i < 4; ++i) {
        Rendering::Vertex v;
        
        // Calculate position based on corner
        float x = (i == 0 || i == 3) ? -halfSize : halfSize;
        float y = (i == 0 || i == 1) ? -halfSize : halfSize;
        
        v.position = Math::WorldCoordinates(center + right * x + up * y);
        v.normal = normal;
        v.color = Rendering::Color(color.x, color.y, color.z, 1.0f);
        v.texCoords = Math::Vector2f((x + halfSize) / size, (y + halfSize) / size);
        
        vertices.push_back(v);
    }
    
    // Add two triangles for the face
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

Rendering::Mesh TestMeshGenerator::createCube(float size, const Math::Vector3f& color) {
    Rendering::Mesh mesh;
    
    std::vector<Rendering::Vertex> vertices;
    std::vector<unsigned int> indices;
    
    float halfSize = size * 0.5f;
    
    // Front face (+Z)
    addCubeFace(vertices, indices,
                Math::Vector3f(0, 0, halfSize),
                Math::Vector3f(0, 0, 1),
                Math::Vector3f(0, 1, 0),
                Math::Vector3f(1, 0, 0),
                size, color);
    
    // Back face (-Z)
    addCubeFace(vertices, indices,
                Math::Vector3f(0, 0, -halfSize),
                Math::Vector3f(0, 0, -1),
                Math::Vector3f(0, 1, 0),
                Math::Vector3f(-1, 0, 0),
                size, color);
    
    // Top face (+Y)
    addCubeFace(vertices, indices,
                Math::Vector3f(0, halfSize, 0),
                Math::Vector3f(0, 1, 0),
                Math::Vector3f(0, 0, -1),
                Math::Vector3f(1, 0, 0),
                size, color);
    
    // Bottom face (-Y)
    addCubeFace(vertices, indices,
                Math::Vector3f(0, -halfSize, 0),
                Math::Vector3f(0, -1, 0),
                Math::Vector3f(0, 0, 1),
                Math::Vector3f(1, 0, 0),
                size, color);
    
    // Right face (+X)
    addCubeFace(vertices, indices,
                Math::Vector3f(halfSize, 0, 0),
                Math::Vector3f(1, 0, 0),
                Math::Vector3f(0, 1, 0),
                Math::Vector3f(0, 0, -1),
                size, color);
    
    // Left face (-X)
    addCubeFace(vertices, indices,
                Math::Vector3f(-halfSize, 0, 0),
                Math::Vector3f(-1, 0, 0),
                Math::Vector3f(0, 1, 0),
                Math::Vector3f(0, 0, 1),
                size, color);
    
    mesh.vertices = std::move(vertices);
    mesh.indices = std::move(indices);
    return mesh;
}

Rendering::Mesh TestMeshGenerator::createGrid(int divisions, float spacing, int majorInterval) {
    Rendering::Mesh mesh;
    
    std::vector<Rendering::Vertex> vertices;
    std::vector<unsigned int> indices;
    
    float halfSize = (divisions * spacing) / 2.0f;
    
    // Create grid lines parallel to X axis
    for (int i = 0; i <= divisions; ++i) {
        float z = -halfSize + i * spacing;
        bool isMajor = (i % majorInterval) == 0;
        
        Rendering::Vertex v1, v2;
        v1.position = Math::WorldCoordinates(-halfSize, 0, z);
        v2.position = Math::WorldCoordinates(halfSize, 0, z);
        
        // Use color alpha to indicate major/minor lines
        float alpha = isMajor ? 1.0f : 0.5f;
        v1.color = v2.color = Rendering::Color(0.7f, 0.7f, 0.7f, alpha);
        
        unsigned int idx = vertices.size();
        vertices.push_back(v1);
        vertices.push_back(v2);
        indices.push_back(idx);
        indices.push_back(idx + 1);
    }
    
    // Create grid lines parallel to Z axis
    for (int i = 0; i <= divisions; ++i) {
        float x = -halfSize + i * spacing;
        bool isMajor = (i % majorInterval) == 0;
        
        Rendering::Vertex v1, v2;
        v1.position = Math::WorldCoordinates(x, 0, -halfSize);
        v2.position = Math::WorldCoordinates(x, 0, halfSize);
        
        float alpha = isMajor ? 1.0f : 0.5f;
        v1.color = v2.color = Rendering::Color(0.7f, 0.7f, 0.7f, alpha);
        
        unsigned int idx = vertices.size();
        vertices.push_back(v1);
        vertices.push_back(v2);
        indices.push_back(idx);
        indices.push_back(idx + 1);
    }
    
    mesh.vertices = std::move(vertices);
    mesh.indices = std::move(indices);
    return mesh;
}

Rendering::Mesh TestMeshGenerator::createWireframeCube(float size, const Math::Vector3f& color) {
    Rendering::Mesh mesh;
    
    std::vector<Rendering::Vertex> vertices;
    std::vector<unsigned int> indices;
    
    float h = size * 0.5f;
    
    // Define the 8 corners of the cube
    Math::Vector3f corners[8] = {
        Math::Vector3f(-h, -h, -h), // 0
        Math::Vector3f( h, -h, -h), // 1
        Math::Vector3f( h,  h, -h), // 2
        Math::Vector3f(-h,  h, -h), // 3
        Math::Vector3f(-h, -h,  h), // 4
        Math::Vector3f( h, -h,  h), // 5
        Math::Vector3f( h,  h,  h), // 6
        Math::Vector3f(-h,  h,  h)  // 7
    };
    
    // Add vertices
    for (int i = 0; i < 8; ++i) {
        Rendering::Vertex v;
        v.position = Math::WorldCoordinates(corners[i]);
        v.normal = Math::Vector3f(0, 1, 0); // Dummy normal
        v.color = Rendering::Color(color.x, color.y, color.z, 1.0f);
        vertices.push_back(v);
    }
    
    // Define the 12 edges
    int edges[12][2] = {
        // Bottom face
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        // Top face
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        // Vertical edges
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    
    // Add indices for edges
    for (int i = 0; i < 12; ++i) {
        indices.push_back(edges[i][0]);
        indices.push_back(edges[i][1]);
    }
    
    mesh.vertices = std::move(vertices);
    mesh.indices = std::move(indices);
    return mesh;
}

Rendering::Mesh TestMeshGenerator::createQuad(float size, const Math::Vector3f& normal) {
    Rendering::Mesh mesh;
    
    std::vector<Rendering::Vertex> vertices;
    std::vector<unsigned int> indices;
    
    // Calculate up and right vectors based on normal
    Math::Vector3f up, right;
    if (std::abs(normal.y) > 0.9f) {
        up = Math::Vector3f(0, 0, 1);
        right = Math::Vector3f(1, 0, 0);
    } else {
        up = Math::Vector3f(0, 1, 0);
        right = normal.cross(up).normalized();
        up = right.cross(normal).normalized();
    }
    
    float halfSize = size * 0.5f;
    
    // Create 4 vertices
    for (int i = 0; i < 4; ++i) {
        Rendering::Vertex v;
        
        float x = (i == 0 || i == 3) ? -halfSize : halfSize;
        float y = (i == 0 || i == 1) ? -halfSize : halfSize;
        
        v.position = Math::WorldCoordinates(right * x + up * y);
        v.normal = normal;
        v.color = Rendering::Color(0.8f, 0.8f, 0.8f, 1.0f);
        v.texCoords = Math::Vector2f((x + halfSize) / size, (y + halfSize) / size);
        
        vertices.push_back(v);
    }
    
    // Two triangles
    indices = {0, 1, 2, 0, 2, 3};
    
    mesh.vertices = std::move(vertices);
    mesh.indices = std::move(indices);
    return mesh;
}

Rendering::Mesh TestMeshGenerator::createVoxelCluster(int xCount, int yCount, int zCount,
                                                     float voxelSize, float spacing) {
    Rendering::Mesh mesh;
    
    std::vector<Rendering::Vertex> allVertices;
    std::vector<unsigned int> allIndices;
    
    float step = voxelSize + spacing;
    
    for (int x = 0; x < xCount; ++x) {
        for (int y = 0; y < yCount; ++y) {
            for (int z = 0; z < zCount; ++z) {
                // Create a cube at this position
                Math::Vector3f offset(x * step, y * step, z * step);
                
                // Get a single cube mesh
                auto cubeMesh = createCube(voxelSize);
                
                // Offset all vertices and add to combined mesh
                unsigned int indexOffset = allVertices.size();
                
                for (auto& vertex : cubeMesh.vertices) {
                    vertex.position = Math::WorldCoordinates(vertex.position.value() + offset);
                    allVertices.push_back(vertex);
                }
                
                for (auto index : cubeMesh.indices) {
                    allIndices.push_back(index + indexOffset);
                }
            }
        }
    }
    
    mesh.vertices = std::move(allVertices);
    mesh.indices = std::move(allIndices);
    return mesh;
}

Rendering::Mesh TestMeshGenerator::createSphere(float radius, int segments, int rings) {
    Rendering::Mesh mesh;
    
    std::vector<Rendering::Vertex> vertices;
    std::vector<unsigned int> indices;
    
    const float PI = 3.14159265359f;
    
    // Generate vertices
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = PI * ring / rings;
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);
        
        for (int seg = 0; seg <= segments; ++seg) {
            float theta = 2.0f * PI * seg / segments;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);
            
            Rendering::Vertex v;
            v.position = Math::WorldCoordinates(
                radius * sinPhi * cosTheta,
                radius * cosPhi,
                radius * sinPhi * sinTheta
            );
            v.normal = v.position.value().normalized();
            v.color = Rendering::Color(0.7f, 0.7f, 0.7f, 1.0f);
            v.texCoords = Math::Vector2f(
                static_cast<float>(seg) / segments,
                static_cast<float>(ring) / rings
            );
            
            vertices.push_back(v);
        }
    }
    
    // Generate indices
    for (int ring = 0; ring < rings; ++ring) {
        for (int seg = 0; seg < segments; ++seg) {
            int current = ring * (segments + 1) + seg;
            int next = current + segments + 1;
            
            // Two triangles per quad
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
    
    mesh.vertices = std::move(vertices);
    mesh.indices = std::move(indices);
    return mesh;
}

Rendering::Mesh TestMeshGenerator::createHighlightBox(float innerSize, float outerSize) {
    Rendering::Mesh mesh;
    
    // Create outer box with transparency
    auto outerBox = createCube(outerSize);
    
    // Set alpha for transparency effect
    for (auto& vertex : outerBox.vertices) {
        vertex.color.a = 0.3f;
    }
    
    mesh = std::move(outerBox);
    return mesh;
}

unsigned int TestMeshGenerator::getPrimitiveType(const std::string& meshType) {
    if (meshType == "grid" || meshType == "wireframe" || meshType == "lines") {
        return GL_LINES;
    }
    // Default to triangles for most meshes
    return GL_TRIANGLES;
}

} // namespace ShaderTest
} // namespace VoxelEditor