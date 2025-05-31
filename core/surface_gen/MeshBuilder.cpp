#include "MeshBuilder.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <map>

namespace VoxelEditor {
namespace SurfaceGen {

// VertexKey implementation
bool MeshBuilder::VertexKey::equals(const VertexKey& other, float epsilon) const {
    if (hasNormal != other.hasNormal || hasUV != other.hasUV) {
        return false;
    }
    
    // Compare positions
    if (std::abs(position.x - other.position.x) > epsilon ||
        std::abs(position.y - other.position.y) > epsilon ||
        std::abs(position.z - other.position.z) > epsilon) {
        return false;
    }
    
    // Compare normals if present
    if (hasNormal) {
        if (std::abs(normal.x - other.normal.x) > epsilon ||
            std::abs(normal.y - other.normal.y) > epsilon ||
            std::abs(normal.z - other.normal.z) > epsilon) {
            return false;
        }
    }
    
    // Compare UVs if present
    if (hasUV) {
        if (std::abs(uv.x - other.uv.x) > epsilon ||
            std::abs(uv.y - other.uv.y) > epsilon) {
            return false;
        }
    }
    
    return true;
}

size_t MeshBuilder::VertexKey::hash() const {
    size_t h = 0;
    
    auto hashCombine = [&h](size_t value) {
        h ^= value + 0x9e3779b9 + (h << 6) + (h >> 2);
    };
    
    // Hash position (quantized)
    hashCombine(std::hash<int>{}(static_cast<int>(position.x * 10000)));
    hashCombine(std::hash<int>{}(static_cast<int>(position.y * 10000)));
    hashCombine(std::hash<int>{}(static_cast<int>(position.z * 10000)));
    
    return h;
}

// MeshBuilder implementation
MeshBuilder::MeshBuilder() : m_currentMaterial(0) {
}

MeshBuilder::~MeshBuilder() = default;

void MeshBuilder::beginMesh() {
    m_vertices.clear();
    m_normals.clear();
    m_uvCoords.clear();
    m_indices.clear();
    m_vertexMap.clear();
    m_currentMaterial = 0;
}

uint32_t MeshBuilder::addVertex(const Math::Vector3f& position) {
    m_vertices.push_back(position);
    return static_cast<uint32_t>(m_vertices.size() - 1);
}

uint32_t MeshBuilder::addVertex(const Math::Vector3f& position, const Math::Vector3f& normal) {
    m_vertices.push_back(position);
    m_normals.push_back(normal);
    
    // Keep normals array same size as vertices
    while (m_normals.size() < m_vertices.size()) {
        m_normals.push_back(Math::Vector3f(0, 1, 0));
    }
    
    return static_cast<uint32_t>(m_vertices.size() - 1);
}

uint32_t MeshBuilder::addVertex(const Math::Vector3f& position, const Math::Vector3f& normal, const Math::Vector2f& uv) {
    m_vertices.push_back(position);
    m_normals.push_back(normal);
    m_uvCoords.push_back(uv);
    
    // Keep arrays same size
    while (m_normals.size() < m_vertices.size()) {
        m_normals.push_back(Math::Vector3f(0, 1, 0));
    }
    while (m_uvCoords.size() < m_vertices.size()) {
        m_uvCoords.push_back(Math::Vector2f(0, 0));
    }
    
    return static_cast<uint32_t>(m_vertices.size() - 1);
}

void MeshBuilder::addTriangle(uint32_t v0, uint32_t v1, uint32_t v2) {
    m_indices.push_back(v0);
    m_indices.push_back(v1);
    m_indices.push_back(v2);
}

void MeshBuilder::addQuad(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3) {
    // Convert quad to two triangles
    addTriangle(v0, v1, v2);
    addTriangle(v0, v2, v3);
}

Mesh MeshBuilder::endMesh() {
    Mesh mesh;
    mesh.vertices = std::move(m_vertices);
    mesh.normals = std::move(m_normals);
    mesh.uvCoords = std::move(m_uvCoords);
    mesh.indices = std::move(m_indices);
    mesh.materialId = m_currentMaterial;
    
    // Calculate bounds
    mesh.calculateBounds();
    
    // Generate normals if not provided
    if (mesh.normals.empty() && !mesh.vertices.empty()) {
        mesh.calculateNormals();
    }
    
    // Clear builder state
    beginMesh();
    
    return mesh;
}

void MeshBuilder::removeDuplicateVertices(float epsilon) {
    std::vector<uint32_t> vertexRemap(m_vertices.size());
    std::vector<Math::Vector3f> uniqueVertices;
    std::vector<Math::Vector3f> uniqueNormals;
    std::vector<Math::Vector2f> uniqueUVs;
    
    // Map from old vertex hash to new unique index
    std::unordered_map<size_t, std::vector<uint32_t>> uniqueVertexMap;
    
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        VertexKey key;
        key.position = m_vertices[i];
        key.hasNormal = i < m_normals.size();
        key.hasUV = i < m_uvCoords.size();
        
        if (key.hasNormal) {
            key.normal = m_normals[i];
        }
        if (key.hasUV) {
            key.uv = m_uvCoords[i];
        }
        
        // Check if we already have this vertex
        size_t h = key.hash();
        auto& bucket = uniqueVertexMap[h];
        
        uint32_t uniqueIndex = static_cast<uint32_t>(uniqueVertices.size());
        bool found = false;
        
        for (uint32_t existingIndex : bucket) {
            VertexKey existing;
            existing.position = uniqueVertices[existingIndex];
            existing.hasNormal = existingIndex < uniqueNormals.size();
            existing.hasUV = existingIndex < uniqueUVs.size();
            
            if (existing.hasNormal) {
                existing.normal = uniqueNormals[existingIndex];
            }
            if (existing.hasUV) {
                existing.uv = uniqueUVs[existingIndex];
            }
            
            if (key.equals(existing, epsilon)) {
                uniqueIndex = existingIndex;
                found = true;
                break;
            }
        }
        
        if (!found) {
            // New unique vertex
            bucket.push_back(uniqueIndex);
            uniqueVertices.push_back(key.position);
            if (key.hasNormal) {
                uniqueNormals.push_back(key.normal);
            }
            if (key.hasUV) {
                uniqueUVs.push_back(key.uv);
            }
        }
        
        vertexRemap[i] = uniqueIndex;
    }
    
