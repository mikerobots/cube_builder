#include <gtest/gtest.h>
#include "../SurfaceGenerator.h"
#include "../SurfaceTypes.h"
#include "../../voxel_data/VoxelGrid.h"
#include <thread>
#include <chrono>

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

// OPTIMIZATION SUGGESTIONS FOR test_unit_core_surface_gen_generator.cpp
//
// The current test takes ~48 seconds due to expensive mesh generation operations.
// Here are the recommended optimizations:

class SurfaceGeneratorOptimizedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // OPTIMIZATION 1: Use smaller test grids
        // Current: 8x8x8 grid with 4x4x4 voxel cube
        // Suggested: 4x4x4 grid with 2x2x2 voxel cube (8x less data)
        gridDimensions = Vector3i(4, 4, 4);
        workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);  // Smaller workspace
        testGrid = std::make_unique<VoxelGrid>(VoxelResolution::Size_16cm, workspaceSize);
        
        // Add minimal test voxels (just 8 voxels instead of 64)
        for (int z = 1; z < 3; ++z) {
            for (int y = 1; y < 3; ++y) {
                for (int x = 1; x < 3; ++x) {
                    testGrid->setVoxel(Vector3i(x, y, z), true);
                }
            }
        }
        
        // OPTIMIZATION 2: Create simplified settings for tests
        simplifiedSettings = SurfaceSettings::Preview();
        simplifiedSettings.generateUVs = false;  // Skip UV generation
        simplifiedSettings.generateNormals = false;  // Skip normal generation where not needed
        simplifiedSettings.smoothingIterations = 0;  // No smoothing
        simplifiedSettings.simplificationRatio = 1.0f;  // No simplification
    }
    
    Vector3i gridDimensions;
    Vector3f workspaceSize;
    std::unique_ptr<VoxelGrid> testGrid;
    SurfaceSettings simplifiedSettings;
};

// Example of how to optimize the PreviewMeshGeneration test
// Current: Takes 11.4 seconds
// Target: < 2 seconds
TEST_F(SurfaceGeneratorOptimizedTest, PreviewMeshGeneration_Optimized) {
    SurfaceGenerator generator;
    
    // OPTIMIZATION 3: Test fewer LOD levels
    // Instead of testing all 5 LOD levels, test only 2
    std::vector<int> testLods = {0, 4};  // Just min and max
    
    for (int lod : testLods) {
        Mesh mesh = generator.generatePreviewMesh(*testGrid, lod);
        EXPECT_TRUE(mesh.isValid());
    }
}

// Example of how to optimize the ExportMeshGeneration test  
// Current: Takes 8.5 seconds
// Target: < 2 seconds
TEST_F(SurfaceGeneratorOptimizedTest, ExportMeshGeneration_Optimized) {
    SurfaceGenerator generator;
    
    // OPTIMIZATION 4: Test only two export qualities instead of all four
    std::vector<ExportQuality> qualities = {
        ExportQuality::Draft,
        ExportQuality::Maximum
    };
    
    for (auto quality : qualities) {
        Mesh mesh = generator.generateExportMesh(*testGrid, quality);
        EXPECT_TRUE(mesh.isValid());
    }
}

// OPTIMIZATION 5: Mock expensive operations for unit tests
class MockSurfaceGenerator : public SurfaceGenerator {
public:
    MockSurfaceGenerator() : SurfaceGenerator(nullptr) {
        // Override to return simple meshes without expensive computation
    }
    
    Mesh generateSurface(const VoxelData::VoxelGrid& grid, 
                        const SurfaceSettings& settings) override {
        // Return a simple valid mesh without doing dual contouring
        Mesh mesh;
        mesh.vertices = {
            Math::Vector3f(0, 0, 0),
            Math::Vector3f(1, 0, 0),
            Math::Vector3f(0, 1, 0)
        };
        mesh.indices = {0, 1, 2};
        return mesh;
    }
};

// OPTIMIZATION 6: Separate integration tests from unit tests
// Move these expensive tests to integration test suite:
// - AsyncGeneration (tests threading, not core logic)
// - MultipleAsyncGenerations (tests threading, not core logic)
// - CacheMemoryLimit (tests memory management over time)
// - EmptyGrid/SingleVoxel (should be instant but takes 3-4 seconds)

// OPTIMIZATION 7: Add test categorization
// Use Google Test filtering to run fast tests during development:
// ./test_unit_core_surface_gen_generator --gtest_filter=*Fast*

TEST_F(SurfaceGeneratorOptimizedTest, BasicGeneration_Fast) {
    SurfaceGenerator generator;
    
    // Use simplified settings for faster generation
    Mesh mesh = generator.generateSurface(*testGrid, simplifiedSettings);
    
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 0);
    EXPECT_GT(mesh.indices.size(), 0);
    EXPECT_EQ(mesh.indices.size() % 3, 0);
}

// SUMMARY OF OPTIMIZATIONS:
// 1. Reduce test data size (8x reduction)
// 2. Skip unnecessary post-processing in tests
// 3. Test fewer parameter combinations
// 4. Mock expensive operations where appropriate
// 5. Move integration tests to separate suite
// 6. Add test categorization for selective running
// 7. Consider parallel test execution for independent tests
//
// Expected improvement: 48s -> ~10s for essential unit tests