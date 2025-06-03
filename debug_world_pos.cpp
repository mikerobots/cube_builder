#include <iostream>
#include "core/voxel_data/VoxelDataManager.h"

int main() {
    VoxelEditor::VoxelData::VoxelDataManager manager;
    
    VoxelEditor::Math::Vector3f worldPos(1.0f, 0.5f, 2.0f);
    VoxelEditor::VoxelData::VoxelResolution resolution = VoxelEditor::VoxelData::VoxelResolution::Size_4cm;
    
    std::cout << "=== Debug World Position Issue ===" << std::endl;
    
    // Check workspace size
    auto workspaceSize = manager.getWorkspaceSize();
    std::cout << "Workspace size: " << workspaceSize.x << "x" << workspaceSize.y << "x" << workspaceSize.z << std::endl;
    
    // Check if world position is valid
    bool validPos = manager.isValidWorldPosition(worldPos);
    std::cout << "World pos (" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ") valid: " << (validPos ? "YES" : "NO") << std::endl;
    
    // Check grid access
    const auto* grid = manager.getGrid(resolution);
    if (grid) {
        std::cout << "Grid exists for resolution: YES" << std::endl;
        // Check if the world position would be valid in the grid
        std::cout << "Grid workspace size: " << grid->getWorkspaceSize().x << "x" << grid->getWorkspaceSize().y << "x" << grid->getWorkspaceSize().z << std::endl;
        bool gridValid = grid->isValidWorldPosition(worldPos);
        std::cout << "World pos valid in grid: " << (gridValid ? "YES" : "NO") << std::endl;
    } else {
        std::cout << "Grid exists for resolution: NO" << std::endl;
    }
    
    // Try setting the voxel
    bool setResult = manager.setVoxelAtWorldPos(worldPos, resolution, true);
    std::cout << "setVoxelAtWorldPos result: " << (setResult ? "SUCCESS" : "FAILED") << std::endl;
    
    return 0;
}