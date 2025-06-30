#include "../include/voxel_math/VoxelMathSIMD.h"
#include "../include/voxel_math/VoxelCollision.h"
#include "../../math/CoordinateConverter.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace VoxelEditor {
namespace Math {

bool VoxelMathSIMD::isSIMDAvailable() {
#ifdef __AVX2__
    return true;
#elif defined(__SSE4_1__)
    return true;
#elif defined(__ARM_NEON)
    return true;
#else
    return false;
#endif
}

const char* VoxelMathSIMD::getSIMDInstructionSet() {
#ifdef __AVX2__
    return "AVX2";
#elif defined(__SSE4_1__)
    return "SSE4.1";
#elif defined(__ARM_NEON)
    return "ARM NEON";
#else
    return "Scalar (no SIMD)";
#endif
}

size_t VoxelMathSIMD::getOptimalBatchSize() {
#ifdef __AVX2__
    return 32;  // Process 32 elements at a time with AVX2
#elif defined(__SSE4_1__)
    return 16;  // Process 16 elements at a time with SSE
#elif defined(__ARM_NEON)
    return 16;  // Process 16 elements at a time with NEON
#else
    return 8;   // Small batches for scalar fallback
#endif
}

void VoxelMathSIMD::worldToIncrementBatch(const WorldCoordinates* world,
                                         IncrementCoordinates* increment,
                                         size_t count) {
    if (count == 0) return;
    
#ifdef __AVX2__
    if (isAligned(world, SIMD_ALIGNMENT) && isAligned(increment, SIMD_ALIGNMENT)) {
        worldToIncrementAVX(reinterpret_cast<const float*>(world),
                           reinterpret_cast<int*>(increment), count);
        return;
    }
#endif
    
#ifdef __SSE4_1__
    if (isAligned(world, SIMD_ALIGNMENT) && isAligned(increment, SIMD_ALIGNMENT)) {
        worldToIncrementSSE(reinterpret_cast<const float*>(world),
                           reinterpret_cast<int*>(increment), count);
        return;
    }
#endif
    
#ifdef __ARM_NEON
    worldToIncrementNEON(reinterpret_cast<const float*>(world),
                        reinterpret_cast<int*>(increment), count);
    return;
#endif
    
    // Fallback to scalar implementation
    worldToIncrementScalar(world, increment, count);
}

void VoxelMathSIMD::incrementToWorldBatch(const IncrementCoordinates* increment,
                                         WorldCoordinates* world,
                                         size_t count) {
    if (count == 0) return;
    
#ifdef __AVX2__
    if (isAligned(increment, SIMD_ALIGNMENT) && isAligned(world, SIMD_ALIGNMENT)) {
        incrementToWorldAVX(reinterpret_cast<const int*>(increment),
                           reinterpret_cast<float*>(world), count);
        return;
    }
#endif
    
#ifdef __SSE4_1__
    if (isAligned(increment, SIMD_ALIGNMENT) && isAligned(world, SIMD_ALIGNMENT)) {
        incrementToWorldSSE(reinterpret_cast<const int*>(increment),
                           reinterpret_cast<float*>(world), count);
        return;
    }
#endif
    
#ifdef __ARM_NEON
    incrementToWorldNEON(reinterpret_cast<const int*>(increment),
                        reinterpret_cast<float*>(world), count);
    return;
#endif
    
    // Fallback to scalar implementation
    incrementToWorldScalar(increment, world, count);
}

void VoxelMathSIMD::calculateBoundsBatch(const IncrementCoordinates* positions,
                                        float voxelSizeMeters,
                                        VoxelBounds* bounds,
                                        size_t count) {
    // This operation is complex and benefits less from SIMD
    // Use scalar implementation for now
    for (size_t i = 0; i < count; ++i) {
        bounds[i] = VoxelBounds(positions[i], voxelSizeMeters);
    }
}

void VoxelMathSIMD::checkCollisionsBatch(const IncrementCoordinates* positions,
                                        const VoxelData::VoxelResolution* resolutions,
                                        const VoxelData::VoxelGrid& grid,
                                        bool* results,
                                        size_t count) {
    // Collision checking involves complex logic that doesn't vectorize well
    // Use scalar implementation
    for (size_t i = 0; i < count; ++i) {
        results[i] = VoxelCollision::checkCollisionWithGrid(positions[i], resolutions[i], grid);
    }
}

void VoxelMathSIMD::calculateDistancesBatch(const WorldCoordinates* positions1,
                                           const WorldCoordinates* positions2,
                                           float* distances,
                                           size_t count) {
    if (count == 0) return;
    
#ifdef __AVX2__
    calculateDistancesAVX(reinterpret_cast<const float*>(positions1),
                         reinterpret_cast<const float*>(positions2),
                         distances, count);
    return;
#endif
    
#ifdef __SSE4_1__
    calculateDistancesSSE(reinterpret_cast<const float*>(positions1),
                         reinterpret_cast<const float*>(positions2),
                         distances, count);
    return;
#endif
    
#ifdef __ARM_NEON
    calculateDistancesNEON(reinterpret_cast<const float*>(positions1),
                          reinterpret_cast<const float*>(positions2),
                          distances, count);
    return;
#endif
    
    // Fallback to scalar implementation
    calculateDistancesScalar(positions1, positions2, distances, count);
}

void VoxelMathSIMD::testBoundsIntersectionsBatch(const VoxelBounds* bounds1,
                                                const VoxelBounds* bounds2,
                                                bool* results,
                                                size_t count) {
    // Bounds intersection involves complex logic, use scalar for now
    for (size_t i = 0; i < count; ++i) {
        results[i] = bounds1[i].intersects(bounds2[i]);
    }
}

void VoxelMathSIMD::testPointInBoundsBatch(const WorldCoordinates* points,
                                          const VoxelBounds* bounds,
                                          bool* results,
                                          size_t count) {
    // Point-in-bounds testing involves complex logic, use scalar for now
    for (size_t i = 0; i < count; ++i) {
        results[i] = bounds[i].contains(points[i]);
    }
}

void VoxelMathSIMD::normalizeVectorsBatch(Vector3f* vectors, size_t count) {
    if (count == 0) return;
    
#ifdef __AVX2__
    normalizeVectorsAVX(reinterpret_cast<float*>(vectors), count);
    return;
#endif
    
#ifdef __SSE4_1__
    normalizeVectorsSSE(reinterpret_cast<float*>(vectors), count);
    return;
#endif
    
#ifdef __ARM_NEON
    normalizeVectorsNEON(reinterpret_cast<float*>(vectors), count);
    return;
#endif
    
    // Fallback to scalar implementation
    normalizeVectorsScalar(vectors, count);
}

void VoxelMathSIMD::calculateDotProductsBatch(const Vector3f* vectors1,
                                             const Vector3f* vectors2,
                                             float* results,
                                             size_t count) {
    if (count == 0) return;
    
#ifdef __AVX2__
    calculateDotProductsAVX(reinterpret_cast<const float*>(vectors1),
                           reinterpret_cast<const float*>(vectors2),
                           results, count);
    return;
#endif
    
#ifdef __SSE4_1__
    calculateDotProductsSSE(reinterpret_cast<const float*>(vectors1),
                           reinterpret_cast<const float*>(vectors2),
                           results, count);
    return;
#endif
    
#ifdef __ARM_NEON
    calculateDotProductsNEON(reinterpret_cast<const float*>(vectors1),
                            reinterpret_cast<const float*>(vectors2),
                            results, count);
    return;
#endif
    
    // Fallback to scalar implementation
    calculateDotProductsScalar(vectors1, vectors2, results, count);
}

// Scalar implementations
void VoxelMathSIMD::worldToIncrementScalar(const WorldCoordinates* world,
                                          IncrementCoordinates* increment,
                                          size_t count) {
    for (size_t i = 0; i < count; ++i) {
        increment[i] = CoordinateConverter::worldToIncrement(world[i]);
    }
}

void VoxelMathSIMD::incrementToWorldScalar(const IncrementCoordinates* increment,
                                          WorldCoordinates* world,
                                          size_t count) {
    for (size_t i = 0; i < count; ++i) {
        world[i] = CoordinateConverter::incrementToWorld(increment[i]);
    }
}

void VoxelMathSIMD::calculateDistancesScalar(const WorldCoordinates* positions1,
                                            const WorldCoordinates* positions2,
                                            float* distances,
                                            size_t count) {
    for (size_t i = 0; i < count; ++i) {
        const Vector3f& p1 = positions1[i].value();
        const Vector3f& p2 = positions2[i].value();
        Vector3f diff = p1 - p2;
        distances[i] = diff.length();
    }
}

void VoxelMathSIMD::normalizeVectorsScalar(Vector3f* vectors, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        vectors[i].normalize();
    }
}

