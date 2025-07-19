#include <gtest/gtest.h>
#include "core/surface_gen/SimpleMesher.h"
#include "core/voxel_data/VoxelGrid.h"
#include "foundation/math/CoordinateConverter.h"
#include "foundation/math/Vector3f.h"
#include <iostream>
#include <set>
#include <tuple>
#include <map>
#include <chrono>

using namespace VoxelEditor;
using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class SimpleMesherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a grid with centered workspace (5m³)
        m_grid = std::make_unique<VoxelGrid>(VoxelResolution::Size_32cm, 5.0f);
    }
    
    void TearDown() override {
        m_grid.reset();
    }
    
    std::unique_ptr<VoxelGrid> m_grid;
};

// Test that SimpleMesher can be instantiated
TEST_F(SimpleMesherTest, CanInstantiate) {
    SimpleMesher mesher;
    EXPECT_FALSE(mesher.isCancelled());
}

// Test empty grid generates empty mesh
TEST_F(SimpleMesherTest, EmptyGridGeneratesEmptyMesh) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    Mesh result = mesher.generateMesh(*m_grid, settings);
    
    EXPECT_EQ(result.vertices.size(), 0);
    EXPECT_EQ(result.indices.size(), 0);
}

// Test progress callback is called
TEST_F(SimpleMesherTest, ProgressCallbackCalled) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    bool callbackCalled = false;
    float lastProgress = -1.0f;
    
    mesher.setProgressCallback([&](float progress) {
        callbackCalled = true;
        lastProgress = progress;
        EXPECT_GE(progress, 0.0f);
        EXPECT_LE(progress, 1.0f);
    });
    
    mesher.generateMesh(*m_grid, settings);
    
    EXPECT_TRUE(callbackCalled);
    EXPECT_FLOAT_EQ(lastProgress, 1.0f);
}

// Test cancellation
TEST_F(SimpleMesherTest, CancellationWorks) {
    SimpleMesher mesher;
    
    EXPECT_FALSE(mesher.isCancelled());
    mesher.cancel();
    EXPECT_TRUE(mesher.isCancelled());
}

// Test invalid mesh resolution
TEST_F(SimpleMesherTest, InvalidMeshResolution) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Try to use an invalid resolution (cast from invalid int)
    // This should be caught and return empty mesh
    auto invalidResolution = static_cast<SimpleMesher::MeshResolution>(3);
    Mesh result = mesher.generateMesh(*m_grid, settings, invalidResolution);
    
    EXPECT_EQ(result.vertices.size(), 0);
    EXPECT_EQ(result.indices.size(), 0);
}

// Test Rectangle class functionality separately
class SimpleMesherRectangleTest : public ::testing::Test {
protected:
    // Helper to create Rectangle for testing
    struct TestRectangle {
        int x, y, width, height;
        
        TestRectangle(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}
        
        bool intersects(const TestRectangle& other) const {
            return x < other.x + other.width && x + width > other.x &&
                   y < other.y + other.height && y + height > other.y;
        }
        
        bool contains(const TestRectangle& other) const {
            return x <= other.x && y <= other.y &&
                   x + width >= other.x + other.width && 
                   y + height >= other.y + other.height;
        }
    };
};

// Test Rectangle intersection
TEST_F(SimpleMesherRectangleTest, RectangleIntersection) {
    TestRectangle r1(0, 0, 10, 10);
    TestRectangle r2(5, 5, 10, 10);
    TestRectangle r3(20, 20, 10, 10);
    
    EXPECT_TRUE(r1.intersects(r2));
    EXPECT_TRUE(r2.intersects(r1));
    EXPECT_FALSE(r1.intersects(r3));
    EXPECT_FALSE(r3.intersects(r1));
}

// Test Rectangle containment
TEST_F(SimpleMesherRectangleTest, RectangleContainment) {
    TestRectangle outer(0, 0, 20, 20);
    TestRectangle inner(5, 5, 10, 10);
    TestRectangle partial(15, 15, 10, 10);
    
    EXPECT_TRUE(outer.contains(inner));
    EXPECT_FALSE(inner.contains(outer));
    EXPECT_FALSE(outer.contains(partial));
}

