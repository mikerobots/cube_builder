#include "voxel_math/VoxelPlacementMath.h"
#include <cmath>
#include <algorithm>

namespace VoxelEditor {
namespace Math {

IncrementCoordinates VoxelPlacementMath::snapToValidIncrement(const WorldCoordinates& worldPos) {
    // Use centralized coordinate converter for consistent snapping
    return CoordinateConverter::worldToIncrement(worldPos);
}

/**
 * Snaps a world position to valid voxel placement position
 * @param worldPos - Position to snap to grid (no center-to-corner conversion)
 * @param resolution - voxel size to place
 * @param shiftPressed - true for 1cm increments, false for resolution-based grid
 * @return IncrementCoordinates - snapped position
 * 
 * Without shift: Snaps to resolution-based grid (e.g., 4cm voxels snap to 4cm grid)
 * With shift: Snaps to 1cm increment grid
 */
IncrementCoordinates VoxelPlacementMath::snapToGridAligned(const WorldCoordinates& worldPos,
                                                           VoxelData::VoxelResolution resolution,
                                                           bool shiftPressed) {
    // If shift is pressed, use 1cm increments
    if (shiftPressed) {
        // With shift, snap directly to 1cm increments without any offset
        return snapToValidIncrement(worldPos);
    }
    
    // Without shift: snap directly to the voxel resolution grid without center-to-corner conversion
    float voxelSize = VoxelData::getVoxelSize(resolution);
    int voxelSizeIncrements = static_cast<int>(voxelSize * CoordinateConverter::METERS_TO_CM);
    
    // Convert to increment coordinates
    IncrementCoordinates baseIncrement = CoordinateConverter::worldToIncrement(worldPos);
    
    // Snap to the nearest voxel size grid point
    // For Y axis (vertical), use floor to snap to bottom of grid cell (voxels are bottom-aligned)
    // For X and Z axes (horizontal), use round to snap to nearest grid point
    int snappedX = static_cast<int>(std::round(static_cast<float>(baseIncrement.x()) / voxelSizeIncrements)) * voxelSizeIncrements;
    int snappedY = static_cast<int>(std::floor(static_cast<float>(baseIncrement.y()) / voxelSizeIncrements)) * voxelSizeIncrements;
    int snappedZ = static_cast<int>(std::round(static_cast<float>(baseIncrement.z()) / voxelSizeIncrements)) * voxelSizeIncrements;
    
    return IncrementCoordinates(snappedX, snappedY, snappedZ);
}

/**
 * Snaps placement position when placing on an existing voxel face
 * @param hitPoint - ray hit position on face (center-bottom of desired placement)
 * @param surfaceFaceVoxelPos - position of voxel being placed on
 * @param surfaceFaceVoxelRes - resolution of voxel being placed on
 * @param surfaceFaceDir - which face (PosX, NegX, PosY, etc.)
 * @param placementResolution - resolution of voxel being placed
 * @param allowOverhang - whether placement can extend beyond face bounds
 * @param shiftPressed - true for 1cm increments with overhang, false for grid snap
 * @return IncrementCoordinates - bottom-left-back corner of new voxel
 *
 * Same-size voxels without shift: Fixed position adjacent to face (ignores hitPoint)
 * Different-size without shift: Snaps to placement voxel's grid, no overhang
 * With shift: 1cm increments, cursor-centered, overhang allowed
 */
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
    
