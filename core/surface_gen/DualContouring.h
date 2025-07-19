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

namespace VoxelEditor {
namespace SurfaceGen {

/**
 * [DEPRECATED] Sparse dual contouring implementation for generating meshes from voxel data.
 * 
 * ============================================================================
 * DEPRECATION NOTICE:
 * This DualContouring implementation is deprecated and should not be used.
 * Use SimpleMesher instead for the following reasons:
 * 
 * 1. DualContouring has unresolved issues with arbitrary voxel placement (1cm increments)
 * 2. SimpleMesher provides more reliable watertight mesh generation
 * 3. SimpleMesher is better suited for our smoothing pipeline (levels 0-10)
 * 4. SimpleMesher guarantees exact voxel preservation
 * 
 * See: core/surface_gen/SimpleMesher.h (to be implemented)
 * ============================================================================
 * 
 * This implementation uses the dual contouring algorithm with sparse cell traversal.
 * Kept for reference only.
 * 
 * Original features:
 * - Variable voxel size support (1cm to 512cm)
 * - Watertight mesh generation
 * - Sharp feature preservation
 * - Multi-threaded processing
 * - Memory efficient sparse storage
 * - Cancellation support for long operations
 * 
 * See DualContouringSparse.md for detailed requirements and implementation notes.
 * 
 * Note: This implementation was formerly known as DualContouringSparse but has now
 * become the primary DualContouring implementation due to its superior performance.
 */
class DualContouring {
public:
    /**
     * Default constructor.
     * Initializes the dual contouring algorithm with default settings.
     */
    DualContouring();
    
    /**
     * Destructor.
     * Cleans up resources and cancels any active operations.
     */
    ~DualContouring();
    
    /**
     * Generate a mesh from a voxel grid using dual contouring.
     * 
     * This is the main entry point for mesh generation. The algorithm:
     * 1. Identifies cells near voxels (sparse traversal)
     * 2. Finds edge intersections where voxels meet empty space
     * 3. Places vertices using QEF solver for optimal positioning
     * 4. Generates quads connecting vertices
     * 5. Triangulates quads to create final mesh
     * 
     * @param grid The voxel grid to generate mesh from
     * @param settings Surface generation settings (quality, smoothing, etc.)
     * @return Generated mesh with vertices, triangles, and normals
     */
    Mesh generateMesh(const VoxelData::VoxelGrid& grid, const SurfaceSettings& settings);
    
    /**
     * Progress callback function signature.
     * Called periodically during mesh generation with progress value 0.0-1.0.
     */
    using ProgressCallback = std::function<void(float progress)>;
    
    /**
     * Set callback for progress reporting during mesh generation.
     * Useful for UI progress bars and long-running operations.
     * 
     * @param callback Function to call with progress updates (0.0-1.0)
     */
    void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
    
    /**
     * Cancel the current mesh generation operation.
     * Thread-safe - can be called from any thread to stop generation.
     */
    void cancel() { m_cancelled = true; }
    
    /**
     * Check if the current operation has been cancelled.
     * @return true if cancel() has been called
     */
    bool isCancelled() const { return m_cancelled; }
    
protected:
    // Edge table constants for dual contouring cube traversal
    static constexpr int EDGE_COUNT = 12;  ///< Number of edges per cube
    
    /**
     * Starting vertices for each of the 12 cube edges.
     * Each edge connects two vertices of a unit cube.
     */
    static const std::array<Math::IncrementCoordinates, EDGE_COUNT> EDGE_VERTICES;
    
    /**
     * Direction vectors for each of the 12 cube edges.
     * Combined with EDGE_VERTICES to define complete edge line segments.
     */
    static const std::array<Math::IncrementCoordinates, EDGE_COUNT> EDGE_DIRECTIONS;
    
    /**
     * The 8 vertex positions of a unit cube in increment coordinates.
     * Used for sampling voxel values at cube corners.
     */
    static const std::array<Math::IncrementCoordinates, 8> CUBE_VERTICES;
    
    /**
     * For each face (0-5), lists the 4 edges that bound that face.
     * Used during quad generation to connect vertices.
     */
    static const std::array<std::array<int, 4>, 6> FACE_EDGES;
    
    /**
     * Normal vectors for each of the 6 cube faces.
     * Used for orientation and winding order calculations.
     */
    static const std::array<Math::IncrementCoordinates, 6> FACE_NORMALS;
    
    /**
     * Data associated with each cell that potentially contains surface geometry.
     * Only cells near voxel boundaries are stored, making this memory efficient.
     */
    struct CellData {
        Math::IncrementCoordinates position;              ///< Cell position in increment coordinates
        std::array<HermiteData, EDGE_COUNT> edges;        ///< Hermite data for each of 12 edges
        Math::WorldCoordinates vertex;                    ///< Generated vertex position in world coordinates
        uint32_t vertexIndex;                            ///< Index in final mesh vertex array
        bool hasVertex;                                  ///< Whether this cell has a valid vertex
        
        CellData() : vertexIndex(0), hasVertex(false) {}
    };
    
