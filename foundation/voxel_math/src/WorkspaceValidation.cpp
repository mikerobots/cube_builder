#include "../include/voxel_math/WorkspaceValidation.h"
#include "../include/voxel_math/VoxelGridMath.h"
#include "../../math/CoordinateConverter.h"
#include <algorithm>
#include <cmath>

namespace VoxelEditor {
namespace Math {

WorkspaceValidation::WorkspaceBounds::WorkspaceBounds(const Vector3f& workspaceSize) {
    size = workspaceSize;
    
    // Calculate bounds in world coordinates (centered at origin)
    float halfX = workspaceSize.x * 0.5f;
    float halfZ = workspaceSize.z * 0.5f;
    
    minWorld = WorldCoordinates(Vector3f(-halfX, 0.0f, -halfZ));
    maxWorld = WorldCoordinates(Vector3f(halfX, workspaceSize.y, halfZ));
    
    // Convert to increment coordinates
    minIncrement = CoordinateConverter::worldToIncrement(minWorld);
    maxIncrement = CoordinateConverter::worldToIncrement(maxWorld);
}

WorkspaceValidation::WorkspaceBounds WorkspaceValidation::createBounds(const Vector3f& workspaceSize) {
    return WorkspaceBounds(workspaceSize);
}

bool WorkspaceValidation::isInBounds(const IncrementCoordinates& pos, const WorkspaceBounds& bounds) {
    const Vector3i& p = pos.value();
    const Vector3i& min = bounds.minIncrement.value();
    const Vector3i& max = bounds.maxIncrement.value();
    
    return p.x >= min.x && p.x <= max.x &&
           p.y >= min.y && p.y <= max.y &&
           p.z >= min.z && p.z <= max.z;
}

bool WorkspaceValidation::isInBounds(const WorldCoordinates& pos, const WorkspaceBounds& bounds) {
    const Vector3f& p = pos.value();
    const Vector3f& min = bounds.minWorld.value();
    const Vector3f& max = bounds.maxWorld.value();
    
    return p.x >= min.x && p.x <= max.x &&
           p.y >= min.y && p.y <= max.y &&
           p.z >= min.z && p.z <= max.z;
}

bool WorkspaceValidation::voxelFitsInBounds(const IncrementCoordinates& pos, 
                                           VoxelData::VoxelResolution resolution,
                                           const WorkspaceBounds& bounds) {
    // Get voxel size in centimeters
    int voxelSizeCm = VoxelGridMath::getVoxelSizeCm(resolution);
    
    const Vector3i& p = pos.value();
    const Vector3i& min = bounds.minIncrement.value();
    const Vector3i& max = bounds.maxIncrement.value();
    
    // Check if voxel bottom-center position allows the voxel to fit
    // Remember: voxel extends from -halfSize to +halfSize in X and Z
    // and from 0 to voxelSize in Y (bottom-center positioning)
    int halfSize = voxelSizeCm / 2;
    
    // Check X bounds
    if (p.x - halfSize < min.x || p.x + halfSize > max.x) {
        return false;
    }
    
    // Check Y bounds (voxel extends upward from position)
    if (p.y < min.y || p.y + voxelSizeCm > max.y) {
        return false;
    }
    
    // Check Z bounds
    if (p.z - halfSize < min.z || p.z + halfSize > max.z) {
        return false;
    }
    
    return true;
}

IncrementCoordinates WorkspaceValidation::clampToBounds(const IncrementCoordinates& pos,
                                                       const WorkspaceBounds& bounds) {
    const Vector3i& p = pos.value();
    const Vector3i& min = bounds.minIncrement.value();
    const Vector3i& max = bounds.maxIncrement.value();
    
    return IncrementCoordinates(
        std::clamp(p.x, min.x, max.x),
        std::clamp(p.y, min.y, max.y),
        std::clamp(p.z, min.z, max.z)
    );
}

bool WorkspaceValidation::isAboveGroundPlane(const IncrementCoordinates& pos) {
    return pos.y() >= 0;
}

IncrementCoordinates WorkspaceValidation::clampToGroundPlane(const IncrementCoordinates& pos) {
    if (pos.y() < 0) {
        return IncrementCoordinates(pos.x(), 0, pos.z());
    }
    return pos;
}

std::optional<VoxelData::VoxelResolution> WorkspaceValidation::getMaxFittingVoxelSize(
    const IncrementCoordinates& pos,
    const WorkspaceBounds& bounds) {
    
    // Try each resolution from largest to smallest
    for (int i = static_cast<int>(VoxelData::VoxelResolution::COUNT) - 1; i >= 0; --i) {
        auto resolution = static_cast<VoxelData::VoxelResolution>(i);
        if (voxelFitsInBounds(pos, resolution, bounds)) {
            return resolution;
        }
    }
    
    return std::nullopt;  // No voxel size fits
}

WorkspaceValidation::Overhang WorkspaceValidation::calculateOverhang(
    const IncrementCoordinates& pos,
    VoxelData::VoxelResolution resolution,
    const WorkspaceBounds& bounds) {
    
    Overhang overhang;
    
    // Get voxel size
    int voxelSizeCm = VoxelGridMath::getVoxelSizeCm(resolution);
    int halfSize = voxelSizeCm / 2;
    
    const Vector3i& p = pos.value();
    const Vector3i& min = bounds.minIncrement.value();
    const Vector3i& max = bounds.maxIncrement.value();
    
    // Calculate overhang for each axis
    // X axis
    int voxelMinX = p.x - halfSize;
    int voxelMaxX = p.x + halfSize;
    if (voxelMinX < min.x) {
        overhang.minX = min.x - voxelMinX;
    }
    if (voxelMaxX > max.x) {
        overhang.maxX = voxelMaxX - max.x;
    }
    
    // Y axis (voxel extends upward from bottom-center)
    if (p.y < min.y) {
        overhang.minY = min.y - p.y;
    }
    int voxelMaxY = p.y + voxelSizeCm;
    if (voxelMaxY > max.y) {
        overhang.maxY = voxelMaxY - max.y;
    }
    
    // Z axis
    int voxelMinZ = p.z - halfSize;
    int voxelMaxZ = p.z + halfSize;
    if (voxelMinZ < min.z) {
        overhang.minZ = min.z - voxelMinZ;
    }
    if (voxelMaxZ > max.z) {
        overhang.maxZ = voxelMaxZ - max.z;
    }
    
    return overhang;
}

IncrementCoordinates WorkspaceValidation::findNearestValidPosition(
    const IncrementCoordinates& pos,
    VoxelData::VoxelResolution resolution,
    const WorkspaceBounds& bounds) {
    
    // First, clamp to ground plane
    IncrementCoordinates validPos = clampToGroundPlane(pos);
    
    // Check if it already fits
    if (voxelFitsInBounds(validPos, resolution, bounds)) {
        return validPos;
    }
    
    // Calculate overhang
    Overhang overhang = calculateOverhang(validPos, resolution, bounds);
    
    const Vector3i& p = validPos.value();
    int adjustedX = p.x;
    int adjustedY = p.y;
    int adjustedZ = p.z;
    
    // Adjust position to eliminate overhang
    if (overhang.minX > 0) {
        adjustedX += overhang.minX;
    } else if (overhang.maxX > 0) {
        adjustedX -= overhang.maxX;
    }
    
    if (overhang.minY > 0) {
        adjustedY += overhang.minY;
    } else if (overhang.maxY > 0) {
        adjustedY -= overhang.maxY;
    }
    
    if (overhang.minZ > 0) {
        adjustedZ += overhang.minZ;
    } else if (overhang.maxZ > 0) {
        adjustedZ -= overhang.maxZ;
    }
    
    return IncrementCoordinates(adjustedX, adjustedY, adjustedZ);
}

bool WorkspaceValidation::isValidWorkspaceSize(const Vector3f& size) {
    return size.x >= MIN_WORKSPACE_SIZE && size.x <= MAX_WORKSPACE_SIZE &&
           size.y >= MIN_WORKSPACE_SIZE && size.y <= MAX_WORKSPACE_SIZE &&
           size.z >= MIN_WORKSPACE_SIZE && size.z <= MAX_WORKSPACE_SIZE;
}

} // namespace Math
} // namespace VoxelEditor