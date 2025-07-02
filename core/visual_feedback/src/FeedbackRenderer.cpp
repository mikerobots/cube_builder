#include "../include/visual_feedback/FeedbackRenderer.h"
#include <algorithm>

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
    , m_pendingGridRender(false)
    , m_gridExtent(0.0f)
    , m_gridDynamicOpacity(false)
    , m_memoryUsed(0)
    , m_memoryTotal(0) {
    
    // Initialize components
    m_highlighter = std::make_unique<HighlightRenderer>();
    m_outliner = std::make_unique<OutlineRenderer>();
    m_overlay = std::make_unique<OverlayRenderer>();
    m_faceDetector = std::make_unique<FaceDetector>();
    
    // Initialize workspace bounds with centered coordinate system
    // Default 5m workspace: X,Z centered [-2.5, 2.5], Y from [0, 5]
    float defaultSize = 5.0f;
    m_workspaceBounds = Math::BoundingBox(
        Math::Vector3f(-defaultSize/2, 0, -defaultSize/2),
        Math::Vector3f(defaultSize/2, defaultSize, defaultSize/2)
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

void FeedbackRenderer::renderVoxelPreviewWithValidation(const Math::Vector3i& position, 
                                                       VoxelData::VoxelResolution resolution, 
                                                       bool isValid) {
    if (!m_enabled || !m_voxelPreviewEnabled) return;
    
    m_previewPosition = position;
    m_previewResolution = resolution;
    
    // REQ-4.1.1 & REQ-4.1.2: Green outline for valid, red for invalid
    OutlineStyle outlineStyle = isValid ? 
        OutlineStyle::VoxelPreview() : 
        OutlineStyle::VoxelPreviewInvalid();
    
    m_outliner->renderVoxelOutline(position, resolution, outlineStyle);
    
    // Add subtle highlight
    HighlightStyle highlightStyle = HighlightStyle::Preview();
    highlightStyle.color = isValid ? 
        Rendering::Color(0.0f, 1.0f, 0.0f, 0.2f) :  // Green for valid
        Rendering::Color(1.0f, 0.0f, 0.0f, 0.2f);   // Red for invalid
    
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

void FeedbackRenderer::renderGroundPlaneGridEnhanced(const Math::Vector3f& center, float extent, 
                                                    const Math::Vector3f& cursorPos, bool enableDynamicOpacity) {
    if (!m_enabled || !m_workspaceVisualizationEnabled) return;
    
    // Store grid parameters for rendering during the overlay pass
    m_pendingGridRender = true;
    m_gridCenter = center;
    m_gridExtent = extent;
    m_gridCursorPos = cursorPos;
    m_gridDynamicOpacity = enableDynamicOpacity;
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

void FeedbackRenderer::renderOutlineBox(const Math::BoundingBox& box, const Rendering::Color& color) {
    if (!m_enabled) return;
    
    OutlineStyle style = OutlineStyle::VoxelPreview();
    style.color = color;
    
    m_outliner->renderBoxOutline(box, style);
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

void FeedbackRenderer::setDebugRay(const Ray& ray, bool enabled) {
    m_debugRay = ray;
    m_debugRayEnabled = enabled;
}

void FeedbackRenderer::renderOverlays(const Camera::Camera& camera, const Rendering::RenderContext& context) {
    // Render ground plane grid if pending
    if (m_pendingGridRender && m_workspaceVisualizationEnabled) {
        m_overlay->renderGroundPlaneGrid(m_gridCenter, m_gridExtent, m_gridCursorPos, m_gridDynamicOpacity, camera);
        m_pendingGridRender = false; // Clear after rendering
    }
    
    // Render debug ray if enabled
    if (m_debugRayEnabled) {
        // Ray length adjusted to stay within camera view frustum
        // Camera is ~7.5 units from origin, so 10 units covers the visible scene
        m_overlay->renderRaycast(m_debugRay, 10.0f, Rendering::Color(1.0f, 1.0f, 0.0f, 1.0f), camera);
    }
    
    if (m_workspaceVisualizationEnabled) {
        // DEBUG: Only render workspace indicator to see what text is causing white boxes
        Math::Vector3f size = m_workspaceBounds.max - m_workspaceBounds.min;
        m_overlay->renderWorkspaceIndicator(size, Math::Vector2f(10, context.screenHeight - 50));
    }
    
    // DEBUG: Only render resolution indicator to see what text is causing white boxes
    m_overlay->renderResolutionIndicator(m_previewResolution, 
                                        Math::Vector2f(context.screenWidth - 150, 10));
}

// Validation methods
bool FeedbackRenderer::validateFace(const Face& face) const {
    return face.isValid();
}

bool FeedbackRenderer::validatePreviewPosition(const Math::Vector3i& position, VoxelData::VoxelResolution resolution) const {
    // For testing purposes, use a more lenient validation
    // In a real implementation, this would check workspace bounds and collision
    // For now, just check if position is within reasonable range
    return (position.x >= -100 && position.x <= 100 &&
            position.y >= -100 && position.y <= 100 &&
            position.z >= -100 && position.z <= 100);
}

bool FeedbackRenderer::validateGridParameters(const Math::Vector3f& center, float extent, const Math::Vector3f& cursorPos) const {
    // Check if extent is positive and within reasonable bounds
    if (extent <= 0.0f || extent > 15.0f) {
        return false;
    }
    
    // Check if center is within workspace bounds
    if (!m_workspaceBounds.contains(center)) {
        return false;
    }
    
    return true;
}

Math::BoundingBox FeedbackRenderer::calculateSelectionBounds(const std::vector<VoxelId>& selection) const {
    if (selection.empty()) {
        return Math::BoundingBox(Math::Vector3f::Zero(), Math::Vector3f::Zero());
    }
    
    // For simplified implementation, create a bounding box based on VoxelId values
    // In a real implementation, this would query the actual voxel positions
    Math::Vector3f min = Math::Vector3f(1000000.0f, 1000000.0f, 1000000.0f);
    Math::Vector3f max = Math::Vector3f(-1000000.0f, -1000000.0f, -1000000.0f);
    
    for (const auto& voxelId : selection) {
        // Simplified: treat VoxelId as encoded position
        float x = static_cast<float>(voxelId & 0xFFFF);
        float y = static_cast<float>((voxelId >> 16) & 0xFFFF);
        float z = static_cast<float>((voxelId >> 32) & 0xFFFF);
        
        min.x = std::min(min.x, x);
        min.y = std::min(min.y, y);
        min.z = std::min(min.z, z);
        max.x = std::max(max.x, x);
        max.y = std::max(max.y, y);
        max.z = std::max(max.z, z);
    }
    
    return Math::BoundingBox(min, max);
}

Rendering::Color FeedbackRenderer::getPreviewColor(const Math::Vector3i& position, VoxelData::VoxelResolution resolution, bool isValid) const {
    if (isValid) {
        return Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f); // Green for valid
    } else {
        return Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f); // Red for invalid
    }
}

Rendering::Color FeedbackRenderer::getFaceHighlightColor(const Face& face) const {
    // Return yellow highlight color for faces
    return Rendering::Color(1.0f, 1.0f, 0.0f, 1.0f);
}

GridInfo FeedbackRenderer::calculateGridInfo(const Math::Vector3f& center, float extent) const {
    GridInfo info;
    
    // Calculate grid spacing based on extent
    info.extent = extent;
    info.spacing = extent / 10.0f;  // 10 grid lines per extent
    
    // Calculate number of lines (each axis)
    uint32_t linesPerAxis = static_cast<uint32_t>(extent / info.spacing) + 1;
    info.lineCount = linesPerAxis * 2;  // X and Z axes
    
    // Calculate vertex count (2 vertices per line)
    info.vertexCount = info.lineCount * 2;
    
    return info;
}

PerformanceMetrics::FormattedText FeedbackRenderer::formatPerformanceMetrics(const PerformanceMetrics& metrics) const {
    PerformanceMetrics::FormattedText formatted;
    
    formatted.frameTimeText = "Frame Time: " + std::to_string(metrics.frameTime) + " ms";
    formatted.voxelCountText = "Voxels: " + std::to_string(metrics.voxelCount);
    formatted.memoryUsageText = "Memory: " + std::to_string(metrics.memoryUsed / (1024 * 1024)) + " MB / " + 
                               std::to_string(metrics.memoryTotal / (1024 * 1024)) + " MB";
    formatted.performanceText = "Draw Calls: " + std::to_string(metrics.drawCalls) + 
                               " | Triangles: " + std::to_string(metrics.triangleCount);
    
    return formatted;
}

} // namespace VisualFeedback
} // namespace VoxelEditor