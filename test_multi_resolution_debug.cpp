#include <iostream>
#include <iomanip>
#include <gtest/gtest.h>
#include "core/voxel_data/VoxelDataManager.h"
#include "core/surface_gen/SurfaceGenerator.h"
#include "core/surface_gen/SimpleMesher.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::SurfaceGen;

void analyzeMesh(const std::string& label, const Mesh& mesh) {
    if (mesh.vertices.empty()) {
        std::cout << label << ": Empty mesh" << std::endl;
        return;
    }
    
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
    
    std::cout << label << ":" << std::endl;
    std::cout << "  Vertices: " << mesh.vertices.size() << std::endl;
    std::cout << "  Min (mm): (" << min.x * 1000 << ", " << min.y * 1000 << ", " << min.z * 1000 << ")" << std::endl;
    std::cout << "  Max (mm): (" << max.x * 1000 << ", " << max.y * 1000 << ", " << max.z * 1000 << ")" << std::endl;
    std::cout << "  Size (mm): " << (max.x - min.x) * 1000 << " x " << (max.y - min.y) * 1000 << " x " << (max.z - min.z) * 1000 << std::endl;
}

TEST(MultiResDebug, TwoVoxelScenario) {
    std::cout << std::fixed << std::setprecision(1);
    
    // Create voxel manager
    VoxelDataManager voxelManager;
    voxelManager.resizeWorkspace(Vector3f(5.0f, 5.0f, 5.0f));
    
    // Place 64cm voxel at (0, 0, 64)
    std::cout << "\n=== Placing voxels ===" << std::endl;
    ASSERT_TRUE(voxelManager.setVoxel(IncrementCoordinates(0, 0, 64), 
                                     VoxelResolution::Size_64cm, true));
    std::cout << "Placed 64cm voxel at increment (0, 0, 64)" << std::endl;
    
    // Place 16cm voxel at (32, 64, 96)
    ASSERT_TRUE(voxelManager.setVoxel(IncrementCoordinates(32, 64, 96), 
                                     VoxelResolution::Size_16cm, true));
    std::cout << "Placed 16cm voxel at increment (32, 64, 96)" << std::endl;
    
    // Create surface generator
    SurfaceGenerator surfaceGen;
    
    // Generate individual meshes first
    std::cout << "\n=== Individual mesh generation ===" << std::endl;
    
    const VoxelGrid* grid64 = voxelManager.getGrid(VoxelResolution::Size_64cm);
    if (grid64) {
        Mesh mesh64 = surfaceGen.generateSurface(*grid64, SurfaceSettings());
        analyzeMesh("64cm mesh", mesh64);
    }
    
    const VoxelGrid* grid16 = voxelManager.getGrid(VoxelResolution::Size_16cm);
    if (grid16) {
        Mesh mesh16 = surfaceGen.generateSurface(*grid16, SurfaceSettings());
        analyzeMesh("16cm mesh", mesh16);
    }
    
    // Generate multi-resolution mesh
    std::cout << "\n=== Multi-resolution mesh generation ===" << std::endl;
    Mesh multiMesh = surfaceGen.generateMultiResMesh(voxelManager, VoxelResolution::Size_16cm);
    analyzeMesh("Multi-resolution mesh", multiMesh);
    
    // The issue: Check if the bounds are correct
    std::cout << "\n=== Checking for issues ===" << std::endl;
    
    // Expected: 64cm voxel should have Y from 0 to 640mm
    // Expected: 16cm voxel should have Y from 640 to 800mm
    // Total mesh should span Y from 0 to 800mm
    
    WorldCoordinates firstVertex = multiMesh.vertices[0];
    float minY = firstVertex.y();
    float maxY = firstVertex.y();
    
    for (const auto& v : multiMesh.vertices) {
        minY = std::min(minY, v.y());
        maxY = std::max(maxY, v.y());
    }
    
    float minY_mm = minY * 1000;
    float maxY_mm = maxY * 1000;
    float totalY_mm = maxY_mm - minY_mm;
    
    std::cout << "Multi-mesh Y range: " << minY_mm << " to " << maxY_mm << " (total: " << totalY_mm << "mm)" << std::endl;
    
    EXPECT_NEAR(minY_mm, 0.0f, 1.0f) << "Minimum Y should be 0mm";
    EXPECT_NEAR(maxY_mm, 800.0f, 1.0f) << "Maximum Y should be 800mm (64cm + 16cm voxel on top)";
    EXPECT_NEAR(totalY_mm, 800.0f, 1.0f) << "Total Y height should be 800mm";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}