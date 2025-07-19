#include "VoxelGrid.h"
#include "../../foundation/logging/Logger.h"
#include <cmath>

namespace VoxelEditor {
namespace VoxelData {

bool VoxelGrid::isInsideVoxel(const Math::IncrementCoordinates& pos) const {
    // Get voxel size in cm
    float voxelSizeMeters = VoxelData::getVoxelSize(m_resolution);
    int voxelSizeCm = static_cast<int>(voxelSizeMeters * 100.0f);
    
    // For each voxel in the grid, check if the position is inside it
    auto allVoxels = getAllVoxels();
    for (const auto& voxel : allVoxels) {
        const Math::Vector3i& voxelPos = voxel.incrementPos.value();
        
        // Check if pos is inside this voxel
        // A voxel at position P with size S contains points from P to P+S (exclusive upper bound)
        if (pos.x() >= voxelPos.x && pos.x() < voxelPos.x + voxelSizeCm &&
            pos.y() >= voxelPos.y && pos.y() < voxelPos.y + voxelSizeCm &&
            pos.z() >= voxelPos.z && pos.z() < voxelPos.z + voxelSizeCm) {
            return true;
        }
    }
    
    return false;
}

}
}