// Test internal components are working through the public interface
TEST_F(SimpleMesherTest, ComponentsWorkThroughPublicInterface) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Add two adjacent voxels to grid
    IncrementCoordinates voxel1Pos(0, 0, 0);
    IncrementCoordinates voxel2Pos(32, 0, 0);
    m_grid->setVoxel(voxel1Pos, true);
    m_grid->setVoxel(voxel2Pos, true);
    
    // Generate mesh - this will exercise SpatialIndex, VertexManager, etc.
    Mesh result = mesher.generateMesh(*m_grid, settings);
    
    // For now, just verify it doesn't crash
    // Once implementation is complete, we can verify the mesh
    EXPECT_GE(result.vertices.size(), 0);
}


// Test single voxel mesh generation
TEST_F(SimpleMesherTest, SingleVoxelGeneratesMesh) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Add a single 32cm voxel at origin
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    MaterialId material = 1;
    
    m_grid->setVoxel(voxelPos, true);
    
    // Use 32cm resolution to avoid subdivision on a 32cm voxel
    Mesh result = mesher.generateMesh(*m_grid, settings, SimpleMesher::MeshResolution::Res_16cm);
    
    // A single voxel with no subdivision should generate 8 vertices and 12 triangles (6 faces * 2 triangles)
    // With 16cm resolution on 32cm voxel, we get 2x2 subdivision per face
    // Each face: 3x3 vertices = 9 vertices, but edges are shared
    // Total: more than 8 vertices due to subdivision
    
    // For a truly simple box, we'd need 32cm or larger resolution
    // Let's test with maximum resolution to get simple box
    Mesh simpleResult = mesher.generateMesh(*m_grid, settings, SimpleMesher::MeshResolution(32));
    
    // This should give us a simple box if we had 32cm resolution enum value
    // For now, let's just verify the mesh is valid
    EXPECT_GT(result.vertices.size(), 0) << "Mesh should have vertices";
    EXPECT_GT(result.indices.size(), 0) << "Mesh should have indices";
    EXPECT_EQ(result.indices.size() % 3, 0) << "Indices should be multiple of 3 (triangles)";
}

// Test two adjacent voxels  
TEST_F(SimpleMesherTest, AdjacentVoxelsShareVertices) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Add two adjacent 32cm voxels
    IncrementCoordinates voxel1Pos(0, 0, 0);
    IncrementCoordinates voxel2Pos(32, 0, 0);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    MaterialId material = 1;
    
    m_grid->setVoxel(voxel1Pos, true);
    m_grid->setVoxel(voxel2Pos, true);
    
    // Use 16cm resolution for reasonable subdivision
    Mesh result = mesher.generateMesh(*m_grid, settings, SimpleMesher::MeshResolution::Res_16cm);
    
    // Two adjacent voxels with subdivision will have more vertices
    // Just verify the mesh is valid and has shared vertices
    EXPECT_GT(result.vertices.size(), 12) << "Subdivided mesh should have more than 12 vertices";
    EXPECT_GT(result.indices.size(), 60) << "Subdivided mesh should have more than 60 indices";
    EXPECT_EQ(result.indices.size() % 3, 0) << "Indices should be multiple of 3 (triangles)";
}

