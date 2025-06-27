#pragma once

#include <glm/glm.hpp>
#include "visual_feedback/FeedbackTypes.h"

namespace VoxelEditor {

// Forward declarations
namespace VoxelData {
    class VoxelDataManager;
    enum class VoxelResolution : uint8_t;
}

namespace Camera {
    class CameraController;
}

namespace Input {
    class InputManager;
}

namespace VisualFeedback {
    class FeedbackRenderer;
}

namespace UndoRedo {
    class HistoryManager;
}

namespace Math {
    struct Ray;
}

// CLI namespace removed - now directly in VoxelEditor

class Application;
class RenderWindow;

class MouseInteraction {
public:
    MouseInteraction(Application* app);
    ~MouseInteraction() = default;
    
    // Initialize mouse interaction
    void initialize();
    
    // Update per frame
    void update();
    
    // Mouse event handlers
    void onMouseMove(float x, float y);
    void onMouseClick(int button, bool pressed, float x, float y);
    void onMouseScroll(float deltaX, float deltaY, bool ctrlPressed, bool shiftPressed);
    
    // Get current hover info
    bool hasHoverFace() const { return m_hasHoverFace; }
    const VisualFeedback::Face& getHoverFace() const { return m_hoverFace; }
    
private:
    Application* m_app;
    
    // Cached system pointers
    VoxelData::VoxelDataManager* m_voxelManager = nullptr;
    Camera::CameraController* m_cameraController = nullptr;
    Input::InputManager* m_inputManager = nullptr;
    VisualFeedback::FeedbackRenderer* m_feedbackRenderer = nullptr;
    UndoRedo::HistoryManager* m_historyManager = nullptr;
    RenderWindow* m_renderWindow = nullptr;
    
    // Mouse state
    glm::vec2 m_mousePos;
    bool m_mousePressed = false;
    bool m_middlePressed = false;
    glm::vec2 m_dragStart;
    
    // Hover state
    bool m_hasHoverFace = false;
    VisualFeedback::Face m_hoverFace;
    glm::ivec3 m_previewPos;
    
    // Camera control
    bool m_orbitMode = false;
    bool m_panMode = false;
    
    // Ray visualization for debugging
    bool m_rayVisualizationEnabled = false;
    
    // Helper methods
    bool performRaycast(const Math::Ray& ray, VisualFeedback::Face& hitFace) const;
    void updateHoverState();
    void placeVoxel();
    void removeVoxel();
    void centerCameraOnVoxels();
    
public:
    // Debug ray visualization
    void setRayVisualizationEnabled(bool enabled) { m_rayVisualizationEnabled = enabled; }
    bool isRayVisualizationEnabled() const { return m_rayVisualizationEnabled; }
    
protected:
    // Made protected for testing
    Math::Ray getMouseRay(float x, float y) const;
    glm::ivec3 getPlacementPosition(const VisualFeedback::Face& face) const;
};

} // namespace VoxelEditor