#include "../include/visual_feedback/OverlayRenderer.h"
#include <sstream>
#include <iomanip>
#include <cstddef>
#include <cassert>
#include <iostream>

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
#endif

namespace VoxelEditor {
namespace VisualFeedback {

OverlayRenderer::OverlayRenderer()
    : m_frameActive(false)
    , m_initialized(false) {
    
    // Initialize line renderer settings
    m_lineRenderer.depthTest = false;  // Overlay lines should render on top
    
    // Delay OpenGL resource creation until first use
}

OverlayRenderer::~OverlayRenderer() {
    // Clean up GPU resources only if initialized
    if (m_initialized) {
        if (m_textRenderer.fontTexture) {
            glDeleteTextures(1, &m_textRenderer.fontTexture);
        }
        if (m_textRenderer.textShader) {
            glDeleteProgram(m_textRenderer.textShader);
        }
        if (m_textRenderer.vertexArray) {
            glDeleteVertexArrays(1, &m_textRenderer.vertexArray);
        }
        if (m_textRenderer.vertexBuffer) {
            glDeleteBuffers(1, &m_textRenderer.vertexBuffer);
        }
        if (m_textRenderer.indexBuffer) {
            glDeleteBuffers(1, &m_textRenderer.indexBuffer);
        }
        if (m_lineRenderer.vertexArray) {
            glDeleteVertexArrays(1, &m_lineRenderer.vertexArray);
        }
        if (m_lineRenderer.vertexBuffer) {
            glDeleteBuffers(1, &m_lineRenderer.vertexBuffer);
        }
        if (m_lineRenderer.lineShader) {
            glDeleteProgram(m_lineRenderer.lineShader);
        }
    }
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
    
    // Get camera position and rotation from the camera object
    Math::Vector3f cameraPos = camera.getPosition().value();
    // Get camera forward direction as rotation representation
    Math::Vector3f forward = camera.getForward();
    Math::Vector3f cameraRot(forward.x, forward.y, forward.z);
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Camera Position: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")\n";
    ss << "Camera Rotation: (" << cameraRot.x << "°, " << cameraRot.y << "°, " << cameraRot.z << "°)";
    
    renderText(ss.str(), position, TextStyle::Debug());
}

void OverlayRenderer::renderGrid(VoxelData::VoxelResolution resolution, const Math::Vector3f& center, 
                                float extent, const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    generateGridLines(resolution, center, extent);
    flushLineBatch(camera);
}

void OverlayRenderer::renderGroundPlaneGrid(const Math::Vector3f& center, float extent, 
                                          const Math::Vector3f& cursorPos, bool enableDynamicOpacity,
                                          const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    generateGroundPlaneGridLines(center, extent, cursorPos, enableDynamicOpacity);
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
    
    Math::Vector3f end = ray.origin.value() + ray.direction * length;
    
    // Debug output removed - ray visualization working correctly
    
    addLine(ray.origin.value(), end, color);
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
    ensureInitialized();
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

void OverlayRenderer::ensureInitialized() {
    if (!m_initialized) {
        initializeTextRenderer();
        initializeLineRenderer();
        m_initialized = true;
    }
}

void OverlayRenderer::initializeTextRenderer() {
    // Create a simple 8x8 bitmap font texture (ASCII characters 32-127)
    const int charWidth = 8;
    const int charHeight = 8;
    const int charsPerRow = 16;
    const int textureSize = 128; // 16x8 characters
    
    // Create a simple bitmap font data (1 = pixel on, 0 = pixel off)
    // For simplicity, we'll create a placeholder texture
    unsigned char* fontData = new unsigned char[textureSize * textureSize * 4];
    
    // Create a simple bitmap font for ASCII characters 32-127
    // Characters are arranged in a 16x8 grid, each character is 8x8 pixels
    memset(fontData, 0, textureSize * textureSize * 4); // Start with transparent
    
    // Define simple bitmaps for common characters
    auto setPixel = [&](int charX, int charY, int pixelX, int pixelY, bool on) {
        if (charX >= 16 || charY >= 8 || pixelX >= 8 || pixelY >= 8) return;
        int x = charX * 8 + pixelX;
        int y = charY * 8 + pixelY;
        int idx = (y * textureSize + x) * 4;
        if (on) {
            fontData[idx] = 255;     // R
            fontData[idx+1] = 255;   // G  
            fontData[idx+2] = 255;   // B
            fontData[idx+3] = 255;   // A - fully opaque white
        }
    };
    
    // Space (ASCII 32) - already transparent
    
    // Colon ':' (ASCII 58 = index 26, row 1, col 10)
    setPixel(10, 1, 3, 2, true);
    setPixel(10, 1, 3, 5, true);
    
    // Numbers 0-9 (ASCII 48-57)
    // '0' (ASCII 48 = index 16, row 1, col 0)
    for (int i = 1; i <= 6; i++) { setPixel(0, 1, 1, i, true); setPixel(0, 1, 5, i, true); }
    for (int i = 2; i <= 4; i++) { setPixel(0, 1, i, 1, true); setPixel(0, 1, i, 6, true); }
    
    // '1' (ASCII 49 = index 17, row 1, col 1)
    for (int i = 1; i <= 6; i++) setPixel(1, 1, 3, i, true);
    setPixel(1, 1, 2, 2, true);
    for (int i = 1; i <= 5; i++) setPixel(1, 1, i, 6, true);
    
    // Letters (simplified versions)
    // 'R' (ASCII 82 = index 50, row 3, col 2)
    for (int i = 1; i <= 6; i++) setPixel(2, 3, 1, i, true);
    for (int i = 2; i <= 4; i++) { setPixel(2, 3, i, 1, true); setPixel(2, 3, i, 3, true); }
    setPixel(2, 3, 5, 2, true);
    setPixel(2, 3, 3, 4, true);
    setPixel(2, 3, 4, 5, true);
    setPixel(2, 3, 5, 6, true);
    
    // 'e' (ASCII 101 = index 69, row 4, col 5)
    for (int i = 2; i <= 4; i++) { setPixel(5, 4, i, 3, true); setPixel(5, 4, i, 6, true); }
    for (int i = 4; i <= 5; i++) setPixel(5, 4, 1, i, true);
    setPixel(5, 4, 5, 4, true);
    
    // 's' (ASCII 115 = index 83, row 5, col 3)
    for (int i = 2; i <= 4; i++) { setPixel(3, 5, i, 3, true); setPixel(3, 5, i, 6, true); }
    setPixel(3, 5, 1, 4, true);
    setPixel(3, 5, 3, 5, true);
    setPixel(3, 5, 5, 5, true);
    
    // 'o' (ASCII 111 = index 79, row 4, col 15)
    for (int i = 2; i <= 4; i++) { setPixel(15, 4, i, 3, true); setPixel(15, 4, i, 6, true); }
    for (int i = 4; i <= 5; i++) { setPixel(15, 4, 1, i, true); setPixel(15, 4, 5, i, true); }
    
    // 'l' (ASCII 108 = index 76, row 4, col 12)
    for (int i = 1; i <= 6; i++) setPixel(12, 4, 2, i, true);
    
    // 'u' (ASCII 117 = index 85, row 5, col 5)
    for (int i = 3; i <= 5; i++) { setPixel(5, 5, 1, i, true); setPixel(5, 5, 5, i, true); }
    for (int i = 2; i <= 4; i++) setPixel(5, 5, i, 6, true);
    
    // 't' (ASCII 116 = index 84, row 5, col 4)
    for (int i = 1; i <= 6; i++) setPixel(4, 5, 2, i, true);
    for (int i = 1; i <= 3; i++) setPixel(4, 5, i, 2, true);
    
    // 'i' (ASCII 105 = index 73, row 4, col 9)
    setPixel(9, 4, 2, 1, true);
    for (int i = 3; i <= 6; i++) setPixel(9, 4, 2, i, true);
    
    // 'n' (ASCII 110 = index 78, row 4, col 14)
    for (int i = 3; i <= 6; i++) { setPixel(14, 4, 1, i, true); setPixel(14, 4, 5, i, true); }
    for (int i = 2; i <= 4; i++) setPixel(14, 4, i, 3, true);
    
    // 'W' (ASCII 87 = index 55, row 3, col 7)
    for (int i = 1; i <= 6; i++) { setPixel(7, 3, 0, i, true); setPixel(7, 3, 6, i, true); }
    for (int i = 4; i <= 6; i++) { setPixel(7, 3, 2, i, true); setPixel(7, 3, 4, i, true); }
    setPixel(7, 3, 3, 6, true);
    
    // 'k' (ASCII 107 = index 75, row 4, col 11)
    for (int i = 1; i <= 6; i++) setPixel(11, 4, 1, i, true);
    setPixel(11, 4, 4, 3, true);
    setPixel(11, 4, 3, 4, true);
    setPixel(11, 4, 4, 5, true);
    setPixel(11, 4, 5, 6, true);
    
    // 'r' (ASCII 114 = index 82, row 5, col 2)
    for (int i = 3; i <= 6; i++) setPixel(2, 5, 1, i, true);
    for (int i = 2; i <= 3; i++) setPixel(2, 5, i, 3, true);
    setPixel(2, 5, 4, 4, true);
    
    // 'p' (ASCII 112 = index 80, row 5, col 0)
    for (int i = 3; i <= 7; i++) setPixel(0, 5, 1, i, true);
    for (int i = 2; i <= 4; i++) { setPixel(0, 5, i, 3, true); setPixel(0, 5, i, 5, true); }
    setPixel(0, 5, 5, 4, true);
    
    // 'a' (ASCII 97 = index 65, row 4, col 1)
    for (int i = 2; i <= 4; i++) setPixel(1, 4, i, 4, true);
    setPixel(1, 4, 1, 5, true);
    for (int i = 4; i <= 6; i++) { setPixel(1, 4, 5, i, true); setPixel(1, 4, i, 6, true); }
    
    // 'c' (ASCII 99 = index 67, row 4, col 3)  
    for (int i = 2; i <= 4; i++) { setPixel(3, 4, i, 3, true); setPixel(3, 4, i, 6, true); }
    for (int i = 4; i <= 5; i++) setPixel(3, 4, 1, i, true);
    
    // 'm' (ASCII 109 = index 77, row 4, col 13)
    for (int i = 3; i <= 6; i++) { setPixel(13, 4, 1, i, true); setPixel(13, 4, 3, i, true); setPixel(13, 4, 5, i, true); }
    setPixel(13, 4, 2, 3, true);
    setPixel(13, 4, 4, 3, true);
    
    // Space for 'x' patterns and other basic shapes
    // Add more characters as needed...
    
    // 'x' (ASCII 120 = index 88, row 5, col 8)
    setPixel(8, 5, 1, 3, true);
    setPixel(8, 5, 2, 4, true);
    setPixel(8, 5, 3, 5, true);
    setPixel(8, 5, 4, 4, true);
    setPixel(8, 5, 5, 3, true);
    setPixel(8, 5, 1, 6, true);
    setPixel(8, 5, 5, 6, true);
    
    // Period '.' (ASCII 46 = index 14, row 0, col 14)
    setPixel(14, 0, 3, 6, true);
    
    // More digits for completeness
    // '2' (ASCII 50 = index 18, row 1, col 2)
    for (int i = 2; i <= 4; i++) { setPixel(2, 1, i, 1, true); setPixel(2, 1, i, 3, true); setPixel(2, 1, i, 6, true); }
    setPixel(2, 1, 5, 2, true);
    setPixel(2, 1, 1, 4, true);
    setPixel(2, 1, 1, 5, true);
    
    // '3' (ASCII 51 = index 19, row 1, col 3)
    for (int i = 2; i <= 4; i++) { setPixel(3, 1, i, 1, true); setPixel(3, 1, i, 3, true); setPixel(3, 1, i, 6, true); }
    setPixel(3, 1, 5, 2, true);
    setPixel(3, 1, 5, 4, true);
    setPixel(3, 1, 5, 5, true);
    
    // '4' (ASCII 52 = index 20, row 1, col 4)
    for (int i = 1; i <= 3; i++) setPixel(4, 1, 1, i, true);
    setPixel(4, 1, 3, 2, true);
    for (int i = 1; i <= 5; i++) setPixel(4, 1, i, 3, true);
    for (int i = 4; i <= 6; i++) setPixel(4, 1, 5, i, true);
    
    // '5' (ASCII 53 = index 21, row 1, col 5)
    for (int i = 1; i <= 5; i++) setPixel(5, 1, i, 1, true);
    for (int i = 2; i <= 3; i++) setPixel(5, 1, 1, i, true);
    for (int i = 2; i <= 4; i++) setPixel(5, 1, i, 3, true);
    setPixel(5, 1, 5, 4, true);
    setPixel(5, 1, 5, 5, true);
    for (int i = 2; i <= 4; i++) setPixel(5, 1, i, 6, true);
    
    // More essential letters
    // 'd' (ASCII 100 = index 68, row 4, col 4)
    for (int i = 1; i <= 6; i++) setPixel(4, 4, 5, i, true);
    for (int i = 2; i <= 4; i++) { setPixel(4, 4, i, 3, true); setPixel(4, 4, i, 6, true); }
    for (int i = 4; i <= 5; i++) setPixel(4, 4, 1, i, true);
    
    // 'h' (ASCII 104 = index 72, row 4, col 8)
    for (int i = 1; i <= 6; i++) setPixel(8, 4, 1, i, true);
    for (int i = 2; i <= 4; i++) setPixel(8, 4, i, 3, true);
    for (int i = 4; i <= 6; i++) setPixel(8, 4, 5, i, true);
    
    // 'f' (ASCII 102 = index 70, row 4, col 6)
    for (int i = 1; i <= 6; i++) setPixel(6, 4, 1, i, true);
    for (int i = 2; i <= 4; i++) { setPixel(6, 4, i, 1, true); setPixel(6, 4, i, 3, true); }
    
    // 'g' (ASCII 103 = index 71, row 4, col 7)
    for (int i = 2; i <= 4; i++) { setPixel(7, 4, i, 1, true); setPixel(7, 4, i, 6, true); }
    for (int i = 2; i <= 5; i++) setPixel(7, 4, 1, i, true);
    setPixel(7, 4, 4, 4, true);
    setPixel(7, 4, 5, 4, true);
    setPixel(7, 4, 5, 5, true);
    
    // 'v' (ASCII 118 = index 86, row 5, col 6)
    setPixel(6, 5, 0, 3, true);
    setPixel(6, 5, 1, 4, true);
    setPixel(6, 5, 2, 5, true);
    setPixel(6, 5, 3, 6, true);
    setPixel(6, 5, 4, 5, true);
    setPixel(6, 5, 5, 4, true);
    setPixel(6, 5, 6, 3, true);
    
    // 'y' (ASCII 121 = index 89, row 5, col 9)
    setPixel(9, 5, 0, 3, true);
    setPixel(9, 5, 1, 4, true);
    setPixel(9, 5, 2, 5, true);
    setPixel(9, 5, 3, 6, true);
    setPixel(9, 5, 4, 5, true);
    setPixel(9, 5, 5, 4, true);
    setPixel(9, 5, 6, 3, true);
    setPixel(9, 5, 2, 7, true);
    setPixel(9, 5, 1, 7, true);
    
    // 'b' (ASCII 98 = index 66, row 4, col 2)
    for (int i = 1; i <= 6; i++) setPixel(2, 4, 1, i, true);
    for (int i = 2; i <= 4; i++) { setPixel(2, 4, i, 3, true); setPixel(2, 4, i, 6, true); }
    setPixel(2, 4, 5, 4, true);
    setPixel(2, 4, 5, 5, true);
    
    // Missing numbers for completeness
    // '6' (ASCII 54 = index 22, row 1, col 6)
    for (int i = 2; i <= 5; i++) setPixel(6, 1, 1, i, true);
    for (int i = 2; i <= 4; i++) { setPixel(6, 1, i, 1, true); setPixel(6, 1, i, 4, true); setPixel(6, 1, i, 6, true); }
    setPixel(6, 1, 5, 5, true);
    
    // '7' (ASCII 55 = index 23, row 1, col 7)
    for (int i = 1; i <= 5; i++) setPixel(7, 1, i, 1, true);
    setPixel(7, 1, 4, 2, true);
    setPixel(7, 1, 3, 3, true);
    setPixel(7, 1, 2, 4, true);
    setPixel(7, 1, 2, 5, true);
    setPixel(7, 1, 2, 6, true);
    
    // '8' (ASCII 56 = index 24, row 1, col 8)
    for (int i = 2; i <= 4; i++) { setPixel(8, 1, i, 1, true); setPixel(8, 1, i, 3, true); setPixel(8, 1, i, 6, true); }
    for (int i = 2; i <= 5; i++) { setPixel(8, 1, 1, i, true); setPixel(8, 1, 5, i, true); }
    
    // '9' (ASCII 57 = index 25, row 1, col 9)
    for (int i = 2; i <= 4; i++) { setPixel(9, 1, i, 1, true); setPixel(9, 1, i, 3, true); setPixel(9, 1, i, 6, true); }
    setPixel(9, 1, 1, 2, true);
    for (int i = 2; i <= 5; i++) setPixel(9, 1, 5, i, true);
    
    // Create font texture
    glGenTextures(1, &m_textRenderer.fontTexture);
    glBindTexture(GL_TEXTURE_2D, m_textRenderer.fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSize, textureSize, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, fontData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    delete[] fontData;
    
    // Create text shader program
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 position;
        layout(location = 1) in vec2 texCoord;
        layout(location = 2) in vec4 color;
        
        uniform vec2 screenSize;
        
        out vec2 fragTexCoord;
        out vec4 fragColor;
        
        void main() {
            // Convert from screen coordinates to NDC
            vec2 ndc = (position / screenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y; // Flip Y axis
            
            gl_Position = vec4(ndc, 0.0, 1.0);
            fragTexCoord = texCoord;
            fragColor = color;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 fragTexCoord;
        in vec4 fragColor;
        
        uniform sampler2D fontTexture;
        
        out vec4 finalColor;
        
        void main() {
            float alpha = texture(fontTexture, fragTexCoord).a;
            finalColor = vec4(fragColor.rgb, fragColor.a * alpha);
        }
    )";
    
    // Compile shaders with error checking
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    // Check vertex shader compilation
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        printf("ERROR: Text vertex shader compilation failed: %s\n", infoLog);
    }
    
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        printf("ERROR: Text fragment shader compilation failed: %s\n", infoLog);
    }
    
    // Create and link program
    m_textRenderer.textShader = glCreateProgram();
    glAttachShader(m_textRenderer.textShader, vertexShader);
    glAttachShader(m_textRenderer.textShader, fragmentShader);
    glLinkProgram(m_textRenderer.textShader);
    
    // Check program linking
    glGetProgramiv(m_textRenderer.textShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_textRenderer.textShader, 512, nullptr, infoLog);
        printf("ERROR: Text shader program linking failed: %s\n", infoLog);
    }
    
    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Create VAO first
    glGenVertexArrays(1, &m_textRenderer.vertexArray);
    glBindVertexArray(m_textRenderer.vertexArray);
    
    // Create vertex and index buffers
    glGenBuffers(1, &m_textRenderer.vertexBuffer);
    glGenBuffers(1, &m_textRenderer.indexBuffer);
    
    // Allocate initial buffer sizes
    glBindBuffer(GL_ARRAY_BUFFER, m_textRenderer.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * 1024, nullptr, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_textRenderer.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * 6 * 256, nullptr, GL_DYNAMIC_DRAW);
    
    // Set up vertex attributes while VAO is bound
    size_t stride = 8 * sizeof(float);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    
    glEnableVertexAttribArray(1); // texCoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    
    glEnableVertexAttribArray(2); // color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
    
    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OverlayRenderer::initializeLineRenderer() {
    // Create VAO first
    glGenVertexArrays(1, &m_lineRenderer.vertexArray);
    glBindVertexArray(m_lineRenderer.vertexArray);
    
    // Create vertex buffer for lines
    glGenBuffers(1, &m_lineRenderer.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineRenderer.vertexBuffer);
    // Allocate initial buffer size
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 7 * 1024, nullptr, GL_DYNAMIC_DRAW);
    
    // Setup vertex attributes while VAO is bound
    size_t stride = 7 * sizeof(float);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    
    glEnableVertexAttribArray(1); // color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    
    // Unbind VAO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // Create line shader program
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec4 color;
        
        uniform mat4 mvpMatrix;
        
        out vec4 fragColor;
        
        void main() {
            gl_Position = mvpMatrix * vec4(position, 1.0);
            fragColor = color;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec4 fragColor;
        
        out vec4 color;
        
        void main() {
            color = fragColor;
        }
    )";
    
    // Compile vertex shader
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    // Check vertex shader compilation
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "OverlayRenderer: Vertex shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return; // Early return to prevent using invalid shader
    }
    
