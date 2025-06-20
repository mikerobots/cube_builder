#include "PlacementValidation.h"
#include "../voxel_data/VoxelDataManager.h"
#include <cmath>

namespace VoxelEditor {
namespace Input {

Math::IncrementCoordinates PlacementUtils::snapToValidIncrement(const Math::Vector3f& worldPos) {
    // Use centralized coordinate converter for consistent snapping
    return Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(worldPos));
}

Math::IncrementCoordinates PlacementUtils::snapToGridAligned(const Math::Vector3f& worldPos, 
                                                VoxelData::VoxelResolution resolution,
                                                bool shiftPressed) {
    // If shift is pressed, allow 1cm increment placement
    if (shiftPressed) {
        return snapToValidIncrement(worldPos);
    }
    
    // Otherwise, snap to voxel-size grid using resolution snapping
    Math::IncrementCoordinates baseIncrement = snapToValidIncrement(worldPos);
    return Math::CoordinateConverter::snapToVoxelResolution(baseIncrement, resolution);
}

PlacementValidationResult PlacementUtils::validatePlacement(const Math::IncrementCoordinates& incrementPos,
                                                           VoxelData::VoxelResolution resolution,
                                                           const Math::Vector3f& workspaceSize) {
    // Check for invalid position (would be caught by NaN/inf in world position)
    if (std::abs(incrementPos.x()) > 1000000 || std::abs(incrementPos.y()) > 1000000 || std::abs(incrementPos.z()) > 1000000) {
        return PlacementValidationResult::InvalidPosition;
    }
    
    // Check Y >= 0 constraint first (before general bounds check)
    if (incrementPos.y() < 0) {
        return PlacementValidationResult::InvalidYBelowZero;
    }
    
    // Check if the voxel (including its size) fits within workspace bounds
    float voxelSize = VoxelData::getVoxelSize(resolution);
    int voxelSize_cm = static_cast<int>(voxelSize * 100.0f);
    
    // Get workspace bounds
    int halfX_cm = static_cast<int>(workspaceSize.x * 100.0f * 0.5f);
    int halfZ_cm = static_cast<int>(workspaceSize.z * 100.0f * 0.5f);
    int height_cm = static_cast<int>(workspaceSize.y * 100.0f);
    
    // Check if voxel extends outside bounds (voxel origin + size must fit)
    if (incrementPos.x() < -halfX_cm || incrementPos.x() + voxelSize_cm > halfX_cm ||
        incrementPos.y() < 0 || incrementPos.y() + voxelSize_cm > height_cm ||
        incrementPos.z() < -halfZ_cm || incrementPos.z() + voxelSize_cm > halfZ_cm) {
        return PlacementValidationResult::InvalidOutOfBounds;
    }
    
    // Note: Overlap checking would be done by VoxelDataManager
    // This validation only checks basic constraints
    
    return PlacementValidationResult::Valid;
}

bool PlacementUtils::isValidIncrementPosition(const Math::IncrementCoordinates& pos) {
    // Basic validation: Y must be >= 0 (ground plane constraint)
    // Workspace bounds check requires workspace size, so this method only checks the basic constraint
    return pos.y() >= 0;
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
    context.snappedIncrementPos = snapToGridAligned(worldPos, resolution, shiftPressed);
    
    // Validate the placement
    context.validation = validatePlacement(context.snappedIncrementPos, resolution, workspaceSize);
    
    return context;
}

// Phase 3 Enhancement: Smart snapping for same-size voxels
Math::IncrementCoordinates PlacementUtils::snapToSameSizeVoxel(const Math::Vector3f& worldPos,
                                                   VoxelData::VoxelResolution resolution,
                                                   const VoxelData::VoxelDataManager& dataManager,
                                                   bool shiftPressed) {
    // If shift is pressed, use 1cm increments
    if (shiftPressed) {
        return snapToValidIncrement(worldPos);
    }
    
    // Start with grid-aligned position for this resolution
    Math::IncrementCoordinates incrementPos = snapToGridAligned(worldPos, resolution, false);
    
    // Convert to world position to check for nearby same-size voxels
    Math::WorldCoordinates worldCoords = Math::CoordinateConverter::incrementToWorld(incrementPos);
    Math::Vector3f gridWorldPos = worldCoords.value();
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Check for adjacent same-size voxels that we should align with
    // Look in a 3x3x3 area around the proposed position
    int searchRadius = 2; // Check 2 voxel widths in each direction
    
    for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
        for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
            for (int dz = -searchRadius; dz <= searchRadius; ++dz) {
                // Skip center position
                if (dx == 0 && dy == 0 && dz == 0) continue;
                
                Math::IncrementCoordinates checkPos(incrementPos.x() + dx, incrementPos.y() + dy, incrementPos.z() + dz);
                
                // Check if there's a voxel at this position with the same resolution
                if (dataManager.getVoxel(checkPos, resolution)) {
                    // Found a same-size voxel nearby!
                    // For same-size voxels, grid alignment is automatic
                    // The grid-aligned position is already correct
                    return incrementPos;
                }
            }
        }
    }
    
    // No same-size voxels found nearby, use standard grid alignment
    return incrementPos;
}

