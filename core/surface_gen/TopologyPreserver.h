#pragma once

#include "SurfaceTypes.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace VoxelEditor {
namespace SurfaceGen {

/**
 * @brief Analyzes and preserves mesh topology during smoothing operations
 * 
 * This class ensures that topological features like loops, holes, and complex
 * geometry are maintained during mesh smoothing operations. It identifies
 * critical vertices and edges that must be preserved to maintain topology.
 * 
 * @requirements REQ-10.1.9
 */
class TopologyPreserver {
public:
    /**
     * @brief Represents a topological feature in the mesh
     */
    struct TopologicalFeature {
        enum Type {
            HOLE,           ///< A hole in the mesh
            LOOP,           ///< A loop/tunnel through the mesh
            HANDLE,         ///< A handle-like protrusion
            BOUNDARY,       ///< Mesh boundary
            BRIDGE          ///< A thin bridge connecting parts
        };
        
        Type type;
        std::vector<uint32_t> criticalVertices;  ///< Vertices that define this feature
        std::vector<std::pair<uint32_t, uint32_t>> criticalEdges; ///< Edges that define this feature
        float importance;  ///< Importance score (0-1) for preservation priority
    };
    
    /**
     * @brief Constraints for preserving topology during smoothing
     */
    struct TopologyConstraints {
        std::unordered_set<uint32_t> lockedVertices;     ///< Vertices that cannot move
        std::unordered_set<uint32_t> constrainedVertices; ///< Vertices with limited movement
        float maxMovementDistance = 0.1f;                 ///< Max movement for constrained vertices
        bool preserveHoles = true;                        ///< Preserve holes in the mesh
        bool preserveLoops = true;                        ///< Preserve loops/tunnels
        bool preserveHandles = true;                      ///< Preserve handle-like features
    };

    TopologyPreserver();
    ~TopologyPreserver();
    
    /**
     * @brief Analyze mesh topology and identify features to preserve
     * @param mesh The mesh to analyze
     * @return List of topological features found
     * @requirements REQ-10.1.9
     */
    std::vector<TopologicalFeature> analyzeTopology(const Mesh& mesh);
    
    /**
     * @brief Generate constraints for topology preservation
     * @param mesh The mesh being smoothed
     * @param features Topological features to preserve
     * @return Constraints for smoothing operations
     */
    TopologyConstraints generateConstraints(const Mesh& mesh, 
                                          const std::vector<TopologicalFeature>& features);
    
    /**
     * @brief Check if a vertex movement would violate topology
     * @param vertexIndex Index of the vertex
     * @param oldPosition Original vertex position
     * @param newPosition Proposed new position
     * @param constraints Active topology constraints
     * @return True if movement is allowed, false if it would break topology
     */
    bool isMovementAllowed(uint32_t vertexIndex, 
                          const Math::Vector3f& oldPosition,
                          const Math::Vector3f& newPosition,
                          const TopologyConstraints& constraints);
    
    /**
     * @brief Adjust vertex position to maintain topology
     * @param vertexIndex Index of the vertex
     * @param oldPosition Original vertex position
     * @param proposedPosition Proposed new position from smoothing
     * @param constraints Active topology constraints
     * @return Adjusted position that maintains topology
     */
    Math::Vector3f constrainMovement(uint32_t vertexIndex,
                               const Math::Vector3f& oldPosition,
                               const Math::Vector3f& proposedPosition,
                               const TopologyConstraints& constraints);
    
    /**
     * @brief Detect holes in the mesh
     * @param mesh The mesh to analyze
     * @return List of hole features with their boundary vertices
     */
    std::vector<TopologicalFeature> detectHoles(const Mesh& mesh);
    
    /**
     * @brief Detect loops/tunnels in the mesh
     * @param mesh The mesh to analyze
     * @return List of loop features
     */
    std::vector<TopologicalFeature> detectLoops(const Mesh& mesh);
    
    /**
     * @brief Calculate genus (number of holes) of the mesh
     * @param mesh The mesh to analyze
     * @return Genus of the mesh (0 = sphere-like, 1 = torus-like, etc.)
     */
    int calculateGenus(const Mesh& mesh);
    
    /**
     * @brief Verify topology is preserved after smoothing
     * @param originalMesh The original mesh
     * @param smoothedMesh The smoothed mesh
     * @return True if topology is preserved
     */
    bool verifyTopologyPreserved(const Mesh& originalMesh, const Mesh& smoothedMesh);

private:
    /**
     * @brief Build edge connectivity information
     */
    struct EdgeInfo {
        uint32_t v0, v1;
        std::vector<uint32_t> faces; // Faces that share this edge
    };
    
    /**
     * @brief Build edge map for topology analysis
     */
    std::unordered_map<uint64_t, EdgeInfo> buildEdgeMap(const Mesh& mesh);
    
    /**
     * @brief Find boundary edges (edges with only one adjacent face)
     */
    std::vector<std::pair<uint32_t, uint32_t>> findBoundaryEdges(
        const std::unordered_map<uint64_t, EdgeInfo>& edgeMap);
    
    /**
     * @brief Trace boundary loops from boundary edges
     */
    std::vector<std::vector<uint32_t>> traceBoundaryLoops(
        const std::vector<std::pair<uint32_t, uint32_t>>& boundaryEdges);
    
    /**
     * @brief Calculate Euler characteristic for topology
     */
    int calculateEulerCharacteristic(const Mesh& mesh);
    
    /**
     * @brief Hash function for edge keys
     */
    static uint64_t edgeKey(uint32_t v0, uint32_t v1) {
        if (v0 > v1) std::swap(v0, v1);
        return (static_cast<uint64_t>(v0) << 32) | v1;
    }
};

} // namespace SurfaceGen
} // namespace VoxelEditor