#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "core/rendering/RenderTypes.h"
#include "core/voxel_data/VoxelTypes.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Vector2f.h"
#include "foundation/math/Vector3i.h"
#include "foundation/math/BoundingBox.h"
#include "foundation/math/Quaternion.h"
#include "foundation/math/Matrix4f.h"
#include "foundation/math/CoordinateTypes.h"
#include "foundation/math/CoordinateConverter.h"

namespace VoxelEditor {
namespace VisualFeedback {

// Forward declarations
class Face;
class HighlightStyle;
class OutlineStyle;
class TextStyle;

// Type aliases
using FaceId = uint64_t;
using GroupId = uint32_t;
using VoxelId = uint64_t;

// Enumerations
enum class FaceDirection : uint8_t {
    PositiveX,
    NegativeX,
    PositiveY,
    NegativeY,
    PositiveZ,
    NegativeZ
};

enum class BlendMode : uint8_t {
    Alpha,
    Additive,
    Multiply,
    Screen
};

enum class LinePattern : uint8_t {
    Solid,
    Dashed,
    Dotted,
    DashDot
};

enum class TextAlignment : uint8_t {
    Left,
    Center,
    Right
};

// Face representation
class Face {
public:
    Face() = default;
    Face(const Math::IncrementCoordinates& voxelPos, VoxelData::VoxelResolution res, FaceDirection dir);
    
    // Special constructor for ground plane (Enhancement)
    static Face GroundPlane(const Math::WorldCoordinates& hitPoint);
    
    // Face identification
    FaceId getId() const;
    bool isValid() const { return m_valid; }
    bool isGroundPlane() const { return m_isGroundPlane; }
    
    // Properties
    Math::IncrementCoordinates getVoxelPosition() const { return m_voxelPosition; }
    VoxelData::VoxelResolution getResolution() const { return m_resolution; }
    FaceDirection getDirection() const { return m_direction; }
    
    // Geometric properties
    Math::WorldCoordinates getWorldPosition() const;
    Math::Vector3f getNormal() const;
    std::array<Math::WorldCoordinates, 4> getCorners() const;
    Math::WorldCoordinates getCenter() const;
    float getArea() const;
    
    // Comparison
    bool operator==(const Face& other) const;
    bool operator!=(const Face& other) const { return !(*this == other); }
    
    // Ground plane specific (Enhancement)
    Math::WorldCoordinates getGroundPlaneHitPoint() const { return m_groundHitPoint; }
    
    // Backward compatibility wrapper methods (to be removed in Phase 7)
    Face(const Math::Vector3i& voxelPos, VoxelData::VoxelResolution res, FaceDirection dir)
        : Face(Math::IncrementCoordinates(voxelPos), res, dir) {}
    static Face GroundPlane(const Math::Vector3f& hitPoint) {
        return GroundPlane(Math::WorldCoordinates(hitPoint));
    }
    Math::Vector3i getVoxelPositionVector() const { return m_voxelPosition.value(); }
    Math::Vector3f getWorldPositionVector() const { return getWorldPosition().value(); }
    Math::Vector3f getGroundPlaneHitPointVector() const { return m_groundHitPoint.value(); }
    
private:
    Math::IncrementCoordinates m_voxelPosition;
    VoxelData::VoxelResolution m_resolution;
    FaceDirection m_direction;
    bool m_valid = false;
    bool m_isGroundPlane = false;  // Enhancement
    Math::WorldCoordinates m_groundHitPoint;  // Enhancement
    
    float getVoxelSize() const;
};

// Highlight style configuration
class HighlightStyle {
public:
    Rendering::Color color = Rendering::Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    float intensity = 1.0f;
    float pulseSpeed = 2.0f;
    bool animated = true;
    bool wireframe = false;
    float lineWidth = 2.0f;
    BlendMode blendMode = BlendMode::Alpha;
    
    // Factory methods
    static HighlightStyle Face();
    static HighlightStyle Selection();
    static HighlightStyle Group();
    static HighlightStyle Preview();
};

// Outline style configuration
class OutlineStyle {
public:
    Rendering::Color color = Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f); // Green
    float lineWidth = 3.0f;
    LinePattern pattern = LinePattern::Solid;
    bool depthTest = false;
    float opacity = 0.8f;
    bool animated = false;
    float animationSpeed = 1.0f;
    