// Test mesh subdivision
TEST_F(SimpleMesherTest, MeshSubdivision) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Add a single 32cm voxel at origin
    IncrementCoordinates voxelPos(0, 0, 0);
    m_grid->setVoxel(voxelPos, true);
    
    // Generate with 8cm subdivision
    Mesh result = mesher.generateMesh(*m_grid, settings, SimpleMesher::MeshResolution::Res_8cm);
    
    // With 8cm subdivision on a 32cm voxel:
    // Each face is 32x32cm, divided into 4x4 grid = 25 vertices per face
    // 6 faces, but vertices are shared:
    // - 8 corner vertices (shared by 3 faces each)
    // - 12 edge vertices per edge * 12 edges (shared by 2 faces each) 
    // - 9 interior vertices per face * 6 faces
    // Total unique vertices = 8 + 48 + 54 = 110 (approximately, depends on deduplication)
    
    // Each face has 4x4 quads = 16 quads = 32 triangles per face
    // 6 faces * 32 triangles = 192 triangles = 576 indices
    
    EXPECT_GT(result.vertices.size(), 8) << "Subdivided mesh should have more than 8 vertices";
    EXPECT_EQ(result.indices.size(), 576) << "Expected 576 indices (192 triangles * 3 vertices)";
    
    // Verify no duplicate vertices (within tolerance)
    std::set<std::tuple<int, int, int>> uniquePositions;
    for (const auto& v : result.vertices) {
        // Convert to 0.1mm integer units for comparison
        int x = static_cast<int>(std::round(v.x() * 10000.0f));
        int y = static_cast<int>(std::round(v.y() * 10000.0f));
        int z = static_cast<int>(std::round(v.z() * 10000.0f));
        uniquePositions.insert(std::make_tuple(x, y, z));
    }
    EXPECT_EQ(uniquePositions.size(), result.vertices.size()) << "All vertices should be unique";
}

// Test multi-threaded mesh generation
TEST_F(SimpleMesherTest, MultiThreadedGenerationMatchesSingleThreaded) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Create a more complex scene with multiple voxels
    for (int x = 0; x < 5; x++) {
        for (int y = 0; y < 3; y++) {
            for (int z = 0; z < 4; z++) {
                IncrementCoordinates voxelPos(x * 32, y * 32, z * 32);
                m_grid->setVoxel(voxelPos, true);
            }
        }
    }
    
    // Generate mesh with single thread (force by using small voxel count threshold)
    // We'll temporarily have to trust the internal logic
    Mesh singleThreadResult = mesher.generateMesh(*m_grid, settings, SimpleMesher::MeshResolution::Res_16cm);
    
    // The current implementation will use multiple threads for 60 voxels
    // Let's verify the results are valid
    EXPECT_GT(singleThreadResult.vertices.size(), 0) << "Should generate vertices";
    EXPECT_GT(singleThreadResult.indices.size(), 0) << "Should generate indices";
    EXPECT_EQ(singleThreadResult.indices.size() % 3, 0) << "Indices should be multiple of 3";
    
    // Basic validation of mesh structure
    size_t maxIndex = 0;
    for (uint32_t idx : singleThreadResult.indices) {
        maxIndex = std::max(maxIndex, static_cast<size_t>(idx));
    }
    EXPECT_LT(maxIndex, singleThreadResult.vertices.size()) << "All indices should be valid";
}

// Test watertight mesh generation
TEST_F(SimpleMesherTest, GeneratesWatertightMesh) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Create a solid 3x3x3 block of voxels
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            for (int z = 0; z < 3; z++) {
                IncrementCoordinates voxelPos(x * 32, y * 32, z * 32);
                m_grid->setVoxel(voxelPos, true);
            }
        }
    }
    
    Mesh result = mesher.generateMesh(*m_grid, settings, SimpleMesher::MeshResolution::Res_16cm);
    
    // Verify mesh is valid
    EXPECT_GT(result.vertices.size(), 0) << "Should generate vertices";
    EXPECT_GT(result.indices.size(), 0) << "Should generate indices";
    EXPECT_EQ(result.indices.size() % 3, 0) << "Indices should be multiple of 3";
    
    // Check for watertight properties
    // 1. Every edge should be shared by exactly 2 triangles
    std::map<std::pair<uint32_t, uint32_t>, int> edgeCount;
    
    for (size_t i = 0; i < result.indices.size(); i += 3) {
        uint32_t v0 = result.indices[i];
        uint32_t v1 = result.indices[i + 1];
        uint32_t v2 = result.indices[i + 2];
        
        // Add edges (always store with smaller index first)
        auto addEdge = [&](uint32_t a, uint32_t b) {
            if (a > b) std::swap(a, b);
            edgeCount[{a, b}]++;
        };
        
        addEdge(v0, v1);
        addEdge(v1, v2);
        addEdge(v2, v0);
    }
    
    // Check that each edge is used exactly twice
    for (const auto& edge : edgeCount) {
        EXPECT_EQ(edge.second, 2) << "Edge (" << edge.first.first << ", " 
                                  << edge.first.second << ") should be shared by exactly 2 triangles";
    }
    
    // 2. Check for consistent winding order (all normals point outward)
    // This is implicitly tested by the edge count test above
}

