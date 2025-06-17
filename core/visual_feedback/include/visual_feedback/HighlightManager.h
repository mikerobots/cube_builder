#pragma once

#include "HighlightRenderer.h"
#include "FeedbackTypes.h"
#include <memory>

namespace VoxelEditor {
namespace VisualFeedback {

/**
 * @brief Manages highlighting with single face constraint and smooth transitions
 * 
 * This class wraps HighlightRenderer to ensure only one face is highlighted
 * at a time and provides smooth transition effects.
 */
class HighlightManager {
public:
    HighlightManager();
    ~HighlightManager() = default;
    
    /**
     * @brief Set the currently highlighted face
     * @param face The face to highlight (or invalid face to clear)
     */
    void setHighlightedFace(const Face& face);
    
    /**
     * @brief Clear the current face highlight
     */
    void clearFaceHighlight();
    
    /**
     * @brief Get the currently highlighted face
     * @return The current face or invalid face if none
     */
    const Face& getCurrentFace() const { return m_currentFace; }
    
    /**
     * @brief Check if a face is currently highlighted
     * @return true if a face is highlighted
     */
    bool hasFaceHighlight() const { return m_currentFace.isValid(); }
    
    /**
     * @brief Update animations and transitions
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime);
    
    /**
     * @brief Render all highlights
     * @param camera The camera for rendering
     */
    void render(const Camera::Camera& camera);
    
    /**
     * @brief Get the underlying highlight renderer
     * @return Pointer to the highlight renderer
     */
    HighlightRenderer* getRenderer() { return m_renderer.get(); }
    
    /**
     * @brief Set animation enabled state
     * @param enabled true to enable animations
     */
    void setAnimationEnabled(bool enabled);
    
private:
    std::unique_ptr<HighlightRenderer> m_renderer;
    Face m_currentFace;
    Face m_previousFace;
    float m_transitionTime;
    static constexpr float TransitionDuration = 0.15f; // 150ms transition
};

} // namespace VisualFeedback
} // namespace VoxelEditor