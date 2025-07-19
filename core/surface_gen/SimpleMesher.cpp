#include "SimpleMesher.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <cmath>
#include <thread>
#include <future>
#include <atomic>
#include <unordered_set>

using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

namespace VoxelEditor {
namespace SurfaceGen {

// SimpleMesher implementation following the algorithm design in SimpleMesher.md
// This file implements the complete Simple Mesh Algorithm for exact box mesh generation
// with intelligent face removal, mesh subdivision, and T-junction prevention.
//
// Algorithm phases implemented:
// - Phase 1: Spatial Indexing (SpatialIndex class)
// - Phase 2: Vertex Management (VertexManager class)
// - Phase 3: Face Occlusion Tracking (FaceOcclusionTracker class)
// - Phase 4: Edge Vertex Registry (EdgeVertexRegistry class)
// - Phase 5: Face Generation (generateFace and triangulateRectangle)
// - Phase 6: Main Algorithm (generateMesh with parallel processing)

// Rectangle implementation
bool SimpleMesher::Rectangle::intersects(const Rectangle& other) const {
    return x < other.right() && right() > other.x &&
           y < other.bottom() && bottom() > other.y;
}

bool SimpleMesher::Rectangle::contains(const Rectangle& other) const {
    return x <= other.x && y <= other.y &&
           right() >= other.right() && bottom() >= other.bottom();
}

SimpleMesher::Rectangle SimpleMesher::Rectangle::intersection(const Rectangle& other) const {
    if (!intersects(other)) {
        return Rectangle(0, 0, 0, 0);
    }
    
    int ix = std::max(x, other.x);
    int iy = std::max(y, other.y);
    int iright = std::min(right(), other.right());
    int ibottom = std::min(bottom(), other.bottom());
    
    return Rectangle(ix, iy, iright - ix, ibottom - iy);
}

// EdgeKey implementation
SimpleMesher::EdgeKey::EdgeKey(const WorldCoordinates& start, const WorldCoordinates& end) {
    // Convert to 0.1mm integer units for exact comparison
    Vector3i p1_int(
        static_cast<int>(std::round(start.x() * 10000.0f)),
        static_cast<int>(std::round(start.y() * 10000.0f)),
        static_cast<int>(std::round(start.z() * 10000.0f))
    );
    
    Vector3i p2_int(
        static_cast<int>(std::round(end.x() * 10000.0f)),
        static_cast<int>(std::round(end.y() * 10000.0f)),
        static_cast<int>(std::round(end.z() * 10000.0f))
    );
    
    // Normalize: smaller point first (lexicographic order)
    if (p1_int.x < p2_int.x || 
        (p1_int.x == p2_int.x && p1_int.y < p2_int.y) ||
        (p1_int.x == p2_int.x && p1_int.y == p2_int.y && p1_int.z < p2_int.z)) {
        p1 = p1_int;
        p2 = p2_int;
    } else {
        p1 = p2_int;
        p2 = p1_int;
    }
}

bool SimpleMesher::EdgeKey::operator==(const EdgeKey& other) const {
    return p1 == other.p1 && p2 == other.p2;
}

size_t SimpleMesher::EdgeKey::hash() const {
    // Combine hashes of both points
    size_t h1 = std::hash<int>()(p1.x) ^ (std::hash<int>()(p1.y) << 1) ^ (std::hash<int>()(p1.z) << 2);
    size_t h2 = std::hash<int>()(p2.x) ^ (std::hash<int>()(p2.y) << 1) ^ (std::hash<int>()(p2.z) << 2);
    return h1 ^ (h2 << 1);
}

// VertexKey implementation
SimpleMesher::VertexKey::VertexKey(const WorldCoordinates& position) {
    pos.x = static_cast<int>(std::round(position.x() * 10000.0f));
    pos.y = static_cast<int>(std::round(position.y() * 10000.0f));
    pos.z = static_cast<int>(std::round(position.z() * 10000.0f));
}

size_t SimpleMesher::VertexKey::hash() const {
    return std::hash<int>()(pos.x) ^ 
           (std::hash<int>()(pos.y) << 1) ^ 
           (std::hash<int>()(pos.z) << 2);
}

// SimpleMesher implementation
SimpleMesher::SimpleMesher() : m_cancelled(false) {
}

SimpleMesher::~SimpleMesher() {
}

Mesh SimpleMesher::generateMesh(const VoxelGrid& grid, 
                               const SurfaceSettings& settings,
                               MeshResolution meshResolution) {
    // Main entry point implementing the Simple Mesh Algorithm
    // See SimpleMesher.md for the complete algorithm specification
    // Reset cancellation flag
    m_cancelled = false;
    
    // Validate inputs
    int resolution = static_cast<int>(meshResolution);
    if (resolution != 1 && resolution != 2 && resolution != 4 && 
        resolution != 8 && resolution != 16) {
        return Mesh();
    }
    
    // Build spatial index
    SpatialIndex spatialIndex(512); // 512cm cell size for largest voxels
    
    // Get all voxels from grid and index them
    std::vector<VoxelInfo> voxels;
    
    // Get all voxel positions from the grid
    auto voxelPositions = grid.getAllVoxels();
    
    // Convert to our format and add to spatial index
    int voxelId = 0;
    for (const auto& voxelPos : voxelPositions) {
        // VoxelPosition already contains increment coordinates
        IncrementCoordinates pos = voxelPos.incrementPos;
        
        // Get voxel size from the voxel's resolution
        int voxelSize = static_cast<int>(VoxelData::getVoxelSize(voxelPos.resolution) * 100.0f); // Convert to cm
        
        voxels.push_back({pos, voxelSize});
        spatialIndex.insert(voxelId++, pos, voxelSize);
    }
    
    reportProgress(0.1f);
    
    // Determine number of threads to use
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // Default to 4 if detection fails
    
    // Don't use more threads than voxels
    numThreads = std::min(numThreads, static_cast<unsigned int>(voxels.size()));
    
    // For small voxel counts, use single thread
    if (voxels.size() < 100) {
        numThreads = 1;
    }
    
    if (numThreads == 1) {
        // Single-threaded path
        VertexManager vertexManager;
        EdgeVertexRegistry edgeRegistry;
        std::vector<uint32_t> indices;
        
        // Reserve approximate memory
        vertexManager.reserve(voxels.size() * 8); // ~8 vertices per voxel
        indices.reserve(voxels.size() * 36); // 12 triangles * 3 indices per voxel
        
        // Process each voxel
        float progressStep = 0.8f / std::max(1.0f, static_cast<float>(voxels.size()));
        for (size_t i = 0; i < voxels.size(); ++i) {
            if (m_cancelled) {
                return Mesh();
            }
            
            generateVoxelMesh(
                static_cast<int>(i),
                voxels[i].position,
                voxels[i].size,
                spatialIndex,
                grid,
                vertexManager,
                edgeRegistry,
                indices,
                resolution,
                voxels
            );
            
            reportProgress(0.1f + (i + 1) * progressStep);
        }
        
        // Build final mesh
        Mesh result;
        result.vertices = vertexManager.getVertices();
        result.indices = indices;
        
        // Calculate normals if requested
        if (settings.generateNormals) {
            result.calculateNormals();
        }
        
        // Calculate bounds
        result.calculateBounds();
        
        // Report completion
        reportProgress(1.0f);
        
        return result;
    }
    
    // Multi-threaded path
    std::vector<ThreadLocalData> threadData(numThreads);
    std::vector<std::future<void>> futures;
    
    // Shared edge registry for preventing T-junctions
    EdgeVertexRegistry sharedEdgeRegistry;
    
    // Initialize thread-local data
    for (auto& data : threadData) {
        data.vertexManager = std::make_unique<VertexManager>();
        data.vertexManager->reserve((voxels.size() / numThreads + 1) * 8);
        data.indices.reserve((voxels.size() / numThreads + 1) * 36);
    }
    
    // Atomic counter for progress reporting
    std::atomic<size_t> processedCount{0};
    
    // Process voxels in parallel
    size_t voxelsPerThread = voxels.size() / numThreads;
    size_t remainder = voxels.size() % numThreads;
    
    for (unsigned int t = 0; t < numThreads; ++t) {
        size_t startIdx = t * voxelsPerThread + std::min(static_cast<size_t>(t), remainder);
        size_t endIdx = startIdx + voxelsPerThread + (t < remainder ? 1 : 0);
        
        futures.push_back(std::async(std::launch::async, [&, t, startIdx, endIdx]() {
            auto& localData = threadData[t];
            
            for (size_t i = startIdx; i < endIdx; ++i) {
                if (m_cancelled) {
                    return;
                }
                
                generateVoxelMesh(
                    static_cast<int>(i),
                    voxels[i].position,
                    voxels[i].size,
                    spatialIndex,
                    grid,
                    *localData.vertexManager,
                    sharedEdgeRegistry,
                    localData.indices,
                    resolution,
                    voxels
                );
                
                // Update progress
                size_t count = ++processedCount;
                reportProgress(0.1f + count * 0.8f / voxels.size());
            }
        }));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    if (m_cancelled) {
        return Mesh();
    }
    
    // Merge thread results
    Mesh result = mergeThreadResults(threadData);
    
    // Calculate normals if requested
    if (settings.generateNormals) {
        result.calculateNormals();
    }
    
    // Calculate bounds
    result.calculateBounds();
    
    // Report completion
    reportProgress(1.0f);
    
    return result;
}

void SimpleMesher::reportProgress(float progress) {
    if (m_progressCallback) {
        m_progressCallback(progress);
    }
}

// SpatialIndex implementation (Phase 2: Basic Components - TODO.md)
// Provides O(1) neighbor lookup as specified in the design
// SpatialIndex implementation - Phase 1 of SimpleMesher.md
// Provides O(1) neighbor lookup using spatial hashing
SimpleMesher::SpatialIndex::SpatialIndex(int cellSize) : m_cellSize(cellSize) {
    if (cellSize <= 0) {
        m_cellSize = 512; // Default to max voxel size
    }
}

void SimpleMesher::SpatialIndex::insert(int voxelId, const IncrementCoordinates& position, int size) {
    // Calculate bounds of the voxel
    int minX = position.x() / m_cellSize;
    int minY = position.y() / m_cellSize;
    int minZ = position.z() / m_cellSize;
    int maxX = (position.x() + size - 1) / m_cellSize;
    int maxY = (position.y() + size - 1) / m_cellSize;
    int maxZ = (position.z() + size - 1) / m_cellSize;
    
    // Insert voxel ID into all cells it overlaps
    for (int z = minZ; z <= maxZ; ++z) {
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                uint64_t key = getCellKey(x, y, z);
                m_grid[key].push_back(voxelId);
            }
        }
    }
}

std::vector<int> SimpleMesher::SpatialIndex::getNeighbors(const IncrementCoordinates& position, int size) const {
    std::unordered_set<int> neighbors;
    
    // Expand bounds by 1cm to catch adjacent voxels
    int minX = (position.x() - 1) / m_cellSize;
    int minY = (position.y() - 1) / m_cellSize;
    int minZ = (position.z() - 1) / m_cellSize;
    int maxX = (position.x() + size) / m_cellSize;
    int maxY = (position.y() + size) / m_cellSize;
    int maxZ = (position.z() + size) / m_cellSize;
    
    // Collect all voxels from overlapping cells
    for (int z = minZ; z <= maxZ; ++z) {
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                uint64_t key = getCellKey(x, y, z);
                auto it = m_grid.find(key);
                if (it != m_grid.end()) {
                    for (int id : it->second) {
                        neighbors.insert(id);
                    }
                }
            }
        }
    }
    
    return std::vector<int>(neighbors.begin(), neighbors.end());
}

