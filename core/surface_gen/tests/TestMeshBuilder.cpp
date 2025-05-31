#include <gtest/gtest.h>
#include "../MeshBuilder.h"
#include "../SurfaceTypes.h"
#include "../SurfaceGenerator.h"
#include <cmath>
#include <thread>
#include <chrono>

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::Math;

TEST(MeshBuilderTest, CreateSimpleTriangle) {
    MeshBuilder builder;
    builder.beginMesh();
    
    // Add vertices
    uint32_t v0 = builder.addVertex(Vector3f(0, 0, 0));
    uint32_t v1 = builder.addVertex(Vector3f(1, 0, 0));
    uint32_t v2 = builder.addVertex(Vector3f(0, 1, 0));
    
    // Add triangle
    builder.addTriangle(v0, v1, v2);
    
    // Get mesh
    Mesh mesh = builder.endMesh();
    
    EXPECT_EQ(mesh.vertices.size(), 3);
    EXPECT_EQ(mesh.indices.size(), 3);
    EXPECT_TRUE(mesh.isValid());
}

TEST(MeshBuilderTest, CreateQuad) {
    MeshBuilder builder;
    builder.beginMesh();
    
    // Add vertices for a square
    uint32_t v0 = builder.addVertex(Vector3f(0, 0, 0));
    uint32_t v1 = builder.addVertex(Vector3f(1, 0, 0));
    uint32_t v2 = builder.addVertex(Vector3f(1, 1, 0));
    uint32_t v3 = builder.addVertex(Vector3f(0, 1, 0));
    
    // Add quad (will be converted to 2 triangles)
    builder.addQuad(v0, v1, v2, v3);
    
    Mesh mesh = builder.endMesh();
    
    EXPECT_EQ(mesh.vertices.size(), 4);
    EXPECT_EQ(mesh.indices.size(), 6); // 2 triangles
    EXPECT_TRUE(mesh.isValid());
}

TEST(MeshBuilderTest, RemoveDuplicateVertices) {
    MeshBuilder builder;
    builder.beginMesh();
    
    // Add duplicate vertices
    builder.addVertex(Vector3f(0, 0, 0));
    builder.addVertex(Vector3f(1, 0, 0));
    builder.addVertex(Vector3f(0, 0, 0)); // Duplicate
    builder.addVertex(Vector3f(1, 0, 0)); // Duplicate
    
    // Add triangles using duplicates
    builder.addTriangle(0, 1, 2);
    builder.addTriangle(1, 3, 2);
    
    // Remove duplicates
    builder.removeDuplicateVertices();
    
    Mesh mesh = builder.endMesh();
    
    EXPECT_EQ(mesh.vertices.size(), 2); // Only unique vertices
    EXPECT_EQ(mesh.indices.size(), 6);
    EXPECT_TRUE(mesh.isValid());
}

TEST(MeshBuilderTest, GenerateNormals) {
    MeshBuilder builder;
    builder.beginMesh();
    
    // Create a simple triangle
    builder.addVertex(Vector3f(0, 0, 0));
    builder.addVertex(Vector3f(1, 0, 0));
    builder.addVertex(Vector3f(0, 1, 0));
    builder.addTriangle(0, 1, 2);
    
    // Generate normals
    builder.generateSmoothNormals();
    
    Mesh mesh = builder.endMesh();
    
    EXPECT_EQ(mesh.normals.size(), mesh.vertices.size());
    
    // Check that normals are normalized
    for (const auto& normal : mesh.normals) {
        float length = normal.length();
        EXPECT_NEAR(length, 1.0f, 0.001f);
    }
}

TEST(MeshBuilderTest, CombineMeshes) {
    // Create first mesh (triangle)
    MeshBuilder builder1;
    builder1.beginMesh();
    builder1.addVertex(Vector3f(0, 0, 0));
    builder1.addVertex(Vector3f(1, 0, 0));
    builder1.addVertex(Vector3f(0, 1, 0));
    builder1.addTriangle(0, 1, 2);
    Mesh mesh1 = builder1.endMesh();
    
    // Create second mesh (another triangle)
    MeshBuilder builder2;
    builder2.beginMesh();
    builder2.addVertex(Vector3f(2, 0, 0));
    builder2.addVertex(Vector3f(3, 0, 0));
    builder2.addVertex(Vector3f(2, 1, 0));
    builder2.addTriangle(0, 1, 2);
    Mesh mesh2 = builder2.endMesh();
    
    // Combine meshes
    std::vector<Mesh> meshes = {mesh1, mesh2};
    Mesh combined = MeshBuilder::combineMeshes(meshes);
    
    EXPECT_EQ(combined.vertices.size(), 6);
    EXPECT_EQ(combined.indices.size(), 6);
    EXPECT_TRUE(combined.isValid());
}

