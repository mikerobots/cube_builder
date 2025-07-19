#include "DualContouring.h"
#include "MeshBuilder.h"
#include "../../foundation/voxel_math/include/voxel_math/VoxelGridMath.h"
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
    
    // The grid's coordinate system is centered, so we don't need bounds checking here
    // The VoxelGrid::getVoxel function will handle bounds checking internally
    
    // Check if the position is inside any voxel
    bool hasVoxel = grid->isInsideVoxel(pos);
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
    
    // CRITICAL: getGridDimensions returns dimensions in 1cm units (since grid stores at 1cm granularity)
    // But for dual contouring, we need to work with dimensions in voxel resolution units
    Math::Vector3i gridDims = grid.getGridDimensions();
    float voxelSize = Math::VoxelGridMath::getVoxelSizeMeters(grid.getResolution());
    int voxelSizeCm = Math::VoxelGridMath::getVoxelSizeCm(grid.getResolution());
    
    // Convert grid dimensions from 1cm units to voxel resolution units
    Math::Vector3i dims(
        gridDims.x / voxelSizeCm,
        gridDims.y / voxelSizeCm,
        gridDims.z / voxelSizeCm
    );
    
    Logging::Logger::getInstance().debugfc("DualContouring", 
        "Starting dual contouring: gridDims(1cm)=%dx%dx%d, dims(voxels)=%dx%dx%d, voxelSize=%.3f, voxelSizeCm=%d", 
        gridDims.x, gridDims.y, gridDims.z, dims.x, dims.y, dims.z, voxelSize, voxelSizeCm);
    
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
    Logging::Logger::getInstance().debugfc("DualContouring", "Generated %zu quads, %zu indices", m_indices.size() / 4, m_indices.size());
    
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
    
    // CRITICAL: getGridDimensions returns dimensions in 1cm units
    // We need to work with dimensions in voxel resolution units
    Math::Vector3i gridDims = grid.getGridDimensions();
    int voxelSizeCm = Math::VoxelGridMath::getVoxelSizeCm(grid.getResolution());
    
    // For a voxel at (0,0,0), we need cells that properly surround it
    // A 32cm voxel at (0,0,0) occupies space from (0,0,0) to (32,32,32)
    // We need cells at positions like (-32,-32,-32), (0,0,0), (32,32,32), etc.
    // to ensure all edges of the voxel are detected
    
    // The grid spans from -250 to +250 in 1cm units
    // We need to create cells that align with voxel boundaries
    // For 32cm voxels, cells should be at multiples of 32: ..., -64, -32, 0, 32, 64, ...
    
    // Calculate the grid bounds for cell generation
    int halfGridX = gridDims.x / 2;  // 250
    int halfGridZ = gridDims.z / 2;  // 250
    
    // Calculate cell range to cover the entire grid with aligned cells
    // We need to go slightly beyond -250 to ensure coverage
    int minCellIndexX = -(halfGridX / voxelSizeCm + 1);  // -8 for 32cm cells
    int maxCellIndexX = halfGridX / voxelSizeCm + 1;     // 8 for 32cm cells
    int minCellIndexZ = -(halfGridZ / voxelSizeCm + 1);  // -8
    int maxCellIndexZ = halfGridZ / voxelSizeCm + 1;     // 8
    int maxCellIndexY = gridDims.y / voxelSizeCm + 1;    // For Y dimension
    
    int minX = minCellIndexX * voxelSizeCm;  // -256 for 32cm cells
    int minZ = minCellIndexZ * voxelSizeCm;  // -256
    // Need one cell below ground to detect bottom faces of voxels at Y=0
    int minY = -voxelSizeCm;  // One cell below ground
    
    // Calculate dimensions based on aligned grid
    int cellsX = maxCellIndexX - minCellIndexX + 1;  // Total cells in X
    int cellsZ = maxCellIndexZ - minCellIndexZ + 1;  // Total cells in Z
    int cellsY = maxCellIndexY + 2;  // Extra cell below ground + cells above
    
    Math::Vector3i dims(cellsX, cellsY, cellsZ);
    
    Logging::Logger::getInstance().debugfc("DualContouring", 
        "extractEdgeIntersections: gridDims=(%d,%d,%d), voxelSize=%dcm, minBounds=(%d,%d,%d), processing %dx%dx%d cells", 
        gridDims.x, gridDims.y, gridDims.z, voxelSizeCm, minX, minY, minZ, dims.x-1, dims.y-1, dims.z-1);
    
    // Process all cells
    int cellsWithEdges = 0;
    for (int z = 0; z < dims.z - 1; ++z) {
        for (int y = 0; y < dims.y - 1; ++y) {
            for (int x = 0; x < dims.x - 1; ++x) {
                if (m_cancelled) return;
                
                // Cell position needs to be in increment coordinates (1cm units)
                // Account for centered coordinate system
                Math::IncrementCoordinates cellPos(
                    minX + x * voxelSizeCm, 
                    minY + y * voxelSizeCm, 
                    minZ + z * voxelSizeCm
                );
                CellData& cell = m_cellData[cellKey(cellPos)];
                cell.position = cellPos;
                
                // Debug cells near origin for single voxel test
                bool isNearOrigin = (cellPos.x() >= -64 && cellPos.x() <= 64 &&
                                    cellPos.y() >= -32 && cellPos.y() <= 64 &&
                                    cellPos.z() >= -64 && cellPos.z() <= 64);
                
                int edgesFoundInCell = 0;
                
                // Check all 12 edges of the cell
                for (int e = 0; e < EDGE_COUNT; ++e) {
                    // Edge vertices are defined with unit offsets, scale by voxel size
                    Math::Vector3i edgeVertexOffset = EDGE_VERTICES[e].value() * voxelSizeCm;
                    Math::Vector3i edgeDirectionOffset = EDGE_DIRECTIONS[e].value() * voxelSizeCm;
                    
                    Math::IncrementCoordinates v0(cellPos.value() + edgeVertexOffset);
                    Math::IncrementCoordinates v1(v0.value() + edgeDirectionOffset);
                    
                    // Check for sign change
                    bool inside0 = m_sampler.isInside(v0);
                    bool inside1 = m_sampler.isInside(v1);
                    
                    if (inside0 != inside1) {
                        findEdgeIntersection(v0, v1, cell.edges[e]);
                        edgesFoundInCell++;
                        
                        // Debug cells near origin
                        if (isNearOrigin) {
                            Logging::Logger::getInstance().debugfc("DualContouring", 
                                "Cell(%d,%d,%d) edge %d: v0(%d,%d,%d)=%s, v1(%d,%d,%d)=%s", 
                                cellPos.x(), cellPos.y(), cellPos.z(), e,
                                v0.x(), v0.y(), v0.z(), inside0 ? "inside" : "outside",
                                v1.x(), v1.y(), v1.z(), inside1 ? "inside" : "outside");
                        }
                    }
                }
                
                if (edgesFoundInCell > 0) {
                    cellsWithEdges++;
                    if (isNearOrigin) {
                        Logging::Logger::getInstance().debugfc("DualContouring",
                            "Cell at (%d,%d,%d) found %d edge intersections",
                            cellPos.x(), cellPos.y(), cellPos.z(), edgesFoundInCell);
                    }
                }
            }
        }
    }
    
    std::cout << "\nDualContouring: Total cells with edge intersections: " << cellsWithEdges << "\n";
    
    // Debug: print cells near origin that have edges
    int cellsNearOrigin = 0;
    std::cout << "Cells near origin with edge intersections:\n";
    for (const auto& [key, cell] : m_cellData) {
        if (std::abs(cell.position.x()) <= 64 && std::abs(cell.position.y()) <= 64 && 
            std::abs(cell.position.z()) <= 64) {
            int edgeCount = 0;
            for (const auto& edge : cell.edges) {
                if (edge.hasIntersection) edgeCount++;
            }
            if (edgeCount > 0) {
                cellsNearOrigin++;
                std::cout << "  Cell at (" << cell.position.x() << "," << cell.position.y() 
                          << "," << cell.position.z() << ") has " << edgeCount << " edge intersections\n";
            }
        }
    }
    std::cout << "Total cells near origin with edges: " << cellsNearOrigin << "\n";
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
    // Convert cell position from increment to world coordinates
    Math::WorldCoordinates cellMinWorld = Math::CoordinateConverter::incrementToWorld(cell.position);
    
    // Get voxel size for proper cell bounds
    int voxelSizeCm = Math::VoxelGridMath::getVoxelSizeCm(m_currentGrid->getResolution());
    Math::IncrementCoordinates cellMaxIncrement(
        cell.position.x() + voxelSizeCm, 
        cell.position.y() + voxelSizeCm, 
        cell.position.z() + voxelSizeCm
    );
    Math::WorldCoordinates cellMaxWorld = Math::CoordinateConverter::incrementToWorld(cellMaxIncrement);
    
    Math::Vector3f vertexPos = vertex.value();
    vertexPos.x = std::max(cellMinWorld.value().x, std::min(cellMaxWorld.value().x, vertexPos.x));
    // Ensure Y is never below ground plane (Y=0)
    vertexPos.y = std::max(std::max(0.0f, cellMinWorld.value().y), std::min(cellMaxWorld.value().y, vertexPos.y));
    vertexPos.z = std::max(cellMinWorld.value().z, std::min(cellMaxWorld.value().z, vertexPos.z));
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
    // CRITICAL: getGridDimensions returns dimensions in 1cm units
    Math::Vector3i gridDims = m_sampler.grid->getGridDimensions();
    int voxelSizeCm = Math::VoxelGridMath::getVoxelSizeCm(m_sampler.grid->getResolution());
    
    // Use the same grid alignment as extractEdgeIntersections
    int halfGridX = gridDims.x / 2;  // 250
    int halfGridZ = gridDims.z / 2;  // 250
    
    // Calculate cell range to cover the entire grid with aligned cells
    int minCellIndexX = -(halfGridX / voxelSizeCm + 1);  // -8 for 32cm cells
    int maxCellIndexX = halfGridX / voxelSizeCm + 1;     // 8 for 32cm cells
    int minCellIndexZ = -(halfGridZ / voxelSizeCm + 1);  // -8
    int maxCellIndexZ = halfGridZ / voxelSizeCm + 1;     // 8
    int maxCellIndexY = gridDims.y / voxelSizeCm + 1;    // For Y dimension
    
    int minX = minCellIndexX * voxelSizeCm;  // -256 for 32cm cells
    int minZ = minCellIndexZ * voxelSizeCm;  // -256
    // Need one cell below ground to detect bottom faces of voxels at Y=0
    int minY = -voxelSizeCm;  // One cell below ground
    
    // Calculate dimensions based on aligned grid (same as in extractEdgeIntersections)
    int cellsX = maxCellIndexX - minCellIndexX + 1;  // Total cells in X
    int cellsZ = maxCellIndexZ - minCellIndexZ + 1;  // Total cells in Z
    int cellsY = maxCellIndexY + 2;  // Extra cells for Y coverage
    
    Math::Vector3i dims(cellsX, cellsY, cellsZ);
    
    std::cout << "\nGenerating quads: grid bounds (" << minX << "," << minY << "," << minZ 
              << "), dims " << dims.x << "x" << dims.y << "x" << dims.z << "\n";
    
    // Generate quads for each face direction
    for (int z = 0; z < dims.z - 1; ++z) {
        for (int y = 0; y < dims.y - 1; ++y) {
            for (int x = 0; x < dims.x - 1; ++x) {
                if (m_cancelled) return;
                
                // Base position needs to be in increment coordinates (1cm units)
                // Account for centered coordinate system
                Math::IncrementCoordinates base(
                    minX + x * voxelSizeCm, 
                    minY + y * voxelSizeCm, 
                    minZ + z * voxelSizeCm
                );
                
                // Check all 6 face directions
                // The canGenerateQuad function will determine if a face should be generated
                for (int face = 0; face < 6; ++face) {
                    generateFaceQuad(base, face);
                }
            }
        }
    }
    
    std::cout << "Total quads generated: " << m_indices.size() / 4 << "\n";
}

