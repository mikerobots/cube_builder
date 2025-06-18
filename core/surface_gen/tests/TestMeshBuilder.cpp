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
    uint32_t v0 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 0)));
    uint32_t v1 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 0, 0)));
    uint32_t v2 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 1, 0)));
    
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
    uint32_t v0 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 0)));
    uint32_t v1 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 0, 0)));
    uint32_t v2 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 1, 0)));
    uint32_t v3 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 1, 0)));
    
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
    builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 0)));
    builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 0, 0)));
    builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 0))); // Duplicate
    builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 0, 0))); // Duplicate
    
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
    builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 0)));
    builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 0, 0)));
    builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 1, 0)));
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
    builder1.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 0)));
    builder1.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 0, 0)));
    builder1.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 1, 0)));
    builder1.addTriangle(0, 1, 2);
    Mesh mesh1 = builder1.endMesh();
    
    // Create second mesh (another triangle)
    MeshBuilder builder2;
    builder2.beginMesh();
    builder2.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(2, 0, 0)));
    builder2.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(3, 0, 0)));
    builder2.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(2, 1, 0)));
    builder2.addTriangle(0, 1, 2);
    Mesh mesh2 = builder2.endMesh();
    
    // Combine meshes
    std::vector<Mesh> meshes = {mesh1, mesh2};
    Mesh combined = MeshBuilder::combineMeshes(meshes);
    
    EXPECT_EQ(combined.vertices.size(), 6);
    EXPECT_EQ(combined.indices.size(), 6);
    EXPECT_TRUE(combined.isValid());
}

TEST(MeshBuilderTest, CreateCubeMesh) {
    // Test creating a cube mesh with correct vertex positions and winding order
    // This is critical for voxel rendering
    
    MeshBuilder builder;
    builder.beginMesh();
    
    // Define cube vertices (8 corners of a unit cube)
    // Bottom face (z = 0)
    uint32_t v0 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 0))); // 0: bottom-left-front
    uint32_t v1 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 0, 0))); // 1: bottom-right-front
    uint32_t v2 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 1, 0))); // 2: bottom-right-back
    uint32_t v3 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 1, 0))); // 3: bottom-left-back
    
    // Top face (z = 1)
    uint32_t v4 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 1))); // 4: top-left-front
    uint32_t v5 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 0, 1))); // 5: top-right-front
    uint32_t v6 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 1, 1))); // 6: top-right-back
    uint32_t v7 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 1, 1))); // 7: top-left-back
    
    // Add faces with counter-clockwise winding when viewed from outside
    // Front face (y = 0)
    builder.addQuad(v0, v1, v5, v4);
    
    // Back face (y = 1)
    builder.addQuad(v2, v3, v7, v6);
    
    // Left face (x = 0)
    builder.addQuad(v3, v0, v4, v7);
    
    // Right face (x = 1)
    builder.addQuad(v1, v2, v6, v5);
    
    // Bottom face (z = 0)
    builder.addQuad(v3, v2, v1, v0);
    
    // Top face (z = 1)
    builder.addQuad(v4, v5, v6, v7);
    
    Mesh mesh = builder.endMesh();
    
    // Verify mesh structure
    EXPECT_EQ(mesh.vertices.size(), 8);  // 8 unique vertices
    EXPECT_EQ(mesh.indices.size(), 36);  // 6 faces * 2 triangles * 3 vertices = 36 indices
    EXPECT_TRUE(mesh.isValid());
    
    // Verify vertex positions
    EXPECT_EQ(mesh.vertices[0], Vector3f(0, 0, 0));
    EXPECT_EQ(mesh.vertices[1], Vector3f(1, 0, 0));
    EXPECT_EQ(mesh.vertices[2], Vector3f(1, 1, 0));
    EXPECT_EQ(mesh.vertices[3], Vector3f(0, 1, 0));
    EXPECT_EQ(mesh.vertices[4], Vector3f(0, 0, 1));
    EXPECT_EQ(mesh.vertices[5], Vector3f(1, 0, 1));
    EXPECT_EQ(mesh.vertices[6], Vector3f(1, 1, 1));
    EXPECT_EQ(mesh.vertices[7], Vector3f(0, 1, 1));
    
    // Calculate bounds
    mesh.calculateBounds();
    EXPECT_EQ(mesh.bounds.min, Vector3f(0, 0, 0));
    EXPECT_EQ(mesh.bounds.max, Vector3f(1, 1, 1));
}