    // Update indices
    for (auto& index : m_indices) {
        index = vertexRemap[index];
    }
    
    // Replace with unique vertices
    m_vertices = std::move(uniqueVertices);
    m_normals = std::move(uniqueNormals);
    m_uvCoords = std::move(uniqueUVs);
}

uint32_t MeshBuilder::findOrAddVertex(const VertexKey& key, float epsilon) {
    size_t h = key.hash();
    auto& bucket = m_vertexMap[h];
    
    for (uint32_t index : bucket) {
        VertexKey existing;
        existing.position = m_vertices[index];
        existing.hasNormal = index < m_normals.size();
        existing.hasUV = index < m_uvCoords.size();
        
        if (existing.hasNormal) {
            existing.normal = m_normals[index];
        }
        if (existing.hasUV) {
            existing.uv = m_uvCoords[index];
        }
        
        if (key.equals(existing, epsilon)) {
            return index;
        }
    }
    
    // Not found, add new
    uint32_t newIndex = static_cast<uint32_t>(m_vertices.size());
    bucket.push_back(newIndex);
    return newIndex;
}

void MeshBuilder::generateSmoothNormals() {
    m_normals.clear();
    m_normals.resize(m_vertices.size(), Math::Vector3f(0, 0, 0));
    
    // Accumulate face normals at vertices
    for (size_t i = 0; i < m_indices.size(); i += 3) {
        uint32_t i0 = m_indices[i];
        uint32_t i1 = m_indices[i + 1];
        uint32_t i2 = m_indices[i + 2];
        
        Math::Vector3f faceNormal;
        calculateFaceNormal(i0, i1, i2, faceNormal);
        
        m_normals[i0] = m_normals[i0] + faceNormal;
        m_normals[i1] = m_normals[i1] + faceNormal;
        m_normals[i2] = m_normals[i2] + faceNormal;
    }
    
    // Normalize
    for (auto& normal : m_normals) {
        float length = normal.length();
        if (length > 0.0001f) {
            normal = normal / length;
        } else {
            normal = Math::Vector3f(0, 1, 0);
        }
    }
}

void MeshBuilder::calculateFaceNormal(uint32_t i0, uint32_t i1, uint32_t i2, Math::Vector3f& normal) {
    const Math::Vector3f& v0 = m_vertices[i0];
    const Math::Vector3f& v1 = m_vertices[i1];
    const Math::Vector3f& v2 = m_vertices[i2];
    
    Math::Vector3f edge1 = v1 - v0;
    Math::Vector3f edge2 = v2 - v0;
    normal = edge1.cross(edge2);
    
    float length = normal.length();
    if (length > 0.0001f) {
        normal = normal / length;
    }
}

void MeshBuilder::generateBoxUVs(float scale) {
    m_uvCoords.clear();
    m_uvCoords.reserve(m_vertices.size());
    
    for (const auto& vertex : m_vertices) {
        // Simple box mapping - use the two most significant axes
        Math::Vector2f uv;
        
        float absX = std::abs(vertex.x);
        float absY = std::abs(vertex.y);
        float absZ = std::abs(vertex.z);
        
        if (absX >= absY && absX >= absZ) {
            // X-axis dominant - use YZ plane
            uv.x = vertex.y * scale;
            uv.y = vertex.z * scale;
        } else if (absY >= absX && absY >= absZ) {
            // Y-axis dominant - use XZ plane
            uv.x = vertex.x * scale;
            uv.y = vertex.z * scale;
        } else {
            // Z-axis dominant - use XY plane
            uv.x = vertex.x * scale;
            uv.y = vertex.y * scale;
        }
        
        m_uvCoords.push_back(uv);
    }
}

Mesh MeshBuilder::combineMeshes(const std::vector<Mesh>& meshes) {
    MeshBuilder builder;
    builder.beginMesh();
    
    for (const auto& mesh : meshes) {
        uint32_t vertexOffset = static_cast<uint32_t>(builder.m_vertices.size());
        
        // Add vertices
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            if (!mesh.normals.empty() && i < mesh.normals.size()) {
                if (!mesh.uvCoords.empty() && i < mesh.uvCoords.size()) {
                    builder.addVertex(mesh.vertices[i], mesh.normals[i], mesh.uvCoords[i]);
                } else {
                    builder.addVertex(mesh.vertices[i], mesh.normals[i]);
                }
            } else {
                builder.addVertex(mesh.vertices[i]);
            }
        }
        
        // Add indices with offset
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            builder.addTriangle(
                mesh.indices[i] + vertexOffset,
                mesh.indices[i + 1] + vertexOffset,
                mesh.indices[i + 2] + vertexOffset
            );
        }
    }
    
    return builder.endMesh();
}