void DualContouring::generateFaceQuad(const Math::IncrementCoordinates& base, int faceIndex) {
    // Determine the four cells that share this face
    Math::IncrementCoordinates cells[4];
    
    // Get voxel size for proper offsets
    int voxelSizeCm = Math::VoxelGridMath::getVoxelSizeCm(m_sampler.grid->getResolution());
    
    // Debug logging for first few quads
    static int debugCount = 0;
    bool shouldDebug = (debugCount < 10);
    
    switch (faceIndex) {
        case 0: // Bottom face (XY plane, Z-)
            cells[0] = base;
            cells[1] = base + Math::IncrementCoordinates(voxelSizeCm, 0, 0);
            cells[2] = base + Math::IncrementCoordinates(voxelSizeCm, voxelSizeCm, 0);
            cells[3] = base + Math::IncrementCoordinates(0, voxelSizeCm, 0);
            break;
        case 1: // Top face (XY plane, Z+)
            cells[0] = base + Math::IncrementCoordinates(0, 0, voxelSizeCm);
            cells[1] = base + Math::IncrementCoordinates(voxelSizeCm, 0, voxelSizeCm);
            cells[2] = base + Math::IncrementCoordinates(voxelSizeCm, voxelSizeCm, voxelSizeCm);
            cells[3] = base + Math::IncrementCoordinates(0, voxelSizeCm, voxelSizeCm);
            break;
        case 2: // Front face (XZ plane, Y-)
            cells[0] = base;
            cells[1] = base + Math::IncrementCoordinates(voxelSizeCm, 0, 0);
            cells[2] = base + Math::IncrementCoordinates(voxelSizeCm, 0, voxelSizeCm);
            cells[3] = base + Math::IncrementCoordinates(0, 0, voxelSizeCm);
            break;
        case 3: // Back face (XZ plane, Y+)
            cells[0] = base + Math::IncrementCoordinates(0, voxelSizeCm, 0);
            cells[1] = base + Math::IncrementCoordinates(voxelSizeCm, voxelSizeCm, 0);
            cells[2] = base + Math::IncrementCoordinates(voxelSizeCm, voxelSizeCm, voxelSizeCm);
            cells[3] = base + Math::IncrementCoordinates(0, voxelSizeCm, voxelSizeCm);
            break;
        case 4: // Left face (YZ plane, X-)
            cells[0] = base;
            cells[1] = base + Math::IncrementCoordinates(0, voxelSizeCm, 0);
            cells[2] = base + Math::IncrementCoordinates(0, voxelSizeCm, voxelSizeCm);
            cells[3] = base + Math::IncrementCoordinates(0, 0, voxelSizeCm);
            break;
        case 5: // Right face (YZ plane, X+)
            cells[0] = base + Math::IncrementCoordinates(voxelSizeCm, 0, 0);
            cells[1] = base + Math::IncrementCoordinates(voxelSizeCm, voxelSizeCm, 0);
            cells[2] = base + Math::IncrementCoordinates(voxelSizeCm, voxelSizeCm, voxelSizeCm);
            cells[3] = base + Math::IncrementCoordinates(voxelSizeCm, 0, voxelSizeCm);
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
        // Debug: Log face generation for cells near origin
        bool nearOrigin = true;
        for (int i = 0; i < 4; ++i) {
            if (std::abs(cells[i].x()) > 64 || std::abs(cells[i].y()) > 64 || std::abs(cells[i].z()) > 64) {
                nearOrigin = false;
                break;
            }
        }
        
        if (nearOrigin && debugCount < 20) {
            const char* faceNames[] = {"Bottom", "Top", "Front", "Back", "Left", "Right"};
            std::cout << "Generating face " << faceNames[faceIndex] 
                      << ": cells (" << cells[0].x() << "," << cells[0].y() << "," << cells[0].z()
                      << "), (" << cells[1].x() << "," << cells[1].y() << "," << cells[1].z() 
                      << "), (" << cells[2].x() << "," << cells[2].y() << "," << cells[2].z()
                      << "), (" << cells[3].x() << "," << cells[3].y() << "," << cells[3].z() << ")\n";
            shouldDebug = true;
        }
        
        // Add quad indices
        m_indices.push_back(indices[0]);
        m_indices.push_back(indices[1]);
        m_indices.push_back(indices[2]);
        m_indices.push_back(indices[3]);
        
        if (shouldDebug) {
            debugCount++;
        }
    }
}

