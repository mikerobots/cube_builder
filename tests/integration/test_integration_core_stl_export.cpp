#include <gtest/gtest.h>
#include "../../core/voxel_data/VoxelGrid.h"
#include "../../core/voxel_data/VoxelDataManager.h"
#include "file_io/STLExporter.h"
#include "../../core/surface_gen/SurfaceGenerator.h"
#include "../../foundation/math/CoordinateConverter.h"
#include "../../foundation/voxel_math/include/voxel_math/VoxelGridMath.h"
#include <fstream>
#include <cstring>

using namespace VoxelEditor;

class STLExportTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create voxel data manager
        manager = std::make_unique<VoxelData::VoxelDataManager>();
    }
    
    std::unique_ptr<VoxelData::VoxelDataManager> manager;
};

TEST_F(STLExportTest, SingleVoxelExportCoordinates) {
    // Set resolution to 32cm
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    // Place a single voxel at origin
    Math::IncrementCoordinates origin(0, 0, 0);
    manager->setVoxel(origin, resolution, true);
    
    // Get the grid
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr) << "Grid should exist";
    
    // Generate surface mesh
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    // Verify mesh has vertices
    ASSERT_GT(mesh.vertices.size(), 0) << "Mesh should have vertices";
    
    // Debug mesh structure
    std::cout << "\nMesh structure:" << std::endl;
    std::cout << "  Vertices: " << mesh.vertices.size() << std::endl;
    std::cout << "  Indices: " << mesh.indices.size() << std::endl;
    std::cout << "  Triangles: " << mesh.indices.size() / 3 << std::endl;
    
    // Check vertex coordinates are in expected range
    // For a 32cm voxel at origin (0,0,0) with bottom-center coordinates:
    // X: -0.16 to +0.16
    // Y: 0.00 to 0.32
    // Z: -0.16 to +0.16
    
    std::cout << "\nGenerated " << mesh.vertices.size() << " vertices for 32cm voxel at origin:" << std::endl;
    
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const Math::WorldCoordinates& vertex = mesh.vertices[i];
        const Math::Vector3f& pos = vertex.value();
        
        std::cout << "Vertex " << i << ": (" 
                  << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        
        // Check X coordinate
        EXPECT_GE(pos.x, -0.20f) << "X coordinate too small at vertex " << i;
        EXPECT_LE(pos.x, 0.20f) << "X coordinate too large at vertex " << i;
        
        // Check Y coordinate  
        EXPECT_GE(pos.y, -0.04f) << "Y coordinate too small at vertex " << i;
        EXPECT_LE(pos.y, 0.36f) << "Y coordinate too large at vertex " << i;
        
        // Check Z coordinate
        EXPECT_GE(pos.z, -0.20f) << "Z coordinate too small at vertex " << i;
        EXPECT_LE(pos.z, 0.20f) << "Z coordinate too large at vertex " << i;
    }
    
    // Convert SurfaceGen::Mesh to Rendering::Mesh for STL export
    Rendering::Mesh renderMesh;
    renderMesh.vertices.reserve(mesh.vertices.size());
    for (const auto& v : mesh.vertices) {
        renderMesh.vertices.push_back(v.value());
    }
    renderMesh.indices = mesh.indices;
    
    // Debug triangles
    std::cout << "\nTriangles (first few):" << std::endl;
    for (size_t i = 0; i < std::min(mesh.indices.size(), size_t(12)); i += 3) {
        if (i + 2 < mesh.indices.size()) {
            std::cout << "Triangle " << i/3 << ": " 
                      << mesh.indices[i] << ", " 
                      << mesh.indices[i+1] << ", " 
                      << mesh.indices[i+2] << std::endl;
        }
    }
    
    // Export to STL
    FileIO::STLExporter exporter;
    std::string testFile = "test_single_voxel.stl";
    FileIO::STLExportOptions options = FileIO::STLExportOptions::Default();
    options.format = FileIO::STLFormat::Binary;
    options.validateWatertight = false;  // Disable validation temporarily
    options.units = FileIO::STLUnits::Meters;  // Use meters instead of millimeters
    bool exportSuccess = exporter.exportMesh(testFile, renderMesh, options);
    ASSERT_TRUE(exportSuccess) << "STL export should succeed";
    
    // Read and verify STL file
    std::ifstream file(testFile, std::ios::binary);
    ASSERT_TRUE(file.is_open()) << "Should be able to open exported STL file";
    
    // Skip header (80 bytes)
    file.seekg(80);
    
    // Read triangle count
    uint32_t triangleCount;
    file.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));
    EXPECT_GT(triangleCount, 0) << "STL should contain triangles";
    
    std::cout << "\nSTL file contains " << triangleCount << " triangles" << std::endl;
    
    // Read first few triangles and check vertices
    for (uint32_t t = 0; t < std::min(triangleCount, 3u); ++t) {
        // Skip normal (12 bytes)
        file.seekg(12, std::ios::cur);
        
        // Read 3 vertices
        for (int v = 0; v < 3; ++v) {
            float vertex[3];
            file.read(reinterpret_cast<char*>(vertex), sizeof(float) * 3);
            
            std::cout << "Triangle " << t << ", Vertex " << v << ": ("
                      << vertex[0] << ", " << vertex[1] << ", " << vertex[2] << ")" << std::endl;
            
            // Verify coordinates are in expected range
            EXPECT_GE(vertex[0], -0.20f) << "STL X coordinate too small";
            EXPECT_LE(vertex[0], 0.20f) << "STL X coordinate too large";
            EXPECT_GE(vertex[1], -0.04f) << "STL Y coordinate too small";
            EXPECT_LE(vertex[1], 0.36f) << "STL Y coordinate too large";
            EXPECT_GE(vertex[2], -0.20f) << "STL Z coordinate too small";
            EXPECT_LE(vertex[2], 0.20f) << "STL Z coordinate too large";
        }
        
        // Skip attribute byte count
        file.seekg(2, std::ios::cur);
    }
    
    file.close();
    
    // Clean up test file
    std::remove(testFile.c_str());
}

