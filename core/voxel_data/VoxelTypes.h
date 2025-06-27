#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"

namespace VoxelEditor {
namespace VoxelData {

// Use the VoxelResolution enum from CoordinateConverter.h
using VoxelResolution = VoxelEditor::VoxelData::VoxelResolution;

// Use the getVoxelSize function from CoordinateConverter.h
using VoxelEditor::VoxelData::getVoxelSize;

constexpr const char* getVoxelSizeName(VoxelResolution resolution) {
    constexpr const char* names[10] = {
        "1cm", "2cm", "4cm", "8cm", "16cm",
        "32cm", "64cm", "128cm", "256cm", "512cm"
    };
    return names[static_cast<uint8_t>(resolution)];
}

constexpr bool isValidResolution(int resolution) {
    return resolution >= 0 && resolution < static_cast<int>(VoxelResolution::COUNT);
}


struct VoxelPosition {
    Math::IncrementCoordinates incrementPos;
    VoxelResolution resolution;
    
    VoxelPosition() : incrementPos(0, 0, 0), resolution(VoxelResolution::Size_1cm) {}
    
    VoxelPosition(const Math::IncrementCoordinates& pos, VoxelResolution res) 
        : incrementPos(pos), resolution(res) {}
    
    VoxelPosition(const Math::Vector3i& pos, VoxelResolution res) 
        : incrementPos(Math::IncrementCoordinates(pos)), resolution(res) {}
    
    VoxelPosition(int x, int y, int z, VoxelResolution res)
        : incrementPos(x, y, z), resolution(res) {}
    
    // Convert increment position to world space coordinates using centered coordinate system
    Math::Vector3f toWorldSpace() const {
        Math::WorldCoordinates worldCoord = Math::CoordinateConverter::incrementToWorld(incrementPos);
        return worldCoord.value();
    }
    
    // Convert world space coordinates to increment position using centered coordinate system
    static VoxelPosition fromWorldSpace(const Math::Vector3f& worldPos, 
                                      VoxelResolution resolution) {
        Math::IncrementCoordinates incCoord = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(worldPos));
        
        return VoxelPosition(incCoord, resolution);
    }
    
    // Get the world space bounds of this voxel
    // The placement position represents the bottom-center of the voxel
    void getWorldBounds(Math::Vector3f& minBounds, 
                       Math::Vector3f& maxBounds) const {
        float voxelSize = getVoxelSize(resolution);
        Math::Vector3f bottomCenter = toWorldSpace();
        
        // Calculate bounds where bottomCenter is at the bottom face center
        float halfSize = voxelSize * 0.5f;
        minBounds = Math::Vector3f(bottomCenter.x - halfSize, bottomCenter.y, bottomCenter.z - halfSize);
        maxBounds = Math::Vector3f(bottomCenter.x + halfSize, bottomCenter.y + voxelSize, bottomCenter.z + halfSize);
    }
    
    // Equality operators
    bool operator==(const VoxelPosition& other) const {
        return incrementPos == other.incrementPos && resolution == other.resolution;
    }
    
    bool operator!=(const VoxelPosition& other) const {
        return !(*this == other);
    }
    
    // For use in hash maps
    struct Hash {
        std::size_t operator()(const VoxelPosition& pos) const {
            std::size_t h1 = std::hash<int>{}(pos.incrementPos.x());
            std::size_t h2 = std::hash<int>{}(pos.incrementPos.y());
            std::size_t h3 = std::hash<int>{}(pos.incrementPos.z());
            std::size_t h4 = std::hash<uint8_t>{}(static_cast<uint8_t>(pos.resolution));
            
            // Combine hashes
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };
};

// Workspace constraints
struct WorkspaceConstraints {
    static constexpr float MIN_SIZE = 2.0f;  // 2m³ minimum
    static constexpr float MAX_SIZE = 8.0f;  // 8m³ maximum
    static constexpr float DEFAULT_SIZE = 5.0f;  // 5m³ default
    
    static bool isValidSize(const Math::Vector3f& size) {
        return size.x >= MIN_SIZE && size.x <= MAX_SIZE &&
               size.y >= MIN_SIZE && size.y <= MAX_SIZE &&
               size.z >= MIN_SIZE && size.z <= MAX_SIZE;
    }
    
    static bool isValidSize(float size) {
        return size >= MIN_SIZE && size <= MAX_SIZE;
    }
    
    static Math::Vector3f clampSize(const Math::Vector3f& size) {
        return Math::Vector3f(
            std::clamp(size.x, MIN_SIZE, MAX_SIZE),
            std::clamp(size.y, MIN_SIZE, MAX_SIZE),
            std::clamp(size.z, MIN_SIZE, MAX_SIZE)
        );
    }
};

// Face direction enum for adjacent position calculation
enum class FaceDirection : uint8_t {
    PosX = 0,  // +X direction (right)
    NegX = 1,  // -X direction (left)
    PosY = 2,  // +Y direction (up)
    NegY = 3,  // -Y direction (down)
    PosZ = 4,  // +Z direction (forward)
    NegZ = 5   // -Z direction (back)
};


// Calculate maximum grid dimensions for a given resolution and workspace size
inline Math::Vector3i calculateMaxGridDimensions(VoxelResolution resolution, const Math::Vector3f& workspaceSize) {
    float voxelSize = getVoxelSize(resolution);
    
    return Math::Vector3i(
        static_cast<int>(std::ceil(workspaceSize.x / voxelSize)),
        static_cast<int>(std::ceil(workspaceSize.y / voxelSize)),
        static_cast<int>(std::ceil(workspaceSize.z / voxelSize))
    );
}

// Check if an increment position is within workspace bounds
inline bool isPositionInBounds(const Math::IncrementCoordinates& incrementPos, const Math::Vector3f& workspaceSize) {
    return Math::CoordinateConverter::isValidIncrementCoordinate(incrementPos, workspaceSize);
}

// Overload for Vector3i for backward compatibility
inline bool isPositionInBounds(const Math::Vector3i& incrementPos, const Math::Vector3f& workspaceSize) {
    return isPositionInBounds(Math::IncrementCoordinates(incrementPos), workspaceSize);
}

}
}