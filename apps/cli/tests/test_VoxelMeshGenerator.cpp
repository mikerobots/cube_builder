#include <gtest/gtest.h>
#include "cli/VoxelMeshGenerator.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "math/Vector3f.h"
#include "math/Vector3i.h"
#include <cmath>
#include <memory>
#include <set>
#include <chrono>

namespace VoxelEditor {
namespace Tests {

class VoxelMeshGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create VoxelDataManager
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
        // Set workspace size to 5m
        voxelManager->resizeWorkspace(Math::Vector3f(5.0f, 5.0f, 5.0f));
        meshGenerator = std::make_unique<VoxelMeshGenerator>();
    }
    
    // Helper to count unique vertices (considering floating point precision)
    size_t countUniqueVertices(const Rendering::Mesh& mesh) {
        std::set<std::tuple<int, int, int>> uniquePositions;
        for (const auto& vertex : mesh.vertices) {
            // Round to avoid floating point comparison issues
            int x = static_cast<int>(std::round(vertex.position.x * 1000));
            int y = static_cast<int>(std::round(vertex.position.y * 1000));
            int z = static_cast<int>(std::round(vertex.position.z * 1000));
            uniquePositions.insert({x, y, z});
        }
        return uniquePositions.size();
    }
    
    // Helper to validate cube geometry
    bool validateCubeGeometry(const Rendering::Mesh& mesh, const Math::Vector3f& expectedCenter, float expectedSize) {
        // A cube should have 24 vertices (4 per face * 6 faces) and 36 indices (6 per face * 6 faces)
        if (mesh.vertices.size() != 24 || mesh.indices.size() != 36) {
            return false;
        }
        
        // Check that all vertices are at the expected distance from center
        float halfSize = expectedSize * 0.5f;
        for (const auto& vertex : mesh.vertices) {
            Math::Vector3f diff = vertex.position - expectedCenter;
            
            // Each component should be either +halfSize or -halfSize
            bool validX = std::abs(std::abs(diff.x) - halfSize) < 0.001f;
            bool validY = std::abs(std::abs(diff.y) - halfSize) < 0.001f;
            bool validZ = std::abs(std::abs(diff.z) - halfSize) < 0.001f;
            
            if (!validX || !validY || !validZ) {
                return false;
            }
        }
        
        return true;
    }
    
    // Helper to validate normals
    bool validateNormals(const Rendering::Mesh& mesh) {
        for (const auto& vertex : mesh.vertices) {
            // Normal should be unit length
            float length = vertex.normal.length();
            if (std::abs(length - 1.0f) > 0.001f) {
                return false;
            }
            
            // Normal should align with one of the 6 cardinal directions
            bool isCardinal = false;
            if (std::abs(std::abs(vertex.normal.x) - 1.0f) < 0.001f && 
                std::abs(vertex.normal.y) < 0.001f && 
                std::abs(vertex.normal.z) < 0.001f) {
                isCardinal = true; // X-aligned
            } else if (std::abs(vertex.normal.x) < 0.001f && 
                       std::abs(std::abs(vertex.normal.y) - 1.0f) < 0.001f && 
                       std::abs(vertex.normal.z) < 0.001f) {
                isCardinal = true; // Y-aligned
            } else if (std::abs(vertex.normal.x) < 0.001f && 
                       std::abs(vertex.normal.y) < 0.001f && 
                       std::abs(std::abs(vertex.normal.z) - 1.0f) < 0.001f) {
                isCardinal = true; // Z-aligned
            }
            
            if (!isCardinal) {
                return false;
            }
        }
        return true;
    }
    
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
    std::unique_ptr<VoxelMeshGenerator> meshGenerator;
};

// Test 1: Empty voxel data should produce empty mesh
TEST_F(VoxelMeshGeneratorTest, EmptyVoxelData) {
    // Generate mesh from empty voxel data
    auto mesh = meshGenerator->generateCubeMesh(*voxelManager);
    
    EXPECT_EQ(mesh.vertices.size(), 0);
    EXPECT_EQ(mesh.indices.size(), 0);
}

// Test 2: Single voxel at origin
TEST_F(VoxelMeshGeneratorTest, SingleVoxelAtOrigin) {
    // Set resolution to 8cm
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    // Place single voxel at grid position (0,0,0)
    Math::Vector3i gridPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(gridPos, VoxelData::VoxelResolution::Size_8cm, true));
    
    // Generate mesh
    auto mesh = meshGenerator->generateCubeMesh(*voxelManager);
    
    // Verify mesh has correct structure
    EXPECT_EQ(mesh.vertices.size(), 24); // 4 vertices * 6 faces
    EXPECT_EQ(mesh.indices.size(), 36);  // 6 indices * 6 faces
    
    // Verify voxel center position (grid (0,0,0) with 8cm voxels -> world center at (0.04, 0.04, 0.04))
    Math::Vector3f expectedCenter(0.04f, 0.04f, 0.04f);
    float expectedSize = 0.08f * 0.95f; // 8cm with 0.95 scale factor
    
    EXPECT_TRUE(validateCubeGeometry(mesh, expectedCenter, expectedSize));
    EXPECT_TRUE(validateNormals(mesh));
    
    // Verify all vertices have red color (1.0, 0.0, 0.0)
    for (const auto& vertex : mesh.vertices) {
        EXPECT_FLOAT_EQ(vertex.color.r, 1.0f);
        EXPECT_FLOAT_EQ(vertex.color.g, 0.0f);
        EXPECT_FLOAT_EQ(vertex.color.b, 0.0f);
        EXPECT_FLOAT_EQ(vertex.color.a, 1.0f);
    }
}

