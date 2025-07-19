#include <gtest/gtest.h>
#include "../../core/voxel_data/VoxelGrid.h"
#include "../../core/voxel_data/VoxelDataManager.h"
#include "../../core/surface_gen/SurfaceGenerator.h"
#include "../../core/surface_gen/DualContouring.h"
#include "../../foundation/math/CoordinateConverter.h"
#include <iostream>
#include <set>

using namespace VoxelEditor;

class DualContouringDebugTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<VoxelData::VoxelDataManager>();
    }
    
    std::unique_ptr<VoxelData::VoxelDataManager> manager;
};

TEST_F(DualContouringDebugTest, DebugSingleVoxelGeneration) {
    // Place a single 32cm voxel at origin
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    Math::IncrementCoordinates pos(0, 0, 0);
    manager->setVoxel(pos, resolution, true);
    
    // Get the grid
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    std::cout << "\n=== Debug Single Voxel Generation ===\n";
    std::cout << "Voxel placed at increment coordinates (0,0,0)\n";
    std::cout << "Voxel size: 32cm (32 increment units)\n";
    std::cout << "Voxel occupies space from (0,0,0) to (32,32,32) in increment coordinates\n";
    
    // Test the isInsideVoxel function at various points
    std::cout << "\nTesting isInsideVoxel at various points:\n";
    
    // Points that should be inside the voxel
    std::vector<Math::IncrementCoordinates> insidePoints = {
        Math::IncrementCoordinates(0, 0, 0),    // Min corner
        Math::IncrementCoordinates(16, 16, 16), // Center
        Math::IncrementCoordinates(31, 31, 31), // Near max corner
        Math::IncrementCoordinates(1, 1, 1),    // Just inside
    };
    
    for (const auto& pt : insidePoints) {
        bool inside = grid->isInsideVoxel(pt);
        std::cout << "  Point (" << pt.x() << "," << pt.y() << "," << pt.z() 
                  << ") - Expected: inside, Actual: " << (inside ? "inside" : "OUTSIDE") << "\n";
        EXPECT_TRUE(inside) << "Point should be inside voxel";
    }
    
    // Points that should be outside the voxel
    std::vector<Math::IncrementCoordinates> outsidePoints = {
        Math::IncrementCoordinates(-1, 0, 0),   // Just outside X-
        Math::IncrementCoordinates(32, 0, 0),   // At max X (exclusive upper bound)
        Math::IncrementCoordinates(0, -1, 0),   // Just outside Y-
        Math::IncrementCoordinates(0, 32, 0),   // At max Y
        Math::IncrementCoordinates(0, 0, -1),   // Just outside Z-
        Math::IncrementCoordinates(0, 0, 32),   // At max Z
        Math::IncrementCoordinates(33, 16, 16), // Beyond max X
        Math::IncrementCoordinates(16, 33, 16), // Beyond max Y
        Math::IncrementCoordinates(16, 16, 33), // Beyond max Z
    };
    
    for (const auto& pt : outsidePoints) {
        bool inside = grid->isInsideVoxel(pt);
        std::cout << "  Point (" << pt.x() << "," << pt.y() << "," << pt.z() 
                  << ") - Expected: outside, Actual: " << (inside ? "INSIDE" : "outside") << "\n";
        EXPECT_FALSE(inside) << "Point should be outside voxel";
    }
    
    // Now test dual contouring directly
    std::cout << "\n=== Testing Dual Contouring Edge Detection ===\n";
    
    // Create a GridSampler to test edge detection
    class TestSampler {
    public:
        const VoxelData::VoxelGrid* grid;
        float isoValue = 0.5f;
        
        float sample(const Math::IncrementCoordinates& pos) const {
            if (!grid) return 0.0f;
            bool hasVoxel = grid->isInsideVoxel(pos);
            return hasVoxel ? 1.0f : 0.0f;
        }
        
        bool isInside(const Math::IncrementCoordinates& pos) const {
            return sample(pos) > isoValue;
        }
    };
    
    TestSampler sampler;
    sampler.grid = grid;
    
    // Test some specific edges that should cross the voxel boundary
    std::cout << "\nTesting edges that should cross voxel boundary:\n";
    
    // Edge from (-32,0,0) to (0,0,0) - crosses at X=0
    {
        Math::IncrementCoordinates v0(-32, 0, 0);
        Math::IncrementCoordinates v1(0, 0, 0);
        bool inside0 = sampler.isInside(v0);
        bool inside1 = sampler.isInside(v1);
        std::cout << "  Edge (" << v0.x() << "," << v0.y() << "," << v0.z() 
                  << ") to (" << v1.x() << "," << v1.y() << "," << v1.z() << "): "
                  << (inside0 ? "inside" : "outside") << " -> " << (inside1 ? "inside" : "outside")
                  << " - " << (inside0 != inside1 ? "CROSSES" : "no cross") << "\n";
    }
    
    // Edge from (16,16,16) to (48,16,16) - crosses at X=32
    {
        Math::IncrementCoordinates v0(16, 16, 16);
        Math::IncrementCoordinates v1(48, 16, 16);
        bool inside0 = sampler.isInside(v0);
        bool inside1 = sampler.isInside(v1);
        std::cout << "  Edge (" << v0.x() << "," << v0.y() << "," << v0.z() 
                  << ") to (" << v1.x() << "," << v1.y() << "," << v1.z() << "): "
                  << (inside0 ? "inside" : "outside") << " -> " << (inside1 ? "inside" : "outside")
                  << " - " << (inside0 != inside1 ? "CROSSES" : "no cross") << "\n";
    }
    
    // Generate mesh to see what happens
    std::cout << "\n=== Generating Mesh ===\n";
    SurfaceGen::SurfaceGenerator generator;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
    SurfaceGen::Mesh mesh = generator.generateSurface(*grid, settings);
    
    std::cout << "Generated mesh:\n";
    std::cout << "  Vertices: " << mesh.vertices.size() << "\n";
    std::cout << "  Triangles: " << (mesh.indices.size() / 3) << "\n";
    
    // Print all vertices
    std::cout << "\nAll vertices:\n";
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const auto& v = mesh.vertices[i];
        std::cout << "  Vertex " << i << ": (" << v.value().x << ", " << v.value().y << ", " << v.value().z << ")\n";
    }
    
    // Print triangles
    std::cout << "\nTriangles:\n";
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        std::cout << "  Triangle " << (i/3) << ": vertices " 
                  << mesh.indices[i] << ", " << mesh.indices[i+1] << ", " << mesh.indices[i+2] << "\n";
    }
}

