#include "../include/voxel_math/FaceOperations.h"
#include "../include/voxel_math/VoxelGrid.h"
#include "../../../core/voxel_data/VoxelGrid.h"
#include <cmath>
#include <algorithm>

namespace VoxelEditor {
namespace Math {

// Define static members
const Vector3f FaceOperations::FACE_NORMALS[6] = {
    Vector3f(1.0f, 0.0f, 0.0f),   // PosX
    Vector3f(-1.0f, 0.0f, 0.0f),  // NegX
    Vector3f(0.0f, 1.0f, 0.0f),   // PosY
    Vector3f(0.0f, -1.0f, 0.0f),  // NegY
    Vector3f(0.0f, 0.0f, 1.0f),   // PosZ
    Vector3f(0.0f, 0.0f, -1.0f)   // NegZ
};

const char* const FaceOperations::FACE_NAMES[6] = {
    "PosX", "NegX", "PosY", "NegY", "PosZ", "NegZ"
};

Vector3f FaceOperations::getFaceNormal(VoxelData::FaceDirection direction) {
    int index = faceDirectionToIndex(direction);
    if (index >= 0 && index < 6) {
        return FACE_NORMALS[index];
    }
    return Vector3f(0.0f, 1.0f, 0.0f);  // Default to up
}

Vector3i FaceOperations::getFaceOffset(VoxelData::FaceDirection direction, int voxelSizeCm) {
    return VoxelGrid::getFaceDirectionOffset(direction, voxelSizeCm);
}

VoxelData::FaceDirection FaceOperations::getOppositeFace(VoxelData::FaceDirection direction) {
    switch (direction) {
        case VoxelData::FaceDirection::PosX:
            return VoxelData::FaceDirection::NegX;
        case VoxelData::FaceDirection::NegX:
            return VoxelData::FaceDirection::PosX;
        case VoxelData::FaceDirection::PosY:
            return VoxelData::FaceDirection::NegY;
        case VoxelData::FaceDirection::NegY:
            return VoxelData::FaceDirection::PosY;
        case VoxelData::FaceDirection::PosZ:
            return VoxelData::FaceDirection::NegZ;
        case VoxelData::FaceDirection::NegZ:
            return VoxelData::FaceDirection::PosZ;
        default:
            return direction;
    }
}

VoxelData::FaceDirection FaceOperations::determineFaceFromHit(const WorldCoordinates& hitPoint,
                                                             const VoxelBounds& voxelBounds,
                                                             float epsilon) {
    const Vector3f& hit = hitPoint.value();
    
    // Get voxel bounds
    Vector3f minBounds = voxelBounds.min().value();
    Vector3f maxBounds = voxelBounds.max().value();
    
    // Calculate distance to each face
    float distToMinX = std::abs(hit.x - minBounds.x);
    float distToMaxX = std::abs(hit.x - maxBounds.x);
    float distToMinY = std::abs(hit.y - minBounds.y);
    float distToMaxY = std::abs(hit.y - maxBounds.y);
    float distToMinZ = std::abs(hit.z - minBounds.z);
    float distToMaxZ = std::abs(hit.z - maxBounds.z);
    
    // Find the minimum distance
    float minDist = std::min({distToMinX, distToMaxX, distToMinY, distToMaxY, distToMinZ, distToMaxZ});
    
    // Return the face direction based on which plane we're closest to
    if (minDist == distToMinX && distToMinX < epsilon) {
        return VoxelData::FaceDirection::NegX;  // Left face
    } else if (minDist == distToMaxX && distToMaxX < epsilon) {
        return VoxelData::FaceDirection::PosX;  // Right face
    } else if (minDist == distToMinY && distToMinY < epsilon) {
        return VoxelData::FaceDirection::NegY;  // Bottom face
    } else if (minDist == distToMaxY && distToMaxY < epsilon) {
        return VoxelData::FaceDirection::PosY;  // Top face
    } else if (minDist == distToMinZ && distToMinZ < epsilon) {
        return VoxelData::FaceDirection::NegZ;  // Front face
    } else if (minDist == distToMaxZ && distToMaxZ < epsilon) {
        return VoxelData::FaceDirection::PosZ;  // Back face
    }
    
    // If we're not close to any face, use the closest one anyway
    if (minDist == distToMinX) return VoxelData::FaceDirection::NegX;
    if (minDist == distToMaxX) return VoxelData::FaceDirection::PosX;
    if (minDist == distToMinY) return VoxelData::FaceDirection::NegY;
    if (minDist == distToMaxY) return VoxelData::FaceDirection::PosY;
    if (minDist == distToMinZ) return VoxelData::FaceDirection::NegZ;
    
    return VoxelData::FaceDirection::PosZ;  // Default
}

VoxelData::FaceDirection FaceOperations::determineFaceFromRayDirection(const Vector3f& rayDirection) {
    // Find which component has the largest absolute value
    float absX = std::abs(rayDirection.x);
    float absY = std::abs(rayDirection.y);
    float absZ = std::abs(rayDirection.z);
    
    if (absX >= absY && absX >= absZ) {
        // X is dominant direction
        return (rayDirection.x > 0) ? VoxelData::FaceDirection::PosX 
                                    : VoxelData::FaceDirection::NegX;
    } else if (absY >= absX && absY >= absZ) {
        // Y is dominant direction
        return (rayDirection.y > 0) ? VoxelData::FaceDirection::PosY 
                                    : VoxelData::FaceDirection::NegY;
    } else {
        // Z is dominant direction
        return (rayDirection.z > 0) ? VoxelData::FaceDirection::PosZ 
                                    : VoxelData::FaceDirection::NegZ;
    }
}

IncrementCoordinates FaceOperations::calculatePlacementPosition(const IncrementCoordinates& voxelPos,
                                                               VoxelData::FaceDirection face,
                                                               VoxelData::VoxelResolution resolution) {
    // Get the offset for the face direction
    int voxelSizeCm = VoxelGrid::getVoxelSizeCm(resolution);
    Vector3i offset = getFaceOffset(face, voxelSizeCm);
    
    // Calculate new position
    const Vector3i& pos = voxelPos.value();
    return IncrementCoordinates(
        pos.x + offset.x,
        pos.y + offset.y,
        pos.z + offset.z
    );
}

// Note: isFaceExposed method commented out due to VoxelGrid namespace conflicts
// Will be implemented later when VoxelGrid dependencies are resolved

void FaceOperations::getAllFaceNormals(Vector3f normals[6]) {
    for (int i = 0; i < 6; ++i) {
        normals[i] = FACE_NORMALS[i];
    }
}

void FaceOperations::getAllFaceOffsets(int voxelSizeCm, Vector3i offsets[6]) {
    offsets[0] = Vector3i(voxelSizeCm, 0, 0);   // PositiveX
    offsets[1] = Vector3i(-voxelSizeCm, 0, 0);  // NegativeX
    offsets[2] = Vector3i(0, voxelSizeCm, 0);   // PositiveY
    offsets[3] = Vector3i(0, -voxelSizeCm, 0);  // NegativeY
    offsets[4] = Vector3i(0, 0, voxelSizeCm);   // PositiveZ
    offsets[5] = Vector3i(0, 0, -voxelSizeCm);  // NegativeZ
}

int FaceOperations::faceDirectionToIndex(VoxelData::FaceDirection direction) {
    switch (direction) {
        case VoxelData::FaceDirection::PosX: return 0;
        case VoxelData::FaceDirection::NegX: return 1;
        case VoxelData::FaceDirection::PosY: return 2;
        case VoxelData::FaceDirection::NegY: return 3;
        case VoxelData::FaceDirection::PosZ: return 4;
        case VoxelData::FaceDirection::NegZ: return 5;
        default: return 0;
    }
}

VoxelData::FaceDirection FaceOperations::indexToFaceDirection(int index) {
    switch (index) {
        case 0: return VoxelData::FaceDirection::PosX;
        case 1: return VoxelData::FaceDirection::NegX;
        case 2: return VoxelData::FaceDirection::PosY;
        case 3: return VoxelData::FaceDirection::NegY;
        case 4: return VoxelData::FaceDirection::PosZ;
        case 5: return VoxelData::FaceDirection::NegZ;
        default: return VoxelData::FaceDirection::PosY;
    }
}

const char* FaceOperations::getFaceDirectionName(VoxelData::FaceDirection direction) {
    int index = faceDirectionToIndex(direction);
    if (index >= 0 && index < 6) {
        return FACE_NAMES[index];
    }
    return "Unknown";
}

} // namespace Math
} // namespace VoxelEditor