    // If shift is pressed, use 1cm increments with overhangs allowed
    if (shiftPressed) {
        // Keep existing 1cm snapping behavior - jump to end of function
    } else {
        // Default behavior: snap to placement voxel's grid, no overhangs
        
        // Special case for same-size voxels: align directly with the existing voxel
        if (placementVoxelSize == surfaceFaceVoxelSize) {
            // For same-size voxels, we want perfect alignment
            // Use the existing voxel's position to ensure alignment
            
            // For now, simply place at the adjacent position
            // This ensures perfect alignment without any mouse-based offset
            Vector3f alignedPos = surfaceFaceWorldPos;
            
            // Adjust position based on face direction
            switch (surfaceFaceDir) {
                case FaceDirection::PosX:
                    alignedPos.x += surfaceFaceVoxelSize;
                    break;
                case FaceDirection::NegX:
                    alignedPos.x -= surfaceFaceVoxelSize;
                    break;
                case FaceDirection::PosY:
                    alignedPos.y += surfaceFaceVoxelSize;
                    break;
                case FaceDirection::NegY:
                    alignedPos.y -= surfaceFaceVoxelSize;
                    break;
                case FaceDirection::PosZ:
                    alignedPos.z += surfaceFaceVoxelSize;
                    break;
                case FaceDirection::NegZ:
                    alignedPos.z -= surfaceFaceVoxelSize;
                    break;
            }
            
            // Convert to increment coordinates
            return CoordinateConverter::worldToIncrement(WorldCoordinates(alignedPos));
        }
        
        // For different-size voxels, use grid alignment from face corner
        // Calculate the grid origin (bottom-left corner of the face)
        Vector3f gridOrigin = calculateFaceGridOrigin(surfaceFaceVoxelPos, surfaceFaceVoxelRes, surfaceFaceDir);
        
        // Get placement voxel grid size
        int placementGridIncrements = static_cast<int>(placementVoxelSize * CoordinateConverter::METERS_TO_CM);
        float halfPlacementSize = placementVoxelSize * 0.5f;
        
        // For smaller voxels, this creates a grid aligned to the face corner
        
        // Project hit point to face plane first
        Vector3f projectedHit = hitPoint.value();
        switch (surfaceFaceDir) {
            case FaceDirection::PosX:
            case FaceDirection::NegX:
                projectedHit.x = surfaceFacePlanePos.x;
                break;
            case FaceDirection::PosY:
            case FaceDirection::NegY:
                projectedHit.y = surfaceFacePlanePos.y;
                break;
            case FaceDirection::PosZ:
            case FaceDirection::NegZ:
                projectedHit.z = surfaceFacePlanePos.z;
                break;
        }
        
        // Calculate offset from grid origin
        Vector3f offset = projectedHit - gridOrigin;
        
        // Snap to placement voxel grid
        int gridX = static_cast<int>(std::round(offset.x * CoordinateConverter::METERS_TO_CM / placementGridIncrements));
        int gridY = static_cast<int>(std::round(offset.y * CoordinateConverter::METERS_TO_CM / placementGridIncrements));
        int gridZ = static_cast<int>(std::round(offset.z * CoordinateConverter::METERS_TO_CM / placementGridIncrements));
        
        // Calculate snapped world position
        Vector3f snappedWorldPos = gridOrigin;
        snappedWorldPos.x += gridX * placementGridIncrements * CoordinateConverter::CM_TO_METERS;
        snappedWorldPos.y += gridY * placementGridIncrements * CoordinateConverter::CM_TO_METERS;
        snappedWorldPos.z += gridZ * placementGridIncrements * CoordinateConverter::CM_TO_METERS;
        
        // For better visual alignment of smaller voxels on larger faces,
        // offset the grid by half the placement voxel size
        // This centers the grid on the face rather than starting at the corner
        if (placementVoxelSize < surfaceFaceVoxelSize) {
            float gridOffset = halfPlacementSize;
            switch (surfaceFaceDir) {
                case FaceDirection::PosY:
                case FaceDirection::NegY:
                    // Offset X and Z for top/bottom faces
                    snappedWorldPos.x += gridOffset;
                    snappedWorldPos.z += gridOffset;
                    break;
                case FaceDirection::PosX:
                case FaceDirection::NegX:
                    // For side faces, only offset Z (not Y, since voxels are bottom-aligned)
                    snappedWorldPos.z += gridOffset;
                    break;
                case FaceDirection::PosZ:
                case FaceDirection::NegZ:
                    // For front/back faces, only offset X (not Y, since voxels are bottom-aligned)
                    snappedWorldPos.x += gridOffset;
                    break;
            }
        }
        
        // Account for placement voxel offset (same as before)
        // halfPlacementSize already calculated above
        switch (surfaceFaceDir) {
            case FaceDirection::PosX:
                snappedWorldPos.x += halfPlacementSize;
                break;
            case FaceDirection::NegX:
                snappedWorldPos.x -= halfPlacementSize;
                break;
            case FaceDirection::PosY:
                // No offset needed for top face
                break;
            case FaceDirection::NegY:
                snappedWorldPos.y -= placementVoxelSize;
                break;
            case FaceDirection::PosZ:
                snappedWorldPos.z += halfPlacementSize;
                break;
            case FaceDirection::NegZ:
                snappedWorldPos.z -= halfPlacementSize;
                break;
        }
        
        // Convert to increment coordinates
        IncrementCoordinates snappedIncrement = CoordinateConverter::worldToIncrement(WorldCoordinates(snappedWorldPos));
        
        // Check if placement fits within face bounds (no overhangs)
        Vector3f minBounds, maxBounds;
        calculateVoxelWorldBounds(surfaceFaceVoxelPos, surfaceFaceVoxelRes, minBounds, maxBounds);
        
        // Get placement voxel bounds
        Vector3f placementMin, placementMax;
        calculateVoxelWorldBounds(snappedIncrement, placementResolution, placementMin, placementMax);
        
        // Check each axis based on face direction (don't constrain in face normal direction)
        bool needsClamping = false;
        switch (surfaceFaceDir) {
            case FaceDirection::PosX:
            case FaceDirection::NegX:
                // Check Y and Z bounds
                if (placementMin.y < minBounds.y || placementMax.y > maxBounds.y ||
                    placementMin.z < minBounds.z || placementMax.z > maxBounds.z) {
                    needsClamping = true;
                }
                break;
            case FaceDirection::PosY:
            case FaceDirection::NegY:
                // Check X and Z bounds
                if (placementMin.x < minBounds.x || placementMax.x > maxBounds.x ||
                    placementMin.z < minBounds.z || placementMax.z > maxBounds.z) {
                    needsClamping = true;
                }
                break;
            case FaceDirection::PosZ:
            case FaceDirection::NegZ:
                // Check X and Y bounds
                if (placementMin.x < minBounds.x || placementMax.x > maxBounds.x ||
                    placementMin.y < minBounds.y || placementMax.y > maxBounds.y) {
                    needsClamping = true;
                }
                break;
        }
        
        if (needsClamping) {
            // Find the closest valid grid position that fits within bounds
            // This is complex, so for now just return an invalid position
            // In practice, the preview system should show this as red/invalid
            return IncrementCoordinates(snappedIncrement.x(), snappedIncrement.y(), snappedIncrement.z());
        }
        
        return snappedIncrement;
    }
    