Mesh MeshBuilder::smoothMesh(const Mesh& mesh, int iterations, float factor) {
    Mesh result = mesh;
    
    for (int iter = 0; iter < iterations; ++iter) {
        MeshBuilder builder;
        builder.m_vertices = result.vertices;
        builder.m_indices = result.indices;
        
        builder.laplacianSmooth(builder.m_vertices, builder.m_indices, factor);
        
        result.vertices = builder.m_vertices;
        result.calculateNormals();
    }
    
    return result;
}

void MeshBuilder::laplacianSmooth(std::vector<Math::Vector3f>& vertices, 
                                 const std::vector<uint32_t>& indices, float factor) {
    // Build vertex adjacency
    std::vector<std::unordered_set<uint32_t>> adjacency(vertices.size());
    
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];
        
        adjacency[i0].insert(i1);
        adjacency[i0].insert(i2);
        adjacency[i1].insert(i0);
        adjacency[i1].insert(i2);
        adjacency[i2].insert(i0);
        adjacency[i2].insert(i1);
    }
    
    // Apply Laplacian smoothing
    std::vector<Math::Vector3f> smoothedVertices = vertices;
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (adjacency[i].empty()) {
            continue;
        }
        
        Math::Vector3f avg(0, 0, 0);
        for (uint32_t neighbor : adjacency[i]) {
            avg = avg + vertices[neighbor];
        }
        avg = avg / static_cast<float>(adjacency[i].size());
        
        smoothedVertices[i] = vertices[i] + (avg - vertices[i]) * factor;
    }
    
    vertices = std::move(smoothedVertices);
}