TEST_F(DualContouringDebugTest, DebugCellProcessing) {
    // Place a single 32cm voxel at origin
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    Math::IncrementCoordinates pos(0, 0, 0);
    manager->setVoxel(pos, resolution, true);
    
    const VoxelData::VoxelGrid* grid = manager->getGrid(resolution);
    ASSERT_NE(grid, nullptr);
    
    std::cout << "\n=== Debug Cell Processing ===\n";
    
    // Get grid dimensions
    Math::Vector3i gridDims = grid->getGridDimensions();
    std::cout << "Grid dimensions (in cm): " << gridDims.x << "x" << gridDims.y << "x" << gridDims.z << "\n";
    
    // Calculate what cells the base DualContouring would process
    int voxelSizeCm = 32; // 32cm voxel
    Math::Vector3i dims(
        gridDims.x / voxelSizeCm,
        gridDims.y / voxelSizeCm,
        gridDims.z / voxelSizeCm
    );
    
    int minX = -(gridDims.x / 2);
    int minZ = -(gridDims.z / 2);
    int minY = 0;
    
    std::cout << "Cell iteration dimensions: " << dims.x << "x" << dims.y << "x" << dims.z << "\n";
    std::cout << "Min bounds: (" << minX << "," << minY << "," << minZ << ")\n";
    
    // Count how many cells would be processed
    int cellCount = 0;
    int cellsWithIntersections = 0;
    
    for (int z = 0; z < dims.z - 1; ++z) {
        for (int y = 0; y < dims.y - 1; ++y) {
            for (int x = 0; x < dims.x - 1; ++x) {
                cellCount++;
                
                // Cell position in increment coordinates
                Math::IncrementCoordinates cellPos(
                    minX + x * voxelSizeCm, 
                    minY + y * voxelSizeCm, 
                    minZ + z * voxelSizeCm
                );
                
                // Check if this cell would have any edge intersections
                // A cell at position C checks vertices from C to C+voxelSize
                bool hasIntersection = false;
                
                // Just check a few key edges
                // Bottom face corners
                Math::IncrementCoordinates v0 = cellPos;
                Math::IncrementCoordinates v1 = cellPos + Math::IncrementCoordinates(voxelSizeCm, 0, 0);
                Math::IncrementCoordinates v2 = cellPos + Math::IncrementCoordinates(0, voxelSizeCm, 0);
                Math::IncrementCoordinates v3 = cellPos + Math::IncrementCoordinates(0, 0, voxelSizeCm);
                
                // Check if any vertex is inside while another is outside
                bool i0 = grid->isInsideVoxel(v0);
                bool i1 = grid->isInsideVoxel(v1);
                bool i2 = grid->isInsideVoxel(v2);
                bool i3 = grid->isInsideVoxel(v3);
                
                if (i0 != i1 || i0 != i2 || i0 != i3) {
                    hasIntersection = true;
                    cellsWithIntersections++;
                    
                    if (cellsWithIntersections <= 10) {
                        std::cout << "Cell at (" << cellPos.x() << "," << cellPos.y() << "," << cellPos.z() 
                                  << ") has intersections - vertices inside: "
                                  << i0 << "," << i1 << "," << i2 << "," << i3 << "\n";
                    }
                }
            }
        }
    }
    
    std::cout << "\nTotal cells processed: " << cellCount << "\n";
    std::cout << "Cells with intersections: " << cellsWithIntersections << "\n";
}