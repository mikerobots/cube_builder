#include <gtest/gtest.h>
#include "../include/voxel_math/VoxelMathSIMD.h"
#include "../../math/CoordinateConverter.h"
#include <vector>
#include <cmath>
#include <random>

using namespace VoxelEditor;
using namespace VoxelEditor::Math;

class VoxelMathSIMDTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize random number generator for test data
        rng.seed(12345);
    }
    
    // Helper to generate random test data
    std::vector<WorldCoordinates> generateRandomWorldCoordinates(size_t count) {
        std::vector<WorldCoordinates> coords;
        std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
        
        for (size_t i = 0; i < count; ++i) {
            coords.emplace_back(Vector3f(dist(rng), dist(rng), dist(rng)));
        }
        return coords;
    }
    
    std::vector<IncrementCoordinates> generateRandomIncrementCoordinates(size_t count) {
        std::vector<IncrementCoordinates> coords;
        std::uniform_int_distribution<int> dist(-1000, 1000);
        
        for (size_t i = 0; i < count; ++i) {
            coords.emplace_back(dist(rng), dist(rng), dist(rng));
        }
        return coords;
    }
    
    std::vector<Vector3f> generateRandomVectors(size_t count) {
        std::vector<Vector3f> vectors;
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (size_t i = 0; i < count; ++i) {
            vectors.emplace_back(dist(rng), dist(rng), dist(rng));
        }
        return vectors;
    }
    
    // Helper to compare floating point results with tolerance
    void expectNearArray(const float* expected, const float* actual, size_t count, float tolerance = 1e-5f) {
        for (size_t i = 0; i < count; ++i) {
            EXPECT_NEAR(expected[i], actual[i], tolerance) << "Mismatch at index " << i;
        }
    }
    
    std::mt19937 rng;
};

// Test SIMD availability detection
TEST_F(VoxelMathSIMDTest, SIMDAvailability) {
    bool available = VoxelMathSIMD::isSIMDAvailable();
    const char* instructionSet = VoxelMathSIMD::getSIMDInstructionSet();
    size_t batchSize = VoxelMathSIMD::getOptimalBatchSize();
    
    // These should return reasonable values
    EXPECT_TRUE(instructionSet != nullptr);
    EXPECT_GT(batchSize, 0);
    EXPECT_LE(batchSize, 64);  // Reasonable upper bound
    
    // Print for information
    std::cout << "SIMD Available: " << (available ? "Yes" : "No") << std::endl;
    std::cout << "Instruction Set: " << instructionSet << std::endl;
    std::cout << "Optimal Batch Size: " << batchSize << std::endl;
}

// Test batch world to increment conversion
TEST_F(VoxelMathSIMDTest, WorldToIncrementBatch) {
    const size_t count = 100;
    auto worldCoords = generateRandomWorldCoordinates(count);
    
    // SIMD batch conversion
    std::vector<IncrementCoordinates> simdResults(count);
    VoxelMathSIMD::worldToIncrementBatch(worldCoords.data(), simdResults.data(), count);
    
    // Scalar reference conversion
    std::vector<IncrementCoordinates> scalarResults(count);
    for (size_t i = 0; i < count; ++i) {
        scalarResults[i] = CoordinateConverter::worldToIncrement(worldCoords[i]);
    }
    
    // Compare results
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(simdResults[i].x(), scalarResults[i].x()) << "X mismatch at index " << i;
        EXPECT_EQ(simdResults[i].y(), scalarResults[i].y()) << "Y mismatch at index " << i;
        EXPECT_EQ(simdResults[i].z(), scalarResults[i].z()) << "Z mismatch at index " << i;
    }
}

