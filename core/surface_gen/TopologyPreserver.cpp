#include "TopologyPreserver.h"
#include <queue>
#include <algorithm>
#include <cmath>

namespace VoxelEditor {
namespace SurfaceGen {

TopologyPreserver::TopologyPreserver() {
}

TopologyPreserver::~TopologyPreserver() {
}

std::vector<TopologyPreserver::TopologicalFeature> TopologyPreserver::analyzeTopology(const Mesh& mesh) {
    std::vector<TopologicalFeature> features;
    
    // Detect holes
    auto holes = detectHoles(mesh);
    features.insert(features.end(), holes.begin(), holes.end());
    
    // Detect loops/tunnels
    auto loops = detectLoops(mesh);
    features.insert(features.end(), loops.begin(), loops.end());
    
    // Calculate importance scores based on feature size
    for (auto& feature : features) {
        // Larger features are more important to preserve
        feature.importance = std::min(1.0f, feature.criticalVertices.size() / 20.0f);
    }
    
    return features;
}

TopologyPreserver::TopologyConstraints TopologyPreserver::generateConstraints(
    const Mesh& mesh, const std::vector<TopologicalFeature>& features) {
    
    TopologyConstraints constraints;
    
    for (const auto& feature : features) {
        // Based on feature type and importance, add constraints
        switch (feature.type) {
            case TopologicalFeature::HOLE:
                if (constraints.preserveHoles) {
                    // Lock vertices that define the hole boundary
                    for (uint32_t vertex : feature.criticalVertices) {
                        if (feature.importance > 0.8f) {
                            constraints.lockedVertices.insert(vertex);
                        } else {
                            constraints.constrainedVertices.insert(vertex);
                        }
                    }
                }
                break;
                
            case TopologicalFeature::LOOP:
                if (constraints.preserveLoops) {
                    // Constrain vertices that form the loop
                    for (uint32_t vertex : feature.criticalVertices) {
                        constraints.constrainedVertices.insert(vertex);
                    }
                }
                break;
                
            case TopologicalFeature::BOUNDARY:
                // Always preserve boundaries
                for (uint32_t vertex : feature.criticalVertices) {
                    constraints.lockedVertices.insert(vertex);
                }
                break;
                
            default:
                break;
        }
    }
    
    return constraints;
}

bool TopologyPreserver::isMovementAllowed(uint32_t vertexIndex, 
                                         const Math::Vector3f& oldPosition,
                                         const Math::Vector3f& newPosition,
                                         const TopologyConstraints& constraints) {
    // Check if vertex is locked
    if (constraints.lockedVertices.count(vertexIndex) > 0) {
        return false;
    }
    
    // Check if vertex is constrained
    if (constraints.constrainedVertices.count(vertexIndex) > 0) {
        float distance = (newPosition - oldPosition).length();
        if (distance > constraints.maxMovementDistance) {
            return false;
        }
    }
    
    return true;
}

Math::Vector3f TopologyPreserver::constrainMovement(uint32_t vertexIndex,
                                              const Math::Vector3f& oldPosition,
                                              const Math::Vector3f& proposedPosition,
                                              const TopologyConstraints& constraints) {
    // If locked, return original position
    if (constraints.lockedVertices.count(vertexIndex) > 0) {
        return oldPosition;
    }
    
    // If constrained, limit movement
    if (constraints.constrainedVertices.count(vertexIndex) > 0) {
        Math::Vector3f delta = proposedPosition - oldPosition;
        float distance = delta.length();
        
        if (distance > constraints.maxMovementDistance) {
            // Scale down movement to stay within constraint
            delta = delta.normalized() * constraints.maxMovementDistance;
            return oldPosition + delta;
        }
    }
    
    // No constraints, allow full movement
    return proposedPosition;
}

std::vector<TopologyPreserver::TopologicalFeature> TopologyPreserver::detectHoles(const Mesh& mesh) {
    std::vector<TopologicalFeature> holes;
    
    // Build edge map
    auto edgeMap = buildEdgeMap(mesh);
    
    // Find boundary edges
    auto boundaryEdges = findBoundaryEdges(edgeMap);
    
    if (boundaryEdges.empty()) {
        return holes; // No holes in a closed mesh
    }
    
    // Trace boundary loops
    auto boundaryLoops = traceBoundaryLoops(boundaryEdges);
    
    // Each boundary loop represents a hole
    for (const auto& loop : boundaryLoops) {
        TopologicalFeature hole;
        hole.type = TopologicalFeature::HOLE;
        hole.criticalVertices = loop;
        
        // Add edges
        for (size_t i = 0; i < loop.size(); ++i) {
            size_t next = (i + 1) % loop.size();
            hole.criticalEdges.push_back({loop[i], loop[next]});
        }
        
        holes.push_back(hole);
    }
    
    return holes;
}

std::vector<TopologyPreserver::TopologicalFeature> TopologyPreserver::detectLoops(const Mesh& mesh) {
    std::vector<TopologicalFeature> loops;
    
    // Calculate mesh genus
    int genus = calculateGenus(mesh);
    
    // For now, we'll mark the entire mesh as having loop features if genus > 0
    // A more sophisticated implementation would identify specific loop regions
    if (genus > 0) {
        TopologicalFeature loop;
        loop.type = TopologicalFeature::LOOP;
        
        // Add all vertices as potentially critical for loop preservation
        // In a real implementation, we'd use more sophisticated analysis
        for (uint32_t i = 0; i < mesh.vertices.size(); ++i) {
            loop.criticalVertices.push_back(i);
        }
        
        loop.importance = 1.0f; // High importance for genus preservation
        loops.push_back(loop);
    }
    
    return loops;
}

int TopologyPreserver::calculateGenus(const Mesh& mesh) {
    // Use Euler characteristic: V - E + F = 2 - 2g
    // where g is the genus
    
    int V = mesh.vertices.size();
    int F = mesh.indices.size() / 3; // Number of triangles
    
    // Count unique edges
    std::unordered_set<uint64_t> uniqueEdges;
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        uint32_t v0 = mesh.indices[i];
        uint32_t v1 = mesh.indices[i + 1];
        uint32_t v2 = mesh.indices[i + 2];
        
        uniqueEdges.insert(edgeKey(v0, v1));
        uniqueEdges.insert(edgeKey(v1, v2));
        uniqueEdges.insert(edgeKey(v2, v0));
    }
    int E = uniqueEdges.size();
    