    // With shift key: cursor-centered placement with 1cm increments
    // The hit point represents where we want the voxel to be centered based on the face
    
    // First, project the hit point to the surface face plane
    Vector3f projectedHit = hitPoint.value();
    switch (surfaceFaceDir) {
        case FaceDirection::PosX:
        case FaceDirection::NegX:
            projectedHit.x = surfaceFacePlanePos.x;
            break;
        case FaceDirection::PosY:
        case FaceDirection::NegY:
            projectedHit.y = surfaceFacePlanePos.y;
            break;
        case FaceDirection::PosZ:
        case FaceDirection::NegZ:
            projectedHit.z = surfaceFacePlanePos.z;
            break;
    }
    
    // Apply face-specific offset to position voxel adjacent to face
    Vector3f offset = calculateFacePlacementOffset(surfaceFaceDir, placementVoxelSize);
    projectedHit += offset;
    
    // For shift key placement, handle cursor-centered placement based on face type
    Vector3f bottomLeftBack;
    float halfPlacementSize = placementVoxelSize * 0.5f;
    
    switch (surfaceFaceDir) {
        case FaceDirection::PosY:
        case FaceDirection::NegY:
            // Top/bottom faces: cursor represents center-bottom of voxel
            bottomLeftBack = Vector3f(
                projectedHit.x - halfPlacementSize,
                projectedHit.y,  // Y stays as-is (bottom-aligned)
                projectedHit.z - halfPlacementSize
            );
            break;
            
        case FaceDirection::PosX:
        case FaceDirection::NegX:
        case FaceDirection::PosZ:
        case FaceDirection::NegZ:
            // Side faces: cursor represents center of the voxel in all dimensions
            bottomLeftBack = Vector3f(
                projectedHit.x - halfPlacementSize,
                projectedHit.y - halfPlacementSize,  // Center Y on cursor
                projectedHit.z - halfPlacementSize
            );
            break;
    }
    
    // Snap to 1cm increments
    IncrementCoordinates incrementPos = snapToValidIncrement(WorldCoordinates(bottomLeftBack));
    
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
    Vector3f bottomLeftBack = worldPos.value();  // This is the bottom-left-back corner
    
    // Get voxel size
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Calculate bounds (bottom-left-back corner coordinate system)
    // The voxel extends from bottomLeftBack to bottomLeftBack + voxelSize in all dimensions
    minCorner = bottomLeftBack;
    
