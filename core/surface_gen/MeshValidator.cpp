#include "MeshValidator.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_set>

namespace VoxelEditor {
namespace SurfaceGen {

MeshValidator::MeshValidator() {
}

MeshValidator::~MeshValidator() {
}

MeshValidator::ValidationResult MeshValidator::validate(const Mesh& mesh, float minFeatureSize) {
    ValidationResult result;
    
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        result.isValid = false;
        result.errors.push_back("Empty mesh");
        return result;
    }
    
    // Check watertight
    result.isWatertight = isWatertight(mesh);
    if (!result.isWatertight) {
        auto holes = findHoles(mesh);
        result.holeCount = holes.size();
        result.errors.push_back("Mesh is not watertight - found " + std::to_string(result.holeCount) + " holes");
    }
    
    // Check manifold
    result.isManifold = isManifold(mesh);
    if (!result.isManifold) {
        auto nonManifoldEdges = findNonManifoldEdges(mesh);
        auto nonManifoldVerts = findNonManifoldVertices(mesh);
        result.nonManifoldEdges = nonManifoldEdges.size();
        result.nonManifoldVertices = nonManifoldVerts.size();
        result.errors.push_back("Mesh has non-manifold geometry");
    }
    
    // Check minimum feature size
    float actualMinFeature = calculateMinimumFeatureSize(mesh);
    result.minFeatureSize = actualMinFeature;
    result.hasMinimumFeatureSize = actualMinFeature >= minFeatureSize;
    if (!result.hasMinimumFeatureSize) {
        result.warnings.push_back("Minimum feature size (" + std::to_string(actualMinFeature) + 
                                "mm) is below threshold (" + std::to_string(minFeatureSize) + "mm)");
    }
    
    // Check degenerate triangles
    auto degenerates = findDegenerateTriangles(mesh);
    result.degenerateTriangles = degenerates.size();
    if (result.degenerateTriangles > 0) {
        result.warnings.push_back("Found " + std::to_string(result.degenerateTriangles) + " degenerate triangles");
    }
    
    // Check face orientation
    result.flippedNormals = checkFaceOrientation(mesh);
    result.hasCorrectOrientation = (result.flippedNormals == 0);
    if (result.flippedNormals > 0) {
        result.warnings.push_back("Found " + std::to_string(result.flippedNormals) + " incorrectly oriented faces");
    }
    
    // Check self-intersections
    result.hasSelfIntersections = hasSelfIntersections(mesh);
    if (result.hasSelfIntersections) {
        result.errors.push_back("Mesh has self-intersections");
    }
    
    // Determine overall validity
    result.isValid = result.isWatertight && 
                    result.isManifold && 
                    !result.hasSelfIntersections &&
                    result.degenerateTriangles == 0;
    
    return result;
}

bool MeshValidator::isWatertight(const Mesh& mesh) {
    auto edgeMap = buildEdgeMap(mesh);
    
    // A watertight mesh has all edges shared by exactly 2 faces
    for (const auto& [key, edge] : edgeMap) {
        if (edge.faces.size() != 2) {
            return false;
        }
    }
    
    return true;
}

bool MeshValidator::isManifold(const Mesh& mesh) {
    auto edgeMap = buildEdgeMap(mesh);
    
    // Check edge manifoldness
    for (const auto& [key, edge] : edgeMap) {
        if (edge.faces.size() > 2) {
            return false; // Non-manifold edge
        }
    }
    
    // Check vertex manifoldness
    // A manifold vertex has a single connected fan of triangles
    for (uint32_t v = 0; v < mesh.vertices.size(); ++v) {
        std::unordered_set<uint32_t> adjacentFaces;
        
        // Find all faces containing this vertex
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            if (mesh.indices[i] == v || mesh.indices[i+1] == v || mesh.indices[i+2] == v) {
                adjacentFaces.insert(i / 3);
            }
        }
        
        if (adjacentFaces.empty()) continue;
        
        // Check if faces form a single connected component
        // This is a simplified check - a full implementation would verify the fan structure
        // For now, we'll assume it's manifold if we get here
    }
    