TEST(MeshTest, CalculateBounds) {
    Mesh mesh;
    mesh.vertices.push_back(Vector3f(-1, -1, -1));
    mesh.vertices.push_back(Vector3f(2, 3, 4));
    mesh.vertices.push_back(Vector3f(0, 0, 0));
    
    mesh.calculateBounds();
    
    EXPECT_EQ(mesh.bounds.min.x, -1);
    EXPECT_EQ(mesh.bounds.min.y, -1);
    EXPECT_EQ(mesh.bounds.min.z, -1);
    EXPECT_EQ(mesh.bounds.max.x, 2);
    EXPECT_EQ(mesh.bounds.max.y, 3);
    EXPECT_EQ(mesh.bounds.max.z, 4);
}

TEST(MeshTest, CalculateNormals) {
    Mesh mesh;
    
    // Create a triangle in XY plane
    mesh.vertices.push_back(Vector3f(0, 0, 0));
    mesh.vertices.push_back(Vector3f(1, 0, 0));
    mesh.vertices.push_back(Vector3f(0, 1, 0));
    mesh.indices = {0, 1, 2};
    
    mesh.calculateNormals();
    
    EXPECT_EQ(mesh.normals.size(), 3);
    
    // All normals should point in +Z direction for a CCW triangle in XY plane
    for (const auto& normal : mesh.normals) {
        EXPECT_NEAR(normal.z, 1.0f, 0.001f);
        EXPECT_NEAR(normal.x, 0.0f, 0.001f);
        EXPECT_NEAR(normal.y, 0.0f, 0.001f);
    }
}

TEST(MeshUtilsTest, CalculateVolume) {
    // Create a unit cube
    Mesh cube;
    
    // 8 vertices
    cube.vertices = {
        Vector3f(0, 0, 0), Vector3f(1, 0, 0),
        Vector3f(1, 1, 0), Vector3f(0, 1, 0),
        Vector3f(0, 0, 1), Vector3f(1, 0, 1),
        Vector3f(1, 1, 1), Vector3f(0, 1, 1)
    };
    
    // 12 triangles (6 faces * 2 triangles each)
    cube.indices = {
        0, 1, 2,  0, 2, 3,  // Bottom
        4, 6, 5,  4, 7, 6,  // Top
        0, 4, 5,  0, 5, 1,  // Front
        2, 6, 7,  2, 7, 3,  // Back
        0, 3, 7,  0, 7, 4,  // Left
        1, 5, 6,  1, 6, 2   // Right
    };
    
    // TODO: Implement MeshUtils::calculateVolume
    // float volume = MeshUtils::calculateVolume(cube);
    // EXPECT_NEAR(volume, 1.0f, 0.001f);
}

TEST(MeshUtilsTest, CalculateSurfaceArea) {
    // Create a unit square
    Mesh square;
    square.vertices = {
        Vector3f(0, 0, 0), Vector3f(1, 0, 0),
        Vector3f(1, 1, 0), Vector3f(0, 1, 0)
    };
    square.indices = {0, 1, 2, 0, 2, 3};
    
    // TODO: Implement MeshUtils::calculateSurfaceArea
    // float area = MeshUtils::calculateSurfaceArea(square);
    // EXPECT_NEAR(area, 1.0f, 0.001f);
}

TEST(SurfaceSettingsTest, Equality) {
    SurfaceSettings s1 = SurfaceSettings::Default();
    SurfaceSettings s2 = SurfaceSettings::Default();
    SurfaceSettings s3 = SurfaceSettings::Preview();
    
    EXPECT_EQ(s1, s2);
    EXPECT_NE(s1, s3);
}

TEST(SurfaceSettingsTest, Hash) {
    SurfaceSettings s1 = SurfaceSettings::Default();
    SurfaceSettings s2 = SurfaceSettings::Default();
    SurfaceSettings s3 = SurfaceSettings::Export();
    
    EXPECT_EQ(s1.hash(), s2.hash());
    EXPECT_NE(s1.hash(), s3.hash());
}

TEST(MeshCacheTest, BasicCaching) {
    MeshCache cache;
    cache.setMaxMemoryUsage(1024 * 1024); // 1MB
    
    // Create a test mesh
    Mesh mesh;
    mesh.vertices = {Vector3f(0, 0, 0), Vector3f(1, 0, 0), Vector3f(0, 1, 0)};
    mesh.indices = {0, 1, 2};
    mesh.calculateBounds();
    
    // Cache the mesh
    std::string key = "test_mesh_1";
    cache.cacheMesh(key, mesh);
    
    // Verify it's cached
    EXPECT_TRUE(cache.hasCachedMesh(key));
    EXPECT_EQ(cache.getHitCount(), 0);
    EXPECT_EQ(cache.getMissCount(), 0);
    
    // Retrieve the mesh
    Mesh retrieved = cache.getCachedMesh(key);
    EXPECT_EQ(retrieved.vertices.size(), mesh.vertices.size());
    EXPECT_EQ(retrieved.indices.size(), mesh.indices.size());
    EXPECT_EQ(cache.getHitCount(), 1);
    
    // Try to get non-existent mesh
    Mesh notFound = cache.getCachedMesh("non_existent");
    EXPECT_TRUE(notFound.isValid()); // Empty mesh is valid
    EXPECT_EQ(notFound.vertices.size(), 0); // But it should be empty
    EXPECT_EQ(notFound.indices.size(), 0);
    EXPECT_EQ(cache.getMissCount(), 1);
}

