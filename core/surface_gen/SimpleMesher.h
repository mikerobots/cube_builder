#pragma once

#include "SurfaceTypes.h"
#include "../voxel_data/VoxelGrid.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include "../../foundation/logging/Logger.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <mutex>
#include <thread>
#include <functional>
#include <memory>

namespace VoxelEditor {
namespace SurfaceGen {

/**
 * SimpleMesher - A straightforward voxel mesh generator that creates exact box meshes.
 * 
 * This implementation follows the design documented in SimpleMesher.md and generates watertight 
 * meshes by creating box meshes for each voxel with intelligent face removal where 
 * voxels are adjacent. It's designed to be the base mesh generator for the smoothing 
 * pipeline (levels 0-10).
 * 
 * Design Reference: See core/surface_gen/SimpleMesher.md for the complete algorithm 
 * specification and implementation details.
 * 
 * Key requirements implemented (from SimpleMesher.md):
 * - REQ-SM-1: Exact voxel representation (every voxel as complete box)
 * - REQ-SM-2: Face removal rules (remove shared faces, keep partial overlaps)
 * - REQ-SM-3: Mesh quality (watertight, no T-junctions, correct winding)
 * - REQ-SM-4: Performance (O(n log n), handle 10,000+ voxels efficiently)
 * - REQ-SM-5: Constraints (no overlaps, mesh resolution 1-16cm)
 * 
 * Core components (as per design phases):
 * - Phase 1: SpatialIndex for O(1) neighbor lookup
 * - Phase 2: VertexManager with 0.1mm tolerance deduplication
 * - Phase 3: FaceOcclusionTracker with rectangle subtraction
 * - Phase 4: EdgeVertexRegistry for T-junction prevention
 * - Phase 5: Face generation with correct coordinate systems
 * - Phase 6: Main algorithm with parallel processing support
 */
class SimpleMesher {
public:
    /**
     * Supported mesh resolutions for subdivision.
     * These determine how finely voxel faces are subdivided.
     */
    enum class MeshResolution {
        Res_1cm = 1,
        Res_2cm = 2,
        Res_4cm = 4,
        Res_8cm = 8,
        Res_16cm = 16
    };

    /**
     * Default constructor.
     */
    SimpleMesher();
    
    /**
     * Destructor.
     */
    ~SimpleMesher();
    
    /**
     * Generate a mesh from a voxel grid.
     * 
     * @param grid The voxel grid to generate mesh from
     * @param settings Surface generation settings
     * @param meshResolution Resolution for face subdivision (default 8cm)
     * @return Generated mesh with vertices, triangles, and normals
     */
    Mesh generateMesh(const VoxelData::VoxelGrid& grid, 
                     const SurfaceSettings& settings,
                     MeshResolution meshResolution = MeshResolution::Res_8cm);
    
    /**
     * Progress callback function signature.
     */
    using ProgressCallback = std::function<void(float progress)>;
    
    /**
     * Set callback for progress reporting during mesh generation.
     * 
     * @param callback Function to call with progress updates (0.0-1.0)
     */
    void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
    
    /**
     * Cancel the current mesh generation operation.
     * Thread-safe - can be called from any thread.
     */
    void cancel() { m_cancelled = true; }
    
    /**
     * Check if the current operation has been cancelled.
     * @return true if cancel() has been called
     */
    bool isCancelled() const { return m_cancelled; }

protected:
    // Forward declarations of internal structures
    struct Rectangle;
    struct EdgeKey;
    struct VertexKey;
    struct FaceData;
    struct VoxelId;
    struct CellKey;
    
    /**
     * Rectangle in face-local coordinates (always in cm units).
     */
    struct Rectangle {
        int x, y;          // Position relative to face origin (cm)
        int width, height; // Size in cm
        
        Rectangle() : x(0), y(0), width(0), height(0) {}
        Rectangle(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}
        
        int right() const { return x + width; }
        int bottom() const { return y + height; }
        bool intersects(const Rectangle& other) const;
        bool contains(const Rectangle& other) const;
        Rectangle intersection(const Rectangle& other) const;
    };
    
    /**
     * Unique key for identifying edges between vertices.
     */
    struct EdgeKey {
        Math::Vector3i p1; // First point (in 0.1mm integer units)
        Math::Vector3i p2; // Second point (in 0.1mm integer units)
        
        EdgeKey() = default;
        EdgeKey(const Math::WorldCoordinates& start, const Math::WorldCoordinates& end);
        
        bool operator==(const EdgeKey& other) const;
        size_t hash() const;
    };
    
    /**
     * Unique key for vertex deduplication.
     */
    struct VertexKey {
        Math::Vector3i pos; // Position in 0.1mm integer units
        
        explicit VertexKey(const Math::WorldCoordinates& position);
        
        bool operator==(const VertexKey& other) const { return pos == other.pos; }
        size_t hash() const;
    };
    
    /**
     * Face directions.
     */
    enum class FaceDirection {
        NEG_X = 0, // Left face
        POS_X = 1, // Right face
        NEG_Y = 2, // Bottom face
        POS_Y = 3, // Top face
        NEG_Z = 4, // Back face
        POS_Z = 5  // Front face
    };
    
    /**
     * Face data including coordinate system and visible regions.
     */
    struct FaceData {
        Math::WorldCoordinates origin;    // Face origin in world space
        Math::Vector3f uDir;             // U direction (normalized)
        Math::Vector3f vDir;             // V direction (normalized)
        Math::Vector3f normal;           // Face normal (normalized)
        int size;                        // Face size in cm
        std::vector<Rectangle> visibleRectangles; // Visible regions
        