void SimpleMesher::SpatialIndex::clear() {
    m_grid.clear();
}

uint64_t SimpleMesher::SpatialIndex::getCellKey(int x, int y, int z) const {
    // Pack 3D coordinates into 64-bit key
    // Use 21 bits for each coordinate (supports ±1,048,576 range)
    const uint64_t COORD_BITS = 21;
    const uint64_t COORD_MASK = (1ULL << COORD_BITS) - 1;
    
    // Offset to handle negative coordinates
    const int64_t OFFSET = 1 << 20;
    
    uint64_t ux = static_cast<uint64_t>(x + OFFSET) & COORD_MASK;
    uint64_t uy = static_cast<uint64_t>(y + OFFSET) & COORD_MASK;
    uint64_t uz = static_cast<uint64_t>(z + OFFSET) & COORD_MASK;
    
    return (ux << (2 * COORD_BITS)) | (uy << COORD_BITS) | uz;
}

// VertexManager implementation (Phase 2: Basic Components - TODO.md)
// Hash-based vertex deduplication with 0.1mm tolerance as per design
// VertexManager implementation - Phase 2 of SimpleMesher.md
// Handles vertex deduplication with 0.1mm tolerance
SimpleMesher::VertexManager::VertexManager() {
}

uint32_t SimpleMesher::VertexManager::getOrCreateVertex(const WorldCoordinates& position) {
    VertexKey key(position);
    
    // Check if vertex already exists
    auto it = m_vertexMap.find(key);
    if (it != m_vertexMap.end()) {
        return it->second;
    }
    
    // Create new vertex
    uint32_t index = static_cast<uint32_t>(m_vertices.size());
    m_vertices.push_back(position);
    m_vertexMap[key] = index;
    
    return index;
}

