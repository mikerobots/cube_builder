#include <gtest/gtest.h>
#include "../../core/voxel_data/VoxelGrid.h"
#include "../../core/voxel_data/VoxelDataManager.h"
#include "../../core/surface_gen/SurfaceGenerator.h"
#include "../../foundation/math/CoordinateConverter.h"
#include <iostream>
#include <set>

using namespace VoxelEditor;

class DualContouringVertexValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<VoxelData::VoxelDataManager>();
    }
    
    std::unique_ptr<VoxelData::VoxelDataManager> manager;
    
    void printMeshInfo(const SurfaceGen::Mesh& mesh, const std::string& label) {
        std::cout << "\n" << label << ":\n";
        std::cout << "  Vertices: " << mesh.vertices.size() << "\n";
        std::cout << "  Triangles: " << (mesh.indices.size() / 3) << "\n";
        
        // Find bounds
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
        
        // Print first few vertices
        for (size_t i = 0; i < std::min(size_t(5), mesh.vertices.size()); ++i) {
            const auto& v = mesh.vertices[i];
            std::cout << "  Vertex " << i << ": (" << v.value().x << ", " << v.value().y << ", " << v.value().z << ")\n";
        }
    }
    
    bool areVerticesReasonable(const SurfaceGen::Mesh& mesh, 
                               const Math::Vector3f& expectedMin, 
                               const Math::Vector3f& expectedMax,
                               float tolerance = 0.5f) {
        if (mesh.vertices.empty()) {
            std::cout << "ERROR: No vertices in mesh\n";
            return false;
        }
        
        // Check that all vertices are within reasonable bounds
        // Dual contouring can generate vertices slightly outside voxel bounds
        Math::Vector3f expandedMin = expectedMin - Math::Vector3f(tolerance, tolerance, tolerance);
        Math::Vector3f expandedMax = expectedMax + Math::Vector3f(tolerance, tolerance, tolerance);
        
        int outOfBoundsCount = 0;
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            const auto& v = mesh.vertices[i].value();
            if (v.x < expandedMin.x || v.x > expandedMax.x ||
                v.y < expandedMin.y || v.y > expandedMax.y ||
                v.z < expandedMin.z || v.z > expandedMax.z) {
                std::cout << "  Vertex " << i << " out of bounds: (" 
                          << v.x << ", " << v.y << ", " << v.z << ")\n";
                outOfBoundsCount++;
            }
        }
        
        if (outOfBoundsCount > 0) {
            std::cout << "  " << outOfBoundsCount << " vertices out of expected bounds\n";
            return false;
        }
        
        return true;
    }
};

TEST_F(DualContouringVertexValidationTest, SingleVoxelAt000) {
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
    
    printMeshInfo(mesh, "Single voxel at (0,0,0)");
    
    // A 32cm voxel at (0,0,0) occupies space from (0,0,0) to (0.32,0.32,0.32) meters
    // We expect vertices to be roughly within this range (with some tolerance for dual contouring)
    Math::Vector3f expectedMin(0.0f, 0.0f, 0.0f);
    Math::Vector3f expectedMax(0.32f, 0.32f, 0.32f);
    
    EXPECT_TRUE(areVerticesReasonable(mesh, expectedMin, expectedMax)) 
        << "Vertices should be within reasonable bounds of the voxel";
    
    // Should have a reasonable number of triangles for a cube-like shape
    EXPECT_GE(mesh.indices.size() / 3, 12) << "Should have at least 12 triangles (simple cube)";
    EXPECT_LE(mesh.indices.size() / 3, 100) << "Shouldn't have excessive triangles";
}

TEST_F(DualContouringVertexValidationTest, TwoAdjacentVoxelsAlongX) {
    // Place two 32cm voxels side by side along X axis
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    manager->setVoxel(Math::IncrementCoordinates(0, 0, 0), resolution, true);
    manager->setVoxel(Math::IncrementCoordinates(32, 0, 0), resolution, true); // 32cm = 32 increments
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    printMeshInfo(mesh, "Two adjacent voxels along X");
    
    // Two 32cm voxels: (0,0,0)-(0.32,0.32,0.32) and (0.32,0,0)-(0.64,0.32,0.32)
    // Combined they occupy (0,0,0) to (0.64,0.32,0.32)
    Math::Vector3f expectedMin(0.0f, 0.0f, 0.0f);
    Math::Vector3f expectedMax(0.64f, 0.32f, 0.32f);
    
    EXPECT_TRUE(areVerticesReasonable(mesh, expectedMin, expectedMax)) 
        << "Vertices should be within reasonable bounds of the two voxels";
    
    // Should have fewer triangles than 2x single voxel due to shared face
    EXPECT_GE(mesh.indices.size() / 3, 20) << "Should have at least 20 triangles";
    EXPECT_LE(mesh.indices.size() / 3, 150) << "Should have reasonable triangle count with shared geometry";
    
    // Check that we don't have duplicate vertices at the shared boundary
    // Vertices near X=0.32 should be shared
    int verticesNearBoundary = 0;
    for (const auto& v : mesh.vertices) {
        if (std::abs(v.value().x - 0.32f) < 0.05f) {
            verticesNearBoundary++;
        }
    }
    std::cout << "  Vertices near shared boundary (X=0.32): " << verticesNearBoundary << "\n";
    EXPECT_GE(verticesNearBoundary, 1) << "Should have vertices at the shared boundary";
}

