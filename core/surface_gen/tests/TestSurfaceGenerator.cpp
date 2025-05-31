#include <gtest/gtest.h>
#include "../SurfaceGenerator.h"
#include "../SurfaceTypes.h"
#include "../../voxel_data/VoxelGrid.h"
#include <thread>
#include <chrono>

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class SurfaceGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple test grid
        gridDimensions = Vector3i(8, 8, 8);
        workspaceSize = Vector3f(10.0f, 10.0f, 10.0f);
        testGrid = std::make_unique<VoxelGrid>(VoxelResolution::Size_32cm, workspaceSize);
        
        // Add some test voxels (create a small cube)
        for (int z = 2; z < 6; ++z) {
            for (int y = 2; y < 6; ++y) {
                for (int x = 2; x < 6; ++x) {
                    testGrid->setVoxel(Vector3i(x, y, z), true);
                }
            }
        }
    }
    
    Vector3i gridDimensions;
    Vector3f workspaceSize;
    std::unique_ptr<VoxelGrid> testGrid;
};

TEST_F(SurfaceGeneratorTest, BasicGeneration) {
    SurfaceGenerator generator;
    
    // Generate surface
    Mesh mesh = generator.generateSurface(*testGrid);
    
    // Verify mesh is valid
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 0);
    EXPECT_GT(mesh.indices.size(), 0);
    EXPECT_EQ(mesh.indices.size() % 3, 0); // Should be triangles
}

TEST_F(SurfaceGeneratorTest, PreviewMeshGeneration) {
    SurfaceGenerator generator;
    
    // Generate preview mesh at different LOD levels
    for (int lod = 0; lod <= 4; ++lod) {
        Mesh mesh = generator.generatePreviewMesh(*testGrid, lod);
        EXPECT_TRUE(mesh.isValid());
        
        // Higher LOD should generally have fewer vertices
        if (lod > 0) {
            Mesh mesh0 = generator.generatePreviewMesh(*testGrid, 0);
            EXPECT_LE(mesh.vertices.size(), mesh0.vertices.size());
        }
    }
}

TEST_F(SurfaceGeneratorTest, ExportMeshGeneration) {
    SurfaceGenerator generator;
    
    // Test different export qualities
    std::vector<ExportQuality> qualities = {
        ExportQuality::Draft,
        ExportQuality::Standard,
        ExportQuality::High,
        ExportQuality::Maximum
    };
    
    for (auto quality : qualities) {
        Mesh mesh = generator.generateExportMesh(*testGrid, quality);
        EXPECT_TRUE(mesh.isValid());
    }
}

TEST_F(SurfaceGeneratorTest, CustomSettings) {
    SurfaceGenerator generator;
    
    // Test with custom settings
    SurfaceSettings settings;
    settings.adaptiveError = 0.001f;
    settings.generateUVs = true;
    settings.generateNormals = true;
    settings.smoothingIterations = 2;
    settings.simplificationRatio = 0.8f;
    
    generator.setSurfaceSettings(settings);
    Mesh mesh = generator.generateSurface(*testGrid);
    
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.normals.size(), 0);
    EXPECT_GT(mesh.uvCoords.size(), 0);
}

TEST_F(SurfaceGeneratorTest, EmptyGrid) {
    SurfaceGenerator generator;
    
    // Create empty grid
    VoxelGrid emptyGrid(VoxelResolution::Size_32cm, workspaceSize);
    
    // Generate surface from empty grid
    Mesh mesh = generator.generateSurface(emptyGrid);
    
    // Should produce empty but valid mesh
    EXPECT_TRUE(mesh.isValid());
    EXPECT_EQ(mesh.vertices.size(), 0);
    EXPECT_EQ(mesh.indices.size(), 0);
}

TEST_F(SurfaceGeneratorTest, SingleVoxel) {
    SurfaceGenerator generator;
    
    // Create grid with single voxel
    VoxelGrid singleVoxelGrid(VoxelResolution::Size_32cm, workspaceSize);
    singleVoxelGrid.setVoxel(Vector3i(4, 4, 4), true);
    
    // Generate surface
    Mesh mesh = generator.generateSurface(singleVoxelGrid);
    
    // Should produce a cube mesh
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 0);
    EXPECT_GT(mesh.indices.size(), 0);
}

TEST_F(SurfaceGeneratorTest, CacheEnabled) {
    SurfaceGenerator generator;
    generator.enableCaching(true);
    
    // Generate mesh first time
    auto start1 = std::chrono::steady_clock::now();
    Mesh mesh1 = generator.generateSurface(*testGrid);
    auto end1 = std::chrono::steady_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    
    // Generate same mesh again (should be cached)
    auto start2 = std::chrono::steady_clock::now();
    Mesh mesh2 = generator.generateSurface(*testGrid);
    auto end2 = std::chrono::steady_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
    // Cached version should be faster (though this might be flaky)
    EXPECT_TRUE(mesh1.isValid());
    EXPECT_TRUE(mesh2.isValid());
    EXPECT_EQ(mesh1.vertices.size(), mesh2.vertices.size());
    EXPECT_EQ(mesh1.indices.size(), mesh2.indices.size());
    
    // Check cache is being used
    EXPECT_GT(generator.getCacheMemoryUsage(), 0);
}