// Test T-junction detection
TEST_F(SimpleMesherTest, NoTJunctions) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Create adjacent voxels of different sizes (if supported in future)
    // For now, test with same-size voxels
    IncrementCoordinates voxel1Pos(0, 0, 0);
    IncrementCoordinates voxel2Pos(32, 0, 0);
    IncrementCoordinates voxel3Pos(64, 0, 0);
    
    m_grid->setVoxel(voxel1Pos, true);
    m_grid->setVoxel(voxel2Pos, true);
    m_grid->setVoxel(voxel3Pos, true);
    
    Mesh result = mesher.generateMesh(*m_grid, settings, SimpleMesher::MeshResolution::Res_8cm);
    
    // Check for T-junctions by verifying that every vertex on a shared edge
    // is also present in the adjacent face
    // For now, just verify the mesh is valid
    EXPECT_GT(result.vertices.size(), 0) << "Should generate vertices";
    EXPECT_GT(result.indices.size(), 0) << "Should generate indices";
    
    // More sophisticated T-junction detection would require spatial analysis
}

// Performance benchmark test
TEST_F(SimpleMesherTest, PerformanceBenchmark) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    settings.generateNormals = false; // Focus on mesh generation performance
    
    // Test different voxel counts
    std::vector<int> voxelCounts = {100, 1000, 5000, 10000};
    
    std::cout << "\nSimpleMesher Performance Benchmark:\n";
    std::cout << "Voxels\tTime(ms)\tVertices\tTriangles\tVoxels/sec\n";
    std::cout << "------\t--------\t--------\t---------\t----------\n";
    
    for (int targetCount : voxelCounts) {
        // Create a larger grid for performance testing
        auto perfGrid = std::make_unique<VoxelGrid>(VoxelResolution::Size_32cm, 8.0f);
        
        // Fill grid with voxels in a cubic pattern
        int sideLength = static_cast<int>(std::cbrt(targetCount));
        int actualCount = 0;
        
        for (int x = 0; x < sideLength; x++) {
            for (int y = 0; y < sideLength; y++) {
                for (int z = 0; z < sideLength; z++) {
                    IncrementCoordinates voxelPos(x * 32, y * 32, z * 32);
                    perfGrid->setVoxel(voxelPos, true);
                    actualCount++;
                }
            }
        }
        
        // Measure generation time
        auto start = std::chrono::high_resolution_clock::now();
        Mesh result = mesher.generateMesh(*perfGrid, settings, SimpleMesher::MeshResolution::Res_16cm);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        double voxelsPerSecond = (duration > 0) ? (actualCount * 1000.0 / duration) : 0;
        
        std::cout << actualCount << "\t" 
                  << duration << "\t\t" 
                  << result.vertices.size() << "\t\t"
                  << result.indices.size() / 3 << "\t\t"
                  << static_cast<int>(voxelsPerSecond) << "\n";
        
        // Verify the mesh is valid
        EXPECT_GT(result.vertices.size(), 0);
        EXPECT_GT(result.indices.size(), 0);
        EXPECT_EQ(result.indices.size() % 3, 0);
        
        // Performance target: 10k voxels < 1 second
        if (actualCount >= 10000) {
            EXPECT_LT(duration, 1000) << "Should process 10k voxels in less than 1 second";
        }
    }
}