TEST(MeshCacheTest, LRUEviction) {
    MeshCache cache;
    cache.setMaxMemoryUsage(1000); // Very small cache
    
    // Create multiple meshes
    for (int i = 0; i < 5; ++i) {
        Mesh mesh;
        // Add enough vertices to exceed cache size
        for (int j = 0; j < 10; ++j) {
            mesh.vertices.push_back(Vector3f(i, j, 0));
        }
        mesh.calculateBounds();
        
        std::string key = "mesh_" + std::to_string(i);
        cache.cacheMesh(key, mesh);
        
        // Small delay to ensure different timestamps
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Earlier meshes should have been evicted
    EXPECT_FALSE(cache.hasCachedMesh("mesh_0"));
    EXPECT_FALSE(cache.hasCachedMesh("mesh_1"));
    
    // Later meshes should still be cached
    EXPECT_TRUE(cache.hasCachedMesh("mesh_4"));
}

TEST(MeshCacheTest, RegionInvalidation) {
    MeshCache cache;
    
    // Create meshes in different regions
    Mesh mesh1;
    mesh1.vertices = {Vector3f(0, 0, 0), Vector3f(1, 1, 1)};
    mesh1.calculateBounds();
    cache.cacheMesh("region1", mesh1);
    
    Mesh mesh2;
    mesh2.vertices = {Vector3f(10, 10, 10), Vector3f(11, 11, 11)};
    mesh2.calculateBounds();
    cache.cacheMesh("region2", mesh2);
    
    // Invalidate first region
    BoundingBox invalidateRegion;
    invalidateRegion.min = Vector3f(-1, -1, -1);
    invalidateRegion.max = Vector3f(2, 2, 2);
    cache.invalidateRegion(invalidateRegion);
    
    // First mesh should be gone, second should remain
    EXPECT_FALSE(cache.hasCachedMesh("region1"));
    EXPECT_TRUE(cache.hasCachedMesh("region2"));
}

TEST(LODManagerTest, LODCalculation) {
    LODManager lodManager;
    
    BoundingBox bounds;
    bounds.min = Vector3f(0, 0, 0);
    bounds.max = Vector3f(10, 10, 10);
    
    // Test different distances
    // Note: bounds size is sqrt(10^2 + 10^2 + 10^2) ≈ 17.32
    // Adjusted distances are distance / size
    EXPECT_EQ(lodManager.calculateLOD(5.0f, bounds), LODLevel::LOD0);    // 5/17.32 ≈ 0.29 < 10
    EXPECT_EQ(lodManager.calculateLOD(200.0f, bounds), LODLevel::LOD1);  // 200/17.32 ≈ 11.5 > 10
    EXPECT_EQ(lodManager.calculateLOD(500.0f, bounds), LODLevel::LOD2);  // 500/17.32 ≈ 28.9 > 25
    EXPECT_EQ(lodManager.calculateLOD(1000.0f, bounds), LODLevel::LOD3); // 1000/17.32 ≈ 57.7 > 50
    EXPECT_EQ(lodManager.calculateLOD(2000.0f, bounds), LODLevel::LOD4); // 2000/17.32 ≈ 115.5 > 100
}

TEST(LODManagerTest, SimplificationRatios) {
    LODManager lodManager;
    
    // Check default ratios
    EXPECT_EQ(lodManager.getSimplificationRatio(LODLevel::LOD0), 1.0f);
    EXPECT_EQ(lodManager.getSimplificationRatio(LODLevel::LOD1), 0.5f);
    EXPECT_EQ(lodManager.getSimplificationRatio(LODLevel::LOD2), 0.25f);
    EXPECT_EQ(lodManager.getSimplificationRatio(LODLevel::LOD3), 0.125f);
    EXPECT_EQ(lodManager.getSimplificationRatio(LODLevel::LOD4), 0.0625f);
    
    // Set custom ratio
    lodManager.setSimplificationRatio(LODLevel::LOD1, 0.75f);
    EXPECT_EQ(lodManager.getSimplificationRatio(LODLevel::LOD1), 0.75f);
}