    return true;
}

std::vector<std::vector<uint32_t>> MeshValidator::findHoles(const Mesh& mesh) {
    std::vector<std::vector<uint32_t>> holes;
    auto edgeMap = buildEdgeMap(mesh);
    
    // Find boundary edges (edges with only 1 face)
    std::vector<std::pair<uint32_t, uint32_t>> boundaryEdges;
    for (const auto& [key, edge] : edgeMap) {
        if (edge.faces.size() == 1) {
            boundaryEdges.push_back({edge.v0, edge.v1});
        }
    }
    
    if (boundaryEdges.empty()) {
        return holes; // No holes
    }
    
    // Build adjacency for boundary vertices
    std::unordered_map<uint32_t, std::vector<uint32_t>> adjacency;
    for (const auto& edge : boundaryEdges) {
        adjacency[edge.first].push_back(edge.second);
        adjacency[edge.second].push_back(edge.first);
    }
    
    // Trace boundary loops
    std::unordered_set<uint32_t> visited;
    for (const auto& edge : boundaryEdges) {
        if (visited.count(edge.first) > 0) continue;
        
        std::vector<uint32_t> loop;
        uint32_t current = edge.first;
        uint32_t previous = UINT32_MAX;
        
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
            
            if (next == UINT32_MAX || next == edge.first || visited.count(next) > 0) {
                break;
            }
            
            previous = current;
            current = next;
        }
        
        if (loop.size() > 2) {
            holes.push_back(loop);
        }
    }
    
    return holes;
}

std::vector<std::pair<uint32_t, uint32_t>> MeshValidator::findNonManifoldEdges(const Mesh& mesh) {
    std::vector<std::pair<uint32_t, uint32_t>> nonManifoldEdges;
    auto edgeMap = buildEdgeMap(mesh);
    
    for (const auto& [key, edge] : edgeMap) {
        if (edge.faces.size() > 2) {
            nonManifoldEdges.push_back({edge.v0, edge.v1});
        }
    }
    
    return nonManifoldEdges;
}

std::vector<uint32_t> MeshValidator::findNonManifoldVertices(const Mesh& mesh) {
    std::vector<uint32_t> nonManifoldVertices;
    
    // This is a simplified implementation
    // A proper implementation would check for vertices where the adjacent faces
    // don't form a single connected fan
    
    return nonManifoldVertices;
}

float MeshValidator::calculateMinimumFeatureSize(const Mesh& mesh) {
    float minFeatureSize = std::numeric_limits<float>::max();
    
    // Check edge lengths
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        Math::Vector3f v0 = mesh.vertices[mesh.indices[i]].value();
        Math::Vector3f v1 = mesh.vertices[mesh.indices[i + 1]].value();
        Math::Vector3f v2 = mesh.vertices[mesh.indices[i + 2]].value();
        
        float edge1 = (v1 - v0).length();
        float edge2 = (v2 - v1).length();
        float edge3 = (v0 - v2).length();
        
        minFeatureSize = std::min(minFeatureSize, std::min({edge1, edge2, edge3}));
    }
    
    // Could also check for thin triangles, small angles, etc.
    
    return minFeatureSize;
}

std::vector<uint32_t> MeshValidator::findDegenerateTriangles(const Mesh& mesh) {
    std::vector<uint32_t> degenerates;
    const float epsilon = 1e-6f;
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        Math::Vector3f v0 = mesh.vertices[mesh.indices[i]].value();
        Math::Vector3f v1 = mesh.vertices[mesh.indices[i + 1]].value();
        Math::Vector3f v2 = mesh.vertices[mesh.indices[i + 2]].value();
        
        // Check for zero area
        float area = triangleArea(v0, v1, v2);
        if (area < epsilon) {
            degenerates.push_back(i / 3);
        }
    }
    
    return degenerates;
}

