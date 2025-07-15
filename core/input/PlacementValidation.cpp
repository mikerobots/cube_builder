#include "PlacementValidation.h"
#include "../voxel_data/VoxelDataManager.h"
#include "voxel_math/VoxelBounds.h"
#include "voxel_math/VoxelCollision.h"
#include "voxel_math/WorkspaceValidation.h"
#include <cmath>

namespace VoxelEditor {
namespace Input {

Math::IncrementCoordinates PlacementUtils::snapToValidIncrement(const Math::WorldCoordinates& worldPos) {
    // Use centralized coordinate converter for consistent snapping
    return Math::CoordinateConverter::worldToIncrement(worldPos);
}

Math::IncrementCoordinates PlacementUtils::snapToGridAligned(const Math::WorldCoordinates& worldPos, 
                                                VoxelData::VoxelResolution resolution,
                                                bool shiftPressed) {
    // With the new requirements, all voxels are placed at exact 1cm increment positions
    // No resolution-based snapping occurs. The shift parameter and resolution are no longer used for snapping.
    (void)resolution;   // Mark as intentionally unused
    (void)shiftPressed; // Mark as intentionally unused
    
    // Always snap to 1cm increment grid - no voxel-size specific snapping
    return snapToValidIncrement(worldPos);
}

PlacementValidationResult PlacementUtils::validatePlacement(const Math::IncrementCoordinates& incrementPos,
                                                           VoxelData::VoxelResolution resolution,
                                                           const Math::Vector3f& workspaceSize) {
    // Check for invalid position (would be caught by NaN/inf in world position)
    if (std::abs(incrementPos.x()) > 1000000 || std::abs(incrementPos.y()) > 1000000 || std::abs(incrementPos.z()) > 1000000) {
        return PlacementValidationResult::InvalidPosition;
    }
    
    // Use WorkspaceValidation to check ground plane constraint
    if (!Math::WorkspaceValidation::isAboveGroundPlane(incrementPos)) {
        return PlacementValidationResult::InvalidYBelowZero;
    }
    
    // Create workspace bounds and check if voxel fits
    Math::WorkspaceValidation::WorkspaceBounds bounds = Math::WorkspaceValidation::createBounds(workspaceSize);
    
    if (!Math::WorkspaceValidation::voxelFitsInBounds(incrementPos, resolution, bounds)) {
        return PlacementValidationResult::InvalidOutOfBounds;
    }
    
    // Note: Overlap checking would be done by VoxelDataManager
    // This validation only checks basic constraints
    
    return PlacementValidationResult::Valid;
}

bool PlacementUtils::isValidIncrementPosition(const Math::IncrementCoordinates& pos) {
    // Use WorkspaceValidation to check ground plane constraint
    return Math::WorkspaceValidation::isAboveGroundPlane(pos);
}

PlacementContext PlacementUtils::getPlacementContext(const Math::WorldCoordinates& worldPos,
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
Math::IncrementCoordinates PlacementUtils::snapToSameSizeVoxel(const Math::WorldCoordinates& worldPos,
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
    (void)worldCoords; // Mark as unused
    float voxelSize = VoxelData::getVoxelSize(resolution);
    (void)voxelSize; // Mark as unused
    
    // Use VoxelCollision to find nearby voxels more efficiently
    Math::VoxelBounds searchBounds(incrementPos, voxelSize * 3.0f); // Search in 3x voxel size radius
    const VoxelData::VoxelGrid* grid = dataManager.getGrid(resolution);
    if (!grid) {
        // No grid for this resolution, use standard grid alignment
        return incrementPos;
    }
    auto nearbyVoxels = Math::VoxelCollision::getVoxelsInRegion(searchBounds, *grid);
    
    if (!nearbyVoxels.empty()) {
        // Found same-size voxels nearby!
        // For same-size voxels, grid alignment is automatic
        // The grid-aligned position is already correct
        return incrementPos;
    }
    
    // No same-size voxels found nearby, use standard grid alignment
    return incrementPos;
}

// Phase 3 Enhancement: Sub-grid positioning for smaller voxels on larger Surface Faces
Math::IncrementCoordinates PlacementUtils::snapToSurfaceFaceGrid(const Math::WorldCoordinates& hitPoint,
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
    Math::Vector3f snappedPos = hitPoint.value();
    
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
    Math::IncrementCoordinates incrementPos = snapToValidIncrement(Math::WorldCoordinates(snappedPos));
    
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
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(incrementPos.x() * Math::CoordinateConverter::CM_TO_METERS, surfaceFaceMin.y, incrementPos.z() * Math::CoordinateConverter::CM_TO_METERS));
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), newPos.y(), incrementPos.z());
            } else if (gridWorldPos.y + placementVoxelSize > surfaceFaceMax.y) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(incrementPos.x() * Math::CoordinateConverter::CM_TO_METERS, surfaceFaceMax.y - placementVoxelSize, incrementPos.z() * Math::CoordinateConverter::CM_TO_METERS));
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), newPos.y(), incrementPos.z());
            }
            if (gridWorldPos.z < surfaceFaceMin.z) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(incrementPos.x() * Math::CoordinateConverter::CM_TO_METERS, incrementPos.y() * Math::CoordinateConverter::CM_TO_METERS, surfaceFaceMin.z));
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), incrementPos.y(), newPos.z());
            } else if (gridWorldPos.z + placementVoxelSize > surfaceFaceMax.z) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(incrementPos.x() * Math::CoordinateConverter::CM_TO_METERS, incrementPos.y() * Math::CoordinateConverter::CM_TO_METERS, surfaceFaceMax.z - placementVoxelSize));
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), incrementPos.y(), newPos.z());
            }
            break;
        case FaceDirection::PosY:
        case FaceDirection::NegY:
            // X and Z must be within surface face bounds
            if (gridWorldPos.x < surfaceFaceMin.x) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(surfaceFaceMin.x, incrementPos.y() * Math::CoordinateConverter::CM_TO_METERS, incrementPos.z() * Math::CoordinateConverter::CM_TO_METERS));
                incrementPos = Math::IncrementCoordinates(newPos.x(), incrementPos.y(), incrementPos.z());
            } else if (gridWorldPos.x + placementVoxelSize > surfaceFaceMax.x) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(surfaceFaceMax.x - placementVoxelSize, incrementPos.y() * Math::CoordinateConverter::CM_TO_METERS, incrementPos.z() * Math::CoordinateConverter::CM_TO_METERS));
                incrementPos = Math::IncrementCoordinates(newPos.x(), incrementPos.y(), incrementPos.z());
            }
            if (gridWorldPos.z < surfaceFaceMin.z) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(incrementPos.x() * Math::CoordinateConverter::CM_TO_METERS, incrementPos.y() * Math::CoordinateConverter::CM_TO_METERS, surfaceFaceMin.z));
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), incrementPos.y(), newPos.z());
            } else if (gridWorldPos.z + placementVoxelSize > surfaceFaceMax.z) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(incrementPos.x() * Math::CoordinateConverter::CM_TO_METERS, incrementPos.y() * Math::CoordinateConverter::CM_TO_METERS, surfaceFaceMax.z - placementVoxelSize));
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), incrementPos.y(), newPos.z());
            }
            break;
        case FaceDirection::PosZ:
        case FaceDirection::NegZ:
            // X and Y must be within surface face bounds
            if (gridWorldPos.x < surfaceFaceMin.x) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(surfaceFaceMin.x, incrementPos.y() * Math::CoordinateConverter::CM_TO_METERS, incrementPos.z() * Math::CoordinateConverter::CM_TO_METERS));
                incrementPos = Math::IncrementCoordinates(newPos.x(), incrementPos.y(), incrementPos.z());
            } else if (gridWorldPos.x + placementVoxelSize > surfaceFaceMax.x) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(surfaceFaceMax.x - placementVoxelSize, incrementPos.y() * Math::CoordinateConverter::CM_TO_METERS, incrementPos.z() * Math::CoordinateConverter::CM_TO_METERS));
                incrementPos = Math::IncrementCoordinates(newPos.x(), incrementPos.y(), incrementPos.z());
            }
            if (gridWorldPos.y < surfaceFaceMin.y) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(incrementPos.x() * Math::CoordinateConverter::CM_TO_METERS, surfaceFaceMin.y, incrementPos.z() * Math::CoordinateConverter::CM_TO_METERS));
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), newPos.y(), incrementPos.z());
            } else if (gridWorldPos.y + placementVoxelSize > surfaceFaceMax.y) {
                Math::IncrementCoordinates newPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(incrementPos.x() * Math::CoordinateConverter::CM_TO_METERS, surfaceFaceMax.y - placementVoxelSize, incrementPos.z() * Math::CoordinateConverter::CM_TO_METERS));
                incrementPos = Math::IncrementCoordinates(incrementPos.x(), newPos.y(), incrementPos.z());
            }
            break;
    }
    
    return incrementPos;
}

