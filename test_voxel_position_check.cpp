#include <iostream>
#include <iomanip>
#include <gtest/gtest.h>
#include "core/voxel_data/VoxelDataManager.h"
#include "core/surface_gen/SurfaceGenerator.h"
#include "foundation/math/CoordinateConverter.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::SurfaceGen;

// Helper to find distinct voxel regions in a mesh based on vertex clustering
std::vector<Vector3f> findVoxelCenters(const Mesh& mesh) {
    std::vector<Vector3f> centers;
    
    // Group vertices by proximity (within voxel size)
    std::vector<std::vector<Vector3f>> clusters;
    
    for (const auto& vertex : mesh.vertices) {
        Vector3f v(vertex.x(), vertex.y(), vertex.z());
        
        // Find which cluster this vertex belongs to
        bool foundCluster = false;
        for (auto& cluster : clusters) {
            if (!cluster.empty()) {
                // Check distance to cluster center
                Vector3f clusterSum(0, 0, 0);
                for (const auto& cv : cluster) {
                    clusterSum.x += cv.x;
                    clusterSum.y += cv.y;
                    clusterSum.z += cv.z;
                }
                Vector3f clusterCenter = clusterSum * (1.0f / cluster.size());
                
                float dx = v.x - clusterCenter.x;
                float dy = v.y - clusterCenter.y;
                float dz = v.z - clusterCenter.z;
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                
                // If within 1 meter (should capture any voxel up to 512cm)
                if (dist < 1.0f) {
                    cluster.push_back(v);
                    foundCluster = true;
                    break;
                }
            }
        }
        
        if (!foundCluster) {
            clusters.push_back({v});
        }
    }
    
    // Calculate center of each cluster
    for (const auto& cluster : clusters) {
        Vector3f sum(0, 0, 0);
        Vector3f min = cluster[0];
        Vector3f max = cluster[0];
        
        for (const auto& v : cluster) {
            sum.x += v.x;
            sum.y += v.y;
            sum.z += v.z;
            
            min.x = std::min(min.x, v.x);
            min.y = std::min(min.y, v.y);
            min.z = std::min(min.z, v.z);
            max.x = std::max(max.x, v.x);
            max.y = std::max(max.y, v.y);
            max.z = std::max(max.z, v.z);
        }
        
        Vector3f center = (min + max) * 0.5f;
        centers.push_back(center);
    }
    
    return centers;
}