// MeshUtils implementation
bool MeshUtils::isWatertight(const Mesh& mesh) {
    // A mesh is watertight if every edge is shared by exactly two triangles
    std::map<std::pair<uint32_t, uint32_t>, int> edgeCount;
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        uint32_t i0 = mesh.indices[i];
        uint32_t i1 = mesh.indices[i + 1];
        uint32_t i2 = mesh.indices[i + 2];
        
        // Add edges (always with smaller index first)
        edgeCount[{std::min(i0, i1), std::max(i0, i1)}]++;
        edgeCount[{std::min(i1, i2), std::max(i1, i2)}]++;
        edgeCount[{std::min(i2, i0), std::max(i2, i0)}]++;
    }
    
    // Check if all edges are shared by exactly 2 triangles
    for (const auto& edge : edgeCount) {
        if (edge.second != 2) {
            return false;
        }
    }
    
    return true;
}

float MeshUtils::calculateVolume(const Mesh& mesh) {
    float volume = 0.0f;
    
    // Use divergence theorem: Volume = 1/6 * sum of (v0 . (v1 x v2))
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Math::Vector3f& v0 = mesh.vertices[mesh.indices[i]];
        const Math::Vector3f& v1 = mesh.vertices[mesh.indices[i + 1]];
        const Math::Vector3f& v2 = mesh.vertices[mesh.indices[i + 2]];
        
        volume += v0.dot(v1.cross(v2));
    }
    
    return std::abs(volume) / 6.0f;
}

float MeshUtils::calculateSurfaceArea(const Mesh& mesh) {
    float area = 0.0f;
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Math::Vector3f& v0 = mesh.vertices[mesh.indices[i]];
        const Math::Vector3f& v1 = mesh.vertices[mesh.indices[i + 1]];
        const Math::Vector3f& v2 = mesh.vertices[mesh.indices[i + 2]];
        
        Math::Vector3f edge1 = v1 - v0;
        Math::Vector3f edge2 = v2 - v0;
        Math::Vector3f cross = edge1.cross(edge2);
        
        area += cross.length() * 0.5f;
    }
    
    return area;
}

void MeshUtils::centerMesh(Mesh& mesh) {
    if (mesh.vertices.empty()) {
        return;
    }
    
    // Calculate center
    Math::Vector3f center(0, 0, 0);
    for (const auto& vertex : mesh.vertices) {
        center = center + vertex;
    }
    center = center / static_cast<float>(mesh.vertices.size());
    
    // Translate to origin
    for (auto& vertex : mesh.vertices) {
        vertex = vertex - center;
    }
    
    mesh.calculateBounds();
}

void MeshUtils::scaleMesh(Mesh& mesh, float scale) {
    for (auto& vertex : mesh.vertices) {
        vertex = vertex * scale;
    }
    
    mesh.calculateBounds();
}

}
}