// Simplified placement context - uses basic 1cm snapping per new requirements
PlacementContext PlacementUtils::getSmartPlacementContext(const Math::WorldCoordinates& worldPos,
                                                         VoxelData::VoxelResolution resolution,
                                                         bool shiftPressed,
                                                         const Math::Vector3f& workspaceSize,
                                                         const VoxelData::VoxelDataManager& dataManager,
                                                         const Math::IncrementCoordinates* surfaceFaceVoxelPos,
                                                         VoxelData::VoxelResolution surfaceFaceVoxelRes,
                                                         VoxelData::FaceDirection surfaceFaceDir) {
    // Mark unused parameters (kept for API compatibility)
    (void)dataManager;
    (void)surfaceFaceVoxelPos;
    (void)surfaceFaceVoxelRes;
    (void)surfaceFaceDir;
    
    PlacementContext context;
    context.worldPosition = worldPos;
    context.resolution = resolution;
    context.shiftPressed = shiftPressed;
    
    // SIMPLIFIED: Always use basic 1cm increment snapping regardless of context
    // This implements the new requirement that all voxels place at exact 1cm positions
    context.snappedIncrementPos = snapToValidIncrement(worldPos);
    
    // Validate the placement
    context.validation = validatePlacement(context.snappedIncrementPos, resolution, workspaceSize);
    
    return context;
}

bool PlacementUtils::isValidForIncrementPlacement(const Math::WorldCoordinates& worldPos, VoxelData::VoxelResolution resolution) {
    (void)resolution; // Mark as unused
    // Check if position is not NaN or infinite
    const Math::Vector3f& pos = worldPos.value();
    if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) {
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