bool DualContouring::canGenerateQuad(const Math::IncrementCoordinates& v0, const Math::IncrementCoordinates& v1,
                                    const Math::IncrementCoordinates& v2, const Math::IncrementCoordinates& v3) const {
    // In dual contouring, a face is generated between 4 cells if:
    // 1. All 4 cells exist and have vertices
    // 2. The face represents a boundary of the isosurface
    
    const CellData* c0 = getCell(v0);
    const CellData* c1 = getCell(v1);
    const CellData* c2 = getCell(v2);
    const CellData* c3 = getCell(v3);
    
    // All cells must exist
    if (!c0 || !c1 || !c2 || !c3) return false;
    
    // All 4 cells must have vertices
    if (!c0->hasVertex || !c1->hasVertex || !c2->hasVertex || !c3->hasVertex) {
        return false;
    }
    
    // To avoid duplicate faces, we need to check if this face has already been generated
    // from the opposite direction. We can do this by ensuring we only generate faces
    // when the current cell is the "minimum" cell of the 4 cells sharing the face.
    // This is a common technique in dual contouring to avoid duplicates.
    
    // Find the minimum cell position
    Math::IncrementCoordinates minPos = v0;
    if (v1.x() < minPos.x() || (v1.x() == minPos.x() && v1.y() < minPos.y()) || 
        (v1.x() == minPos.x() && v1.y() == minPos.y() && v1.z() < minPos.z())) {
        minPos = v1;
    }
    if (v2.x() < minPos.x() || (v2.x() == minPos.x() && v2.y() < minPos.y()) || 
        (v2.x() == minPos.x() && v2.y() == minPos.y() && v2.z() < minPos.z())) {
        minPos = v2;
    }
    if (v3.x() < minPos.x() || (v3.x() == minPos.x() && v3.y() < minPos.y()) || 
        (v3.x() == minPos.x() && v3.y() == minPos.y() && v3.z() < minPos.z())) {
        minPos = v3;
    }
    
    // Only generate the face if v0 is the minimum cell
    // This ensures each face is generated exactly once
    bool isMinCell = (v0.x() == minPos.x() && v0.y() == minPos.y() && v0.z() == minPos.z());
    
    if (!isMinCell) return false;
    
    // Additional check: For a voxel at (0,0,0), we should only generate faces
    // for cells in the range (-32,-32,-32) to (0,0,0)
    // This prevents duplicate faces from cells further away
    
    // Get the center of the 4 cells to determine which face this represents
    int voxelSizeCm = Math::VoxelGridMath::getVoxelSizeCm(m_sampler.grid->getResolution());
    Math::IncrementCoordinates center(
        (v0.x() + v1.x() + v2.x() + v3.x()) / 4,
        (v0.y() + v1.y() + v2.y() + v3.y()) / 4,
        (v0.z() + v1.z() + v2.z() + v3.z()) / 4
    );
    
    // Check if this face is at a voxel boundary
    // For a 32cm voxel at (0,0,0), faces should be at:
    // X faces at x = 0 and x = 32
    // Y faces at y = 0 and y = 32
    // Z faces at z = 0 and z = 32
    
    // If the face center is at a multiple of voxelSize, it's a valid boundary
    bool atBoundary = false;
    if (center.x() % voxelSizeCm == 0 && v0.x() != v1.x()) {
        // X-aligned face
        atBoundary = true;
    } else if (center.y() % voxelSizeCm == 0 && v0.y() != v2.y()) {
        // Y-aligned face
        atBoundary = true;
    } else if (center.z() % voxelSizeCm == 0 && v0.z() != v1.z() && v0.z() != v2.z()) {
        // Z-aligned face
        atBoundary = true;
    }
    
    return atBoundary;
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