void SimpleMesher::VertexManager::clear() {
    m_vertices.clear();
    m_vertexMap.clear();
}

void SimpleMesher::VertexManager::reserve(size_t count) {
    m_vertices.reserve(count);
    // Reserve approximately the same size for the map
    m_vertexMap.reserve(count);
}

// FaceOcclusionTracker implementation (Phase 5: Occlusion Tracking - TODO.md)
// Rectangle-based occlusion tracking as specified in design
// FaceOcclusionTracker implementation - Phase 3 of SimpleMesher.md
// Tracks visible regions of faces using rectangle subtraction
SimpleMesher::FaceOcclusionTracker::FaceOcclusionTracker(int faceSize) 
    : m_faceSize(faceSize) {
}

void SimpleMesher::FaceOcclusionTracker::addOcclusion(const Rectangle& rect) {
    m_occludedRegions.push_back(rect);
}

std::vector<SimpleMesher::Rectangle> SimpleMesher::FaceOcclusionTracker::computeVisibleRectangles() {
    // Start with the full face as visible
    std::vector<Rectangle> visibleRects;
    visibleRects.push_back(Rectangle(0, 0, m_faceSize, m_faceSize));
    
    // Subtract each occlusion
    for (const Rectangle& occlusion : m_occludedRegions) {
        std::vector<Rectangle> newVisible;
        
        for (const Rectangle& rect : visibleRects) {
            std::vector<Rectangle> remaining = subtractRectangle(rect, occlusion);
            newVisible.insert(newVisible.end(), remaining.begin(), remaining.end());
        }
        
        visibleRects = newVisible;
    }
    
    // Merge adjacent rectangles to reduce complexity
    return mergeAdjacentRectangles(visibleRects);
}

