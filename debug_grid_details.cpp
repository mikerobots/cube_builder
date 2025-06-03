#include <iostream>
#include "core/voxel_data/VoxelDataManager.h"

int main() {
    std::cout << "=== Detailed Grid Debug ===" << std::endl;
    
    // Create with event dispatcher like the test does
    VoxelEditor::Events::EventDispatcher eventDispatcher;
    VoxelEditor::VoxelData::VoxelDataManager manager(&eventDispatcher);
    
    VoxelEditor::Math::Vector3f worldPos(1.0f, 0.5f, 2.0f);
    VoxelEditor::VoxelData::VoxelResolution resolution = VoxelEditor::VoxelData::VoxelResolution::Size_4cm;
    
    // Check workspace size
    auto workspaceSize = manager.getWorkspaceSize();
    std::cout << "Workspace size: " << workspaceSize.x << "x" << workspaceSize.y << "x" << workspaceSize.z << std::endl;
    
    // Check voxel size for this resolution
    float voxelSize = VoxelEditor::VoxelData::getVoxelSize(resolution);
    std::cout << "Voxel size for Size_4cm: " << voxelSize << std::endl;
    
    // Calculate what the grid position would be
    VoxelEditor::Math::Vector3i gridPos(
        static_cast<int>(std::floor(worldPos.x / voxelSize)),
        static_cast<int>(std::floor(worldPos.y / voxelSize)),
        static_cast<int>(std::floor(worldPos.z / voxelSize))
    );
    std::cout << "Calculated grid pos: (" << gridPos.x << ", " << gridPos.y << ", " << gridPos.z << ")" << std::endl;
    
    // Check grid dimensions
    const auto* grid = manager.getGrid(resolution);
    if (grid) {
        auto gridDims = grid->getGridDimensions();
        std::cout << "Grid dimensions: " << gridDims.x << "x" << gridDims.y << "x" << gridDims.z << std::endl;
        
        // Check bounds
        bool validGrid = (gridPos.x >= 0 && gridPos.x < gridDims.x &&
                         gridPos.y >= 0 && gridPos.y < gridDims.y &&
                         gridPos.z >= 0 && gridPos.z < gridDims.z);
        std::cout << "Grid position valid: " << (validGrid ? "YES" : "NO") << std::endl;
        
        // Check world position validity
        bool validWorld = grid->isValidWorldPosition(worldPos);
        std::cout << "World position valid in grid: " << (validWorld ? "YES" : "NO") << std::endl;
    }
    
    // Try setting the voxel
    bool setResult = manager.setVoxelAtWorldPos(worldPos, resolution, true);
    std::cout << "setVoxelAtWorldPos result: " << (setResult ? "SUCCESS" : "FAILED") << std::endl;
    
    if (setResult) {
        bool getResult = manager.getVoxelAtWorldPos(worldPos, resolution);
        std::cout << "getVoxelAtWorldPos result: " << (getResult ? "SUCCESS" : "FAILED") << std::endl;
    }
    
    return 0;
}