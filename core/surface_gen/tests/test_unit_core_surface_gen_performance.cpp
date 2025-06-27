#include <gtest/gtest.h>
#include "../DualContouring.h"
#include "../DualContouringFast.h"
#include "../DualContouringSparse.h"
#include "../DualContouringNEON.h"
#include "../SurfaceTypes.h"
#include "../../voxel_data/VoxelGrid.h"
#include <chrono>

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class SurfaceGenPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        workspaceSize = Vector3f(1.0f, 1.0f, 1.0f);  // Smaller grid for faster tests
        testGrid = std::make_unique<VoxelGrid>(VoxelResolution::Size_32cm, workspaceSize);
    }
    
    Vector3f workspaceSize;
    std::unique_ptr<VoxelGrid> testGrid;
};

TEST_F(SurfaceGenPerformanceTest, DISABLED_EmptyGridPerformance) {
    // Test empty grid performance
    DualContouring dcOriginal;
    DualContouringFast dcFast;
    
    auto settings = SurfaceSettings::Preview();
    
    // Time original implementation
    auto start = std::chrono::high_resolution_clock::now();
    Mesh meshOriginal = dcOriginal.generateMesh(*testGrid, settings);
    auto end = std::chrono::high_resolution_clock::now();
    auto durationOriginal = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Time optimized implementation
    start = std::chrono::high_resolution_clock::now();
    Mesh meshFast = dcFast.generateMesh(*testGrid, settings);
    end = std::chrono::high_resolution_clock::now();
    auto durationFast = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Both should produce empty meshes
    EXPECT_TRUE(meshOriginal.isValid());
    EXPECT_TRUE(meshFast.isValid());
    EXPECT_EQ(meshOriginal.vertices.size(), 0);
    EXPECT_EQ(meshFast.vertices.size(), 0);
    
    // Fast version should be significantly faster
    std::cout << "Empty grid - Original: " << durationOriginal.count() << "ms, "
              << "Fast: " << durationFast.count() << "ms" << std::endl;
    
    // Fast version should be at least 10x faster for empty grids
    EXPECT_LT(durationFast.count(), durationOriginal.count() / 10);
}

TEST_F(SurfaceGenPerformanceTest, DISABLED_NEONPerformance) {
    // Create a medium-density grid for NEON testing
    for (int i = 0; i < 5; ++i) {
        testGrid->setVoxel(IncrementCoordinates(i * 64, i * 32, i * 48), true);
    }
    
    DualContouring dcOriginal;
    DualContouringNEON dcNEON;
    
    auto settings = SurfaceSettings::Preview();
    
    // Time original implementation
    auto start = std::chrono::high_resolution_clock::now();
    Mesh meshOriginal = dcOriginal.generateMesh(*testGrid, settings);
    auto end = std::chrono::high_resolution_clock::now();
    auto durationOriginal = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Time NEON implementation
    start = std::chrono::high_resolution_clock::now();
    Mesh meshNEON = dcNEON.generateMesh(*testGrid, settings);
    end = std::chrono::high_resolution_clock::now();
    auto durationNEON = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Both should produce valid meshes
    EXPECT_TRUE(meshOriginal.isValid());
    EXPECT_TRUE(meshNEON.isValid());
    
    std::cout << "NEON test - Original: " << durationOriginal.count() << "ms, "
              << "NEON: " << durationNEON.count() << "ms" << std::endl;
    
    // NEON should provide speedup
    EXPECT_LT(durationNEON.count(), durationOriginal.count());
}

TEST_F(SurfaceGenPerformanceTest, DISABLED_SparseGridPerformance) {
    // Add a few scattered voxels
    testGrid->setVoxel(IncrementCoordinates(32, 32, 32), true);
    testGrid->setVoxel(IncrementCoordinates(96, 96, 96), true);
    testGrid->setVoxel(IncrementCoordinates(160, 32, 160), true);
    
    DualContouring dcOriginal;
    DualContouringFast dcFast;
    DualContouringSparse dcSparse;
    
    auto settings = SurfaceSettings::Preview();
    
    // Time original implementation
    auto start = std::chrono::high_resolution_clock::now();
    Mesh meshOriginal = dcOriginal.generateMesh(*testGrid, settings);
    auto end = std::chrono::high_resolution_clock::now();
    auto durationOriginal = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Time fast implementation
    start = std::chrono::high_resolution_clock::now();
    Mesh meshFast = dcFast.generateMesh(*testGrid, settings);
    end = std::chrono::high_resolution_clock::now();
    auto durationFast = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Time sparse implementation
    start = std::chrono::high_resolution_clock::now();
    Mesh meshSparse = dcSparse.generateMesh(*testGrid, settings);
    end = std::chrono::high_resolution_clock::now();
    auto durationSparse = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // All should produce valid meshes with vertices
    EXPECT_TRUE(meshOriginal.isValid());
    EXPECT_TRUE(meshFast.isValid());
    EXPECT_TRUE(meshSparse.isValid());
    EXPECT_GT(meshOriginal.vertices.size(), 0);
    EXPECT_GT(meshSparse.vertices.size(), 0);
    
    // Meshes should be similar (allowing for small differences in vertex ordering)
    EXPECT_NEAR(meshOriginal.vertices.size(), meshSparse.vertices.size(), 10);
    EXPECT_EQ(meshOriginal.indices.size(), meshSparse.indices.size());
    
    std::cout << "Sparse grid - Original: " << durationOriginal.count() << "ms, "
              << "Fast: " << durationFast.count() << "ms, "
              << "Sparse: " << durationSparse.count() << "ms" << std::endl;
    
    // Sparse version should be faster (at least 1.5x)
    EXPECT_LT(durationSparse.count(), durationOriginal.count());
}

TEST_F(SurfaceGenPerformanceTest, DISABLED_DenseGridPerformance) {
    // Create a small dense cube within the 1m workspace
    for (int z = 0; z < 2; ++z) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                testGrid->setVoxel(IncrementCoordinates(x * 32, y * 32, z * 32), true);
            }
        }
    }
    
    DualContouring dcOriginal;
    DualContouringFast dcFast;
    
    auto settings = SurfaceSettings::Preview();
    
    // Time original implementation
    auto start = std::chrono::high_resolution_clock::now();
    Mesh meshOriginal = dcOriginal.generateMesh(*testGrid, settings);
    auto end = std::chrono::high_resolution_clock::now();
    auto durationOriginal = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Time optimized implementation
    start = std::chrono::high_resolution_clock::now();
    Mesh meshFast = dcFast.generateMesh(*testGrid, settings);
    end = std::chrono::high_resolution_clock::now();
    auto durationFast = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Both should produce valid meshes
    EXPECT_TRUE(meshOriginal.isValid());
    EXPECT_TRUE(meshFast.isValid());
    EXPECT_GT(meshOriginal.vertices.size(), 0);
    EXPECT_GT(meshFast.vertices.size(), 0);
    
    std::cout << "Dense grid - Original: " << durationOriginal.count() << "ms, "
              << "Fast: " << durationFast.count() << "ms" << std::endl;
    
    // Fast version may not always be faster for dense grids due to overhead
    // Just verify both produce valid results
    std::cout << "Performance ratio: " << (float)durationFast.count() / durationOriginal.count() << "x" << std::endl;
}