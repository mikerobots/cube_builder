#pragma once

#include "SelectionTypes.h"
#include "SelectionSet.h"
#include "SelectionManager.h"
#include "../rendering/RenderEngine.h"
#include "../rendering/ShaderManager.h"
#include "../camera/Camera.h"
#include "../../foundation/math/Vector4f.h"
#include <memory>

namespace VoxelEditor {
namespace Selection {

class SelectionRenderer {
public:
    explicit SelectionRenderer(Rendering::RenderEngine* renderEngine = nullptr);
    ~SelectionRenderer();
    
    // Initialize rendering resources
    bool initialize();
    void shutdown();
    
    // Set dependencies
    void setRenderEngine(Rendering::RenderEngine* engine) { m_renderEngine = engine; }
    void setSelectionManager(SelectionManager* manager) { m_selectionManager = manager; }
    
    // Render selection
    void render(const Camera::Camera* camera, float deltaTime);
    void renderPreview(const SelectionSet& preview, const Camera::Camera* camera);
    
    // Render specific selection shapes
    void renderBox(const Math::BoundingBox& box, const Rendering::Color& color, float thickness = 1.0f);
    void renderSphere(const Math::Vector3f& center, float radius, const Rendering::Color& color);
    void renderCylinder(const Math::Vector3f& base, const Math::Vector3f& direction, 
                       float radius, float height, const Rendering::Color& color);
    
    // Configuration
    void setStyle(const SelectionStyle& style) { m_style = style; }
    const SelectionStyle& getStyle() const { return m_style; }
    
    // Render modes
    enum class SelectionRenderMode {
        Outline,        // Wireframe outline only
        Fill,          // Semi-transparent fill only
        OutlineAndFill, // Both outline and fill
        Highlight      // Animated highlight effect
    };
    
    void setRenderMode(SelectionRenderMode mode) { m_renderMode = mode; }
    SelectionRenderMode getRenderMode() const { return m_renderMode; }
    
    void setShowGizmos(bool show) { m_showGizmos = show; }
    bool getShowGizmos() const { return m_showGizmos; }
    
private:
    Rendering::RenderEngine* m_renderEngine;
    SelectionManager* m_selectionManager;
    SelectionStyle m_style;
    SelectionRenderMode m_renderMode;
    bool m_showGizmos;
    float m_animationTime;
    
    // Rendering resources
    struct RenderResources {
        Rendering::VertexBufferId outlineVBO;
        Rendering::VertexBufferId fillVBO;
        Rendering::IndexBufferId outlineIBO;
        Rendering::IndexBufferId fillIBO;
        Rendering::VertexArrayId outlineVAO;
        Rendering::VertexArrayId fillVAO;
        Rendering::ShaderId selectionShader;
        size_t outlineVertexCount;
        size_t fillVertexCount;
        bool needsUpdate;
    };
    
    std::unique_ptr<RenderResources> m_resources;
    
    // Helper methods
    void updateGeometry(const SelectionSet& selection);
    void generateOutlineGeometry(const SelectionSet& selection);
    void generateFillGeometry(const SelectionSet& selection);
    void renderOutline(const Math::Matrix4f& viewProj);
    void renderFill(const Math::Matrix4f& viewProj);
    void renderBounds(const Math::BoundingBox& bounds, const Math::Matrix4f& viewProj);
    void renderStats(const SelectionStats& stats, const Math::Matrix4f& viewProj);
    Math::Vector4f getAnimatedColor(const Rendering::Color& baseColor);
    void createShaders();
};

}
}