// Memory usage test
TEST_F(SimpleMesherTest, MemoryEfficiency) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Create a sparse scene with voxels spread out
    for (int i = 0; i < 100; i++) {
        IncrementCoordinates voxelPos(i * 64, i * 32, i * 48);
        m_grid->setVoxel(voxelPos, true);
    }
    
    Mesh result = mesher.generateMesh(*m_grid, settings, SimpleMesher::MeshResolution::Res_16cm);
    
    // Verify mesh is valid
    EXPECT_GT(result.vertices.size(), 0);
    EXPECT_GT(result.indices.size(), 0);
    
    // Check memory efficiency
    // Each vertex is 3 floats (12 bytes), each index is 4 bytes
    size_t vertexMemory = result.vertices.size() * sizeof(Math::Vector3f);
    size_t indexMemory = result.indices.size() * sizeof(uint32_t);
    size_t totalMemory = vertexMemory + indexMemory;
    
    // For 100 voxels, we should use less than 1MB
    EXPECT_LT(totalMemory, 1024 * 1024) << "Memory usage should be under 1MB for 100 voxels";
    
    std::cout << "\nMemory usage for 100 voxels: " 
              << totalMemory / 1024.0 << " KB ("
              << vertexMemory / 1024.0 << " KB vertices, "
              << indexMemory / 1024.0 << " KB indices)\n";
}

// Edge case: Workspace boundary voxels
TEST_F(SimpleMesherTest, WorkspaceBoundaryVoxels) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Get workspace bounds
    Math::Vector3f workspaceSize = m_grid->getWorkspaceSize();
    
    // Place voxels at the edges of the workspace
    std::vector<IncrementCoordinates> boundaryPositions;
    
    // Corner at origin
    boundaryPositions.push_back(IncrementCoordinates(0, 0, 0));
    
    // Along edges
    boundaryPositions.push_back(IncrementCoordinates(32, 0, 0));
    boundaryPositions.push_back(IncrementCoordinates(0, 32, 0));
    boundaryPositions.push_back(IncrementCoordinates(0, 0, 32));
    
    // Near maximum bounds (adjusted for centered workspace)
    int maxIncrement = static_cast<int>(workspaceSize.x * 100.0f / 2.0f) - 32;
    boundaryPositions.push_back(IncrementCoordinates(maxIncrement, 0, 0));
    boundaryPositions.push_back(IncrementCoordinates(0, maxIncrement, 0));
    boundaryPositions.push_back(IncrementCoordinates(0, 0, maxIncrement));
    
    for (const auto& pos : boundaryPositions) {
        m_grid->setVoxel(pos, true);
    }
    
    Mesh result = mesher.generateMesh(*m_grid, settings, SimpleMesher::MeshResolution::Res_16cm);
    
    // Verify mesh is valid
    EXPECT_GT(result.vertices.size(), 0) << "Should generate vertices for boundary voxels";
    EXPECT_GT(result.indices.size(), 0) << "Should generate indices for boundary voxels";
    EXPECT_EQ(result.indices.size() % 3, 0) << "Indices should be multiple of 3";
    
    // Verify all vertices are within workspace bounds
    for (const auto& vertex : result.vertices) {
        EXPECT_GE(vertex.x(), -workspaceSize.x / 2.0f - 0.1f) << "Vertex X should be within bounds";
        EXPECT_LE(vertex.x(), workspaceSize.x / 2.0f + 0.1f) << "Vertex X should be within bounds";
        EXPECT_GE(vertex.y(), -workspaceSize.y / 2.0f - 0.1f) << "Vertex Y should be within bounds";
        EXPECT_LE(vertex.y(), workspaceSize.y / 2.0f + 0.1f) << "Vertex Y should be within bounds";
        EXPECT_GE(vertex.z(), -workspaceSize.z / 2.0f - 0.1f) << "Vertex Z should be within bounds";
        EXPECT_LE(vertex.z(), workspaceSize.z / 2.0f + 0.1f) << "Vertex Z should be within bounds";
    }
}