    // Factory methods
    static OutlineStyle VoxelPreview();
    static OutlineStyle VoxelPreviewInvalid(); // Red outline for invalid placement
    static OutlineStyle GroupBoundary();
    static OutlineStyle SelectionBox();
    static OutlineStyle WorkspaceBounds();
};

// Text style configuration
class TextStyle {
public:
    Rendering::Color color = Rendering::Color::White();
    float size = 16.0f;
    TextAlignment alignment = TextAlignment::Left;
    bool background = false;
    Rendering::Color backgroundColor = Rendering::Color::Black();
    float backgroundOpacity = 0.5f;
    
    // Factory methods
    static TextStyle Default();
    static TextStyle Header();
    static TextStyle Debug();
    static TextStyle Warning();
    static TextStyle Error();
};

// Ray for face detection
struct Ray {
    Math::WorldCoordinates origin;
    Math::Vector3f direction;  // Direction remains as Vector3f (normalized vector)
    
    Ray() = default;
    Ray(const Math::WorldCoordinates& o, const Math::Vector3f& d) : origin(o), direction(d.normalized()) {}
    
    // Backward compatibility constructors
    Ray(const Math::Vector3f& o, const Math::Vector3f& d) : origin(Math::WorldCoordinates(o)), direction(d.normalized()) {}
    
    Math::WorldCoordinates pointAt(float t) const { return Math::WorldCoordinates(origin.value() + direction * t); }
    Math::Vector3f pointAtVector(float t) const { return origin.value() + direction * t; }
};

// Raycast hit information
struct RaycastHit {
    bool hit = false;
    Math::WorldCoordinates position;
    Math::Vector3f normal;  // Normal remains as Vector3f (normalized direction)
    Face face;
    float distance = 0.0f;
};

// Transform for instances
struct Transform {
    Math::WorldCoordinates position;
    Math::Quaternion rotation;
    Math::Vector3f scale;
    
    Transform() : position(Math::WorldCoordinates::zero()), scale(1.0f, 1.0f, 1.0f) {}
    
    Math::Matrix4f toMatrix() const;
};

// Performance stats
struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t triangleCount = 0;
    uint32_t vertexCount = 0;
    float frameTime = 0.0f;
    float cpuTime = 0.0f;
    float gpuTime = 0.0f;
};

// Performance metrics for display
struct PerformanceMetrics {
    float frameTime = 0.0f;
    uint32_t voxelCount = 0;
    uint32_t triangleCount = 0;
    uint32_t drawCalls = 0;
    size_t memoryUsed = 0;
    size_t memoryTotal = 0;
    
    struct FormattedText {
        std::string frameTimeText;
        std::string voxelCountText;
        std::string memoryUsageText;
        std::string performanceText;
    };
};

// Grid information
struct GridInfo {
    uint32_t lineCount = 0;
    float extent = 0.0f;
    uint32_t vertexCount = 0;
    float spacing = 0.0f;
};

// Utility functions
inline Math::Vector3f faceDirectionToNormal(FaceDirection dir) {
    switch (dir) {
        case FaceDirection::PositiveX: return Math::Vector3f(1, 0, 0);
        case FaceDirection::NegativeX: return Math::Vector3f(-1, 0, 0);
        case FaceDirection::PositiveY: return Math::Vector3f(0, 1, 0);
        case FaceDirection::NegativeY: return Math::Vector3f(0, -1, 0);
        case FaceDirection::PositiveZ: return Math::Vector3f(0, 0, 1);
        case FaceDirection::NegativeZ: return Math::Vector3f(0, 0, -1);
    }
    return Math::Vector3f(0, 1, 0);
}

inline FaceDirection oppositeDirection(FaceDirection dir) {
    switch (dir) {
        case FaceDirection::PositiveX: return FaceDirection::NegativeX;
        case FaceDirection::NegativeX: return FaceDirection::PositiveX;
        case FaceDirection::PositiveY: return FaceDirection::NegativeY;
        case FaceDirection::NegativeY: return FaceDirection::PositiveY;
        case FaceDirection::PositiveZ: return FaceDirection::NegativeZ;
        case FaceDirection::NegativeZ: return FaceDirection::PositiveZ;
    }
    return dir;
}

} // namespace VisualFeedback
} // namespace VoxelEditor