std::vector<SimpleMesher::Rectangle> SimpleMesher::FaceOcclusionTracker::subtractRectangle(
    const Rectangle& rect, const Rectangle& occlusion) {
    std::vector<Rectangle> result;
    
    // If no intersection, return the original rectangle
    if (!rect.intersects(occlusion)) {
        result.push_back(rect);
        return result;
    }
    
    // If fully occluded, return empty
    if (occlusion.contains(rect)) {
        return result;
    }
    
    // Calculate intersection bounds
    int intersectLeft = std::max(rect.x, occlusion.x);
    int intersectTop = std::max(rect.y, occlusion.y);
    int intersectRight = std::min(rect.right(), occlusion.right());
    int intersectBottom = std::min(rect.bottom(), occlusion.bottom());
    
    // Top strip
    if (intersectTop > rect.y) {
        result.push_back(Rectangle(rect.x, rect.y, rect.width, intersectTop - rect.y));
    }
    
    // Bottom strip
    if (intersectBottom < rect.bottom()) {
        result.push_back(Rectangle(rect.x, intersectBottom, rect.width, rect.bottom() - intersectBottom));
    }
    
    // Left strip (only middle part)
    if (intersectLeft > rect.x && intersectBottom > intersectTop) {
        result.push_back(Rectangle(rect.x, intersectTop, intersectLeft - rect.x, intersectBottom - intersectTop));
    }
    
    // Right strip (only middle part)
    if (intersectRight < rect.right() && intersectBottom > intersectTop) {
        result.push_back(Rectangle(intersectRight, intersectTop, rect.right() - intersectRight, intersectBottom - intersectTop));
    }
    
    return result;
}

std::vector<SimpleMesher::Rectangle> SimpleMesher::FaceOcclusionTracker::mergeAdjacentRectangles(
    const std::vector<Rectangle>& rects) {
    // Simple implementation - just return as-is for now
    // A more sophisticated implementation would try to merge rectangles
    // that share edges and have the same width or height
    return rects;
}

// EdgeVertexRegistry implementation (Phase 6: Mesh Subdivision - TODO.md)
// Thread-safe edge vertex registry to prevent T-junctions as per design
// EdgeVertexRegistry implementation - Phase 4 of SimpleMesher.md
// Prevents T-junctions by ensuring consistent edge vertices
SimpleMesher::EdgeVertexRegistry::EdgeVertexRegistry() {
}

std::vector<uint32_t> SimpleMesher::EdgeVertexRegistry::getOrCreateEdgeVertices(
    const WorldCoordinates& start,
    const WorldCoordinates& end,
    int subdivisionSize,
    VertexManager& vertexManager) {
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Create normalized edge key
    EdgeKey key(start, end);
    
    // Check if edge vertices already exist
    auto it = m_edgeVertices.find(key);
    if (it != m_edgeVertices.end()) {
        return it->second;
    }
    
    // Create vertices at regular intervals along the edge
    std::vector<uint32_t> vertices;
    
    // Calculate edge length and number of segments
    float dx = end.x() - start.x();
    float dy = end.y() - start.y();
    float dz = end.z() - start.z();
    float edgeLength = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    // Convert subdivision size from cm to meters
    float subdivisionSizeMeters = subdivisionSize * 0.01f;
    
    // Calculate number of segments
    int numSegments = static_cast<int>(std::ceil(edgeLength / subdivisionSizeMeters));
    if (numSegments < 1) numSegments = 1;
    
    // Create vertices along the edge
    for (int i = 0; i <= numSegments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numSegments);
        
        WorldCoordinates position(
            start.x() + t * dx,
            start.y() + t * dy,
            start.z() + t * dz
        );
        
        uint32_t vertexIndex = vertexManager.getOrCreateVertex(position);
        vertices.push_back(vertexIndex);
    }
    
    // Store in registry
    m_edgeVertices[key] = vertices;
    
    return vertices;
}

void SimpleMesher::EdgeVertexRegistry::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_edgeVertices.clear();
}

// Main algorithm stub implementations
void SimpleMesher::generateVoxelMesh(
    int voxelId,
    const IncrementCoordinates& position,
    int size,
    const SpatialIndex& spatialIndex,
    const VoxelGrid& grid,
    VertexManager& vertexManager,
    EdgeVertexRegistry& edgeRegistry,
    std::vector<uint32_t>& indices,
    int meshResolution,
    const std::vector<VoxelInfo>& voxels) {
    
    // Phase 3-4: Face Generation and Removal (TODO.md)
    // Generate faces with intelligent removal for adjacent voxels
    
    // Process each of the 6 faces
    const FaceDirection faces[] = {
        FaceDirection::NEG_X, FaceDirection::POS_X,
        FaceDirection::NEG_Y, FaceDirection::POS_Y,
        FaceDirection::NEG_Z, FaceDirection::POS_Z
    };
    
    for (FaceDirection face : faces) {
        // Find occlusions from neighboring voxels
        FaceOcclusionTracker occlusionTracker(size);
        std::vector<int> neighbors = spatialIndex.getNeighbors(position, size);
        
        for (int neighborId : neighbors) {
            if (neighborId == voxelId) continue; // Skip self
            
            // Get neighbor voxel info
            const VoxelInfo& neighbor = voxels[neighborId];
            
            // Check if this neighbor occludes the current face
            if (faceIsAdjacent(position, size, face, neighbor.position, neighbor.size)) {
                Rectangle overlap = calculateOverlap(position, size, face, neighbor.position, neighbor.size);
                if (overlap.width > 0 && overlap.height > 0) {
                    occlusionTracker.addOcclusion(overlap);
                }
            }
        }
        
        // Get visible regions
        std::vector<Rectangle> visibleRects = occlusionTracker.computeVisibleRectangles();
        
        if (!visibleRects.empty()) {
            FaceData faceData = createFaceData(position, size, face, visibleRects);
            generateFace(faceData, indices, edgeRegistry, vertexManager, meshResolution);
        }
    }
}

