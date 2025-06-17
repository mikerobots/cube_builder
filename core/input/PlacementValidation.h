#pragma once

#include <cmath>
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector3i.h"
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
    Math::Vector3f worldPosition;     // World position from ray cast
    Math::Vector3i snappedGridPos;    // Snapped grid position (1cm increments)
    VoxelData::VoxelResolution resolution;  // Current voxel resolution
    bool shiftPressed;                // Shift key modifier state
    PlacementValidationResult validation; // Validation result
    
    PlacementContext() 
        : worldPosition(0.0f, 0.0f, 0.0f)
        , snappedGridPos(0, 0, 0)
        , resolution(VoxelData::VoxelResolution::Size_1cm)
        , shiftPressed(false)
        , validation(PlacementValidationResult::Valid) {}
};

// Utility functions for placement snapping and validation
class PlacementUtils {
public:
    // Snap a world position to the nearest 1cm increment
    static Math::Vector3i snapToValidIncrement(const Math::Vector3f& worldPos);
    
    // Snap to grid-aligned position for same-size voxels (unless shift is pressed)
    static Math::Vector3i snapToGridAligned(const Math::Vector3f& worldPos, 
                                           VoxelData::VoxelResolution resolution,
                                           bool shiftPressed);
    
    // Validate if a position is valid for placement
    static PlacementValidationResult validatePlacement(const Math::Vector3i& gridPos,
                                                      VoxelData::VoxelResolution resolution,
                                                      const Math::Vector3f& workspaceSize);
    
    // Check if position is a valid 1cm increment
    static bool isValidIncrementPosition(const Math::Vector3i& pos);
    
    // Convert world position to 1cm grid coordinates
    static Math::Vector3i worldToIncrementGrid(const Math::Vector3f& worldPos);
    
    // Convert 1cm grid coordinates to world position
    static Math::Vector3f incrementGridToWorld(const Math::Vector3i& gridPos);
    
    // Get the placement context from a world position
    static PlacementContext getPlacementContext(const Math::Vector3f& worldPos,
                                               VoxelData::VoxelResolution resolution,
                                               bool shiftPressed,
                                               const Math::Vector3f& workspaceSize);
    
    // Phase 3 Enhancement: Smart snapping for same-size voxels
    static Math::Vector3i snapToSameSizeVoxel(const Math::Vector3f& worldPos,
                                             VoxelData::VoxelResolution resolution,
                                             const VoxelData::VoxelDataManager& dataManager,
                                             bool shiftPressed);
    
    // Phase 3 Enhancement: Sub-grid positioning for smaller voxels on larger Surface Faces
    static Math::Vector3i snapToSurfaceFaceGrid(const Math::Vector3f& hitPoint,
                                               const Math::Vector3i& surfaceFaceVoxelPos,
                                               VoxelData::VoxelResolution surfaceFaceVoxelRes,
                                               VoxelData::FaceDirection surfaceFaceDir,
                                               VoxelData::VoxelResolution placementResolution);
    
    // Phase 3 Enhancement: Smart placement context with voxel data awareness
    static PlacementContext getSmartPlacementContext(const Math::Vector3f& worldPos,
                                                    VoxelData::VoxelResolution resolution,
                                                    bool shiftPressed,
                                                    const Math::Vector3f& workspaceSize,
                                                    const VoxelData::VoxelDataManager& dataManager,
                                                    const Math::Vector3i* surfaceFaceVoxelPos = nullptr,
                                                    VoxelData::VoxelResolution surfaceFaceVoxelRes = VoxelData::VoxelResolution::Size_1cm,
                                                    VoxelData::FaceDirection surfaceFaceDir = VoxelData::FaceDirection::PosY);
    
    // Validate if a world position is valid for increment placement
    static bool isValidForIncrementPlacement(const Math::Vector3f& worldPos, VoxelData::VoxelResolution resolution);
    
private:
    // 1cm in world units
    static constexpr float INCREMENT_SIZE = 0.01f;
};

}
}