    // Compile fragment shader
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "OverlayRenderer: Fragment shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return; // Early return to prevent using invalid shader
    }
    
    // Create and link program
    m_lineRenderer.lineShader = glCreateProgram();
    glAttachShader(m_lineRenderer.lineShader, vertexShader);
    glAttachShader(m_lineRenderer.lineShader, fragmentShader);
    glLinkProgram(m_lineRenderer.lineShader);
    
    // Check program linking
    glGetProgramiv(m_lineRenderer.lineShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_lineRenderer.lineShader, 512, nullptr, infoLog);
        std::cerr << "OverlayRenderer: Shader program linking failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_lineRenderer.lineShader);
        m_lineRenderer.lineShader = 0; // Mark as invalid
        return; // Early return to prevent using invalid program
    }
    
    // Clean up shader objects (no longer needed after successful linking)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    m_lineRenderer.depthTest = true;
}

void OverlayRenderer::renderTextQuad(const std::string& text, const Math::Vector2f& position, 
                                    const TextStyle& style) {
    if (text.empty()) return;
    
    const float charWidth = 8.0f * style.size;
    const float charHeight = 8.0f * style.size;
    const float uvCharWidth = 1.0f / 16.0f; // 16 chars per row in texture
    const float uvCharHeight = 1.0f / 8.0f;  // 8 rows in texture
    
    Math::Vector2f currentPos = position;
    
    // Handle alignment
    if (style.alignment != TextAlignment::Left) {
        Math::Vector2f textSize = measureText(text, style.size);
        if (style.alignment == TextAlignment::Center) {
            currentPos.x -= textSize.x * 0.5f;
        } else if (style.alignment == TextAlignment::Right) {
            currentPos.x -= textSize.x;
        }
    }
    
    // Reserve space in vertex array (position + texcoord + color = 2 + 2 + 4 = 8 floats per vertex)
    size_t baseVertex = m_textRenderer.vertices.size() / 8;
    
    for (char c : text) {
        if (c == '\n') {
            currentPos.x = position.x;
            currentPos.y += charHeight * 1.2f;
            continue;
        }
        
        if (c < 32 || c > 127) continue; // Skip non-printable characters
        
        // Calculate texture coordinates for this character
        int charIndex = c - 32;
        int row = charIndex / 16;
        int col = charIndex % 16;
        
        float u0 = col * uvCharWidth;
        float v0 = row * uvCharHeight;
        float u1 = u0 + uvCharWidth;
        float v1 = v0 + uvCharHeight;
        
        // Add vertices for this character quad
        // Top-left
        m_textRenderer.vertices.push_back(currentPos.x);
        m_textRenderer.vertices.push_back(currentPos.y);
        m_textRenderer.vertices.push_back(u0);
        m_textRenderer.vertices.push_back(v0);
        m_textRenderer.vertices.push_back(style.color.r);
        m_textRenderer.vertices.push_back(style.color.g);
        m_textRenderer.vertices.push_back(style.color.b);
        m_textRenderer.vertices.push_back(style.color.a);
        
        // Top-right
        m_textRenderer.vertices.push_back(currentPos.x + charWidth);
        m_textRenderer.vertices.push_back(currentPos.y);
        m_textRenderer.vertices.push_back(u1);
        m_textRenderer.vertices.push_back(v0);
        m_textRenderer.vertices.push_back(style.color.r);
        m_textRenderer.vertices.push_back(style.color.g);
        m_textRenderer.vertices.push_back(style.color.b);
        m_textRenderer.vertices.push_back(style.color.a);
        
        // Bottom-right
        m_textRenderer.vertices.push_back(currentPos.x + charWidth);
        m_textRenderer.vertices.push_back(currentPos.y + charHeight);
        m_textRenderer.vertices.push_back(u1);
        m_textRenderer.vertices.push_back(v1);
        m_textRenderer.vertices.push_back(style.color.r);
        m_textRenderer.vertices.push_back(style.color.g);
        m_textRenderer.vertices.push_back(style.color.b);
        m_textRenderer.vertices.push_back(style.color.a);
        
        // Bottom-left
        m_textRenderer.vertices.push_back(currentPos.x);
        m_textRenderer.vertices.push_back(currentPos.y + charHeight);
        m_textRenderer.vertices.push_back(u0);
        m_textRenderer.vertices.push_back(v1);
        m_textRenderer.vertices.push_back(style.color.r);
        m_textRenderer.vertices.push_back(style.color.g);
        m_textRenderer.vertices.push_back(style.color.b);
        m_textRenderer.vertices.push_back(style.color.a);
        
        // Add indices for this quad
        uint32_t vertexOffset = static_cast<uint32_t>(baseVertex);
        m_textRenderer.indices.push_back(vertexOffset + 0);
        m_textRenderer.indices.push_back(vertexOffset + 1);
        m_textRenderer.indices.push_back(vertexOffset + 2);
        
        m_textRenderer.indices.push_back(vertexOffset + 0);
        m_textRenderer.indices.push_back(vertexOffset + 2);
        m_textRenderer.indices.push_back(vertexOffset + 3);
        
        // Move to next character position
        currentPos.x += charWidth;
        baseVertex += 4;
    }
}

