#include "MeshBuilder.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <map>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace VoxelEditor {
namespace SurfaceGen {

// Forward declarations for MeshSimplifier
struct MeshSimplifier::Vertex {
    Math::Vector3f position;
    MeshSimplifier::Quadric quadric;
    std::vector<MeshSimplifier::Triangle*> triangles;
    std::vector<MeshSimplifier::Edge*> edges;
    bool deleted = false;
};

struct MeshSimplifier::Triangle {
    MeshSimplifier::Vertex* vertices[3];
    Math::Vector3f normal;
    bool deleted = false;
};

struct MeshSimplifier::Edge {
    MeshSimplifier::Vertex* v0;
    MeshSimplifier::Vertex* v1;
    float cost;
    Math::Vector3f optimalPosition;
    bool deleted = false;
};

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

// Stub implementations for missing methods
void MeshBuilder::optimizeVertexCache() {
    // TODO: Implement vertex cache optimization
    // This would reorder vertices/indices for better GPU cache usage
}

void MeshBuilder::generateFlatNormals() {
    // TODO: Implement flat shading normals
    // Each face would get its own set of vertices with the same normal
}

void MeshBuilder::generateSphericalUVs() {
    // TODO: Implement spherical UV mapping
    m_uvCoords.clear();
    m_uvCoords.reserve(m_vertices.size());
    
    for (const auto& vertex : m_vertices) {
        Math::Vector2f uv;
        // Simple spherical mapping
        float r = vertex.length();
        if (r > 0.0001f) {
            float theta = std::atan2(vertex.z, vertex.x);
            float phi = std::acos(vertex.y / r);
            uv.x = (theta + M_PI) / (2.0f * M_PI);
            uv.y = phi / M_PI;
        }
        m_uvCoords.push_back(uv);
    }
}

void MeshBuilder::generateCylindricalUVs(const Math::Vector3f& axis) {
    // TODO: Implement cylindrical UV mapping
    m_uvCoords.clear();
    m_uvCoords.reserve(m_vertices.size());
    
    for (const auto& vertex : m_vertices) {
        Math::Vector2f uv;
        // Simple cylindrical mapping along Y axis
        float theta = std::atan2(vertex.z, vertex.x);
        uv.x = (theta + M_PI) / (2.0f * M_PI);
        uv.y = vertex.y;
        m_uvCoords.push_back(uv);
    }
}

Mesh MeshBuilder::transformMesh(const Mesh& mesh, const Math::Matrix4f& transform) {
    // TODO: Implement mesh transformation
    Mesh result = mesh;
    result.transform(transform);
    return result;
}

MeshStats MeshBuilder::analyzeMesh(const Mesh& mesh) {
    // TODO: Implement mesh analysis
    MeshStats stats;
    stats.triangleCount = mesh.indices.size() / 3;
    stats.vertexCount = mesh.vertices.size();
    stats.bounds = mesh.bounds;
    stats.isManifold = MeshUtils::isManifold(mesh);
    stats.isWatertight = MeshUtils::isWatertight(mesh);
    stats.volume = MeshUtils::calculateVolume(mesh);
    stats.surfaceArea = MeshUtils::calculateSurfaceArea(mesh);
    return stats;
}

bool MeshBuilder::repairMesh(Mesh& mesh) {
    // TODO: Implement mesh repair
    bool repaired = false;
    
    // Remove degenerate triangles
    MeshUtils::removeDegenerateTriangles(mesh);
    
    // Fix normals if needed
    if (mesh.normals.empty() || mesh.normals.size() != mesh.vertices.size()) {
        mesh.calculateNormals();
        repaired = true;
    }
    
    return repaired;
}

// MeshSimplifier implementation
MeshSimplifier::MeshSimplifier() : m_lastError(0.0f), m_collapsedEdges(0) {
}

MeshSimplifier::~MeshSimplifier() = default;

// Quadric implementation
MeshSimplifier::Quadric::Quadric() {
    for (int i = 0; i < 10; ++i) {
        m[i] = 0.0;
    }
}

MeshSimplifier::Quadric MeshSimplifier::Quadric::operator+(const Quadric& q) const {
    Quadric result;
    for (int i = 0; i < 10; ++i) {
        result.m[i] = m[i] + q.m[i];
    }
    return result;
}

double MeshSimplifier::Quadric::evaluate(const Math::Vector3f& v) const {
    // Evaluate v^T * Q * v where Q is the 4x4 symmetric matrix
    double x = v.x, y = v.y, z = v.z, w = 1.0;
    
    // Matrix multiplication for symmetric matrix stored as upper triangle
    // m[0] m[1] m[2] m[3]
    //      m[4] m[5] m[6]
    //           m[7] m[8]
    //                m[9]
    
    double vQ[4];
    vQ[0] = x*m[0] + y*m[1] + z*m[2] + w*m[3];
    vQ[1] = x*m[1] + y*m[4] + z*m[5] + w*m[6];
    vQ[2] = x*m[2] + y*m[5] + z*m[7] + w*m[8];
    vQ[3] = x*m[3] + y*m[6] + z*m[8] + w*m[9];
    
    return x*vQ[0] + y*vQ[1] + z*vQ[2] + w*vQ[3];
}

Math::Vector3f MeshSimplifier::Quadric::minimize() const {
    // Solve for the optimal position that minimizes the quadric error
    // This involves solving a 3x3 linear system
    
    // Extract the 3x3 matrix A and vector b from the 4x4 quadric
    double A[3][3] = {
        {m[0], m[1], m[2]},
        {m[1], m[4], m[5]},
        {m[2], m[5], m[7]}
    };
    
    double b[3] = {-m[3], -m[6], -m[8]};
    
    // Solve Ax = b using Cramer's rule (for simplicity)
    double det = A[0][0] * (A[1][1]*A[2][2] - A[1][2]*A[2][1])
               - A[0][1] * (A[1][0]*A[2][2] - A[1][2]*A[2][0])
               + A[0][2] * (A[1][0]*A[2][1] - A[1][1]*A[2][0]);
    
    if (std::abs(det) < 1e-10) {
        // Singular matrix, return origin
        return Math::Vector3f(0, 0, 0);
    }
    
    // Calculate solution using Cramer's rule
    double x = (b[0] * (A[1][1]*A[2][2] - A[1][2]*A[2][1])
              - A[0][1] * (b[1]*A[2][2] - A[1][2]*b[2])
              + A[0][2] * (b[1]*A[2][1] - A[1][1]*b[2])) / det;
              
    double y = (A[0][0] * (b[1]*A[2][2] - A[1][2]*b[2])
              - b[0] * (A[1][0]*A[2][2] - A[1][2]*A[2][0])
              + A[0][2] * (A[1][0]*b[2] - b[1]*A[2][0])) / det;
              
    double z = (A[0][0] * (A[1][1]*b[2] - b[1]*A[2][1])
              - A[0][1] * (A[1][0]*b[2] - b[1]*A[2][0])
              + b[0] * (A[1][0]*A[2][1] - A[1][1]*A[2][0])) / det;
    
    return Math::Vector3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

Mesh MeshSimplifier::simplify(const Mesh& mesh, const SimplificationSettings& settings) {
    if (settings.targetRatio > 0.0f && settings.targetRatio <= 1.0f) {
        size_t targetTriangles = static_cast<size_t>(mesh.indices.size() / 3 * settings.targetRatio);
        return simplifyToTargetCount(mesh, targetTriangles);
    } else if (settings.maxError > 0.0f) {
        return simplifyByError(mesh, settings.maxError);
    }
    
    // No simplification requested
    return mesh;
}

Mesh MeshSimplifier::simplifyToTargetCount(const Mesh& mesh, size_t targetTriangles) {
    // Build internal data structures
    buildDataStructures(mesh);
    
    // Compute initial quadrics and edge costs
    computeQuadrics();
    computeEdgeCosts();
    
    // Main simplification loop
    size_t currentTriangles = m_triangles.size();
    m_collapsedEdges = 0;
    
    while (currentTriangles > targetTriangles && !m_edges.empty()) {
        // Find the edge with minimum cost
        Edge* minEdge = findMinCostEdge();
        if (!minEdge || minEdge->cost == std::numeric_limits<float>::infinity()) {
            break; // No more valid edges to collapse
        }
        
        // Collapse the edge
        collapseEdge(minEdge);
        currentTriangles = m_triangles.size();
        m_collapsedEdges++;
    }
    
    // Extract the simplified mesh
    return extractMesh();
}

Mesh MeshSimplifier::simplifyByError(const Mesh& mesh, float maxError) {
    // Build internal data structures
    buildDataStructures(mesh);
    
    // Compute initial quadrics and edge costs
    computeQuadrics();
    computeEdgeCosts();
    
    // Main simplification loop
    m_collapsedEdges = 0;
    m_lastError = 0.0f;
    
    while (!m_edges.empty()) {
        // Find the edge with minimum cost
        Edge* minEdge = findMinCostEdge();
        if (!minEdge || minEdge->cost > maxError) {
            break; // No more edges within error threshold
        }
        
        m_lastError = minEdge->cost;
        
        // Collapse the edge
        collapseEdge(minEdge);
        m_collapsedEdges++;
    }
    
    // Extract the simplified mesh
    return extractMesh();
}


void MeshSimplifier::buildDataStructures(const Mesh& mesh) {
    // Clear previous data
    m_vertices.clear();
    m_triangles.clear();
    m_edges.clear();
    
    // Create vertices
    m_vertices.reserve(mesh.vertices.size());
    for (const auto& pos : mesh.vertices) {
        auto vertex = std::make_unique<Vertex>();
        vertex->position = pos;
        m_vertices.push_back(std::move(vertex));
    }
    
    // Create triangles and build connectivity
    std::map<std::pair<Vertex*, Vertex*>, Edge*> edgeMap;
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        auto triangle = std::make_unique<Triangle>();
        
        // Set vertices
        for (int j = 0; j < 3; ++j) {
            triangle->vertices[j] = m_vertices[mesh.indices[i + j]].get();
            triangle->vertices[j]->triangles.push_back(triangle.get());
        }
        
        // Calculate normal
        Math::Vector3f edge1 = triangle->vertices[1]->position - triangle->vertices[0]->position;
        Math::Vector3f edge2 = triangle->vertices[2]->position - triangle->vertices[0]->position;
        triangle->normal = edge1.cross(edge2);
        float length = triangle->normal.length();
        if (length > 0.0001f) {
            triangle->normal = triangle->normal / length;
        }
        
        // Create edges
        for (int j = 0; j < 3; ++j) {
            Vertex* v0 = triangle->vertices[j];
            Vertex* v1 = triangle->vertices[(j + 1) % 3];
            
            // Order vertices consistently
            if (v0 > v1) std::swap(v0, v1);
            
            auto key = std::make_pair(v0, v1);
            if (edgeMap.find(key) == edgeMap.end()) {
                auto edge = std::make_unique<Edge>();
                edge->v0 = v0;
                edge->v1 = v1;
                
                v0->edges.push_back(edge.get());
                v1->edges.push_back(edge.get());
                
                edgeMap[key] = edge.get();
                m_edges.push_back(std::move(edge));
            }
        }
        
        m_triangles.push_back(std::move(triangle));
    }
}

void MeshSimplifier::computeQuadrics() {
    // Initialize vertex quadrics from face planes
    for (auto& triangle : m_triangles) {
        if (triangle->deleted) continue;
        
        // Compute plane equation ax + by + cz + d = 0
        const Math::Vector3f& n = triangle->normal;
        float d = -n.dot(triangle->vertices[0]->position);
        
        // Build quadric from plane
        Quadric q;
        q.m[0] = n.x * n.x;  // a*a
        q.m[1] = n.x * n.y;  // a*b
        q.m[2] = n.x * n.z;  // a*c
        q.m[3] = n.x * d;    // a*d
        q.m[4] = n.y * n.y;  // b*b
        q.m[5] = n.y * n.z;  // b*c
        q.m[6] = n.y * d;    // b*d
        q.m[7] = n.z * n.z;  // c*c
        q.m[8] = n.z * d;    // c*d
        q.m[9] = d * d;      // d*d
        
        // Add to vertex quadrics
        for (int i = 0; i < 3; ++i) {
            triangle->vertices[i]->quadric = triangle->vertices[i]->quadric + q;
        }
    }
}

void MeshSimplifier::computeEdgeCosts() {
    for (auto& edge : m_edges) {
        if (edge->deleted) continue;
        updateEdgeCost(edge.get());
    }
}

void MeshSimplifier::updateEdgeCost(Edge* edge) {
    // Compute the quadric for the unified vertex
    Quadric q = edge->v0->quadric + edge->v1->quadric;
    
    // Find optimal position
    edge->optimalPosition = q.minimize();
    
    // If minimize failed, try edge endpoints and midpoint
    float minError = q.evaluate(edge->optimalPosition);
    
    float error0 = q.evaluate(edge->v0->position);
    if (error0 < minError) {
        minError = error0;
        edge->optimalPosition = edge->v0->position;
    }
    
    float error1 = q.evaluate(edge->v1->position);
    if (error1 < minError) {
        minError = error1;
        edge->optimalPosition = edge->v1->position;
    }
    
    Math::Vector3f midpoint = (edge->v0->position + edge->v1->position) * 0.5f;
    float errorMid = q.evaluate(midpoint);
    if (errorMid < minError) {
        minError = errorMid;
        edge->optimalPosition = midpoint;
    }
    
    edge->cost = static_cast<float>(minError);
}

MeshSimplifier::Edge* MeshSimplifier::findMinCostEdge() {
    Edge* minEdge = nullptr;
    float minCost = std::numeric_limits<float>::infinity();
    
    for (auto& edge : m_edges) {
        if (!edge->deleted && edge->cost < minCost) {
            minCost = edge->cost;
            minEdge = edge.get();
        }
    }
    
    return minEdge;
}

void MeshSimplifier::collapseEdge(Edge* edge) {
    Vertex* v0 = edge->v0;
    Vertex* v1 = edge->v1;
    
    // Move v0 to optimal position
    v0->position = edge->optimalPosition;
    
    // Update v0's quadric
    v0->quadric = v0->quadric + v1->quadric;
    
    // Update triangles that use v1 to use v0 instead
    for (auto* triangle : v1->triangles) {
        if (triangle->deleted) continue;
        
        // Check if this triangle would become degenerate
        bool hasV0 = false;
        for (int i = 0; i < 3; ++i) {
            if (triangle->vertices[i] == v0) {
                hasV0 = true;
                break;
            }
        }
        
        if (hasV0) {
            // Triangle will become degenerate, remove it
            removeTriangle(triangle);
        } else {
            // Update triangle to use v0 instead of v1
            for (int i = 0; i < 3; ++i) {
                if (triangle->vertices[i] == v1) {
                    triangle->vertices[i] = v0;
                    v0->triangles.push_back(triangle);
                }
            }
        }
    }
    
    // Update edges
    for (auto* e : v1->edges) {
        if (e->deleted || e == edge) continue;
        
        // Update edge to point to v0
        if (e->v0 == v1) e->v0 = v0;
        if (e->v1 == v1) e->v1 = v0;
        
        // Check if edge became degenerate
        if (e->v0 == e->v1) {
            e->deleted = true;
        } else {
            v0->edges.push_back(e);
            updateEdgeCost(e);
        }
    }
    
    // Update costs of edges connected to v0
    for (auto* e : v0->edges) {
        if (!e->deleted) {
            updateEdgeCost(e);
        }
    }
    
    // Mark edge and v1 as deleted
    edge->deleted = true;
    v1->deleted = true;
}

void MeshSimplifier::removeTriangle(Triangle* triangle) {
    triangle->deleted = true;
    
    // Remove from vertex triangle lists
    for (int i = 0; i < 3; ++i) {
        auto& triangles = triangle->vertices[i]->triangles;
        triangles.erase(std::remove(triangles.begin(), triangles.end(), triangle), triangles.end());
    }
}

Mesh MeshSimplifier::extractMesh() {
    MeshBuilder builder;
    builder.beginMesh();
    
    // Map from old vertex to new index
    std::unordered_map<Vertex*, uint32_t> vertexMap;
    
    // Add non-deleted vertices
    for (auto& vertex : m_vertices) {
        if (!vertex->deleted) {
            uint32_t index = builder.addVertex(vertex->position);
            vertexMap[vertex.get()] = index;
        }
    }
    
    // Add non-deleted triangles
    for (auto& triangle : m_triangles) {
        if (!triangle->deleted) {
            builder.addTriangle(
                vertexMap[triangle->vertices[0]],
                vertexMap[triangle->vertices[1]],
                vertexMap[triangle->vertices[2]]
            );
        }
    }
    
    return builder.endMesh();
}

// MeshUtils additional implementations
bool MeshUtils::isManifold(const Mesh& mesh) {
    // TODO: Implement full manifold check
    // For now, just check if it's watertight
    return isWatertight(mesh);
}

void MeshUtils::flipNormals(Mesh& mesh) {
    for (auto& normal : mesh.normals) {
        normal = normal * -1.0f;
    }
    
    // Also flip triangle winding
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        std::swap(mesh.indices[i + 1], mesh.indices[i + 2]);
    }
}

