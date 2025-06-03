#include "DualContouring.h"
#include "MeshBuilder.h"
#include <cmath>
#include <algorithm>

namespace VoxelEditor {
namespace SurfaceGen {

// Static member definitions
const std::array<Math::Vector3i, 8> DualContouring::CUBE_VERTICES = {{
    {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
}};

const std::array<Math::Vector3i, 12> DualContouring::EDGE_VERTICES = {{
    {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 0},  // Bottom edges
    {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {0, 0, 1},  // Top edges
    {0, 0, 0}, {0, 1, 0}, {1, 0, 0}, {0, 0, 0}   // Vertical edges
}};

const std::array<Math::Vector3i, 12> DualContouring::EDGE_DIRECTIONS = {{
    {1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0},  // Bottom edges
    {1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0},  // Top edges
    {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}     // Vertical edges
}};

const std::array<std::array<int, 4>, 6> DualContouring::FACE_EDGES = {{
    {0, 1, 2, 3},    // Bottom face
    {4, 5, 6, 7},    // Top face
    {0, 9, 4, 8},    // Front face
    {2, 10, 6, 11},  // Back face
    {3, 11, 7, 9},   // Left face
    {1, 8, 5, 10}    // Right face
}};

const std::array<Math::Vector3i, 6> DualContouring::FACE_NORMALS = {{
    {0, 0, -1}, {0, 0, 1},   // Bottom, Top
    {0, -1, 0}, {0, 1, 0},   // Front, Back
    {-1, 0, 0}, {1, 0, 0}    // Left, Right
}};

// GridSampler implementation
float DualContouring::GridSampler::sample(const Math::Vector3i& pos) const {
    if (!grid) return 0.0f;
    
    // Check if position is in bounds
    if (pos.x < 0 || pos.y < 0 || pos.z < 0 ||
        pos.x >= grid->getGridDimensions().x ||
        pos.y >= grid->getGridDimensions().y ||
        pos.z >= grid->getGridDimensions().z) {
        return 0.0f;
    }
    
    // Sample voxel value
    bool hasVoxel = grid->getVoxel(pos);
    return hasVoxel ? 1.0f : 0.0f;
}

bool DualContouring::GridSampler::isInside(const Math::Vector3i& pos) const {
    return sample(pos) > isoValue;
}

Math::Vector3f DualContouring::GridSampler::gradient(const Math::Vector3i& pos) const {
    // Compute gradient using central differences
    float dx = sample(pos + Math::Vector3i(1, 0, 0)) - sample(pos - Math::Vector3i(1, 0, 0));
    float dy = sample(pos + Math::Vector3i(0, 1, 0)) - sample(pos - Math::Vector3i(0, 1, 0));
    float dz = sample(pos + Math::Vector3i(0, 0, 1)) - sample(pos - Math::Vector3i(0, 0, 1));
    
    Math::Vector3f grad(dx, dy, dz);
    grad = grad * 0.5f; // Scale by 1/2h where h=1
    
    // Normalize if possible
    float length = grad.length();
    if (length > 0.0001f) {
        grad = grad / length;
    }
    
    return grad;
}

// QEFSolver implementation
void DualContouring::QEFSolver::add(const Math::Vector3f& pos, const Math::Vector3f& normal) {
    positions.push_back(pos);
    normals.push_back(normal);
}

Math::Vector3f DualContouring::QEFSolver::solve() const {
    if (positions.empty()) {
        return Math::Vector3f(0, 0, 0);
    }
    
    // Build the system ATA * x = ATb
    float ATA[6] = {0}; // Upper triangular part of symmetric matrix
    float ATb[3] = {0};
    
    for (size_t i = 0; i < positions.size(); ++i) {
        const Math::Vector3f& p = positions[i];
        const Math::Vector3f& n = normals[i];
        
        // ATA accumulation
        ATA[0] += n.x * n.x;
        ATA[1] += n.x * n.y;
        ATA[2] += n.x * n.z;
        ATA[3] += n.y * n.y;
        ATA[4] += n.y * n.z;
        ATA[5] += n.z * n.z;
        
        // ATb accumulation
        float b = n.dot(p);
        ATb[0] += n.x * b;
        ATb[1] += n.y * b;
        ATb[2] += n.z * b;
    }
    
    // Solve the system
    float x[3];
    if (solveSystem(ATA, ATb, x)) {
        return Math::Vector3f(x[0], x[1], x[2]);
    }
    
    // Fallback to mass point
    return computeMassPoint();
}

Math::Vector3f DualContouring::QEFSolver::computeMassPoint() const {
    if (positions.empty()) {
        return Math::Vector3f(0, 0, 0);
    }
    
    Math::Vector3f sum(0, 0, 0);
    for (const auto& pos : positions) {
        sum = sum + pos;
    }
    
    return sum / static_cast<float>(positions.size());
}

bool DualContouring::QEFSolver::solveSystem(float ATA[6], float ATb[3], float x[3]) const {
    // Solve 3x3 symmetric system using Cholesky decomposition
    // Matrix layout: [0 1 2]
    //                [1 3 4]
    //                [2 4 5]
    
    const float EPSILON = 1e-6f;
    
    // Cholesky decomposition
    float L[6]; // Lower triangular
    
    L[0] = std::sqrt(std::max(ATA[0], EPSILON));
    L[1] = ATA[1] / L[0];
    L[2] = ATA[2] / L[0];
    
    float temp = ATA[3] - L[1] * L[1];
    if (temp < EPSILON) return false;
    L[3] = std::sqrt(temp);
    
    L[4] = (ATA[4] - L[1] * L[2]) / L[3];
    
    temp = ATA[5] - L[2] * L[2] - L[4] * L[4];
    if (temp < EPSILON) return false;
    L[5] = std::sqrt(temp);
    
    // Forward substitution: L * y = ATb
    float y[3];
    y[0] = ATb[0] / L[0];
    y[1] = (ATb[1] - L[1] * y[0]) / L[3];
    y[2] = (ATb[2] - L[2] * y[0] - L[4] * y[1]) / L[5];
    
    // Back substitution: L^T * x = y
    x[2] = y[2] / L[5];
    x[1] = (y[1] - L[4] * x[2]) / L[3];
    x[0] = (y[0] - L[1] * x[1] - L[2] * x[2]) / L[0];
    
    return true;
}

void DualContouring::QEFSolver::clear() {
    positions.clear();
    normals.clear();
}

// DualContouring implementation
DualContouring::DualContouring() : m_cancelled(false), m_currentGrid(nullptr) {
}

DualContouring::~DualContouring() = default;

Mesh DualContouring::generateMesh(const VoxelData::VoxelGrid& grid, const SurfaceSettings& settings) {
    m_settings = settings;
    m_cancelled = false;
    m_sampler.grid = &grid;
    m_sampler.isoValue = 0.5f; // Use default iso value
    
    // Clear previous data
    m_cellData.clear();
    m_vertices.clear();
    m_indices.clear();
    
    // Step 1: Extract edge intersections
    reportProgress(0.0f);
    extractEdgeIntersections(grid);
    if (m_cancelled) return Mesh();
    
    // Step 2: Generate vertices
    reportProgress(0.33f);
    generateVertices();
    if (m_cancelled) return Mesh();
    
    // Step 3: Generate quads
    reportProgress(0.66f);
    generateQuads();
    if (m_cancelled) return Mesh();
    
    // Build final mesh
    reportProgress(0.9f);
    MeshBuilder builder;
    builder.beginMesh();
    
    // Convert vertices from grid coordinates to world coordinates
    float voxelSize = grid.getVoxelSize();
    for (const auto& vertex : m_vertices) {
        Math::Vector3f worldVertex = vertex * voxelSize;
        builder.addVertex(worldVertex);
    }
    
    for (size_t i = 0; i < m_indices.size(); i += 4) {
        builder.addQuad(m_indices[i], m_indices[i+1], m_indices[i+2], m_indices[i+3]);
    }
    
    Mesh mesh = builder.endMesh();
    
    // Apply smoothing if requested
    if (settings.smoothingIterations > 0) {
        mesh = MeshBuilder::smoothMesh(mesh, settings.smoothingIterations, 0.5f);
    }
    
    reportProgress(1.0f);
    return mesh;
}

void DualContouring::extractEdgeIntersections(const VoxelData::VoxelGrid& grid) {
    m_currentGrid = &grid;
    Math::Vector3i dims = grid.getGridDimensions();
    
    // Process all cells
    for (int z = 0; z < dims.z - 1; ++z) {
        for (int y = 0; y < dims.y - 1; ++y) {
            for (int x = 0; x < dims.x - 1; ++x) {
                if (m_cancelled) return;
                
                Math::Vector3i cellPos(x, y, z);
                CellData& cell = m_cellData[cellKey(cellPos)];
                cell.position = cellPos;
                
                // Check all 12 edges of the cell
                for (int e = 0; e < EDGE_COUNT; ++e) {
                    Math::Vector3i v0 = cellPos + EDGE_VERTICES[e];
                    Math::Vector3i v1 = v0 + EDGE_DIRECTIONS[e];
                    
                    // Check for sign change
                    bool inside0 = m_sampler.isInside(v0);
                    bool inside1 = m_sampler.isInside(v1);
                    
                    if (inside0 != inside1) {
                        findEdgeIntersection(v0, v1, cell.edges[e]);
                    }
                }
            }
        }
    }
}

bool DualContouring::findEdgeIntersection(const Math::Vector3i& v0, const Math::Vector3i& v1, 
                                         HermiteData& hermite) {
    float val0 = m_sampler.sample(v0);
    float val1 = m_sampler.sample(v1);
    
    // Check for sign change
    if ((val0 - m_sampler.isoValue) * (val1 - m_sampler.isoValue) >= 0) {
        hermite.hasIntersection = false;
        return false;
    }
    
    // Interpolate position
    Math::Vector3f p0(v0.x, v0.y, v0.z);
    Math::Vector3f p1(v1.x, v1.y, v1.z);
    hermite.position = interpolateEdge(val0, val1, p0, p1);
    
    // Compute normal at intersection
    Math::Vector3f n0 = m_sampler.gradient(v0);
    Math::Vector3f n1 = m_sampler.gradient(v1);
    
    float t = (m_sampler.isoValue - val0) / (val1 - val0);
    hermite.normal = n0 + (n1 - n0) * t;
    
    float length = hermite.normal.length();
    if (length > 0.0001f) {
        hermite.normal = hermite.normal / length;
    }
    
    hermite.value = m_sampler.isoValue;
    hermite.hasIntersection = true;
    
    return true;
}

Math::Vector3f DualContouring::interpolateEdge(float val0, float val1, 
                                              const Math::Vector3f& p0, const Math::Vector3f& p1) {
    float t = (m_sampler.isoValue - val0) / (val1 - val0);
    t = std::max(0.0f, std::min(1.0f, t)); // Clamp to [0, 1]
    return p0 + (p1 - p0) * t;
}

void DualContouring::generateVertices() {
    for (auto& [key, cell] : m_cellData) {
        if (m_cancelled) return;
        
        if (shouldGenerateVertex(cell)) {
            generateCellVertex(cell);
            cell.vertexIndex = static_cast<uint32_t>(m_vertices.size());
            m_vertices.push_back(cell.vertex);
            cell.hasVertex = true;
        }
    }
}

bool DualContouring::shouldGenerateVertex(const CellData& cell) const {
    // Check if any edge has an intersection
    for (const auto& edge : cell.edges) {
        if (edge.hasIntersection) {
            return true;
        }
    }
    return false;
}

void DualContouring::generateCellVertex(CellData& cell) {
    QEFSolver qef;
    
    // Add all edge intersections to QEF
    std::vector<HermiteData> activeEdges;
    for (const auto& edge : cell.edges) {
        if (edge.hasIntersection) {
            qef.add(edge.position, edge.normal);
            activeEdges.push_back(edge);
        }
    }
    
    // Solve for vertex position
    Math::Vector3f vertex = qef.solve();
    
    // Constrain vertex to cell bounds
    Math::Vector3f cellMin(cell.position.x, cell.position.y, cell.position.z);
    Math::Vector3f cellMax = cellMin + Math::Vector3f(1, 1, 1);
    
    vertex.x = std::max(cellMin.x, std::min(cellMax.x, vertex.x));
    vertex.y = std::max(cellMin.y, std::min(cellMax.y, vertex.y));
    vertex.z = std::max(cellMin.z, std::min(cellMax.z, vertex.z));
    
    // Check for sharp features
    if (m_settings.preserveSharpFeatures && isSharpFeature(activeEdges)) {
        // For sharp features, bias toward edge intersections
        Math::Vector3f edgeCenter(0, 0, 0);
        for (const auto& edge : activeEdges) {
            edgeCenter = edgeCenter + edge.position;
        }
        edgeCenter = edgeCenter / static_cast<float>(activeEdges.size());
        
        // Blend between QEF solution and edge center
        float sharpness = 0.7f; // Could be based on angle
        vertex = vertex * (1.0f - sharpness) + edgeCenter * sharpness;
    }
    
    cell.vertex = vertex;
}

bool DualContouring::isSharpFeature(const std::vector<HermiteData>& edges) const {
    if (edges.size() < 2) return false;
    
    // Check angle between normals
    for (size_t i = 0; i < edges.size(); ++i) {
        for (size_t j = i + 1; j < edges.size(); ++j) {
            float angle = computeFeatureAngle(edges[i].normal, edges[j].normal);
            float angleThresholdRadians = m_settings.sharpFeatureAngle * 3.14159f / 180.0f;
            if (angle > angleThresholdRadians) {
                return true;
            }
        }
    }
    
    return false;
}

float DualContouring::computeFeatureAngle(const Math::Vector3f& n1, const Math::Vector3f& n2) const {
    float dot = n1.dot(n2);
    dot = std::max(-1.0f, std::min(1.0f, dot)); // Clamp to [-1, 1]
    return std::acos(dot);
}

void DualContouring::generateQuads() {
    Math::Vector3i dims = m_sampler.grid->getGridDimensions();
    
    // Generate quads for each face direction
    for (int z = 0; z < dims.z - 1; ++z) {
        for (int y = 0; y < dims.y - 1; ++y) {
            for (int x = 0; x < dims.x - 1; ++x) {
                if (m_cancelled) return;
                
                Math::Vector3i base(x, y, z);
                
                // Check all 6 face directions
                for (int face = 0; face < 6; ++face) {
                    generateFaceQuad(base, face);
                }
            }
        }
    }
}

void DualContouring::generateFaceQuad(const Math::Vector3i& base, int faceIndex) {
    // Determine the four cells that share this face
    Math::Vector3i cells[4];
    
    switch (faceIndex) {
        case 0: // Bottom face (XY plane, Z-)
            cells[0] = base;
            cells[1] = base + Math::Vector3i(1, 0, 0);
            cells[2] = base + Math::Vector3i(1, 1, 0);
            cells[3] = base + Math::Vector3i(0, 1, 0);
            break;
        case 1: // Top face (XY plane, Z+)
            cells[0] = base + Math::Vector3i(0, 0, 1);
            cells[1] = base + Math::Vector3i(1, 0, 1);
            cells[2] = base + Math::Vector3i(1, 1, 1);
            cells[3] = base + Math::Vector3i(0, 1, 1);
            break;
        case 2: // Front face (XZ plane, Y-)
            cells[0] = base;
            cells[1] = base + Math::Vector3i(1, 0, 0);
            cells[2] = base + Math::Vector3i(1, 0, 1);
            cells[3] = base + Math::Vector3i(0, 0, 1);
            break;
        case 3: // Back face (XZ plane, Y+)
            cells[0] = base + Math::Vector3i(0, 1, 0);
            cells[1] = base + Math::Vector3i(1, 1, 0);
            cells[2] = base + Math::Vector3i(1, 1, 1);
            cells[3] = base + Math::Vector3i(0, 1, 1);
            break;
        case 4: // Left face (YZ plane, X-)
            cells[0] = base;
            cells[1] = base + Math::Vector3i(0, 1, 0);
            cells[2] = base + Math::Vector3i(0, 1, 1);
            cells[3] = base + Math::Vector3i(0, 0, 1);
            break;
        case 5: // Right face (YZ plane, X+)
            cells[0] = base + Math::Vector3i(1, 0, 0);
            cells[1] = base + Math::Vector3i(1, 1, 0);
            cells[2] = base + Math::Vector3i(1, 1, 1);
            cells[3] = base + Math::Vector3i(1, 0, 1);
            break;
    }
    
    // Check if we can generate a quad
    if (!canGenerateQuad(cells[0], cells[1], cells[2], cells[3])) {
        return;
    }
    
    // Get vertex indices
    uint32_t indices[4];
    bool allValid = true;
    
    for (int i = 0; i < 4; ++i) {
        const CellData* cell = getCell(cells[i]);
        if (!cell || !cell->hasVertex) {
            allValid = false;
            break;
        }
        indices[i] = cell->vertexIndex;
    }
    
    if (allValid) {
        // Add quad indices
        m_indices.push_back(indices[0]);
        m_indices.push_back(indices[1]);
        m_indices.push_back(indices[2]);
        m_indices.push_back(indices[3]);
    }
}

bool DualContouring::canGenerateQuad(const Math::Vector3i& v0, const Math::Vector3i& v1,
                                    const Math::Vector3i& v2, const Math::Vector3i& v3) const {
    // Check if the face has a sign change
    // This is a simplified check - a more robust version would check the actual edge
    
    const CellData* c0 = getCell(v0);
    const CellData* c1 = getCell(v1);
    const CellData* c2 = getCell(v2);
    const CellData* c3 = getCell(v3);
    
    // All cells must exist and have vertices
    if (!c0 || !c1 || !c2 || !c3) return false;
    
    // At least one edge should have an intersection
    // This is a simplified check
    return c0->hasVertex || c1->hasVertex || c2->hasVertex || c3->hasVertex;
}

uint64_t DualContouring::cellKey(const Math::Vector3i& pos) const {
    // Pack 3D position into 64-bit key
    uint64_t key = 0;
    key |= (uint64_t(pos.x) & 0xFFFFF) << 0;
    key |= (uint64_t(pos.y) & 0xFFFFF) << 20;
    key |= (uint64_t(pos.z) & 0xFFFFF) << 40;
    return key;
}

DualContouring::CellData* DualContouring::getCell(const Math::Vector3i& pos) {
    auto it = m_cellData.find(cellKey(pos));
    return (it != m_cellData.end()) ? &it->second : nullptr;
}

const DualContouring::CellData* DualContouring::getCell(const Math::Vector3i& pos) const {
    auto it = m_cellData.find(cellKey(pos));
    return (it != m_cellData.end()) ? &it->second : nullptr;
}

void DualContouring::reportProgress(float progress) {
    if (m_progressCallback) {
        m_progressCallback(progress);
    }
}

}
}