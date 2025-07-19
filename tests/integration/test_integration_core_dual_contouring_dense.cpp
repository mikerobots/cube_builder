#include <gtest/gtest.h>
#include "../../core/voxel_data/VoxelGrid.h"
#include "../../core/voxel_data/VoxelDataManager.h"
#include "../../core/surface_gen/SurfaceGenerator.h"
#include "../../foundation/math/CoordinateConverter.h"
#include <iostream>

using namespace VoxelEditor;

class DualContouringDenseTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<VoxelData::VoxelDataManager>();
    }
    
    std::unique_ptr<VoxelData::VoxelDataManager> manager;
};

TEST_F(DualContouringDenseTest, DenseVoxelGrid) {
    // Create a 3x3x3 cube of voxels to see if dense placement helps
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    std::cout << "\nPlacing 3x3x3 voxel cube:\n";
    int count = 0;
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            for (int z = 0; z < 3; z++) {
                Math::IncrementCoordinates pos(x * 32, y * 32, z * 32);
                manager->setVoxel(pos, resolution, true);
                count++;
            }
        }
    }
    std::cout << "Placed " << count << " voxels\n";
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    std::cout << "\n3x3x3 voxel cube mesh:\n";
    std::cout << "  Vertices: " << mesh.vertices.size() << "\n";
    std::cout << "  Triangles: " << mesh.indices.size() / 3 << "\n";
    
    // With 27 voxels in a 3x3x3 cube, we should get a proper mesh
    EXPECT_GT(mesh.vertices.size(), 20) << "Should have many vertices for dense voxel cube";
    EXPECT_GT(mesh.indices.size() / 3, 30) << "Should have many triangles for dense voxel cube";
    
    // Check bounds
    if (!mesh.vertices.empty()) {
        Math::Vector3f minBounds = mesh.vertices[0].value();
        Math::Vector3f maxBounds = mesh.vertices[0].value();
        
        for (const auto& v : mesh.vertices) {
            minBounds.x = std::min(minBounds.x, v.value().x);
            minBounds.y = std::min(minBounds.y, v.value().y);
            minBounds.z = std::min(minBounds.z, v.value().z);
            maxBounds.x = std::max(maxBounds.x, v.value().x);
            maxBounds.y = std::max(maxBounds.y, v.value().y);
            maxBounds.z = std::max(maxBounds.z, v.value().z);
        }
        
        std::cout << "  Bounds: (" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z 
                  << ") to (" << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")\n";
    }
}

TEST_F(DualContouringDenseTest, SingleVoxelAlignedToGrid) {
    // Try placing a voxel at a position that aligns with the dual contouring grid
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    // The dual contouring grid starts at (-250, 0, -250) with 32cm cells
    // So positions like (-250, 0, -250), (-218, 0, -218), etc. should align
    // Let's try a position that's a multiple of 32 from the grid origin
    
    // Calculate aligned position: we want a position that when divided by 32 gives a clean grid position
    // Grid cell (7,0,7) would be at position (-250 + 7*32, 0, -250 + 7*32) = (-26, 0, -26)
    // But we want the voxel position, which should be at the cell position for alignment
    
    Math::IncrementCoordinates pos(-26, 0, -26);
    manager->setVoxel(pos, resolution, true);
    std::cout << "\nPlaced voxel at grid-aligned position (-26, 0, -26)\n";
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    std::cout << "Grid-aligned single voxel mesh:\n";
    std::cout << "  Vertices: " << mesh.vertices.size() << "\n";
    std::cout << "  Triangles: " << mesh.indices.size() / 3 << "\n";
    
    // Even with grid alignment, we expect a complete mesh
    EXPECT_GE(mesh.vertices.size(), 6) << "Should have vertices for complete mesh";
    EXPECT_GE(mesh.indices.size() / 3, 8) << "Should have triangles for complete mesh";
}

TEST_F(DualContouringDenseTest, SmallVoxelTest) {
    // Test with smaller voxels to see if that helps
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    // Place a small cube of 1cm voxels
    std::cout << "\nPlacing 8x8x8 cube of 1cm voxels:\n";
    int count = 0;
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            for (int z = 0; z < 8; z++) {
                Math::IncrementCoordinates pos(x, y, z);
                manager->setVoxel(pos, resolution, true);
                count++;
            }
        }
    }
    std::cout << "Placed " << count << " 1cm voxels\n";
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    std::cout << "8x8x8 1cm voxel cube mesh:\n";
    std::cout << "  Vertices: " << mesh.vertices.size() << "\n";
    std::cout << "  Triangles: " << mesh.indices.size() / 3 << "\n";
    
    // With 1cm voxels matching the cell size, we should get a proper mesh
    EXPECT_GT(mesh.vertices.size(), 50) << "Should have many vertices for 1cm voxel cube";
    EXPECT_GT(mesh.indices.size() / 3, 80) << "Should have many triangles for 1cm voxel cube";
}