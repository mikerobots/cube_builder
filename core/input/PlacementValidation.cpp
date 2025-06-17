#include "PlacementValidation.h"
#include "../voxel_data/VoxelDataManager.h"
#include <cmath>

namespace VoxelEditor {
namespace Input {

Math::Vector3i PlacementUtils::snapToValidIncrement(const Math::Vector3f& worldPos) {
    // Snap to nearest 1cm increment
    return Math::Vector3i(
        static_cast<int>(std::round(worldPos.x / INCREMENT_SIZE)),
        static_cast<int>(std::round(worldPos.y / INCREMENT_SIZE)),
        static_cast<int>(std::round(worldPos.z / INCREMENT_SIZE))
    );
}

Math::Vector3i PlacementUtils::snapToGridAligned(const Math::Vector3f& worldPos, 
                                                VoxelData::VoxelResolution resolution,
                                                bool shiftPressed) {
    // If shift is pressed, allow 1cm increment placement
    if (shiftPressed) {
        return snapToValidIncrement(worldPos);
    }
    
    // Otherwise, snap to voxel-size grid
    float voxelSize = VoxelData::getVoxelSize(resolution);
    float voxelSizeInIncrements = voxelSize / INCREMENT_SIZE;
    
    return Math::Vector3i(
        static_cast<int>(std::round(worldPos.x / voxelSize) * voxelSizeInIncrements),
        static_cast<int>(std::round(worldPos.y / voxelSize) * voxelSizeInIncrements),
        static_cast<int>(std::round(worldPos.z / voxelSize) * voxelSizeInIncrements)
    );
}

PlacementValidationResult PlacementUtils::validatePlacement(const Math::Vector3i& gridPos,
                                                           VoxelData::VoxelResolution resolution,
                                                           const Math::Vector3f& workspaceSize) {
    // Check for invalid position (would be caught by NaN/inf in world position)
    if (std::abs(gridPos.x) > 1000000 || std::abs(gridPos.y) > 1000000 || std::abs(gridPos.z) > 1000000) {
        return PlacementValidationResult::InvalidPosition;
    }
    
    // Check Y >= 0 constraint
    if (gridPos.y < 0) {
        return PlacementValidationResult::InvalidYBelowZero;
    }
    
    // Convert to world position to check workspace bounds
    Math::Vector3f worldPos = incrementGridToWorld(gridPos);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Check if voxel would be within workspace bounds
    // Workspace is centered at origin, so bounds are [-size/2, size/2]
    float halfX = workspaceSize.x / 2.0f;
    float halfZ = workspaceSize.z / 2.0f;
    
    if (worldPos.x < -halfX || worldPos.x + voxelSize > halfX ||
        worldPos.y < 0 || worldPos.y + voxelSize > workspaceSize.y ||
        worldPos.z < -halfZ || worldPos.z + voxelSize > halfZ) {
        return PlacementValidationResult::InvalidOutOfBounds;
    }
    
    // Note: Overlap checking would be done by VoxelDataManager
    // This validation only checks basic constraints
    
    return PlacementValidationResult::Valid;
}

bool PlacementUtils::isValidIncrementPosition(const Math::Vector3i& pos) {
    // Y must be non-negative (no voxels below ground)
    return pos.y >= 0;
}

Math::Vector3i PlacementUtils::worldToIncrementGrid(const Math::Vector3f& worldPos) {
    return Math::Vector3i(
        static_cast<int>(std::floor(worldPos.x / INCREMENT_SIZE)),
        static_cast<int>(std::floor(worldPos.y / INCREMENT_SIZE)),
        static_cast<int>(std::floor(worldPos.z / INCREMENT_SIZE))
    );
}

Math::Vector3f PlacementUtils::incrementGridToWorld(const Math::Vector3i& gridPos) {
    return Math::Vector3f(
        static_cast<float>(gridPos.x) * INCREMENT_SIZE,
        static_cast<float>(gridPos.y) * INCREMENT_SIZE,
        static_cast<float>(gridPos.z) * INCREMENT_SIZE
    );
}

PlacementContext PlacementUtils::getPlacementContext(const Math::Vector3f& worldPos,
                                                    VoxelData::VoxelResolution resolution,
                                                    bool shiftPressed,
                                                    const Math::Vector3f& workspaceSize) {
    PlacementContext context;
    context.worldPosition = worldPos;
    context.resolution = resolution;
    context.shiftPressed = shiftPressed;
    
    // Snap position based on shift key state
    context.snappedGridPos = snapToGridAligned(worldPos, resolution, shiftPressed);
    
    // Validate the placement
    context.validation = validatePlacement(context.snappedGridPos, resolution, workspaceSize);
    
    return context;
}

// Phase 3 Enhancement: Smart snapping for same-size voxels
Math::Vector3i PlacementUtils::snapToSameSizeVoxel(const Math::Vector3f& worldPos,
                                                   VoxelData::VoxelResolution resolution,
                                                   const VoxelData::VoxelDataManager& dataManager,
                                                   bool shiftPressed) {
    // If shift is pressed, use 1cm increments
    if (shiftPressed) {
        return snapToValidIncrement(worldPos);
    }
    
    // Start with grid-aligned position for this resolution
    Math::Vector3i gridPos = snapToGridAligned(worldPos, resolution, false);
    
    // Convert to world position to check for nearby same-size voxels
    Math::Vector3f gridWorldPos = incrementGridToWorld(gridPos);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Check for adjacent same-size voxels that we should align with
    // Look in a 3x3x3 area around the proposed position
    int searchRadius = 2; // Check 2 voxel widths in each direction
    
    for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
        for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
            for (int dz = -searchRadius; dz <= searchRadius; ++dz) {
                // Skip center position
                if (dx == 0 && dy == 0 && dz == 0) continue;
                
                Math::Vector3i checkPos = gridPos + Math::Vector3i(dx, dy, dz);
                
                // Check if there's a voxel at this position with the same resolution
                if (dataManager.getVoxel(checkPos, resolution)) {
                    // Found a same-size voxel nearby!
                    // For same-size voxels, grid alignment is automatic
                    // The grid-aligned position is already correct
                    return gridPos;
                }
            }
        }
    }
    
