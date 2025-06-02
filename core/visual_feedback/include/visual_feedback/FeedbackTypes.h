#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "RenderTypes.h"
#include "VoxelTypes.h"
#include "Vector3f.h"
#include "Vector2f.h"
#include "Vector3i.h"
#include "BoundingBox.h"
#include "Quaternion.h"
#include "Matrix4f.h"

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
    Face(const Math::Vector3i& voxelPos, VoxelData::VoxelResolution res, FaceDirection dir);
    
    // Face identification
    FaceId getId() const;
    bool isValid() const { return m_valid; }
    
    // Properties
    Math::Vector3i getVoxelPosition() const { return m_voxelPosition; }
    VoxelData::VoxelResolution getResolution() const { return m_resolution; }
    FaceDirection getDirection() const { return m_direction; }
    
    // Geometric properties
    Math::Vector3f getWorldPosition() const;
    Math::Vector3f getNormal() const;
    std::array<Math::Vector3f, 4> getCorners() const;
    Math::Vector3f getCenter() const;
    float getArea() const;
    
    // Comparison
    bool operator==(const Face& other) const;
    bool operator!=(const Face& other) const { return !(*this == other); }
    
private:
    Math::Vector3i m_voxelPosition;
    VoxelData::VoxelResolution m_resolution;
    FaceDirection m_direction;
    bool m_valid = false;
    
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
    Math::Vector3f origin;
    Math::Vector3f direction;
    
    Ray() = default;
    Ray(const Math::Vector3f& o, const Math::Vector3f& d) : origin(o), direction(d.normalized()) {}
    
    Math::Vector3f pointAt(float t) const { return origin + direction * t; }
};

// Raycast hit information
struct RaycastHit {
    bool hit = false;
    Math::Vector3f position;
    Math::Vector3f normal;
    Face face;
    float distance = 0.0f;
};

// Transform for instances
struct Transform {
    Math::Vector3f position;
    Math::Quaternion rotation;
    Math::Vector3f scale;
    
    Transform() : scale(1.0f, 1.0f, 1.0f) {}
    
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