#include "BoxSelector.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>

namespace VoxelEditor {
namespace Selection {

BoxSelector::BoxSelector(VoxelData::VoxelDataManager* voxelManager)
    : m_voxelManager(voxelManager)
    , m_selectionMode(SelectionMode::Replace)
    , m_includePartial(true) {
}

SelectionSet BoxSelector::selectFromScreen(const Math::Vector2i& screenStart, 
                                         const Math::Vector2i& screenEnd,
                                         const Math::Matrix4f& viewMatrix,
                                         const Math::Matrix4f& projMatrix,
                                         const Math::Vector2i& viewportSize,
                                         VoxelData::VoxelResolution resolution) {
    // Compute world space bounding box from screen coordinates
    Math::BoundingBox worldBox = computeScreenBox(screenStart, screenEnd, viewMatrix, projMatrix, viewportSize);
    
    // Select voxels within the box
    return selectFromWorld(worldBox, resolution, true);
}

SelectionSet BoxSelector::selectFromWorld(const Math::BoundingBox& worldBox,
                                        VoxelData::VoxelResolution resolution,
                                        bool checkExistence) {
    SelectionSet result;
    
    float voxelSize = VoxelId(Math::Vector3i::Zero(), resolution).getVoxelSize();
    
    // Calculate grid range
    Math::Vector3i minGrid(
        static_cast<int>(std::floor(worldBox.min.x / voxelSize)),
        static_cast<int>(std::floor(worldBox.min.y / voxelSize)),
        static_cast<int>(std::floor(worldBox.min.z / voxelSize))
    );
    
    Math::Vector3i maxGrid(
        static_cast<int>(std::ceil(worldBox.max.x / voxelSize)),
        static_cast<int>(std::ceil(worldBox.max.y / voxelSize)),
        static_cast<int>(std::ceil(worldBox.max.z / voxelSize))
    );
    
    // Select voxels in range
    for (int x = minGrid.x; x <= maxGrid.x; ++x) {
        for (int y = minGrid.y; y <= maxGrid.y; ++y) {
            for (int z = minGrid.z; z <= maxGrid.z; ++z) {
                VoxelId voxel(Math::Vector3i(x, y, z), resolution);
                
                if (isVoxelInBox(voxel, worldBox)) {
                    if (!checkExistence || voxelExists(voxel)) {
                        result.add(voxel);
                    }
                }
            }
        }
    }
    
    return result;
}

SelectionSet BoxSelector::selectFromRays(const Math::Ray& startRay,
                                       const Math::Ray& endRay,
                                       float maxDistance,
                                       VoxelData::VoxelResolution resolution) {
    // Find intersection points
    Math::Vector3f startPoint = startRay.origin + startRay.direction * maxDistance;
    Math::Vector3f endPoint = endRay.origin + endRay.direction * maxDistance;
    
    // TODO: Implement proper ray-scene intersection
    // For now, use the ray endpoints to create a box
    Math::BoundingBox worldBox(
        Math::Vector3f::min(startPoint, endPoint),
        Math::Vector3f::max(startPoint, endPoint)
    );
    
    return selectFromWorld(worldBox, resolution, true);
}

SelectionSet BoxSelector::selectFromGrid(const Math::Vector3i& minGrid,
                                       const Math::Vector3i& maxGrid,
                                       VoxelData::VoxelResolution resolution,
                                       bool checkExistence) {
    SelectionSet result;
    
    // Ensure proper ordering
    Math::Vector3i actualMin = Math::Vector3i::min(minGrid, maxGrid);
    Math::Vector3i actualMax = Math::Vector3i::max(minGrid, maxGrid);
    
    // Select all voxels in grid range
    for (int x = actualMin.x; x <= actualMax.x; ++x) {
        for (int y = actualMin.y; y <= actualMax.y; ++y) {
            for (int z = actualMin.z; z <= actualMax.z; ++z) {
                VoxelId voxel(Math::Vector3i(x, y, z), resolution);
                
                if (!checkExistence || voxelExists(voxel)) {
                    result.add(voxel);
                }
            }
        }
    }
    
    return result;
}

Math::BoundingBox BoxSelector::computeScreenBox(const Math::Vector2i& screenStart,
                                               const Math::Vector2i& screenEnd,
                                               const Math::Matrix4f& viewMatrix,
                                               const Math::Matrix4f& projMatrix,
                                               const Math::Vector2i& viewportSize) {
    // TODO: Implement screen to world space conversion
    // This requires unprojecting screen coordinates to world space
    // For now, return a default box
    Logging::Logger::getInstance().warning("BoxSelector::computeScreenBox: Not fully implemented");
    return Math::BoundingBox(
        Math::Vector3f(-1.0f, -1.0f, -1.0f),
        Math::Vector3f(1.0f, 1.0f, 1.0f)
    );
}

bool BoxSelector::isVoxelInBox(const VoxelId& voxel, const Math::BoundingBox& box) const {
    Math::BoundingBox voxelBounds = voxel.getBounds();
    
    if (m_includePartial) {
        // Include if voxel intersects box
        return box.intersects(voxelBounds);
    } else {
        // Include only if voxel is fully contained
        return box.contains(voxelBounds);
    }
}

bool BoxSelector::voxelExists(const VoxelId& voxel) const {
    if (!m_voxelManager) return true; // Assume exists if no manager
    
    // TODO: Implement actual voxel existence check
    return true;
}

}
}