TEST_F(DualContouringVertexValidationTest, TwoAdjacentVoxelsAlongY) {
    // Place two 32cm voxels stacked along Y axis
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    manager->setVoxel(Math::IncrementCoordinates(0, 0, 0), resolution, true);
    manager->setVoxel(Math::IncrementCoordinates(0, 32, 0), resolution, true);
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    printMeshInfo(mesh, "Two adjacent voxels along Y (stacked)");
    
    // Two 32cm voxels: (0,0,0)-(0.32,0.32,0.32) and (0,0.32,0)-(0.32,0.64,0.32)
    Math::Vector3f expectedMin(0.0f, 0.0f, 0.0f);
    Math::Vector3f expectedMax(0.32f, 0.64f, 0.32f);
    
    EXPECT_TRUE(areVerticesReasonable(mesh, expectedMin, expectedMax)) 
        << "Vertices should be within reasonable bounds of the stacked voxels";
    
    // Check vertices near shared boundary at Y=0.32
    int verticesNearBoundary = 0;
    for (const auto& v : mesh.vertices) {
        if (std::abs(v.value().y - 0.32f) < 0.05f) {
            verticesNearBoundary++;
        }
    }
    std::cout << "  Vertices near shared boundary (Y=0.32): " << verticesNearBoundary << "\n";
    EXPECT_GE(verticesNearBoundary, 1) << "Should have vertices at the shared boundary";
}

TEST_F(DualContouringVertexValidationTest, SingleVoxelAtNegativeCoords) {
    // Place a single 32cm voxel at negative coordinates
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    Math::IncrementCoordinates pos(-32, 0, -32); // -32cm in X and Z
    manager->setVoxel(pos, resolution, true);
    
    // Generate mesh
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    printMeshInfo(mesh, "Single voxel at (-32,0,-32)");
    
    // Voxel at (-32,0,-32) occupies (-0.32,0,-0.32) to (0,0.32,0) in world coords
    Math::Vector3f expectedMin(-0.32f, 0.0f, -0.32f);
    Math::Vector3f expectedMax(0.0f, 0.32f, 0.0f);
    
    EXPECT_TRUE(areVerticesReasonable(mesh, expectedMin, expectedMax)) 
        << "Vertices should be within reasonable bounds of the voxel at negative coords";
}

TEST_F(DualContouringVertexValidationTest, FourVoxelSquareXZ) {
    // Place four 32cm voxels in a 2x2 square on the XZ plane
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
    
    printMeshInfo(mesh, "Four voxel square (2x2 on XZ plane)");
    
    // Four voxels form a 0.64x0.32x0.64 meter block
    Math::Vector3f expectedMin(0.0f, 0.0f, 0.0f);
    Math::Vector3f expectedMax(0.64f, 0.32f, 0.64f);
    
    EXPECT_TRUE(areVerticesReasonable(mesh, expectedMin, expectedMax)) 
        << "Vertices should be within reasonable bounds of the 2x2 voxel square";
    
    // Check for vertices at internal shared edges
    int internalVertices = 0;
    for (const auto& v : mesh.vertices) {
        // Check if near X=0.32 or Z=0.32 (internal boundaries)
        bool nearInternalX = std::abs(v.value().x - 0.32f) < 0.05f && 
                            v.value().z > 0.05f && v.value().z < 0.59f;
        bool nearInternalZ = std::abs(v.value().z - 0.32f) < 0.05f && 
                            v.value().x > 0.05f && v.value().x < 0.59f;
        if (nearInternalX || nearInternalZ) {
            internalVertices++;
        }
    }
    std::cout << "  Vertices near internal boundaries: " << internalVertices << "\n";
    EXPECT_GE(internalVertices, 1) << "Should have vertices at internal shared boundaries";
}