void VoxelMathSIMD::calculateDotProductsScalar(const Vector3f* vectors1,
                                              const Vector3f* vectors2,
                                              float* results,
                                              size_t count) {
    for (size_t i = 0; i < count; ++i) {
        results[i] = vectors1[i].dot(vectors2[i]);
    }
}

// SIMD implementations
#ifdef __SSE4_1__
void VoxelMathSIMD::worldToIncrementSSE(const float* world, int* increment, size_t count) {
    const __m128 scale = _mm_set1_ps(METERS_TO_CM);
    
    size_t simdCount = (count * 3) / 4 * 4;  // Process in groups of 4 floats
    
    for (size_t i = 0; i < simdCount; i += 4) {
        __m128 worldVec = _mm_load_ps(&world[i]);
        __m128 scaled = _mm_mul_ps(worldVec, scale);
        __m128i rounded = _mm_cvtps_epi32(scaled);
        _mm_store_si128(reinterpret_cast<__m128i*>(&increment[i]), rounded);
    }
    
    // Handle remaining elements
    for (size_t i = simdCount; i < count * 3; ++i) {
        increment[i] = static_cast<int>(std::round(world[i] * METERS_TO_CM));
    }
}

void VoxelMathSIMD::incrementToWorldSSE(const int* increment, float* world, size_t count) {
    const __m128 scale = _mm_set1_ps(CM_TO_METERS);
    
    size_t simdCount = (count * 3) / 4 * 4;
    
    for (size_t i = 0; i < simdCount; i += 4) {
        __m128i incrementVec = _mm_load_si128(reinterpret_cast<const __m128i*>(&increment[i]));
        __m128 converted = _mm_cvtepi32_ps(incrementVec);
        __m128 scaled = _mm_mul_ps(converted, scale);
        _mm_store_ps(&world[i], scaled);
    }
    
    // Handle remaining elements
    for (size_t i = simdCount; i < count * 3; ++i) {
        world[i] = increment[i] * CM_TO_METERS;
    }
}

