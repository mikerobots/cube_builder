#include "BoxSelector.h"
#include "../../foundation/math/Matrix4f.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <limits>

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
    
    // Get workspace size for coordinate conversion
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f); // Default workspace size
    if (m_voxelManager) {
        workspaceSize = m_voxelManager->getWorkspaceSize();
    }
    
    // Convert world box corners to increment coordinates using the coordinate converter
    Math::WorldCoordinates minWorld(worldBox.min);
    Math::WorldCoordinates maxWorld(worldBox.max);
    
    Math::IncrementCoordinates minIncrementCoord = Math::CoordinateConverter::worldToIncrement(minWorld);
    Math::IncrementCoordinates maxIncrementCoord = Math::CoordinateConverter::worldToIncrement(maxWorld);
    
    Math::Vector3i minGrid = minIncrementCoord.value();
    Math::Vector3i maxGrid = maxIncrementCoord.value();
    
    // Ensure proper ordering (min <= max)
    Math::Vector3i actualMin = Math::Vector3i::min(minGrid, maxGrid);
    Math::Vector3i actualMax = Math::Vector3i::max(minGrid, maxGrid);
    
    // Get voxel size in centimeters for the given resolution
    float voxelSizeMeters = VoxelData::getVoxelSize(resolution);
    int voxelSizeCm = static_cast<int>(voxelSizeMeters * 100.0f);
    
    // Snap min/max to voxel boundaries
    Math::IncrementCoordinates snappedMin = Math::CoordinateConverter::snapToVoxelResolution(
        Math::IncrementCoordinates(actualMin), resolution);
    Math::IncrementCoordinates snappedMax = Math::CoordinateConverter::snapToVoxelResolution(
        Math::IncrementCoordinates(actualMax), resolution);
    
    // Select voxels in range, stepping by voxel size
    for (int x = snappedMin.x(); x <= snappedMax.x(); x += voxelSizeCm) {
        for (int y = snappedMin.y(); y <= snappedMax.y(); y += voxelSizeCm) {
            for (int z = snappedMin.z(); z <= snappedMax.z(); z += voxelSizeCm) {
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
    // Get workspace bounds from voxel manager with centered coordinate system
    Math::BoundingBox workspaceBounds;
    if (m_voxelManager) {
        Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceSize();
        // Use centered coordinate system: X,Z centered [-size/2, size/2], Y from [0, size]
        workspaceBounds = Math::BoundingBox(
            Math::Vector3f(-workspaceSize.x/2, 0.0f, -workspaceSize.z/2),
            Math::Vector3f(workspaceSize.x/2, workspaceSize.y, workspaceSize.z/2)
        );
    } else {
        // Default workspace bounds if no manager (5m workspace)
        workspaceBounds = Math::BoundingBox(
            Math::Vector3f(-2.5f, 0.0f, -2.5f),
            Math::Vector3f(2.5f, 5.0f, 2.5f)
        );
    }
    
    // Find intersection points with workspace bounds
    float t1, t2;
    Math::Vector3f startPoint, endPoint;
    
    // Intersect start ray with workspace
    if (workspaceBounds.intersectRay(startRay, t1, t2)) {
        // Use the entry point or maxDistance, whichever is closer
        float t = std::min(t1, maxDistance);
        startPoint = startRay.origin + startRay.direction * t;
    } else {
        // No intersection, use max distance
        startPoint = startRay.origin + startRay.direction * maxDistance;
    }
    
    // Intersect end ray with workspace
    if (workspaceBounds.intersectRay(endRay, t1, t2)) {
        // Use the entry point or maxDistance, whichever is closer
        float t = std::min(t1, maxDistance);
        endPoint = endRay.origin + endRay.direction * t;
    } else {
        // No intersection, use max distance
        endPoint = endRay.origin + endRay.direction * maxDistance;
    }
    
    // Create selection box from the two points
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
    // Compute the inverse of the combined view-projection matrix
    Math::Matrix4f viewProjMatrix = projMatrix * viewMatrix;
    Math::Matrix4f invViewProjMatrix = viewProjMatrix.inverse();
    
    // Convert screen coordinates to normalized device coordinates (NDC)
    float x1 = (2.0f * screenStart.x / viewportSize.x) - 1.0f;
    float y1 = 1.0f - (2.0f * screenStart.y / viewportSize.y); // Y is inverted
    float x2 = (2.0f * screenEnd.x / viewportSize.x) - 1.0f;
    float y2 = 1.0f - (2.0f * screenEnd.y / viewportSize.y);
    
    // Create corner points in NDC space at near and far planes
    Math::Vector4f corners[8] = {
        Math::Vector4f(x1, y1, -1.0f, 1.0f), // Near plane corners
        Math::Vector4f(x2, y1, -1.0f, 1.0f),
        Math::Vector4f(x1, y2, -1.0f, 1.0f),
        Math::Vector4f(x2, y2, -1.0f, 1.0f),
        Math::Vector4f(x1, y1,  1.0f, 1.0f), // Far plane corners
        Math::Vector4f(x2, y1,  1.0f, 1.0f),
        Math::Vector4f(x1, y2,  1.0f, 1.0f),
        Math::Vector4f(x2, y2,  1.0f, 1.0f)
    };
    
    // Transform corners to world space
    Math::Vector3f minPoint(std::numeric_limits<float>::max());
    Math::Vector3f maxPoint(std::numeric_limits<float>::lowest());
    
    for (int i = 0; i < 8; ++i) {
        Math::Vector4f worldPoint = invViewProjMatrix * corners[i];
        
        // Perspective divide
        if (std::abs(worldPoint.w) > std::numeric_limits<float>::epsilon()) {
            worldPoint.x /= worldPoint.w;
            worldPoint.y /= worldPoint.w;
            worldPoint.z /= worldPoint.w;
        }
        
        // Update bounding box
        minPoint.x = std::min(minPoint.x, worldPoint.x);
        minPoint.y = std::min(minPoint.y, worldPoint.y);
        minPoint.z = std::min(minPoint.z, worldPoint.z);
        maxPoint.x = std::max(maxPoint.x, worldPoint.x);
        maxPoint.y = std::max(maxPoint.y, worldPoint.y);
        maxPoint.z = std::max(maxPoint.z, worldPoint.z);
    }
    
    return Math::BoundingBox(minPoint, maxPoint);
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
    if (!m_voxelManager) return true; // For testing: assume all voxels exist when no manager
    
    // VoxelId.position is already IncrementCoordinates, so no conversion needed
    return m_voxelManager->hasVoxel(voxel.position, voxel.resolution);
}

}
}