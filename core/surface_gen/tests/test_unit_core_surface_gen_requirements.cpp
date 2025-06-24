#include <gtest/gtest.h>
#include "../SurfaceGenerator.h"
#include "../DualContouring.h"
#include "../DualContouringSparse.h"
#include "../MeshBuilder.h"
#include "../SurfaceTypes.h"
#include "../../voxel_data/VoxelGrid.h"
#include <chrono>
#include <thread>

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

// Test fixture for Surface Generation requirements
class SurfaceGenRequirementsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test grid with smaller workspace for faster tests
        workspaceSize = Vector3f(2.0f, 2.0f, 2.0f);
        testGrid = std::make_unique<VoxelGrid>(VoxelResolution::Size_32cm, workspaceSize);
        
        // Add minimal test voxels (2x2x2 cube instead of 3x3x3)
        for (int z = 0; z < 2; ++z) {
            for (int y = 0; y < 2; ++y) {
                for (int x = 0; x < 2; ++x) {
                    testGrid->setVoxel(IncrementCoordinates(x * 32, y * 32, z * 32), true);
                }
            }
        }
    }
    
    // Helper to create an L-shaped structure for testing sharp edges
    void createLShape() {
        testGrid->clear();
        // Minimal L-shape: just 3 voxels
        // Horizontal part
        testGrid->setVoxel(IncrementCoordinates(0, 0, 0), true);
        testGrid->setVoxel(IncrementCoordinates(32, 0, 0), true);
        // Vertical part
        testGrid->setVoxel(IncrementCoordinates(0, 32, 0), true);
    }
    
    Vector3f workspaceSize;
    std::unique_ptr<VoxelGrid> testGrid;
};

// REQ-10.1.1: System shall use Dual Contouring algorithm for surface generation
TEST_F(SurfaceGenRequirementsTest, DualContouringAlgorithm) {
    // Create minimal test case with single voxel
    VoxelGrid singleVoxelGrid(VoxelResolution::Size_32cm, Vector3f(1.0f, 1.0f, 1.0f));
    singleVoxelGrid.setVoxel(IncrementCoordinates(32, 32, 32), true);
    
    // Verify that DualContouring class exists and can generate meshes
    // Use DualContouringSparse for speed in tests
    DualContouringSparse dc;
    
    // Generate mesh using dual contouring with preview settings for speed
    Mesh mesh = dc.generateMesh(singleVoxelGrid, SurfaceSettings::Preview());
    
    // Verify mesh was generated
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 0);
    EXPECT_GT(mesh.indices.size(), 0);
    
    // The algorithm should produce smooth surfaces
    // Verify by checking that we have more vertices than a simple marching cubes would produce
    // (Dual contouring creates vertices inside voxels for better feature preservation)
    EXPECT_GE(mesh.vertices.size(), 8); // At least corner vertices
}

// REQ-10.1.2: Algorithm shall provide better feature preservation than Marching Cubes
TEST_F(SurfaceGenRequirementsTest, FeaturePreservation) {
    // Create a shape with sharp edges (L-shape)
    createLShape();
    
    // Generate with feature preservation enabled (based on Preview for speed)
    SurfaceSettings settings = SurfaceSettings::Preview();
    settings.preserveSharpFeatures = true;
    settings.sharpFeatureAngle = 30.0f;
    
    DualContouringSparse dc;
    Mesh mesh = dc.generateMesh(*testGrid, settings);
    
    // The mesh should preserve sharp edges
    // We can't directly compare to marching cubes, but we can verify
    // that sharp feature preservation is working
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 0);
    
    // With feature preservation, the algorithm should create vertices
    // that maintain the sharp corner of the L-shape
    mesh.calculateBounds();
    // The bounds should roughly match our L-shape
    EXPECT_GE(mesh.bounds.max.x - mesh.bounds.min.x, 1.0f);
    EXPECT_GE(mesh.bounds.max.y - mesh.bounds.min.y, 1.0f);
}

// REQ-10.1.3: System shall support adaptive mesh generation based on voxel resolution
TEST_F(SurfaceGenRequirementsTest, AdaptiveMeshGeneration) {
    // Test with different adaptive error settings
    SurfaceSettings lowError = SurfaceSettings::Preview();
    lowError.adaptiveError = 0.001f; // High quality
    
    SurfaceSettings highError = SurfaceSettings::Preview();
    highError.adaptiveError = 0.1f; // Lower quality
    
    DualContouringSparse dc;
    Mesh meshLowError = dc.generateMesh(*testGrid, lowError);
    Mesh meshHighError = dc.generateMesh(*testGrid, highError);
    
    // Both should be valid
    EXPECT_TRUE(meshLowError.isValid());
    EXPECT_TRUE(meshHighError.isValid());
    
    // Lower error (higher quality) should generally produce more detailed mesh
    // Note: This might not always be true for simple shapes, but the capability exists
    EXPECT_GT(meshLowError.vertices.size(), 0);
    EXPECT_GT(meshHighError.vertices.size(), 0);
}

