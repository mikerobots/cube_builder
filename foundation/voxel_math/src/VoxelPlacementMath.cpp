#include "voxel_math/VoxelPlacementMath.h"
#include <cmath>
#include <algorithm>

namespace VoxelEditor {
namespace Math {

IncrementCoordinates VoxelPlacementMath::snapToValidIncrement(const WorldCoordinates& worldPos) {
    // Use centralized coordinate converter for consistent snapping
    return CoordinateConverter::worldToIncrement(worldPos);
}

IncrementCoordinates VoxelPlacementMath::snapToGridAligned(const WorldCoordinates& worldPos,
                                                           VoxelData::VoxelResolution resolution,
                                                           bool shiftPressed) {
    // If shift is pressed, use 1cm increments
    if (shiftPressed) {
        return snapToValidIncrement(worldPos);
    }
    
    // Otherwise, snap to the voxel resolution grid
    float voxelSize = VoxelData::getVoxelSize(resolution);
    int voxelSizeIncrements = static_cast<int>(voxelSize * CoordinateConverter::METERS_TO_CM);
    
    // Convert to increment coordinates
    IncrementCoordinates baseIncrement = CoordinateConverter::worldToIncrement(worldPos);
    
    // For bottom-center coordinates, we need to align voxel centers, not bottoms
    // This means we need to offset by half voxel size when snapping
    int halfVoxelIncrements = voxelSizeIncrements / 2;
    
    // Snap X and Z to voxel grid (centered)
    int snappedX = ((baseIncrement.x() + halfVoxelIncrements) / voxelSizeIncrements) * voxelSizeIncrements;
    int snappedZ = ((baseIncrement.z() + halfVoxelIncrements) / voxelSizeIncrements) * voxelSizeIncrements;
    
    // For Y, just snap to voxel size increments (bottom alignment)
    int snappedY = (baseIncrement.y() / voxelSizeIncrements) * voxelSizeIncrements;
    
    return IncrementCoordinates(snappedX, snappedY, snappedZ);
}

IncrementCoordinates VoxelPlacementMath::snapToSurfaceFaceGrid(const WorldCoordinates& hitPoint,
                                                               const IncrementCoordinates& surfaceFaceVoxelPos,
                                                               VoxelData::VoxelResolution surfaceFaceVoxelRes,
                                                               VoxelData::FaceDirection surfaceFaceDir,
                                                               VoxelData::VoxelResolution placementResolution,
                                                               bool allowOverhang,
                                                               bool shiftPressed) {
    using namespace VoxelData;
    
    float surfaceFaceVoxelSize = getVoxelSize(surfaceFaceVoxelRes);
    float placementVoxelSize = getVoxelSize(placementResolution);
    
    // Convert surface face voxel position to world coordinates
    WorldCoordinates surfaceFaceWorldCoords = CoordinateConverter::incrementToWorld(surfaceFaceVoxelPos);
    Vector3f surfaceFaceWorldPos = surfaceFaceWorldCoords.value();
    
    // Calculate the surface face plane position
    Vector3f surfaceFacePlanePos = surfaceFaceWorldPos;
    switch (surfaceFaceDir) {
        case FaceDirection::PosX:
            surfaceFacePlanePos.x += surfaceFaceVoxelSize * 0.5f;  // Right face
            break;
        case FaceDirection::NegX:
            surfaceFacePlanePos.x -= surfaceFaceVoxelSize * 0.5f;  // Left face
            break;
        case FaceDirection::PosY:
            surfaceFacePlanePos.y += surfaceFaceVoxelSize;  // Top face
            break;
        case FaceDirection::NegY:
            // Bottom face - plane is at the voxel's Y position
            break;
        case FaceDirection::PosZ:
            surfaceFacePlanePos.z += surfaceFaceVoxelSize * 0.5f;  // Back face
            break;
        case FaceDirection::NegZ:
            surfaceFacePlanePos.z -= surfaceFaceVoxelSize * 0.5f;  // Front face
            break;
    }
    
    // For same-size voxels without shift, we want perfect alignment
    if (surfaceFaceVoxelRes == placementResolution && !shiftPressed) {
        // Calculate the adjacent position for same-size voxel
        IncrementCoordinates adjacentPos = surfaceFaceVoxelPos;
        
        // Offset by the voxel size in the face normal direction
        int voxelSizeIncrements = static_cast<int>(surfaceFaceVoxelSize * CoordinateConverter::METERS_TO_CM);
        
        switch (surfaceFaceDir) {
            case FaceDirection::PosX:
                adjacentPos = IncrementCoordinates(adjacentPos.x() + voxelSizeIncrements, adjacentPos.y(), adjacentPos.z());
                break;
            case FaceDirection::NegX:
                adjacentPos = IncrementCoordinates(adjacentPos.x() - voxelSizeIncrements, adjacentPos.y(), adjacentPos.z());
                break;
            case FaceDirection::PosY:
                adjacentPos = IncrementCoordinates(adjacentPos.x(), adjacentPos.y() + voxelSizeIncrements, adjacentPos.z());
                break;
            case FaceDirection::NegY:
                adjacentPos = IncrementCoordinates(adjacentPos.x(), adjacentPos.y() - voxelSizeIncrements, adjacentPos.z());
                break;
            case FaceDirection::PosZ:
                adjacentPos = IncrementCoordinates(adjacentPos.x(), adjacentPos.y(), adjacentPos.z() + voxelSizeIncrements);
                break;
            case FaceDirection::NegZ:
                adjacentPos = IncrementCoordinates(adjacentPos.x(), adjacentPos.y(), adjacentPos.z() - voxelSizeIncrements);
                break;
        }
        
        return adjacentPos;
    }
    
    // Project the hit point onto the surface face plane
    Vector3f snappedPos = hitPoint.value();
    
    // Constrain the position to the surface face plane and adjust for placement voxel size
    // In bottom-center coordinates, we need to offset by the placement voxel's half-size
    // for side faces to ensure the voxel is placed adjacent to the face
    float halfPlacementSize = placementVoxelSize * 0.5f;
    
    switch (surfaceFaceDir) {
        case FaceDirection::PosX:
            snappedPos.x = surfaceFacePlanePos.x + halfPlacementSize;  // Offset right
            break;
        case FaceDirection::NegX:
            snappedPos.x = surfaceFacePlanePos.x - halfPlacementSize;  // Offset left
            break;
        case FaceDirection::PosY:
            snappedPos.y = surfaceFacePlanePos.y;  // No offset needed (bottom-aligned)
            break;
        case FaceDirection::NegY:
            snappedPos.y = surfaceFacePlanePos.y - placementVoxelSize;  // Offset down by full size
            break;
        case FaceDirection::PosZ:
            snappedPos.z = surfaceFacePlanePos.z + halfPlacementSize;  // Offset back
            break;
        case FaceDirection::NegZ:
            snappedPos.z = surfaceFacePlanePos.z - halfPlacementSize;  // Offset front
            break;
    }
    
    // Snap to 1cm increments
    IncrementCoordinates incrementPos = snapToValidIncrement(WorldCoordinates(snappedPos));
    
    // If overhang is not allowed and placement voxel is same size or larger, clamp to bounds
    if (!allowOverhang && placementVoxelSize >= surfaceFaceVoxelSize) {
        WorldCoordinates gridWorldCoords = CoordinateConverter::incrementToWorld(incrementPos);
        Vector3f gridWorldPos = gridWorldCoords.value();
        
        // Calculate surface voxel bounds (bottom-center coordinate system)
        float halfSize = surfaceFaceVoxelSize * 0.5f;
        Vector3f surfaceFaceMin(
            surfaceFaceWorldPos.x - halfSize,
            surfaceFaceWorldPos.y,
            surfaceFaceWorldPos.z - halfSize
        );
        Vector3f surfaceFaceMax(
            surfaceFaceWorldPos.x + halfSize,
            surfaceFaceWorldPos.y + surfaceFaceVoxelSize,
            surfaceFaceWorldPos.z + halfSize
        );
        
        // Clamp to surface face bounds (except in the surface face normal direction)
        float halfPlacementSize = placementVoxelSize * 0.5f;
        
        switch (surfaceFaceDir) {
            case FaceDirection::PosX:
            case FaceDirection::NegX:
                // Y and Z must be within surface face bounds
                if (gridWorldPos.y < surfaceFaceMin.y) {
                    incrementPos = IncrementCoordinates(
                        incrementPos.x(),
                        static_cast<int>(surfaceFaceMin.y * CoordinateConverter::METERS_TO_CM),
                        incrementPos.z()
                    );
                } else if (gridWorldPos.y + placementVoxelSize > surfaceFaceMax.y) {
                    incrementPos = IncrementCoordinates(
                        incrementPos.x(),
                        static_cast<int>((surfaceFaceMax.y - placementVoxelSize) * CoordinateConverter::METERS_TO_CM),
                        incrementPos.z()
                    );
                }
                
                if (gridWorldPos.z - halfPlacementSize < surfaceFaceMin.z) {
                    incrementPos = IncrementCoordinates(
                        incrementPos.x(),
                        incrementPos.y(),
                        static_cast<int>((surfaceFaceMin.z + halfPlacementSize) * CoordinateConverter::METERS_TO_CM)
                    );
                } else if (gridWorldPos.z + halfPlacementSize > surfaceFaceMax.z) {
                    incrementPos = IncrementCoordinates(
                        incrementPos.x(),
                        incrementPos.y(),
                        static_cast<int>((surfaceFaceMax.z - halfPlacementSize) * CoordinateConverter::METERS_TO_CM)
                    );
                }
                break;
                
            case FaceDirection::PosY:
            case FaceDirection::NegY:
                // X and Z must be within surface face bounds
                if (gridWorldPos.x - halfPlacementSize < surfaceFaceMin.x) {
                    incrementPos = IncrementCoordinates(
                        static_cast<int>((surfaceFaceMin.x + halfPlacementSize) * CoordinateConverter::METERS_TO_CM),
                        incrementPos.y(),
                        incrementPos.z()
                    );
                } else if (gridWorldPos.x + halfPlacementSize > surfaceFaceMax.x) {
                    incrementPos = IncrementCoordinates(
                        static_cast<int>((surfaceFaceMax.x - halfPlacementSize) * CoordinateConverter::METERS_TO_CM),
                        incrementPos.y(),
                        incrementPos.z()
                    );
                }
                
                if (gridWorldPos.z - halfPlacementSize < surfaceFaceMin.z) {
                    incrementPos = IncrementCoordinates(
                        incrementPos.x(),
                        incrementPos.y(),
                        static_cast<int>((surfaceFaceMin.z + halfPlacementSize) * CoordinateConverter::METERS_TO_CM)
                    );
                } else if (gridWorldPos.z + halfPlacementSize > surfaceFaceMax.z) {
                    incrementPos = IncrementCoordinates(
                        incrementPos.x(),
                        incrementPos.y(),
                        static_cast<int>((surfaceFaceMax.z - halfPlacementSize) * CoordinateConverter::METERS_TO_CM)
                    );
                }
                break;
                
            case FaceDirection::PosZ:
            case FaceDirection::NegZ:
                // X and Y must be within surface face bounds
                if (gridWorldPos.x - halfPlacementSize < surfaceFaceMin.x) {
                    incrementPos = IncrementCoordinates(
                        static_cast<int>((surfaceFaceMin.x + halfPlacementSize) * CoordinateConverter::METERS_TO_CM),
                        incrementPos.y(),
                        incrementPos.z()
                    );
                } else if (gridWorldPos.x + halfPlacementSize > surfaceFaceMax.x) {
                    incrementPos = IncrementCoordinates(
                        static_cast<int>((surfaceFaceMax.x - halfPlacementSize) * CoordinateConverter::METERS_TO_CM),
                        incrementPos.y(),
                        incrementPos.z()
                    );
                }
                
                if (gridWorldPos.y < surfaceFaceMin.y) {
                    incrementPos = IncrementCoordinates(
                        incrementPos.x(),
                        static_cast<int>(surfaceFaceMin.y * CoordinateConverter::METERS_TO_CM),
                        incrementPos.z()
                    );
                } else if (gridWorldPos.y + placementVoxelSize > surfaceFaceMax.y) {
                    incrementPos = IncrementCoordinates(
                        incrementPos.x(),
                        static_cast<int>((surfaceFaceMax.y - placementVoxelSize) * CoordinateConverter::METERS_TO_CM),
                        incrementPos.z()
                    );
                }
                break;
        }
    }
    
    return incrementPos;
}

bool VoxelPlacementMath::isValidIncrementPosition(const IncrementCoordinates& pos) {
    // Check Y >= 0 constraint (no voxels below ground)
    return pos.y() >= 0;
}

bool VoxelPlacementMath::isValidForIncrementPlacement(const WorldCoordinates& worldPos) {
    const Vector3f& pos = worldPos.value();
    
    // Check if position is not NaN or infinite
    if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) {
        return false;
    }
    