int MeshValidator::checkFaceOrientation(const Mesh& mesh) {
    // Check consistency of face orientations
    // For a properly oriented mesh, adjacent faces should have consistent winding
    
    auto edgeMap = buildEdgeMap(mesh);
    int flippedCount = 0;
    
    // Build face adjacency
    std::unordered_map<uint32_t, std::vector<uint32_t>> faceAdjacency;
    for (const auto& [key, edge] : edgeMap) {
        if (edge.faces.size() == 2) {
            faceAdjacency[edge.faces[0]].push_back(edge.faces[1]);
            faceAdjacency[edge.faces[1]].push_back(edge.faces[0]);
        }
    }
    
    // Track which faces we've checked
    std::unordered_set<uint32_t> checked;
    std::queue<uint32_t> toCheck;
    
    // Start with face 0 as reference
    if (!mesh.indices.empty()) {
        toCheck.push(0);
        checked.insert(0);
    }
    
    // Propagate orientation check through mesh
    while (!toCheck.empty()) {
        uint32_t currentFace = toCheck.front();
        toCheck.pop();
        
        // Get vertices of current face
        uint32_t cv0 = mesh.indices[currentFace * 3];
        uint32_t cv1 = mesh.indices[currentFace * 3 + 1];
        uint32_t cv2 = mesh.indices[currentFace * 3 + 2];
        
        // Check adjacent faces
        for (uint32_t adjacentFace : faceAdjacency[currentFace]) {
            if (checked.count(adjacentFace) > 0) continue;
            checked.insert(adjacentFace);
            
            // Get vertices of adjacent face
            uint32_t av0 = mesh.indices[adjacentFace * 3];
            uint32_t av1 = mesh.indices[adjacentFace * 3 + 1];
            uint32_t av2 = mesh.indices[adjacentFace * 3 + 2];
            
            // Find shared edge
            int sharedCount = 0;
            if (cv0 == av0 || cv0 == av1 || cv0 == av2) sharedCount++;
            if (cv1 == av0 || cv1 == av1 || cv1 == av2) sharedCount++;
            if (cv2 == av0 || cv2 == av1 || cv2 == av2) sharedCount++;
            
            // If they share an edge (2 vertices), check orientation consistency
            if (sharedCount == 2) {
                // Find the shared edge vertices in both triangles
                bool sameOrder = false;
                
                // Check all edge combinations
                bool foundSharedEdge = false;
                for (int i = 0; i < 3 && !foundSharedEdge; i++) {
                    uint32_t ce0 = mesh.indices[currentFace * 3 + i];
                    uint32_t ce1 = mesh.indices[currentFace * 3 + ((i + 1) % 3)];
                    
                    for (int j = 0; j < 3; j++) {
                        uint32_t ae0 = mesh.indices[adjacentFace * 3 + j];
                        uint32_t ae1 = mesh.indices[adjacentFace * 3 + ((j + 1) % 3)];
                        
                        if ((ce0 == ae0 && ce1 == ae1) || (ce0 == ae1 && ce1 == ae0)) {
                            // Found shared edge
                            sameOrder = (ce0 == ae0 && ce1 == ae1);
                            foundSharedEdge = true;
                            break;
                        }
                    }
                }
                
                // Adjacent faces should traverse shared edge in opposite directions
                // If they traverse in the same direction, one is flipped
                if (foundSharedEdge && sameOrder) {
                    flippedCount++;
                }
            }
            
            toCheck.push(adjacentFace);
        }
    }
    
    // For unconnected components, check volume
    float totalVolume = 0.0f;
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        Math::Vector3f v0 = mesh.vertices[mesh.indices[i]].value();
        Math::Vector3f v1 = mesh.vertices[mesh.indices[i + 1]].value();
        Math::Vector3f v2 = mesh.vertices[mesh.indices[i + 2]].value();
        
        totalVolume += signedVolumeOfTriangle(v0, v1, v2);
    }
    
    // Don't count flips if the entire mesh is consistently oriented
    // (even if inside-out). Only count actual inconsistencies.
    if (flippedCount == 0) {
        return 0;  // All faces are consistent
    }
    
    return flippedCount;
}

