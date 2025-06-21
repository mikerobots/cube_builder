#include "../include/visual_feedback/PreviewManager.h"
#include "../include/visual_feedback/OutlineRenderer.h"
#include "../include/visual_feedback/FeedbackRenderer.h"
#include "../../rendering/RenderTypes.h"

namespace VoxelEditor {
namespace VisualFeedback {

PreviewManager::PreviewManager()
    : m_hasPreview(false)
    , m_previewPosition(0, 0, 0)
    , m_previewResolution(VoxelData::VoxelResolution::Size_1cm)
    , m_isValid(true)
    , m_validationResult(Input::PlacementValidationResult::Valid)
    , m_validColor(0.0f, 1.0f, 0.0f, 1.0f)    // Green
    , m_invalidColor(1.0f, 0.0f, 0.0f, 1.0f)  // Red
    , m_lineWidth(3.0f)
    , m_animated(false)
    , m_animationSpeed(1.0f)
    , m_animationTime(0.0f)
    , m_lastMousePosition(0.0f, 0.0f)
    , m_autoClearDistance(500.0f)  // Clear if mouse moves more than 500 pixels
    , m_timeSinceLastUpdate(0.0f) {
}

PreviewManager::~PreviewManager() = default;

void PreviewManager::setPreviewPosition(const Math::Vector3i& position, VoxelData::VoxelResolution resolution) {
    m_previewPosition = position;
    m_previewResolution = resolution;
    m_hasPreview = true;
    m_timeSinceLastUpdate = 0.0f;
}

void PreviewManager::setValidationResult(Input::PlacementValidationResult result) {
    m_validationResult = result;
    m_isValid = (result == Input::PlacementValidationResult::Valid);
}

void PreviewManager::clearPreview() {
    m_hasPreview = false;
    m_timeSinceLastUpdate = 0.0f;
}

void PreviewManager::render(OutlineRenderer& renderer) const {
    if (!m_hasPreview) return;
    
    OutlineStyle style = createOutlineStyle();
    renderer.renderVoxelOutline(m_previewPosition, m_previewResolution, style);
}

void PreviewManager::renderWithFeedback(FeedbackRenderer& renderer) const {
    if (!m_hasPreview) return;
    
    // Use the feedback renderer's voxel preview method
    renderer.renderVoxelPreview(m_previewPosition, m_previewResolution, getCurrentColor());
}

void PreviewManager::setValidColor(const Rendering::Color& color) {
    m_validColor = color;
}

void PreviewManager::setInvalidColor(const Rendering::Color& color) {
    m_invalidColor = color;
}

void PreviewManager::update(float deltaTime) {
    if (m_animated && m_hasPreview) {
        m_animationTime += deltaTime * m_animationSpeed;
    }
    
    m_timeSinceLastUpdate += deltaTime;
    
    // Auto-clear if preview hasn't been updated in a while
    if (m_hasPreview && m_timeSinceLastUpdate > 1.0f) {
        clearPreview();
    }
}

void PreviewManager::updateMousePosition(const Math::Vector2f& mousePos) {
    // Check if mouse has moved significantly
    float distance = (mousePos - m_lastMousePosition).length();
    
    if (distance > m_autoClearDistance && m_hasPreview) {
        clearPreview();
    }
    
    m_lastMousePosition = mousePos;
}

OutlineStyle PreviewManager::createOutlineStyle() const {
    OutlineStyle style;
    style.color = getCurrentColor();
    style.lineWidth = m_lineWidth;
    style.pattern = LinePattern::Solid;
    style.depthTest = false;  // Always visible
    style.opacity = 0.8f;
    style.animated = m_animated;
    style.animationSpeed = m_animationSpeed;
    
    // For invalid placements, use dashed pattern
    if (!m_isValid) {
        style.pattern = LinePattern::Dashed;
    }
    
    return style;
}

Rendering::Color PreviewManager::getCurrentColor() const {
    if (m_isValid) {
        return m_validColor;
    } else {
        // For invalid, we can pulse the color for better visibility
        if (m_animated) {
            float pulse = (std::sin(m_animationTime * 2.0f) + 1.0f) * 0.5f;
            return Rendering::Color(
                m_invalidColor.r,
                m_invalidColor.g * pulse,
                m_invalidColor.b * pulse,
                m_invalidColor.a
            );
        }
        return m_invalidColor;
    }
}

// Additional validation methods for requirements tests
bool PreviewManager::isValidIncrementPosition(const Math::IncrementCoordinates& position) const {
    // Simple bounds check - ensure position is within reasonable range
    const Math::Vector3i pos = position.value();
    return (pos.x >= -1000 && pos.x <= 1000 &&
            pos.y >= -1000 && pos.y <= 1000 &&
            pos.z >= -1000 && pos.z <= 1000);
}

bool PreviewManager::isValidPlacement(const Math::IncrementCoordinates& position, VoxelData::VoxelResolution resolution, 
                                     const VoxelData::VoxelGrid& grid) const {
    // Check if position is valid and doesn't collide with existing voxels
    if (!isValidIncrementPosition(position)) {
        return false;
    }
    
    // Check if there's already a voxel at this position
    return !grid.getVoxel(position);
}

Rendering::Color PreviewManager::getPreviewColor(bool isValid) const {
    return isValid ? m_validColor : m_invalidColor;
}

void PreviewManager::updatePreview(const Math::IncrementCoordinates& position, VoxelData::VoxelResolution resolution, bool isValid) {
    m_previewPosition = position.value();
    m_previewResolution = resolution;
    m_isValid = isValid;
    m_hasPreview = true;
}

} // namespace VisualFeedback
} // namespace VoxelEditor