// Phase 5 of SimpleMesher.md - Face Generation
// Generates triangulated mesh for visible regions of a face
void SimpleMesher::generateFace(
    const FaceData& faceData,
    std::vector<uint32_t>& indices,
    EdgeVertexRegistry& edgeRegistry,
    VertexManager& vertexManager,
    int meshResolution) {
    
    // First, ensure edge vertices exist for this face
    FaceData& mutableFaceData = const_cast<FaceData&>(faceData);
    ensureEdgeVertices(mutableFaceData, edgeRegistry, vertexManager, meshResolution);
    
    // Process each visible rectangle
    for (const Rectangle& rect : faceData.visibleRectangles) {
        // Skip very small rectangles
        if (rect.width < meshResolution/2 || rect.height < meshResolution/2) {
            continue;
        }
        
        // Check if we need subdivision
        if (rect.width > meshResolution || rect.height > meshResolution) {
            // Use subdivision for larger rectangles
            triangulateRectangle(rect, faceData, indices, vertexManager, meshResolution);
        } else {
            // Small rectangle - just create a quad
            float u0 = rect.x * 0.01f; // Convert cm to meters
            float v0 = rect.y * 0.01f;
            float u1 = (rect.x + rect.width) * 0.01f;
            float v1 = (rect.y + rect.height) * 0.01f;
            
            // Calculate world positions for the 4 corners
            WorldCoordinates p0 = faceData.origin + 
                WorldCoordinates(faceData.uDir * u0 + faceData.vDir * v0);
            WorldCoordinates p1 = faceData.origin + 
                WorldCoordinates(faceData.uDir * u1 + faceData.vDir * v0);
            WorldCoordinates p2 = faceData.origin + 
                WorldCoordinates(faceData.uDir * u1 + faceData.vDir * v1);
            WorldCoordinates p3 = faceData.origin + 
                WorldCoordinates(faceData.uDir * u0 + faceData.vDir * v1);
            
            // Get or create vertices
            uint32_t idx0 = vertexManager.getOrCreateVertex(p0);
            uint32_t idx1 = vertexManager.getOrCreateVertex(p1);
            uint32_t idx2 = vertexManager.getOrCreateVertex(p2);
            uint32_t idx3 = vertexManager.getOrCreateVertex(p3);
            
            // Add quad as two triangles
            addQuad(idx0, idx1, idx2, idx3, indices);
        }
    }
}

