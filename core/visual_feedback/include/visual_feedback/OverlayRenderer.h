#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "FeedbackTypes.h"
#include "Camera.h"

namespace VoxelEditor {
namespace VisualFeedback {

class OverlayRenderer {
public:
    OverlayRenderer();
    ~OverlayRenderer();
    
    // Text overlays
    void renderText(const std::string& text, const Math::Vector2f& position, 
                   const TextStyle& style);
    void renderWorldText(const std::string& text, const Math::Vector3f& worldPosition, 
                        const TextStyle& style, const Camera::Camera& camera);
    
    // Metric overlays
    void renderPerformanceMetrics(const RenderStats& stats, const Math::Vector2f& position);
    void renderMemoryUsage(size_t used, size_t total, const Math::Vector2f& position);
    void renderVoxelCount(const std::unordered_map<VoxelData::VoxelResolution, size_t>& counts, 
                         const Math::Vector2f& position);
    
    // Indicator overlays
    void renderResolutionIndicator(VoxelData::VoxelResolution current, 
                                 const Math::Vector2f& position);
    void renderWorkspaceIndicator(const Math::Vector3f& size, const Math::Vector2f& position);
    void renderCameraInfo(const Camera::Camera& camera, const Math::Vector2f& position);
    
    // Grid overlays - Enhanced for requirements
    void renderGrid(VoxelData::VoxelResolution resolution, const Math::Vector3f& center, 
                   float extent, const Camera::Camera& camera);
    void renderGroundPlaneGrid(const Math::Vector3f& center, float extent, 
                             const Math::Vector3f& cursorPos, bool enableDynamicOpacity,
                             const Camera::Camera& camera);
    void renderAxes(const Math::Vector3f& origin, float length, const Camera::Camera& camera);
    void renderCompass(const Camera::Camera& camera, const Math::Vector2f& position, float size);
    
    // Debug overlays
    void renderBoundingBoxes(const std::vector<Math::BoundingBox>& boxes, 
                           const Rendering::Color& color, const Camera::Camera& camera);
    void renderRaycast(const Ray& ray, float length, const Rendering::Color& color, 
                      const Camera::Camera& camera);
    // void renderFrustum(const Math::Frustum& frustum, const Rendering::Color& color, 
    //                   const Camera::Camera& camera);
    
    // Batch control
    void beginFrame(int screenWidth, int screenHeight);
    void endFrame();
    
private:
    struct TextRenderer {
        uint32_t fontTexture;
        uint32_t textShader;
        uint32_t vertexBuffer;
        uint32_t indexBuffer;
        uint32_t vertexArray;  // Add VAO
        std::vector<float> vertices;
        std::vector<uint32_t> indices;
        int screenWidth;
        int screenHeight;
    } m_textRenderer;
    
    struct LineRenderer {
        std::vector<Math::Vector3f> vertices;
        std::vector<Rendering::Color> colors;
        uint32_t vertexBuffer;
        uint32_t lineShader;
        bool depthTest;
    } m_lineRenderer;
    
    bool m_frameActive;
    bool m_initialized;
    
    // Initialization
    void initializeTextRenderer();
    void initializeLineRenderer();
    void ensureInitialized();
    
    // Text rendering helpers
    void renderTextQuad(const std::string& text, const Math::Vector2f& position, 
                       const TextStyle& style);
    Math::Vector2f measureText(const std::string& text, float size) const;
    void flushTextBatch();
    
    // Line rendering helpers
    void addLine(const Math::Vector3f& start, const Math::Vector3f& end, 
                const Rendering::Color& color);
    void flushLineBatch(const Camera::Camera& camera);
    
    // Grid generation
    void generateGridLines(VoxelData::VoxelResolution resolution, 
                          const Math::Vector3f& center, float extent);
    void generateGroundPlaneGridLines(const Math::Vector3f& center, float extent, 
                                    const Math::Vector3f& cursorPos, bool enableDynamicOpacity);
    
    // Axis generation
    void generateAxisLines(const Math::Vector3f& origin, float length);
    
    // Compass generation
    void generateCompassGeometry(const Camera::Camera& camera, 
                               const Math::Vector2f& position, float size);
    
    // Formatting helpers
    std::string formatBytes(size_t bytes) const;
    std::string formatTime(float milliseconds) const;
    std::string formatResolution(VoxelData::VoxelResolution res) const;
    
    // Screen space conversion
    Math::Vector2f worldToScreen(const Math::Vector3f& worldPos, 
                                const Camera::Camera& camera) const;
    bool isOnScreen(const Math::Vector2f& screenPos) const;
};

} // namespace VisualFeedback
} // namespace VoxelEditor