    int eulerChar = V - E + F;
    int genus = (2 - eulerChar) / 2;
    
    return std::max(0, genus);
}

bool TopologyPreserver::verifyTopologyPreserved(const Mesh& originalMesh, const Mesh& smoothedMesh) {
    // Basic check: same number of vertices and faces
    if (originalMesh.vertices.size() != smoothedMesh.vertices.size() ||
        originalMesh.indices.size() != smoothedMesh.indices.size()) {
        return false;
    }
    
    // Check genus is preserved
    int originalGenus = calculateGenus(originalMesh);
    int smoothedGenus = calculateGenus(smoothedMesh);
    
    return originalGenus == smoothedGenus;
}

std::unordered_map<uint64_t, TopologyPreserver::EdgeInfo> TopologyPreserver::buildEdgeMap(const Mesh& mesh) {
    std::unordered_map<uint64_t, EdgeInfo> edgeMap;
    
    for (size_t faceIdx = 0; faceIdx < mesh.indices.size(); faceIdx += 3) {
        uint32_t v0 = mesh.indices[faceIdx];
        uint32_t v1 = mesh.indices[faceIdx + 1];
        uint32_t v2 = mesh.indices[faceIdx + 2];
        uint32_t faceId = faceIdx / 3;
        
        // Add three edges of the triangle
        auto addEdge = [&edgeMap, faceId](uint32_t a, uint32_t b) {
            uint64_t key = edgeKey(a, b);
            if (edgeMap.find(key) == edgeMap.end()) {
                edgeMap[key] = EdgeInfo{std::min(a, b), std::max(a, b), {}};
            }
            edgeMap[key].faces.push_back(faceId);
        };
        
        addEdge(v0, v1);
        addEdge(v1, v2);
        addEdge(v2, v0);
    }
    
    return edgeMap;
}

std::vector<std::pair<uint32_t, uint32_t>> TopologyPreserver::findBoundaryEdges(
    const std::unordered_map<uint64_t, EdgeInfo>& edgeMap) {
    
    std::vector<std::pair<uint32_t, uint32_t>> boundaryEdges;
    
    for (const auto& [key, edge] : edgeMap) {
        // Boundary edges have exactly one adjacent face
        if (edge.faces.size() == 1) {
            boundaryEdges.push_back({edge.v0, edge.v1});
        }
    }
    
    return boundaryEdges;
}

std::vector<std::vector<uint32_t>> TopologyPreserver::traceBoundaryLoops(
    const std::vector<std::pair<uint32_t, uint32_t>>& boundaryEdges) {
    
    std::vector<std::vector<uint32_t>> loops;
    
    if (boundaryEdges.empty()) {
        return loops;
    }
    
    // Build adjacency for boundary vertices
    std::unordered_map<uint32_t, std::vector<uint32_t>> adjacency;
    for (const auto& edge : boundaryEdges) {
        adjacency[edge.first].push_back(edge.second);
        adjacency[edge.second].push_back(edge.first);
    }
    
    // Track visited vertices
    std::unordered_set<uint32_t> visited;
    
    // Trace each connected boundary loop
    for (const auto& edge : boundaryEdges) {
        if (visited.count(edge.first) > 0) {
            continue;
        }
        
        std::vector<uint32_t> loop;
        uint32_t current = edge.first;
        uint32_t previous = UINT32_MAX;
        
        // Follow the boundary
        while (true) {
            loop.push_back(current);
            visited.insert(current);
            
            // Find next vertex
            uint32_t next = UINT32_MAX;
            for (uint32_t neighbor : adjacency[current]) {
                if (neighbor != previous) {
                    next = neighbor;
                    break;
                }
            }
            
            if (next == UINT32_MAX || next == edge.first) {
                break; // Loop complete or dead end
            }
            
            previous = current;
            current = next;
        }
        
        if (loop.size() > 2) {
            loops.push_back(loop);
        }
    }
    
    return loops;
}

int TopologyPreserver::calculateEulerCharacteristic(const Mesh& mesh) {
    int V = mesh.vertices.size();
    int F = mesh.indices.size() / 3;
    
    // Count edges
    std::unordered_set<uint64_t> uniqueEdges;
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        uniqueEdges.insert(edgeKey(mesh.indices[i], mesh.indices[i + 1]));
        uniqueEdges.insert(edgeKey(mesh.indices[i + 1], mesh.indices[i + 2]));
        uniqueEdges.insert(edgeKey(mesh.indices[i + 2], mesh.indices[i]));
    }
    int E = uniqueEdges.size();
    
    return V - E + F;
}

} // namespace SurfaceGen
} // namespace VoxelEditor