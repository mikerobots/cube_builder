#include "SelectionRenderer.h"
#include "../../foundation/logging/Logger.h"
#include <cmath>
#include <vector>

namespace VoxelEditor {
namespace Selection {

SelectionRenderer::SelectionRenderer(Rendering::RenderEngine* renderEngine)
    : m_renderEngine(renderEngine)
    , m_selectionManager(nullptr)
    , m_renderMode(SelectionRenderMode::OutlineAndFill)
    , m_showGizmos(true)
    , m_animationTime(0.0f)
    , m_resources(std::make_unique<RenderResources>()) {
    m_resources->needsUpdate = true;
}

SelectionRenderer::~SelectionRenderer() {
    shutdown();
}

bool SelectionRenderer::initialize() {
    if (!m_renderEngine) {
        Logging::Logger::getInstance().error("SelectionRenderer::initialize: No render engine set");
        return false;
    }
    
    // Create vertex buffers
    m_resources->outlineVBO = m_renderEngine->createVertexBuffer(nullptr, 0, Rendering::BufferUsage::Dynamic);
    m_resources->fillVBO = m_renderEngine->createVertexBuffer(nullptr, 0, Rendering::BufferUsage::Dynamic);
    
    // Create index buffers
    m_resources->outlineIBO = m_renderEngine->createIndexBuffer(nullptr, 0, Rendering::BufferUsage::Dynamic);
    m_resources->fillIBO = m_renderEngine->createIndexBuffer(nullptr, 0, Rendering::BufferUsage::Dynamic);
    
    // Create vertex arrays
    Rendering::VertexLayout layout;
    layout.addAttribute(Rendering::VertexAttribute::Position, 3, Rendering::VertexAttributeType::Float);
    layout.addAttribute(Rendering::VertexAttribute::Normal, 3, Rendering::VertexAttributeType::Float);
    
    m_resources->outlineVAO = m_renderEngine->createVertexArray(layout);
    m_resources->fillVAO = m_renderEngine->createVertexArray(layout);
    
    // Bind buffers to vertex arrays
    m_renderEngine->bindVertexBuffer(m_resources->outlineVAO, m_resources->outlineVBO, 0);
    m_renderEngine->bindIndexBuffer(m_resources->outlineVAO, m_resources->outlineIBO);
    
    m_renderEngine->bindVertexBuffer(m_resources->fillVAO, m_resources->fillVBO, 0);
    m_renderEngine->bindIndexBuffer(m_resources->fillVAO, m_resources->fillIBO);
    
    // Create shaders
    createShaders();
    
    return true;
}

void SelectionRenderer::shutdown() {
    if (!m_renderEngine) return;
    
    // Clean up resources
    if (m_resources->outlineVBO != Rendering::InvalidId) {
        m_renderEngine->deleteVertexBuffer(m_resources->outlineVBO);
    }
    if (m_resources->fillVBO != Rendering::InvalidId) {
        m_renderEngine->deleteVertexBuffer(m_resources->fillVBO);
    }
    if (m_resources->outlineIBO != Rendering::InvalidId) {
        m_renderEngine->deleteIndexBuffer(m_resources->outlineIBO);
    }
    if (m_resources->fillIBO != Rendering::InvalidId) {
        m_renderEngine->deleteIndexBuffer(m_resources->fillIBO);
    }
    if (m_resources->outlineVAO != Rendering::InvalidId) {
        m_renderEngine->deleteVertexArray(m_resources->outlineVAO);
    }
    if (m_resources->fillVAO != Rendering::InvalidId) {
        m_renderEngine->deleteVertexArray(m_resources->fillVAO);
    }
}

void SelectionRenderer::render(const Camera::Camera* camera, float deltaTime) {
    if (!m_renderEngine || !m_selectionManager || !camera) return;
    
    const SelectionSet& selection = m_selectionManager->getSelection();
    if (selection.empty()) return;
    
    // Update animation time
    m_animationTime += deltaTime * m_style.animationSpeed;
    
    // Update geometry if needed
    if (m_resources->needsUpdate) {
        updateGeometry(selection);
        m_resources->needsUpdate = false;
    }
    
    // Calculate view-projection matrix
    Math::Matrix4f viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();
    
    // Render based on mode
    switch (m_renderMode) {
        case SelectionRenderMode::Outline:
            renderOutline(viewProj);
            break;
        case SelectionRenderMode::Fill:
            renderFill(viewProj);
            break;
        case SelectionRenderMode::OutlineAndFill:
            renderFill(viewProj);
            renderOutline(viewProj);
            break;
        case SelectionRenderMode::Highlight:
            renderFill(viewProj);
            renderOutline(viewProj);
            break;
    }
    
    // Render additional UI elements
    if (m_showGizmos) {
        if (m_style.showBounds) {
            renderBounds(selection.getBounds(), viewProj);
        }
        if (m_style.showCount) {
            renderStats(selection.getStats(), viewProj);
        }
    }
}

void SelectionRenderer::renderPreview(const SelectionSet& preview, const Camera::Camera* camera) {
    if (!m_renderEngine || !camera || preview.empty()) return;
    
    // Temporarily update geometry for preview
    updateGeometry(preview);
    
    // Render with preview style
    SelectionStyle previewStyle = m_style;
    previewStyle.outlineColor.a *= 0.5f;
    previewStyle.fillColor.a *= 0.5f;
    
    Math::Matrix4f viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();
    renderFill(viewProj);
    renderOutline(viewProj);
    
    // Mark for update to restore regular selection
    m_resources->needsUpdate = true;
}

void SelectionRenderer::renderBox(const Math::BoundingBox& box, const Rendering::Color& color, float thickness) {
    // TODO: Implement box rendering
    Logging::Logger::getInstance().warning("SelectionRenderer::renderBox: Not implemented");
}

void SelectionRenderer::renderSphere(const Math::Vector3f& center, float radius, const Rendering::Color& color) {
    // TODO: Implement sphere rendering
    Logging::Logger::getInstance().warning("SelectionRenderer::renderSphere: Not implemented");
}

void SelectionRenderer::renderCylinder(const Math::Vector3f& base, const Math::Vector3f& direction, 
                                     float radius, float height, const Rendering::Color& color) {
    // TODO: Implement cylinder rendering
    Logging::Logger::getInstance().warning("SelectionRenderer::renderCylinder: Not implemented");
}

void SelectionRenderer::updateGeometry(const SelectionSet& selection) {
    generateOutlineGeometry(selection);
    generateFillGeometry(selection);
}

void SelectionRenderer::generateOutlineGeometry(const SelectionSet& selection) {
    std::vector<Rendering::Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Generate wireframe cube for each voxel
    uint32_t vertexOffset = 0;
    for (const auto& voxel : selection) {
        Math::BoundingBox bounds = voxel.getBounds();
        Math::Vector3f min = bounds.min;
        Math::Vector3f max = bounds.max;
        
        // Add 8 vertices for the cube corners
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, min.y, min.z), Math::Vector3f(0, 0, 0)));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, min.y, min.z), Math::Vector3f(0, 0, 0)));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, max.y, min.z), Math::Vector3f(0, 0, 0)));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, max.y, min.z), Math::Vector3f(0, 0, 0)));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, min.y, max.z), Math::Vector3f(0, 0, 0)));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, min.y, max.z), Math::Vector3f(0, 0, 0)));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, max.y, max.z), Math::Vector3f(0, 0, 0)));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, max.y, max.z), Math::Vector3f(0, 0, 0)));
        
        // Add line indices (12 edges of the cube)
        uint32_t base = vertexOffset;
        // Bottom face
        indices.push_back(base + 0); indices.push_back(base + 1);
        indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 2); indices.push_back(base + 3);
        indices.push_back(base + 3); indices.push_back(base + 0);
        // Top face
        indices.push_back(base + 4); indices.push_back(base + 5);
        indices.push_back(base + 5); indices.push_back(base + 6);
        indices.push_back(base + 6); indices.push_back(base + 7);
        indices.push_back(base + 7); indices.push_back(base + 4);
        // Vertical edges
        indices.push_back(base + 0); indices.push_back(base + 4);
        indices.push_back(base + 1); indices.push_back(base + 5);
        indices.push_back(base + 2); indices.push_back(base + 6);
        indices.push_back(base + 3); indices.push_back(base + 7);
        
        vertexOffset += 8;
    }
    
    // Update buffers
    if (!vertices.empty()) {
        m_renderEngine->updateVertexBuffer(m_resources->outlineVBO, vertices.data(), 
                                         vertices.size() * sizeof(Rendering::Vertex), 0);
        m_renderEngine->updateIndexBuffer(m_resources->outlineIBO, indices.data(), 
                                        indices.size() * sizeof(uint32_t), 0);
        m_resources->outlineVertexCount = indices.size();
    }
}

