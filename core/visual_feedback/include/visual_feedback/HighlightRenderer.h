#pragma once

#include <vector>
#include <memory>

#include "FeedbackTypes.h"
#include "SelectionSet.h"
#include "Camera.h"

namespace VoxelEditor {
namespace VisualFeedback {

class HighlightRenderer {
public:
    HighlightRenderer();
    ~HighlightRenderer();
    
    // Face highlighting
    void renderFaceHighlight(const Face& face, const HighlightStyle& style);
    void clearFaceHighlights();
    
    // Voxel highlighting
    void renderVoxelHighlight(const Math::Vector3i& position, 
                             VoxelData::VoxelResolution resolution, 
                             const HighlightStyle& style);
    void clearVoxelHighlights();
    
    // Multi-selection highlighting
    void renderMultiSelection(const Selection::SelectionSet& selection, 
                             const HighlightStyle& style);
    void clearSelectionHighlights();
    
    // Clear all highlights
    void clearAll();
    
    // Animation
    void update(float deltaTime);
    void setGlobalAnimationEnabled(bool enabled) { m_animationEnabled = enabled; }
    bool isAnimationEnabled() const { return m_animationEnabled; }
    
    // Performance
    void setMaxHighlights(int maxCount) { m_maxHighlights = maxCount; }
    void enableInstancing(bool enabled) { m_instancingEnabled = enabled; }
    
    // Rendering
    void render(const Camera::Camera& camera);
    
private:
    struct HighlightInstance {
        Transform transform;
        Rendering::Color color;
        float animationPhase;
        HighlightStyle style;
        bool isFace;
        Face face; // Only used if isFace is true
    };
    
    std::vector<HighlightInstance> m_highlights;
    uint32_t m_highlightMesh;
    uint32_t m_faceMesh;
    uint32_t m_instanceBuffer;
    uint32_t m_highlightShader;
    float m_globalTime;
    bool m_animationEnabled;
    bool m_instancingEnabled;
    int m_maxHighlights;
    
    // Mesh generation
    void createHighlightMeshes();
    void createBoxMesh();
    void createFaceMesh();
    
    // Instance buffer management
    void updateInstanceBuffer();
    
    // Rendering methods
    void renderInstanced(const Camera::Camera& camera);
    void renderImmediate(const Camera::Camera& camera);
    
    // Animation helpers
    Rendering::Color calculateAnimatedColor(const Rendering::Color& baseColor, 
                                          float phase, float time) const;
    float calculatePulse(float time, float speed) const;
    
    // Geometry helpers
    Transform calculateVoxelTransform(const Math::Vector3i& position, 
                                     VoxelData::VoxelResolution resolution) const;
    Transform calculateFaceTransform(const Face& face) const;
};

} // namespace VisualFeedback
} // namespace VoxelEditor