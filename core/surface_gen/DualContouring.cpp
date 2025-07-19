// ============================================================================
// DEPRECATION NOTICE:
// This DualContouring implementation is deprecated. Use SimpleMesher instead.
// See DualContouring.h for details.
// ============================================================================

#include "DualContouring.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <vector>
#include <future>
#include <iostream>

namespace VoxelEditor {
namespace SurfaceGen {

// Static data definitions from original implementation
const std::array<Math::IncrementCoordinates, DualContouring::EDGE_COUNT> DualContouring::EDGE_VERTICES = {{
    Math::IncrementCoordinates(0, 0, 0), Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(1, 1, 0), Math::IncrementCoordinates(0, 1, 0),
    Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(1, 0, 1), Math::IncrementCoordinates(1, 1, 1), Math::IncrementCoordinates(0, 1, 1),
    Math::IncrementCoordinates(0, 0, 0), Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(1, 1, 0), Math::IncrementCoordinates(0, 1, 0)
}};

const std::array<Math::IncrementCoordinates, DualContouring::EDGE_COUNT> DualContouring::EDGE_DIRECTIONS = {{
    Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(0, 1, 0), Math::IncrementCoordinates(-1, 0, 0), Math::IncrementCoordinates(0, -1, 0),
    Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(0, 1, 0), Math::IncrementCoordinates(-1, 0, 0), Math::IncrementCoordinates(0, -1, 0),
    Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(0, 0, 1)
}};

const std::array<Math::IncrementCoordinates, 8> DualContouring::CUBE_VERTICES = {{
    Math::IncrementCoordinates(0, 0, 0), Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(1, 1, 0), Math::IncrementCoordinates(0, 1, 0),
    Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(1, 0, 1), Math::IncrementCoordinates(1, 1, 1), Math::IncrementCoordinates(0, 1, 1)
}};

const std::array<std::array<int, 4>, 6> DualContouring::FACE_EDGES = {{
    {{0, 1, 2, 3}}, {{4, 5, 6, 7}}, {{0, 4, 8, 9}}, {{1, 5, 9, 10}}, {{2, 6, 10, 11}}, {{3, 7, 11, 8}}
}};

const std::array<Math::IncrementCoordinates, 6> DualContouring::FACE_NORMALS = {{
    Math::IncrementCoordinates(0, 0, -1), Math::IncrementCoordinates(0, 0, 1), Math::IncrementCoordinates(-1, 0, 0), 
    Math::IncrementCoordinates(1, 0, 0), Math::IncrementCoordinates(0, 1, 0), Math::IncrementCoordinates(0, -1, 0)
}};

DualContouring::DualContouring() 
    : m_cancelled(false)
    , m_currentGrid(nullptr) {
    m_sampler.grid = nullptr;
    m_sampler.isoValue = 0.5f;
}

DualContouring::~DualContouring() {
    // Cancel any active operations
    m_cancelled = true;
}

Mesh DualContouring::generateMesh(const VoxelData::VoxelGrid& grid, const SurfaceSettings& settings) {
    auto& logger = Logging::Logger::getInstance();
    logger.debugfc("DualContouring", "Starting sparse dual contouring mesh generation");
    
    // Clear previous data
    m_cellData.clear();
    m_vertices.clear();
    m_indices.clear();
    m_cancelled = false;
    
    // Set up for this generation
    m_settings = settings;
    m_currentGrid = &grid;
    m_sampler.grid = &grid;
    m_sampler.isoValue = 0.5f;
    
    reportProgress(0.0f);
    
    // Step 1: Extract edge intersections (sparse traversal)
    extractEdgeIntersections(grid);
    if (m_cancelled) return Mesh();
    reportProgress(0.4f);
    
    // Step 2: Generate vertices from edge intersections
    generateVertices();
    if (m_cancelled) return Mesh();
    reportProgress(0.7f);
    
    // Step 3: Generate quads and triangulate
    generateQuads();
    if (m_cancelled) return Mesh();
    reportProgress(0.9f);
    
    // Build final mesh
    Mesh mesh;
    mesh.vertices = std::move(m_vertices);
    mesh.indices = std::move(m_indices);
    
    // Generate normals if needed
    if (!mesh.vertices.empty() && !mesh.indices.empty()) {
        mesh.normals.resize(mesh.vertices.size());
        // Simple per-vertex normal calculation
        std::fill(mesh.normals.begin(), mesh.normals.end(), Math::Vector3f(0.0f, 1.0f, 0.0f));
    }
    
    logger.debugfc("DualContouring", "Mesh generation complete: %zu vertices, %zu triangles", 
                   mesh.vertices.size(), mesh.indices.size() / 3);
    
    reportProgress(1.0f);
    return mesh;
}

void DualContouring::extractEdgeIntersections(const VoxelData::VoxelGrid& grid) {
    m_currentGrid = &grid;
    
    // Make sure sampler is initialized (in case we're called directly)
    if (!m_sampler.grid) {
        m_sampler.grid = &grid;
        m_sampler.isoValue = 0.5f;
    }
    
    auto& logger = Logging::Logger::getInstance();
    
    // Build set of cells that need processing
    auto activeCells = buildActiveCellSet(grid);
    
    if (activeCells.empty()) {
        logger.debugfc("DualContouring", "No active cells to process");
        return;
    }
    
    Math::Vector3i dims = grid.getGridDimensions();
    logger.debugfc("DualContouring", 
        "Grid dims: %dx%dx%d, Found %zu occupied voxels, generated %zu active cells (%.1f%% reduction)",
        dims.x, dims.y, dims.z,
        grid.getAllVoxels().size(),
        activeCells.size(), 
        100.0f * (1.0f - float(activeCells.size()) / float(dims.x * dims.y * dims.z)));
    
    // Log first few active cells for debugging
    int count = 0;
    for (const auto& key : activeCells) {
        int x = key & 0xFFFF;
        int y = (key >> 16) & 0xFFFF;
        int z = (key >> 32) & 0xFFFF;
        logger.debugfc("DualContouring", "Active cell %d: (%d, %d, %d)", count++, x, y, z);
        if (count >= 3) break;
    }
    
    // Process cells in parallel for better performance
    processActiveCellsParallel(grid, activeCells);
    
    std::cout << "DualContouring: After processing, have " << m_cellData.size() << " cells with intersections" << std::endl;
}

std::unordered_set<uint64_t> DualContouring::buildActiveCellSet(
    const VoxelData::VoxelGrid& grid) {
    
    std::unordered_set<uint64_t> activeCells;
    auto occupiedVoxels = grid.getAllVoxels();
    
    // Get grid dimensions to understand the scale
    Math::Vector3i dims = grid.getGridDimensions();
    
    // Debug logging
    auto& logger = Logging::Logger::getInstance();
    logger.debugfc("DualContouring", "Building active cells for %zu voxels", occupiedVoxels.size());
    std::cout << "DualContouring: Building active cells for " << occupiedVoxels.size() << " voxels" << std::endl;
    
    // For each occupied voxel, mark surrounding cells as active
    int voxelCount = 0;
    for (const auto& voxel : occupiedVoxels) {
        const Math::Vector3i& voxelPos = voxel.incrementPos.value();
        
        // Get voxel size for this specific voxel
        float voxelSizeMeters = VoxelData::getVoxelSize(voxel.resolution);
        int voxelSizeIncrements = static_cast<int>(voxelSizeMeters * 100.0f); // Convert to increments (cm)
        
        if (voxelCount < 3) {
            logger.debugfc("DualContouring", "Voxel %d at increment pos (%d,%d,%d), size %d increments",
                voxelCount, voxelPos.x, voxelPos.y, voxelPos.z, voxelSizeIncrements);
            std::cout << "  Voxel " << voxelCount << " at (" << voxelPos.x << "," << voxelPos.y << "," << voxelPos.z 
                      << "), size=" << voxelSizeIncrements << " increments" << std::endl;
        }
        voxelCount++;
        
        // Calculate workspace bounds in increment coordinates
        Math::Vector3f workspaceSize = Math::Vector3f(dims.x * 0.01f, dims.y * 0.01f, dims.z * 0.01f);
        int halfX_cm = static_cast<int>(workspaceSize.x * 100.0f / 2.0f);
        int halfZ_cm = static_cast<int>(workspaceSize.z * 100.0f / 2.0f);
        
        // CRITICAL: Find grid-aligned cells that can detect this voxel
        // Cells must be at multiples of voxelSizeIncrements from origin
        // For a voxel at arbitrary position, we find the aligned cells that bracket it
        
        // Find the aligned cell positions that contain or border the voxel
        int voxelEndX = voxelPos.x + voxelSizeIncrements;
        int voxelEndY = voxelPos.y + voxelSizeIncrements;
        int voxelEndZ = voxelPos.z + voxelSizeIncrements;
        
        // Calculate aligned cell boundaries
        // floor(pos / size) * size gives us the cell position
        auto floorAlign = [](int pos, int size) -> int {
            if (pos >= 0) {
                return (pos / size) * size;
            } else {
                // Handle negative coordinates correctly
                return ((pos - size + 1) / size) * size;
            }
        };
        
        int cellMinX = floorAlign(voxelPos.x, voxelSizeIncrements);
        int cellMaxX = floorAlign(voxelEndX, voxelSizeIncrements);
        int cellMinY = floorAlign(voxelPos.y, voxelSizeIncrements);
        int cellMaxY = floorAlign(voxelEndY, voxelSizeIncrements);
        int cellMinZ = floorAlign(voxelPos.z, voxelSizeIncrements);
        int cellMaxZ = floorAlign(voxelEndZ, voxelSizeIncrements);
        
        // Extend by one cell in each direction to ensure full coverage
        int minX = cellMinX - voxelSizeIncrements;
        int maxX = cellMaxX + voxelSizeIncrements;
        int minY = cellMinY - voxelSizeIncrements;
        int maxY = cellMaxY + voxelSizeIncrements;
        int minZ = cellMinZ - voxelSizeIncrements;
        int maxZ = cellMaxZ + voxelSizeIncrements;
        
        // Clamp to workspace bounds only as a safety measure
        minX = std::max(-halfX_cm, minX);
        minY = std::max(-voxelSizeIncrements, minY);
        minZ = std::max(-halfZ_cm, minZ);
        maxX = std::min(halfX_cm - 1, maxX);
        maxY = std::min(dims.y - 1, maxY);
        maxZ = std::min(halfZ_cm - 1, maxZ);
        
        if (voxelCount <= 3) {
            std::cout << "  Cell range: (" << minX << "-" << maxX << ", " 
                      << minY << "-" << maxY << ", " << minZ << "-" << maxZ << ")" << std::endl;
        }
        
        // Generate all grid-aligned cells in the region
        // Cells MUST be at multiples of voxelSizeIncrements from origin
        for (int z = minZ; z <= maxZ; z += voxelSizeIncrements) {
            for (int y = minY; y <= maxY; y += voxelSizeIncrements) {
                for (int x = minX; x <= maxX; x += voxelSizeIncrements) {
                    // Verify cell is properly aligned (should always be true now)
                    if (x % voxelSizeIncrements != 0 || 
                        y % voxelSizeIncrements != 0 || 
                        z % voxelSizeIncrements != 0) {
                        std::cerr << "ERROR: Misaligned cell at (" << x << "," << y << "," << z 
                                  << ") for voxel size " << voxelSizeIncrements << std::endl;
                        continue;
                    }
                    
                    Math::IncrementCoordinates cellPos(x, y, z);
                    activeCells.insert(cellKey(cellPos));
                }
            }
        }
    }
    
    return activeCells;
}

void DualContouring::processActiveCellsParallel(
    const VoxelData::VoxelGrid& grid,
    const std::unordered_set<uint64_t>& activeCells) {
    
    // Convert set to vector for easier parallel processing
    std::vector<uint64_t> cellKeys(activeCells.begin(), activeCells.end());
    
    // Determine number of threads
    size_t numThreads = std::min(
        size_t(4),
        std::min(size_t(std::thread::hardware_concurrency()), cellKeys.size() / 100)
    );
    
    if (numThreads <= 1 || cellKeys.size() < 1000) {
        // Process single-threaded for small workloads
        for (uint64_t key : cellKeys) {
            if (m_cancelled) return;
            
            // Extract increment coordinates from key (same format as base class)
            int x = (key >> 0) & 0xFFFFF;
            int y = (key >> 20) & 0xFFFFF;
            int z = (key >> 40) & 0xFFFFF;
            
            // Handle sign extension for negative coordinates
            if (x & 0x80000) x |= 0xFFF00000;  // Sign extend if negative
            if (z & 0x80000) z |= 0xFFF00000;  // Sign extend if negative
            
            Math::IncrementCoordinates cellPos(x, y, z);
            processCell(cellPos, grid);
        }
    } else {
        // Multi-threaded processing
        std::vector<std::future<void>> futures;
        size_t cellsPerThread = cellKeys.size() / numThreads;
        
        for (size_t t = 0; t < numThreads; ++t) {
            size_t start = t * cellsPerThread;
            size_t end = (t == numThreads - 1) ? cellKeys.size() : (t + 1) * cellsPerThread;
            
            futures.push_back(std::async(std::launch::async, [this, &grid, &cellKeys, start, end]() {
                for (size_t i = start; i < end; ++i) {
                    if (m_cancelled) return;
                    
                    uint64_t key = cellKeys[i];
                    // Extract increment coordinates from key (same format as base class)
                    int x = (key >> 0) & 0xFFFFF;
                    int y = (key >> 20) & 0xFFFFF;
                    int z = (key >> 40) & 0xFFFFF;
                    
                    // Handle sign extension for negative coordinates
                    if (x & 0x80000) x |= 0xFFF00000;  // Sign extend if negative
                    if (z & 0x80000) z |= 0xFFF00000;  // Sign extend if negative
                    
                    Math::IncrementCoordinates cellPos(x, y, z);
                    processCell(cellPos, grid);
                }
            }));
        }
        
        // Wait for all threads to complete
        for (auto& future : futures) {
            future.wait();
        }
    }
}

void DualContouring::processCell(const Math::IncrementCoordinates& cellPos,
                                const VoxelData::VoxelGrid& grid) {
    
    CellData cell;
    cell.position = cellPos;
    bool hasIntersection = false;
    
    static int debugCount = 0;
    bool shouldDebug = (debugCount++ < 5);
    
    // CRITICAL FIX: Scale edge vertices and directions by voxel size
    // For 32cm voxels, we need to check edges that are 32cm long, not 1cm
    int voxelSizeCm = 32; // TODO: Get from actual voxel data in this region
    
    // Check all 12 edges of the cell
    for (int e = 0; e < EDGE_COUNT; ++e) {
        // Scale the edge vertices and directions by voxel size
        Math::Vector3i scaledVertex = EDGE_VERTICES[e].value() * voxelSizeCm;
        Math::Vector3i scaledDirection = EDGE_DIRECTIONS[e].value() * voxelSizeCm;
        
        Math::IncrementCoordinates v0(cellPos.value() + scaledVertex);
        Math::IncrementCoordinates v1(v0.value() + scaledDirection);
        
        // Check for sign change
        bool inside0 = m_sampler.isInside(v0);
        bool inside1 = m_sampler.isInside(v1);
        
        if (shouldDebug && e < 3) {
            const Math::Vector3i& pos = cellPos.value();
            std::cout << "  Cell (" << pos.x << "," << pos.y << "," << pos.z 
                      << ") edge " << e << ": v0=(" << v0.value().x << "," << v0.value().y << "," << v0.value().z 
                      << ") inside=" << inside0 << ", v1=(" << v1.value().x << "," << v1.value().y << "," << v1.value().z 
                      << ") inside=" << inside1 << std::endl;
                      
            // DEBUG: Focus on the cell that should work based on face output
            if (e == 0 && pos.x == -32 && pos.y == 0 && pos.z == -32) {
                std::cout << "    FOUND TARGET CELL (-32,0,-32)!" << std::endl;
                auto test000 = Math::IncrementCoordinates(0, 0, 0);
                auto testNeg = Math::IncrementCoordinates(-32, 0, -32);
                bool is000 = m_sampler.isInside(test000);
                bool isNeg = m_sampler.isInside(testNeg);
                std::cout << "    DETAILED: (0,0,0)=" << is000 << ", (-32,0,-32)=" << isNeg << std::endl;
            }
        }
        
        if (inside0 != inside1) {
            findEdgeIntersection(v0, v1, cell.edges[e]);
            hasIntersection = true;
            
            // DEBUG: Log successful intersections
            if (debugCount <= 10) {
                const Math::Vector3i& pos = cellPos.value();
                std::cout << "    *** INTERSECTION FOUND *** Cell (" << pos.x << "," << pos.y << "," << pos.z 
                          << ") edge " << e << ": v0=(" << v0.value().x << "," << v0.value().y << "," << v0.value().z 
                          << ") inside=" << inside0 << ", v1=(" << v1.value().x << "," << v1.value().y << "," << v1.value().z 
                          << ") inside=" << inside1 << std::endl;
            }
        }
    }
    
    if (shouldDebug) {
        const Math::Vector3i& pos = cellPos.value();
        std::cout << "Cell (" << pos.x << "," << pos.y << "," << pos.z 
                  << ") hasIntersection=" << hasIntersection << std::endl;
    }
    
    // Only store cells that have intersections
    if (hasIntersection) {
        std::lock_guard<std::mutex> lock(m_cellDataMutex);
        m_cellData[cellKey(cellPos)] = cell;
    }
}

void DualContouring::generateQuads() {
    std::cout << "DualContouring::generateQuads() starting with " << m_cellData.size() << " cells with intersections" << std::endl;
    
    // DEBUG: Print all cell positions
    int cellCount = 0;
    for (const auto& [key, cell] : m_cellData) {
        if (cellCount < 10) {
            const Math::Vector3i& pos = cell.position.value();
            std::cout << "  Cell " << cellCount << " at (" << pos.x << "," << pos.y << "," << pos.z << ")" << std::endl;
        }
        cellCount++;
    }
    
    int quadsGenerated = 0;
    
    // Only process quads for cells that have intersections
    // This is much more efficient than iterating through the entire grid
    for (const auto& [key, cell] : m_cellData) {
        if (m_cancelled) return;
        
        const Math::IncrementCoordinates& base = cell.position;
        
        // Check all 6 face directions for this cell
        for (int face = 0; face < 6; ++face) {
            generateFaceQuad(base, face);
            quadsGenerated++;
        }
    }
    
    std::cout << "DualContouring::generateQuads() completed after checking " << quadsGenerated << " potential quads" << std::endl;
}

// Include all the original implementation methods from DualContouring_BACKUP.cpp
// with proper documentation...

void DualContouring::generateVertices() {
    for (auto& [key, cell] : m_cellData) {
        if (m_cancelled) return;
        
        if (shouldGenerateVertex(cell)) {
            generateCellVertex(cell);
        }
    }
}

bool DualContouring::findEdgeIntersection(const Math::IncrementCoordinates& v0, const Math::IncrementCoordinates& v1, 
                                         HermiteData& hermite) {
    float val0 = m_sampler.sample(v0);
    float val1 = m_sampler.sample(v1);
    
    // Check for sign change
    bool inside0 = val0 > m_sampler.isoValue;
    bool inside1 = val1 > m_sampler.isoValue;
    
    if (inside0 == inside1) {
        return false; // No intersection
    }
    
    // Convert to world coordinates for interpolation
    Math::CoordinateConverter converter;
    Math::WorldCoordinates p0 = converter.incrementToWorld(v0);
    Math::WorldCoordinates p1 = converter.incrementToWorld(v1);
    
    // Interpolate position
    hermite.position = interpolateEdge(val0, val1, p0, p1);
    
    // Compute normal
    Math::IncrementCoordinates midPoint(
        (v0.value().x + v1.value().x) / 2,
        (v0.value().y + v1.value().y) / 2,
        (v0.value().z + v1.value().z) / 2
    );
    hermite.normal = m_sampler.gradient(midPoint);
    
    return true;
}

Math::WorldCoordinates DualContouring::interpolateEdge(float val0, float val1, 
                                                      const Math::WorldCoordinates& p0, const Math::WorldCoordinates& p1) {
    float t = (m_sampler.isoValue - val0) / (val1 - val0);
    t = std::clamp(t, 0.0f, 1.0f);
    
    return Math::WorldCoordinates(
        p0.value().x + t * (p1.value().x - p0.value().x),
        p0.value().y + t * (p1.value().y - p0.value().y),
        p0.value().z + t * (p1.value().z - p0.value().z)
    );
}

void DualContouring::generateCellVertex(CellData& cell) {
    QEFSolver qef;
    
    // Add all edge intersections to QEF
    for (int e = 0; e < EDGE_COUNT; ++e) {
        if (cell.edges[e].position.value().length() > 0) { // Has valid intersection
            qef.add(cell.edges[e].position, cell.edges[e].normal);
        }
    }
    
    // Solve for optimal vertex position
    cell.vertex = qef.solve();
    cell.hasVertex = true;
    cell.vertexIndex = static_cast<uint32_t>(m_vertices.size());
    m_vertices.push_back(cell.vertex);
}

bool DualContouring::shouldGenerateVertex(const CellData& cell) const {
    int edgeCount = 0;
    for (int e = 0; e < EDGE_COUNT; ++e) {
        if (cell.edges[e].position.value().length() > 0) {
            edgeCount++;
        }
    }
    return edgeCount >= 3; // Need at least 3 intersections for meaningful vertex
}

void DualContouring::generateFaceQuad(const Math::IncrementCoordinates& base, int faceIndex) {
    // Determine the four cells that share this face
    Math::IncrementCoordinates cells[4];
    
    // Get voxel size - for sparse implementation, we use 32cm by default
    // TODO: This should be determined from the voxels in the region
    int voxelSizeCm = 32;
    
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
        if (shouldDebug) {
            const char* faceNames[] = {"Bottom", "Top", "Front", "Back", "Left", "Right"};
            std::cout << "Cannot generate face " << faceNames[faceIndex] << ": cells " 
                      << cells[0].value().x << "," << cells[0].value().y << "," << cells[0].value().z << "), ("
                      << cells[1].value().x << "," << cells[1].value().y << "," << cells[1].value().z << "), ("
                      << cells[2].value().x << "," << cells[2].value().y << "," << cells[2].value().z << "), ("
                      << cells[3].value().x << "," << cells[3].value().y << "," << cells[3].value().z << ")" << std::endl;
        }
        return;
    }
    