TEST(MeshBuilderTest, CubeWindingOrder) {
    // Test that cube faces have correct winding order for back-face culling
    
    MeshBuilder builder;
    builder.beginMesh();
    
    // Create a simple quad to test winding order
    uint32_t v0 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 0)));
    uint32_t v1 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 0, 0)));
    uint32_t v2 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 1, 0)));
    uint32_t v3 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 1, 0)));
    
    // Add quad with CCW winding
    builder.addQuad(v0, v1, v2, v3);
    
    Mesh mesh = builder.endMesh();
    
    // Verify the triangulation maintains CCW order
    // First triangle: v0, v1, v2
    EXPECT_EQ(mesh.indices[0], 0);
    EXPECT_EQ(mesh.indices[1], 1);
    EXPECT_EQ(mesh.indices[2], 2);
    
    // Second triangle: v0, v2, v3
    EXPECT_EQ(mesh.indices[3], 0);
    EXPECT_EQ(mesh.indices[4], 2);
    EXPECT_EQ(mesh.indices[5], 3);
    
    // Generate normals to verify they point outward
    builder.generateFlatNormals();
    mesh = builder.endMesh();
    
    // All normals should point in +Z direction for a quad in XY plane
    for (const auto& normal : mesh.normals) {
        EXPECT_GT(normal.z, 0.9f);  // Should be close to 1.0
        EXPECT_NEAR(normal.x, 0.0f, 0.1f);
        EXPECT_NEAR(normal.y, 0.0f, 0.1f);
    }
}

TEST(MeshTest, CalculateBounds) {
    Mesh mesh;
    mesh.vertices.push_back(WorldCoordinates(Vector3f(-1, -1, -1)));
    mesh.vertices.push_back(WorldCoordinates(Vector3f(2, 3, 4)));
    mesh.vertices.push_back(WorldCoordinates(Vector3f(0, 0, 0)));
    
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

TEST(MeshBuilderTest, VertexWindingOrderValidation) {
    // Test that all triangles have consistent counter-clockwise winding order
    // This is critical for proper back-face culling and lighting
    
    MeshBuilder builder;
    builder.beginMesh();
    
    // Create a cube with each face added separately
    // We'll verify each face has CCW winding when viewed from outside
    
    // Front face vertices (facing +Z)
    uint32_t f0 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 1)));
    uint32_t f1 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 0, 1)));
    uint32_t f2 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(1, 1, 1)));
    uint32_t f3 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(0, 1, 1)));
    
    // Add front face with CCW winding
    builder.addQuad(f0, f1, f2, f3);
    
    Mesh mesh = builder.endMesh();
    
    // Verify winding order by calculating face normal
    Vector3f v0 = mesh.vertices[mesh.indices[0]];
    Vector3f v1 = mesh.vertices[mesh.indices[1]];
    Vector3f v2 = mesh.vertices[mesh.indices[2]];
    
    Vector3f edge1 = v1 - v0;
    Vector3f edge2 = v2 - v0;
    Vector3f normal = edge1.cross(edge2).normalized();
    
    // For front face, normal should point in +Z direction
    EXPECT_GT(normal.z, 0.9f);
    EXPECT_NEAR(normal.x, 0.0f, 0.1f);
    EXPECT_NEAR(normal.y, 0.0f, 0.1f);
}

