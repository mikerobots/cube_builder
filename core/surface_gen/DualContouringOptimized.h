#pragma once

#include "DualContouring.h"
#include <immintrin.h>  // For SIMD intrinsics
#include <tbb/parallel_for.h>
#include <tbb/blocked_range3d.h>
#include <vector>
#include <atomic>

namespace VoxelEditor {
namespace SurfaceGen {

/**
 * Optimized dual contouring implementation using:
 * 1. SIMD vectorization for edge processing
 * 2. Parallel processing with Intel TBB
 * 3. Sparse voxel traversal (only process occupied regions)
 * 4. Cache-friendly memory access patterns
 */
class DualContouringOptimized : public DualContouring {
public:
    DualContouringOptimized() = default;
    
    Mesh generateMesh(const VoxelData::VoxelGrid& grid, 
                     const SurfaceSettings& settings = SurfaceSettings::Default()) override;

private:
    // SIMD-optimized edge processing
    void extractEdgeIntersectionsSIMD(const VoxelData::VoxelGrid& grid);
    
    // Parallel voxel traversal using TBB
    void extractEdgeIntersectionsParallel(const VoxelData::VoxelGrid& grid);
    
    // Sparse voxel traversal - only process occupied chunks
    void extractEdgeIntersectionsSparse(const VoxelData::VoxelGrid& grid);
    
    // Process 4 edges at once using AVX2
    void processEdgesSIMD(const Math::IncrementCoordinates& cellPos, 
                         CellData& cell,
                         const float* voxelData);
    
    // Batch gradient computation using SIMD
    void computeGradientsSIMD(const Math::IncrementCoordinates* positions,
                              Math::Vector3f* gradients,
                              int count);
    
    // Cache-friendly chunk processing
    static constexpr int CHUNK_SIZE = 8;
    void processChunk(int chunkX, int chunkY, int chunkZ, 
                     const VoxelData::VoxelGrid& grid);
    
    // Thread-local storage for parallel processing
    struct ThreadData {
        std::vector<std::pair<uint64_t, CellData>> localCells;
        std::vector<Math::WorldCoordinates> localVertices;
        std::vector<uint32_t> localIndices;
    };
    
    std::vector<ThreadData> m_threadData;
    std::atomic<size_t> m_totalVertices{0};
};

// Implementation example for SIMD edge processing
inline void DualContouringOptimized::processEdgesSIMD(
    const Math::IncrementCoordinates& cellPos, 
    CellData& cell,
    const float* voxelData) {
    
    // Process 4 edges at once using AVX2
    __m256 v0_vals = _mm256_loadu_ps(&voxelData[0]);  // Load 8 vertex values
    __m256 iso = _mm256_set1_ps(m_sampler.isoValue);
    
    // Check for sign changes on 4 edges simultaneously
    __m256 diff0 = _mm256_sub_ps(v0_vals, iso);
    __m256 signs = _mm256_cmp_ps(diff0, _mm256_setzero_ps(), _CMP_LT_OQ);
    
    // Extract sign change mask
    int mask = _mm256_movemask_ps(signs);
    
    // Process edges that have sign changes
    if (mask != 0 && mask != 0xFF) {
        // Handle edge intersections...
    }
}

// Sparse traversal optimization
inline void DualContouringOptimized::extractEdgeIntersectionsSparse(
    const VoxelData::VoxelGrid& grid) {
    
    // Get all occupied voxels from the sparse octree
    auto occupiedVoxels = grid.getAllVoxels();
    
    // Build a set of cells that need processing
    std::unordered_set<uint64_t> cellsToProcess;
    
    for (const auto& voxel : occupiedVoxels) {
        // Add this cell and its 26 neighbors
        const Math::Vector3i& pos = voxel.incrementPos.value();
        
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    Math::IncrementCoordinates cellPos(
                        pos.x + dx, pos.y + dy, pos.z + dz);
                    cellsToProcess.insert(cellKey(cellPos));
                }
            }
        }
    }
    
    // Process only the cells that might have surface
    tbb::parallel_for_each(cellsToProcess.begin(), cellsToProcess.end(),
        [this, &grid](uint64_t key) {
            // Extract x, y, z from key
            int x = key & 0xFFFF;
            int y = (key >> 16) & 0xFFFF;
            int z = (key >> 32) & 0xFFFF;
            
            Math::IncrementCoordinates cellPos(x, y, z);
            CellData cell;
            cell.position = cellPos;
            
            // Process edges for this cell
            // ... edge processing code ...
        });
}

// Example of vectorized gradient computation
inline void DualContouringOptimized::computeGradientsSIMD(
    const Math::IncrementCoordinates* positions,
    Math::Vector3f* gradients,
    int count) {
    
    // Process 4 gradients at once
    for (int i = 0; i < count; i += 4) {
        // Load positions
        __m256 px = _mm256_setr_ps(
            positions[i].x(), positions[i+1].x(), 
            positions[i+2].x(), positions[i+3].x(),
            0, 0, 0, 0);
            
        // Sample +X and -X directions
        __m256 samples_px = /* sample at (x+1, y, z) for 4 positions */;
        __m256 samples_nx = /* sample at (x-1, y, z) for 4 positions */;
        
        // Compute X gradient component
        __m256 dx = _mm256_sub_ps(samples_px, samples_nx);
        __m256 scale = _mm256_set1_ps(0.5f);
        dx = _mm256_mul_ps(dx, scale);
        
        // Store results
        _mm_store_ps(&gradients[i].x, _mm256_extractf128_ps(dx, 0));
        
        // Repeat for Y and Z components...
    }
}

}
}