TEST_F(STLExportTest, CoordinateConversionChain) {
    // Test the coordinate conversion chain for a 32cm voxel
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    float voxelSize = Math::VoxelGridMath::getVoxelSizeMeters(resolution);
    
    // Voxel at increment origin (0,0,0)
    Math::IncrementCoordinates incrementOrigin(0, 0, 0);
    
    // Convert to world coordinates
    Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(incrementOrigin);
    
    std::cout << "\nCoordinate conversion for 32cm voxel at increment (0,0,0):" << std::endl;
    std::cout << "Voxel size: " << voxelSize << " meters" << std::endl;
    std::cout << "Increment: (" << incrementOrigin.x() << ", " << incrementOrigin.y() << ", " << incrementOrigin.z() << ")" << std::endl;
    std::cout << "World: (" << worldPos.x() << ", " << worldPos.y() << ", " << worldPos.z() << ")" << std::endl;
    
    // Increment coordinates are always in 1cm units
    // So increment (0,0,0) is world (0,0,0)
    EXPECT_NEAR(worldPos.x(), 0.0f, 0.001f);
    EXPECT_NEAR(worldPos.y(), 0.0f, 0.001f);
    EXPECT_NEAR(worldPos.z(), 0.0f, 0.001f);
    
    // Test corner positions
    Math::IncrementCoordinates corner(1, 1, 1);
    Math::WorldCoordinates cornerWorld = Math::CoordinateConverter::incrementToWorld(corner);
    
    std::cout << "Corner increment (1,1,1) -> World: (" 
              << cornerWorld.x() << ", " << cornerWorld.y() << ", " << cornerWorld.z() << ")" << std::endl;
    
    // Increment (1,1,1) should be at (0.01, 0.01, 0.01) meters
    EXPECT_NEAR(cornerWorld.x(), 0.01f, 0.001f);
    EXPECT_NEAR(cornerWorld.y(), 0.01f, 0.001f);
    EXPECT_NEAR(cornerWorld.z(), 0.01f, 0.001f);
}