// Implements triangulateRectangle from Phase 5 of SimpleMesher.md
// Creates a grid of vertices and triangulates a visible rectangle
void SimpleMesher::triangulateRectangle(
    const Rectangle& rect,
    const FaceData& faceData,
    std::vector<uint32_t>& indices,
    VertexManager& vertexManager,
    int meshResolution) {
    
    // Calculate subdivisions
    int uSubdivisions = rect.width / meshResolution;
    int vSubdivisions = rect.height / meshResolution;
    int uRemainder = rect.width % meshResolution;
    int vRemainder = rect.height % meshResolution;
    
    // Adjust subdivisions if we have remainders
    int uVertices = uSubdivisions + (uRemainder > 0 ? 2 : 1);
    int vVertices = vSubdivisions + (vRemainder > 0 ? 2 : 1);
    
    // Build vertex grid for this rectangle
    std::vector<std::vector<uint32_t>> vertices(vVertices, std::vector<uint32_t>(uVertices));
    
    // Generate vertices
    for (int v = 0; v < vVertices; ++v) {
        for (int u = 0; u < uVertices; ++u) {
            // Calculate position in face-local coordinates
            float uPos, vPos;
            
            if (u < uSubdivisions) {
                uPos = (rect.x + u * meshResolution) * 0.01f;
            } else {
                uPos = (rect.x + rect.width) * 0.01f;
            }
            
            if (v < vSubdivisions) {
                vPos = (rect.y + v * meshResolution) * 0.01f;
            } else {
                vPos = (rect.y + rect.height) * 0.01f;
            }
            
            // Check if this vertex is on a face edge and use existing edge vertex
            bool onLeftEdge = (rect.x == 0 && u == 0);
            bool onRightEdge = (rect.x + rect.width == faceData.size && u == uVertices - 1);
            bool onBottomEdge = (rect.y == 0 && v == 0);
            bool onTopEdge = (rect.y + rect.height == faceData.size && v == vVertices - 1);
            
            if (onLeftEdge && !faceData.leftEdgeVertices.empty()) {
                int edgeIndex = getEdgeIndex(rect.y + v * meshResolution, faceData.size, meshResolution);
                if (edgeIndex < faceData.leftEdgeVertices.size()) {
                    vertices[v][u] = faceData.leftEdgeVertices[edgeIndex];
                    continue;
                }
            }
            
            if (onRightEdge && !faceData.rightEdgeVertices.empty()) {
                int edgeIndex = getEdgeIndex(rect.y + v * meshResolution, faceData.size, meshResolution);
                if (edgeIndex < faceData.rightEdgeVertices.size()) {
                    vertices[v][u] = faceData.rightEdgeVertices[edgeIndex];
                    continue;
                }
            }
            
            if (onBottomEdge && !faceData.bottomEdgeVertices.empty()) {
                int edgeIndex = getEdgeIndex(rect.x + u * meshResolution, faceData.size, meshResolution);
                if (edgeIndex < faceData.bottomEdgeVertices.size()) {
                    vertices[v][u] = faceData.bottomEdgeVertices[edgeIndex];
                    continue;
                }
            }
            
            if (onTopEdge && !faceData.topEdgeVertices.empty()) {
                int edgeIndex = getEdgeIndex(rect.x + u * meshResolution, faceData.size, meshResolution);
                if (edgeIndex < faceData.topEdgeVertices.size()) {
                    vertices[v][u] = faceData.topEdgeVertices[edgeIndex];
                    continue;
                }
            }
            
            // Interior vertex - create new
            WorldCoordinates vertexPos = faceData.origin + 
                WorldCoordinates(faceData.uDir * uPos + faceData.vDir * vPos);
            vertices[v][u] = vertexManager.getOrCreateVertex(vertexPos);
        }
    }
    
    // Generate quads (2 triangles each)
    for (int v = 0; v < vVertices - 1; ++v) {
        for (int u = 0; u < uVertices - 1; ++u) {
            addQuad(vertices[v][u], vertices[v][u+1], 
                   vertices[v+1][u+1], vertices[v+1][u], indices);
        }
    }
}

SimpleMesher::FaceData SimpleMesher::createFaceData(
    const IncrementCoordinates& voxelPos,
    int voxelSize,
    FaceDirection face,
    const std::vector<Rectangle>& visibleRects) {
    
    FaceData faceData;
    faceData.size = voxelSize;
    faceData.visibleRectangles = visibleRects;
    
    // Convert voxel position to world coordinates
    WorldCoordinates worldPos = CoordinateConverter::incrementToWorld(voxelPos);
    float size = voxelSize * 0.01f; // Convert cm to meters
    
    // Define consistent coordinate system for each face
    // Origin is always at minimum corner of face
    switch (face) {
        case FaceDirection::NEG_X:  // Left face
            faceData.origin = worldPos;
            faceData.uDir = Vector3f(0, 0, 1);   // Z axis
            faceData.vDir = Vector3f(0, 1, 0);   // Y axis
            faceData.normal = Vector3f(-1, 0, 0);
            break;
            
        case FaceDirection::POS_X:  // Right face
            faceData.origin = WorldCoordinates(worldPos.x() + size, worldPos.y(), worldPos.z());
            faceData.uDir = Vector3f(0, 0, 1);   // Z axis
            faceData.vDir = Vector3f(0, 1, 0);   // Y axis
            faceData.normal = Vector3f(1, 0, 0);
            break;
            
        case FaceDirection::NEG_Y:  // Bottom face
            faceData.origin = worldPos;
            faceData.uDir = Vector3f(1, 0, 0);   // X axis
            faceData.vDir = Vector3f(0, 0, 1);   // Z axis
            faceData.normal = Vector3f(0, -1, 0);
            break;
            
        case FaceDirection::POS_Y:  // Top face
            faceData.origin = WorldCoordinates(worldPos.x(), worldPos.y() + size, worldPos.z());
            faceData.uDir = Vector3f(1, 0, 0);   // X axis
            faceData.vDir = Vector3f(0, 0, 1);   // Z axis
            faceData.normal = Vector3f(0, 1, 0);
            break;
            
        case FaceDirection::NEG_Z:  // Back face
            faceData.origin = worldPos;
            faceData.uDir = Vector3f(1, 0, 0);   // X axis
            faceData.vDir = Vector3f(0, 1, 0);   // Y axis
            faceData.normal = Vector3f(0, 0, -1);
            break;
            
        case FaceDirection::POS_Z:  // Front face
            faceData.origin = WorldCoordinates(worldPos.x(), worldPos.y(), worldPos.z() + size);
            faceData.uDir = Vector3f(1, 0, 0);   // X axis
            faceData.vDir = Vector3f(0, 1, 0);   // Y axis
            faceData.normal = Vector3f(0, 0, 1);
            break;
    }
    
    return faceData;
}