void SelectionRenderer::generateFillGeometry(const SelectionSet& selection) {
    std::vector<Rendering::Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Generate solid cube for each voxel
    uint32_t vertexOffset = 0;
    for (const auto& voxel : selection) {
        Math::BoundingBox bounds = voxel.getBounds();
        Math::Vector3f min = bounds.min;
        Math::Vector3f max = bounds.max;
        Math::Vector3f center = bounds.getCenter();
        
        // Add vertices for 6 faces
        // Front face (positive Z)
        Math::Vector3f n1(0, 0, 1);
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, min.y, max.z), n1));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, min.y, max.z), n1));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, max.y, max.z), n1));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, max.y, max.z), n1));
        
        // Back face (negative Z)
        Math::Vector3f n2(0, 0, -1);
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, min.y, min.z), n2));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, min.y, min.z), n2));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, max.y, min.z), n2));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, max.y, min.z), n2));
        
        // Right face (positive X)
        Math::Vector3f n3(1, 0, 0);
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, min.y, min.z), n3));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, min.y, max.z), n3));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, max.y, max.z), n3));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, max.y, min.z), n3));
        
        // Left face (negative X)
        Math::Vector3f n4(-1, 0, 0);
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, min.y, max.z), n4));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, min.y, min.z), n4));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, max.y, min.z), n4));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, max.y, max.z), n4));
        
        // Top face (positive Y)
        Math::Vector3f n5(0, 1, 0);
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, max.y, max.z), n5));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, max.y, max.z), n5));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, max.y, min.z), n5));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, max.y, min.z), n5));
        
        // Bottom face (negative Y)
        Math::Vector3f n6(0, -1, 0);
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, min.y, min.z), n6));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, min.y, min.z), n6));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, min.y, max.z), n6));
        vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, min.y, max.z), n6));
        
        // Add indices for 6 faces (2 triangles per face)
        for (int face = 0; face < 6; ++face) {
            uint32_t base = vertexOffset + face * 4;
            indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
        }
        
        vertexOffset += 24; // 6 faces * 4 vertices
    }
    
    // Update buffers
    if (!vertices.empty()) {
        m_renderEngine->updateVertexBuffer(m_resources->fillVBO, vertices.data(), 
                                         vertices.size() * sizeof(Rendering::Vertex), 0);
        m_renderEngine->updateIndexBuffer(m_resources->fillIBO, indices.data(), 
                                        indices.size() * sizeof(uint32_t), 0);
        m_resources->fillVertexCount = indices.size();
    }
}

