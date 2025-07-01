#pragma once

#include <cmath>
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include "../voxel_data/VoxelTypes.h"

// Forward declarations for Phase 3 enhancements
namespace VoxelEditor {
    namespace VoxelData {
        class VoxelDataManager;
    }
    namespace VisualFeedback {
        class Face;
    }
}

namespace VoxelEditor {
namespace Input {

// Result of placement validation
enum class PlacementValidationResult {
    Valid,              // Placement is valid
    InvalidYBelowZero,  // Y coordinate is below ground (< 0)
    InvalidOverlap,     // Would overlap with existing voxel
    InvalidOutOfBounds, // Outside workspace bounds
    InvalidPosition     // Invalid position (NaN, inf, etc.)
};

// Placement context information
struct PlacementContext {
    Math::WorldCoordinates worldPosition;     // World position from ray cast
    Math::IncrementCoordinates snappedIncrementPos;    // Snapped increment position (1cm increments)
    VoxelData::VoxelResolution resolution;  // Current voxel resolution
    bool shiftPressed;                // Shift key modifier state
    PlacementValidationResult validation; // Validation result
    
    PlacementContext() 
        : worldPosition(Math::WorldCoordinates(0.0f, 0.0f, 0.0f))
        , snappedIncrementPos(0, 0, 0)
        , resolution(VoxelData::VoxelResolution::Size_1cm)
        , shiftPressed(false)
        , validation(PlacementValidationResult::Valid) {}
};

// Utility functions for placement snapping and validation
class PlacementUtils {
public:
    // Snap a world position to the nearest 1cm increment
    static Math::IncrementCoordinates snapToValidIncrement(const Math::WorldCoordinates& worldPos);
    
    // Snap to 1cm increment grid (resolution and shift parameters maintained for compatibility)
    // NOTE: With new requirements, all voxels place at exact 1cm positions without resolution-based snapping
    static Math::IncrementCoordinates snapToGridAligned(const Math::WorldCoordinates& worldPos, 
                                           VoxelData::VoxelResolution resolution,
                                           bool shiftPressed);
    
    // Validate if a position is valid for placement
    static PlacementValidationResult validatePlacement(const Math::IncrementCoordinates& incrementPos,
                                                      VoxelData::VoxelResolution resolution,
                                                      const Math::Vector3f& workspaceSize);
    
    // Check if position is a valid 1cm increment
    static bool isValidIncrementPosition(const Math::IncrementCoordinates& pos);
    
    // Note: These methods replaced by CoordinateConverter
    // Use CoordinateConverter::worldToIncrement() and incrementToWorld() instead
    
    // Get the placement context from a world position
    static PlacementContext getPlacementContext(const Math::WorldCoordinates& worldPos,
                                               VoxelData::VoxelResolution resolution,
                                               bool shiftPressed,
                                               const Math::Vector3f& workspaceSize);
    
    // Phase 3 Enhancement: Smart snapping for same-size voxels
    static Math::IncrementCoordinates snapToSameSizeVoxel(const Math::WorldCoordinates& worldPos,
                                             VoxelData::VoxelResolution resolution,
                                             const VoxelData::VoxelDataManager& dataManager,
                                             bool shiftPressed);
    
    // Phase 3 Enhancement: Sub-grid positioning for smaller voxels on larger Surface Faces
    static Math::IncrementCoordinates snapToSurfaceFaceGrid(const Math::WorldCoordinates& hitPoint,
                                               const Math::IncrementCoordinates& surfaceFaceVoxelPos,
                                               VoxelData::VoxelResolution surfaceFaceVoxelRes,
                                               VoxelData::FaceDirection surfaceFaceDir,
                                               VoxelData::VoxelResolution placementResolution);
    
    // Phase 3 Enhancement: Smart placement context with voxel data awareness
    static PlacementContext getSmartPlacementContext(const Math::WorldCoordinates& worldPos,
                                                    VoxelData::VoxelResolution resolution,
                                                    bool shiftPressed,
                                                    const Math::Vector3f& workspaceSize,
                                                    const VoxelData::VoxelDataManager& dataManager,
                                                    const Math::IncrementCoordinates* surfaceFaceVoxelPos = nullptr,
                                                    VoxelData::VoxelResolution surfaceFaceVoxelRes = VoxelData::VoxelResolution::Size_1cm,
                                                    VoxelData::FaceDirection surfaceFaceDir = VoxelData::FaceDirection::PosY);
    
    // Validate if a world position is valid for increment placement
    static bool isValidForIncrementPlacement(const Math::WorldCoordinates& worldPos, VoxelData::VoxelResolution resolution);
};

}
}