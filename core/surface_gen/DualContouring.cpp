#include "DualContouring.h"
#include "MeshBuilder.h"
#include <cmath>
#include <algorithm>

namespace VoxelEditor {
namespace SurfaceGen {

// Static member definitions
const std::array<Math::IncrementCoordinates, 8> DualContouring::CUBE_VERTICES = {{
    Math::IncrementCoordinates(0, 0, 0), Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(1, 1, 0), Math::IncrementCoordinates(0, 1, 0),
    Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(1, 0, 1), Math::IncrementCoordinates(1, 1, 1), Math::IncrementCoordinates(0, 1, 1)
}};

const std::array<Math::IncrementCoordinates, DualContouring::EDGE_COUNT> DualContouring::EDGE_VERTICES = {{
    Math::IncrementCoordinates(0, 0, 0), Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(0, 1, 0), Math::IncrementCoordinates(0, 0, 0),  // Bottom edges
    Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(1, 0, 1), Math::IncrementCoordinates(0, 1, 1), Math::IncrementCoordinates(0, 0, 1),  // Top edges
    Math::IncrementCoordinates(0, 0, 0), Math::IncrementCoordinates(0, 1, 0), Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(0, 0, 0)   // Vertical edges
}};

const std::array<Math::IncrementCoordinates, DualContouring::EDGE_COUNT> DualContouring::EDGE_DIRECTIONS = {{
    Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(0, 1, 0), Math::IncrementCoordinates(-1, 0, 0), Math::IncrementCoordinates(0, -1, 0),  // Bottom edges
    Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(0, 1, 0), Math::IncrementCoordinates(-1, 0, 0), Math::IncrementCoordinates(0, -1, 0),  // Top edges
    Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(0, 0, 1)     // Vertical edges
}};

const std::array<std::array<int, 4>, 6> DualContouring::FACE_EDGES = {{
    {0, 1, 2, 3},    // Bottom face
    {4, 5, 6, 7},    // Top face
    {0, 9, 4, 8},    // Front face
    {2, 10, 6, 11},  // Back face
    {3, 11, 7, 9},   // Left face
    {1, 8, 5, 10}    // Right face
}};

const std::array<Math::IncrementCoordinates, 6> DualContouring::FACE_NORMALS = {{
    Math::IncrementCoordinates(0, 0, -1), Math::IncrementCoordinates(0, 0, 1),   // Bottom, Top
    Math::IncrementCoordinates(0, -1, 0), Math::IncrementCoordinates(0, 1, 0),   // Front, Back
    Math::IncrementCoordinates(-1, 0, 0), Math::IncrementCoordinates(1, 0, 0)    // Left, Right
}};

// GridSampler implementation
float DualContouring::GridSampler::sample(const Math::IncrementCoordinates& pos) const {
    if (!grid) return 0.0f;
    
    // Check if position is in bounds
    const Math::Vector3i& gridPos = pos.value();
    if (gridPos.x < 0 || gridPos.y < 0 || gridPos.z < 0 ||
        gridPos.x >= grid->getGridDimensions().x ||
        gridPos.y >= grid->getGridDimensions().y ||
        gridPos.z >= grid->getGridDimensions().z) {
        return 0.0f;
    }
    
    // Sample voxel value
    bool hasVoxel = grid->getVoxel(pos);
    return hasVoxel ? 1.0f : 0.0f;
}

bool DualContouring::GridSampler::isInside(const Math::IncrementCoordinates& pos) const {
    return sample(pos) > isoValue;
}

Math::Vector3f DualContouring::GridSampler::gradient(const Math::IncrementCoordinates& pos) const {
    // Compute gradient using central differences
    Math::IncrementCoordinates offsetX(pos.value() + Math::Vector3i(1, 0, 0));
    Math::IncrementCoordinates negOffsetX(pos.value() - Math::Vector3i(1, 0, 0));
    Math::IncrementCoordinates offsetY(pos.value() + Math::Vector3i(0, 1, 0));
    Math::IncrementCoordinates negOffsetY(pos.value() - Math::Vector3i(0, 1, 0));
    Math::IncrementCoordinates offsetZ(pos.value() + Math::Vector3i(0, 0, 1));
    Math::IncrementCoordinates negOffsetZ(pos.value() - Math::Vector3i(0, 0, 1));
    
    float dx = sample(offsetX) - sample(negOffsetX);
    float dy = sample(offsetY) - sample(negOffsetY);
    float dz = sample(offsetZ) - sample(negOffsetZ);
    
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
void DualContouring::QEFSolver::add(const Math::WorldCoordinates& pos, const Math::Vector3f& normal) {
    positions.push_back(pos);
    normals.push_back(normal);
}

Math::WorldCoordinates DualContouring::QEFSolver::solve() const {
    if (positions.empty()) {
        return Math::WorldCoordinates(0, 0, 0);
    }
    
    // Build the system ATA * x = ATb
    float ATA[6] = {0}; // Upper triangular part of symmetric matrix
    float ATb[3] = {0};
    
    for (size_t i = 0; i < positions.size(); ++i) {
        const Math::WorldCoordinates& worldPos = positions[i];
        const Math::Vector3f& p = worldPos.value();
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
        return Math::WorldCoordinates(Math::Vector3f(x[0], x[1], x[2]));
    }
    
    // Fallback to mass point
    return computeMassPoint();
}

Math::WorldCoordinates DualContouring::QEFSolver::computeMassPoint() const {
    if (positions.empty()) {
        return Math::WorldCoordinates(0, 0, 0);
    }
    
    Math::Vector3f sum(0, 0, 0);
    for (const auto& pos : positions) {
        sum = sum + pos.value();
    }
    
    return Math::WorldCoordinates(sum / static_cast<float>(positions.size()));
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
    
    Math::Vector3i dims = grid.getGridDimensions();
    float voxelSize = Math::VoxelGridMath::getVoxelSizeMeters(grid.getResolution());
    Logging::Logger::getInstance().debugfc("DualContouring", 
        "Starting dual contouring: grid=%dx%dx%d, voxelSize=%.3f", 
        dims.x, dims.y, dims.z, voxelSize);
    
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
    Logging::Logger::getInstance().debugfc("DualContouring", "Extracted %zu edge intersections", m_cellData.size());
    generateVertices();
    if (m_cancelled) return Mesh();
    
    // Step 3: Generate quads
    reportProgress(0.66f);
    Logging::Logger::getInstance().debugfc("DualContouring", "Generated %zu vertices", m_vertices.size());
    generateQuads();
    if (m_cancelled) return Mesh();
    
    // Build final mesh
    reportProgress(0.9f);
    Logging::Logger::getInstance().debugfc("DualContouring", "Generated %zu quads", m_indices.size() / 4);
    
    MeshBuilder builder;
    builder.beginMesh();
    
    // Add vertices directly - they're already in world coordinates
    for (const auto& vertex : m_vertices) {
        builder.addVertex(vertex);
    }
    
    for (size_t i = 0; i < m_indices.size(); i += 4) {
        builder.addQuad(m_indices[i], m_indices[i+1], m_indices[i+2], m_indices[i+3]);
    }
    
    Mesh mesh = builder.endMesh();
    
    Logging::Logger::getInstance().debugfc("DualContouring", 
        "Final mesh: %zu vertices, %zu triangles", 
        mesh.vertices.size(), mesh.indices.size() / 3);
    
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
                
                Math::IncrementCoordinates cellPos(x, y, z);
                CellData& cell = m_cellData[cellKey(cellPos)];
                cell.position = cellPos;
                
                // Check all 12 edges of the cell
                for (int e = 0; e < EDGE_COUNT; ++e) {
                    Math::IncrementCoordinates v0(cellPos.value() + EDGE_VERTICES[e].value());
                    Math::IncrementCoordinates v1(v0.value() + EDGE_DIRECTIONS[e].value());
                    
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

bool DualContouring::findEdgeIntersection(const Math::IncrementCoordinates& v0, const Math::IncrementCoordinates& v1, 
                                         HermiteData& hermite) {
    float val0 = m_sampler.sample(v0);
    float val1 = m_sampler.sample(v1);
    
    // Check for sign change
    if ((val0 - m_sampler.isoValue) * (val1 - m_sampler.isoValue) >= 0) {
        hermite.hasIntersection = false;
        return false;
    }
    
    // Interpolate position - convert grid to world coordinates using proper converter
    Math::WorldCoordinates p0 = Math::CoordinateConverter::incrementToWorld(v0);
    Math::WorldCoordinates p1 = Math::CoordinateConverter::incrementToWorld(v1);
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

Math::WorldCoordinates DualContouring::interpolateEdge(float val0, float val1, 
                                                      const Math::WorldCoordinates& p0, const Math::WorldCoordinates& p1) {
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
    Math::WorldCoordinates vertex = qef.solve();
    
    // Constrain vertex to cell bounds
    Math::Vector3f cellMin(cell.position.x(), cell.position.y(), cell.position.z());
    Math::Vector3f cellMax = cellMin + Math::Vector3f(1, 1, 1);
    
    Math::Vector3f vertexPos = vertex.value();
    vertexPos.x = std::max(cellMin.x, std::min(cellMax.x, vertexPos.x));
    vertexPos.y = std::max(cellMin.y, std::min(cellMax.y, vertexPos.y));
    vertexPos.z = std::max(cellMin.z, std::min(cellMax.z, vertexPos.z));
    vertex = Math::WorldCoordinates(vertexPos);
    
    // Check for sharp features
    if (m_settings.preserveSharpFeatures && isSharpFeature(activeEdges)) {
        // For sharp features, bias toward edge intersections
        Math::WorldCoordinates edgeCenter(0, 0, 0);
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
                
                Math::IncrementCoordinates base(x, y, z);
                
                // Check all 6 face directions
                for (int face = 0; face < 6; ++face) {
                    generateFaceQuad(base, face);
                }
            }
        }
    }
}

void DualContouring::generateFaceQuad(const Math::IncrementCoordinates& base, int faceIndex) {
    // Determine the four cells that share this face
    Math::IncrementCoordinates cells[4];
    
    switch (faceIndex) {
        case 0: // Bottom face (XY plane, Z-)
            cells[0] = base;
            cells[1] = base + Math::IncrementCoordinates(1, 0, 0);
            cells[2] = base + Math::IncrementCoordinates(1, 1, 0);
            cells[3] = base + Math::IncrementCoordinates(0, 1, 0);
            break;
        case 1: // Top face (XY plane, Z+)
            cells[0] = base + Math::IncrementCoordinates(0, 0, 1);
            cells[1] = base + Math::IncrementCoordinates(1, 0, 1);
            cells[2] = base + Math::IncrementCoordinates(1, 1, 1);
            cells[3] = base + Math::IncrementCoordinates(0, 1, 1);
            break;
        case 2: // Front face (XZ plane, Y-)
            cells[0] = base;
            cells[1] = base + Math::IncrementCoordinates(1, 0, 0);
            cells[2] = base + Math::IncrementCoordinates(1, 0, 1);
            cells[3] = base + Math::IncrementCoordinates(0, 0, 1);
            break;
        case 3: // Back face (XZ plane, Y+)
            cells[0] = base + Math::IncrementCoordinates(0, 1, 0);
            cells[1] = base + Math::IncrementCoordinates(1, 1, 0);
            cells[2] = base + Math::IncrementCoordinates(1, 1, 1);
            cells[3] = base + Math::IncrementCoordinates(0, 1, 1);
            break;
        case 4: // Left face (YZ plane, X-)
            cells[0] = base;
            cells[1] = base + Math::IncrementCoordinates(0, 1, 0);
            cells[2] = base + Math::IncrementCoordinates(0, 1, 1);
            cells[3] = base + Math::IncrementCoordinates(0, 0, 1);
            break;
        case 5: // Right face (YZ plane, X+)
            cells[0] = base + Math::IncrementCoordinates(1, 0, 0);
            cells[1] = base + Math::IncrementCoordinates(1, 1, 0);
            cells[2] = base + Math::IncrementCoordinates(1, 1, 1);
            cells[3] = base + Math::IncrementCoordinates(1, 0, 1);
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

bool DualContouring::canGenerateQuad(const Math::IncrementCoordinates& v0, const Math::IncrementCoordinates& v1,
                                    const Math::IncrementCoordinates& v2, const Math::IncrementCoordinates& v3) const {
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

uint64_t DualContouring::cellKey(const Math::IncrementCoordinates& pos) const {
    // Pack 3D position into 64-bit key
    uint64_t key = 0;
    const Math::Vector3i& v = pos.value();
    key |= (uint64_t(v.x) & 0xFFFFF) << 0;
    key |= (uint64_t(v.y) & 0xFFFFF) << 20;
    key |= (uint64_t(v.z) & 0xFFFFF) << 40;
    return key;
}

DualContouring::CellData* DualContouring::getCell(const Math::IncrementCoordinates& pos) {
    auto it = m_cellData.find(cellKey(pos));
    return (it != m_cellData.end()) ? &it->second : nullptr;
}

const DualContouring::CellData* DualContouring::getCell(const Math::IncrementCoordinates& pos) const {
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