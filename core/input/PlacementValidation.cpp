#include "PlacementValidation.h"
#include "../voxel_data/VoxelDataManager.h"
#include "voxel_math/VoxelBounds.h"
#include "voxel_math/VoxelCollision.h"
#include "voxel_math/WorkspaceValidation.h"
#include "voxel_math/VoxelPlacementMath.h"
#include <cmath>

namespace VoxelEditor {
namespace Input {

Math::IncrementCoordinates PlacementUtils::snapToValidIncrement(const Math::WorldCoordinates& worldPos) {
    // Delegate to voxel_math library
    return Math::VoxelPlacementMath::snapToValidIncrement(worldPos);
}

Math::IncrementCoordinates PlacementUtils::snapToGridAligned(const Math::WorldCoordinates& worldPos, 
                                                VoxelData::VoxelResolution resolution,
                                                bool shiftPressed) {
    // Delegate to voxel_math library
    return Math::VoxelPlacementMath::snapToGridAligned(worldPos, resolution, shiftPressed);
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
    // Delegate to voxel_math library
    return Math::VoxelPlacementMath::isValidIncrementPosition(pos);
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
                                                   VoxelData::VoxelResolution placementResolution,
                                                   bool shiftPressed) {
    // Delegate to voxel_math library
    // Allow overhangs for smaller voxels
    bool allowOverhang = (VoxelData::getVoxelSize(placementResolution) < VoxelData::getVoxelSize(surfaceFaceVoxelRes));
    
    return Math::VoxelPlacementMath::snapToSurfaceFaceGrid(
        hitPoint,
        surfaceFaceVoxelPos,
        surfaceFaceVoxelRes,
        surfaceFaceDir,
        placementResolution,
        allowOverhang,
        shiftPressed
    );
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
    
    PlacementContext context;
    context.worldPosition = worldPos;
    context.resolution = resolution;
    context.shiftPressed = shiftPressed;
    
    // Check if we're placing on a surface face
    if (surfaceFaceVoxelPos != nullptr) {
        // Validate that the hit point is within the bounds of the surface face
        bool withinBounds = Math::VoxelPlacementMath::isWithinFaceBounds(
            worldPos,
            *surfaceFaceVoxelPos,
            surfaceFaceVoxelRes,
            surfaceFaceDir
        );
        
        if (!withinBounds) {
            // Hit point is outside surface face bounds
            context.snappedIncrementPos = Math::IncrementCoordinates(0, 0, 0);
            context.validation = PlacementValidationResult::InvalidOutOfBounds;
            return context;
        }
    }
    
    // Use appropriate snapping based on context
    if (surfaceFaceVoxelPos != nullptr) {
        // Placing on a surface face - always use surface face grid snapping
        // This ensures alignment to the specific voxel face we clicked on
        context.snappedIncrementPos = snapToSurfaceFaceGrid(
            worldPos,
            *surfaceFaceVoxelPos,
            surfaceFaceVoxelRes,
            surfaceFaceDir,
            resolution,
            shiftPressed
        );
    } else {
        // Placing on ground or in open space - use basic 1cm increment snapping
        context.snappedIncrementPos = snapToValidIncrement(worldPos);
    }
    
    // Validate the placement
    context.validation = validatePlacement(context.snappedIncrementPos, resolution, workspaceSize);
    
    return context;
}

bool PlacementUtils::isValidForIncrementPlacement(const Math::WorldCoordinates& worldPos, VoxelData::VoxelResolution resolution) {
    (void)resolution; // Mark as unused
    // Delegate to voxel_math library
    return Math::VoxelPlacementMath::isValidForIncrementPlacement(worldPos);
}

}
}