TEST(VoxelPositionCheck, ExactScenarioPositions) {
    std::cout << std::fixed << std::setprecision(3);
    
    // Enable debug logging
    VoxelEditor::Logging::Logger::getInstance().setLevel(VoxelEditor::Logging::LogLevel::Debug);
    
    // Create voxel manager
    VoxelDataManager voxelManager;
    voxelManager.resizeWorkspace(Vector3f(5.0f, 5.0f, 5.0f));
    
    // Place 64cm voxel at (0, 0, 64)
    IncrementCoordinates pos64(0, 0, 64);
    ASSERT_TRUE(voxelManager.setVoxel(pos64, VoxelResolution::Size_64cm, true));
    std::cout << "Placed 64cm voxel at increment (" << pos64.x() << ", " << pos64.y() << ", " << pos64.z() << ")" << std::endl;
    
    // Convert to world coordinates to know expected center
    WorldCoordinates world64 = CoordinateConverter::incrementToWorld(pos64);
    Vector3f expected64Center(
        world64.x() + 0.32f,  // center is at position + half size
        world64.y() + 0.32f,  // 64cm = 0.64m, half = 0.32m
        world64.z() + 0.32f
    );
    std::cout << "Expected 64cm voxel center: (" << expected64Center.x << ", " << expected64Center.y << ", " << expected64Center.z << ") meters" << std::endl;
    
    // Place 16cm voxel at (24, 64, 88) - the exact user scenario
    IncrementCoordinates pos16(24, 64, 88);
    ASSERT_TRUE(voxelManager.setVoxel(pos16, VoxelResolution::Size_16cm, true));
    std::cout << "\nPlaced 16cm voxel at increment (" << pos16.x() << ", " << pos16.y() << ", " << pos16.z() << ")" << std::endl;
    
    // Convert to world coordinates to know expected center
    WorldCoordinates world16 = CoordinateConverter::incrementToWorld(pos16);
    Vector3f expected16Center(
        world16.x() + 0.08f,  // center is at position + half size
        world16.y() + 0.08f,  // 16cm = 0.16m, half = 0.08m
        world16.z() + 0.08f
    );
    std::cout << "Expected 16cm voxel center: (" << expected16Center.x << ", " << expected16Center.y << ", " << expected16Center.z << ") meters" << std::endl;
    
    // Generate multi-resolution mesh
    SurfaceGenerator surfaceGen;
    Mesh multiMesh = surfaceGen.generateMultiResMesh(voxelManager, VoxelResolution::Size_16cm);
    
    // Find voxel centers in the mesh
    std::vector<Vector3f> meshCenters = findVoxelCenters(multiMesh);
    
    std::cout << "\n=== Mesh Analysis ===" << std::endl;
    std::cout << "Found " << meshCenters.size() << " distinct voxel regions in mesh" << std::endl;
    
    ASSERT_EQ(meshCenters.size(), 2) << "Should find exactly 2 voxels in the mesh";
    
    // Sort centers by Y coordinate to identify which is which
    std::sort(meshCenters.begin(), meshCenters.end(), 
              [](const Vector3f& a, const Vector3f& b) { return a.y < b.y; });
    
    // Lower voxel should be the 64cm voxel
    Vector3f& found64Center = meshCenters[0];
    std::cout << "\nLower voxel center: (" << found64Center.x << ", " << found64Center.y << ", " << found64Center.z << ") meters" << std::endl;
    std::cout << "Expected 64cm center: (" << expected64Center.x << ", " << expected64Center.y << ", " << expected64Center.z << ") meters" << std::endl;
    
    // Upper voxel should be the 16cm voxel
    Vector3f& found16Center = meshCenters[1];
    std::cout << "\nUpper voxel center: (" << found16Center.x << ", " << found16Center.y << ", " << found16Center.z << ") meters" << std::endl;
    std::cout << "Expected 16cm center: (" << expected16Center.x << ", " << expected16Center.y << ", " << expected16Center.z << ") meters" << std::endl;
    
    // Check if positions are correct
    float tolerance = 0.01f; // 1cm tolerance
    
    // Check 64cm voxel position
    EXPECT_NEAR(found64Center.x, expected64Center.x, tolerance) << "64cm voxel X position incorrect";
    EXPECT_NEAR(found64Center.y, expected64Center.y, tolerance) << "64cm voxel Y position incorrect";
    EXPECT_NEAR(found64Center.z, expected64Center.z, tolerance) << "64cm voxel Z position incorrect";
    
    // Check 16cm voxel position
    EXPECT_NEAR(found16Center.x, expected16Center.x, tolerance) << "16cm voxel X position incorrect";
    EXPECT_NEAR(found16Center.y, expected16Center.y, tolerance) << "16cm voxel Y position incorrect";
    EXPECT_NEAR(found16Center.z, expected16Center.z, tolerance) << "16cm voxel Z position incorrect";
    
    // The bug would manifest as the 16cm voxel being at the center of the 64cm voxel
    // So let's explicitly check that it's NOT there
    float distToCenterOf64 = std::sqrt(
        std::pow(found16Center.x - expected64Center.x, 2) +
        std::pow(found16Center.y - expected64Center.y, 2) +
        std::pow(found16Center.z - expected64Center.z, 2)
    );
    
    std::cout << "\nDistance from 16cm voxel to center of 64cm voxel: " << distToCenterOf64 << " meters" << std::endl;
    EXPECT_GT(distToCenterOf64, 0.1f) << "16cm voxel should NOT be at the center of 64cm voxel";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}