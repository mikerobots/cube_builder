#include "../include/visual_feedback/FeedbackTypes.h"
#include "../../../foundation/math/Matrix4f.h"
#include <cmath>

namespace VoxelEditor {
namespace VisualFeedback {

// Face implementation
Face::Face(const Math::Vector3i& voxelPos, VoxelData::VoxelResolution res, FaceDirection dir)
    : m_voxelPosition(voxelPos)
    , m_resolution(res)
    , m_direction(dir)
    , m_valid(true) {
}

FaceId Face::getId() const {
    // Pack face data into 64-bit ID
    FaceId id = 0;
    id |= static_cast<FaceId>(m_voxelPosition.x & 0xFFFF) << 0;
    id |= static_cast<FaceId>(m_voxelPosition.y & 0xFFFF) << 16;
    id |= static_cast<FaceId>(m_voxelPosition.z & 0xFFFF) << 32;
    id |= static_cast<FaceId>(m_resolution) << 48;
    id |= static_cast<FaceId>(m_direction) << 56;
    return id;
}

float Face::getVoxelSize() const {
    return VoxelData::getVoxelSize(m_resolution);
}

Math::Vector3f Face::getWorldPosition() const {
    float voxelSize = getVoxelSize();
    Math::Vector3f basePos = Math::Vector3f(m_voxelPosition.x, m_voxelPosition.y, m_voxelPosition.z) * voxelSize;
    
    // Offset to face center
    Math::Vector3f offset(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    
    switch (m_direction) {
        case FaceDirection::PositiveX: offset.x = voxelSize; break;
        case FaceDirection::NegativeX: offset.x = 0; break;
        case FaceDirection::PositiveY: offset.y = voxelSize; break;
        case FaceDirection::NegativeY: offset.y = 0; break;
        case FaceDirection::PositiveZ: offset.z = voxelSize; break;
        case FaceDirection::NegativeZ: offset.z = 0; break;
    }
    
    return basePos + offset;
}

Math::Vector3f Face::getNormal() const {
    return faceDirectionToNormal(m_direction);
}

std::array<Math::Vector3f, 4> Face::getCorners() const {
    float voxelSize = getVoxelSize();
    Math::Vector3f basePos = Math::Vector3f(m_voxelPosition.x, m_voxelPosition.y, m_voxelPosition.z) * voxelSize;
    
    std::array<Math::Vector3f, 4> corners;
    
    switch (m_direction) {
        case FaceDirection::PositiveX:
            corners[0] = basePos + Math::Vector3f(voxelSize, 0, 0);
            corners[1] = basePos + Math::Vector3f(voxelSize, voxelSize, 0);
            corners[2] = basePos + Math::Vector3f(voxelSize, voxelSize, voxelSize);
            corners[3] = basePos + Math::Vector3f(voxelSize, 0, voxelSize);
            break;
        case FaceDirection::NegativeX:
            corners[0] = basePos + Math::Vector3f(0, 0, 0);
            corners[1] = basePos + Math::Vector3f(0, 0, voxelSize);
            corners[2] = basePos + Math::Vector3f(0, voxelSize, voxelSize);
            corners[3] = basePos + Math::Vector3f(0, voxelSize, 0);
            break;
        case FaceDirection::PositiveY:
            corners[0] = basePos + Math::Vector3f(0, voxelSize, 0);
            corners[1] = basePos + Math::Vector3f(voxelSize, voxelSize, 0);
            corners[2] = basePos + Math::Vector3f(voxelSize, voxelSize, voxelSize);
            corners[3] = basePos + Math::Vector3f(0, voxelSize, voxelSize);
            break;
        case FaceDirection::NegativeY:
            corners[0] = basePos + Math::Vector3f(0, 0, 0);
            corners[1] = basePos + Math::Vector3f(0, 0, voxelSize);
            corners[2] = basePos + Math::Vector3f(voxelSize, 0, voxelSize);
            corners[3] = basePos + Math::Vector3f(voxelSize, 0, 0);
            break;
        case FaceDirection::PositiveZ:
            corners[0] = basePos + Math::Vector3f(0, 0, voxelSize);
            corners[1] = basePos + Math::Vector3f(voxelSize, 0, voxelSize);
            corners[2] = basePos + Math::Vector3f(voxelSize, voxelSize, voxelSize);
            corners[3] = basePos + Math::Vector3f(0, voxelSize, voxelSize);
            break;
        case FaceDirection::NegativeZ:
            corners[0] = basePos + Math::Vector3f(0, 0, 0);
            corners[1] = basePos + Math::Vector3f(0, voxelSize, 0);
            corners[2] = basePos + Math::Vector3f(voxelSize, voxelSize, 0);
            corners[3] = basePos + Math::Vector3f(voxelSize, 0, 0);
            break;
    }
    
    return corners;
}

Math::Vector3f Face::getCenter() const {
    auto corners = getCorners();
    Math::Vector3f center(0, 0, 0);
    for (const auto& corner : corners) {
        center = center + corner;
    }
    return center * 0.25f;
}

float Face::getArea() const {
    float voxelSize = getVoxelSize();
    return voxelSize * voxelSize;
}

bool Face::operator==(const Face& other) const {
    return m_voxelPosition == other.m_voxelPosition &&
           m_resolution == other.m_resolution &&
           m_direction == other.m_direction &&
           m_valid == other.m_valid;
}

// Transform implementation
Math::Matrix4f Transform::toMatrix() const {
    Math::Matrix4f mat; // Default constructor should create identity
    
    // TODO: Apply transformations - requires Matrix4f implementation
    // mat = mat * Math::Matrix4f::scale(scale);
    // mat = mat * rotation.toMatrix();
    // mat = mat * Math::Matrix4f::translation(position);
    
    return mat;
}

// Style factory methods
HighlightStyle HighlightStyle::Face() {
    HighlightStyle style;
    style.color = Rendering::Color(1.0f, 1.0f, 0.0f, 0.6f); // Yellow
    style.intensity = 1.0f;
    style.pulseSpeed = 2.0f;
    style.animated = true;
    style.wireframe = false;
    style.lineWidth = 2.0f;
    style.blendMode = BlendMode::Alpha;
    return style;
}

HighlightStyle HighlightStyle::Selection() {
    HighlightStyle style;
    style.color = Rendering::Color(0.0f, 1.0f, 1.0f, 0.5f); // Cyan
    style.intensity = 0.8f;
    style.pulseSpeed = 1.5f;
    style.animated = true;
    style.wireframe = false;
    style.lineWidth = 3.0f;
    style.blendMode = BlendMode::Alpha;
    return style;
}

HighlightStyle HighlightStyle::Group() {
    HighlightStyle style;
    style.color = Rendering::Color(1.0f, 0.5f, 0.0f, 0.4f); // Orange
    style.intensity = 0.7f;
    style.pulseSpeed = 1.0f;
    style.animated = false;
    style.wireframe = true;
    style.lineWidth = 2.0f;
    style.blendMode = BlendMode::Alpha;
    return style;
}

HighlightStyle HighlightStyle::Preview() {
    HighlightStyle style;
    style.color = Rendering::Color(0.0f, 1.0f, 0.0f, 0.3f); // Green
    style.intensity = 0.6f;
    style.pulseSpeed = 0.0f;
    style.animated = false;
    style.wireframe = true;
    style.lineWidth = 3.0f;
    style.blendMode = BlendMode::Alpha;
    return style;
}

OutlineStyle OutlineStyle::VoxelPreview() {
    OutlineStyle style;
    style.color = Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f); // Green
    style.lineWidth = 3.0f;
    style.pattern = LinePattern::Solid;
    style.depthTest = false;
    style.opacity = 0.8f;
    style.animated = false;
    return style;
}

OutlineStyle OutlineStyle::GroupBoundary() {
    OutlineStyle style;
    style.color = Rendering::Color(1.0f, 0.5f, 0.0f, 1.0f); // Orange
    style.lineWidth = 2.0f;
    style.pattern = LinePattern::Dashed;
    style.depthTest = true;
    style.opacity = 0.7f;
    style.animated = false;
    return style;
}

OutlineStyle OutlineStyle::SelectionBox() {
    OutlineStyle style;
    style.color = Rendering::Color(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
    style.lineWidth = 2.0f;
    style.pattern = LinePattern::Solid;
    style.depthTest = false;
    style.opacity = 0.9f;
    style.animated = true;
    style.animationSpeed = 2.0f;
    return style;
}

OutlineStyle OutlineStyle::WorkspaceBounds() {
    OutlineStyle style;
    style.color = Rendering::Color(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    style.lineWidth = 1.0f;
    style.pattern = LinePattern::Dotted;
    style.depthTest = true;
    style.opacity = 0.5f;
    style.animated = false;
    return style;
}

TextStyle TextStyle::Default() {
    TextStyle style;
    style.color = Rendering::Color::White();
    style.size = 16.0f;
    style.alignment = TextAlignment::Left;
    style.background = false;
    return style;
}

TextStyle TextStyle::Header() {
    TextStyle style;
    style.color = Rendering::Color::White();
    style.size = 24.0f;
    style.alignment = TextAlignment::Center;
    style.background = true;
    style.backgroundColor = Rendering::Color(0.0f, 0.0f, 0.0f, 0.7f);
    style.backgroundOpacity = 0.7f;
    return style;
}

TextStyle TextStyle::Debug() {
    TextStyle style;
    style.color = Rendering::Color(0.8f, 0.8f, 0.8f, 1.0f);
    style.size = 14.0f;
    style.alignment = TextAlignment::Left;
    style.background = true;
    style.backgroundColor = Rendering::Color(0.0f, 0.0f, 0.0f, 0.5f);
    style.backgroundOpacity = 0.5f;
    return style;
}

TextStyle TextStyle::Warning() {
    TextStyle style;
    style.color = Rendering::Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    style.size = 18.0f;
    style.alignment = TextAlignment::Center;
    style.background = true;
    style.backgroundColor = Rendering::Color(0.0f, 0.0f, 0.0f, 0.8f);
    style.backgroundOpacity = 0.8f;
    return style;
}

TextStyle TextStyle::Error() {
    TextStyle style;
    style.color = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f); // Red
    style.size = 20.0f;
    style.alignment = TextAlignment::Center;
    style.background = true;
    style.backgroundColor = Rendering::Color(0.0f, 0.0f, 0.0f, 0.9f);
    style.backgroundOpacity = 0.9f;
    return style;
}

} // namespace VisualFeedback
} // namespace VoxelEditor