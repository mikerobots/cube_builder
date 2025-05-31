#pragma once

#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector3i.h"
#include <cstdint>
#include <cmath>

namespace VoxelEditor {
namespace VoxelData {

enum class VoxelResolution : uint8_t {
    Size_1cm = 0,   // 1cm voxels (0.01m)
    Size_2cm = 1,   // 2cm voxels (0.02m)
    Size_4cm = 2,   // 4cm voxels (0.04m)
    Size_8cm = 3,   // 8cm voxels (0.08m)
    Size_16cm = 4,  // 16cm voxels (0.16m)
    Size_32cm = 5,  // 32cm voxels (0.32m)
    Size_64cm = 6,  // 64cm voxels (0.64m)
    Size_128cm = 7, // 128cm voxels (1.28m)
    Size_256cm = 8, // 256cm voxels (2.56m)
    Size_512cm = 9, // 512cm voxels (5.12m)
    COUNT = 10
};

constexpr float getVoxelSize(VoxelResolution resolution) {
    constexpr float sizes[10] = {
        0.01f, 0.02f, 0.04f, 0.08f, 0.16f,
        0.32f, 0.64f, 1.28f, 2.56f, 5.12f
    };
    return sizes[static_cast<uint8_t>(resolution)];
}

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
    Math::Vector3i gridPos;
    VoxelResolution resolution;
    
    VoxelPosition() : gridPos(0, 0, 0), resolution(VoxelResolution::Size_1cm) {}
    
    VoxelPosition(const Math::Vector3i& pos, VoxelResolution res) 
        : gridPos(pos), resolution(res) {}
    
    VoxelPosition(int x, int y, int z, VoxelResolution res)
        : gridPos(x, y, z), resolution(res) {}
    
    // Convert grid position to world space coordinates
    Math::Vector3f toWorldSpace(const Math::Vector3f& workspaceSize) const {
        float voxelSize = getVoxelSize(resolution);
        
        // Convert grid position to world position
        // Grid is centered at origin, so we need to offset by half workspace
        Math::Vector3f worldPos = Math::Vector3f(
            static_cast<float>(gridPos.x) * voxelSize,
            static_cast<float>(gridPos.y) * voxelSize,
            static_cast<float>(gridPos.z) * voxelSize
        );
        
        // Center around origin
        worldPos = worldPos - workspaceSize * 0.5f;
        
        return worldPos;
    }
    
    // Convert world space coordinates to grid position
    static VoxelPosition fromWorldSpace(const Math::Vector3f& worldPos, 
                                      VoxelResolution resolution,
                                      const Math::Vector3f& workspaceSize) {
        float voxelSize = getVoxelSize(resolution);
        
        // Center around origin and convert to grid coordinates
        Math::Vector3f centeredPos = worldPos + workspaceSize * 0.5f;
        
        Math::Vector3i gridPos(
            static_cast<int>(std::floor(centeredPos.x / voxelSize)),
            static_cast<int>(std::floor(centeredPos.y / voxelSize)),
            static_cast<int>(std::floor(centeredPos.z / voxelSize))
        );
        
        return VoxelPosition(gridPos, resolution);
    }
    
    // Get the world space bounds of this voxel
    void getWorldBounds(const Math::Vector3f& workspaceSize, 
                       Math::Vector3f& minBounds, 
                       Math::Vector3f& maxBounds) const {
        float voxelSize = getVoxelSize(resolution);
        Math::Vector3f worldPos = toWorldSpace(workspaceSize);
        
        minBounds = worldPos;
        maxBounds = worldPos + Math::Vector3f(voxelSize, voxelSize, voxelSize);
    }
    
    // Equality operators
    bool operator==(const VoxelPosition& other) const {
        return gridPos == other.gridPos && resolution == other.resolution;
    }
    
    bool operator!=(const VoxelPosition& other) const {
        return !(*this == other);
    }
    
    // For use in hash maps
    struct Hash {
        std::size_t operator()(const VoxelPosition& pos) const {
            std::size_t h1 = std::hash<int>{}(pos.gridPos.x);
            std::size_t h2 = std::hash<int>{}(pos.gridPos.y);
            std::size_t h3 = std::hash<int>{}(pos.gridPos.z);
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

// Calculate maximum grid dimensions for a given resolution and workspace size
Math::Vector3i calculateMaxGridDimensions(VoxelResolution resolution, const Math::Vector3f& workspaceSize) {
    float voxelSize = getVoxelSize(resolution);
    
    return Math::Vector3i(
        static_cast<int>(std::ceil(workspaceSize.x / voxelSize)),
        static_cast<int>(std::ceil(workspaceSize.y / voxelSize)),
        static_cast<int>(std::ceil(workspaceSize.z / voxelSize))
    );
}

// Check if a grid position is within workspace bounds
bool isPositionInBounds(const Math::Vector3i& gridPos, VoxelResolution resolution, const Math::Vector3f& workspaceSize) {
    Math::Vector3i maxDims = calculateMaxGridDimensions(resolution, workspaceSize);
    
    return gridPos.x >= 0 && gridPos.x < maxDims.x &&
           gridPos.y >= 0 && gridPos.y < maxDims.y &&
           gridPos.z >= 0 && gridPos.z < maxDims.z;
}

}
}