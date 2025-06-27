#include "core/surface_gen/DualContouring.h"
#include "core/voxel_data/VoxelGrid.h"
#include "foundation/logging/Logger.h"
#include <iostream>

using namespace VoxelEditor;

int main() {
    // Set up debug logging
    auto& logger = Logging::Logger::getInstance();
    logger.setLevel(Logging::LogLevel::DEBUG);
    
    // Create small test grid
    Math::Vector3f workspaceSize(2.0f, 2.0f, 2.0f);  // 2m³ workspace
    VoxelData::VoxelGrid grid(VoxelData::VoxelResolution::Size_32cm, workspaceSize);
    
    std::cout << "Grid dimensions: " << grid.getGridDimensions().x << "x" 
              << grid.getGridDimensions().y << "x" << grid.getGridDimensions().z << std::endl;
    
    // Place a single voxel at center-ish position
    Math::IncrementCoordinates voxelPos(4 * 32, 4 * 32, 4 * 32);  // 128cm = 1.28m from center
    std::cout << "Placing voxel at increment position: (" << voxelPos.x() << ", " 
              << voxelPos.y() << ", " << voxelPos.z() << ")" << std::endl;
    
    bool success = grid.setVoxel(voxelPos, true);
    std::cout << "Voxel placement " << (success ? "successful" : "failed") << std::endl;
    
    // Check if voxel exists
    bool exists = grid.getVoxel(voxelPos);
    std::cout << "Voxel exists: " << (exists ? "yes" : "no") << std::endl;
    
    // Get all voxels to verify
    auto allVoxels = grid.getAllVoxels();
    std::cout << "Total voxels in grid: " << allVoxels.size() << std::endl;
    
    if (!allVoxels.empty()) {
        auto& firstVoxel = allVoxels[0];
        std::cout << "First voxel at: (" << firstVoxel.incrementPos.x() << ", " 
                  << firstVoxel.incrementPos.y() << ", " << firstVoxel.incrementPos.z() << ")" << std::endl;
        std::cout << "Voxel resolution: " << static_cast<int>(firstVoxel.resolution) << std::endl;
    }
    
    // Try dual contouring
    SurfaceGen::DualContouring dc;
    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Preview();
    
    std::cout << "\nGenerating mesh..." << std::endl;
    auto mesh = dc.generateMesh(grid, settings);
    
    std::cout << "Mesh vertices: " << mesh.vertices.size() << std::endl;
    std::cout << "Mesh indices: " << mesh.indices.size() << std::endl;
    std::cout << "Mesh valid: " << (mesh.isValid() ? "yes" : "no") << std::endl;
    
    return 0;
}