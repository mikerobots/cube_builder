#include <iostream>
#include <iomanip>
#include <gtest/gtest.h>
#include "core/voxel_data/VoxelDataManager.h"
#include "core/voxel_data/VoxelGrid.h"
#include "core/surface_gen/SimpleMesher.h"
#include "foundation/logging/Logger.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::SurfaceGen;

TEST(SimpleMeshDebug, SingleVoxelMeshGeneration) {
    // Create a simple voxel grid with one 64cm voxel
    VoxelGrid grid(VoxelResolution::Size_64cm, Vector3f(5.0f, 5.0f, 5.0f));
    
    // Place voxel at (0, 0, 64)
    IncrementCoordinates pos(0, 0, 64);
    ASSERT_TRUE(grid.setVoxel(pos, true));
    
    // Get all voxels to verify
    auto voxels = grid.getAllVoxels();
    ASSERT_EQ(voxels.size(), 1);
    EXPECT_EQ(voxels[0].incrementPos.x(), 0);
    EXPECT_EQ(voxels[0].incrementPos.y(), 0);
    EXPECT_EQ(voxels[0].incrementPos.z(), 64);
    
    // Generate mesh using SimpleMesher
    SimpleMesher mesher;
    SurfaceSettings settings;
    settings.smoothingLevel = 0;
    
    Mesh mesh = mesher.generateMesh(grid, settings, SimpleMesher::MeshResolution::Res_8cm);
    
    // Analyze mesh bounds
    ASSERT_GT(mesh.vertices.size(), 0);
    
    WorldCoordinates firstVertex = mesh.vertices[0];
    Vector3f min(firstVertex.x(), firstVertex.y(), firstVertex.z());
    Vector3f max(firstVertex.x(), firstVertex.y(), firstVertex.z());
    
    for (const auto& v : mesh.vertices) {
        min.x = std::min(min.x, v.x());
        min.y = std::min(min.y, v.y());
        min.z = std::min(min.z, v.z());
        max.x = std::max(max.x, v.x());
        max.y = std::max(max.y, v.y());
        max.z = std::max(max.z, v.z());
    }
    
    std::cout << "\nMesh bounds analysis:" << std::endl;
    std::cout << "Min (meters): (" << min.x << ", " << min.y << ", " << min.z << ")" << std::endl;
    std::cout << "Max (meters): (" << max.x << ", " << max.y << ", " << max.z << ")" << std::endl;
    std::cout << "Size (meters): " << (max.x - min.x) << " x " << (max.y - min.y) << " x " << (max.z - min.z) << std::endl;
    std::cout << "Min (mm): (" << min.x * 1000 << ", " << min.y * 1000 << ", " << min.z * 1000 << ")" << std::endl;
    std::cout << "Max (mm): (" << max.x * 1000 << ", " << max.y * 1000 << ", " << max.z * 1000 << ")" << std::endl;
    std::cout << "Size (mm): " << (max.x - min.x) * 1000 << " x " << (max.y - min.y) * 1000 << " x " << (max.z - min.z) * 1000 << std::endl;
    
    // Expected bounds for 64cm voxel at (0, 0, 64):
    // World position: (0.0, 0.0, 0.64) meters
    // Size: 0.64 x 0.64 x 0.64 meters
    // So bounds should be: (0.0, 0.0, 0.64) to (0.64, 0.64, 1.28)
    
    EXPECT_NEAR(min.x, 0.0f, 0.001f);
    EXPECT_NEAR(min.y, 0.0f, 0.001f);
    EXPECT_NEAR(min.z, 0.64f, 0.001f);
    
    EXPECT_NEAR(max.x, 0.64f, 0.001f);
    EXPECT_NEAR(max.y, 0.64f, 0.001f);
    EXPECT_NEAR(max.z, 1.28f, 0.001f);
    
    // Check size
    EXPECT_NEAR(max.x - min.x, 0.64f, 0.001f) << "X size should be 0.64m";
    EXPECT_NEAR(max.y - min.y, 0.64f, 0.001f) << "Y size should be 0.64m";
    EXPECT_NEAR(max.z - min.z, 0.64f, 0.001f) << "Z size should be 0.64m";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}