Math::Vector2f OverlayRenderer::measureText(const std::string& text, float size) const {
    // TODO: Calculate text dimensions based on font metrics
    return Math::Vector2f(text.length() * size * 0.6f, size);
}

void OverlayRenderer::flushTextBatch() {
    if (m_textRenderer.vertices.empty()) return;
    
    // Enable blending for text
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    // Use text shader
    glUseProgram(m_textRenderer.textShader);
    
    // Set uniforms
    GLint screenSizeLoc = glGetUniformLocation(m_textRenderer.textShader, "screenSize");
    GLint fontTextureLoc = glGetUniformLocation(m_textRenderer.textShader, "fontTexture");
    
    glUniform2f(screenSizeLoc, static_cast<float>(m_textRenderer.screenWidth), 
                static_cast<float>(m_textRenderer.screenHeight));
    glUniform1i(fontTextureLoc, 0);
    
    // Bind font texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textRenderer.fontTexture);
    
    // Bind VAO (contains all vertex attribute setup)
    glBindVertexArray(m_textRenderer.vertexArray);
    
    // Check and resize vertex buffer if needed
    size_t vertexDataSize = m_textRenderer.vertices.size() * sizeof(float);
    size_t currentVertexBufferSize = sizeof(float) * 8 * 1024; // Initial size
    
    glBindBuffer(GL_ARRAY_BUFFER, m_textRenderer.vertexBuffer);
    if (vertexDataSize > currentVertexBufferSize) {
        // Reallocate buffer with larger size
        size_t newSize = vertexDataSize * 2; // Double the required size for future growth
        glBufferData(GL_ARRAY_BUFFER, newSize, nullptr, GL_DYNAMIC_DRAW);
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexDataSize, m_textRenderer.vertices.data());
    
    // Check and resize index buffer if needed
    size_t indexDataSize = m_textRenderer.indices.size() * sizeof(uint32_t);
    size_t currentIndexBufferSize = sizeof(uint32_t) * 6 * 256; // Initial size
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_textRenderer.indexBuffer);
    if (indexDataSize > currentIndexBufferSize) {
        // Reallocate buffer with larger size
        size_t newSize = indexDataSize * 2; // Double the required size for future growth
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, newSize, nullptr, GL_DYNAMIC_DRAW);
    }
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexDataSize, m_textRenderer.indices.data());
    
    // Draw text (VAO already has vertex attributes set up)
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_textRenderer.indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    
    // Cleanup
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    
    glEnable(GL_DEPTH_TEST);
    
    // Check for OpenGL errors after text rendering
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OverlayRenderer GL Error after flushTextBatch: " << error << std::endl;
        // Assert when failing to ensure we are not masking problems
        assert(false && "OpenGL error in OverlayRenderer text rendering - failing hard to catch issues early");
    }
    
    // Clear for next frame
    m_textRenderer.vertices.clear();
    m_textRenderer.indices.clear();
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
    
    // Check if shader is valid
    if (m_lineRenderer.lineShader == 0) {
        std::cerr << "OverlayRenderer: Cannot render lines - line shader is invalid" << std::endl;
        return;
    }
    
    // Use line shader
    glUseProgram(m_lineRenderer.lineShader);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OverlayRenderer: GL error after glUseProgram: " << error << std::endl;
        return;
    }
    
    // Set MVP matrix
    GLint mvpLoc = glGetUniformLocation(m_lineRenderer.lineShader, "mvpMatrix");
    if (mvpLoc == -1) {
        std::cerr << "OverlayRenderer: Warning - mvpMatrix uniform not found in line shader" << std::endl;
    }
    Math::Matrix4f mvpMatrix = camera.getProjectionMatrix() * camera.getViewMatrix();
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvpMatrix.data());
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OverlayRenderer: GL error after setting uniforms: " << error << std::endl;
        return;
    }
    
    // Create interleaved vertex data
    std::vector<float> interleavedData;
    interleavedData.reserve(m_lineRenderer.vertices.size() * 7);
    
    for (size_t i = 0; i < m_lineRenderer.vertices.size(); ++i) {
        interleavedData.push_back(m_lineRenderer.vertices[i].x);
        interleavedData.push_back(m_lineRenderer.vertices[i].y);
        interleavedData.push_back(m_lineRenderer.vertices[i].z);
        interleavedData.push_back(m_lineRenderer.colors[i].r);
        interleavedData.push_back(m_lineRenderer.colors[i].g);
        interleavedData.push_back(m_lineRenderer.colors[i].b);
        interleavedData.push_back(m_lineRenderer.colors[i].a);
    }
    
    // Bind VAO (contains all vertex attribute setup)
    glBindVertexArray(m_lineRenderer.vertexArray);
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OverlayRenderer: GL error after glBindVertexArray: " << error << std::endl;
        return;
    }
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_lineRenderer.vertexBuffer);
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OverlayRenderer: GL error after glBindBuffer: " << error << std::endl;
        return;
    }
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, interleavedData.size() * sizeof(float),
                    interleavedData.data());
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OverlayRenderer: GL error after glBufferSubData: " << error << std::endl;
        return;
    }
    
    // Enable/disable depth test based on settings
    if (m_lineRenderer.depthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    // Enable blending for lines
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OverlayRenderer: GL error after setting blend state: " << error << std::endl;
        return;
    }
    
    // Draw lines
    std::cerr << "OverlayRenderer: About to draw " << m_lineRenderer.vertices.size() << " vertices as " 
              << (m_lineRenderer.vertices.size() / 2) << " lines" << std::endl;
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_lineRenderer.vertices.size()));
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OverlayRenderer: GL error in glDrawArrays: " << error << std::endl;
        return;
    }
    
    // Cleanup
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    
    // Reset depth test
    glEnable(GL_DEPTH_TEST);
    
    // Check for OpenGL errors after line rendering
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OverlayRenderer GL Error after flushLineBatch: " << error << std::endl;
        // Assert when failing to ensure we are not masking problems
        assert(false && "OpenGL error in OverlayRenderer line rendering - failing hard to catch issues early");
    }
    
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

