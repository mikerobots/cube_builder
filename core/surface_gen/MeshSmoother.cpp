#include "MeshSmoother.h"
#include "TopologyPreserver.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace VoxelEditor {
namespace SurfaceGen {

MeshSmoother::MeshSmoother() : m_cancelled(false) {
}

MeshSmoother::~MeshSmoother() {
}

Mesh MeshSmoother::smooth(const Mesh& inputMesh, const SmoothingConfig& config, 
                          ProgressCallback progressCallback) {
    // Reset cancellation flag
    m_cancelled = false;
    
    // No smoothing requested
    if (config.smoothingLevel == 0) {
        return inputMesh;
    }
    
    // Make a copy to work on
    Mesh result = inputMesh;
    
    // Auto-select algorithm if not specified
    Algorithm algorithm = config.algorithm;
    if (algorithm == Algorithm::None && config.smoothingLevel > 0) {
        algorithm = getAlgorithmForLevel(config.smoothingLevel);
    }
    
    // Get iterations for the smoothing level
    int iterations = getIterationsForLevel(config.smoothingLevel, algorithm);
    
    // Apply preview quality optimizations based on level
    PreviewQuality previewQuality = config.previewQuality;
    if (config.usePreviewQuality && previewQuality == PreviewQuality::Disabled) {
        previewQuality = PreviewQuality::Balanced; // Backward compatibility
    }
    
    switch (previewQuality) {
        case PreviewQuality::Fast:
            iterations = std::max(1, iterations / 4); // Very aggressive reduction
            algorithm = Algorithm::Laplacian; // Force fastest algorithm
            break;
        case PreviewQuality::Balanced:
            iterations = std::max(1, iterations / 3); // Moderate reduction
            break;
        case PreviewQuality::HighQuality:
            iterations = std::max(1, iterations / 2); // Minimal reduction
            break;
        case PreviewQuality::Disabled:
        default:
            // No preview optimizations
            break;
    }
    
    // Analyze topology if preservation is requested
    TopologyPreserver::TopologyConstraints topologyConstraints;
    if (config.preserveTopology) {
        TopologyPreserver topologyPreserver;
        auto features = topologyPreserver.analyzeTopology(result);
        topologyConstraints = topologyPreserver.generateConstraints(result, features);
        // preserveBoundaries is handled separately in the smoothing algorithms
    }
    
    // Apply the selected smoothing algorithm
    bool success = true;
    switch (algorithm) {
        case Algorithm::Laplacian:
            success = applyLaplacianSmoothingWithTopology(result, iterations, 0.5f, 
                                            topologyConstraints, progressCallback);
            break;
            
        case Algorithm::Taubin:
            success = applyTaubinSmoothingWithTopology(result, iterations, 0.5f, -0.53f,
                                         topologyConstraints, progressCallback);
            break;
            
        case Algorithm::BiLaplacian:
            success = applyBiLaplacianSmoothingWithTopology(result, iterations,
                                              topologyConstraints, progressCallback);
            break;
            
        case Algorithm::None:
        default:
            // No smoothing
            break;
    }
    
    // Return empty mesh if cancelled
    if (!success || m_cancelled) {
        return Mesh();
    }
    
    // Enforce minimum feature size if specified
    if (config.minFeatureSize > 0) {
        enforceMinimumFeatureSize(result, config.minFeatureSize);
    }
    
    return result;
}

MeshSmoother::Algorithm MeshSmoother::getAlgorithmForLevel(int level) {
    if (level <= 0) return Algorithm::None;
    if (level <= 3) return Algorithm::Laplacian;
    if (level <= 7) return Algorithm::Taubin;
    return Algorithm::BiLaplacian;
}

int MeshSmoother::getIterationsForLevel(int level, Algorithm algorithm) {
    if (level <= 0) return 0;
    
    switch (algorithm) {
        case Algorithm::Laplacian:
            // Levels 1-3: 2, 4, 6 iterations
            return std::min(level, 3) * 2;
            
        case Algorithm::Taubin:
            // Levels 4-7: 3, 5, 7, 9 iterations
            return 1 + (std::min(std::max(level - 3, 1), 4) * 2);
            
        case Algorithm::BiLaplacian:
            // Levels 8-10+: 4, 6, 8+ iterations
            return 2 + (std::max(level - 7, 1) * 2);
            
        default:
            return 0;
    }
}

bool MeshSmoother::applyLaplacianSmoothingWithTopology(Mesh& mesh, int iterations, float lambda,
                                          const TopologyPreserver::TopologyConstraints& constraints,
                                          ProgressCallback progressCallback) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return true;
    }
    
    // Compute vertex neighbors
    auto neighbors = computeVertexNeighbors(mesh);
    
    // Create TopologyPreserver for constraint application
    TopologyPreserver topologyPreserver;
    
    // Temporary vertex storage for smoothing
    std::vector<Math::Vector3f> newVertices(mesh.vertices.size());
    std::vector<Math::Vector3f> originalVertices;
    originalVertices.reserve(mesh.vertices.size());
    for (const auto& vertex : mesh.vertices) {
        originalVertices.push_back(vertex.value());
    }
    
    // Perform smoothing iterations
    for (int iter = 0; iter < iterations; ++iter) {
        // Check for cancellation
        if (m_cancelled) {
            return false;
        }
        
        // Update progress
        if (progressCallback) {
            float progress = static_cast<float>(iter) / iterations;
            if (!progressCallback(progress)) {
                m_cancelled = true;
                return false;
            }
        }
        
        // Apply Laplacian smoothing to each vertex
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            // Calculate average of neighbors
            Math::Vector3f avgNeighbor(0.0f);
            const auto& vertexNeighbors = neighbors[i];
            
            if (vertexNeighbors.empty()) {
                newVertices[i] = mesh.vertices[i].value();
                continue;
            }
            
            for (uint32_t neighborIdx : vertexNeighbors) {
                avgNeighbor += mesh.vertices[neighborIdx].value();
            }
            avgNeighbor = avgNeighbor / static_cast<float>(vertexNeighbors.size());
            
            // Apply Laplacian smoothing
            Math::Vector3f currentPos = mesh.vertices[i].value();
            Math::Vector3f proposedPosition = currentPos + (avgNeighbor - currentPos) * lambda;
            
            // Apply topology constraints
            newVertices[i] = topologyPreserver.constrainMovement(i, originalVertices[i], 
                                                               proposedPosition, constraints);
        }
        
        // Update vertices
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            mesh.vertices[i] = Math::WorldCoordinates(newVertices[i]);
        }
    }
    
    // Final progress update
    if (progressCallback) {
        progressCallback(1.0f);
    }
    
    return true;
}

