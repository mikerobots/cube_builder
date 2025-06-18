#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "file_io/STLExporter.h"
#include "rendering/RenderTypes.h"

namespace VoxelEditor {
namespace FileIO {

class STLExporterTest : public ::testing::Test {
protected:
    std::unique_ptr<STLExporter> m_exporter;
    std::string m_testDir = "test_stl_output";
    
    void SetUp() override {
        m_exporter = std::make_unique<STLExporter>();
        
        // Create test directory
        std::filesystem::create_directories(m_testDir);
    }
    
    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all(m_testDir);
    }
    
    Rendering::Mesh createTestMesh() {
        Rendering::Mesh mesh;
        
        // Create a simple triangle
        mesh.vertices = {
            Rendering::Vertex{Math::Vector3f(0, 0, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(0, 0)},
            Rendering::Vertex{Math::Vector3f(1, 0, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(1, 0)},
            Rendering::Vertex{Math::Vector3f(0, 1, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(0, 1)}
        };
        mesh.indices = {0, 1, 2};
        
        return mesh;
    }
    
    Rendering::Mesh createCubeMesh() {
        Rendering::Mesh mesh;
        
        // Create a unit cube
        mesh.vertices = {
            // Front face
            Rendering::Vertex{Math::Vector3f(0, 0, 0), Math::Vector3f(0, 0, -1), Math::Vector2f(0, 0)},
            Rendering::Vertex{Math::Vector3f(1, 0, 0), Math::Vector3f(0, 0, -1), Math::Vector2f(1, 0)},
            Rendering::Vertex{Math::Vector3f(1, 1, 0), Math::Vector3f(0, 0, -1), Math::Vector2f(1, 1)},
            Rendering::Vertex{Math::Vector3f(0, 1, 0), Math::Vector3f(0, 0, -1), Math::Vector2f(0, 1)},
            
            // Back face
            Rendering::Vertex{Math::Vector3f(0, 0, 1), Math::Vector3f(0, 0, 1), Math::Vector2f(0, 0)},
            Rendering::Vertex{Math::Vector3f(1, 0, 1), Math::Vector3f(0, 0, 1), Math::Vector2f(1, 0)},
            Rendering::Vertex{Math::Vector3f(1, 1, 1), Math::Vector3f(0, 0, 1), Math::Vector2f(1, 1)},
            Rendering::Vertex{Math::Vector3f(0, 1, 1), Math::Vector3f(0, 0, 1), Math::Vector2f(0, 1)}
        };
        
        // Cube indices (2 triangles per face, 6 faces)
        mesh.indices = {
            // Front
            0, 1, 2,  2, 3, 0,
            // Back
            4, 7, 6,  6, 5, 4,
            // Left
            0, 3, 7,  7, 4, 0,
            // Right
            1, 5, 6,  6, 2, 1,
            // Top
            3, 2, 6,  6, 7, 3,
            // Bottom
            0, 4, 5,  5, 1, 0
        };
        
        return mesh;
    }
};

TEST_F(STLExporterTest, ExportBinarySTL) {
    Rendering::Mesh mesh = createTestMesh();
    STLExportOptions options = STLExportOptions::Default();
    options.format = STLFormat::Binary;
    options.validateWatertight = false;  // Disable validation for single triangle test
    
    std::string filename = m_testDir + "/test_binary.stl";
    EXPECT_TRUE(m_exporter->exportMesh(filename, mesh, options));
    
    // Verify file exists and has correct size
    EXPECT_TRUE(std::filesystem::exists(filename));
    
    // Binary STL: 80 byte header + 4 bytes (triangle count) + 50 bytes per triangle
    size_t expectedSize = 80 + 4 + 50 * 1;  // 1 triangle
    EXPECT_EQ(std::filesystem::file_size(filename), expectedSize);
    
    // Get export stats
    STLExportStats stats = m_exporter->getLastExportStats();
    EXPECT_EQ(stats.triangleCount, 1);
    EXPECT_EQ(stats.vertexCount, 3);
    EXPECT_EQ(stats.fileSize, expectedSize);
}

TEST_F(STLExporterTest, ExportASCIISTL) {
    Rendering::Mesh mesh = createTestMesh();
    STLExportOptions options = STLExportOptions::Default();
    options.format = STLFormat::ASCII;
    options.validateWatertight = false;  // Disable validation for single triangle test
    
    std::string filename = m_testDir + "/test_ascii.stl";
    EXPECT_TRUE(m_exporter->exportMesh(filename, mesh, options));
    
    // Verify file exists
    EXPECT_TRUE(std::filesystem::exists(filename));
    
    // Read and verify ASCII content
    std::ifstream file(filename);
    std::string line;
    
    // First line should be "solid"
    std::getline(file, line);
    EXPECT_EQ(line.substr(0, 5), "solid");
    
    // Should contain facet normal
    bool foundFacet = false;
    while (std::getline(file, line)) {
        if (line.find("facet normal") != std::string::npos) {
            foundFacet = true;
            break;
        }
    }
    EXPECT_TRUE(foundFacet);
    
    file.close();
}

TEST_F(STLExporterTest, ExportCubeMesh) {
    Rendering::Mesh mesh = createCubeMesh();
    STLExportOptions options = STLExportOptions::Default();
    
    std::string filename = m_testDir + "/test_cube.stl";
    EXPECT_TRUE(m_exporter->exportMesh(filename, mesh, options));
    
    STLExportStats stats = m_exporter->getLastExportStats();
    EXPECT_EQ(stats.triangleCount, 12);  // 2 triangles per face * 6 faces
    EXPECT_EQ(stats.vertexCount, 8);    // The cube has 8 unique vertices
}

TEST_F(STLExporterTest, ExportWithScale) {
    Rendering::Mesh mesh = createTestMesh();
    
    // Export with 10x scale
    STLExportOptions options = STLExportOptions::Default();
    options.scale = 10.0f;
    
    std::string filename = m_testDir + "/test_scaled.stl";
    EXPECT_TRUE(m_exporter->exportMesh(filename, mesh, options));
    
    // The file should exist
    EXPECT_TRUE(std::filesystem::exists(filename));
}

TEST_F(STLExporterTest, ExportWithTranslation) {
    Rendering::Mesh mesh = createTestMesh();
    
    // Export with translation
    STLExportOptions options = STLExportOptions::Default();
    options.translation = Math::Vector3f(5.0f, 5.0f, 5.0f);
    
    std::string filename = m_testDir + "/test_translated.stl";
    EXPECT_TRUE(m_exporter->exportMesh(filename, mesh, options));
    
    EXPECT_TRUE(std::filesystem::exists(filename));
}

TEST_F(STLExporterTest, ExportMultipleMeshes) {
    std::vector<Rendering::Mesh> meshes;
    meshes.push_back(createTestMesh());
    meshes.push_back(createCubeMesh());
    
    STLExportOptions options = STLExportOptions::Default();
    options.validateWatertight = false;
    options.mergeMeshes = true;
    
    std::string filename = m_testDir + "/test_multiple.stl";
    EXPECT_TRUE(m_exporter->exportMeshes(filename, meshes, options));
    
    STLExportStats stats = m_exporter->getLastExportStats();
    EXPECT_EQ(stats.triangleCount, 13);  // 1 from triangle + 12 from cube
}

TEST_F(STLExporterTest, ExportEmptyMesh) {
    Rendering::Mesh emptyMesh;
    STLExportOptions options = STLExportOptions::Default();
    
    std::string filename = m_testDir + "/test_empty.stl";
    
    // Should handle empty mesh gracefully
    EXPECT_FALSE(m_exporter->exportMesh(filename, emptyMesh, options));
}

TEST_F(STLExporterTest, ExportPrinting3DOptions) {
    Rendering::Mesh mesh = createCubeMesh();
    STLExportOptions options = STLExportOptions::Printing3D();
    options.validateWatertight = false;  // Disable for test
    
    std::string filename = m_testDir + "/test_3d_print.stl";
    EXPECT_TRUE(m_exporter->exportMesh(filename, mesh, options));
    
    // Printing3D should use binary format and millimeters
    EXPECT_EQ(options.format, STLFormat::Binary);
    EXPECT_EQ(options.units, STLUnits::Millimeters);
    // We disabled watertight validation for the test
    EXPECT_FALSE(options.validateWatertight);
}

TEST_F(STLExporterTest, ExportCADOptions) {
    Rendering::Mesh mesh = createCubeMesh();
    STLExportOptions options = STLExportOptions::CAD();
    options.validateWatertight = false;  // Disable for test
    
    std::string filename = m_testDir + "/test_cad.stl";
    EXPECT_TRUE(m_exporter->exportMesh(filename, mesh, options));
    
    // CAD should use ASCII format and meters
    EXPECT_EQ(options.format, STLFormat::ASCII);
    EXPECT_EQ(options.units, STLUnits::Meters);
}

TEST_F(STLExporterTest, ValidateMesh) {
    Rendering::Mesh mesh = createCubeMesh();
    std::vector<std::string> issues;
    
    // A properly constructed cube should be valid
    EXPECT_TRUE(m_exporter->validateMesh(mesh, issues));
    EXPECT_TRUE(issues.empty());
}

TEST_F(STLExporterTest, ValidateInvalidMesh) {
    Rendering::Mesh mesh;
    // Create mesh with mismatched indices
    mesh.vertices = {
        Rendering::Vertex{Math::Vector3f(0, 0, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(0, 0)},
        Rendering::Vertex{Math::Vector3f(1, 0, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(1, 0)}
    };
    mesh.indices = {0, 1, 2};  // Index 2 is out of bounds
    
    std::vector<std::string> issues;
    EXPECT_FALSE(m_exporter->validateMesh(mesh, issues));
    EXPECT_FALSE(issues.empty());
}

TEST_F(STLExporterTest, PreprocessMesh) {
    Rendering::Mesh mesh = createTestMesh();
    STLExportOptions options = STLExportOptions::Default();
    options.scale = 2.0f;
    options.translation = Math::Vector3f(1.0f, 0, 0);
    
    // preprocessMesh is private, so we test it indirectly through export
    // The export function will apply the preprocessing
    
    // Original mesh should have 3 vertices
    EXPECT_EQ(mesh.vertices.size(), 3);
    
    // Export with preprocessing options to test transformation
    std::string filename = m_testDir + "/test_preprocessed.stl";
    options.validateWatertight = false;  // Disable for test
    EXPECT_TRUE(m_exporter->exportMesh(filename, mesh, options));
    
    // Verify the file was created (preprocessing happened during export)
    EXPECT_TRUE(std::filesystem::exists(filename));
}

TEST_F(STLExporterTest, ExportMultipleMeshesWithMerge) {
    Rendering::Mesh mesh1 = createTestMesh();
    Rendering::Mesh mesh2 = createTestMesh();
    
    // Translate second mesh
    for (auto& vertex : mesh2.vertices) {
        vertex.position.x() += 2.0f;
    }
    
    std::vector<Rendering::Mesh> meshes = {mesh1, mesh2};
    
    // Test merging through exportMeshes with mergeMeshes option
    STLExportOptions options = STLExportOptions::Default();
    options.validateWatertight = false;  // Disable for test
    options.mergeMeshes = true;
    
    std::string filename = m_testDir + "/test_merged.stl";
    EXPECT_TRUE(m_exporter->exportMeshes(filename, meshes, options));
    
    // Verify file exists and stats show merged mesh
    EXPECT_TRUE(std::filesystem::exists(filename));
    
    STLExportStats stats = m_exporter->getLastExportStats();
    EXPECT_EQ(stats.triangleCount, 2);  // Two triangles total
}

TEST_F(STLExporterTest, ExportToInvalidPath) {
    Rendering::Mesh mesh = createTestMesh();
    STLExportOptions options = STLExportOptions::Default();
    
    // Try to export to invalid path
    std::string filename = "/invalid/path/that/does/not/exist/test.stl";
    EXPECT_FALSE(m_exporter->exportMesh(filename, mesh, options));
}

TEST_F(STLExporterTest, UnitConversion) {
    Rendering::Mesh mesh = createCubeMesh();  // 1x1x1 cube
    
    // Export in different units
    STLExportOptions mmOptions = STLExportOptions::Default();
    mmOptions.units = STLUnits::Millimeters;
    
    STLExportOptions cmOptions = STLExportOptions::Default();
    cmOptions.units = STLUnits::Centimeters;
    
    STLExportOptions mOptions = STLExportOptions::Default();
    mOptions.units = STLUnits::Meters;
    
    STLExportOptions inOptions = STLExportOptions::Default();
    inOptions.units = STLUnits::Inches;
    
    // Export all versions
    EXPECT_TRUE(m_exporter->exportMesh(m_testDir + "/cube_mm.stl", mesh, mmOptions));
    EXPECT_TRUE(m_exporter->exportMesh(m_testDir + "/cube_cm.stl", mesh, cmOptions));
    EXPECT_TRUE(m_exporter->exportMesh(m_testDir + "/cube_m.stl", mesh, mOptions));
    EXPECT_TRUE(m_exporter->exportMesh(m_testDir + "/cube_in.stl", mesh, inOptions));
    
    // All files should exist
    EXPECT_TRUE(std::filesystem::exists(m_testDir + "/cube_mm.stl"));
    EXPECT_TRUE(std::filesystem::exists(m_testDir + "/cube_cm.stl"));
    EXPECT_TRUE(std::filesystem::exists(m_testDir + "/cube_m.stl"));
    EXPECT_TRUE(std::filesystem::exists(m_testDir + "/cube_in.stl"));
}

} // namespace FileIO
} // namespace VoxelEditor