    /**
     * Grid sampling interface for checking voxel occupancy and computing gradients.
     * Provides abstraction over the underlying voxel grid representation.
     */
    struct GridSampler {
        const VoxelData::VoxelGrid* grid;  ///< Pointer to voxel grid being processed
        float isoValue;                    ///< Threshold value for inside/outside determination
        
        /**
         * Sample the scalar field at a given position.
         * @param pos Position in increment coordinates
         * @return Scalar value (>isoValue = inside voxel, <isoValue = outside)
         */
        float sample(const Math::IncrementCoordinates& pos) const;
        
        /**
         * Check if a position is inside a voxel.
         * @param pos Position in increment coordinates  
         * @return true if position is inside any voxel
         */
        bool isInside(const Math::IncrementCoordinates& pos) const;
        
        /**
         * Compute gradient (surface normal direction) at a position.
         * @param pos Position in increment coordinates
         * @return Normalized gradient vector pointing outward from surface
         */
        Math::Vector3f gradient(const Math::IncrementCoordinates& pos) const;
    };
    
    /**
     * Quadratic Error Function solver for optimal vertex placement.
     * 
     * Given multiple edge intersections around a cell, solves for the vertex
     * position that minimizes error relative to all intersections. This is
     * what makes dual contouring superior to marching cubes for sharp features.
     */
    struct QEFSolver {
        std::vector<Math::WorldCoordinates> positions;  ///< Edge intersection positions
        std::vector<Math::Vector3f> normals;           ///< Surface normals at intersections
        
        /**
         * Add an edge intersection constraint to the QEF system.
         * @param pos World position of edge intersection
         * @param normal Surface normal at intersection point
         */
        void add(const Math::WorldCoordinates& pos, const Math::Vector3f& normal);
        
        /**
         * Solve the QEF system for optimal vertex position.
         * @return World coordinates of optimal vertex placement
         */
        Math::WorldCoordinates solve() const;
        
        /**
         * Clear all constraints for reuse.
         */
        void clear();
        
    private:
        /**
         * Compute mass point (average of all constraint positions).
         * Used as fallback when QEF system is ill-conditioned.
         */
        Math::WorldCoordinates computeMassPoint() const;
        
        /**
         * Solve the linear system ATA*x = ATb using Gaussian elimination.
         * @param ATA 3x3 symmetric matrix (stored as 6 elements)
         * @param ATb Right-hand side vector
         * @param x Output solution vector
         * @return true if system was solvable
         */
        bool solveSystem(float ATA[6], float ATb[3], float x[3]) const;
    };
    
    // Core member variables
    GridSampler m_sampler;                                   ///< Interface to voxel grid
    SurfaceSettings m_settings;                              ///< Current generation settings
    ProgressCallback m_progressCallback;                     ///< Progress reporting callback
    std::atomic<bool> m_cancelled;                          ///< Cancellation flag (thread-safe)
    
    // Working data - cleared between generations
    std::unordered_map<uint64_t, CellData> m_cellData;      ///< Sparse storage of active cells
    std::vector<Math::WorldCoordinates> m_vertices;          ///< Final mesh vertices
    std::vector<uint32_t> m_indices;                        ///< Final mesh triangle indices
    const VoxelData::VoxelGrid* m_currentGrid;              ///< Currently processed grid
    
    // Thread synchronization
    mutable std::mutex m_cellDataMutex;                     ///< Protects m_cellData during parallel processing
    
    /**
     * Extract edge intersections using sparse traversal.
     * 
     * This is the core optimization - instead of checking every cell in a regular grid,
     * we only check cells near actual voxels. For each occupied voxel, we generate
     * a small set of cells around it that might contain surface intersections.
     * 
     * @param grid Voxel grid to process
     */
    virtual void extractEdgeIntersections(const VoxelData::VoxelGrid& grid);
    
    /**
     * Generate vertices for all cells with edge intersections.
     * Uses QEF solver to place vertices optimally relative to edge intersections.
     */
    void generateVertices();
    
    /**
     * Generate quads (4-sided faces) connecting vertices.
     * Only processes cells that actually have vertices, making this efficient.
     */
    virtual void generateQuads();
    
    /**
     * Find intersection point along an edge where surface crosses.
     * @param v0 First edge vertex in increment coordinates
     * @param v1 Second edge vertex in increment coordinates
     * @param hermite Output: intersection position and normal
     * @return true if intersection found
     */
    bool findEdgeIntersection(const Math::IncrementCoordinates& v0, const Math::IncrementCoordinates& v1, 
                             HermiteData& hermite);
    
    /**
     * Linear interpolation along edge based on scalar field values.
     * @param val0 Scalar value at first vertex
     * @param val1 Scalar value at second vertex  
     * @param p0 Position of first vertex
     * @param p1 Position of second vertex
     * @return Interpolated position where surface crosses edge
     */
    Math::WorldCoordinates interpolateEdge(float val0, float val1, 
                                          const Math::WorldCoordinates& p0, const Math::WorldCoordinates& p1);
    
