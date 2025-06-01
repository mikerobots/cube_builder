#include "../include/visual_feedback/FeedbackRenderer.h"

namespace VoxelEditor {
namespace VisualFeedback {

FeedbackRenderer::FeedbackRenderer(Rendering::RenderEngine* renderEngine)
    : m_renderEngine(renderEngine)
    , m_enabled(true)
    , m_faceHighlightEnabled(true)
    , m_voxelPreviewEnabled(true)
    , m_selectionAnimationEnabled(true)
    , m_groupVisualizationEnabled(true)
    , m_workspaceVisualizationEnabled(true)
    , m_debugOverlaysEnabled(false)
    , m_animationsPaused(false)
    , m_animationTime(0.0f)
    , m_animationSpeed(1.0f)
    , m_renderOrder(1000)
    , m_previewResolution(VoxelData::VoxelResolution::Size_32cm)
    , m_memoryUsed(0)
    , m_memoryTotal(0) {
    
    // Initialize components
    m_highlighter = std::make_unique<HighlightRenderer>();
    m_outliner = std::make_unique<OutlineRenderer>();
    m_overlay = std::make_unique<OverlayRenderer>();
    m_faceDetector = std::make_unique<FaceDetector>();
    
    // Initialize workspace bounds
    m_workspaceBounds = Math::BoundingBox(
        Math::Vector3f(0, 0, 0),
        Math::Vector3f(10, 10, 10)
    );
}

FeedbackRenderer::~FeedbackRenderer() = default;

void FeedbackRenderer::renderFaceHighlight(const Face& face, const Rendering::Color& color) {
    if (!m_enabled || !m_faceHighlightEnabled) return;
    
    m_currentFace = face;
    
    HighlightStyle style = HighlightStyle::Face();
    style.color = color;
    
    m_highlighter->clearFaceHighlights();
    m_highlighter->renderFaceHighlight(face, style);
}

void FeedbackRenderer::clearFaceHighlight() {
    m_highlighter->clearFaceHighlights();
    m_currentFace = Face(); // Invalid face
}

void FeedbackRenderer::renderVoxelPreview(const Math::Vector3i& position, 
                                         VoxelData::VoxelResolution resolution, 
                                         const Rendering::Color& color) {
    if (!m_enabled || !m_voxelPreviewEnabled) return;
    
    m_previewPosition = position;
    m_previewResolution = resolution;
    
    // Render outline
    OutlineStyle outlineStyle = OutlineStyle::VoxelPreview();
    outlineStyle.color = color;
    m_outliner->renderVoxelOutline(position, resolution, outlineStyle);
    
    // Render highlight
    HighlightStyle highlightStyle = HighlightStyle::Preview();
    highlightStyle.color = Rendering::Color(color.r, color.g, color.b, 0.3f);
    m_highlighter->renderVoxelHighlight(position, resolution, highlightStyle);
}

void FeedbackRenderer::clearVoxelPreview() {
    m_highlighter->clearVoxelHighlights();
    m_outliner->clearBatch();
}

void FeedbackRenderer::renderSelection(const Selection::SelectionSet& selection, 
                                      const Rendering::Color& color) {
    if (!m_enabled) return;
    
    HighlightStyle style = HighlightStyle::Selection();
    style.color = color;
    style.animated = m_selectionAnimationEnabled;
    
    m_highlighter->clearSelectionHighlights();
    m_highlighter->renderMultiSelection(selection, style);
}

void FeedbackRenderer::renderSelectionBounds(const Math::BoundingBox& bounds, 
                                            const Rendering::Color& color) {
    if (!m_enabled) return;
    
    OutlineStyle style = OutlineStyle::SelectionBox();
    style.color = color;
    style.animated = m_selectionAnimationEnabled;
    
    m_outliner->renderBoxOutline(bounds, style);
}

void FeedbackRenderer::renderGroupOutlines(const std::vector<GroupId>& groups) {
    if (!m_enabled || !m_groupVisualizationEnabled) return;
    
    for (GroupId groupId : groups) {
        auto it = m_groupData.find(groupId);
        if (it != m_groupData.end()) {
            OutlineStyle style = OutlineStyle::GroupBoundary();
            style.color = it->second.color;
            
            m_outliner->renderGroupOutline(it->second.voxels, style);
        }
    }
}

void FeedbackRenderer::renderGroupBounds(GroupId groupId, const Rendering::Color& color) {
    if (!m_enabled || !m_groupVisualizationEnabled) return;
    
    // Store group data for later use
    if (m_groupData.find(groupId) == m_groupData.end()) {
        m_groupData[groupId] = GroupData();
    }
    m_groupData[groupId].color = color;
    
    // TODO: Calculate group bounding box and render outline
}

void FeedbackRenderer::renderWorkspaceBounds(const Math::BoundingBox& workspace, 
                                            const Rendering::Color& color) {
    if (!m_enabled || !m_workspaceVisualizationEnabled) return;
    
    m_workspaceBounds = workspace;
    
    OutlineStyle style = OutlineStyle::WorkspaceBounds();
    style.color = color;
    
    m_outliner->renderBoxOutline(workspace, style);
}

void FeedbackRenderer::renderGridLines(VoxelData::VoxelResolution resolution, float opacity) {
    if (!m_enabled || !m_workspaceVisualizationEnabled) return;
    
    Math::Vector3f center = m_workspaceBounds.getCenter();
    float extent = (m_workspaceBounds.max - m_workspaceBounds.min).length() * 0.5f;
    
    // This would be rendered in the overlay system
    // m_overlay->renderGrid(resolution, center, extent, camera);
}

void FeedbackRenderer::renderPerformanceMetrics(const RenderStats& stats) {
    if (!m_enabled || !m_debugOverlaysEnabled) return;
    
    m_lastStats = stats;
    
    // This would be rendered during the overlay pass
    // m_overlay->renderPerformanceMetrics(stats, Vector2f(10, 10));
}

void FeedbackRenderer::renderMemoryUsage(size_t used, size_t total) {
    if (!m_enabled || !m_debugOverlaysEnabled) return;
    
    m_memoryUsed = used;
    m_memoryTotal = total;
    
    // This would be rendered during the overlay pass
    // m_overlay->renderMemoryUsage(used, total, Vector2f(10, 100));
}

void FeedbackRenderer::update(float deltaTime) {
    if (!m_enabled) return;
    
    updateAnimations(deltaTime);
    
    // Update components
    m_highlighter->update(deltaTime);
    m_outliner->update(deltaTime);
}

void FeedbackRenderer::render(const Camera::Camera& camera, const Rendering::RenderContext& context) {
    if (!m_enabled) return;
    
    // Begin overlay frame
    m_overlay->beginFrame(context.screenWidth, context.screenHeight);
    
    // Render components in order
    renderHighlights(camera);
    renderOutlines(camera);
    renderOverlays(camera, context);
    
    // End overlay frame
    m_overlay->endFrame();
}

void FeedbackRenderer::updateAnimations(float deltaTime) {
    if (!m_animationsPaused) {
        m_animationTime += deltaTime * m_animationSpeed;
    }
}

void FeedbackRenderer::renderHighlights(const Camera::Camera& camera) {
    if (m_faceHighlightEnabled || m_voxelPreviewEnabled) {
        m_highlighter->render(camera);
    }
}

void FeedbackRenderer::renderOutlines(const Camera::Camera& camera) {
    m_outliner->renderBatch(camera);
}

void FeedbackRenderer::renderOverlays(const Camera::Camera& camera, const Rendering::RenderContext& context) {
    if (m_workspaceVisualizationEnabled) {
        // Render grid
        Math::Vector3f center = m_workspaceBounds.getCenter();
        float extent = (m_workspaceBounds.max - m_workspaceBounds.min).length() * 0.5f;
        m_overlay->renderGrid(m_previewResolution, center, extent, camera);
        
        // Render workspace indicator
        Math::Vector3f size = m_workspaceBounds.max - m_workspaceBounds.min;
        m_overlay->renderWorkspaceIndicator(size, Math::Vector2f(10, context.screenHeight - 50));
    }
    
    if (m_debugOverlaysEnabled) {
        // Render performance metrics
        m_overlay->renderPerformanceMetrics(m_lastStats, Math::Vector2f(10, 10));
        
        // Render memory usage
        if (m_memoryTotal > 0) {
            m_overlay->renderMemoryUsage(m_memoryUsed, m_memoryTotal, Math::Vector2f(10, 120));
        }
        
        // Render camera info
        m_overlay->renderCameraInfo(camera, Math::Vector2f(10, 180));
    }
    
    // Render resolution indicator
    m_overlay->renderResolutionIndicator(m_previewResolution, 
                                        Math::Vector2f(context.screenWidth - 150, 10));
}

} // namespace VisualFeedback
} // namespace VoxelEditor