void SelectionRenderer::renderOutline(const Math::Matrix4f& viewProj) {
    if (m_resources->outlineVertexCount == 0) return;
    
    // TODO: Set shader and uniforms
    // m_renderEngine->setShader(m_resources->selectionShader);
    // m_renderEngine->setUniform("u_viewProj", viewProj);
    // m_renderEngine->setUniform("u_color", getAnimatedColor(m_style.outlineColor));
    // m_renderEngine->setUniform("u_thickness", m_style.outlineThickness);
    
    // Draw lines
    m_renderEngine->setVertexArray(m_resources->outlineVAO);
    m_renderEngine->drawIndexed(Rendering::PrimitiveType::Lines, m_resources->outlineVertexCount, 
                               Rendering::IndexType::UInt32, 0);
}

void SelectionRenderer::renderFill(const Math::Matrix4f& viewProj) {
    if (m_resources->fillVertexCount == 0) return;
    
    // Enable alpha blending
    m_renderEngine->setBlendMode(Rendering::BlendMode::Alpha);
    m_renderEngine->setDepthWrite(false);
    
    // TODO: Set shader and uniforms
    // m_renderEngine->setShader(m_resources->selectionShader);
    // m_renderEngine->setUniform("u_viewProj", viewProj);
    // m_renderEngine->setUniform("u_color", getAnimatedColor(m_style.fillColor));
    
    // Draw triangles
    m_renderEngine->setVertexArray(m_resources->fillVAO);
    m_renderEngine->drawIndexed(Rendering::PrimitiveType::Triangles, m_resources->fillVertexCount, 
                               Rendering::IndexType::UInt32, 0);
    
    // Restore state
    m_renderEngine->setDepthWrite(true);
    m_renderEngine->setBlendMode(Rendering::BlendMode::Opaque);
}

void SelectionRenderer::renderBounds(const Math::BoundingBox& bounds, const Math::Matrix4f& viewProj) {
    // TODO: Implement bounds rendering
}

void SelectionRenderer::renderStats(const SelectionStats& stats, const Math::Matrix4f& viewProj) {
    // TODO: Implement stats rendering (text overlay)
}

Math::Vector4f SelectionRenderer::getAnimatedColor(const Rendering::Color& baseColor) {
    if (!m_style.animated || m_renderMode != SelectionRenderMode::Highlight) {
        return Math::Vector4f(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    }
    
    // Pulse alpha for highlight effect
    float pulse = (std::sin(m_animationTime * 2.0f) + 1.0f) * 0.5f;
    float alpha = baseColor.a * (0.5f + pulse * 0.5f);
    
    return Math::Vector4f(baseColor.r, baseColor.g, baseColor.b, alpha);
}

void SelectionRenderer::createShaders() {
    // TODO: Create selection shader program
    // This would typically load vertex and fragment shaders for selection rendering
    Logging::Logger::getInstance().info("SelectionRenderer::createShaders: Shader creation not implemented");
}

}
}
