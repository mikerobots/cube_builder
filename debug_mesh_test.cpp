#include <iostream>
#include "cli/VoxelMeshGenerator.h"
#include "voxel_data/VoxelDataManager.h"

int main() {
    // Create voxel manager
    auto voxelManager = std::make_unique<VoxelEditor::VoxelData::VoxelDataManager>();
    voxelManager->resizeWorkspace(VoxelEditor::Math::Vector3f(5.0f, 5.0f, 5.0f));
    
    // Create mesh generator
    auto meshGenerator = std::make_unique<VoxelEditor::VoxelMeshGenerator>();
    
    // Set resolution to 8cm
    voxelManager->setActiveResolution(VoxelEditor::VoxelData::VoxelResolution::Size_8cm);
    
    // Place single voxel at grid position (0,0,0)
    VoxelEditor::Math::Vector3i gridPos(0, 0, 0);
    voxelManager->setVoxel(gridPos, VoxelEditor::VoxelData::VoxelResolution::Size_8cm, true);
    
    // Generate mesh
    auto mesh = meshGenerator->generateCubeMesh(*voxelManager);
    
    std::cout << "Mesh has " << mesh.vertices.size() << " vertices and " << mesh.indices.size() << " indices\n";
    
    // Expected values
    VoxelEditor::Math::Vector3f expectedCenter(0.04f, 0.04f, 0.04f);
    float expectedSize = 0.08f * 0.95f; // 8cm with 0.95 scale factor
    
    std::cout << "Expected center: (" << expectedCenter.x << ", " << expectedCenter.y << ", " << expectedCenter.z << ")\n";
    std::cout << "Expected size: " << expectedSize << "\n";
    
    // Print first few vertex positions and colors
    for (size_t i = 0; i < std::min(size_t(8), mesh.vertices.size()); ++i) {
        const auto& v = mesh.vertices[i];
        std::cout << "Vertex " << i << ": pos(" << v.position.x << ", " << v.position.y << ", " << v.position.z << ") ";
        std::cout << "color(" << v.color.r << ", " << v.color.g << ", " << v.color.b << ", " << v.color.a << ")\n";
        
        // Check distance from expected center
        VoxelEditor::Math::Vector3f diff = v.position - expectedCenter;
        std::cout << "  Diff from center: (" << diff.x << ", " << diff.y << ", " << diff.z << ")\n";
        std::cout << "  Distance components: |" << std::abs(diff.x) << "|, |" << std::abs(diff.y) << "|, |" << std::abs(diff.z) << "|\n";
        std::cout << "  Expected half size: " << (expectedSize * 0.5f) << "\n";
    }
    
    return 0;
}