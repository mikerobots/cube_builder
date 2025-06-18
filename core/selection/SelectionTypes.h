#pragma once

#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/BoundingBox.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include "../voxel_data/VoxelTypes.h"
#include "../rendering/RenderTypes.h"
#include <functional>
#include <optional>

namespace VoxelEditor {
namespace Selection {

// Forward declarations
class SelectionSet;
class SelectionManager;

// Unique identifier for a voxel
struct VoxelId {
    Math::IncrementCoordinates position;
    VoxelData::VoxelResolution resolution;
    
    VoxelId() : position(Math::IncrementCoordinates(Math::Vector3i::Zero())), resolution(VoxelData::VoxelResolution::Size_1cm) {}
    
    VoxelId(const Math::IncrementCoordinates& pos, VoxelData::VoxelResolution res)
        : position(pos), resolution(res) {}
    
    // Backward compatibility constructor
    VoxelId(const Math::Vector3i& pos, VoxelData::VoxelResolution res)
        : position(Math::IncrementCoordinates(pos)), resolution(res) {}
    
    bool operator==(const VoxelId& other) const {
        return position == other.position && resolution == other.resolution;
    }
    
    bool operator!=(const VoxelId& other) const {
        return !(*this == other);
    }
    
    bool operator<(const VoxelId& other) const {
        if (resolution != other.resolution) {
            return static_cast<int>(resolution) < static_cast<int>(other.resolution);
        }
        if (position.x() != other.position.x()) return position.x() < other.position.x();
        if (position.y() != other.position.y()) return position.y() < other.position.y();
        return position.z() < other.position.z();
    }
    
    size_t hash() const {
        size_t h1 = std::hash<int>{}(position.x());
        size_t h2 = std::hash<int>{}(position.y());
        size_t h3 = std::hash<int>{}(position.z());
        size_t h4 = std::hash<int>{}(static_cast<int>(resolution));
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
    
    // Convert to world position
    Math::Vector3f getWorldPosition() const;
    
    // Get voxel size
    float getVoxelSize() const;
    
    // Get bounding box
    Math::BoundingBox getBounds() const;
};

// Selection mode for multi-selection operations
enum class SelectionMode {
    Replace,    // Replace current selection
    Add,        // Add to current selection
    Subtract,   // Remove from current selection
    Intersect   // Intersect with current selection
};

// Selection change type for events
enum class SelectionChangeType {
    Added,
    Removed,
    Replaced,
    Cleared,
    Modified
};

// Selection operation type
enum class SelectionOperationType {
    Move,
    Copy,
    Delete,
    Transform,
    Duplicate
};

// Flood fill criteria
enum class FloodFillCriteria {
    Connected6,         // 6-connected voxels (face neighbors)
    Connected18,        // 18-connected voxels (face + edge neighbors)  
    Connected26,        // 26-connected voxels (face + edge + corner neighbors)
    SameResolution,     // Same resolution level
    ConnectedSameRes    // Connected + same resolution
};

// Selection statistics
struct SelectionStats {
    size_t voxelCount = 0;
    size_t groupCount = 0;
    std::unordered_map<VoxelData::VoxelResolution, size_t> countByResolution;
    Math::BoundingBox bounds;
    Math::Vector3f center;
    float totalVolume = 0.0f;
    
    void clear() {
        voxelCount = 0;
        groupCount = 0;
        countByResolution.clear();
        bounds = Math::BoundingBox();
        center = Math::Vector3f::Zero();
        totalVolume = 0.0f;
    }
};

// Selection filter predicate
using SelectionPredicate = std::function<bool(const VoxelId&)>;

// Selection visitor function
using SelectionVisitor = std::function<void(const VoxelId&)>;

// Selection region shapes
struct SelectionRegion {
    enum Type {
        Box,
        Sphere,
        Cylinder,
        Cone,
        Custom
    } type;
    
    Math::BoundingBox box;
    Math::Vector3f center;
    float radius;
    float height;
    Math::Vector3f direction;
    
    SelectionRegion() : type(Box), radius(0.0f), height(0.0f) {}
};

// Selection highlight style
struct SelectionStyle {
    Rendering::Color outlineColor = Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f); // Green
    Rendering::Color fillColor = Rendering::Color(0.0f, 1.0f, 0.0f, 0.2f);    // Semi-transparent green
    float outlineThickness = 2.0f;
    bool animated = true;
    float animationSpeed = 1.0f;
    bool showBounds = true;
    bool showCount = true;
};

// Selection context for operations
struct SelectionContext {
    SelectionMode mode = SelectionMode::Replace;
    bool continuous = false;           // For drag selection
    bool preview = false;              // Show preview before applying
    std::optional<SelectionRegion> region;
    std::optional<SelectionPredicate> filter;
};

}
}

// Hash specialization for VoxelId
namespace std {
    template<>
    struct hash<VoxelEditor::Selection::VoxelId> {
        size_t operator()(const VoxelEditor::Selection::VoxelId& id) const {
            return id.hash();
        }
    };
}