// Test batch increment to world conversion
TEST_F(VoxelMathSIMDTest, IncrementToWorldBatch) {
    const size_t count = 100;
    auto incrementCoords = generateRandomIncrementCoordinates(count);
    
    // SIMD batch conversion
    std::vector<WorldCoordinates> simdResults(count);
    VoxelMathSIMD::incrementToWorldBatch(incrementCoords.data(), simdResults.data(), count);
    
    // Scalar reference conversion
    std::vector<WorldCoordinates> scalarResults(count);
    for (size_t i = 0; i < count; ++i) {
        scalarResults[i] = CoordinateConverter::incrementToWorld(incrementCoords[i]);
    }
    
    // Compare results
    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(simdResults[i].value().x, scalarResults[i].value().x) << "X mismatch at index " << i;
        EXPECT_FLOAT_EQ(simdResults[i].value().y, scalarResults[i].value().y) << "Y mismatch at index " << i;
        EXPECT_FLOAT_EQ(simdResults[i].value().z, scalarResults[i].value().z) << "Z mismatch at index " << i;
    }
}

// Test batch bounds calculation
TEST_F(VoxelMathSIMDTest, CalculateBoundsBatch) {
    const size_t count = 50;
    auto positions = generateRandomIncrementCoordinates(count);
    float voxelSize = 0.32f;  // 32cm voxels
    
    // SIMD batch calculation
    std::vector<VoxelBounds> simdResults(count);
    VoxelMathSIMD::calculateBoundsBatch(positions.data(), voxelSize, simdResults.data(), count);
    
    // Scalar reference calculation
    std::vector<VoxelBounds> scalarResults;
    for (size_t i = 0; i < count; ++i) {
        scalarResults.emplace_back(positions[i], voxelSize);
    }
    
    // Compare results
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(simdResults[i], scalarResults[i]) << "Bounds mismatch at index " << i;
    }
}

// Test batch distance calculation
TEST_F(VoxelMathSIMDTest, CalculateDistancesBatch) {
    const size_t count = 100;
    auto positions1 = generateRandomWorldCoordinates(count);
    auto positions2 = generateRandomWorldCoordinates(count);
    
    // SIMD batch calculation
    std::vector<float> simdResults(count);
    VoxelMathSIMD::calculateDistancesBatch(positions1.data(), positions2.data(), 
                                          simdResults.data(), count);
    
    // Scalar reference calculation
    std::vector<float> scalarResults(count);
    for (size_t i = 0; i < count; ++i) {
        Vector3f diff = positions1[i].value() - positions2[i].value();
        scalarResults[i] = diff.length();
    }
    
    // Compare results with tolerance for floating point
    expectNearArray(scalarResults.data(), simdResults.data(), count, 1e-4f);
}

// Test batch vector normalization
TEST_F(VoxelMathSIMDTest, NormalizeVectorsBatch) {
    const size_t count = 100;
    auto vectors = generateRandomVectors(count);
    
    // Make a copy for SIMD processing
    std::vector<Vector3f> simdVectors = vectors;
    VoxelMathSIMD::normalizeVectorsBatch(simdVectors.data(), count);
    
    // Scalar reference normalization
    std::vector<Vector3f> scalarVectors = vectors;
    for (size_t i = 0; i < count; ++i) {
        scalarVectors[i].normalize();
    }
    
    // Compare results
    for (size_t i = 0; i < count; ++i) {
        EXPECT_NEAR(simdVectors[i].x, scalarVectors[i].x, 1e-4f) << "X mismatch at index " << i;
        EXPECT_NEAR(simdVectors[i].y, scalarVectors[i].y, 1e-4f) << "Y mismatch at index " << i;
        EXPECT_NEAR(simdVectors[i].z, scalarVectors[i].z, 1e-4f) << "Z mismatch at index " << i;
        
        // Check that vectors are actually normalized
        float length = simdVectors[i].length();
        EXPECT_NEAR(length, 1.0f, 1e-4f) << "Vector not normalized at index " << i;
    }
}

// Test batch dot product calculation
TEST_F(VoxelMathSIMDTest, CalculateDotProductsBatch) {
    const size_t count = 100;
    auto vectors1 = generateRandomVectors(count);
    auto vectors2 = generateRandomVectors(count);
    
    // SIMD batch calculation
    std::vector<float> simdResults(count);
    VoxelMathSIMD::calculateDotProductsBatch(vectors1.data(), vectors2.data(), 
                                            simdResults.data(), count);
    
    // Scalar reference calculation
    std::vector<float> scalarResults(count);
    for (size_t i = 0; i < count; ++i) {
        scalarResults[i] = vectors1[i].dot(vectors2[i]);
    }
    
    // Compare results
    expectNearArray(scalarResults.data(), simdResults.data(), count, 1e-4f);
}

