#include "BoxSelector.h"
#include "../../foundation/math/Matrix4f.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <limits>
#include <string>

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
    
    // Calculate workspace bounds in world coordinates (centered coordinate system)
    Math::BoundingBox workspaceBounds(
        Math::Vector3f(-workspaceSize.x/2, 0.0f, -workspaceSize.z/2),
        Math::Vector3f(workspaceSize.x/2, workspaceSize.y, workspaceSize.z/2)
    );
    
    // Clamp the selection box to workspace bounds to prevent excessive iteration
    Math::BoundingBox clampedBox;
    clampedBox.min.x = std::max(worldBox.min.x, workspaceBounds.min.x);
    clampedBox.min.y = std::max(worldBox.min.y, workspaceBounds.min.y);
    clampedBox.min.z = std::max(worldBox.min.z, workspaceBounds.min.z);
    clampedBox.max.x = std::min(worldBox.max.x, workspaceBounds.max.x);
    clampedBox.max.y = std::min(worldBox.max.y, workspaceBounds.max.y);
    clampedBox.max.z = std::min(worldBox.max.z, workspaceBounds.max.z);
    
    // Early exit if clamped box is invalid (min > max)
    if (clampedBox.min.x > clampedBox.max.x || 
        clampedBox.min.y > clampedBox.max.y || 
        clampedBox.min.z > clampedBox.max.z) {
        return result; // Return empty selection
    }
    
    // Convert clamped world box corners to increment coordinates using the coordinate converter
    Math::WorldCoordinates minWorld(clampedBox.min);
    Math::WorldCoordinates maxWorld(clampedBox.max);
    
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
    
    // Use direct coordinates without snapping to allow any 1cm position
    Math::IncrementCoordinates snappedMin = Math::IncrementCoordinates(actualMin);
    Math::IncrementCoordinates snappedMax = Math::IncrementCoordinates(actualMax);
    
    
    // Additional safety check: limit maximum iterations to prevent hanging
    const int maxIterationsPerAxis = 1000; // Reasonable limit for any axis
    int xRange = (snappedMax.x() - snappedMin.x()) / voxelSizeCm + 1;
    int yRange = (snappedMax.y() - snappedMin.y()) / voxelSizeCm + 1;
    int zRange = (snappedMax.z() - snappedMin.z()) / voxelSizeCm + 1;
    
    if (xRange > maxIterationsPerAxis || yRange > maxIterationsPerAxis || zRange > maxIterationsPerAxis) {
        std::string warningMsg = "BoxSelector: Selection range too large (" + 
            std::to_string(xRange) + "x" + std::to_string(yRange) + "x" + 
            std::to_string(zRange) + " voxels), clamping to prevent excessive computation";
        Logging::Logger::getInstance().warning(warningMsg);
        
        // Clamp the range to maximum iterations
        if (xRange > maxIterationsPerAxis) {
            int centerX = (snappedMin.x() + snappedMax.x()) / 2;
            int halfRange = (maxIterationsPerAxis * voxelSizeCm) / 2;
            snappedMin = Math::IncrementCoordinates(centerX - halfRange, snappedMin.y(), snappedMin.z());
            snappedMax = Math::IncrementCoordinates(centerX + halfRange, snappedMax.y(), snappedMax.z());
        }
        if (yRange > maxIterationsPerAxis) {
            int centerY = (snappedMin.y() + snappedMax.y()) / 2;
            int halfRange = (maxIterationsPerAxis * voxelSizeCm) / 2;
            snappedMin = Math::IncrementCoordinates(snappedMin.x(), centerY - halfRange, snappedMin.z());
            snappedMax = Math::IncrementCoordinates(snappedMax.x(), centerY + halfRange, snappedMax.z());
        }
        if (zRange > maxIterationsPerAxis) {
            int centerZ = (snappedMin.z() + snappedMax.z()) / 2;
            int halfRange = (maxIterationsPerAxis * voxelSizeCm) / 2;
            snappedMin = Math::IncrementCoordinates(snappedMin.x(), snappedMin.y(), centerZ - halfRange);
            snappedMax = Math::IncrementCoordinates(snappedMax.x(), snappedMax.y(), centerZ + halfRange);
        }
    }
    
    // Select voxels in range - we need to check every position where a voxel could exist
    // that would intersect with the selection box
    // For each axis, we need to find all voxel positions that could potentially intersect
    
    // Expand the range to include any voxel that might intersect the box
    // A voxel at position P with size S has bounds [P, P+S)
    // For it to intersect the box [boxMin, boxMax], we need P < boxMax and P+S > boxMin
    // This means P must be in range (boxMin - S, boxMax]
    // We use boxMin - S + 1 because we want intersection, not just touching
    int expandedMinX = snappedMin.x() - voxelSizeCm + 1;
    int expandedMinY = snappedMin.y() - voxelSizeCm + 1;
    int expandedMinZ = snappedMin.z() - voxelSizeCm + 1;
    // We don't expand max because a voxel starting after boxMax won't intersect
    int expandedMaxX = snappedMax.x();
    int expandedMaxY = snappedMax.y();
    int expandedMaxZ = snappedMax.z();
    
    // Safety check for iteration count
    int totalIterations = (expandedMaxX - expandedMinX + 1) * 
                         (expandedMaxY - expandedMinY + 1) * 
                         (expandedMaxZ - expandedMinZ + 1);
    const int maxTotalIterations = 1000000; // 100x100x100
    
    if (totalIterations > maxTotalIterations) {
        std::string errorMsg = "BoxSelector: Too many iterations required (" + 
            std::to_string(totalIterations) + "), aborting to prevent performance issues";
        Logging::Logger::getInstance().error(errorMsg);
        return result; // Return empty selection
    }
    
    // Since voxels can be placed at any 1cm position, we iterate through all 1cm positions
    for (int x = expandedMinX; x <= expandedMaxX; x += 1) {
        for (int y = expandedMinY; y <= expandedMaxY; y += 1) {
            for (int z = expandedMinZ; z <= expandedMaxZ; z += 1) {
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