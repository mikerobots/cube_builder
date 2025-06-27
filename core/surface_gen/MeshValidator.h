#pragma once

#include "SurfaceTypes.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>

namespace VoxelEditor {
namespace SurfaceGen {

/**
 * @brief Validates mesh properties for 3D printing and quality assurance
 * 
 * This class checks meshes for printability including watertight validation,
 * manifold geometry checks, and minimum feature size constraints. It also
 * provides mesh repair suggestions and basic repair functionality.
 * 
 * @requirements REQ-10.1.11, REQ-10.1.14
 */
class MeshValidator {
public:
    /**
     * @brief Validation results structure
     */
    struct ValidationResult {
        bool isValid = true;                    ///< Overall validity
        bool isWatertight = false;              ///< No holes or gaps
        bool isManifold = false;                ///< Valid manifold geometry
        bool hasMinimumFeatureSize = false;     ///< All features meet minimum size
        bool hasCorrectOrientation = false;     ///< Consistent face orientation
        bool hasSelfIntersections = false;      ///< No self-intersecting faces
        
        float minFeatureSize = 0.0f;            ///< Smallest feature found (mm)
        int holeCount = 0;                      ///< Number of holes detected
        int nonManifoldEdges = 0;               ///< Number of non-manifold edges
        int nonManifoldVertices = 0;            ///< Number of non-manifold vertices
        int degenerateTriangles = 0;            ///< Number of degenerate triangles
        int flippedNormals = 0;                 ///< Number of incorrectly oriented faces
        
        std::vector<std::string> errors;        ///< List of error messages
        std::vector<std::string> warnings;      ///< List of warning messages
    };
    
    /**
     * @brief Mesh statistics
     */
    struct MeshStatistics {
        int vertexCount = 0;
        int triangleCount = 0;
        int edgeCount = 0;
        float surfaceArea = 0.0f;
        float volume = 0.0f;
        Math::Vector3f boundingBoxMin;
        Math::Vector3f boundingBoxMax;
        Math::Vector3f centerOfMass;
        int connectedComponents = 0;
        int genus = 0;  // Topological genus (holes)
    };

    MeshValidator();
    ~MeshValidator();
    
    /**
     * @brief Validate mesh for 3D printing
     * @param mesh The mesh to validate
     * @param minFeatureSize Minimum feature size in mm (default 1.0mm)
     * @return Validation results
     * @requirements REQ-10.1.11, REQ-10.1.14
     */
    ValidationResult validate(const Mesh& mesh, float minFeatureSize = 1.0f);
    
    /**
     * @brief Check if mesh is watertight (closed)
     * @param mesh The mesh to check
     * @return True if watertight
     * @requirements REQ-10.1.11
     */
    bool isWatertight(const Mesh& mesh);
    
    /**
     * @brief Check if mesh has manifold geometry
     * @param mesh The mesh to check
     * @return True if manifold
     * @requirements REQ-10.1.11
     */
    bool isManifold(const Mesh& mesh);
    
    /**
     * @brief Find holes in the mesh
     * @param mesh The mesh to analyze
     * @return List of boundary loops representing holes
     */
    std::vector<std::vector<uint32_t>> findHoles(const Mesh& mesh);
    
    /**
     * @brief Find non-manifold edges
     * @param mesh The mesh to analyze
     * @return List of non-manifold edge pairs
     */
    std::vector<std::pair<uint32_t, uint32_t>> findNonManifoldEdges(const Mesh& mesh);
    
    /**
     * @brief Find non-manifold vertices
     * @param mesh The mesh to analyze
     * @return List of non-manifold vertex indices
     */
    std::vector<uint32_t> findNonManifoldVertices(const Mesh& mesh);
    
    /**
     * @brief Calculate minimum feature size
     * @param mesh The mesh to analyze
     * @return Minimum feature size in mesh units
     * @requirements REQ-10.1.14
     */
    float calculateMinimumFeatureSize(const Mesh& mesh);
    
    /**
     * @brief Find degenerate triangles
     * @param mesh The mesh to analyze
     * @return List of degenerate triangle indices
     */
    std::vector<uint32_t> findDegenerateTriangles(const Mesh& mesh);
    
    /**
     * @brief Check face orientation consistency
     * @param mesh The mesh to check
     * @return Number of incorrectly oriented faces
     */
    int checkFaceOrientation(const Mesh& mesh);
    
    /**
     * @brief Detect self-intersections
     * @param mesh The mesh to check
     * @return True if self-intersections found
     */
    bool hasSelfIntersections(const Mesh& mesh);
    
    /**
     * @brief Calculate mesh statistics
     * @param mesh The mesh to analyze
     * @return Mesh statistics
     */
    MeshStatistics calculateStatistics(const Mesh& mesh);
    
    /**
     * @brief Simple mesh repair for common issues
     * @param mesh The mesh to repair (modified in place)
     * @return True if any repairs were made
     */
    bool repairBasicIssues(Mesh& mesh);
    
    /**
     * @brief Remove degenerate triangles
     * @param mesh The mesh to clean
     * @return Number of triangles removed
     */
    int removeDegenerateTriangles(Mesh& mesh);
    
    /**
     * @brief Fix face orientation to be consistent
     * @param mesh The mesh to fix
     * @return Number of faces flipped
     */
    int fixFaceOrientation(Mesh& mesh);

private:
    /**
     * @brief Edge structure for manifold checking
     */
    struct Edge {
        uint32_t v0, v1;
        std::vector<uint32_t> faces;
    };
    
    /**
     * @brief Build edge connectivity map
     */
    std::unordered_map<uint64_t, Edge> buildEdgeMap(const Mesh& mesh);
    
    /**
     * @brief Hash function for edges
     */
    static uint64_t edgeKey(uint32_t v0, uint32_t v1) {
        if (v0 > v1) std::swap(v0, v1);
        return (static_cast<uint64_t>(v0) << 32) | v1;
    }
    
    /**
     * @brief Calculate triangle area
     */
    float triangleArea(const Math::Vector3f& v0, const Math::Vector3f& v1, const Math::Vector3f& v2);
    
    /**
     * @brief Calculate signed volume contribution of a triangle
     */
    float signedVolumeOfTriangle(const Math::Vector3f& v0, const Math::Vector3f& v1, const Math::Vector3f& v2);
    
    /**
     * @brief Check if two triangles intersect
     */
    bool trianglesIntersect(const Math::Vector3f& t1v0, const Math::Vector3f& t1v1, const Math::Vector3f& t1v2,
                           const Math::Vector3f& t2v0, const Math::Vector3f& t2v1, const Math::Vector3f& t2v2);
};

} // namespace SurfaceGen
} // namespace VoxelEditor