void MeshUtils::removeDegenerateTriangles(Mesh& mesh, float epsilon) {
    std::vector<uint32_t> newIndices;
    newIndices.reserve(mesh.indices.size());
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Math::Vector3f& v0 = mesh.vertices[mesh.indices[i]];
        const Math::Vector3f& v1 = mesh.vertices[mesh.indices[i + 1]];
        const Math::Vector3f& v2 = mesh.vertices[mesh.indices[i + 2]];
        
        // Check if triangle is degenerate
        Math::Vector3f edge1 = v1 - v0;
        Math::Vector3f edge2 = v2 - v0;
        Math::Vector3f cross = edge1.cross(edge2);
        
        if (cross.length() > epsilon) {
            // Keep this triangle
            newIndices.push_back(mesh.indices[i]);
            newIndices.push_back(mesh.indices[i + 1]);
            newIndices.push_back(mesh.indices[i + 2]);
        }
    }
    
    mesh.indices = std::move(newIndices);
}

void MeshUtils::fillHoles(Mesh& mesh) {
    // TODO: Implement hole filling algorithm
    // This is complex and would involve finding boundary loops and triangulating them
}

void MeshUtils::makeWatertight(Mesh& mesh) {
    // TODO: Implement watertight mesh creation
    fillHoles(mesh);
}