    // Check for extreme values that would overflow increment coordinates
    if (std::abs(pos.x) > MAX_INCREMENT_COORD * CoordinateConverter::CM_TO_METERS ||
        std::abs(pos.y) > MAX_INCREMENT_COORD * CoordinateConverter::CM_TO_METERS ||
        std::abs(pos.z) > MAX_INCREMENT_COORD * CoordinateConverter::CM_TO_METERS) {
        return false;
    }
    
    return true;
}

void VoxelPlacementMath::calculateVoxelWorldBounds(const IncrementCoordinates& incrementPos,
                                                   VoxelData::VoxelResolution resolution,
                                                   Vector3f& minCorner,
                                                   Vector3f& maxCorner) {
    // Convert increment position to world coordinates
    WorldCoordinates worldPos = CoordinateConverter::incrementToWorld(incrementPos);
    Vector3f centerBottom = worldPos.value();
    
    // Get voxel size
    float voxelSize = VoxelData::getVoxelSize(resolution);
    float halfSize = voxelSize * 0.5f;
    
    // Calculate bounds (bottom-center coordinate system)
    minCorner = Vector3f(
        centerBottom.x - halfSize,
        centerBottom.y,
        centerBottom.z - halfSize
    );
    
    maxCorner = Vector3f(
        centerBottom.x + halfSize,
        centerBottom.y + voxelSize,
        centerBottom.z + halfSize
    );
}

bool VoxelPlacementMath::isWithinFaceBounds(const WorldCoordinates& placementPos,
                                            const IncrementCoordinates& surfaceVoxelPos,
                                            VoxelData::VoxelResolution surfaceResolution,
                                            VoxelData::FaceDirection faceDir,
                                            float epsilon) {
    // Calculate surface voxel bounds
    Vector3f surfaceMin, surfaceMax;
    calculateVoxelWorldBounds(surfaceVoxelPos, surfaceResolution, surfaceMin, surfaceMax);
    
    const Vector3f& hitPos = placementPos.value();
    
    // Check bounds based on which face we're placing on
    switch (faceDir) {
        case VoxelData::FaceDirection::PosY:
        case VoxelData::FaceDirection::NegY:
            // For top/bottom faces, check XZ bounds
            return (hitPos.x >= surfaceMin.x - epsilon && hitPos.x <= surfaceMax.x + epsilon) &&
                   (hitPos.z >= surfaceMin.z - epsilon && hitPos.z <= surfaceMax.z + epsilon);
            
        case VoxelData::FaceDirection::PosX:
        case VoxelData::FaceDirection::NegX:
            // For left/right faces, check YZ bounds
            return (hitPos.y >= surfaceMin.y - epsilon && hitPos.y <= surfaceMax.y + epsilon) &&
                   (hitPos.z >= surfaceMin.z - epsilon && hitPos.z <= surfaceMax.z + epsilon);
            
        case VoxelData::FaceDirection::PosZ:
        case VoxelData::FaceDirection::NegZ:
            // For front/back faces, check XY bounds
            return (hitPos.x >= surfaceMin.x - epsilon && hitPos.x <= surfaceMax.x + epsilon) &&
                   (hitPos.y >= surfaceMin.y - epsilon && hitPos.y <= surfaceMax.y + epsilon);
    }
    
    return false;
}

} // namespace Math
} // namespace VoxelEditor