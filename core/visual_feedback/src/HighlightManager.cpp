#include "../include/visual_feedback/HighlightManager.h"

namespace VoxelEditor {
namespace VisualFeedback {

HighlightManager::HighlightManager()
    : m_renderer(std::make_unique<HighlightRenderer>())
    , m_transitionTime(0.0f) {
}

void HighlightManager::setHighlightedFace(const Face& face) {
    if (face == m_currentFace) {
        return; // No change
    }
    
    // Clear existing highlights
    m_renderer->clearFaceHighlights();
    
    // Store previous face for potential transition effects
    m_previousFace = m_currentFace;
    m_currentFace = face;
    m_transitionTime = 0.0f;
    
    // Add new highlight if face is valid
    if (face.isValid()) {
        // Use default yellow highlight style
        HighlightStyle style = HighlightStyle::Face();
        m_renderer->renderFaceHighlight(face, style);
    }
}

void HighlightManager::clearFaceHighlight() {
    m_renderer->clearFaceHighlights();
    m_previousFace = m_currentFace;
    m_currentFace = Face(); // Invalid face
    m_transitionTime = 0.0f;
}

void HighlightManager::update(float deltaTime) {
    // Update transition time
    if (m_transitionTime < TransitionDuration) {
        m_transitionTime += deltaTime;
    }
    
    // Update renderer animations
    m_renderer->update(deltaTime);
}

void HighlightManager::render(const Camera::Camera& camera) {
    m_renderer->render(camera);
}

void HighlightManager::setAnimationEnabled(bool enabled) {
    m_renderer->setGlobalAnimationEnabled(enabled);
}

} // namespace VisualFeedback
} // namespace VoxelEditor