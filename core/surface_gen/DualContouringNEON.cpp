#include "DualContouringNEON.h"
#include "../../foundation/logging/Logger.h"
#include <chrono>
#include <algorithm>

namespace VoxelEditor {
namespace SurfaceGen {

DualContouringNEON::DualContouringNEON() : DualContouring() {
}

void DualContouringNEON::extractEdgeIntersections(const VoxelData::VoxelGrid& grid) {
    m_currentGrid = &grid;
    Math::Vector3i dims = grid.getGridDimensions();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process cells in chunks for better cache locality
    const int CHUNK_SIZE = 8;  // Process 8x8x8 chunks
    
    for (int cz = 0; cz < dims.z - 1; cz += CHUNK_SIZE) {
        for (int cy = 0; cy < dims.y - 1; cy += CHUNK_SIZE) {
            for (int cx = 0; cx < dims.x - 1; cx += CHUNK_SIZE) {
                // Process chunk
                int maxZ = std::min(cz + CHUNK_SIZE, dims.z - 1);
                int maxY = std::min(cy + CHUNK_SIZE, dims.y - 1);
                int maxX = std::min(cx + CHUNK_SIZE, dims.x - 1);
                
                for (int z = cz; z < maxZ; ++z) {
                    for (int y = cy; y < maxY; ++y) {
                        for (int x = cx; x < maxX; ++x) {
                            if (m_cancelled) return;
                            
                            Math::IncrementCoordinates cellPos(x, y, z);
                            CellData& cell = m_cellData[cellKey(cellPos)];
                            cell.position = cellPos;
                            
                            // Use NEON optimized edge processing
                            processEdgesNEON(cellPos, grid, cell);
                        }
                    }
                }
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    Logging::Logger::getInstance().debugfc("DualContouringNEON", 
        "Edge extraction completed in %lld ms, found %zu cells with intersections",
        duration.count(), m_cellData.size());
}

void DualContouringNEON::processEdgesNEON(const Math::IncrementCoordinates& cellPos,
                                          const VoxelData::VoxelGrid& grid,
                                          CellData& cell) {
    // First, batch sample all 8 vertices of the cell
    Math::IncrementCoordinates vertices[8];
    float values[8];
    
    for (int i = 0; i < 8; ++i) {
        vertices[i] = Math::IncrementCoordinates(cellPos.value() + CUBE_VERTICES[i].value());
    }
    
    batchSampleVertices(vertices, grid, values);
    
    // Process edges using NEON - process 4 at a time
    for (int e = 0; e < EDGE_COUNT; e += 4) {
        // Load vertex values for 4 edges
        float32x4_t v0_vals, v1_vals;
        
        // Edge vertex indices for first 4 edges
        float v0_temp[4] = {
            values[0], // Edge 0: vertex 0
            values[1], // Edge 1: vertex 1  
            values[3], // Edge 2: vertex 3
            values[2]  // Edge 3: vertex 2
        };
        
        float v1_temp[4] = {
            values[1], // Edge 0: vertex 1
            values[3], // Edge 1: vertex 3
            values[2], // Edge 2: vertex 2
            values[0]  // Edge 3: vertex 0
        };
        
        v0_vals = vld1q_f32(v0_temp);
        v1_vals = vld1q_f32(v1_temp);
        
        // Check for sign changes using NEON
        float32x4_t iso = vdupq_n_f32(m_sampler.isoValue);
        float32x4_t diff0 = vsubq_f32(v0_vals, iso);
        float32x4_t diff1 = vsubq_f32(v1_vals, iso);
        float32x4_t signs = vmulq_f32(diff0, diff1);
        
        // Check which edges have sign changes (negative product)
        uint32x4_t mask = vcltq_f32(signs, vdupq_n_f32(0.0f));
        
        // Extract mask values
        uint32_t maskArray[4];
        vst1q_u32(maskArray, mask);
        
        // Process edges that have intersections
        for (int i = 0; i < 4 && e + i < EDGE_COUNT; ++i) {
            if (maskArray[i]) {
                int edgeIdx = e + i;
                Math::IncrementCoordinates v0(cellPos.value() + EDGE_VERTICES[edgeIdx].value());
                Math::IncrementCoordinates v1(v0.value() + EDGE_DIRECTIONS[edgeIdx].value());
                
                // Use optimized intersection finding
                findEdgeIntersectionNEON(v0, v1, 
                    m_sampler.sample(v0), m_sampler.sample(v1),
                    cell.edges[edgeIdx]);
            }
        }
    }
}

void DualContouringNEON::batchSampleVertices(const Math::IncrementCoordinates vertices[8],
                                             const VoxelData::VoxelGrid& grid,
                                             float results[8]) {
    // Sample 8 vertices in a cache-friendly manner
    // Could be further optimized with NEON if sampling logic allows
    for (int i = 0; i < 8; ++i) {
        results[i] = m_sampler.sample(vertices[i]);
    }
}

bool DualContouringNEON::findEdgeIntersectionNEON(const Math::IncrementCoordinates& v0,
                                                  const Math::IncrementCoordinates& v1,
                                                  float val0, float val1,
                                                  HermiteData& hermite) {
    // Calculate interpolation parameter t
    float t = (m_sampler.isoValue - val0) / (val1 - val0);
    t = std::max(0.0f, std::min(1.0f, t));
    
    // Interpolate position
    Math::WorldCoordinates p0 = Math::CoordinateConverter::incrementToWorld(v0);
    Math::WorldCoordinates p1 = Math::CoordinateConverter::incrementToWorld(v1);
    
    // Use NEON for position interpolation
    float32x4_t pos0 = {p0.x(), p0.y(), p0.z(), 0};
    float32x4_t pos1 = {p1.x(), p1.y(), p1.z(), 0};
    float32x4_t t_vec = vdupq_n_f32(t);
    float32x4_t one_minus_t = vdupq_n_f32(1.0f - t);
    
    // result = pos0 * (1-t) + pos1 * t
    float32x4_t result = vaddq_f32(
        vmulq_f32(pos0, one_minus_t),
        vmulq_f32(pos1, t_vec)
    );
    
    float resultArray[4];
    vst1q_f32(resultArray, result);
    hermite.position = Math::WorldCoordinates(resultArray[0], resultArray[1], resultArray[2]);
    
    // Compute normal (simplified for now)
    Math::Vector3f n0 = m_sampler.gradient(v0);
    Math::Vector3f n1 = m_sampler.gradient(v1);
    
    // Use NEON for normal interpolation
    float32x4_t norm0 = {n0.x, n0.y, n0.z, 0};
    float32x4_t norm1 = {n1.x, n1.y, n1.z, 0};
    float32x4_t norm_result = vaddq_f32(
        vmulq_f32(norm0, one_minus_t),
        vmulq_f32(norm1, t_vec)
    );
    
    float normArray[4];
    vst1q_f32(normArray, norm_result);
    hermite.normal = Math::Vector3f(normArray[0], normArray[1], normArray[2]);
    hermite.normal = hermite.normal.normalized();
    
    hermite.value = m_sampler.isoValue;
    hermite.hasIntersection = true;
    
    return true;
}

} // namespace SurfaceGen
} // namespace VoxelEditor