    maxCorner = Vector3f(
        bottomLeftBack.x + voxelSize,
        bottomLeftBack.y + voxelSize,
        bottomLeftBack.z + voxelSize
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

Vector3f VoxelPlacementMath::calculateFaceGridOrigin(const IncrementCoordinates& surfaceVoxelPos,
                                                     VoxelData::VoxelResolution surfaceResolution,
                                                     VoxelData::FaceDirection faceDir) {
    // Get voxel world position and size
    WorldCoordinates worldPos = CoordinateConverter::incrementToWorld(surfaceVoxelPos);
    float voxelSize = VoxelData::getVoxelSize(surfaceResolution);
    float halfSize = voxelSize * 0.5f;
    
    Vector3f origin = worldPos.value();
    
    // Calculate bottom-left corner based on face direction
    // In bottom-center coordinate system:
    // - Voxel extends from -halfSize to +halfSize in X and Z
    // - Voxel extends from 0 to voxelSize in Y
    
    switch (faceDir) {
        case VoxelData::FaceDirection::PosY:  // Top face
            // Bottom-left is at minimum X and Z
            origin.x -= halfSize;
            origin.y += voxelSize;  // Top of voxel
            origin.z -= halfSize;
            break;
            
        case VoxelData::FaceDirection::NegY:  // Bottom face
            // Bottom-left is at minimum X and Z
            origin.x -= halfSize;
            // Y is already at bottom
            origin.z -= halfSize;
            break;
            
        case VoxelData::FaceDirection::PosX:  // Right face
            // Bottom-left when looking at face is at minimum Y and Z
            origin.x += halfSize;  // Right side of voxel
            // Y is already at bottom
            origin.z -= halfSize;
            break;
            
        case VoxelData::FaceDirection::NegX:  // Left face
            // Bottom-left when looking at face is at minimum Y and maximum Z
            origin.x -= halfSize;  // Left side of voxel
            // Y is already at bottom
            origin.z += halfSize;  // Need to go to back for left face view
            break;
            
        case VoxelData::FaceDirection::PosZ:  // Back face
            // Bottom-left when looking at face is at minimum X and Y
            origin.x -= halfSize;
            // Y is already at bottom
            origin.z += halfSize;  // Back side of voxel
            break;
            
        case VoxelData::FaceDirection::NegZ:  // Front face
            // Bottom-left when looking at face is at maximum X and minimum Y
            origin.x += halfSize;  // Need to go to right for front face view
            // Y is already at bottom
            origin.z -= halfSize;  // Front side of voxel
            break;
    }
    
    return origin;
}

WorldCoordinates VoxelPlacementMath::convertCenterBottomToBottomLeftBack(const WorldCoordinates& centerBottom,
                                                                         float voxelSize) {
    // Convert from center-bottom to bottom-left-back
    // The cursor position represents the center of the bottom face of the voxel
    // We need to subtract half the voxel size from X and Z to get the bottom-left-back corner
    const Vector3f& center = centerBottom.value();
    float halfSize = voxelSize * 0.5f;
    
    Vector3f bottomLeftBack(
        center.x - halfSize,  // Move left by half size
        center.y,             // Y stays the same (bottom)
        center.z - halfSize   // Move back by half size
    );
    
    return WorldCoordinates(bottomLeftBack);
}

Vector3f VoxelPlacementMath::calculateFacePlacementOffset(VoxelData::FaceDirection faceDir,
                                                         float voxelSize) {
    // Calculate the offset needed to position a voxel adjacent to a face
    // The offset depends on which face we're placing against
    float halfSize = voxelSize * 0.5f;
    
    switch (faceDir) {
        case VoxelData::FaceDirection::PosX:
            // Right face: offset by half size to the right
            return Vector3f(halfSize, 0.0f, 0.0f);
            
        case VoxelData::FaceDirection::NegX:
            // Left face: offset by half size to the left
            return Vector3f(-halfSize, 0.0f, 0.0f);
            
        case VoxelData::FaceDirection::PosY:
            // Top face: no offset needed (bottom-aligned)
            return Vector3f(0.0f, 0.0f, 0.0f);
            
        case VoxelData::FaceDirection::NegY:
            // Bottom face: offset down by full voxel size
            return Vector3f(0.0f, -voxelSize, 0.0f);
            
        case VoxelData::FaceDirection::PosZ:
            // Back face: offset by half size backward
            return Vector3f(0.0f, 0.0f, halfSize);
            
        case VoxelData::FaceDirection::NegZ:
            // Front face: offset by half size forward
            return Vector3f(0.0f, 0.0f, -halfSize);
            
        default:
            // Should not happen
            return Vector3f(0.0f, 0.0f, 0.0f);
    }
}

} // namespace Math
} // namespace VoxelEditor