// Phase 3 Enhancement: Sub-grid positioning for smaller voxels on larger Surface Faces
Math::IncrementCoordinates PlacementUtils::snapToSurfaceFaceGrid(const Math::Vector3f& hitPoint,
                                                   const Math::IncrementCoordinates& surfaceFaceVoxelPos,
                                                   VoxelData::VoxelResolution surfaceFaceVoxelRes,
                                                   VoxelData::FaceDirection surfaceFaceDir,
                                                   VoxelData::VoxelResolution placementResolution) {
    using namespace VoxelData;
    
    float surfaceFaceVoxelSize = getVoxelSize(surfaceFaceVoxelRes);
    float placementVoxelSize = getVoxelSize(placementResolution);
    
    // Convert surface face voxel position to world coordinates
    Math::WorldCoordinates surfaceFaceWorldCoords = Math::CoordinateConverter::incrementToWorld(surfaceFaceVoxelPos);
    Math::Vector3f surfaceFaceWorldPos = surfaceFaceWorldCoords.value();
    
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
    Math::IncrementCoordinates incrementPos = snapToValidIncrement(snappedPos);
    
    // Ensure the smaller voxel fits within the surface face bounds
    Math::WorldCoordinates gridWorldCoords = Math::CoordinateConverter::incrementToWorld(incrementPos);
    Math::Vector3f gridWorldPos = gridWorldCoords.value();
    Math::Vector3f surfaceFaceMin = surfaceFaceWorldPos;
    Math::Vector3f surfaceFaceMax = surfaceFaceWorldPos + Math::Vector3f(surfaceFaceVoxelSize, surfaceFaceVoxelSize, surfaceFaceVoxelSize);
    
    // Clamp to surface face bounds (except in the surface face normal direction)
    switch (surfaceFaceDir) {
        case FaceDirection::PosX:
        case FaceDirection::NegX:
            // Y and Z must be within surface face bounds
            if (gridWorldPos.y < surfaceFaceMin.y) {
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), static_cast<int>(std::round(surfaceFaceMin.y / INCREMENT_SIZE)), incrementPos.z());
            } else if (gridWorldPos.y + placementVoxelSize > surfaceFaceMax.y) {
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), static_cast<int>(std::round((surfaceFaceMax.y - placementVoxelSize) / INCREMENT_SIZE)), incrementPos.z());
            }
            if (gridWorldPos.z < surfaceFaceMin.z) {
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), incrementPos.y(), static_cast<int>(std::round(surfaceFaceMin.z / INCREMENT_SIZE)));
            } else if (gridWorldPos.z + placementVoxelSize > surfaceFaceMax.z) {
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), incrementPos.y(), static_cast<int>(std::round((surfaceFaceMax.z - placementVoxelSize) / INCREMENT_SIZE)));
            }
            break;
        case FaceDirection::PosY:
        case FaceDirection::NegY:
            // X and Z must be within surface face bounds
            if (gridWorldPos.x < surfaceFaceMin.x) {
                incrementPos = Math::IncrementCoordinates(static_cast<int>(std::round(surfaceFaceMin.x / INCREMENT_SIZE)), incrementPos.y(), incrementPos.z());
            } else if (gridWorldPos.x + placementVoxelSize > surfaceFaceMax.x) {
                incrementPos = Math::IncrementCoordinates(static_cast<int>(std::round((surfaceFaceMax.x - placementVoxelSize) / INCREMENT_SIZE)), incrementPos.y(), incrementPos.z());
            }
            if (gridWorldPos.z < surfaceFaceMin.z) {
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), incrementPos.y(), static_cast<int>(std::round(surfaceFaceMin.z / INCREMENT_SIZE)));
            } else if (gridWorldPos.z + placementVoxelSize > surfaceFaceMax.z) {
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), incrementPos.y(), static_cast<int>(std::round((surfaceFaceMax.z - placementVoxelSize) / INCREMENT_SIZE)));
            }
            break;
        case FaceDirection::PosZ:
        case FaceDirection::NegZ:
            // X and Y must be within surface face bounds
            if (gridWorldPos.x < surfaceFaceMin.x) {
                incrementPos = Math::IncrementCoordinates(static_cast<int>(std::round(surfaceFaceMin.x / INCREMENT_SIZE)), incrementPos.y(), incrementPos.z());
            } else if (gridWorldPos.x + placementVoxelSize > surfaceFaceMax.x) {
                incrementPos = Math::IncrementCoordinates(static_cast<int>(std::round((surfaceFaceMax.x - placementVoxelSize) / INCREMENT_SIZE)), incrementPos.y(), incrementPos.z());
            }
            if (gridWorldPos.y < surfaceFaceMin.y) {
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), static_cast<int>(std::round(surfaceFaceMin.y / INCREMENT_SIZE)), incrementPos.z());
            } else if (gridWorldPos.y + placementVoxelSize > surfaceFaceMax.y) {
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), static_cast<int>(std::round((surfaceFaceMax.y - placementVoxelSize) / INCREMENT_SIZE)), incrementPos.z());
            }
            break;
    }
    
    return incrementPos;
}

