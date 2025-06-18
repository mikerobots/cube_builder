#pragma once

#include "math/Vector3f.h"
#include "math/Vector3i.h"
#include "math/BoundingBox.h"
#include "math/CoordinateTypes.h"
#include "math/CoordinateConverter.h"
#include "rendering/RenderTypes.h"
#include "voxel_data/VoxelTypes.h"
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace VoxelEditor {
namespace Groups {

// Type aliases
using GroupId = uint32_t;
constexpr GroupId INVALID_GROUP_ID = 0;

// VoxelId structure for group membership
struct VoxelId {
    Math::IncrementCoordinates position;
    VoxelData::VoxelResolution resolution;
    
    VoxelId() = default;
    VoxelId(const Math::IncrementCoordinates& pos, VoxelData::VoxelResolution res)
        : position(pos), resolution(res) {}
    VoxelId(const Math::Vector3i& pos, VoxelData::VoxelResolution res)
        : position(Math::IncrementCoordinates(pos)), resolution(res) {}
    
    bool operator==(const VoxelId& other) const {
        return position == other.position && resolution == other.resolution;
    }
    
    bool operator!=(const VoxelId& other) const {
        return !(*this == other);
    }
    
    size_t hash() const {
        const Math::Vector3i& pos = position.value();
        size_t h1 = std::hash<int>{}(pos.x);
        size_t h2 = std::hash<int>{}(pos.y);
        size_t h3 = std::hash<int>{}(pos.z);
        size_t h4 = std::hash<int>{}(static_cast<int>(resolution));
        
        // Combine hashes
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
    
    // Convert to world position 
    Math::WorldCoordinates getWorldPosition() const {
        // With the new coordinate system, IncrementCoordinates can be directly converted to world coordinates
        return Math::CoordinateConverter::incrementToWorld(position);
    }
    
    // Get voxel size
    float getVoxelSize() const {
        return VoxelData::getVoxelSize(resolution);
    }
    
    // Get bounding box in world space
    Math::BoundingBox getBounds() const {
        Math::WorldCoordinates worldPos = getWorldPosition();
        Math::Vector3f pos = worldPos.value();
        float voxelSize = getVoxelSize();
        return Math::BoundingBox(pos, pos + Math::Vector3f(voxelSize, voxelSize, voxelSize));
    }
};

// Group modification types for events
enum class GroupModificationType {
    Created,
    Deleted,
    Renamed,
    VoxelAdded,
    VoxelRemoved,
    Moved,
    Rotated,
    Scaled,
    VisibilityChanged,
    OpacityChanged,
    ColorChanged,
    Locked,
    Unlocked,
    ParentChanged,
    PropertiesChanged,
    LockChanged
};

// Group metadata
struct GroupMetadata {
    std::string name;
    Rendering::Color color;
    bool visible = true;
    bool locked = false;
    float opacity = 1.0f;
    Math::Vector3f pivot = Math::Vector3f(0, 0, 0);
    std::string description;
    std::chrono::time_point<std::chrono::system_clock> created;
    std::chrono::time_point<std::chrono::system_clock> modified;
    
    GroupMetadata() {
        auto now = std::chrono::system_clock::now();
        created = now;
        modified = now;
    }
    
    void updateModified() {
        modified = std::chrono::system_clock::now();
    }
};

// Group information for queries
struct GroupInfo {
    GroupId id = INVALID_GROUP_ID;
    std::string name;
    Rendering::Color color;
    bool visible = true;
    bool locked = false;
    float opacity = 1.0f;
    size_t voxelCount = 0;
    GroupId parentId = INVALID_GROUP_ID;
    std::vector<GroupId> childIds;
    Math::BoundingBox bounds;
    
    GroupInfo() = default;
    GroupInfo(GroupId gid, const GroupMetadata& metadata)
        : id(gid), name(metadata.name), color(metadata.color)
        , visible(metadata.visible), locked(metadata.locked)
        , opacity(metadata.opacity) {}
};

// Transform structure for group operations
struct GroupTransform {
    Math::Vector3f translation = Math::Vector3f(0, 0, 0);
    Math::Vector3f rotation = Math::Vector3f(0, 0, 0); // Euler angles in degrees
    Math::Vector3f scale = Math::Vector3f(1, 1, 1);
    
    GroupTransform() = default;
    GroupTransform(const Math::Vector3f& trans) : translation(trans) {}
    
    bool isIdentity() const {
        return translation.length() < 0.0001f &&
               rotation.length() < 0.0001f &&
               (scale - Math::Vector3f(1, 1, 1)).length() < 0.0001f;
    }
};

// Color palette for auto-assigning group colors
class GroupColorPalette {
public:
    static const std::vector<Rendering::Color>& getDefaultPalette() {
        static std::vector<Rendering::Color> palette = {
            Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f),   // Red
            Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f),   // Green
            Rendering::Color(0.0f, 0.0f, 1.0f, 1.0f),   // Blue
            Rendering::Color(1.0f, 1.0f, 0.0f, 1.0f),   // Yellow
            Rendering::Color(1.0f, 0.0f, 1.0f, 1.0f),   // Magenta
            Rendering::Color(0.0f, 1.0f, 1.0f, 1.0f),   // Cyan
            Rendering::Color(1.0f, 0.5f, 0.0f, 1.0f),   // Orange
            Rendering::Color(0.5f, 0.0f, 1.0f, 1.0f),   // Purple
            Rendering::Color(0.0f, 0.5f, 0.0f, 1.0f),   // Dark Green
            Rendering::Color(0.5f, 0.5f, 0.5f, 1.0f),   // Gray
        };
        return palette;
    }
    
    static Rendering::Color getColorForIndex(size_t index) {
        const auto& palette = getDefaultPalette();
        return palette[index % palette.size()];
    }
    
    static Rendering::Color getRandomColor() {
        const auto& palette = getDefaultPalette();
        size_t index = static_cast<size_t>(rand()) % palette.size();
        return palette[index];
    }
};

// Statistics for group performance monitoring
struct GroupStats {
    size_t totalGroups = 0;
    size_t totalVoxels = 0;
    size_t maxGroupSize = 0;
    size_t maxHierarchyDepth = 0;
    float averageGroupSize = 0.0f;
    size_t memoryUsage = 0;
};

} // namespace Groups
} // namespace VoxelEditor

// Hash function for VoxelId to use in unordered containers
namespace std {
template<>
struct hash<VoxelEditor::Groups::VoxelId> {
    size_t operator()(const VoxelEditor::Groups::VoxelId& voxel) const {
        return voxel.hash();
    }
};
}