bool SimpleMesher::faceIsAdjacent(
    const IncrementCoordinates& voxel1Pos,
    int voxel1Size,
    FaceDirection face,
    const IncrementCoordinates& voxel2Pos,
    int voxel2Size) {
    
    // Get the position of voxel1's face in the given direction
    int facePlanePos = 0;
    int faceAxis = 0; // 0=X, 1=Y, 2=Z
    
    switch (face) {
        case FaceDirection::NEG_X:
            facePlanePos = voxel1Pos.x();
            faceAxis = 0;
            break;
        case FaceDirection::POS_X:
            facePlanePos = voxel1Pos.x() + voxel1Size;
            faceAxis = 0;
            break;
        case FaceDirection::NEG_Y:
            facePlanePos = voxel1Pos.y();
            faceAxis = 1;
            break;
        case FaceDirection::POS_Y:
            facePlanePos = voxel1Pos.y() + voxel1Size;
            faceAxis = 1;
            break;
        case FaceDirection::NEG_Z:
            facePlanePos = voxel1Pos.z();
            faceAxis = 2;
            break;
        case FaceDirection::POS_Z:
            facePlanePos = voxel1Pos.z() + voxel1Size;
            faceAxis = 2;
            break;
    }
    
    // Check if voxel2 has a face at the same plane position
    int voxel2FacePos1 = 0, voxel2FacePos2 = 0;
    if (faceAxis == 0) { // X axis
        voxel2FacePos1 = voxel2Pos.x();
        voxel2FacePos2 = voxel2Pos.x() + voxel2Size;
    } else if (faceAxis == 1) { // Y axis
        voxel2FacePos1 = voxel2Pos.y();
        voxel2FacePos2 = voxel2Pos.y() + voxel2Size;
    } else { // Z axis
        voxel2FacePos1 = voxel2Pos.z();
        voxel2FacePos2 = voxel2Pos.z() + voxel2Size;
    }
    
    // Check if faces are coplanar
    bool coplanar = (facePlanePos == voxel2FacePos1) || (facePlanePos == voxel2FacePos2);
    if (!coplanar) {
        return false;
    }
    
    // Check if faces overlap in the other two dimensions
    bool overlapsInU = false, overlapsInV = false;
    
    if (faceAxis == 0) { // X face, check Y and Z overlap
        overlapsInU = (voxel1Pos.y() < voxel2Pos.y() + voxel2Size) && 
                      (voxel1Pos.y() + voxel1Size > voxel2Pos.y());
        overlapsInV = (voxel1Pos.z() < voxel2Pos.z() + voxel2Size) && 
                      (voxel1Pos.z() + voxel1Size > voxel2Pos.z());
    } else if (faceAxis == 1) { // Y face, check X and Z overlap
        overlapsInU = (voxel1Pos.x() < voxel2Pos.x() + voxel2Size) && 
                      (voxel1Pos.x() + voxel1Size > voxel2Pos.x());
        overlapsInV = (voxel1Pos.z() < voxel2Pos.z() + voxel2Size) && 
                      (voxel1Pos.z() + voxel1Size > voxel2Pos.z());
    } else { // Z face, check X and Y overlap
        overlapsInU = (voxel1Pos.x() < voxel2Pos.x() + voxel2Size) && 
                      (voxel1Pos.x() + voxel1Size > voxel2Pos.x());
        overlapsInV = (voxel1Pos.y() < voxel2Pos.y() + voxel2Size) && 
                      (voxel1Pos.y() + voxel1Size > voxel2Pos.y());
    }
    
    return coplanar && overlapsInU && overlapsInV;
}

SimpleMesher::Rectangle SimpleMesher::calculateOverlap(
    const IncrementCoordinates& voxel1Pos,
    int voxel1Size,
    FaceDirection face,
    const IncrementCoordinates& voxel2Pos,
    int voxel2Size) {
    
    // Calculate the 2D overlap in face-local coordinates
    // Face-local coordinates: origin at voxel1's face minimum corner
    
    int overlapX = 0, overlapY = 0, overlapWidth = 0, overlapHeight = 0;
    
    switch (face) {
        case FaceDirection::NEG_X:
        case FaceDirection::POS_X:
            // For X faces, U=Z, V=Y
            overlapX = std::max(0, voxel2Pos.z() - voxel1Pos.z());
            overlapY = std::max(0, voxel2Pos.y() - voxel1Pos.y());
            overlapWidth = std::min(voxel1Pos.z() + voxel1Size, voxel2Pos.z() + voxel2Size) - 
                          std::max(voxel1Pos.z(), voxel2Pos.z());
            overlapHeight = std::min(voxel1Pos.y() + voxel1Size, voxel2Pos.y() + voxel2Size) - 
                           std::max(voxel1Pos.y(), voxel2Pos.y());
            break;
            
        case FaceDirection::NEG_Y:
        case FaceDirection::POS_Y:
            // For Y faces, U=X, V=Z
            overlapX = std::max(0, voxel2Pos.x() - voxel1Pos.x());
            overlapY = std::max(0, voxel2Pos.z() - voxel1Pos.z());
            overlapWidth = std::min(voxel1Pos.x() + voxel1Size, voxel2Pos.x() + voxel2Size) - 
                          std::max(voxel1Pos.x(), voxel2Pos.x());
            overlapHeight = std::min(voxel1Pos.z() + voxel1Size, voxel2Pos.z() + voxel2Size) - 
                           std::max(voxel1Pos.z(), voxel2Pos.z());
            break;
            
        case FaceDirection::NEG_Z:
        case FaceDirection::POS_Z:
            // For Z faces, U=X, V=Y
            overlapX = std::max(0, voxel2Pos.x() - voxel1Pos.x());
            overlapY = std::max(0, voxel2Pos.y() - voxel1Pos.y());
            overlapWidth = std::min(voxel1Pos.x() + voxel1Size, voxel2Pos.x() + voxel2Size) - 
                          std::max(voxel1Pos.x(), voxel2Pos.x());
            overlapHeight = std::min(voxel1Pos.y() + voxel1Size, voxel2Pos.y() + voxel2Size) - 
                           std::max(voxel1Pos.y(), voxel2Pos.y());
            break;
    }
    
    // Ensure we have valid dimensions
    if (overlapWidth <= 0 || overlapHeight <= 0) {
        return Rectangle(0, 0, 0, 0);
    }
    
    return Rectangle(overlapX, overlapY, overlapWidth, overlapHeight);
}