bool MeshSmoother::applyTaubinSmoothingWithTopology(Mesh& mesh, int iterations, float lambda, float mu,
                                       const TopologyPreserver::TopologyConstraints& constraints,
                                       ProgressCallback progressCallback) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return true;
    }
    
    // Compute vertex neighbors
    auto neighbors = computeVertexNeighbors(mesh);
    
    // Create TopologyPreserver for constraint application
    TopologyPreserver topologyPreserver;
    
    // Temporary vertex storage
    std::vector<Math::Vector3f> newVertices(mesh.vertices.size());
    std::vector<Math::Vector3f> originalVertices;
    originalVertices.reserve(mesh.vertices.size());
    for (const auto& vertex : mesh.vertices) {
        originalVertices.push_back(vertex.value());
    }
    
    // Taubin smoothing alternates between positive and negative smoothing factors
    for (int iter = 0; iter < iterations; ++iter) {
        // Check for cancellation
        if (m_cancelled) {
            return false;
        }
        
        // Update progress
        if (progressCallback) {
            float progress = static_cast<float>(iter) / iterations;
            if (!progressCallback(progress)) {
                m_cancelled = true;
                return false;
            }
        }
        
        // Use lambda for odd iterations, mu for even iterations
        float factor = (iter % 2 == 0) ? lambda : mu;
        
        // Apply smoothing to each vertex
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            // Calculate weighted average of neighbors
            Math::Vector3f laplacian(0.0f);
            const auto& vertexNeighbors = neighbors[i];
            
            if (vertexNeighbors.empty()) {
                newVertices[i] = mesh.vertices[i].value();
                continue;
            }
            
            Math::Vector3f currentPos = mesh.vertices[i].value();
            for (uint32_t neighborIdx : vertexNeighbors) {
                laplacian += mesh.vertices[neighborIdx].value() - currentPos;
            }
            laplacian = laplacian / static_cast<float>(vertexNeighbors.size());
            
            // Apply Taubin step
            Math::Vector3f proposedPosition = currentPos + laplacian * factor;
            
            // Apply topology constraints
            newVertices[i] = topologyPreserver.constrainMovement(i, originalVertices[i],
                                                               proposedPosition, constraints);
        }
        
        // Update vertices
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            mesh.vertices[i] = Math::WorldCoordinates(newVertices[i]);
        }
    }
    
    // Final progress update
    if (progressCallback) {
        progressCallback(1.0f);
    }
    
    return true;
}

bool MeshSmoother::applyBiLaplacianSmoothingWithTopology(Mesh& mesh, int iterations,
                                            const TopologyPreserver::TopologyConstraints& constraints,
                                            ProgressCallback progressCallback) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return true;
    }
    
    // BiLaplacian is essentially two passes of Laplacian per iteration
    // This creates a more aggressive smoothing effect
    for (int iter = 0; iter < iterations; ++iter) {
        // Check for cancellation
        if (m_cancelled) {
            return false;
        }
        
        // Update progress
        if (progressCallback) {
            float progress = static_cast<float>(iter) / iterations;
            if (!progressCallback(progress)) {
                m_cancelled = true;
                return false;
            }
        }
        
        // First Laplacian pass with topology constraints
        if (!applyLaplacianSmoothingWithTopology(mesh, 1, 0.5f, constraints, nullptr)) {
            return false;
        }
        
        // Second Laplacian pass with topology constraints
        if (!applyLaplacianSmoothingWithTopology(mesh, 1, 0.5f, constraints, nullptr)) {
            return false;
        }
    }
    
    // Final progress update
    if (progressCallback) {
        progressCallback(1.0f);
    }
    
    return true;
}