// Test 3: Multiple voxels in a line
TEST_F(VoxelMeshGeneratorTest, MultipleVoxelsInLine) {
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    // Place 3 voxels in a line along X axis
    const int voxelCount = 3;
    for (int i = 0; i < voxelCount; ++i) {
        Math::Vector3i gridPos(i, 0, 0);
        ASSERT_TRUE(voxelManager->setVoxel(gridPos, VoxelData::VoxelResolution::Size_8cm, true));
    }
    
    // Generate mesh
    auto mesh = meshGenerator->generateCubeMesh(*voxelManager);
    
    // Each voxel contributes 24 vertices and 36 indices
    EXPECT_EQ(mesh.vertices.size(), 24 * voxelCount);
    EXPECT_EQ(mesh.indices.size(), 36 * voxelCount);
    
    // Verify index ranges are correct
    uint32_t maxIndex = 0;
    for (uint32_t idx : mesh.indices) {
        maxIndex = std::max(maxIndex, idx);
    }
    EXPECT_LT(maxIndex, mesh.vertices.size());
}

// Test 4: 5x5x5 voxel cube (edge case for default workspace)
TEST_F(VoxelMeshGeneratorTest, FullWorkspaceCube) {
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    // Fill a 5x5x5 grid
    int placedCount = 0;
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            for (int z = 0; z < 5; ++z) {
                Math::Vector3i gridPos(x, y, z);
                if (voxelManager->setVoxel(gridPos, VoxelData::VoxelResolution::Size_8cm, true)) {
                    placedCount++;
                }
            }
        }
    }
    
    EXPECT_EQ(placedCount, 125); // 5^3
    
    // Generate mesh
    auto mesh = meshGenerator->generateCubeMesh(*voxelManager);
    
    EXPECT_EQ(mesh.vertices.size(), 24 * 125);
    EXPECT_EQ(mesh.indices.size(), 36 * 125);
}

// Test 5: Different resolutions
TEST_F(VoxelMeshGeneratorTest, DifferentResolutions) {
    // Test with 1cm resolution
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_1cm);
    Math::Vector3i gridPos1cm(10, 10, 10);
    ASSERT_TRUE(voxelManager->setVoxel(gridPos1cm, VoxelData::VoxelResolution::Size_1cm, true));
    
    auto mesh1cm = meshGenerator->generateCubeMesh(*voxelManager);
    EXPECT_EQ(mesh1cm.vertices.size(), 24);
    
    // Expected world position for 1cm voxel at grid (10,10,10)
    Math::Vector3f expectedCenter1cm(0.105f, 0.105f, 0.105f); // (10.5 * 0.01)
    float expectedSize1cm = 0.01f * 0.95f;
    EXPECT_TRUE(validateCubeGeometry(mesh1cm, expectedCenter1cm, expectedSize1cm));
    
    // Clear and test with 64cm resolution
    voxelManager->clearAll();
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_64cm);
    Math::Vector3i gridPos64cm(1, 1, 1);
    ASSERT_TRUE(voxelManager->setVoxel(gridPos64cm, VoxelData::VoxelResolution::Size_64cm, true));
    
    auto mesh64cm = meshGenerator->generateCubeMesh(*voxelManager);
    EXPECT_EQ(mesh64cm.vertices.size(), 24);
    
    // Expected world position for 64cm voxel at grid (1,1,1)
    Math::Vector3f expectedCenter64cm(0.96f, 0.96f, 0.96f); // (1.5 * 0.64)
    float expectedSize64cm = 0.64f * 0.95f;
    EXPECT_TRUE(validateCubeGeometry(mesh64cm, expectedCenter64cm, expectedSize64cm));
}

// Test 6: Verify static cube data
TEST_F(VoxelMeshGeneratorTest, ValidateStaticCubeData) {
    // This test validates the static arrays by checking a known configuration
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    Math::Vector3i gridPos(1, 1, 1);
    ASSERT_TRUE(voxelManager->setVoxel(gridPos, VoxelData::VoxelResolution::Size_8cm, true));
    
    auto mesh = meshGenerator->generateCubeMesh(*voxelManager);
    
    // Check triangle winding order (should be counter-clockwise when viewed from outside)
    // This is a simplified check - we verify that indices form valid triangles
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        uint32_t i0 = mesh.indices[i];
        uint32_t i1 = mesh.indices[i + 1];
        uint32_t i2 = mesh.indices[i + 2];
        
        // All indices should be valid
        EXPECT_LT(i0, mesh.vertices.size());
        EXPECT_LT(i1, mesh.vertices.size());
        EXPECT_LT(i2, mesh.vertices.size());
        
        // Indices should be different
        EXPECT_NE(i0, i1);
        EXPECT_NE(i1, i2);
        EXPECT_NE(i0, i2);
    }
}