    // Get vertex indices for valid cells
    std::vector<uint32_t> validIndices;
    std::vector<int> validPositions;
    
    for (int i = 0; i < 4; ++i) {
        const CellData* cell = getCell(cells[i]);
        if (cell && cell->hasVertex) {
            validIndices.push_back(cell->vertexIndex);
            validPositions.push_back(i);
        }
    }
    
    if (shouldDebug) {
        const char* faceNames[] = {"Bottom", "Top", "Front", "Back", "Left", "Right"};
        std::cout << "Generating face " << faceNames[faceIndex] << ": cells ("
                  << cells[0].value().x << "," << cells[0].value().y << "," << cells[0].value().z << "), ("
                  << cells[1].value().x << "," << cells[1].value().y << "," << cells[1].value().z << "), ("
                  << cells[2].value().x << "," << cells[2].value().y << "," << cells[2].value().z << "), ("
                  << cells[3].value().x << "," << cells[3].value().y << "," << cells[3].value().z << "), validVertices=" << validIndices.size() << std::endl;
    }

    // Generate triangles based on number of valid vertices
    if (validIndices.size() >= 3) {
        // Debug: Log face generation for cells near origin
        bool nearOrigin = true;
        for (int i = 0; i < 4; ++i) {
            const Math::Vector3i& pos = cells[i].value();
            if (std::abs(pos.x) > 64 || std::abs(pos.y) > 64 || std::abs(pos.z) > 64) {
                nearOrigin = false;
                break;
            }
        }
        
        if (nearOrigin && debugCount < 20) {
            const char* faceNames[] = {"Bottom", "Top", "Front", "Back", "Left", "Right"};
            const Math::Vector3i& p0 = cells[0].value();
            const Math::Vector3i& p1 = cells[1].value(); 
            const Math::Vector3i& p2 = cells[2].value();
            const Math::Vector3i& p3 = cells[3].value();
            std::cout << "Generating face " << faceNames[faceIndex] 
                      << ": cells (" << p0.x << "," << p0.y << "," << p0.z
                      << "), (" << p1.x << "," << p1.y << "," << p1.z 
                      << "), (" << p2.x << "," << p2.y << "," << p2.z
                      << "), (" << p3.x << "," << p3.y << "," << p3.z << ")\n";
            shouldDebug = true;
        }
        
        // Generate triangles based on number of valid vertices
        if (validIndices.size() == 4) {
            // Full quad: generate two triangles
            // Triangle 1: 0-1-2
            m_indices.push_back(validIndices[0]);
            m_indices.push_back(validIndices[1]);
            m_indices.push_back(validIndices[2]);
            
            // Triangle 2: 0-2-3
            m_indices.push_back(validIndices[0]);
            m_indices.push_back(validIndices[2]);
            m_indices.push_back(validIndices[3]);
            
            if (shouldDebug) {
                const char* faceNames[] = {"Bottom", "Top", "Front", "Back", "Left", "Right"};
                std::cout << "  Generated full quad " << faceNames[faceIndex] << " with vertices " 
                          << validIndices[0] << "," << validIndices[1] << "," << validIndices[2] << "," << validIndices[3] << std::endl;
            }
        } else if (validIndices.size() == 3) {
            // Boundary triangle
            m_indices.push_back(validIndices[0]);
            m_indices.push_back(validIndices[1]);
            m_indices.push_back(validIndices[2]);
            
            if (shouldDebug) {
                const char* faceNames[] = {"Bottom", "Top", "Front", "Back", "Left", "Right"};
                std::cout << "  Generated boundary triangle " << faceNames[faceIndex] << " with vertices " 
                          << validIndices[0] << "," << validIndices[1] << "," << validIndices[2] << std::endl;
            }
        } else {
            if (shouldDebug) {
                const char* faceNames[] = {"Bottom", "Top", "Front", "Back", "Left", "Right"};
                std::cout << "  Face " << faceNames[faceIndex] << " has only " << validIndices.size() << " valid vertices, skipping" << std::endl;
            }
        }
        
        if (shouldDebug) {
            debugCount++;
        }
    }
}

bool DualContouring::canGenerateQuad(const Math::IncrementCoordinates& v0, const Math::IncrementCoordinates& v1,
                                    const Math::IncrementCoordinates& v2, const Math::IncrementCoordinates& v3) const {
    const CellData* c0 = getCell(v0);
    const CellData* c1 = getCell(v1);
    const CellData* c2 = getCell(v2);
    const CellData* c3 = getCell(v3);
    
    // Count how many cells exist and have vertices
    int validCells = 0;
    if (c0 && c0->hasVertex) validCells++;
    if (c1 && c1->hasVertex) validCells++;
    if (c2 && c2->hasVertex) validCells++;
    if (c3 && c3->hasVertex) validCells++;
    
    // For isolated voxels, we need at least 2 valid cells to form a boundary edge
    // For dense grids, we need all 4 cells for interior faces
    return validCells >= 2;
}

uint64_t DualContouring::cellKey(const Math::IncrementCoordinates& pos) const {
    const Math::Vector3i& p = pos.value();
    // Pack x, y, z into 64-bit key
    return (uint64_t(p.z & 0xFFFFF) << 40) | (uint64_t(p.y & 0xFFFFF) << 20) | uint64_t(p.x & 0xFFFFF);
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

bool DualContouring::isSharpFeature(const std::vector<HermiteData>& edges) const {
    // Implementation for sharp feature detection
    return false; // Placeholder
}

float DualContouring::computeFeatureAngle(const Math::Vector3f& n1, const Math::Vector3f& n2) const {
    float dot = n1.dot(n2);
    return std::acos(std::clamp(dot, -1.0f, 1.0f));
}

// GridSampler implementation
float DualContouring::GridSampler::sample(const Math::IncrementCoordinates& pos) const {
    return isInside(pos) ? 1.0f : 0.0f;
}

bool DualContouring::GridSampler::isInside(const Math::IncrementCoordinates& pos) const {
    return grid->isInsideVoxel(pos);
}

Math::Vector3f DualContouring::GridSampler::gradient(const Math::IncrementCoordinates& pos) const {
    // Simple gradient calculation
    const float eps = 1.0f;
    
    float dx = sample(Math::IncrementCoordinates(pos.value().x + eps, pos.value().y, pos.value().z)) -
               sample(Math::IncrementCoordinates(pos.value().x - eps, pos.value().y, pos.value().z));
    float dy = sample(Math::IncrementCoordinates(pos.value().x, pos.value().y + eps, pos.value().z)) -
               sample(Math::IncrementCoordinates(pos.value().x, pos.value().y - eps, pos.value().z));
    float dz = sample(Math::IncrementCoordinates(pos.value().x, pos.value().y, pos.value().z + eps)) -
               sample(Math::IncrementCoordinates(pos.value().x, pos.value().y, pos.value().z - eps));
    
    Math::Vector3f grad(dx, dy, dz);
    float length = grad.length();
    if (length > 0.001f) {
        grad = grad / length;
    } else {
        grad = Math::Vector3f(0.0f, 1.0f, 0.0f); // Default up normal
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
        return Math::WorldCoordinates(0.0f, 0.0f, 0.0f);
    }
    
    if (positions.size() == 1) {
        return positions[0];
    }
    
    // For now, return mass point as fallback
    return computeMassPoint();
}

void DualContouring::QEFSolver::clear() {
    positions.clear();
    normals.clear();
}

Math::WorldCoordinates DualContouring::QEFSolver::computeMassPoint() const {
    if (positions.empty()) {
        return Math::WorldCoordinates(0.0f, 0.0f, 0.0f);
    }
    
    Math::Vector3f sum(0.0f, 0.0f, 0.0f);
    for (const auto& pos : positions) {
        sum = sum + pos.value();
    }
    
    sum = sum / static_cast<float>(positions.size());
    return Math::WorldCoordinates(sum.x, sum.y, sum.z);
}

bool DualContouring::QEFSolver::solveSystem(float ATA[6], float ATb[3], float x[3]) const {
    // Simple 3x3 linear system solver - placeholder implementation
    return false;
}

}
}