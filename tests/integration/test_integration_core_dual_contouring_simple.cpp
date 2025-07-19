#include <gtest/gtest.h>
#include "../../core/voxel_data/VoxelGrid.h"
#include "../../core/voxel_data/VoxelDataManager.h"
#include "../../core/surface_gen/SurfaceGenerator.h"
#include "../../core/surface_gen/MeshValidator.h"
#include "../../foundation/math/CoordinateConverter.h"
#include <iostream>

using namespace VoxelEditor;

// Helper function to validate and print mesh details
void validateAndPrintMesh(const SurfaceGen::Mesh& mesh, const std::string& testName) {
    SurfaceGen::MeshValidator validator;
    auto result = validator.validate(mesh);
    auto stats = validator.calculateStatistics(mesh);
    
    std::cout << "\n=== " << testName << " Mesh Validation ===\n";
    std::cout << "Vertices: " << stats.vertexCount << "\n";
    std::cout << "Triangles: " << stats.triangleCount << "\n";
    std::cout << "Edges: " << stats.edgeCount << "\n";
    std::cout << "Watertight: " << (result.isWatertight ? "YES" : "NO") << "\n";
    std::cout << "Manifold: " << (result.isManifold ? "YES" : "NO") << "\n";
    std::cout << "Holes: " << result.holeCount << "\n";
    std::cout << "Non-manifold edges: " << result.nonManifoldEdges << "\n";
    std::cout << "Bounds: (" << stats.boundingBoxMin.x << "," << stats.boundingBoxMin.y << "," << stats.boundingBoxMin.z << ") to ("
              << stats.boundingBoxMax.x << "," << stats.boundingBoxMax.y << "," << stats.boundingBoxMax.z << ")\n";
    
    if (!result.errors.empty()) {
        std::cout << "ERRORS:\n";
        for (const auto& err : result.errors) {
            std::cout << "  - " << err << "\n";
        }
    }
    if (!result.warnings.empty()) {
        std::cout << "WARNINGS:\n";
        for (const auto& warn : result.warnings) {
            std::cout << "  - " << warn << "\n";
        }
    }
    std::cout << "================================\n";
}

class DualContouringSimpleTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<VoxelData::VoxelDataManager>();
    }
    
    std::unique_ptr<VoxelData::VoxelDataManager> manager;
};

TEST_F(DualContouringSimpleTest, SingleVoxelShouldGenerateCompleteMesh) {
    // Place a single 32cm voxel at origin
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    Math::IncrementCoordinates pos(0, 0, 0);
    manager->setVoxel(pos, resolution, true);
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    // Validate mesh using MeshValidator
    validateAndPrintMesh(mesh, "Single voxel at (0,0,0)");
    
    // A complete cube should have at least 8 vertices and 12 triangles
    // Dual contouring might generate more for smooth surfaces
    EXPECT_GE(mesh.vertices.size(), 8) << "Should have at least 8 vertices for a complete mesh";
    EXPECT_GE(mesh.indices.size() / 3, 12) << "Should have at least 12 triangles for a complete mesh";
    
    // Check if mesh is watertight
    SurfaceGen::MeshValidator validator;
    EXPECT_TRUE(validator.isWatertight(mesh)) << "Mesh should be watertight";
    
    // Check that vertices span the expected volume
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
        
        // The mesh should span at least part of all three dimensions
        float xSpan = maxBounds.x - minBounds.x;
        float ySpan = maxBounds.y - minBounds.y;
        float zSpan = maxBounds.z - minBounds.z;
        
        EXPECT_GT(xSpan, 0.1f) << "Mesh should span in X direction";
        EXPECT_GT(ySpan, 0.1f) << "Mesh should span in Y direction";
        EXPECT_GT(zSpan, 0.1f) << "Mesh should span in Z direction";
    }
}

TEST_F(DualContouringSimpleTest, TwoAdjacentVoxelsShouldShareGeometry) {
    // Place two 32cm voxels side by side
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    manager->setVoxel(Math::IncrementCoordinates(0, 0, 0), resolution, true);
    manager->setVoxel(Math::IncrementCoordinates(32, 0, 0), resolution, true);
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    // Validate mesh using MeshValidator
    validateAndPrintMesh(mesh, "Two adjacent voxels along X");
    
    // Should have more triangles than a single voxel but less than 2x
    // because the shared face should not be duplicated
    EXPECT_GE(mesh.indices.size() / 3, 20) << "Should have at least 20 triangles (no shared face)";
    EXPECT_LE(mesh.indices.size() / 3, 24) << "Should not have more than 24 triangles";
    
    // Check if mesh is watertight
    SurfaceGen::MeshValidator validator;
    EXPECT_TRUE(validator.isWatertight(mesh)) << "Mesh should be watertight";
    
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
        
        // Should span from approximately 0 to 0.64 in X (two 32cm voxels)
        EXPECT_LT(minBounds.x, 0.1f) << "Min X should be near 0";
        EXPECT_GT(maxBounds.x, 0.5f) << "Max X should be beyond 0.5";
    }
}

TEST_F(DualContouringSimpleTest, VoxelAtDifferentPosition) {
    // Place a single 32cm voxel at a different position to test coordinate handling
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    Math::IncrementCoordinates pos(64, 0, 64); // 64cm offset in X and Z
    manager->setVoxel(pos, resolution, true);
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    std::cout << "\nSingle voxel at (64,0,64):\n";
    std::cout << "  Vertices: " << mesh.vertices.size() << "\n";
    std::cout << "  Triangles: " << mesh.indices.size() / 3 << "\n";
    
    // Should generate similar mesh as voxel at origin
    EXPECT_GE(mesh.vertices.size(), 6) << "Should have at least 6 vertices";
    EXPECT_GE(mesh.indices.size() / 3, 8) << "Should have at least 8 triangles";
    
    // Check bounds - should be around (0.64, 0, 0.64) to (0.96, 0.32, 0.96)
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
        
        // Should be centered around 0.64-0.96 in X and Z
        EXPECT_GT(minBounds.x, 0.5f) << "Min X should be above 0.5";
        EXPECT_GT(minBounds.z, 0.5f) << "Min Z should be above 0.5";
    }
}