TEST_F(SurfaceGeneratorTest, CacheDisabled) {
    SurfaceGenerator generator;
    generator.enableCaching(false);
    
    // Generate mesh twice
    Mesh mesh1 = generator.generateSurface(*testGrid);
    Mesh mesh2 = generator.generateSurface(*testGrid);
    
    // Both should be valid
    EXPECT_TRUE(mesh1.isValid());
    EXPECT_TRUE(mesh2.isValid());
    
    // Cache should be empty
    EXPECT_EQ(generator.getCacheMemoryUsage(), 0);
}

TEST_F(SurfaceGeneratorTest, AsyncGeneration) {
    SurfaceGenerator generator;
    
    // Start async generation
    auto future = generator.generateSurfaceAsync(*testGrid, SurfaceSettings::Default());
    
    // Wait for completion
    EXPECT_EQ(future.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    
    // Get result
    Mesh mesh = future.get();
    EXPECT_TRUE(mesh.isValid());
}

TEST_F(SurfaceGeneratorTest, MultipleAsyncGenerations) {
    SurfaceGenerator generator;
    
    // Start multiple async generations
    std::vector<std::future<Mesh>> futures;
    for (int i = 0; i < 3; ++i) {
        futures.push_back(generator.generateSurfaceAsync(*testGrid, SurfaceSettings::Default()));
    }
    
    // Wait for all to complete
    for (auto& future : futures) {
        EXPECT_EQ(future.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        Mesh mesh = future.get();
        EXPECT_TRUE(mesh.isValid());
    }
}

TEST_F(SurfaceGeneratorTest, ProgressCallback) {
    SurfaceGenerator generator;
    
    // Track progress
    float lastProgress = -1.0f;
    std::string lastStatus;
    int callbackCount = 0;
    
    generator.setProgressCallback([&](float progress, const std::string& status) {
        EXPECT_GE(progress, 0.0f);
        EXPECT_LE(progress, 1.0f);
        EXPECT_GE(progress, lastProgress); // Progress should increase
        lastProgress = progress;
        lastStatus = status;
        callbackCount++;
    });
    
    // Generate mesh
    Mesh mesh = generator.generateSurface(*testGrid);
    
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(callbackCount, 0);
    EXPECT_EQ(lastProgress, 1.0f);
}

TEST_F(SurfaceGeneratorTest, VoxelDataChanged) {
    SurfaceGenerator generator;
    generator.enableCaching(true);
    
    // Generate and cache mesh
    Mesh mesh1 = generator.generateSurface(*testGrid);
    size_t cacheSize1 = generator.getCacheMemoryUsage();
    EXPECT_GT(cacheSize1, 0);
    
    // Notify of voxel data change
    BoundingBox changedRegion;
    changedRegion.min = Vector3f(2, 2, 2);
    changedRegion.max = Vector3f(6, 6, 6);
    generator.onVoxelDataChanged(changedRegion, VoxelResolution::Size_32cm);
    
    // Cache should be invalidated
    size_t cacheSize2 = generator.getCacheMemoryUsage();
    EXPECT_LT(cacheSize2, cacheSize1);
}

TEST_F(SurfaceGeneratorTest, LODSettings) {
    SurfaceGenerator generator;
    
    // Test LOD enabled/disabled
    generator.setLODEnabled(true);
    EXPECT_TRUE(generator.isLODEnabled());
    
    generator.setLODEnabled(false);
    EXPECT_FALSE(generator.isLODEnabled());
    
    // Test LOD calculation
    BoundingBox bounds;
    bounds.min = Vector3f(0, 0, 0);
    bounds.max = Vector3f(10, 10, 10);
    
    int lod = generator.calculateLOD(50.0f, bounds);
    EXPECT_GE(lod, 0);
    EXPECT_LE(lod, 4);
}

TEST_F(SurfaceGeneratorTest, CacheMemoryLimit) {
    SurfaceGenerator generator;
    generator.enableCaching(true);
    
    // Set very small cache limit
    generator.setCacheMaxMemory(1024); // 1KB
    
    // Generate multiple different meshes
    for (int i = 0; i < 5; ++i) {
        // Modify grid slightly
        testGrid->setVoxel(Vector3i(i, i, i), true);
        Mesh mesh = generator.generateSurface(*testGrid);
        EXPECT_TRUE(mesh.isValid());
    }
    
    // Cache should not exceed limit
    EXPECT_LE(generator.getCacheMemoryUsage(), 1024);
}

TEST_F(SurfaceGeneratorTest, ClearCache) {
    SurfaceGenerator generator;
    generator.enableCaching(true);
    
    // Generate and cache mesh
    Mesh mesh = generator.generateSurface(*testGrid);
    EXPECT_GT(generator.getCacheMemoryUsage(), 0);
    
    // Clear cache
    generator.clearCache();
    EXPECT_EQ(generator.getCacheMemoryUsage(), 0);
}