    // No same-size voxels found nearby, use standard grid alignment
    return gridPos;
}

// Phase 3 Enhancement: Sub-grid positioning for smaller voxels on larger Surface Faces
Math::Vector3i PlacementUtils::snapToSurfaceFaceGrid(const Math::Vector3f& hitPoint,
                                                   const Math::Vector3i& surfaceFaceVoxelPos,
                                                   VoxelData::VoxelResolution surfaceFaceVoxelRes,
                                                   VoxelData::FaceDirection surfaceFaceDir,
                                                   VoxelData::VoxelResolution placementResolution) {
    using namespace VoxelData;
    
    float surfaceFaceVoxelSize = getVoxelSize(surfaceFaceVoxelRes);
    float placementVoxelSize = getVoxelSize(placementResolution);
    
    // Convert surface face voxel position to world coordinates
    Math::Vector3f surfaceFaceWorldPos = incrementGridToWorld(surfaceFaceVoxelPos);
    
    // Calculate the surface face plane position
    Math::Vector3f surfaceFacePlanePos = surfaceFaceWorldPos;
    switch (surfaceFaceDir) {
        case FaceDirection::PosX:
            surfaceFacePlanePos.x += surfaceFaceVoxelSize;
            break;
        case FaceDirection::NegX:
            // Plane is at the left edge
            break;
        case FaceDirection::PosY:
            surfaceFacePlanePos.y += surfaceFaceVoxelSize;
            break;
        case FaceDirection::NegY:
            // Plane is at the bottom edge
            break;
        case FaceDirection::PosZ:
            surfaceFacePlanePos.z += surfaceFaceVoxelSize;
            break;
        case FaceDirection::NegZ:
            // Plane is at the front edge
            break;
    }
    
    // Project the hit point onto the surface face plane and find the nearest 1cm grid position
    Math::Vector3f snappedPos = hitPoint;
    
    // Constrain the position to the surface face plane
    switch (surfaceFaceDir) {
        case FaceDirection::PosX:
        case FaceDirection::NegX:
            snappedPos.x = surfaceFacePlanePos.x;
            break;
        case FaceDirection::PosY:
        case FaceDirection::NegY:
            snappedPos.y = surfaceFacePlanePos.y;
            break;
        case FaceDirection::PosZ:
        case FaceDirection::NegZ:
            snappedPos.z = surfaceFacePlanePos.z;
            break;
    }
    
    // Snap to 1cm increments
    Math::Vector3i gridPos = snapToValidIncrement(snappedPos);
    
    // Ensure the smaller voxel fits within the surface face bounds
    Math::Vector3f gridWorldPos = incrementGridToWorld(gridPos);
    Math::Vector3f surfaceFaceMin = surfaceFaceWorldPos;
    Math::Vector3f surfaceFaceMax = surfaceFaceWorldPos + Math::Vector3f(surfaceFaceVoxelSize, surfaceFaceVoxelSize, surfaceFaceVoxelSize);
    
    // Clamp to surface face bounds (except in the surface face normal direction)
    switch (surfaceFaceDir) {
        case FaceDirection::PosX:
        case FaceDirection::NegX:
            // Y and Z must be within surface face bounds
            if (gridWorldPos.y < surfaceFaceMin.y) {
                gridPos.y = static_cast<int>(std::round(surfaceFaceMin.y / INCREMENT_SIZE));
            } else if (gridWorldPos.y + placementVoxelSize > surfaceFaceMax.y) {
                gridPos.y = static_cast<int>(std::round((surfaceFaceMax.y - placementVoxelSize) / INCREMENT_SIZE));
            }
            if (gridWorldPos.z < surfaceFaceMin.z) {
                gridPos.z = static_cast<int>(std::round(surfaceFaceMin.z / INCREMENT_SIZE));
            } else if (gridWorldPos.z + placementVoxelSize > surfaceFaceMax.z) {
                gridPos.z = static_cast<int>(std::round((surfaceFaceMax.z - placementVoxelSize) / INCREMENT_SIZE));
            }
            break;
        case FaceDirection::PosY:
        case FaceDirection::NegY:
            // X and Z must be within surface face bounds
            if (gridWorldPos.x < surfaceFaceMin.x) {
                gridPos.x = static_cast<int>(std::round(surfaceFaceMin.x / INCREMENT_SIZE));
            } else if (gridWorldPos.x + placementVoxelSize > surfaceFaceMax.x) {
                gridPos.x = static_cast<int>(std::round((surfaceFaceMax.x - placementVoxelSize) / INCREMENT_SIZE));
            }
            if (gridWorldPos.z < surfaceFaceMin.z) {
                gridPos.z = static_cast<int>(std::round(surfaceFaceMin.z / INCREMENT_SIZE));
            } else if (gridWorldPos.z + placementVoxelSize > surfaceFaceMax.z) {
                gridPos.z = static_cast<int>(std::round((surfaceFaceMax.z - placementVoxelSize) / INCREMENT_SIZE));
            }
            break;
        case FaceDirection::PosZ:
        case FaceDirection::NegZ:
            // X and Y must be within surface face bounds
            if (gridWorldPos.x < surfaceFaceMin.x) {
                gridPos.x = static_cast<int>(std::round(surfaceFaceMin.x / INCREMENT_SIZE));
            } else if (gridWorldPos.x + placementVoxelSize > surfaceFaceMax.x) {
                gridPos.x = static_cast<int>(std::round((surfaceFaceMax.x - placementVoxelSize) / INCREMENT_SIZE));
            }
            if (gridWorldPos.y < surfaceFaceMin.y) {
                gridPos.y = static_cast<int>(std::round(surfaceFaceMin.y / INCREMENT_SIZE));
            } else if (gridWorldPos.y + placementVoxelSize > surfaceFaceMax.y) {
                gridPos.y = static_cast<int>(std::round((surfaceFaceMax.y - placementVoxelSize) / INCREMENT_SIZE));
            }
            break;
    }
    
    return gridPos;
}