bool MeshSmoother::applyLaplacianSmoothing(Mesh& mesh, int iterations, float lambda,
                                          bool preserveBoundaries, ProgressCallback progressCallback) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return true;
    }
    
    // Compute vertex neighbors
    auto neighbors = computeVertexNeighbors(mesh);
    
    // Identify boundary vertices if needed
    std::unordered_set<uint32_t> boundaryVertices;
    if (preserveBoundaries) {
        boundaryVertices = identifyBoundaryVertices(mesh);
    }
    
    // Temporary vertex storage for smoothing
    std::vector<Math::Vector3f> newVertices(mesh.vertices.size());
    
    // Perform smoothing iterations
    for (int iter = 0; iter < iterations; ++iter) {
        // Check for cancellation
        if (m_cancelled) {
            return false;
        }
        
        // Update progress
        if (progressCallback) {
            float progress = static_cast<float>(iter) / iterations;
            if (!progressCallback(progress)) {
                m_cancelled = true;
                return false;
            }
        }
        
        // Apply Laplacian smoothing to each vertex
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            // Skip boundary vertices if preserving boundaries
            if (preserveBoundaries && boundaryVertices.count(i) > 0) {
                newVertices[i] = mesh.vertices[i].value();
                continue;
            }
            
            // Calculate average of neighbors
            Math::Vector3f avgNeighbor(0.0f);
            const auto& vertexNeighbors = neighbors[i];
            
            if (vertexNeighbors.empty()) {
                newVertices[i] = mesh.vertices[i].value();
                continue;
            }
            
            for (uint32_t neighborIdx : vertexNeighbors) {
                avgNeighbor += mesh.vertices[neighborIdx].value();
            }
            avgNeighbor = avgNeighbor / static_cast<float>(vertexNeighbors.size());
            
            // Apply Laplacian smoothing
            Math::Vector3f currentPos = mesh.vertices[i].value();
            newVertices[i] = currentPos + (avgNeighbor - currentPos) * lambda;
        }
        
        // Update vertices
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            mesh.vertices[i] = Math::WorldCoordinates(newVertices[i]);
        }
    }
    
    // Final progress update
    if (progressCallback) {
        progressCallback(1.0f);
    }
    
    return true;
}

std::vector<std::vector<uint32_t>> MeshSmoother::computeVertexNeighbors(const Mesh& mesh) {
    std::vector<std::vector<uint32_t>> neighbors(mesh.vertices.size());
    
    // Build adjacency list from triangle indices
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        uint32_t v0 = mesh.indices[i];
        uint32_t v1 = mesh.indices[i + 1];
        uint32_t v2 = mesh.indices[i + 2];
        
        // Add neighbors (avoiding duplicates)
        auto addNeighbor = [&neighbors](uint32_t vertex, uint32_t neighbor) {
            if (std::find(neighbors[vertex].begin(), neighbors[vertex].end(), neighbor) 
                == neighbors[vertex].end()) {
                neighbors[vertex].push_back(neighbor);
            }
        };
        
        addNeighbor(v0, v1);
        addNeighbor(v0, v2);
        addNeighbor(v1, v0);
        addNeighbor(v1, v2);
        addNeighbor(v2, v0);
        addNeighbor(v2, v1);
    }
    
    return neighbors;
}

std::unordered_set<uint32_t> MeshSmoother::identifyBoundaryVertices(const Mesh& mesh) {
    std::unordered_set<uint32_t> boundaryVertices;
    
    // Count edge occurrences - boundary edges appear only once
    struct Edge {
        uint32_t v0, v1;
        
        bool operator==(const Edge& other) const {
            return (v0 == other.v0 && v1 == other.v1) || 
                   (v0 == other.v1 && v1 == other.v0);
        }
    };
    
    struct EdgeHash {
        size_t operator()(const Edge& edge) const {
            uint32_t min_v = std::min(edge.v0, edge.v1);
            uint32_t max_v = std::max(edge.v0, edge.v1);
            return std::hash<uint64_t>()((static_cast<uint64_t>(min_v) << 32) | max_v);
        }
    };
    
    std::unordered_map<Edge, int, EdgeHash> edgeCount;
    
    // Count edge occurrences
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        uint32_t v0 = mesh.indices[i];
        uint32_t v1 = mesh.indices[i + 1];
        uint32_t v2 = mesh.indices[i + 2];
        
        edgeCount[{v0, v1}]++;
        edgeCount[{v1, v2}]++;
        edgeCount[{v2, v0}]++;
    }
    
    // Find boundary edges (appear only once)
    for (const auto& [edge, count] : edgeCount) {
        if (count == 1) {
            boundaryVertices.insert(edge.v0);
            boundaryVertices.insert(edge.v1);
        }
    }
    
    return boundaryVertices;
}

void MeshSmoother::enforceMinimumFeatureSize(Mesh& mesh, float minFeatureSize) {
    // TODO: Implement minimum feature size enforcement
    // This would involve detecting thin features and thickening them
    // For now, this is a placeholder for future implementation
}

} // namespace SurfaceGen
} // namespace VoxelEditor