bool MeshValidator::hasSelfIntersections(const Mesh& mesh) {
    // This is an expensive O(n²) check
    // For production, use spatial acceleration structures
    
    const size_t maxChecks = 1000; // Limit checks for performance
    size_t checks = 0;
    
    for (size_t i = 0; i < mesh.indices.size() && checks < maxChecks; i += 3) {
        Math::Vector3f t1v0 = mesh.vertices[mesh.indices[i]].value();
        Math::Vector3f t1v1 = mesh.vertices[mesh.indices[i + 1]].value();
        Math::Vector3f t1v2 = mesh.vertices[mesh.indices[i + 2]].value();
        
        for (size_t j = i + 3; j < mesh.indices.size() && checks < maxChecks; j += 3) {
            // Skip triangles that share vertices
            bool shareVertex = false;
            for (int k = 0; k < 3; k++) {
                for (int l = 0; l < 3; l++) {
                    if (mesh.indices[i + k] == mesh.indices[j + l]) {
                        shareVertex = true;
                        break;
                    }
                }
                if (shareVertex) break;
            }
            
            if (!shareVertex) {
                Math::Vector3f t2v0 = mesh.vertices[mesh.indices[j]].value();
                Math::Vector3f t2v1 = mesh.vertices[mesh.indices[j + 1]].value();
                Math::Vector3f t2v2 = mesh.vertices[mesh.indices[j + 2]].value();
                
                if (trianglesIntersect(t1v0, t1v1, t1v2, t2v0, t2v1, t2v2)) {
                    return true;
                }
                
                checks++;
            }
        }
    }
    
    return false;
}

MeshValidator::MeshStatistics MeshValidator::calculateStatistics(const Mesh& mesh) {
    MeshStatistics stats;
    
    stats.vertexCount = mesh.vertices.size();
    stats.triangleCount = mesh.indices.size() / 3;
    
    if (mesh.vertices.empty()) {
        return stats;
    }
    
    // Calculate bounding box
    stats.boundingBoxMin = mesh.vertices[0].value();
    stats.boundingBoxMax = mesh.vertices[0].value();
    
    for (const auto& vertex : mesh.vertices) {
        stats.boundingBoxMin.x = std::min(stats.boundingBoxMin.x, vertex.x());
        stats.boundingBoxMin.y = std::min(stats.boundingBoxMin.y, vertex.y());
        stats.boundingBoxMin.z = std::min(stats.boundingBoxMin.z, vertex.z());
        stats.boundingBoxMax.x = std::max(stats.boundingBoxMax.x, vertex.x());
        stats.boundingBoxMax.y = std::max(stats.boundingBoxMax.y, vertex.y());
        stats.boundingBoxMax.z = std::max(stats.boundingBoxMax.z, vertex.z());
    }
    
    // Calculate surface area and volume
    Math::Vector3f centerOfMass(0.0f);
    float totalVolume = 0.0f;
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        Math::Vector3f v0 = mesh.vertices[mesh.indices[i]].value();
        Math::Vector3f v1 = mesh.vertices[mesh.indices[i + 1]].value();
        Math::Vector3f v2 = mesh.vertices[mesh.indices[i + 2]].value();
        
        stats.surfaceArea += triangleArea(v0, v1, v2);
        
        float vol = signedVolumeOfTriangle(v0, v1, v2);
        totalVolume += vol;
        centerOfMass += (v0 + v1 + v2) * vol;
    }
    
    stats.volume = std::abs(totalVolume);
    if (stats.volume > 0) {
        stats.centerOfMass = centerOfMass / (4.0f * totalVolume);
    }
    
    // Count edges
    auto edgeMap = buildEdgeMap(mesh);
    stats.edgeCount = edgeMap.size();
    
    // Calculate genus using Euler characteristic
    // V - E + F = 2 - 2g for closed surfaces
    stats.genus = (2 - (stats.vertexCount - stats.edgeCount + stats.triangleCount)) / 2;
    
    return stats;
}

