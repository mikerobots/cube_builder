#pragma once

#include <memory>
#include "FeedbackTypes.h"
#include "../../input/PlacementValidation.h"
#include "../../voxel_data/VoxelTypes.h"
#include "../../voxel_data/VoxelGrid.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/Vector3f.h"

namespace VoxelEditor {

// Forward declarations
namespace Rendering {
    class Color;
}

namespace VisualFeedback {

// Forward declarations
class OutlineRenderer;
class FeedbackRenderer;

/**
 * @brief Manages voxel placement preview rendering and state
 * 
 * This class handles:
 * - Tracking current preview position
 * - Updating preview on mouse movement
 * - Color switching between valid (green) and invalid (red)
 * - Clearing preview when not hovering
 */
class PreviewManager {
public:
    PreviewManager();
    ~PreviewManager();
    
    // Preview state management
    void setPreviewPosition(const Math::Vector3i& position, VoxelData::VoxelResolution resolution);
    void setValidationResult(Input::PlacementValidationResult result);
    void clearPreview();
    
    // State queries
    bool hasPreview() const { return m_hasPreview; }
    Math::Vector3i getPreviewPosition() const { return m_previewPosition; }
    VoxelData::VoxelResolution getPreviewResolution() const { return m_previewResolution; }
    bool isValid() const { return m_isValid; }
    
    // Validation methods (for requirements tests)
    bool isValidIncrementPosition(const Math::IncrementCoordinates& position) const;
    bool isValidPlacement(const Math::IncrementCoordinates& position, VoxelData::VoxelResolution resolution, 
                         const VoxelData::VoxelGrid& grid) const;
    Rendering::Color getPreviewColor(bool isValid) const;
    void updatePreview(const Math::IncrementCoordinates& position, VoxelData::VoxelResolution resolution, bool isValid);
    
    // Rendering
    void render(OutlineRenderer& renderer) const;
    void renderWithFeedback(FeedbackRenderer& renderer) const;
    
    // Configuration
    void setValidColor(const Rendering::Color& color);
    void setInvalidColor(const Rendering::Color& color);
    void setLineWidth(float width) { m_lineWidth = width; }
    void setAnimated(bool animated) { m_animated = animated; }
    void setAnimationSpeed(float speed) { m_animationSpeed = speed; }
    
    // Update for animation
    void update(float deltaTime);
    
    // Mouse position tracking for automatic clearing
    void updateMousePosition(const Math::Vector2f& mousePos);
    void setAutoClearDistance(float distance) { m_autoClearDistance = distance; }
    
private:
    // Preview state
    bool m_hasPreview;
    Math::Vector3i m_previewPosition;
    VoxelData::VoxelResolution m_previewResolution;
    bool m_isValid;
    Input::PlacementValidationResult m_validationResult;
    
    // Visual settings
    Rendering::Color m_validColor;
    Rendering::Color m_invalidColor;
    float m_lineWidth;
    bool m_animated;
    float m_animationSpeed;
    float m_animationTime;
    
    // Auto-clear settings
    Math::Vector2f m_lastMousePosition;
    float m_autoClearDistance;
    float m_timeSinceLastUpdate;
    
    // Helper methods
    OutlineStyle createOutlineStyle() const;
    Rendering::Color getCurrentColor() const;
};

} // namespace VisualFeedback
} // namespace VoxelEditor