Mesh MeshUtils::quadToTriangleMesh(const std::vector<QuadFace>& quads, const std::vector<Math::Vector3f>& vertices) {
    MeshBuilder builder;
    builder.beginMesh();
    
    // Add all vertices
    for (const auto& vertex : vertices) {
        builder.addVertex(vertex);
    }
    
    // Convert quads to triangles
    for (const auto& quad : quads) {
        builder.addQuad(quad.vertices[0], quad.vertices[1], quad.vertices[2], quad.vertices[3]);
    }
    
    return builder.endMesh();
}

std::vector<QuadFace> MeshUtils::triangleToQuadMesh(const Mesh& mesh) {
    // TODO: Implement triangle to quad conversion
    // This is a complex algorithm that would involve finding pairs of triangles that form good quads
    std::vector<QuadFace> quads;
    return quads;
}

Mesh MeshUtils::subdivide(const Mesh& mesh, int levels) {
    // TODO: Implement subdivision (e.g., Loop subdivision)
    Mesh result = mesh;
    
    for (int level = 0; level < levels; ++level) {
        // Simple midpoint subdivision for now
        MeshBuilder builder;
        builder.beginMesh();
        
        // Add original vertices
        for (const auto& vertex : result.vertices) {
            builder.addVertex(vertex);
        }
        
        // Add edge midpoints and subdivide triangles
        std::map<std::pair<uint32_t, uint32_t>, uint32_t> edgeMidpoints;
        
        for (size_t i = 0; i < result.indices.size(); i += 3) {
            uint32_t v0 = result.indices[i];
            uint32_t v1 = result.indices[i + 1];
            uint32_t v2 = result.indices[i + 2];
            
            // Get or create edge midpoints
            auto getOrCreateMidpoint = [&](uint32_t a, uint32_t b) -> uint32_t {
                if (a > b) std::swap(a, b);
                auto key = std::make_pair(a, b);
                
                auto it = edgeMidpoints.find(key);
                if (it != edgeMidpoints.end()) {
                    return it->second;
                }
                
                Math::Vector3f midpoint = (result.vertices[a] + result.vertices[b]) * 0.5f;
                uint32_t index = builder.addVertex(midpoint);
                edgeMidpoints[key] = index;
                return index;
            };
            
            uint32_t m01 = getOrCreateMidpoint(v0, v1);
            uint32_t m12 = getOrCreateMidpoint(v1, v2);
            uint32_t m20 = getOrCreateMidpoint(v2, v0);
            
            // Create 4 new triangles
            builder.addTriangle(v0, m01, m20);
            builder.addTriangle(v1, m12, m01);
            builder.addTriangle(v2, m20, m12);
            builder.addTriangle(m01, m12, m20);
        }
        
        result = builder.endMesh();
    }
    
    return result;
}

Mesh MeshUtils::decimate(const Mesh& mesh, float ratio) {
    // Use the mesh simplifier
    MeshSimplifier simplifier;
    size_t targetTriangles = static_cast<size_t>(mesh.indices.size() / 3 * ratio);
    return simplifier.simplifyToTargetCount(mesh, targetTriangles);
}

Mesh MeshUtils::remesh(const Mesh& mesh, float targetEdgeLength) {
    // TODO: Implement remeshing algorithm
    // This would involve edge splits, collapses, and flips to achieve uniform edge lengths
    return mesh;
}

void MeshUtils::translateMesh(Mesh& mesh, const Math::Vector3f& translation) {
    for (auto& vertex : mesh.vertices) {
        vertex = vertex + translation;
    }
    mesh.calculateBounds();
}

}
}