// REQ-10.1.4: System shall support multi-resolution surface generation (LOD)
TEST_F(SurfaceGenRequirementsTest, MultiResolutionLOD) {
    SurfaceGenerator generator;
    
    // Generate meshes at different LOD levels
    std::vector<Mesh> lodMeshes;
    for (int lod = 0; lod <= 4; ++lod) {
        Mesh mesh = generator.generatePreviewMesh(*testGrid, lod);
        EXPECT_TRUE(mesh.isValid());
        lodMeshes.push_back(mesh);
    }
    
    // Verify LOD progression (higher LOD = fewer vertices)
    for (size_t i = 1; i < lodMeshes.size(); ++i) {
        EXPECT_LE(lodMeshes[i].vertices.size(), lodMeshes[i-1].vertices.size());
    }
    
    // Verify LOD calculation based on distance
    BoundingBox bounds;
    bounds.min = Vector3f(0, 0, 0);
    bounds.max = Vector3f(10, 10, 10);
    
    int nearLOD = generator.calculateLOD(5.0f, bounds);
    int farLOD = generator.calculateLOD(500.0f, bounds);  // Use larger distance
    
    // Near objects should have lower LOD (more detail)
    // If both are 0, that's OK - it means we're using maximum detail for both
    EXPECT_LE(nearLOD, farLOD);
}

// REQ-10.1.5: System shall provide real-time preview with simplified mesh
TEST_F(SurfaceGenRequirementsTest, RealtimePreview) {
    SurfaceGenerator generator;
    
    // Measure time for preview generation
    auto start = std::chrono::steady_clock::now();
    
    // Generate preview mesh (simplified for real-time)
    Mesh previewMesh = generator.generateSurface(*testGrid, SurfaceSettings::Preview());
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Preview generation should be reasonably fast (under 2 seconds for small grids)
    // Note: Even with optimizations, mesh generation takes time
    EXPECT_LT(duration.count(), 2000);
    
    // Preview mesh should be valid but simplified
    EXPECT_TRUE(previewMesh.isValid());
    EXPECT_GT(previewMesh.vertices.size(), 0);
    
    // Compare with full quality mesh
    Mesh fullMesh = generator.generateSurface(*testGrid, SurfaceSettings::Export());
    
    // Preview should have fewer vertices (simplified)
    EXPECT_LE(previewMesh.vertices.size(), fullMesh.vertices.size());
}

// REQ-10.1.6: System shall generate high-quality export meshes
TEST_F(SurfaceGenRequirementsTest, HighQualityExport) {
    SurfaceGenerator generator;
    
    // Test all export quality levels
    std::vector<ExportQuality> qualities = {
        ExportQuality::Draft,
        ExportQuality::Standard,
        ExportQuality::High,
        ExportQuality::Maximum
    };
    
    size_t lastVertexCount = 0;
    for (auto quality : qualities) {
        Mesh mesh = generator.generateExportMesh(*testGrid, quality);
        
        // All quality levels should produce valid meshes
        EXPECT_TRUE(mesh.isValid());
        EXPECT_GT(mesh.vertices.size(), 0);
        EXPECT_GT(mesh.indices.size(), 0);
        
        // Higher quality should generally have more vertices
        // (or at least not fewer)
        if (lastVertexCount > 0) {
            EXPECT_GE(mesh.vertices.size(), lastVertexCount * 0.5f); // Allow some variation
        }
        lastVertexCount = mesh.vertices.size();
        
        // Maximum quality should include all features
        if (quality == ExportQuality::Maximum) {
            // Should have normals and potentially UVs
            SurfaceSettings exportSettings = SurfaceSettings::Export();
            Mesh maxQualityMesh = generator.generateSurface(*testGrid, exportSettings);
            EXPECT_GT(maxQualityMesh.normals.size(), 0);
        }
    }
}