bool MeshValidator::repairBasicIssues(Mesh& mesh) {
    bool repaired = false;
    
    // Remove degenerate triangles
    int removed = removeDegenerateTriangles(mesh);
    if (removed > 0) {
        repaired = true;
    }
    
    // Fix face orientation
    int flipped = fixFaceOrientation(mesh);
    if (flipped > 0) {
        repaired = true;
    }
    
    return repaired;
}

int MeshValidator::removeDegenerateTriangles(Mesh& mesh) {
    auto degenerates = findDegenerateTriangles(mesh);
    
    if (degenerates.empty()) {
        return 0;
    }
    
    // Sort in reverse order to remove from back to front
    std::sort(degenerates.rbegin(), degenerates.rend());
    
    // Remove degenerate triangles
    for (uint32_t triIdx : degenerates) {
        mesh.indices.erase(mesh.indices.begin() + triIdx * 3,
                          mesh.indices.begin() + triIdx * 3 + 3);
    }
    
    return degenerates.size();
}

int MeshValidator::fixFaceOrientation(Mesh& mesh) {
    // Calculate total volume before any fixes
    float totalVolume = 0.0f;
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        Math::Vector3f v0 = mesh.vertices[mesh.indices[i]].value();
        Math::Vector3f v1 = mesh.vertices[mesh.indices[i + 1]].value();
        Math::Vector3f v2 = mesh.vertices[mesh.indices[i + 2]].value();
        
        totalVolume += signedVolumeOfTriangle(v0, v1, v2);
    }
    
    // If volume is negative, the mesh is inside-out - flip all faces
    if (totalVolume < 0) {
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            std::swap(mesh.indices[i + 1], mesh.indices[i + 2]);
        }
        return mesh.indices.size() / 3;  // Return number of faces fixed
    }
    
    // If volume is already positive, check for inconsistent orientations
    int inconsistentFaces = checkFaceOrientation(mesh);
    
    // For now, we don't fix individual inconsistent faces
    // A full implementation would identify and flip only the problematic faces
    
    return 0;  // No fixes applied
}

std::unordered_map<uint64_t, MeshValidator::Edge> MeshValidator::buildEdgeMap(const Mesh& mesh) {
    std::unordered_map<uint64_t, Edge> edgeMap;
    
    for (size_t faceIdx = 0; faceIdx < mesh.indices.size(); faceIdx += 3) {
        uint32_t v0 = mesh.indices[faceIdx];
        uint32_t v1 = mesh.indices[faceIdx + 1];
        uint32_t v2 = mesh.indices[faceIdx + 2];
        uint32_t faceId = faceIdx / 3;
        
        auto addEdge = [&edgeMap, faceId](uint32_t a, uint32_t b) {
            uint64_t key = edgeKey(a, b);
            if (edgeMap.find(key) == edgeMap.end()) {
                edgeMap[key] = Edge{std::min(a, b), std::max(a, b), {}};
            }
            edgeMap[key].faces.push_back(faceId);
        };
        
        addEdge(v0, v1);
        addEdge(v1, v2);
        addEdge(v2, v0);
    }
    
    return edgeMap;
}

float MeshValidator::triangleArea(const Math::Vector3f& v0, const Math::Vector3f& v1, const Math::Vector3f& v2) {
    Math::Vector3f cross = (v1 - v0).cross(v2 - v0);
    return 0.5f * cross.length();
}

float MeshValidator::signedVolumeOfTriangle(const Math::Vector3f& v0, const Math::Vector3f& v1, const Math::Vector3f& v2) {
    // Volume contribution of a triangle with respect to origin
    return v0.dot(v1.cross(v2)) / 6.0f;
}

bool MeshValidator::trianglesIntersect(const Math::Vector3f& t1v0, const Math::Vector3f& t1v1, const Math::Vector3f& t1v2,
                                      const Math::Vector3f& t2v0, const Math::Vector3f& t2v1, const Math::Vector3f& t2v2) {
    // Simplified triangle-triangle intersection test
    // A full implementation would use Möller's algorithm or similar
    
    // For now, return false (no intersection detected)
    // This is a placeholder for the actual intersection test
    return false;
}

} // namespace SurfaceGen
} // namespace VoxelEditor