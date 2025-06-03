#include <iostream>
#include "foundation/math/Vector3f.h"
#include "foundation/math/Vector3i.h"
#include "core/voxel_data/VoxelTypes.h"

int main() {
    std::cout << "=== Simple Debug Test ===" << std::endl;
    
    VoxelEditor::Math::Vector3f worldPos(1.0f, 0.5f, 2.0f);
    VoxelEditor::VoxelData::VoxelResolution resolution = VoxelEditor::VoxelData::VoxelResolution::Size_4cm;
    
    std::cout << "World position: (" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")" << std::endl;
    
    // Check voxel size calculation
    float voxelSize = VoxelEditor::VoxelData::getVoxelSize(resolution);
    std::cout << "Voxel size for Size_4cm: " << voxelSize << std::endl;
    
    // Calculate what the grid position would be
    VoxelEditor::Math::Vector3i gridPos(
        static_cast<int>(std::floor(worldPos.x / voxelSize)),
        static_cast<int>(std::floor(worldPos.y / voxelSize)),
        static_cast<int>(std::floor(worldPos.z / voxelSize))
    );
    std::cout << "Calculated grid pos: (" << gridPos.x << ", " << gridPos.y << ", " << gridPos.z << ")" << std::endl;
    
    // For a 5x5x5 workspace with 4cm voxels:
    // Grid dimensions should be 125x125x125 (5.0/0.04 = 125)
    float workspaceSize = 5.0f;
    int expectedGridDim = static_cast<int>(std::ceil(workspaceSize / voxelSize));
    std::cout << "Expected grid dimension: " << expectedGridDim << std::endl;
    
    // Check if our calculated position is valid
    bool validPosition = (gridPos.x >= 0 && gridPos.x < expectedGridDim &&
                         gridPos.y >= 0 && gridPos.y < expectedGridDim &&
                         gridPos.z >= 0 && gridPos.z < expectedGridDim);
    std::cout << "Position valid: " << (validPosition ? "YES" : "NO") << std::endl;
    
    return 0;
}