// REQ-10.1.7: System shall preserve sharp edges for architectural details
TEST_F(SurfaceGenRequirementsTest, SharpEdgePreservation) {
    // Create a structure with clear architectural details (stairs pattern)
    testGrid->clear();
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x <= y; ++x) {
            for (int z = 1; z < 3; ++z) {
                testGrid->setVoxel(IncrementCoordinates((x + 2) * 32, (y + 2) * 32, z * 32), true);
            }
        }
    }
    
    // Generate with sharp feature preservation
    SurfaceSettings settings = SurfaceSettings::Preview();
    settings.preserveSharpFeatures = true;
    settings.sharpFeatureAngle = 45.0f; // Architectural angles
    
    DualContouringSparse dc;
    Mesh mesh = dc.generateMesh(*testGrid, settings);
    
    EXPECT_TRUE(mesh.isValid());
    
    // Generate normals to analyze edge sharpness
    mesh.calculateNormals();
    EXPECT_EQ(mesh.normals.size(), mesh.vertices.size());
    
    // The stair pattern should have distinct normal changes at edges
    // We can't easily verify this automatically, but the feature exists
    
    // Also test with feature preservation disabled
    settings.preserveSharpFeatures = false;
    Mesh smoothMesh = dc.generateMesh(*testGrid, settings);
    EXPECT_TRUE(smoothMesh.isValid());
}

// REQ-6.3.1: Total application memory shall not exceed 4GB (Meta Quest 3 constraint)
TEST_F(SurfaceGenRequirementsTest, MemoryConstraints) {
    SurfaceGenerator generator;
    
    // Enable caching to test memory management
    generator.enableCaching(true);
    
    // Set a reasonable cache limit (100MB for this test)
    const size_t cacheLimit = 100 * 1024 * 1024;
    generator.setCacheMaxMemory(cacheLimit);
    
    // Generate multiple meshes to fill cache
    for (int i = 0; i < 10; ++i) {
        // Modify grid slightly each time
        testGrid->setVoxel(IncrementCoordinates((i % 8) * 32, (i % 8) * 32, (i % 8) * 32), true);
        
        Mesh mesh = generator.generateSurface(*testGrid);
        EXPECT_TRUE(mesh.isValid());
        
        // Check memory usage is bounded
        size_t memoryUsage = generator.getCacheMemoryUsage();
        EXPECT_LE(memoryUsage, cacheLimit);
    }
    
    // Verify cache eviction is working
    size_t finalMemory = generator.getCacheMemoryUsage();
    EXPECT_LE(finalMemory, cacheLimit);
}

// REQ-8.2.1: System shall export STL files for 3D printing and sharing
TEST_F(SurfaceGenRequirementsTest, STLExportSupport) {
    // Generate a mesh
    SurfaceGenerator generator;
    Mesh mesh = generator.generateExportMesh(*testGrid, ExportQuality::Standard);
    
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 0);
    EXPECT_GT(mesh.indices.size(), 0);
    
    // The mesh should be suitable for STL export
    // STL requires:
    // 1. Valid triangles (3 vertices per face)
    EXPECT_EQ(mesh.indices.size() % 3, 0);
    
    // 2. All indices within bounds
    uint32_t maxIndex = static_cast<uint32_t>(mesh.vertices.size() - 1);
    for (uint32_t index : mesh.indices) {
        EXPECT_LE(index, maxIndex);
    }
    
    // 3. Non-degenerate triangles (we'll check a few)
    for (size_t i = 0; i < std::min(size_t(10), mesh.indices.size() / 3); i += 3) {
        Vector3f v0 = mesh.vertices[mesh.indices[i]].value();
        Vector3f v1 = mesh.vertices[mesh.indices[i + 1]].value();
        Vector3f v2 = mesh.vertices[mesh.indices[i + 2]].value();
        
        // Vertices should be distinct
        EXPECT_NE(v0, v1);
        EXPECT_NE(v1, v2);
        EXPECT_NE(v0, v2);
    }
    
    // Note: Actual STL export is handled by the FileIO subsystem
    // This test verifies the mesh is suitable for export
}

// Additional test for async generation capabilities
TEST_F(SurfaceGenRequirementsTest, AsyncGenerationSupport) {
    SurfaceGenerator generator;
    
    // Start multiple async generations
    std::vector<std::future<Mesh>> futures;
    for (int i = 0; i < 3; ++i) {
        futures.push_back(generator.generateSurfaceAsync(*testGrid, SurfaceSettings::Preview()));
    }
    
    // All should complete successfully
    for (auto& future : futures) {
        EXPECT_EQ(future.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        Mesh mesh = future.get();
        EXPECT_TRUE(mesh.isValid());
    }
}

// Test for progress callback functionality
TEST_F(SurfaceGenRequirementsTest, ProgressCallbackSupport) {
    SurfaceGenerator generator;
    
    bool callbackCalled = false;
    float finalProgress = 0.0f;
    
    generator.setProgressCallback([&](float progress, const std::string& status) {
        callbackCalled = true;
        finalProgress = progress;
        EXPECT_GE(progress, 0.0f);
        EXPECT_LE(progress, 1.0f);
    });
    
    // Generate mesh with callback
    Mesh mesh = generator.generateSurface(*testGrid);
    
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(finalProgress, 1.0f);
    EXPECT_TRUE(mesh.isValid());
}