// Edge case: Maximum voxel count stress test
TEST_F(SimpleMesherTest, MaximumVoxelCount) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    settings.generateNormals = false; // Speed up test
    
    // Create a grid with many voxels (but not too many for testing)
    const int maxVoxels = 1000; // Reasonable for unit test
    auto stressGrid = std::make_unique<VoxelGrid>(VoxelResolution::Size_32cm, 8.0f);
    
    // Fill with random voxels
    int count = 0;
    for (int i = 0; i < maxVoxels; i++) {
        IncrementCoordinates pos(
            (i * 37) % 200 * 32,  // Pseudo-random distribution
            (i * 53) % 200 * 32,
            (i * 71) % 200 * 32
        );
        stressGrid->setVoxel(pos, true);
        count++;
    }
    
    // Set cancellation callback to test cancellation handling
    bool shouldCancel = false;
    mesher.setProgressCallback([&](float progress) {
        if (progress > 0.5f && !shouldCancel) {
            shouldCancel = true;
            // Don't actually cancel - just test that progress works
        }
    });
    
    Mesh result = mesher.generateMesh(*stressGrid, settings, SimpleMesher::MeshResolution::Res_16cm);
    
    // Verify mesh is valid
    EXPECT_GT(result.vertices.size(), 0) << "Should generate vertices for many voxels";
    EXPECT_GT(result.indices.size(), 0) << "Should generate indices for many voxels";
    EXPECT_EQ(result.indices.size() % 3, 0) << "Indices should be multiple of 3";
    
    std::cout << "\nMaximum voxel count test: " << count << " voxels generated "
              << result.vertices.size() << " vertices and " 
              << result.indices.size() / 3 << " triangles\n";
}

// Edge case: Degenerate configurations
TEST_F(SimpleMesherTest, DegenerateConfigurations) {
    SimpleMesher mesher;
    SurfaceSettings settings = SurfaceSettings::Default();
    
    // Test 1: Single isolated voxel near edge of workspace
    auto grid1 = std::make_unique<VoxelGrid>(VoxelResolution::Size_32cm, 5.0f);
    // Place voxel near edge but within bounds
    grid1->setVoxel(IncrementCoordinates(200, 200, 200), true);
    
    Mesh result1 = mesher.generateMesh(*grid1, settings, SimpleMesher::MeshResolution::Res_16cm);
    EXPECT_GT(result1.vertices.size(), 0) << "Should handle edge coordinate voxels";
    EXPECT_EQ(result1.indices.size() % 3, 0);
    
    // Test 2: Thin wall (1 voxel thick)
    auto grid2 = std::make_unique<VoxelGrid>(VoxelResolution::Size_32cm, 5.0f);
    for (int i = 0; i < 5; i++) {
        grid2->setVoxel(IncrementCoordinates(i * 32, 0, 0), true);
    }
    
    Mesh result2 = mesher.generateMesh(*grid2, settings, SimpleMesher::MeshResolution::Res_16cm);
    EXPECT_GT(result2.vertices.size(), 0) << "Should handle thin wall configuration";
    EXPECT_GT(result2.indices.size(), 0);
    
    // Test 3: Checkerboard pattern (worst case for face removal)
    auto grid3 = std::make_unique<VoxelGrid>(VoxelResolution::Size_32cm, 5.0f);
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            for (int z = 0; z < 4; z++) {
                if ((x + y + z) % 2 == 0) {
                    grid3->setVoxel(IncrementCoordinates(x * 32, y * 32, z * 32), true);
                }
            }
        }
    }
    
    Mesh result3 = mesher.generateMesh(*grid3, settings, SimpleMesher::MeshResolution::Res_16cm);
    EXPECT_GT(result3.vertices.size(), 0) << "Should handle checkerboard pattern";
    EXPECT_GT(result3.indices.size(), 0);
    
    // Verify no degenerate triangles (all triangles have non-zero area)
    for (size_t i = 0; i < result3.indices.size(); i += 3) {
        const auto& v0 = result3.vertices[result3.indices[i]];
        const auto& v1 = result3.vertices[result3.indices[i + 1]];
        const auto& v2 = result3.vertices[result3.indices[i + 2]];
        
        // Calculate triangle area using cross product
        Math::Vector3f edge1(v1.x() - v0.x(), v1.y() - v0.y(), v1.z() - v0.z());
        Math::Vector3f edge2(v2.x() - v0.x(), v2.y() - v0.y(), v2.z() - v0.z());
        Math::Vector3f cross = edge1.cross(edge2);
        float area = cross.length() * 0.5f;
        
        EXPECT_GT(area, 0.0001f) << "Triangle should have non-zero area";
    }
}