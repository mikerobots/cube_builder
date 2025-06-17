#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "FeedbackTypes.h"
#include "HighlightRenderer.h"
#include "OutlineRenderer.h"
#include "OverlayRenderer.h"
#include "FaceDetector.h"
#include "SelectionSet.h"
#include "Camera.h"
#include "RenderContext.h"

namespace VoxelEditor {
namespace VisualFeedback {

class FeedbackRenderer {
public:
    FeedbackRenderer(Rendering::RenderEngine* renderEngine);
    ~FeedbackRenderer();
    
    // Face highlighting
    void renderFaceHighlight(const Face& face, 
                           const Rendering::Color& color = Rendering::Color(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
    void clearFaceHighlight();
    void setFaceHighlightEnabled(bool enabled) { m_faceHighlightEnabled = enabled; }
    bool isFaceHighlightEnabled() const { return m_faceHighlightEnabled; }
    
    // Voxel preview - Enhanced for requirements
    void renderVoxelPreview(const Math::Vector3i& position, 
                           VoxelData::VoxelResolution resolution, 
                           const Rendering::Color& color = Rendering::Color::Green());
    void renderVoxelPreviewWithValidation(const Math::Vector3i& position, 
                                        VoxelData::VoxelResolution resolution, 
                                        bool isValid);
    void clearVoxelPreview();
    void setVoxelPreviewEnabled(bool enabled) { m_voxelPreviewEnabled = enabled; }
    bool isVoxelPreviewEnabled() const { return m_voxelPreviewEnabled; }
    
    // Selection visualization
    void renderSelection(const Selection::SelectionSet& selection, 
                        const Rendering::Color& color = Rendering::Color(0.0f, 1.0f, 1.0f, 1.0f)); // Cyan
    void renderSelectionBounds(const Math::BoundingBox& bounds, 
                              const Rendering::Color& color = Rendering::Color::White());
    void setSelectionAnimationEnabled(bool enabled) { m_selectionAnimationEnabled = enabled; }
    bool isSelectionAnimationEnabled() const { return m_selectionAnimationEnabled; }
    
    // Group visualization
    void renderGroupOutlines(const std::vector<GroupId>& groups);
    void renderGroupBounds(GroupId groupId, const Rendering::Color& color);
    void setGroupVisualizationEnabled(bool enabled) { m_groupVisualizationEnabled = enabled; }
    bool isGroupVisualizationEnabled() const { return m_groupVisualizationEnabled; }
    
    // Workspace visualization - Enhanced for requirements
    void renderWorkspaceBounds(const Math::BoundingBox& workspace, 
                              const Rendering::Color& color = Rendering::Color(0.5f, 0.5f, 0.5f, 1.0f)); // Gray
    void renderGridLines(VoxelData::VoxelResolution resolution, float opacity = 0.1f);
    void renderGroundPlaneGridEnhanced(const Math::Vector3f& center, float extent, 
                                     const Math::Vector3f& cursorPos, bool enableDynamicOpacity = true);
    void setWorkspaceVisualizationEnabled(bool enabled) { m_workspaceVisualizationEnabled = enabled; }
    bool isWorkspaceVisualizationEnabled() const { return m_workspaceVisualizationEnabled; }
    
    // Performance overlays
    void renderPerformanceMetrics(const RenderStats& stats);
    void renderMemoryUsage(size_t used, size_t total);
    void setDebugOverlaysEnabled(bool enabled) { m_debugOverlaysEnabled = enabled; }
    bool areDebugOverlaysEnabled() const { return m_debugOverlaysEnabled; }
    
    // Animation control
    void update(float deltaTime);
    void setAnimationSpeed(float speed) { m_animationSpeed = speed; }
    float getAnimationSpeed() const { return m_animationSpeed; }
    void pauseAnimations(bool paused) { m_animationsPaused = paused; }
    bool areAnimationsPaused() const { return m_animationsPaused; }
    
    // Rendering
    void render(const Camera::Camera& camera, const Rendering::RenderContext& context);
    void setRenderOrder(int order) { m_renderOrder = order; }
    int getRenderOrder() const { return m_renderOrder; }
    
    // Global enable/disable
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    // Face detection
    FaceDetector& getFaceDetector() { return *m_faceDetector; }
    const FaceDetector& getFaceDetector() const { return *m_faceDetector; }
    
    // Component access
    HighlightRenderer& getHighlightRenderer() { return *m_highlighter; }
    OutlineRenderer& getOutlineRenderer() { return *m_outliner; }
    OverlayRenderer& getOverlayRenderer() { return *m_overlay; }
    
private:
    std::unique_ptr<HighlightRenderer> m_highlighter;
    std::unique_ptr<OutlineRenderer> m_outliner;
    std::unique_ptr<OverlayRenderer> m_overlay;
    std::unique_ptr<FaceDetector> m_faceDetector;
    
    Rendering::RenderEngine* m_renderEngine;
    
    // State flags
    bool m_enabled;
    bool m_faceHighlightEnabled;
    bool m_voxelPreviewEnabled;
    bool m_selectionAnimationEnabled;
    bool m_groupVisualizationEnabled;
    bool m_workspaceVisualizationEnabled;
    bool m_debugOverlaysEnabled;
    bool m_animationsPaused;
    
    // Animation state
    float m_animationTime;
    float m_animationSpeed;
    
    // Render order
    int m_renderOrder;
    
    // Current state
    Face m_currentFace;
    Math::Vector3i m_previewPosition;
    VoxelData::VoxelResolution m_previewResolution;
    Math::BoundingBox m_workspaceBounds;
    
    // Group data (temporary - should come from Groups system)
    struct GroupData {
        std::vector<VoxelId> voxels;
        Rendering::Color color;
    };
    std::unordered_map<GroupId, GroupData> m_groupData;
    
    // Performance metrics
    RenderStats m_lastStats;
    size_t m_memoryUsed;
    size_t m_memoryTotal;
    
    // Helper methods
    void updateAnimations(float deltaTime);
    void renderHighlights(const Camera::Camera& camera);
    void renderOutlines(const Camera::Camera& camera);
    void renderOverlays(const Camera::Camera& camera, const Rendering::RenderContext& context);
};

} // namespace VisualFeedback
} // namespace VoxelEditor