void VoxelMathSIMD::calculateDistancesSSE(const float* pos1, const float* pos2, 
                                         float* distances, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        size_t idx = i * 3;
        __m128 p1 = _mm_set_ps(0.0f, pos1[idx + 2], pos1[idx + 1], pos1[idx]);
        __m128 p2 = _mm_set_ps(0.0f, pos2[idx + 2], pos2[idx + 1], pos2[idx]);
        __m128 diff = _mm_sub_ps(p1, p2);
        __m128 squared = _mm_mul_ps(diff, diff);
        
        // Sum the components
        __m128 sum = _mm_hadd_ps(squared, squared);
        sum = _mm_hadd_ps(sum, sum);
        
        float result;
        _mm_store_ss(&result, _mm_sqrt_ss(sum));
        distances[i] = result;
    }
}

void VoxelMathSIMD::normalizeVectorsSSE(float* vectors, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        size_t idx = i * 3;
        __m128 vec = _mm_set_ps(0.0f, vectors[idx + 2], vectors[idx + 1], vectors[idx]);
        __m128 squared = _mm_mul_ps(vec, vec);
        
        // Sum the components
        __m128 sum = _mm_hadd_ps(squared, squared);
        sum = _mm_hadd_ps(sum, sum);
        
        __m128 length = _mm_sqrt_ss(sum);
        __m128 lengthBcast = _mm_shuffle_ps(length, length, 0);
        __m128 normalized = _mm_div_ps(vec, lengthBcast);
        
        vectors[idx] = _mm_cvtss_f32(normalized);
        vectors[idx + 1] = _mm_cvtss_f32(_mm_shuffle_ps(normalized, normalized, 1));
        vectors[idx + 2] = _mm_cvtss_f32(_mm_shuffle_ps(normalized, normalized, 2));
    }
}

void VoxelMathSIMD::calculateDotProductsSSE(const float* vectors1, const float* vectors2,
                                           float* results, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        size_t idx = i * 3;
        __m128 v1 = _mm_set_ps(0.0f, vectors1[idx + 2], vectors1[idx + 1], vectors1[idx]);
        __m128 v2 = _mm_set_ps(0.0f, vectors2[idx + 2], vectors2[idx + 1], vectors2[idx]);
        __m128 product = _mm_mul_ps(v1, v2);
        
        // Sum the components
        __m128 sum = _mm_hadd_ps(product, product);
        sum = _mm_hadd_ps(sum, sum);
        
        _mm_store_ss(&results[i], sum);
    }
}
#endif // __SSE4_1__

// Note: AVX2 and ARM NEON implementations would follow similar patterns
// but are omitted here for brevity. In a production implementation,
// you would implement these for maximum performance.

bool VoxelMathSIMD::isAligned(const void* ptr, size_t alignment) {
    return reinterpret_cast<uintptr_t>(ptr) % alignment == 0;
}

size_t VoxelMathSIMD::alignedSize(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

} // namespace Math
} // namespace VoxelEditor