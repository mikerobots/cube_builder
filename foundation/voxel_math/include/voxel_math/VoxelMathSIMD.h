#pragma once

#include "../../../math/CoordinateTypes.h"
#include "../../../core/voxel_data/VoxelTypes.h"
#include "VoxelBounds.h"
#include <cstddef>

// Use Eigen for cross-platform optimized vector math
// Eigen automatically detects and uses the best available SIMD instructions

namespace VoxelEditor {
namespace VoxelData {
    class VoxelGrid;
}

namespace Math {

/**
 * Performance-critical operations using SIMD instructions where available.
 * Provides vectorized implementations for bulk voxel mathematics operations.
 */
class VoxelMathSIMD {
public:
    /**
     * Bulk coordinate conversions using SIMD when available
     * @param world Array of world coordinates to convert
     * @param[out] increment Array to store increment coordinates
     * @param count Number of coordinates to convert
     */
    static void worldToIncrementBatch(const WorldCoordinates* world, 
                                     IncrementCoordinates* increment,
                                     size_t count);
    
    /**
     * Bulk coordinate conversions using SIMD when available
     * @param increment Array of increment coordinates to convert
     * @param[out] world Array to store world coordinates
     * @param count Number of coordinates to convert
     */
    static void incrementToWorldBatch(const IncrementCoordinates* increment,
                                     WorldCoordinates* world,
                                     size_t count);
    
    /**
     * Bulk bounds calculations using SIMD when available
     * @param positions Array of voxel positions in increment coordinates
     * @param voxelSizeMeters Size of voxels in meters
     * @param[out] bounds Array to store calculated bounds
     * @param count Number of bounds to calculate
     */
    static void calculateBoundsBatch(const IncrementCoordinates* positions,
                                    float voxelSizeMeters,
                                    VoxelBounds* bounds,
                                    size_t count);
    
    /**
     * Bulk collision checks using SIMD when available
     * @param positions Array of positions to check
     * @param resolutions Array of resolutions for each position
     * @param grid Voxel grid to check against
     * @param[out] results Array to store collision results
     * @param count Number of positions to check
     */
    static void checkCollisionsBatch(const IncrementCoordinates* positions,
                                    const VoxelData::VoxelResolution* resolutions,
                                    const VoxelData::VoxelGrid& grid,
                                    bool* results,
                                    size_t count);
    
    /**
     * Bulk distance calculations using SIMD when available
     * @param positions1 Array of first positions
     * @param positions2 Array of second positions
     * @param[out] distances Array to store calculated distances
     * @param count Number of distance calculations
     */
    static void calculateDistancesBatch(const WorldCoordinates* positions1,
                                       const WorldCoordinates* positions2,
                                       float* distances,
                                       size_t count);
    
    /**
     * Bulk bounds intersection tests using SIMD when available
     * @param bounds1 Array of first bounds
     * @param bounds2 Array of second bounds
     * @param[out] results Array to store intersection results
     * @param count Number of intersection tests
     */
    static void testBoundsIntersectionsBatch(const VoxelBounds* bounds1,
                                            const VoxelBounds* bounds2,
                                            bool* results,
                                            size_t count);
    
    /**
     * Bulk point-in-bounds tests using SIMD when available
     * @param points Array of points to test
     * @param bounds Array of bounds to test against
     * @param[out] results Array to store test results
     * @param count Number of tests to perform
     */
    static void testPointInBoundsBatch(const WorldCoordinates* points,
                                      const VoxelBounds* bounds,
                                      bool* results,
                                      size_t count);
    
    /**
     * Bulk vector normalization using SIMD when available
     * @param[in,out] vectors Array of vectors to normalize
     * @param count Number of vectors to normalize
     */
    static void normalizeVectorsBatch(Vector3f* vectors, size_t count);
    
    /**
     * Bulk dot product calculations using SIMD when available
     * @param vectors1 Array of first vectors
     * @param vectors2 Array of second vectors
     * @param[out] results Array to store dot products
     * @param count Number of dot products to calculate
     */
    static void calculateDotProductsBatch(const Vector3f* vectors1,
                                         const Vector3f* vectors2,
                                         float* results,
                                         size_t count);
    
    /**
     * Check if SIMD is available on this platform
     * @return True if SIMD optimizations are available
     */
    static bool isSIMDAvailable();
    
    /**
     * Get the name of the SIMD instruction set being used
     * @return String describing the SIMD capabilities
     */
    static const char* getSIMDInstructionSet();
    
    /**
     * Get the optimal batch size for operations
     * @return Recommended batch size for best performance with Eigen
     */
    static size_t getOptimalBatchSize();
    
private:
    // Constants for coordinate conversions
    static constexpr float METERS_TO_CM = 100.0f;
    static constexpr float CM_TO_METERS = 0.01f;
    
    // Fallback implementations (scalar)
    static void worldToIncrementScalar(const WorldCoordinates* world,
                                      IncrementCoordinates* increment,
                                      size_t count);
    
    static void incrementToWorldScalar(const IncrementCoordinates* increment,
                                      WorldCoordinates* world,
                                      size_t count);
    
    static void calculateDistancesScalar(const WorldCoordinates* positions1,
                                        const WorldCoordinates* positions2,
                                        float* distances,
                                        size_t count);
    
    static void normalizeVectorsScalar(Vector3f* vectors, size_t count);
    
    static void calculateDotProductsScalar(const Vector3f* vectors1,
                                          const Vector3f* vectors2,
                                          float* results,
                                          size_t count);
    
    // Helper functions for memory alignment (less critical with Eigen but still useful)
    static bool isAligned(const void* ptr, size_t alignment);
    static size_t alignedSize(size_t size, size_t alignment);
};

} // namespace Math
} // namespace VoxelEditor