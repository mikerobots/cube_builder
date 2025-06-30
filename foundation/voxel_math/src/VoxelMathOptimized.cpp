#include "../include/voxel_math/VoxelMathSIMD.h"
#include "../include/voxel_math/VoxelCollision.h"
#include "../../math/CoordinateConverter.h"
#include <Eigen/Dense>
#include <Eigen/Core>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace VoxelEditor {
namespace Math {

// Eigen-optimized implementations that automatically use the best SIMD instructions
// available on the target platform (SSE, AVX, NEON, etc.)

void VoxelMathSIMD::worldToIncrementBatch(const WorldCoordinates* world,
                                         IncrementCoordinates* increment,
                                         size_t count) {
    if (count == 0) return;
    
    // Use Eigen for vectorized coordinate conversion
    // Process in chunks for optimal memory access
    constexpr size_t CHUNK_SIZE = 64; // Process 64 coordinates at a time
    
    for (size_t start = 0; start < count; start += CHUNK_SIZE) {
        size_t end = std::min(start + CHUNK_SIZE, count);
        size_t chunkSize = end - start;
        
        // Create Eigen matrices for vectorized operations
        Eigen::Matrix<float, 3, Eigen::Dynamic> worldMatrix(3, chunkSize);
        
        // Load world coordinates into Eigen matrix
        for (size_t i = 0; i < chunkSize; ++i) {
            const Vector3f& worldVec = world[start + i].value();
            worldMatrix.col(i) = Eigen::Vector3f(worldVec.x, worldVec.y, worldVec.z);
        }
        
        // Vectorized conversion: multiply by 100 to convert meters to centimeters
        Eigen::Matrix<float, 3, Eigen::Dynamic> cmMatrix = worldMatrix * METERS_TO_CM;
        
        // Round to nearest integer (vectorized)
        Eigen::Matrix<int, 3, Eigen::Dynamic> incrementMatrix = cmMatrix.array().round().cast<int>();
        
        // Store results back to increment coordinates
        for (size_t i = 0; i < chunkSize; ++i) {
            Eigen::Vector3i incVec = incrementMatrix.col(i);
            increment[start + i] = IncrementCoordinates(incVec.x(), incVec.y(), incVec.z());
        }
    }
}

void VoxelMathSIMD::incrementToWorldBatch(const IncrementCoordinates* increment,
                                         WorldCoordinates* world,
                                         size_t count) {
    if (count == 0) return;
    
    constexpr size_t CHUNK_SIZE = 64;
    
    for (size_t start = 0; start < count; start += CHUNK_SIZE) {
        size_t end = std::min(start + CHUNK_SIZE, count);
        size_t chunkSize = end - start;
        
        // Create Eigen matrices for vectorized operations
        Eigen::Matrix<int, 3, Eigen::Dynamic> incrementMatrix(3, chunkSize);
        
        // Load increment coordinates into Eigen matrix
        for (size_t i = 0; i < chunkSize; ++i) {
            const Vector3i& incVec = increment[start + i].value();
            incrementMatrix.col(i) = Eigen::Vector3i(incVec.x, incVec.y, incVec.z);
        }
        
        // Vectorized conversion: convert to float and multiply by 0.01 to convert cm to meters
        Eigen::Matrix<float, 3, Eigen::Dynamic> worldMatrix = incrementMatrix.cast<float>() * CM_TO_METERS;
        
        // Store results back to world coordinates
        for (size_t i = 0; i < chunkSize; ++i) {
            Eigen::Vector3f worldVec = worldMatrix.col(i);
            world[start + i] = WorldCoordinates(Vector3f(worldVec.x(), worldVec.y(), worldVec.z()));
        }
    }
}

void VoxelMathSIMD::calculateDistancesBatch(const WorldCoordinates* positions1,
                                           const WorldCoordinates* positions2,
                                           float* distances,
                                           size_t count) {
    if (count == 0) return;
    
    constexpr size_t CHUNK_SIZE = 64;
    
    for (size_t start = 0; start < count; start += CHUNK_SIZE) {
        size_t end = std::min(start + CHUNK_SIZE, count);
        size_t chunkSize = end - start;
        
        // Create Eigen matrices for vectorized operations
        Eigen::Matrix<float, 3, Eigen::Dynamic> pos1Matrix(3, chunkSize);
        Eigen::Matrix<float, 3, Eigen::Dynamic> pos2Matrix(3, chunkSize);
        
        // Load positions into Eigen matrices
        for (size_t i = 0; i < chunkSize; ++i) {
            const Vector3f& p1 = positions1[start + i].value();
            const Vector3f& p2 = positions2[start + i].value();
            pos1Matrix.col(i) = Eigen::Vector3f(p1.x, p1.y, p1.z);
            pos2Matrix.col(i) = Eigen::Vector3f(p2.x, p2.y, p2.z);
        }
        
        // Vectorized distance calculation
        Eigen::Matrix<float, 3, Eigen::Dynamic> diffMatrix = pos1Matrix - pos2Matrix;
        Eigen::Array<float, 1, Eigen::Dynamic> distanceArray = diffMatrix.colwise().norm();
        
        // Store results
        for (size_t i = 0; i < chunkSize; ++i) {
            distances[start + i] = distanceArray(i);
        }
    }
}

void VoxelMathSIMD::normalizeVectorsBatch(Vector3f* vectors, size_t count) {
    if (count == 0) return;
    
    constexpr size_t CHUNK_SIZE = 64;
    
    for (size_t start = 0; start < count; start += CHUNK_SIZE) {
        size_t end = std::min(start + CHUNK_SIZE, count);
        size_t chunkSize = end - start;
        
        // Create Eigen matrix for vectorized operations
        Eigen::Matrix<float, 3, Eigen::Dynamic> vectorMatrix(3, chunkSize);
        
        // Load vectors into Eigen matrix
        for (size_t i = 0; i < chunkSize; ++i) {
            const Vector3f& vec = vectors[start + i];
            vectorMatrix.col(i) = Eigen::Vector3f(vec.x, vec.y, vec.z);
        }
        
        // Vectorized normalization
        vectorMatrix.colwise().normalize();
        
        // Store results back
        for (size_t i = 0; i < chunkSize; ++i) {
            Eigen::Vector3f normalizedVec = vectorMatrix.col(i);
            vectors[start + i] = Vector3f(normalizedVec.x(), normalizedVec.y(), normalizedVec.z());
        }
    }
}

void VoxelMathSIMD::calculateDotProductsBatch(const Vector3f* vectors1,
                                             const Vector3f* vectors2,
                                             float* results,
                                             size_t count) {
    if (count == 0) return;
    
    constexpr size_t CHUNK_SIZE = 64;
    
    for (size_t start = 0; start < count; start += CHUNK_SIZE) {
        size_t end = std::min(start + CHUNK_SIZE, count);
        size_t chunkSize = end - start;
        
        // Create Eigen matrices for vectorized operations
        Eigen::Matrix<float, 3, Eigen::Dynamic> vec1Matrix(3, chunkSize);
        Eigen::Matrix<float, 3, Eigen::Dynamic> vec2Matrix(3, chunkSize);
        
        // Load vectors into Eigen matrices
        for (size_t i = 0; i < chunkSize; ++i) {
            const Vector3f& v1 = vectors1[start + i];
            const Vector3f& v2 = vectors2[start + i];
            vec1Matrix.col(i) = Eigen::Vector3f(v1.x, v1.y, v1.z);
            vec2Matrix.col(i) = Eigen::Vector3f(v2.x, v2.y, v2.z);
        }
        
        // Vectorized dot product calculation
        Eigen::Array<float, 1, Eigen::Dynamic> dotProducts = 
            (vec1Matrix.array() * vec2Matrix.array()).colwise().sum();
        
        // Store results
        for (size_t i = 0; i < chunkSize; ++i) {
            results[start + i] = dotProducts(i);
        }
    }
}

bool VoxelMathSIMD::isSIMDAvailable() {
    // With Eigen, SIMD is always "available" since it provides optimized
    // implementations regardless of the underlying hardware
    return true;
}

const char* VoxelMathSIMD::getSIMDInstructionSet() {
    // Eigen automatically detects and uses the best available instructions
#if defined(EIGEN_VECTORIZE_AVX512)
    return "Eigen (AVX512)";
#elif defined(EIGEN_VECTORIZE_AVX2)
    return "Eigen (AVX2)";
#elif defined(EIGEN_VECTORIZE_AVX)
    return "Eigen (AVX)";
#elif defined(EIGEN_VECTORIZE_SSE4_2)
    return "Eigen (SSE4.2)";
#elif defined(EIGEN_VECTORIZE_SSE4_1)
    return "Eigen (SSE4.1)";
#elif defined(EIGEN_VECTORIZE_SSSE3)
    return "Eigen (SSSE3)";
#elif defined(EIGEN_VECTORIZE_SSE3)
    return "Eigen (SSE3)";
#elif defined(EIGEN_VECTORIZE_SSE2)
    return "Eigen (SSE2)";
#elif defined(EIGEN_VECTORIZE_NEON)
    return "Eigen (ARM NEON)";
#elif defined(EIGEN_VECTORIZE_VSX)
    return "Eigen (PowerPC VSX)";
#elif defined(EIGEN_VECTORIZE_ZVECTOR)
    return "Eigen (IBM z13 SIMD)";
#else
    return "Eigen (Scalar)";
#endif
}

size_t VoxelMathSIMD::getOptimalBatchSize() {
    // With Eigen's optimizations, we can use larger batch sizes efficiently
    // The chunking in our implementation handles memory locality
    return 64;
}

// Keep existing implementation methods for compatibility
void VoxelMathSIMD::calculateBoundsBatch(const IncrementCoordinates* positions,
                                        float voxelSizeMeters,
                                        VoxelBounds* bounds,
                                        size_t count) {
    // This operation is complex and benefits less from SIMD
    // Use scalar implementation for now, but could be optimized with Eigen if needed
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

// Fallback scalar implementations - now optimized with Eigen where beneficial
void VoxelMathSIMD::worldToIncrementScalar(const WorldCoordinates* world,
                                          IncrementCoordinates* increment,
                                          size_t count) {
    // For small counts, use direct scalar conversion
    for (size_t i = 0; i < count; ++i) {
        increment[i] = CoordinateConverter::worldToIncrement(world[i]);
    }
}

void VoxelMathSIMD::incrementToWorldScalar(const IncrementCoordinates* increment,
                                          WorldCoordinates* world,
                                          size_t count) {
    // For small counts, use direct scalar conversion
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

// Helper functions remain the same
bool VoxelMathSIMD::isAligned(const void* ptr, size_t alignment) {
    return reinterpret_cast<uintptr_t>(ptr) % alignment == 0;
}

size_t VoxelMathSIMD::alignedSize(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

} // namespace Math
} // namespace VoxelEditor