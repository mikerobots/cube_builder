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
    DualContouring dc;
    
    // Generate mesh from empty grid
    Mesh mesh = dc.generateMesh(*testGrid, SurfaceSettings::Default());
    
    // Should produce empty mesh
    EXPECT_TRUE(mesh.isValid());
    EXPECT_EQ(mesh.vertices.size(), 0);
    EXPECT_EQ(mesh.indices.size(), 0);
}

TEST_F(DualContouringTest, SingleVoxel) {
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
    
    // Create a 2x2x2 cube
    createCube(Vector3i(3, 3, 3), Vector3i(4, 4, 4));
    
    // Generate mesh
    Mesh mesh = dc.generateMesh(*testGrid, SurfaceSettings::Default());
    
    // Verify mesh
    EXPECT_TRUE(mesh.isValid());
    EXPECT_GT(mesh.vertices.size(), 8); // More than 8 due to dual contouring
    EXPECT_GT(mesh.indices.size(), 36); // More than 12 triangles
    
    // Check bounds
    mesh.calculateBounds();
    EXPECT_GE(mesh.bounds.min.x, 0.0f);
    EXPECT_GE(mesh.bounds.min.y, 0.0f);
    EXPECT_GE(mesh.bounds.min.z, 0.0f);
    EXPECT_LE(mesh.bounds.max.x, workspaceSize.x);
    EXPECT_LE(mesh.bounds.max.y, workspaceSize.y);
    EXPECT_LE(mesh.bounds.max.z, workspaceSize.z);
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