// Phase 3 Enhancement: Smart placement context with voxel data awareness
PlacementContext PlacementUtils::getSmartPlacementContext(const Math::Vector3f& worldPos,
                                                         VoxelData::VoxelResolution resolution,
                                                         bool shiftPressed,
                                                         const Math::Vector3f& workspaceSize,
                                                         const VoxelData::VoxelDataManager& dataManager,
                                                         const Math::Vector3i* surfaceFaceVoxelPos,
                                                         VoxelData::VoxelResolution surfaceFaceVoxelRes,
                                                         VoxelData::FaceDirection surfaceFaceDir) {
    PlacementContext context;
    context.worldPosition = worldPos;
    context.resolution = resolution;
    context.shiftPressed = shiftPressed;
    
    // Choose snapping method based on context
    if (surfaceFaceVoxelPos != nullptr) {
        // We're placing on a specific surface face - use sub-grid positioning
        context.snappedGridPos = snapToSurfaceFaceGrid(worldPos, *surfaceFaceVoxelPos, surfaceFaceVoxelRes, surfaceFaceDir, resolution);
    } else {
        // General placement - use smart same-size snapping
        context.snappedGridPos = snapToSameSizeVoxel(worldPos, resolution, dataManager, shiftPressed);
    }
    
    // Validate the placement
    context.validation = validatePlacement(context.snappedGridPos, resolution, workspaceSize);
    
    return context;
}

bool PlacementUtils::isValidForIncrementPlacement(const Math::Vector3f& worldPos, VoxelData::VoxelResolution resolution) {
    // Check if position is not NaN or infinite
    if (!std::isfinite(worldPos.x) || !std::isfinite(worldPos.y) || !std::isfinite(worldPos.z)) {
        return false;
    }
    
    // Y must be >= 0 (ground constraint)
    if (worldPos.y < 0.0f) {
        return false;
    }
    
    // The position itself is valid from an increment perspective
    // VoxelDataManager will handle workspace bounds and overlap checking
    return true;
}

}
}