// Phase 3 Enhancement: Smart placement context with voxel data awareness
PlacementContext PlacementUtils::getSmartPlacementContext(const Math::Vector3f& worldPos,
                                                         VoxelData::VoxelResolution resolution,
                                                         bool shiftPressed,
                                                         const Math::Vector3f& workspaceSize,
                                                         const VoxelData::VoxelDataManager& dataManager,
                                                         const Math::IncrementCoordinates* surfaceFaceVoxelPos,
                                                         VoxelData::VoxelResolution surfaceFaceVoxelRes,
                                                         VoxelData::FaceDirection surfaceFaceDir) {
    PlacementContext context;
    context.worldPosition = worldPos;
    context.resolution = resolution;
    context.shiftPressed = shiftPressed;
    
    // Choose snapping method based on context
    if (surfaceFaceVoxelPos != nullptr) {
        // We're placing on a specific surface face - use sub-grid positioning
        context.snappedIncrementPos = snapToSurfaceFaceGrid(worldPos, *surfaceFaceVoxelPos, surfaceFaceVoxelRes, surfaceFaceDir, resolution);
    } else {
        // General placement - use smart same-size snapping
        context.snappedIncrementPos = snapToSameSizeVoxel(worldPos, resolution, dataManager, shiftPressed);
    }
    
    // Validate the placement
    context.validation = validatePlacement(context.snappedIncrementPos, resolution, workspaceSize);
    
    return context;
}

bool PlacementUtils::isValidForIncrementPlacement(const Math::Vector3f& worldPos, VoxelData::VoxelResolution resolution) {
    // Check if position is not NaN or infinite
    if (!std::isfinite(worldPos.x) || !std::isfinite(worldPos.y) || !std::isfinite(worldPos.z)) {
        return false;
    }
    
    // Position itself doesn't need Y >= 0 constraint at increment level
    // Workspace bounds checking will be handled by validatePlacement()
    
    // The position itself is valid from an increment perspective
    // VoxelDataManager will handle workspace bounds and overlap checking
    return true;
}

}
}