TEST_F(STLExportTest, SimpleMesherSTLExport) {
    // Test STL export with SimpleMesher (smoothing level 0)
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    // Create a 2x2x2 block of voxels
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            for (int z = 0; z < 2; z++) {
                Math::IncrementCoordinates pos(x * 32, y * 32, z * 32);
                manager->setVoxel(pos, resolution, true);
            }
        }
    }
    
    // Get the grid
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr) << "Grid should exist";
    
    // Generate surface mesh with smoothing level 0 (uses SimpleMesher)
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    settings.smoothingLevel = 0; // Force SimpleMesher usage
    settings.generateNormals = false; // Speed up test
    
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    // Verify mesh is valid
    ASSERT_GT(mesh.vertices.size(), 0) << "Mesh should have vertices";
    ASSERT_GT(mesh.indices.size(), 0) << "Mesh should have indices";
    ASSERT_EQ(mesh.indices.size() % 3, 0) << "Indices should be multiple of 3";
    
    std::cout << "\nSimpleMesher generated mesh for 2x2x2 voxel block:" << std::endl;
    std::cout << "  Vertices: " << mesh.vertices.size() << std::endl;
    std::cout << "  Triangles: " << mesh.indices.size() / 3 << std::endl;
    
    // Convert to Rendering::Mesh for STL export
    Rendering::Mesh renderMesh;
    renderMesh.vertices.reserve(mesh.vertices.size());
    for (const auto& v : mesh.vertices) {
        renderMesh.vertices.push_back(v.value());
    }
    renderMesh.indices = mesh.indices;
    
    // Export to STL
    FileIO::STLExporter exporter;
    std::string testFile = "test_simple_mesher.stl";
    FileIO::STLExportOptions options = FileIO::STLExportOptions::Default();
    options.format = FileIO::STLFormat::Binary;
    
    bool exported = exporter.exportMesh(testFile, renderMesh, options);
    ASSERT_TRUE(exported) << "STL export should succeed";
    
    // Verify STL file exists and has correct size
    std::ifstream file(testFile, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(file.is_open()) << "STL file should exist";
    
    size_t fileSize = file.tellg();
    size_t expectedSize = 80 + 4 + (mesh.indices.size() / 3) * 50; // Header + count + triangles
    EXPECT_EQ(fileSize, expectedSize) << "STL file size mismatch";
    
    // Read and verify triangle count
    file.seekg(80);
    uint32_t triangleCount;
    file.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));
    EXPECT_EQ(triangleCount, mesh.indices.size() / 3) << "Triangle count mismatch";
    
    // Verify first few triangles have valid normals and vertices
    std::cout << "\nVerifying STL triangles:\n";
    for (uint32_t i = 0; i < std::min(triangleCount, 3u); ++i) {
        float normal[3];
        file.read(reinterpret_cast<char*>(normal), sizeof(float) * 3);
        
        // Normal should be unit length (approximately)
        float normalLength = std::sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
        EXPECT_NEAR(normalLength, 1.0f, 0.01f) << "Normal should be unit length for triangle " << i;
        
        std::cout << "Triangle " << i << " normal: (" << normal[0] << ", " << normal[1] << ", " << normal[2] << ")\n";
        
        // Read vertices
        for (int v = 0; v < 3; ++v) {
            float vertex[3];
            file.read(reinterpret_cast<char*>(vertex), sizeof(float) * 3);
            
            std::cout << "  Vertex " << v << ": (" << vertex[0] << ", " << vertex[1] << ", " << vertex[2] << ")\n";
            
            // Vertices should be within reasonable bounds for 2x2x2 block of 32cm voxels
            // The block spans from (0,0,0) to (64,64,64) in cm = (0,0,0) to (0.64,0.64,0.64) in meters
            // But STL export converts to millimeters by default, so multiply by 1000
            EXPECT_GE(vertex[0], -100.0f) << "X coordinate too small";
            EXPECT_LE(vertex[0], 700.0f) << "X coordinate too large";
            EXPECT_GE(vertex[1], -100.0f) << "Y coordinate too small";
            EXPECT_LE(vertex[1], 700.0f) << "Y coordinate too large";
            EXPECT_GE(vertex[2], -100.0f) << "Z coordinate too small";
            EXPECT_LE(vertex[2], 700.0f) << "Z coordinate too large";
        }
        
        // Skip attribute byte count
        file.seekg(2, std::ios::cur);
    }
    
    file.close();
    
    // Clean up test file
    std::remove(testFile.c_str());
    
    std::cout << "SimpleMesher STL export test passed!" << std::endl;
}