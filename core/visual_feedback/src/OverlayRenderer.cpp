#include "../include/visual_feedback/OverlayRenderer.h"
#include <sstream>
#include <iomanip>

namespace VoxelEditor {
namespace VisualFeedback {

OverlayRenderer::OverlayRenderer()
    : m_frameActive(false) {
    
    initializeTextRenderer();
    initializeLineRenderer();
}

OverlayRenderer::~OverlayRenderer() {
    // TODO: Clean up GPU resources
}

void OverlayRenderer::renderText(const std::string& text, const Math::Vector2f& position, 
                                const TextStyle& style) {
    if (!m_frameActive) return;
    
    renderTextQuad(text, position, style);
}

void OverlayRenderer::renderWorldText(const std::string& text, const Math::Vector3f& worldPosition, 
                                     const TextStyle& style, const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    Math::Vector2f screenPos = worldToScreen(worldPosition, camera);
    if (isOnScreen(screenPos)) {
        renderTextQuad(text, screenPos, style);
    }
}

void OverlayRenderer::renderPerformanceMetrics(const RenderStats& stats, const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    std::stringstream ss;
    ss << "Frame Time: " << formatTime(stats.frameTime) << "\n";
    ss << "CPU Time: " << formatTime(stats.cpuTime) << "\n";
    ss << "GPU Time: " << formatTime(stats.gpuTime) << "\n";
    ss << "Draw Calls: " << stats.drawCalls << "\n";
    ss << "Triangles: " << stats.triangleCount << "\n";
    ss << "Vertices: " << stats.vertexCount;
    
    renderText(ss.str(), position, TextStyle::Debug());
}

void OverlayRenderer::renderMemoryUsage(size_t used, size_t total, const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    std::stringstream ss;
    ss << "Memory: " << formatBytes(used) << " / " << formatBytes(total);
    ss << " (" << std::fixed << std::setprecision(1) << (100.0f * used / total) << "%)";
    
    renderText(ss.str(), position, TextStyle::Debug());
}

void OverlayRenderer::renderVoxelCount(const std::unordered_map<VoxelData::VoxelResolution, size_t>& counts, 
                                      const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    std::stringstream ss;
    ss << "Voxel Count:\n";
    for (const auto& pair : counts) {
        ss << formatResolution(pair.first) << ": " << pair.second << "\n";
    }
    
    renderText(ss.str(), position, TextStyle::Debug());
}

void OverlayRenderer::renderResolutionIndicator(VoxelData::VoxelResolution current, 
                                               const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    std::string text = "Resolution: " + formatResolution(current);
    renderText(text, position, TextStyle::Default());
}

void OverlayRenderer::renderWorkspaceIndicator(const Math::Vector3f& size, const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    std::stringstream ss;
    ss << "Workspace: " << std::fixed << std::setprecision(1);
    ss << size.x << "m x " << size.y << "m x " << size.z << "m";
    
    renderText(ss.str(), position, TextStyle::Default());
}

void OverlayRenderer::renderCameraInfo(const Camera::Camera& camera, const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    // TODO: Get camera position and rotation
    std::stringstream ss;
    ss << "Camera Position: (0.0, 0.0, 0.0)\n";
    ss << "Camera Rotation: (0.0, 0.0, 0.0)";
    
    renderText(ss.str(), position, TextStyle::Debug());
}

void OverlayRenderer::renderGrid(VoxelData::VoxelResolution resolution, const Math::Vector3f& center, 
                                float extent, const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    generateGridLines(resolution, center, extent);
    flushLineBatch(camera);
}

void OverlayRenderer::renderAxes(const Math::Vector3f& origin, float length, const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    generateAxisLines(origin, length);
    flushLineBatch(camera);
}

void OverlayRenderer::renderCompass(const Camera::Camera& camera, const Math::Vector2f& position, float size) {
    if (!m_frameActive) return;
    
    generateCompassGeometry(camera, position, size);
}

void OverlayRenderer::renderBoundingBoxes(const std::vector<Math::BoundingBox>& boxes, 
                                         const Rendering::Color& color, const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    for (const auto& box : boxes) {
        // Add box edges to line renderer
        Math::Vector3f corners[8] = {
            Math::Vector3f(box.min.x, box.min.y, box.min.z),
            Math::Vector3f(box.max.x, box.min.y, box.min.z),
            Math::Vector3f(box.max.x, box.max.y, box.min.z),
            Math::Vector3f(box.min.x, box.max.y, box.min.z),
            Math::Vector3f(box.min.x, box.min.y, box.max.z),
            Math::Vector3f(box.max.x, box.min.y, box.max.z),
            Math::Vector3f(box.max.x, box.max.y, box.max.z),
            Math::Vector3f(box.min.x, box.max.y, box.max.z)
        };
        
        // Bottom face
        addLine(corners[0], corners[1], color);
        addLine(corners[1], corners[2], color);
        addLine(corners[2], corners[3], color);
        addLine(corners[3], corners[0], color);
        
        // Top face
        addLine(corners[4], corners[5], color);
        addLine(corners[5], corners[6], color);
        addLine(corners[6], corners[7], color);
        addLine(corners[7], corners[4], color);
        
        // Vertical edges
        addLine(corners[0], corners[4], color);
        addLine(corners[1], corners[5], color);
        addLine(corners[2], corners[6], color);
        addLine(corners[3], corners[7], color);
    }
    
    flushLineBatch(camera);
}

void OverlayRenderer::renderRaycast(const Ray& ray, float length, const Rendering::Color& color, 
                                   const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    Math::Vector3f end = ray.origin + ray.direction * length;
    addLine(ray.origin, end, color);
    flushLineBatch(camera);
}

// void OverlayRenderer::renderFrustum(const Math::Frustum& frustum, const Rendering::Color& color, 
//                                    const Camera::Camera& camera) {
//     if (!m_frameActive) return;
//     
//     // TODO: Extract frustum corners and render lines
//     flushLineBatch(camera);
// }

void OverlayRenderer::beginFrame(int screenWidth, int screenHeight) {
    m_frameActive = true;
    m_textRenderer.screenWidth = screenWidth;
    m_textRenderer.screenHeight = screenHeight;
    
    // Clear batches
    m_textRenderer.vertices.clear();
    m_textRenderer.indices.clear();
    m_lineRenderer.vertices.clear();
    m_lineRenderer.colors.clear();
}

void OverlayRenderer::endFrame() {
    if (m_frameActive) {
        flushTextBatch();
        m_frameActive = false;
    }
}

void OverlayRenderer::initializeTextRenderer() {
    // TODO: Initialize text rendering resources
    // - Load font texture
    // - Create shader program
    // - Set up vertex/index buffers
    m_textRenderer.fontTexture = 0;
    m_textRenderer.textShader = 0;
    m_textRenderer.vertexBuffer = 0;
    m_textRenderer.indexBuffer = 0;
}

void OverlayRenderer::initializeLineRenderer() {
    // TODO: Initialize line rendering resources
    m_lineRenderer.vertexBuffer = 0;
    m_lineRenderer.lineShader = 0;
    m_lineRenderer.depthTest = true;
}

void OverlayRenderer::renderTextQuad(const std::string& text, const Math::Vector2f& position, 
                                    const TextStyle& style) {
    // TODO: Generate text geometry and add to batch
    // This involves:
    // - Calculating glyph positions
    // - Adding vertices/indices for each character
    // - Handling alignment and background
}

Math::Vector2f OverlayRenderer::measureText(const std::string& text, float size) const {
    // TODO: Calculate text dimensions based on font metrics
    return Math::Vector2f(text.length() * size * 0.6f, size);
}

void OverlayRenderer::flushTextBatch() {
    if (m_textRenderer.vertices.empty()) return;
    
    // TODO: Upload vertex data and render text batch
}

void OverlayRenderer::addLine(const Math::Vector3f& start, const Math::Vector3f& end, 
                             const Rendering::Color& color) {
    m_lineRenderer.vertices.push_back(start);
    m_lineRenderer.vertices.push_back(end);
    m_lineRenderer.colors.push_back(color);
    m_lineRenderer.colors.push_back(color);
}

void OverlayRenderer::flushLineBatch(const Camera::Camera& camera) {
    if (m_lineRenderer.vertices.empty()) return;
    
    // TODO: Upload vertex data and render line batch
    
    // Clear for next batch
    m_lineRenderer.vertices.clear();
    m_lineRenderer.colors.clear();
}

void OverlayRenderer::generateGridLines(VoxelData::VoxelResolution resolution, 
                                       const Math::Vector3f& center, float extent) {
    float voxelSize = VoxelData::getVoxelSize(resolution);
    Rendering::Color gridColor(0.5f, 0.5f, 0.5f, 0.3f);
    
    int gridCount = static_cast<int>(extent / voxelSize);
    float halfExtent = gridCount * voxelSize * 0.5f;
    
    // Generate grid lines in XZ plane
    for (int i = -gridCount; i <= gridCount; ++i) {
        float offset = i * voxelSize;
        
        // Lines parallel to X axis
        addLine(
            Math::Vector3f(center.x - halfExtent, center.y, center.z + offset),
            Math::Vector3f(center.x + halfExtent, center.y, center.z + offset),
            gridColor
        );
        
        // Lines parallel to Z axis
        addLine(
            Math::Vector3f(center.x + offset, center.y, center.z - halfExtent),
            Math::Vector3f(center.x + offset, center.y, center.z + halfExtent),
            gridColor
        );
    }
}

void OverlayRenderer::generateAxisLines(const Math::Vector3f& origin, float length) {
    // X axis - Red
    addLine(origin, origin + Math::Vector3f(length, 0, 0), Rendering::Color::Red());
    
    // Y axis - Green
    addLine(origin, origin + Math::Vector3f(0, length, 0), Rendering::Color::Green());
    
    // Z axis - Blue
    addLine(origin, origin + Math::Vector3f(0, 0, length), Rendering::Color::Blue());
}

void OverlayRenderer::generateCompassGeometry(const Camera::Camera& camera, 
                                             const Math::Vector2f& position, float size) {
    // TODO: Generate compass rose geometry based on camera orientation
}

std::string OverlayRenderer::formatBytes(size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return ss.str();
}

std::string OverlayRenderer::formatTime(float milliseconds) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << milliseconds << " ms";
    return ss.str();
}

std::string OverlayRenderer::formatResolution(VoxelData::VoxelResolution res) const {
    switch (res) {
        case VoxelData::VoxelResolution::Size_1cm: return "1cm";
        case VoxelData::VoxelResolution::Size_2cm: return "2cm";
        case VoxelData::VoxelResolution::Size_4cm: return "4cm";
        case VoxelData::VoxelResolution::Size_8cm: return "8cm";
        case VoxelData::VoxelResolution::Size_16cm: return "16cm";
        case VoxelData::VoxelResolution::Size_32cm: return "32cm";
        case VoxelData::VoxelResolution::Size_64cm: return "64cm";
        case VoxelData::VoxelResolution::Size_128cm: return "128cm";
        case VoxelData::VoxelResolution::Size_256cm: return "256cm";
        case VoxelData::VoxelResolution::Size_512cm: return "512cm";
        case VoxelData::VoxelResolution::COUNT: return "Invalid";
    }
    return "Unknown";
}

Math::Vector2f OverlayRenderer::worldToScreen(const Math::Vector3f& worldPos, 
                                             const Camera::Camera& camera) const {
    // TODO: Convert world position to screen coordinates using camera matrices
    return Math::Vector2f(0, 0);
}

bool OverlayRenderer::isOnScreen(const Math::Vector2f& screenPos) const {
    return screenPos.x >= 0 && screenPos.x < m_textRenderer.screenWidth &&
           screenPos.y >= 0 && screenPos.y < m_textRenderer.screenHeight;
}

} // namespace VisualFeedback
} // namespace VoxelEditor