// Test 7: Coordinate system alignment
TEST_F(VoxelMeshGeneratorTest, CoordinateSystemAlignment) {
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    // Place voxel at a known position
    Math::Vector3i gridPos(2, 3, 4);
    ASSERT_TRUE(voxelManager->setVoxel(gridPos, VoxelData::VoxelResolution::Size_8cm, true));
    
    auto mesh = meshGenerator->generateCubeMesh(*voxelManager);
    
    // Calculate expected world center
    // worldPos = gridPos * voxelSize + voxelSize * 0.5
    float voxelSize = 0.08f;
    Math::Vector3f expectedCenter(
        2 * voxelSize + voxelSize * 0.5f,
        3 * voxelSize + voxelSize * 0.5f,
        4 * voxelSize + voxelSize * 0.5f
    );
    
    // Verify the mesh is centered at the expected position
    Math::Vector3f actualCenter(0.0f, 0.0f, 0.0f);
    for (const auto& vertex : mesh.vertices) {
        actualCenter = actualCenter + vertex.position;
    }
    actualCenter = actualCenter * (1.0f / mesh.vertices.size());
    
    EXPECT_NEAR(actualCenter.x, expectedCenter.x, 0.001f);
    EXPECT_NEAR(actualCenter.y, expectedCenter.y, 0.001f);
    EXPECT_NEAR(actualCenter.z, expectedCenter.z, 0.001f);
}

// Test 8: Large voxel count performance characteristics
TEST_F(VoxelMeshGeneratorTest, LargeVoxelCount) {
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    // Create a 10x10x10 cube (1000 voxels)
    const int gridSize = 10;
    int placedCount = 0;
    
    for (int x = 0; x < gridSize; ++x) {
        for (int y = 0; y < gridSize; ++y) {
            for (int z = 0; z < gridSize; ++z) {
                Math::Vector3i gridPos(x, y, z);
                if (voxelManager->setVoxel(gridPos, VoxelData::VoxelResolution::Size_8cm, true)) {
                    placedCount++;
                }
            }
        }
    }
    
    EXPECT_GT(placedCount, 0); // Should place at least some voxels
    
    // Measure mesh generation
    auto start = std::chrono::high_resolution_clock::now();
    auto mesh = meshGenerator->generateCubeMesh(*voxelManager);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Verify mesh size
    EXPECT_EQ(mesh.vertices.size(), 24 * placedCount);
    EXPECT_EQ(mesh.indices.size(), 36 * placedCount);
    
    // Performance should be reasonable (less than 1 second for 1000 voxels)
    EXPECT_LT(duration.count(), 1000);
    
    // Log performance info
    std::cout << "Generated mesh for " << placedCount << " voxels in " 
              << duration.count() << "ms" << std::endl;
}

// Test 9: Sparse voxel pattern
TEST_F(VoxelMeshGeneratorTest, SparseVoxelPattern) {
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    // Create a checkerboard pattern
    int placedCount = 0;
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            for (int z = 0; z < 8; ++z) {
                if ((x + y + z) % 2 == 0) {
                    Math::Vector3i gridPos(x, y, z);
                    if (voxelManager->setVoxel(gridPos, VoxelData::VoxelResolution::Size_8cm, true)) {
                        placedCount++;
                    }
                }
            }
        }
    }
    
    auto mesh = meshGenerator->generateCubeMesh(*voxelManager);
    
    // Should have exactly half the voxels of a full 8x8x8 cube
    EXPECT_EQ(mesh.vertices.size(), 24 * placedCount);
    EXPECT_EQ(mesh.indices.size(), 36 * placedCount);
    EXPECT_EQ(placedCount, 256); // Half of 512
}

// Test 10: Face normal orientation
TEST_F(VoxelMeshGeneratorTest, FaceNormalOrientation) {
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    Math::Vector3i gridPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(gridPos, VoxelData::VoxelResolution::Size_8cm, true));
    
    auto mesh = meshGenerator->generateCubeMesh(*voxelManager);
    
    // Count normals by direction
    int posX = 0, negX = 0, posY = 0, negY = 0, posZ = 0, negZ = 0;
    
    for (const auto& vertex : mesh.vertices) {
        if (vertex.normal.x > 0.9f) posX++;
        else if (vertex.normal.x < -0.9f) negX++;
        else if (vertex.normal.y > 0.9f) posY++;
        else if (vertex.normal.y < -0.9f) negY++;
        else if (vertex.normal.z > 0.9f) posZ++;
        else if (vertex.normal.z < -0.9f) negZ++;
    }
    
    // Each face should have exactly 4 vertices
    EXPECT_EQ(posX, 4); // Right face
    EXPECT_EQ(negX, 4); // Left face
    EXPECT_EQ(posY, 4); // Top face
    EXPECT_EQ(negY, 4); // Bottom face
    EXPECT_EQ(posZ, 4); // Back face
    EXPECT_EQ(negZ, 4); // Front face
}

} // namespace Tests
} // namespace VoxelEditor