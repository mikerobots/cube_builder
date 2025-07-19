#include <gtest/gtest.h>
#include "../../core/voxel_data/VoxelGrid.h"
#include "../../core/voxel_data/VoxelDataManager.h"
#include "../../core/surface_gen/SurfaceGenerator.h"
#include "../../foundation/math/CoordinateConverter.h"
#include <unordered_set>
#include <iostream>

using namespace VoxelEditor;

class DualContouringAdjacentTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<VoxelData::VoxelDataManager>();
    }
    
    std::unique_ptr<VoxelData::VoxelDataManager> manager;
    
    // Helper to check if a mesh is watertight
    bool isWatertight(const SurfaceGen::Mesh& mesh) {
        // Count edges - each edge should be shared by exactly 2 triangles
        std::map<std::pair<uint32_t, uint32_t>, int> edgeCount;
        
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            uint32_t v0 = mesh.indices[i];
            uint32_t v1 = mesh.indices[i + 1];
            uint32_t v2 = mesh.indices[i + 2];
            
            // Add edges (always store smaller index first for consistency)
            edgeCount[{std::min(v0, v1), std::max(v0, v1)}]++;
            edgeCount[{std::min(v1, v2), std::max(v1, v2)}]++;
            edgeCount[{std::min(v2, v0), std::max(v2, v0)}]++;
        }
        
        // Check all edges are shared by exactly 2 triangles
        for (const auto& [edge, count] : edgeCount) {
            if (count != 2) {
                std::cout << "Edge (" << edge.first << ", " << edge.second 
                         << ") has " << count << " triangles (expected 2)" << std::endl;
                return false;
            }
        }
        
        return true;
    }
    
    // Helper to count unique vertices within tolerance
    size_t countUniqueVertices(const std::vector<Math::WorldCoordinates>& vertices, float tolerance = 0.001f) {
        std::vector<Math::Vector3f> uniqueVerts;
        
        for (const auto& v : vertices) {
            bool found = false;
            for (const auto& u : uniqueVerts) {
                if ((v.value() - u).length() < tolerance) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                uniqueVerts.push_back(v.value());
            }
        }
        
        return uniqueVerts.size();
    }
};

TEST_F(DualContouringAdjacentTest, SingleVoxelMeshProperties) {
    // Place a single 32cm voxel
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    Math::IncrementCoordinates pos(0, 0, 0);
    manager->setVoxel(pos, resolution, true);
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    // Verify mesh properties
    EXPECT_GT(mesh.vertices.size(), 0) << "Mesh should have vertices";
    EXPECT_GT(mesh.indices.size(), 0) << "Mesh should have indices";
    EXPECT_EQ(mesh.indices.size() % 3, 0) << "Indices should be multiple of 3";
    
    size_t triangleCount = mesh.indices.size() / 3;
    std::cout << "Single voxel: " << mesh.vertices.size() << " vertices, " 
              << triangleCount << " triangles" << std::endl;
    
    // Check if mesh is watertight
    EXPECT_TRUE(isWatertight(mesh)) << "Single voxel mesh should be watertight";
}

TEST_F(DualContouringAdjacentTest, TwoAdjacentVoxelsX) {
    // Place two adjacent 32cm voxels along X axis
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    manager->setVoxel(Math::IncrementCoordinates(0, 0, 0), resolution, true);
    manager->setVoxel(Math::IncrementCoordinates(32, 0, 0), resolution, true);
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    size_t triangleCount = mesh.indices.size() / 3;
    size_t uniqueVertCount = countUniqueVertices(mesh.vertices);
    
    std::cout << "Two adjacent voxels (X): " << mesh.vertices.size() << " vertices ("
              << uniqueVertCount << " unique), " 
              << triangleCount << " triangles" << std::endl;
    
    // Check if mesh is watertight
    EXPECT_TRUE(isWatertight(mesh)) << "Adjacent voxels mesh should be watertight";
    
    // The mesh should have fewer triangles than 2x single cube due to shared face
    // A single cube might have ~2000 triangles, two separate would be ~4000
    // With shared face removed, should be significantly less
    EXPECT_LT(triangleCount, 3500) << "Adjacent voxels should share geometry";
}

TEST_F(DualContouringAdjacentTest, TwoAdjacentVoxelsY) {
    // Place two adjacent 32cm voxels along Y axis (stacked)
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    manager->setVoxel(Math::IncrementCoordinates(0, 0, 0), resolution, true);
    manager->setVoxel(Math::IncrementCoordinates(0, 32, 0), resolution, true);
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    size_t triangleCount = mesh.indices.size() / 3;
    std::cout << "Two adjacent voxels (Y): " << mesh.vertices.size() << " vertices, " 
              << triangleCount << " triangles" << std::endl;
    
    // Check if mesh is watertight
    EXPECT_TRUE(isWatertight(mesh)) << "Stacked voxels mesh should be watertight";
}

TEST_F(DualContouringAdjacentTest, FourVoxelSquare) {
    // Place four 32cm voxels in a 2x2 pattern
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    manager->setVoxel(Math::IncrementCoordinates(0, 0, 0), resolution, true);
    manager->setVoxel(Math::IncrementCoordinates(32, 0, 0), resolution, true);
    manager->setVoxel(Math::IncrementCoordinates(0, 0, 32), resolution, true);
    manager->setVoxel(Math::IncrementCoordinates(32, 0, 32), resolution, true);
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    size_t triangleCount = mesh.indices.size() / 3;
    std::cout << "Four voxel square: " << mesh.vertices.size() << " vertices, " 
              << triangleCount << " triangles" << std::endl;
    
    // Check if mesh is watertight
    EXPECT_TRUE(isWatertight(mesh)) << "Four voxel square mesh should be watertight";
}

TEST_F(DualContouringAdjacentTest, CubeOfEightVoxels) {
    // Place eight 32cm voxels in a 2x2x2 cube
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    for (int z = 0; z <= 32; z += 32) {
        for (int y = 0; y <= 32; y += 32) {
            for (int x = 0; x <= 32; x += 32) {
                manager->setVoxel(Math::IncrementCoordinates(x, y, z), resolution, true);
            }
        }
    }
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    size_t triangleCount = mesh.indices.size() / 3;
    std::cout << "Eight voxel cube: " << mesh.vertices.size() << " vertices, " 
              << triangleCount << " triangles" << std::endl;
    
    // Check if mesh is watertight
    EXPECT_TRUE(isWatertight(mesh)) << "Eight voxel cube mesh should be watertight";
    
    // For a 2x2x2 voxel cube, the mesh should be much simpler than 8 separate cubes
    // Should be close to just the outer shell
    EXPECT_LT(triangleCount, 5000) << "2x2x2 cube should have optimized geometry";
}