void SimpleMesher::ensureEdgeVertices(
    FaceData& faceData,
    EdgeVertexRegistry& edgeRegistry,
    VertexManager& vertexManager,
    int meshResolution) {
    
    // Get the 4 corners of the face in world coordinates
    WorldCoordinates origin = faceData.origin;
    float size = faceData.size * 0.01f; // Convert cm to meters
    
    WorldCoordinates uMax = origin + WorldCoordinates(faceData.uDir * size);
    WorldCoordinates vMax = origin + WorldCoordinates(faceData.vDir * size);
    WorldCoordinates uvMax = origin + WorldCoordinates(faceData.uDir * size + faceData.vDir * size);
    
    // Get or create vertices for each edge
    // Bottom edge (along U direction)
    faceData.bottomEdgeVertices = edgeRegistry.getOrCreateEdgeVertices(
        origin, uMax, meshResolution, vertexManager);
    
    // Top edge (along U direction)
    faceData.topEdgeVertices = edgeRegistry.getOrCreateEdgeVertices(
        vMax, uvMax, meshResolution, vertexManager);
    
    // Left edge (along V direction)
    faceData.leftEdgeVertices = edgeRegistry.getOrCreateEdgeVertices(
        origin, vMax, meshResolution, vertexManager);
    
    // Right edge (along V direction)
    faceData.rightEdgeVertices = edgeRegistry.getOrCreateEdgeVertices(
        uMax, uvMax, meshResolution, vertexManager);
}

int SimpleMesher::getEdgeIndex(int position, int edgeLength, int meshResolution) {
    return position / meshResolution;
}

void SimpleMesher::addQuad(
    uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3,
    std::vector<uint32_t>& indices) {
    // CCW winding order when viewed from outside
    // First triangle: v0, v1, v2
    indices.push_back(v0);
    indices.push_back(v1);
    indices.push_back(v2);
    
    // Second triangle: v0, v2, v3
    indices.push_back(v0);
    indices.push_back(v2);
    indices.push_back(v3);
}

WorldCoordinates SimpleMesher::calculateVertexPosition(
    const Rectangle& rect,
    int u, int v,
    const FaceData& faceData,
    int meshResolution) {
    
    // Calculate position in face-local coordinates
    float uPos = (rect.x + u * meshResolution) * 0.01f; // Convert cm to meters
    float vPos = (rect.y + v * meshResolution) * 0.01f;
    
    // Transform to world coordinates
    return faceData.origin + WorldCoordinates(faceData.uDir * uPos + faceData.vDir * vPos);
}

Mesh SimpleMesher::mergeThreadResults(const std::vector<ThreadLocalData>& threadData) {
    // Phase 7: Threading - Thread result merging (TODO.md)
    Mesh result;
    
    // Count total vertices and indices
    size_t totalVertices = 0;
    size_t totalIndices = 0;
    
    for (const auto& data : threadData) {
        if (data.vertexManager) {
            totalVertices += data.vertexManager->getVertices().size();
            totalIndices += data.indices.size();
        }
    }
    
    // Reserve space
    result.vertices.reserve(totalVertices);
    result.indices.reserve(totalIndices);
    
    // Merge vertices and build index offset map
    std::vector<uint32_t> vertexOffsets;
    vertexOffsets.reserve(threadData.size() + 1);
    vertexOffsets.push_back(0);
    
    for (const auto& data : threadData) {
        if (data.vertexManager) {
            const auto& vertices = data.vertexManager->getVertices();
            result.vertices.insert(result.vertices.end(), vertices.begin(), vertices.end());
            vertexOffsets.push_back(static_cast<uint32_t>(result.vertices.size()));
        } else {
            vertexOffsets.push_back(vertexOffsets.back());
        }
    }
    
    // Merge indices with offset adjustment
    for (size_t t = 0; t < threadData.size(); ++t) {
        const auto& data = threadData[t];
        if (data.vertexManager) {
            uint32_t offset = vertexOffsets[t];
            for (uint32_t idx : data.indices) {
                result.indices.push_back(idx + offset);
            }
        }
    }
    
    return result;
}

}
}