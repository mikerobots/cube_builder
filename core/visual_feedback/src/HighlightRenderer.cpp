#include "../include/visual_feedback/HighlightRenderer.h"
#include <algorithm>
#include <cmath>

namespace VoxelEditor {
namespace VisualFeedback {

HighlightRenderer::HighlightRenderer()
    : m_highlightMesh(0)
    , m_faceMesh(0)
    , m_instanceBuffer(0)
    , m_highlightShader(0)
    , m_globalTime(0.0f)
    , m_animationEnabled(true)
    , m_instancingEnabled(true)
    , m_maxHighlights(1000) {
    
    createHighlightMeshes();
}

HighlightRenderer::~HighlightRenderer() {
    // TODO: Clean up GPU resources
}

void HighlightRenderer::renderFaceHighlight(const Face& face, const HighlightStyle& style) {
    if (m_highlights.size() >= static_cast<size_t>(m_maxHighlights)) {
        return; // Limit reached
    }
    
    HighlightInstance instance;
    instance.transform = calculateFaceTransform(face);
    instance.color = style.color;
    instance.animationPhase = 0.0f;
    instance.style = style;
    instance.isFace = true;
    instance.face = face;
    
    m_highlights.push_back(instance);
}

void HighlightRenderer::clearFaceHighlights() {
    m_highlights.erase(
        std::remove_if(m_highlights.begin(), m_highlights.end(),
            [](const HighlightInstance& h) { return h.isFace; }),
        m_highlights.end()
    );
}

void HighlightRenderer::renderVoxelHighlight(const Math::Vector3i& position, 
                                            VoxelData::VoxelResolution resolution, 
                                            const HighlightStyle& style) {
    if (m_highlights.size() >= static_cast<size_t>(m_maxHighlights)) {
        return; // Limit reached
    }
    
    HighlightInstance instance;
    instance.transform = calculateVoxelTransform(position, resolution);
    instance.color = style.color;
    instance.animationPhase = 0.0f;
    instance.style = style;
    instance.isFace = false;
    
    m_highlights.push_back(instance);
}

void HighlightRenderer::clearVoxelHighlights() {
    m_highlights.erase(
        std::remove_if(m_highlights.begin(), m_highlights.end(),
            [](const HighlightInstance& h) { return !h.isFace; }),
        m_highlights.end()
    );
}

void HighlightRenderer::renderMultiSelection(const Selection::SelectionSet& selection, 
                                            const HighlightStyle& style) {
    // TODO: Iterate through selection when VoxelId structure is finalized
    // for (const auto& voxelId : selection) {
    //     // Extract position and resolution from VoxelId
    //     renderVoxelHighlight(position, resolution, style);
    // }
}

void HighlightRenderer::clearSelectionHighlights() {
    clearVoxelHighlights();
}

void HighlightRenderer::clearAll() {
    m_highlights.clear();
}

void HighlightRenderer::update(float deltaTime) {
    if (m_animationEnabled) {
        m_globalTime += deltaTime;
        
        // Update animation phases
        for (auto& highlight : m_highlights) {
            if (highlight.style.animated) {
                highlight.animationPhase += deltaTime * highlight.style.pulseSpeed;
            }
        }
    }
}

void HighlightRenderer::render(const Camera::Camera& camera) {
    if (m_highlights.empty()) {
        return;
    }
    
    if (m_instancingEnabled && m_instanceBuffer != 0) {
        renderInstanced(camera);
    } else {
        renderImmediate(camera);
    }
}

void HighlightRenderer::createHighlightMeshes() {
    createBoxMesh();
    createFaceMesh();
    
    // TODO: Create shader program
    // m_highlightShader = createHighlightShader();
}

void HighlightRenderer::createBoxMesh() {
    // Create unit cube mesh for voxel highlights
    // TODO: Create GPU mesh
}

void HighlightRenderer::createFaceMesh() {
    // Create quad mesh for face highlights
    // TODO: Create GPU mesh
}

void HighlightRenderer::updateInstanceBuffer() {
    if (m_highlights.empty()) {
        return;
    }
    
    // TODO: Update GPU instance buffer with highlight data
}

void HighlightRenderer::renderInstanced(const Camera::Camera& camera) {
    updateInstanceBuffer();
    
    // TODO: Set shader uniforms
    // - View/projection matrices
    // - Global time
    // - Animation settings
    
    // TODO: Render instanced highlights
}

void HighlightRenderer::renderImmediate(const Camera::Camera& camera) {
    // TODO: Render each highlight individually
    for (const auto& highlight : m_highlights) {
        // Set transform
        // Set color with animation
        // Draw mesh
    }
}

Rendering::Color HighlightRenderer::calculateAnimatedColor(const Rendering::Color& baseColor, 
                                                         float phase, float time) const {
    if (!m_animationEnabled) {
        return baseColor;
    }
    
    // Calculate pulse effect
    float pulse = calculatePulse(time + phase, 1.0f);
    
    // Modulate alpha and intensity
    Rendering::Color animatedColor = baseColor;
    animatedColor.a *= (0.5f + 0.5f * pulse);
    
    return animatedColor;
}

float HighlightRenderer::calculatePulse(float time, float speed) const {
    return (std::sin(time * speed * 2.0f * 3.14159f) + 1.0f) * 0.5f;
}

Transform HighlightRenderer::calculateVoxelTransform(const Math::Vector3i& position, 
                                                   VoxelData::VoxelResolution resolution) const {
    Transform transform;
    
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Position at voxel center
    transform.position = Math::Vector3f(
        position.x * voxelSize + voxelSize * 0.5f,
        position.y * voxelSize + voxelSize * 0.5f,
        position.z * voxelSize + voxelSize * 0.5f
    );
    
    // Scale to voxel size
    transform.scale = Math::Vector3f(voxelSize, voxelSize, voxelSize);
    
    // No rotation
    transform.rotation = Math::Quaternion();
    
    return transform;
}

Transform HighlightRenderer::calculateFaceTransform(const Face& face) const {
    Transform transform;
    
    // Get face center
    transform.position = face.getCenter();
    
    // Scale based on face size
    float voxelSize = VoxelData::getVoxelSize(face.getResolution());
    
    switch (face.getDirection()) {
        case FaceDirection::PositiveX:
        case FaceDirection::NegativeX:
            transform.scale = Math::Vector3f(0.01f, voxelSize, voxelSize);
            break;
        case FaceDirection::PositiveY:
        case FaceDirection::NegativeY:
            transform.scale = Math::Vector3f(voxelSize, 0.01f, voxelSize);
            break;
        case FaceDirection::PositiveZ:
        case FaceDirection::NegativeZ:
            transform.scale = Math::Vector3f(voxelSize, voxelSize, 0.01f);
            break;
    }
    
    // TODO: Calculate rotation to align with face normal
    
    return transform;
}

} // namespace VisualFeedback
} // namespace VoxelEditor