TEST(MeshBuilderTest, NormalDirectionValidation) {
    // Test that normals point outward from the surface
    // This is critical for proper lighting calculations
    
    MeshBuilder builder;
    builder.beginMesh();
    
    // Create a complete cube
    float size = 1.0f;
    
    // Define all 8 vertices
    std::vector<Vector3f> positions = {
        Vector3f(0, 0, 0), Vector3f(size, 0, 0),
        Vector3f(size, size, 0), Vector3f(0, size, 0),
        Vector3f(0, 0, size), Vector3f(size, 0, size),
        Vector3f(size, size, size), Vector3f(0, size, size)
    };
    
    std::vector<uint32_t> vertexIds;
    for (const auto& pos : positions) {
        vertexIds.push_back(builder.addVertex(VoxelEditor::Math::WorldCoordinates(pos)));
    }
    
    // Add all 6 faces with proper CCW winding
    // Front face (+Z)
    builder.addQuad(vertexIds[4], vertexIds[5], vertexIds[6], vertexIds[7]);
    // Back face (-Z)
    builder.addQuad(vertexIds[1], vertexIds[0], vertexIds[3], vertexIds[2]);
    // Right face (+X)
    builder.addQuad(vertexIds[5], vertexIds[1], vertexIds[2], vertexIds[6]);
    // Left face (-X)
    builder.addQuad(vertexIds[0], vertexIds[4], vertexIds[7], vertexIds[3]);
    // Top face (+Y)
    builder.addQuad(vertexIds[7], vertexIds[6], vertexIds[2], vertexIds[3]);
    // Bottom face (-Y)
    builder.addQuad(vertexIds[0], vertexIds[1], vertexIds[5], vertexIds[4]);
    
    // Generate flat normals (one per face)
    builder.generateFlatNormals();
    Mesh mesh = builder.endMesh();
    
    // Verify normals point outward
    // For a cube centered around (0.5, 0.5, 0.5), we can check normal directions
    Vector3f center(size/2, size/2, size/2);
    
    // Check several vertices and their normals
    for (size_t i = 0; i < mesh.vertices.size() && i < mesh.normals.size(); ++i) {
        Vector3f toVertex = (mesh.vertices[i] - center).normalized();
        float dot = mesh.normals[i].dot(toVertex);
        
        // Normal should point in same general direction as vector from center to vertex
        EXPECT_GT(dot, 0.5f) << "Normal at vertex " << i << " doesn't point outward";
    }
}

TEST(MeshBuilderTest, ConsistentTriangulation) {
    // Test that quad triangulation maintains consistent winding order
    
    MeshBuilder builder;
    builder.beginMesh();
    
    // Create multiple quads to ensure triangulation is consistent
    for (int i = 0; i < 3; ++i) {
        float offset = i * 2.0f;
        uint32_t v0 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(offset, 0, 0)));
        uint32_t v1 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(offset + 1, 0, 0)));
        uint32_t v2 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(offset + 1, 1, 0)));
        uint32_t v3 = builder.addVertex(VoxelEditor::Math::WorldCoordinates(Vector3f(offset, 1, 0)));
        
        builder.addQuad(v0, v1, v2, v3);
    }
    
    Mesh mesh = builder.endMesh();
    
    // Each quad should produce 2 triangles with consistent winding
    EXPECT_EQ(mesh.indices.size(), 18); // 3 quads * 2 triangles * 3 vertices
    
    // Check that all triangles have positive area (CCW winding)
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        Vector3f v0 = mesh.vertices[mesh.indices[i]];
        Vector3f v1 = mesh.vertices[mesh.indices[i + 1]];
        Vector3f v2 = mesh.vertices[mesh.indices[i + 2]];
        
        // Calculate 2D cross product (assuming Z=0 plane)
        float crossZ = (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
        
        // Should be positive for CCW winding
        EXPECT_GT(crossZ, 0.0f) << "Triangle " << i/3 << " has incorrect winding order";
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
    mesh.vertices = {WorldCoordinates(Vector3f(0, 0, 0)), WorldCoordinates(Vector3f(1, 0, 0)), WorldCoordinates(Vector3f(0, 1, 0))};
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
    mesh1.vertices = {WorldCoordinates(Vector3f(0, 0, 0)), WorldCoordinates(Vector3f(1, 1, 1))};
    mesh1.calculateBounds();
    cache.cacheMesh("region1", mesh1);
    
    Mesh mesh2;
    mesh2.vertices = {WorldCoordinates(Vector3f(10, 10, 10)), WorldCoordinates(Vector3f(11, 11, 11))};
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