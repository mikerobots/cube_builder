#include "foundation/math/CoordinateConverter.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Vector3i.h"
#include "core/voxel_data/VoxelTypes.h"
#include <iostream>
#include <iomanip>

using namespace VoxelEditor;
using namespace VoxelEditor::Math;

int main() {
    std::cout << std::fixed << std::setprecision(4);
    
    // Test coordinate conversion for 1cm voxels (collision test)
    Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    std::cout << "=== Testing CollisionSimple coordinate mapping ===" << std::endl;
    std::cout << "Workspace size: " << workspaceSize.x << ", " << workspaceSize.y << ", " << workspaceSize.z << std::endl;
    std::cout << "Voxel size: " << VoxelData::getVoxelSize(resolution) << " meters" << std::endl;
    
    // Test the positions from the failing test
    WorldCoordinates pos1(Vector3f(0.1f, 0.0f, 0.1f));
    WorldCoordinates pos2(Vector3f(0.11f, 0.0f, 0.1f));
    
    GridCoordinates grid1 = CoordinateConverter::worldToGrid(pos1, resolution, workspaceSize);
    GridCoordinates grid2 = CoordinateConverter::worldToGrid(pos2, resolution, workspaceSize);
    
    std::cout << "World (0.1, 0, 0.1) -> Grid (" << grid1.x() << ", " << grid1.y() << ", " << grid1.z() << ")" << std::endl;
    std::cout << "World (0.11, 0, 0.1) -> Grid (" << grid2.x() << ", " << grid2.y() << ", " << grid2.z() << ")" << std::endl;
    
    // Check if they map to the same grid cell
    if (grid1.value() == grid2.value()) {
        std::cout << "ERROR: Both positions map to the same grid cell!" << std::endl;
    } else {
        std::cout << "OK: Positions map to different grid cells" << std::endl;
    }
    
    // Test what grid positions these would be in raw division (old system)
    float voxelSize = VoxelData::getVoxelSize(resolution);
    Vector3i rawGrid1(
        static_cast<int>(std::floor(0.1f / voxelSize)),
        static_cast<int>(std::floor(0.0f / voxelSize)),
        static_cast<int>(std::floor(0.1f / voxelSize))
    );
    Vector3i rawGrid2(
        static_cast<int>(std::floor(0.11f / voxelSize)),
        static_cast<int>(std::floor(0.0f / voxelSize)),
        static_cast<int>(std::floor(0.1f / voxelSize))
    );
    
    std::cout << "Raw division (0.1, 0, 0.1) -> Grid (" << rawGrid1.x << ", " << rawGrid1.y << ", " << rawGrid1.z << ")" << std::endl;
    std::cout << "Raw division (0.11, 0, 0.1) -> Grid (" << rawGrid2.x << ", " << rawGrid2.y << ", " << rawGrid2.z << ")" << std::endl;
    
    return 0;
}