        // Edge vertex indices for T-junction prevention
        std::vector<uint32_t> bottomEdgeVertices;
        std::vector<uint32_t> topEdgeVertices;
        std::vector<uint32_t> leftEdgeVertices;
        std::vector<uint32_t> rightEdgeVertices;
    };
    
    /**
     * Spatial index for O(1) neighbor lookup.
     */
    class SpatialIndex {
    public:
        explicit SpatialIndex(int cellSize = 512); // Cell size in cm
        
        void insert(int voxelId, const Math::IncrementCoordinates& position, int size);
        std::vector<int> getNeighbors(const Math::IncrementCoordinates& position, int size) const;
        void clear();
        
    private:
        int m_cellSize;
        std::unordered_map<uint64_t, std::vector<int>> m_grid;
        
        uint64_t getCellKey(int x, int y, int z) const;
        
        friend class SimpleMesherTest; // For unit testing
    };
    
    /**
     * Vertex manager with deduplication.
     */
    class VertexManager {
    public:
        VertexManager();
        
        uint32_t getOrCreateVertex(const Math::WorldCoordinates& position);
        const std::vector<Math::WorldCoordinates>& getVertices() const { return m_vertices; }
        void clear();
        void reserve(size_t count);
        
    private:
        std::vector<Math::WorldCoordinates> m_vertices;
        struct VertexKeyHash {
            size_t operator()(const VertexKey& key) const { return key.hash(); }
        };
        
        std::unordered_map<VertexKey, uint32_t, VertexKeyHash> m_vertexMap;
    };
    
    /**
     * Face occlusion tracker using rectangle-based tracking.
     */
    class FaceOcclusionTracker {
    public:
        explicit FaceOcclusionTracker(int faceSize);
        
        void addOcclusion(const Rectangle& rect);
        std::vector<Rectangle> computeVisibleRectangles();
        
    private:
        int m_faceSize;
        std::vector<Rectangle> m_occludedRegions;
        
        std::vector<Rectangle> subtractRectangle(const Rectangle& rect, const Rectangle& occlusion);
        std::vector<Rectangle> mergeAdjacentRectangles(const std::vector<Rectangle>& rects);
    };
    
    /**
     * Edge vertex registry for T-junction prevention.
     */
    class EdgeVertexRegistry {
    public:
        EdgeVertexRegistry();
        
        std::vector<uint32_t> getOrCreateEdgeVertices(
            const Math::WorldCoordinates& start,
            const Math::WorldCoordinates& end,
            int subdivisionSize,
            VertexManager& vertexManager);
        
        void clear();
        
    private:
        struct EdgeKeyHash {
            size_t operator()(const EdgeKey& key) const { return key.hash(); }
        };
        
        std::unordered_map<EdgeKey, std::vector<uint32_t>, EdgeKeyHash> m_edgeVertices;
        mutable std::mutex m_mutex;
    };
    
    // Thread-local data for parallel processing
    struct ThreadLocalData {
        std::unique_ptr<VertexManager> vertexManager;
        std::vector<uint32_t> indices;
        std::vector<EdgeKey> localEdgeVertices;
    };
    
    // Core member variables
    ProgressCallback m_progressCallback;
    std::atomic<bool> m_cancelled;
    
    // Main algorithm functions
    struct VoxelInfo {
        Math::IncrementCoordinates position;
        int size;
    };
    
    void generateVoxelMesh(
        int voxelId,
        const Math::IncrementCoordinates& position,
        int size,
        const SpatialIndex& spatialIndex,
        const VoxelData::VoxelGrid& grid,
        VertexManager& vertexManager,
        EdgeVertexRegistry& edgeRegistry,
        std::vector<uint32_t>& indices,
        int meshResolution,
        const std::vector<VoxelInfo>& voxels);
    
    void generateFace(
        const FaceData& faceData,
        std::vector<uint32_t>& indices,
        EdgeVertexRegistry& edgeRegistry,
        VertexManager& vertexManager,
        int meshResolution);
    
    void triangulateRectangle(
        const Rectangle& rect,
        const FaceData& faceData,
        std::vector<uint32_t>& indices,
        VertexManager& vertexManager,
        int meshResolution);
    
    // Helper functions
    FaceData createFaceData(
        const Math::IncrementCoordinates& voxelPos,
        int voxelSize,
        FaceDirection face,
        const std::vector<Rectangle>& visibleRects);
    
    bool faceIsAdjacent(
        const Math::IncrementCoordinates& voxel1Pos,
        int voxel1Size,
        FaceDirection face,
        const Math::IncrementCoordinates& voxel2Pos,
        int voxel2Size);
    
    Rectangle calculateOverlap(
        const Math::IncrementCoordinates& voxel1Pos,
        int voxel1Size,
        FaceDirection face,
        const Math::IncrementCoordinates& voxel2Pos,
        int voxel2Size);
    
    void ensureEdgeVertices(
        FaceData& faceData,
        EdgeVertexRegistry& edgeRegistry,
        VertexManager& vertexManager,
        int meshResolution);
    
    int getEdgeIndex(int position, int edgeLength, int meshResolution);
    
    void addQuad(
        uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3,
        std::vector<uint32_t>& indices);
    
    Math::WorldCoordinates calculateVertexPosition(
        const Rectangle& rect,
        int u, int v,
        const FaceData& faceData,
        int meshResolution);
    
    // Progress reporting
    void reportProgress(float progress);
    
    // Thread result merging
    Mesh mergeThreadResults(const std::vector<ThreadLocalData>& threadData);
};

}
}