    /**
     * Generate vertex for a cell using QEF solver.
     * Collects all edge intersections around the cell and solves for optimal position.
     * @param cell Cell data to generate vertex for
     */
    void generateCellVertex(CellData& cell);
    
    /**
     * Check if a cell should generate a vertex.
     * @param cell Cell to check
     * @return true if cell has enough edge intersections to warrant a vertex
     */
    bool shouldGenerateVertex(const CellData& cell) const;
    
    /**
     * Generate a quad (4-sided face) for one face of a cell.
     * @param base Base cell position
     * @param faceIndex Which face to generate (0-5)
     */
    void generateFaceQuad(const Math::IncrementCoordinates& base, int faceIndex);
    
    /**
     * Check if a quad can be generated from 4 vertex positions.
     * @param v0 First quad vertex position
     * @param v1 Second quad vertex position
     * @param v2 Third quad vertex position
     * @param v3 Fourth quad vertex position
     * @return true if all 4 vertices exist and can form a valid quad
     */
    bool canGenerateQuad(const Math::IncrementCoordinates& v0, const Math::IncrementCoordinates& v1,
                        const Math::IncrementCoordinates& v2, const Math::IncrementCoordinates& v3) const;
    
    /**
     * Generate unique key for cell position for hash map storage.
     * @param pos Cell position in increment coordinates
     * @return 64-bit hash key for position
     */
    uint64_t cellKey(const Math::IncrementCoordinates& pos) const;
    
    /**
     * Get cell data for a position (mutable version).
     * @param pos Cell position in increment coordinates
     * @return Pointer to cell data, or nullptr if not found
     */
    CellData* getCell(const Math::IncrementCoordinates& pos);
    
    /**
     * Get cell data for a position (const version).
     * @param pos Cell position in increment coordinates
     * @return Pointer to cell data, or nullptr if not found
     */
    const CellData* getCell(const Math::IncrementCoordinates& pos) const;
    
    /**
     * Report progress to callback if one is set.
     * @param progress Progress value from 0.0 to 1.0
     */
    void reportProgress(float progress);
    
    /**
     * Detect if edge configuration represents a sharp feature.
     * @param edges List of edge intersections around a cell
     * @return true if this represents a sharp corner or edge
     */
    bool isSharpFeature(const std::vector<HermiteData>& edges) const;
    
    /**
     * Compute angle between two surface normals.
     * @param n1 First normal vector
     * @param n2 Second normal vector
     * @return Angle in radians between normals
     */
    float computeFeatureAngle(const Math::Vector3f& n1, const Math::Vector3f& n2) const;

private:
    /**
     * Build set of cells that need processing based on voxel positions.
     * 
     * For each occupied voxel, generates cells in a pattern around it that
     * might contain surface intersections. Cell increment matches voxel size
     * to ensure proper alignment.
     * 
     * @param grid Voxel grid to analyze
     * @return Hash set of cell keys for cells to process
     */
    std::unordered_set<uint64_t> buildActiveCellSet(const VoxelData::VoxelGrid& grid);
    
    /**
     * Process active cells in parallel for better performance.
     * 
     * Splits the cell list across multiple threads when beneficial.
     * Uses thread-safe access to shared cell data storage.
     * 
     * @param grid Voxel grid being processed
     * @param activeCells Set of cell keys to process
     */
    void processActiveCellsParallel(const VoxelData::VoxelGrid& grid,
                                   const std::unordered_set<uint64_t>& activeCells);
    
    /**
     * Process a single cell for edge intersections.
     * 
     * Checks all 12 edges of the cell for intersections with voxel boundaries.
     * Only stores cell data if at least one intersection is found.
     * 
     * @param cellPos Position of cell in increment coordinates
     * @param grid Voxel grid being processed
     */
    void processCell(const Math::IncrementCoordinates& cellPos,
                    const VoxelData::VoxelGrid& grid);
};

/**
 * Pre-computed lookup tables for dual contouring optimization.
 * Contains edge configurations, vertex offsets, and other constants
 * used throughout the dual contouring algorithm.
 */
class DualContouringTables {
public:
    /**
     * Edge table for marching cubes-like edge enumeration.
     * Maps 8-bit voxel occupancy patterns to edge intersection patterns.
     */
    static const int edgeTable[256];
    
    /**
     * Vertex offsets for the 8 corners of a unit cube.
     * Used as base positions for edge intersection calculations.
     */
    static const float vertexOffsets[8][3];
    
    /**
     * Edge connections defining which cube vertices each edge connects.
     * Each edge connects exactly 2 of the 8 cube vertices.
     */
    static const int edgeConnections[12][2];
    
    /**
     * For each face (0-5), lists the 4 edges that bound that face.
     * Used during quad generation to determine face topology.
     */
    static const int faceEdges[6][4];
    
    /**
     * Initialize lookup tables (called once at startup).
     * Must be called before using any DualContouring functionality.
     */
    static void initialize();
    
private:
    static bool s_initialized;  ///< Whether tables have been initialized
};

}
}