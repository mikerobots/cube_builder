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
#include <array>

namespace VoxelEditor {
namespace SurfaceGen {

class DualContouring {
public:
    DualContouring();
    ~DualContouring();
    
    // Generate mesh from voxel grid
    Mesh generateMesh(const VoxelData::VoxelGrid& grid, const SurfaceSettings& settings);
    
    // Set callbacks for progress reporting
    using ProgressCallback = std::function<void(float progress)>;
    void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
    
    // Control generation
    void cancel() { m_cancelled = true; }
    bool isCancelled() const { return m_cancelled; }
    
protected:
    // Edge directions
    static constexpr int EDGE_COUNT = 12;
    static const std::array<Math::IncrementCoordinates, EDGE_COUNT> EDGE_VERTICES;
    static const std::array<Math::IncrementCoordinates, EDGE_COUNT> EDGE_DIRECTIONS;
    
    // Cube vertex offsets
    static const std::array<Math::IncrementCoordinates, 8> CUBE_VERTICES;
    
    // Face data for quad generation
    static const std::array<std::array<int, 4>, 6> FACE_EDGES;
    static const std::array<Math::IncrementCoordinates, 6> FACE_NORMALS;
    
    // Internal structures
    struct CellData {
        Math::IncrementCoordinates position;
        std::array<HermiteData, EDGE_COUNT> edges;
        Math::WorldCoordinates vertex;  // Mesh vertex in world coordinates
        uint32_t vertexIndex;
        bool hasVertex;
        
        CellData() : vertexIndex(0), hasVertex(false) {}
    };
    
    // Grid sampling
    struct GridSampler {
        const VoxelData::VoxelGrid* grid;
        float isoValue;
        
        float sample(const Math::IncrementCoordinates& pos) const;
        bool isInside(const Math::IncrementCoordinates& pos) const;
        Math::Vector3f gradient(const Math::IncrementCoordinates& pos) const;
    };
    
    // QEF (Quadratic Error Function) solver
    struct QEFSolver {
        std::vector<Math::WorldCoordinates> positions;
        std::vector<Math::Vector3f> normals;
        
        void add(const Math::WorldCoordinates& pos, const Math::Vector3f& normal);
        Math::WorldCoordinates solve() const;
        void clear();
        
    private:
        Math::WorldCoordinates computeMassPoint() const;
        bool solveSystem(float ATA[6], float ATb[3], float x[3]) const;
    };
    
    // Member variables
    GridSampler m_sampler;
    SurfaceSettings m_settings;
    ProgressCallback m_progressCallback;
    bool m_cancelled;
    
    // Working data
    std::unordered_map<uint64_t, CellData> m_cellData;
    std::vector<Math::WorldCoordinates> m_vertices;
    std::vector<uint32_t> m_indices;
    const VoxelData::VoxelGrid* m_currentGrid;
    
    // Core algorithm steps
    virtual void extractEdgeIntersections(const VoxelData::VoxelGrid& grid);
    void generateVertices();
    virtual void generateQuads();
    
    // Edge processing
    bool findEdgeIntersection(const Math::IncrementCoordinates& v0, const Math::IncrementCoordinates& v1, 
                             HermiteData& hermite);
    Math::WorldCoordinates interpolateEdge(float val0, float val1, 
                                          const Math::WorldCoordinates& p0, const Math::WorldCoordinates& p1);
    
    // Vertex generation
    void generateCellVertex(CellData& cell);
    bool shouldGenerateVertex(const CellData& cell) const;
    
    // Quad generation
    void generateFaceQuad(const Math::IncrementCoordinates& base, int faceIndex);
    bool canGenerateQuad(const Math::IncrementCoordinates& v0, const Math::IncrementCoordinates& v1,
                        const Math::IncrementCoordinates& v2, const Math::IncrementCoordinates& v3) const;
    
    // Utility functions
    uint64_t cellKey(const Math::IncrementCoordinates& pos) const;
    CellData* getCell(const Math::IncrementCoordinates& pos);
    const CellData* getCell(const Math::IncrementCoordinates& pos) const;
    
    // Progress reporting
    void reportProgress(float progress);
    
    // Sharp feature detection
    bool isSharpFeature(const std::vector<HermiteData>& edges) const;
    float computeFeatureAngle(const Math::Vector3f& n1, const Math::Vector3f& n2) const;

private:
    // Keep some members truly private
};

// Dual contouring tables
class DualContouringTables {
public:
    // Edge table for marching cubes-like edge enumeration
    static const int edgeTable[256];
    
    // Vertex offsets for cube corners
    static const float vertexOffsets[8][3];
    
    // Edge connections
    static const int edgeConnections[12][2];
    
    // Face edges
    static const int faceEdges[6][4];
    
    // Initialize tables (called once)
    static void initialize();
    
private:
    static bool s_initialized;
};

}
}