// Test edge cases
TEST_F(VoxelMathSIMDTest, EdgeCases) {
    // Empty batch
    VoxelMathSIMD::worldToIncrementBatch(nullptr, nullptr, 0);
    VoxelMathSIMD::incrementToWorldBatch(nullptr, nullptr, 0);
    VoxelMathSIMD::calculateDistancesBatch(nullptr, nullptr, nullptr, 0);
    VoxelMathSIMD::normalizeVectorsBatch(nullptr, 0);
    VoxelMathSIMD::calculateDotProductsBatch(nullptr, nullptr, nullptr, 0);
    
    // Single element batch
    WorldCoordinates singleWorld(Vector3f(1.23f, 4.56f, 7.89f));
    IncrementCoordinates singleIncrement;
    VoxelMathSIMD::worldToIncrementBatch(&singleWorld, &singleIncrement, 1);
    
    // Compare with scalar
    IncrementCoordinates expectedIncrement = CoordinateConverter::worldToIncrement(singleWorld);
    EXPECT_EQ(singleIncrement, expectedIncrement);
    
    // Small batch (less than SIMD width)
    const size_t smallCount = 3;
    auto smallWorldCoords = generateRandomWorldCoordinates(smallCount);
    std::vector<IncrementCoordinates> smallResults(smallCount);
    VoxelMathSIMD::worldToIncrementBatch(smallWorldCoords.data(), smallResults.data(), smallCount);
    
    // Verify small batch results
    for (size_t i = 0; i < smallCount; ++i) {
        IncrementCoordinates expected = CoordinateConverter::worldToIncrement(smallWorldCoords[i]);
        EXPECT_EQ(smallResults[i], expected);
    }
}

// Test performance characteristics (informational)
TEST_F(VoxelMathSIMDTest, PerformanceCharacteristics) {
    const size_t largeCount = 10000;
    auto worldCoords = generateRandomWorldCoordinates(largeCount);
    std::vector<IncrementCoordinates> results(largeCount);
    
    // This test doesn't verify correctness, just that large batches work
    VoxelMathSIMD::worldToIncrementBatch(worldCoords.data(), results.data(), largeCount);
    
    // Verify at least some results are reasonable
    for (size_t i = 0; i < std::min(largeCount, size_t(10)); ++i) {
        EXPECT_TRUE(std::abs(results[i].x()) < 10000);  // Reasonable range
        EXPECT_TRUE(std::abs(results[i].y()) < 10000);
        EXPECT_TRUE(std::abs(results[i].z()) < 10000);
    }
}

// Test vector operations with special values
TEST_F(VoxelMathSIMDTest, SpecialValueHandling) {
    // Test with zero vectors
    std::vector<Vector3f> zeroVectors(10, Vector3f(0.0f, 0.0f, 0.0f));
    VoxelMathSIMD::normalizeVectorsBatch(zeroVectors.data(), 10);
    
    // Zero vectors should remain zero or become NaN (implementation dependent)
    for (const auto& vec : zeroVectors) {
        bool isZeroOrNaN = (vec.length() == 0.0f) || 
                          (std::isnan(vec.x) || std::isnan(vec.y) || std::isnan(vec.z));
        EXPECT_TRUE(isZeroOrNaN);
    }
    
    // Test with unit vectors
    std::vector<Vector3f> unitVectors = {
        Vector3f(1.0f, 0.0f, 0.0f),
        Vector3f(0.0f, 1.0f, 0.0f),
        Vector3f(0.0f, 0.0f, 1.0f)
    };
    
    VoxelMathSIMD::normalizeVectorsBatch(unitVectors.data(), 3);
    
    // Unit vectors should remain unit vectors
    for (const auto& vec : unitVectors) {
        EXPECT_NEAR(vec.length(), 1.0f, 1e-4f);
    }
}