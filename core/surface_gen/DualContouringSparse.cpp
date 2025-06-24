#include "DualContouringSparse.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <vector>
#include <future>
#include <iostream>

namespace VoxelEditor {
namespace SurfaceGen {

void DualContouringSparse::extractEdgeIntersections(const VoxelData::VoxelGrid& grid) {
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
        logger.debugfc("DualContouringSparse", "No active cells to process");
        return;
    }
    
    Math::Vector3i dims = grid.getGridDimensions();
    logger.debugfc("DualContouringSparse", 
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
        logger.debugfc("DualContouringSparse", "Active cell %d: (%d, %d, %d)", count++, x, y, z);
        if (count >= 3) break;
    }
    
    // Process cells in parallel for better performance
    processActiveCellsParallel(grid, activeCells);
    
    std::cout << "DualContouringSparse: After processing, have " << m_cellData.size() << " cells with intersections" << std::endl;
}

std::unordered_set<uint64_t> DualContouringSparse::buildActiveCellSet(
    const VoxelData::VoxelGrid& grid) {
    
    std::unordered_set<uint64_t> activeCells;
    auto occupiedVoxels = grid.getAllVoxels();
    
    // Get grid dimensions to understand the scale
    Math::Vector3i dims = grid.getGridDimensions();
    
    // Debug logging
    auto& logger = Logging::Logger::getInstance();
    logger.debugfc("DualContouringSparse", "Building active cells for %zu voxels", occupiedVoxels.size());
    std::cout << "DualContouringSparse: Building active cells for " << occupiedVoxels.size() << " voxels" << std::endl;
    
    // For each occupied voxel, mark surrounding cells as active
    int voxelCount = 0;
    for (const auto& voxel : occupiedVoxels) {
        const Math::Vector3i& voxelPos = voxel.incrementPos.value();
        
        // Get voxel size for this specific voxel
        float voxelSizeMeters = VoxelData::getVoxelSize(voxel.resolution);
        int voxelSizeIncrements = static_cast<int>(voxelSizeMeters * 100.0f); // Convert to increments (cm)
        
        if (voxelCount < 3) {
            logger.debugfc("DualContouringSparse", "Voxel %d at increment pos (%d,%d,%d), size %d increments",
                voxelCount, voxelPos.x, voxelPos.y, voxelPos.z, voxelSizeIncrements);
            std::cout << "  Voxel " << voxelCount << " at (" << voxelPos.x << "," << voxelPos.y << "," << voxelPos.z 
                      << "), size=" << voxelSizeIncrements << " increments" << std::endl;
        }
        voxelCount++;
        
        // In dual contouring, cells check 8 vertices to form edges
        // A cell at (x,y,z) checks vertices from (x,y,z) to (x+1,y+1,z+1)
        // So a voxel at increment position (px,py,pz) affects cells
        // from (px-1,py-1,pz-1) to (px,py,pz)
        
        // For dual contouring, a cell at grid position (x,y,z) checks the 8 vertices
        // at positions (x,y,z) through (x+1,y+1,z+1) in increment space.
        // So a voxel at increment position affects cells whose vertex range overlaps the voxel.
        
        // CRITICAL FIX: Work directly in increment coordinates, not grid coordinates
        // The dual contouring cells work in increment space, where each cell is 1 increment
        // A voxel affects cells in a range around its position
        
        // Calculate workspace bounds in increment coordinates
        Math::Vector3f workspaceSize = Math::Vector3f(dims.x * 0.01f, dims.y * 0.01f, dims.z * 0.01f);
        int halfX_cm = static_cast<int>(workspaceSize.x * 100.0f / 2.0f);
        int halfZ_cm = static_cast<int>(workspaceSize.z * 100.0f / 2.0f);
        
        int minX = std::max(-halfX_cm, voxelPos.x - voxelSizeIncrements);
        int minY = std::max(0, voxelPos.y - voxelSizeIncrements);
        int minZ = std::max(-halfZ_cm, voxelPos.z - voxelSizeIncrements);
        int maxX = std::min(halfX_cm - 1, voxelPos.x + voxelSizeIncrements);
        int maxY = std::min(dims.y - 1, voxelPos.y + voxelSizeIncrements);
        int maxZ = std::min(halfZ_cm - 1, voxelPos.z + voxelSizeIncrements);
        
        if (voxelCount < 3) {
            std::cout << "  Cell range: (" << minX << "-" << maxX << ", " 
                      << minY << "-" << maxY << ", " << minZ << "-" << maxZ << ")" << std::endl;
        }
        
        // Add all cells in this range
        // Work directly in increment coordinates to match base class
        for (int z = minZ; z <= maxZ; ++z) {
            for (int y = minY; y <= maxY; ++y) {
                for (int x = minX; x <= maxX; ++x) {
                    // Create increment coordinates and use base class cellKey function
                    Math::IncrementCoordinates cellPos(x, y, z);
                    activeCells.insert(cellKey(cellPos));
                }
            }
        }
    }
    
    return activeCells;
}

void DualContouringSparse::processActiveCellsParallel(
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

void DualContouringSparse::processCell(const Math::IncrementCoordinates& cellPos,
                                      const VoxelData::VoxelGrid& grid) {
    
    CellData cell;
    cell.position = cellPos;
    bool hasIntersection = false;
    
    static int debugCount = 0;
    bool shouldDebug = (debugCount++ < 5);
    
    // Check all 12 edges of the cell
    for (int e = 0; e < EDGE_COUNT; ++e) {
        Math::IncrementCoordinates v0(cellPos.value() + EDGE_VERTICES[e].value());
        Math::IncrementCoordinates v1(v0.value() + EDGE_DIRECTIONS[e].value());
        
        // Check for sign change
        bool inside0 = m_sampler.isInside(v0);
        bool inside1 = m_sampler.isInside(v1);
        
        if (inside0 != inside1) {
            findEdgeIntersection(v0, v1, cell.edges[e]);
            hasIntersection = true;
        }
    }
    
    // Only store cells that have intersections
    if (hasIntersection) {
        std::lock_guard<std::mutex> lock(m_cellDataMutex);
        m_cellData[cellKey(cellPos)] = cell;
    }
}

}
}