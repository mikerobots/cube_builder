#pragma once

#include "SurfaceTypes.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector3i.h"
#include <unordered_map>
#include <memory>

namespace VoxelEditor {
namespace SurfaceGen {

class MeshBuilder {
public:
    MeshBuilder();
    ~MeshBuilder();
    
    // Start building a new mesh
    void beginMesh();
    
    // Add geometry
    uint32_t addVertex(const Math::Vector3f& position);
    uint32_t addVertex(const Math::Vector3f& position, const Math::Vector3f& normal);
    uint32_t addVertex(const Math::Vector3f& position, const Math::Vector3f& normal, const Math::Vector2f& uv);
    
    void addTriangle(uint32_t v0, uint32_t v1, uint32_t v2);
    void addQuad(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3);
    
    // Set current material
    void setMaterial(MaterialId material) { m_currentMaterial = material; }
    
    // Finalize and get mesh
    Mesh endMesh();
    
    // Mesh optimization
    void removeDuplicateVertices(float epsilon = 0.0001f);
    void optimizeVertexCache();
    void generateSmoothNormals();
    void generateFlatNormals();
    
    // UV generation
    void generateBoxUVs(float scale = 1.0f);
    void generateSphericalUVs();
    void generateCylindricalUVs(const Math::Vector3f& axis = Math::Vector3f(0, 1, 0));
    
    // Mesh operations
    static Mesh combineMeshes(const std::vector<Mesh>& meshes);
    static Mesh transformMesh(const Mesh& mesh, const Math::Matrix4f& transform);
    static Mesh smoothMesh(const Mesh& mesh, int iterations, float factor = 0.5f);
    
    // Mesh validation
    static MeshStats analyzeMesh(const Mesh& mesh);
    static bool repairMesh(Mesh& mesh);
    
    // Get current state
    size_t getCurrentVertexCount() const { return m_vertices.size(); }
    size_t getCurrentTriangleCount() const { return m_indices.size() / 3; }
    
private:
    // Current mesh data
    std::vector<Math::Vector3f> m_vertices;
    std::vector<Math::Vector3f> m_normals;
    std::vector<Math::Vector2f> m_uvCoords;
    std::vector<uint32_t> m_indices;
    MaterialId m_currentMaterial;
    
    // Vertex deduplication
    struct VertexKey {
        Math::Vector3f position;
        Math::Vector3f normal;
        Math::Vector2f uv;
        bool hasNormal;
        bool hasUV;
        
        bool equals(const VertexKey& other, float epsilon) const;
        size_t hash() const;
    };
    
    std::unordered_map<size_t, std::vector<uint32_t>> m_vertexMap;
    
    // Helper methods
    uint32_t findOrAddVertex(const VertexKey& key, float epsilon);
    void calculateFaceNormal(uint32_t i0, uint32_t i1, uint32_t i2, Math::Vector3f& normal);
    void laplacianSmooth(std::vector<Math::Vector3f>& vertices, const std::vector<uint32_t>& indices, float factor);
};

// Mesh simplification
class MeshSimplifier {
public:
    MeshSimplifier();
    ~MeshSimplifier();
    
    // Simplify mesh
    Mesh simplify(const Mesh& mesh, const SimplificationSettings& settings);
    Mesh simplifyToTargetCount(const Mesh& mesh, size_t targetTriangles);
    Mesh simplifyByError(const Mesh& mesh, float maxError);
    
    // Get simplification metrics
    float getLastError() const { return m_lastError; }
    size_t getCollapsedEdges() const { return m_collapsedEdges; }
    
private:
    struct Vertex;
    struct Triangle;
    struct Edge;
    
    // Quadric error metric
    struct Quadric {
        double m[10]; // Symmetric 4x4 matrix stored as upper triangle
        
        Quadric();
        Quadric operator+(const Quadric& q) const;
        double evaluate(const Math::Vector3f& v) const;
        Math::Vector3f minimize() const;
    };
    
    // Internal data structures
    std::vector<std::unique_ptr<Vertex>> m_vertices;
    std::vector<std::unique_ptr<Triangle>> m_triangles;
    std::vector<std::unique_ptr<Edge>> m_edges;
    
    float m_lastError;
    size_t m_collapsedEdges;
    
    // Simplification methods
    void buildDataStructures(const Mesh& mesh);
    void computeQuadrics();
    void computeEdgeCosts();
    Edge* findMinCostEdge();
    void collapseEdge(Edge* edge);
    Mesh extractMesh();
    
    // Helper methods
    void updateVertexQuadric(Vertex* vertex);
    void updateEdgeCost(Edge* edge);
    void removeTriangle(Triangle* triangle);
};

// Mesh utilities
class MeshUtils {
public:
    // Mesh analysis
    static bool isManifold(const Mesh& mesh);
    static bool isWatertight(const Mesh& mesh);
    static float calculateVolume(const Mesh& mesh);
    static float calculateSurfaceArea(const Mesh& mesh);
    
    // Mesh repair
    static void flipNormals(Mesh& mesh);
    static void removeDegenerateTriangles(Mesh& mesh, float epsilon = 0.0001f);
    static void fillHoles(Mesh& mesh);
    static void makeWatertight(Mesh& mesh);
    
    // Mesh conversion
    static Mesh quadToTriangleMesh(const std::vector<QuadFace>& quads, const std::vector<Math::Vector3f>& vertices);
    static std::vector<QuadFace> triangleToQuadMesh(const Mesh& mesh);
    
    // Mesh operations
    static Mesh subdivide(const Mesh& mesh, int levels = 1);
    static Mesh decimate(const Mesh& mesh, float ratio);
    static Mesh remesh(const Mesh& mesh, float targetEdgeLength);
    
    // Export helpers
    static void centerMesh(Mesh& mesh);
    static void scaleMesh(Mesh& mesh, float scale);
    static void translateMesh(Mesh& mesh, const Math::Vector3f& translation);
};

}
}