void OverlayRenderer::generateGroundPlaneGridLines(const Math::Vector3f& center, float extent, 
                                                  const Math::Vector3f& cursorPos, bool enableDynamicOpacity) {
    // REQ-1.1.1: Grid with 32cm squares
    const float gridSize = 0.32f; // 32cm
    
    // REQ-1.1.3: Grid line colors and opacity
    const Rendering::Color normalGridColor(180.0f/255.0f, 180.0f/255.0f, 180.0f/255.0f, 0.35f);
    
    // REQ-1.1.4: Major grid lines every 160cm (5 * 32cm)
    const float majorGridInterval = 1.60f; // 160cm
    const Rendering::Color majorGridColor(200.0f/255.0f, 200.0f/255.0f, 200.0f/255.0f, 0.35f);
    
    // REQ-1.2.2: Dynamic opacity near cursor
    const float dynamicOpacityRadius = 2.0f * gridSize; // 2 grid squares = 64cm
    const float enhancedOpacity = 0.65f;
    
    int gridCount = static_cast<int>(extent / gridSize);
    float halfExtent = gridCount * gridSize * 0.5f;
    
    // Generate grid lines in XZ plane at Y=0 (ground plane)
    for (int i = -gridCount; i <= gridCount; ++i) {
        float offset = i * gridSize;
        
        // Determine if this is a major grid line
        bool isMajorLine = (std::abs(offset) < 0.001f) || 
                          (std::abs(std::fmod(offset, majorGridInterval)) < 0.001f);
        
        // Calculate distance to cursor for dynamic opacity
        float distanceToX = enableDynamicOpacity ? 
            std::abs(cursorPos.x - (center.x + offset)) : std::numeric_limits<float>::max();
        float distanceToZ = enableDynamicOpacity ? 
            std::abs(cursorPos.z - (center.z + offset)) : std::numeric_limits<float>::max();
        
        // Determine line color and opacity
        Rendering::Color lineColor = isMajorLine ? majorGridColor : normalGridColor;
        
        // REQ-1.2.2: Enhance opacity near cursor
        if (enableDynamicOpacity) {
            bool enhanceOpacityX = distanceToX <= dynamicOpacityRadius;
            bool enhanceOpacityZ = distanceToZ <= dynamicOpacityRadius;
            
            if (enhanceOpacityX || enhanceOpacityZ) {
                lineColor.a = enhancedOpacity;
            }
        }
        
        // Lines parallel to X axis (running east-west)
        addLine(
            Math::Vector3f(center.x - halfExtent, 0.0f, center.z + offset),
            Math::Vector3f(center.x + halfExtent, 0.0f, center.z + offset),
            lineColor
        );
        
        // Lines parallel to Z axis (running north-south)  
        addLine(
            Math::Vector3f(center.x + offset, 0.0f, center.z - halfExtent),
            Math::Vector3f(center.x + offset, 0.0f, center.z + halfExtent),
            lineColor
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
    // Transform world position to clip space
    Math::Vector4f worldPos4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
    Math::Matrix4f viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();
    Math::Vector4f clipPos = viewProj * worldPos4;
    
    // Perform perspective divide
    if (clipPos.w == 0.0f) {
        return Math::Vector2f(-1.0f, -1.0f); // Behind camera
    }
    
    Math::Vector3f ndcPos(clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w);
    
    // Convert from NDC (-1 to 1) to screen coordinates
    float screenX = (ndcPos.x + 1.0f) * 0.5f * m_textRenderer.screenWidth;
    float screenY = (1.0f - ndcPos.y) * 0.5f * m_textRenderer.screenHeight; // Flip Y
    
    return Math::Vector2f(screenX, screenY);
}

bool OverlayRenderer::isOnScreen(const Math::Vector2f& screenPos) const {
    return screenPos.x >= 0 && screenPos.x < m_textRenderer.screenWidth &&
           screenPos.y >= 0 && screenPos.y < m_textRenderer.screenHeight;
}

} // namespace VisualFeedback
} // namespace VoxelEditor