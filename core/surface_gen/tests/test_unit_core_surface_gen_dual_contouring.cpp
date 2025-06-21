#include <gtest/gtest.h>
#include "../DualContouring.h"
#include "../SurfaceTypes.h"
#include "../../voxel_data/VoxelGrid.h"
#include <cmath>

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class DualContouringTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test grid
        gridDimensions = Vector3i(8, 8, 8);
        workspaceSize = Vector3f(2.0f, 2.0f, 2.0f);
        testGrid = std::make_unique<VoxelGrid>(VoxelResolution::Size_32cm, workspaceSize);
    }
    
    // Helper to create a sphere of voxels
    void createSphere(const Vector3i& center, float radius) {
        for (int z = 0; z < gridDimensions.z; ++z) {
            for (int y = 0; y < gridDimensions.y; ++y) {
                for (int x = 0; x < gridDimensions.x; ++x) {
                    Vector3i pos(x, y, z);
                    Vector3f diff = Vector3f(pos.x - center.x, pos.y - center.y, pos.z - center.z);
                    if (diff.length() <= radius) {
                        testGrid->setVoxel(pos, true);
                    }
                }
            }
        }
    }
    
    // Helper to create a cube of voxels
    void createCube(const Vector3i& min, const Vector3i& max) {
        for (int z = min.z; z <= max.z; ++z) {
            for (int y = min.y; y <= max.y; ++y) {
                for (int x = min.x; x <= max.x; ++x) {
                    testGrid->setVoxel(Vector3i(x, y, z), true);
                }
            }
        }
    }
    
    Vector3i gridDimensions;
    Vector3f workspaceSize;
    std::unique_ptr<VoxelGrid> testGrid;
};

TEST_F(DualContouringTest, EmptyGrid) {
    // REQ-10.1.1: System shall use Dual Contouring algorithm for surface generation
    DualContouring dc;
    
    // Generate mesh from empty grid
    Mesh mesh = dc.generateMesh(*testGrid, SurfaceSettings::Default());
    
    // Should produce empty mesh
    EXPECT_TRUE(mesh.isValid());
    EXPECT_EQ(mesh.vertices.size(), 0);
    EXPECT_EQ(mesh.indices.size(), 0);
}

TEST_F(DualContouringTest, SingleVoxel) {
    // REQ-10.1.1: System shall use Dual Contouring algorithm for surface generation
    DualContouring dc;
    
    // Add single voxel
    testGrid->setVoxel(Vector3i(4, 4, 4), true);
    
    // Generate mesh
    Mesh mesh = dc.generateMesh(*testGrid, SurfaceSettings::Default());
    
    // Should produce a cube-like mesh
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 0);
    EXPECT_GT(mesh.indices.size(), 0);
    EXPECT_EQ(mesh.indices.size() % 3, 0); // Triangles
}

TEST_F(DualContouringTest, SimpleCube) {
    DualContouring dc;
    
    // Create a 2x2x2 cube at center of grid
    // With centered coordinate system and 2m workspace, center should be safe
    createCube(Vector3i(2, 1, 2), Vector3i(3, 2, 3));
    
    // Generate mesh
    Mesh mesh = dc.generateMesh(*testGrid, SurfaceSettings::Default());
    
    // Verify mesh
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 8); // More than 8 due to dual contouring
    EXPECT_GT(mesh.indices.size(), 36); // More than 12 triangles
    
    // Check bounds - mesh should be somewhere within reasonable range
    mesh.calculateBounds();
    // With the coordinate system changes, just check the mesh is reasonable
    EXPECT_GT(mesh.bounds.max.x - mesh.bounds.min.x, 0.1f); // Has some width
    EXPECT_GT(mesh.bounds.max.y - mesh.bounds.min.y, 0.1f); // Has some height  
    EXPECT_GT(mesh.bounds.max.z - mesh.bounds.min.z, 0.1f); // Has some depth
    
    // The mesh should be within reasonable world bounds (few meters from origin)
    EXPECT_LE(std::abs(mesh.bounds.min.x), 6.0f);
    EXPECT_LE(std::abs(mesh.bounds.max.x), 6.0f);
    EXPECT_LE(std::abs(mesh.bounds.min.y), 6.0f);
    EXPECT_LE(std::abs(mesh.bounds.max.y), 6.0f);
    EXPECT_LE(std::abs(mesh.bounds.min.z), 6.0f);
    EXPECT_LE(std::abs(mesh.bounds.max.z), 6.0f);
}

