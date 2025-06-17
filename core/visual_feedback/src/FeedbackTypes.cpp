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
    , m_valid(true)
    , m_isGroundPlane(false)
    , m_groundHitPoint(0, 0, 0) {
}

// Enhancement: Create a ground plane face
Face Face::GroundPlane(const Math::Vector3f& hitPoint) {
    Face face;
    face.m_valid = true;
    face.m_isGroundPlane = true;
    face.m_groundHitPoint = hitPoint;
    face.m_direction = FaceDirection::PositiveY; // Ground plane faces up
    face.m_resolution = VoxelData::VoxelResolution::Size_1cm; // Default resolution
    
    // Convert hit point to nearest grid position for voxel position
    // This helps with face ID generation
    face.m_voxelPosition = Math::Vector3i(
        static_cast<int>(std::round(hitPoint.x / 0.01f)),
        0,
        static_cast<int>(std::round(hitPoint.z / 0.01f))
    );
    
    return face;
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
    // Enhancement: Handle ground plane
    if (m_isGroundPlane) {
        return m_groundHitPoint;
    }
    
    float voxelSize = getVoxelSize();
    
    // Face stores grid coordinates, need to convert to world coordinates
    // accounting for centered workspace
    // For now, use the simple multiplication approach
    // TODO: This should use VoxelGrid::gridToWorld() for proper centered workspace handling
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
    // Enhancement: Handle ground plane comparison
    if (m_isGroundPlane != other.m_isGroundPlane) {
        return false;
    }
    
    if (m_isGroundPlane) {
        // For ground plane, compare hit points
        const float epsilon = 0.001f;
        return std::abs(m_groundHitPoint.x - other.m_groundHitPoint.x) < epsilon &&
               std::abs(m_groundHitPoint.y - other.m_groundHitPoint.y) < epsilon &&
               std::abs(m_groundHitPoint.z - other.m_groundHitPoint.z) < epsilon;
    }
    
    return m_voxelPosition == other.m_voxelPosition &&
           m_resolution == other.m_resolution &&
           m_direction == other.m_direction &&
           m_valid == other.m_valid;
}

// Transform implementation
Math::Matrix4f Transform::toMatrix() const {
    // Build transformation matrix manually
    // First create rotation matrix from quaternion
    float xx = rotation.x * rotation.x;
    float yy = rotation.y * rotation.y;
    float zz = rotation.z * rotation.z;
    float xy = rotation.x * rotation.y;
    float xz = rotation.x * rotation.z;
    float yz = rotation.y * rotation.z;
    float wx = rotation.w * rotation.x;
    float wy = rotation.w * rotation.y;
    float wz = rotation.w * rotation.z;
    
    // Build transformation matrix using proper array indexing
    // Matrix4f uses column-major order: m[col * 4 + row]
    Math::Matrix4f mat;
    
    // Column 0
    mat[0] = (1.0f - 2.0f * (yy + zz)) * scale.x;
    mat[1] = (2.0f * (xy + wz)) * scale.y;
    mat[2] = (2.0f * (xz - wy)) * scale.z;
    mat[3] = 0.0f;
    
    // Column 1
    mat[4] = (2.0f * (xy - wz)) * scale.x;
    mat[5] = (1.0f - 2.0f * (xx + zz)) * scale.y;
    mat[6] = (2.0f * (yz + wx)) * scale.z;
    mat[7] = 0.0f;
    
    // Column 2
    mat[8] = (2.0f * (xz + wy)) * scale.x;
    mat[9] = (2.0f * (yz - wx)) * scale.y;
    mat[10] = (1.0f - 2.0f * (xx + yy)) * scale.z;
    mat[11] = 0.0f;
    
    // Column 3 (translation)
    mat[12] = position.x;
    mat[13] = position.y;
    mat[14] = position.z;
    mat[15] = 1.0f;
    
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

OutlineStyle OutlineStyle::VoxelPreviewInvalid() {
    OutlineStyle style;
    style.color = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f); // Red
    style.lineWidth = 3.0f;
    style.pattern = LinePattern::Solid;
    style.depthTest = false;
    style.opacity = 0.8f;
    style.animated = true; // Pulse to indicate invalid
    style.animationSpeed = 2.0f;
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