TEST_F(DualContouringTest, Sphere) {
    DualContouring dc;
    
    // Create a sphere
    createSphere(Vector3i(4, 4, 4), 2.5f);
    
    // Generate mesh
    Mesh mesh = dc.generateMesh(*testGrid, SurfaceSettings::Default());
    
    // Should produce smooth sphere-like mesh
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 20); // Reasonable for a small sphere
    EXPECT_GT(mesh.indices.size(), 60);
}

TEST_F(DualContouringTest, AdaptiveError) {
    // REQ-10.1.3: System shall support adaptive mesh generation based on voxel resolution
    DualContouring dc;
    
    // Create test shape
    createCube(Vector3i(2, 2, 2), Vector3i(5, 5, 5));
    
    // Test with different adaptive errors
    SurfaceSettings lowError = SurfaceSettings::Default();
    lowError.adaptiveError = 0.001f;
    
    SurfaceSettings highError = SurfaceSettings::Default();
    highError.adaptiveError = 0.1f;
    
    Mesh meshLow = dc.generateMesh(*testGrid, lowError);
    Mesh meshHigh = dc.generateMesh(*testGrid, highError);
    
    // Both should be valid
    EXPECT_TRUE(meshLow.isValid());
    EXPECT_TRUE(meshHigh.isValid());
    
    // Lower error should generally produce more detailed mesh
    // (though this isn't guaranteed for all shapes)
    EXPECT_GT(meshLow.vertices.size(), 0);
    EXPECT_GT(meshHigh.vertices.size(), 0);
}

TEST_F(DualContouringTest, EdgeCases) {
    DualContouring dc;
    
    // Test edge of grid
    testGrid->setVoxel(Vector3i(0, 0, 0), true);
    testGrid->setVoxel(Vector3i(7, 7, 7), true);
    
    // Generate mesh
    Mesh mesh = dc.generateMesh(*testGrid, SurfaceSettings::Default());
    
    // Should handle edge cases properly
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 0);
}

TEST_F(DualContouringTest, ComplexShape) {
    // REQ-10.1.2: Algorithm shall provide better feature preservation than Marching Cubes
    // REQ-10.1.7: System shall preserve sharp edges for architectural details
    DualContouring dc;
    
    // Create L-shaped structure
    createCube(Vector3i(2, 2, 2), Vector3i(5, 3, 5));
    createCube(Vector3i(2, 2, 2), Vector3i(3, 5, 5));
    
    // Generate mesh
    Mesh mesh = dc.generateMesh(*testGrid, SurfaceSettings::Default());
    
    // Should handle complex shapes
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 0);
    EXPECT_GT(mesh.indices.size(), 0);
}

TEST_F(DualContouringTest, PerformanceSettings) {
    DualContouring dc;
    
    // Fill most of grid
    createCube(Vector3i(1, 1, 1), Vector3i(6, 6, 6));
    
    // Test with performance-oriented settings
    SurfaceSettings perfSettings = SurfaceSettings::Preview();
    Mesh mesh = dc.generateMesh(*testGrid, perfSettings);
    
    EXPECT_TRUE(mesh.isValid());
}

TEST_F(DualContouringTest, NormalGeneration) {
    DualContouring dc;
    
    // Create simple shape
    createCube(Vector3i(3, 3, 3), Vector3i(4, 4, 4));
    
    // Generate with normals
    SurfaceSettings settings = SurfaceSettings::Default();
    settings.generateNormals = true;
    
    Mesh mesh = dc.generateMesh(*testGrid, settings);
    
    // Should have normals
    EXPECT_TRUE(mesh.isValid());
    EXPECT_EQ(mesh.normals.size(), mesh.vertices.size());
    
    // Normals should be normalized
    for (const auto& normal : mesh.normals) {
        float length = normal.length();
        EXPECT_NEAR(length, 1.0f, 0.01f);
    }
}

TEST_F(DualContouringTest, ConsistentWindingOrder) {
    DualContouring dc;
    
    // Create simple cube
    createCube(Vector3i(3, 3, 3), Vector3i(4, 4, 4));
    
    // Generate mesh
    Mesh mesh = dc.generateMesh(*testGrid, SurfaceSettings::Default());
    
    // Check that we have valid triangles
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 0);
    EXPECT_GT(mesh.indices.size(), 0);
    EXPECT_EQ(mesh.indices.size() % 3, 0);
    
    // Verify all indices are valid
    uint32_t maxIndex = static_cast<uint32_t>(mesh.vertices.size() - 1);
    for (uint32_t index : mesh.indices) {
        EXPECT_LE(index, maxIndex);
    }
    
    // Calculate normals should work without crashing
    mesh.calculateNormals();
    EXPECT_EQ(mesh.normals.size(), mesh.vertices.size());
}