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
    if (!m_renderEngine) return;
    
    // Generate box wireframe vertices
    std::vector<Rendering::Vertex> vertices;
    std::vector<uint32_t> indices;
    
    Math::Vector3f min = box.min;
    Math::Vector3f max = box.max;
    
    // Add 8 vertices for the box corners
    vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, min.y, min.z), Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, min.y, min.z), Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, max.y, min.z), Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, max.y, min.z), Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, min.y, max.z), Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, min.y, max.z), Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(Math::Vector3f(max.x, max.y, max.z), Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(Math::Vector3f(min.x, max.y, max.z), Math::Vector3f(0, 0, 0)));
    
    // Add line indices for box edges
    // Bottom face
    indices.push_back(0); indices.push_back(1);
    indices.push_back(1); indices.push_back(2);
    indices.push_back(2); indices.push_back(3);
    indices.push_back(3); indices.push_back(0);
    // Top face
    indices.push_back(4); indices.push_back(5);
    indices.push_back(5); indices.push_back(6);
    indices.push_back(6); indices.push_back(7);
    indices.push_back(7); indices.push_back(4);
    // Vertical edges
    indices.push_back(0); indices.push_back(4);
    indices.push_back(1); indices.push_back(5);
    indices.push_back(2); indices.push_back(6);
    indices.push_back(3); indices.push_back(7);
    
    // Create temporary buffers for immediate rendering
    Rendering::VertexBufferId tempVBO = m_renderEngine->createVertexBuffer(
        vertices.data(), vertices.size() * sizeof(Rendering::Vertex), Rendering::BufferUsage::Static);
    Rendering::IndexBufferId tempIBO = m_renderEngine->createIndexBuffer(
        indices.data(), indices.size() * sizeof(uint32_t), Rendering::BufferUsage::Static);
    
    // Create temporary vertex array
    Rendering::VertexLayout layout;
    layout.addAttribute(Rendering::VertexAttribute::Position, 3, Rendering::VertexAttributeType::Float);
    layout.addAttribute(Rendering::VertexAttribute::Normal, 3, Rendering::VertexAttributeType::Float);
    Rendering::VertexArrayId tempVAO = m_renderEngine->createVertexArray(layout);
    
    m_renderEngine->bindVertexBuffer(tempVAO, tempVBO, 0);
    m_renderEngine->bindIndexBuffer(tempVAO, tempIBO);
    
    // Set line width if supported
    m_renderEngine->setLineWidth(thickness);
    
    // TODO: Set shader and color uniform
    // m_renderEngine->setShader(m_resources->selectionShader);
    // m_renderEngine->setUniform("u_color", Math::Vector4f(color.r, color.g, color.b, color.a));
    
    // Draw
    m_renderEngine->setVertexArray(tempVAO);
    m_renderEngine->drawIndexed(Rendering::PrimitiveType::Lines, indices.size(), 
                               Rendering::IndexType::UInt32, 0);
    
    // Clean up temporary resources
    m_renderEngine->deleteVertexArray(tempVAO);
    m_renderEngine->deleteIndexBuffer(tempIBO);
    m_renderEngine->deleteVertexBuffer(tempVBO);
}

void SelectionRenderer::renderSphere(const Math::Vector3f& center, float radius, const Rendering::Color& color) {
    if (!m_renderEngine) return;
    
    // Generate sphere wireframe using icosahedron subdivision
    std::vector<Rendering::Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Create icosahedron vertices
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f; // Golden ratio
    const float s = 1.0f / std::sqrt(1.0f + t * t); // Normalize factor
    
    // 12 vertices of icosahedron
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f(-s, t*s, 0) * radius, Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f( s, t*s, 0) * radius, Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f(-s,-t*s, 0) * radius, Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f( s,-t*s, 0) * radius, Math::Vector3f(0, 0, 0)));
    
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f(0, -s, t*s) * radius, Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f(0,  s, t*s) * radius, Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f(0, -s,-t*s) * radius, Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f(0,  s,-t*s) * radius, Math::Vector3f(0, 0, 0)));
    
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f( t*s, 0, -s) * radius, Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f( t*s, 0,  s) * radius, Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f(-t*s, 0, -s) * radius, Math::Vector3f(0, 0, 0)));
    vertices.push_back(Rendering::Vertex(center + Math::Vector3f(-t*s, 0,  s) * radius, Math::Vector3f(0, 0, 0)));
    
    // Create icosahedron edges
    // 5 faces around vertex 0
    indices.push_back(0); indices.push_back(11);
    indices.push_back(0); indices.push_back(5);
    indices.push_back(0); indices.push_back(1);
    indices.push_back(0); indices.push_back(7);
    indices.push_back(0); indices.push_back(10);
    
    // 5 adjacent faces
    indices.push_back(11); indices.push_back(5);
    indices.push_back(5); indices.push_back(1);
    indices.push_back(1); indices.push_back(7);
    indices.push_back(7); indices.push_back(10);
    indices.push_back(10); indices.push_back(11);
    
    // 5 faces around vertex 3
    indices.push_back(3); indices.push_back(9);
    indices.push_back(3); indices.push_back(4);
    indices.push_back(3); indices.push_back(2);
    indices.push_back(3); indices.push_back(6);
    indices.push_back(3); indices.push_back(8);
    
    // 5 adjacent faces
    indices.push_back(9); indices.push_back(4);
    indices.push_back(4); indices.push_back(2);
    indices.push_back(2); indices.push_back(6);
    indices.push_back(6); indices.push_back(8);
    indices.push_back(8); indices.push_back(9);
    
    // 5 edges connecting two halves
    indices.push_back(11); indices.push_back(4);
    indices.push_back(5); indices.push_back(9);
    indices.push_back(1); indices.push_back(8);
    indices.push_back(7); indices.push_back(6);
    indices.push_back(10); indices.push_back(2);
    
    // Create and render temporary buffers
    Rendering::VertexBufferId tempVBO = m_renderEngine->createVertexBuffer(
        vertices.data(), vertices.size() * sizeof(Rendering::Vertex), Rendering::BufferUsage::Static);
    Rendering::IndexBufferId tempIBO = m_renderEngine->createIndexBuffer(
        indices.data(), indices.size() * sizeof(uint32_t), Rendering::BufferUsage::Static);
    
    Rendering::VertexLayout layout;
    layout.addAttribute(Rendering::VertexAttribute::Position, 3, Rendering::VertexAttributeType::Float);
    layout.addAttribute(Rendering::VertexAttribute::Normal, 3, Rendering::VertexAttributeType::Float);
    Rendering::VertexArrayId tempVAO = m_renderEngine->createVertexArray(layout);
    
    m_renderEngine->bindVertexBuffer(tempVAO, tempVBO, 0);
    m_renderEngine->bindIndexBuffer(tempVAO, tempIBO);
    
    // TODO: Set shader and color uniform
    // m_renderEngine->setShader(m_resources->selectionShader);
    // m_renderEngine->setUniform("u_color", Math::Vector4f(color.r, color.g, color.b, color.a));
    
    // Draw
    m_renderEngine->setVertexArray(tempVAO);
    m_renderEngine->drawIndexed(Rendering::PrimitiveType::Lines, indices.size(), 
                               Rendering::IndexType::UInt32, 0);
    
    // Clean up
    m_renderEngine->deleteVertexArray(tempVAO);
    m_renderEngine->deleteIndexBuffer(tempIBO);
    m_renderEngine->deleteVertexBuffer(tempVBO);
}

void SelectionRenderer::renderCylinder(const Math::Vector3f& base, const Math::Vector3f& direction, 
                                     float radius, float height, const Rendering::Color& color) {
    if (!m_renderEngine) return;
    
    // Generate cylinder wireframe
    std::vector<Rendering::Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Number of segments for cylinder
    const int segments = 16;
    
    // Calculate perpendicular vectors for cylinder orientation
    Math::Vector3f up = direction.normalized();
    Math::Vector3f right(1, 0, 0);
    if (std::abs(up.dot(right)) > 0.99f) {
        right = Math::Vector3f(0, 1, 0);
    }
    Math::Vector3f tangent = up.cross(right).normalized();
    Math::Vector3f bitangent = up.cross(tangent).normalized();
    
    // Generate vertices for top and bottom circles
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        
        Math::Vector3f offset = (tangent * cos_a + bitangent * sin_a) * radius;
        
        // Bottom circle vertex
        vertices.push_back(Rendering::Vertex(base + offset, Math::Vector3f(0, 0, 0)));
        // Top circle vertex
        vertices.push_back(Rendering::Vertex(base + up * height + offset, Math::Vector3f(0, 0, 0)));
    }
    
    // Generate indices for circle edges and vertical lines
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        
        // Bottom circle edge
        indices.push_back(i * 2);
        indices.push_back(next * 2);
        
        // Top circle edge
        indices.push_back(i * 2 + 1);
        indices.push_back(next * 2 + 1);
        
        // Vertical line
        indices.push_back(i * 2);
        indices.push_back(i * 2 + 1);
    }
    
    // Create and render temporary buffers
    Rendering::VertexBufferId tempVBO = m_renderEngine->createVertexBuffer(
        vertices.data(), vertices.size() * sizeof(Rendering::Vertex), Rendering::BufferUsage::Static);
    Rendering::IndexBufferId tempIBO = m_renderEngine->createIndexBuffer(
        indices.data(), indices.size() * sizeof(uint32_t), Rendering::BufferUsage::Static);
    
    Rendering::VertexLayout layout;
    layout.addAttribute(Rendering::VertexAttribute::Position, 3, Rendering::VertexAttributeType::Float);
    layout.addAttribute(Rendering::VertexAttribute::Normal, 3, Rendering::VertexAttributeType::Float);
    Rendering::VertexArrayId tempVAO = m_renderEngine->createVertexArray(layout);
    
    m_renderEngine->bindVertexBuffer(tempVAO, tempVBO, 0);
    m_renderEngine->bindIndexBuffer(tempVAO, tempIBO);
    
    // TODO: Set shader and color uniform
    // m_renderEngine->setShader(m_resources->selectionShader);
    // m_renderEngine->setUniform("u_color", Math::Vector4f(color.r, color.g, color.b, color.a));
    
    // Draw
    m_renderEngine->setVertexArray(tempVAO);
    m_renderEngine->drawIndexed(Rendering::PrimitiveType::Lines, indices.size(), 
                               Rendering::IndexType::UInt32, 0);
    
    // Clean up
    m_renderEngine->deleteVertexArray(tempVAO);
    m_renderEngine->deleteIndexBuffer(tempIBO);
    m_renderEngine->deleteVertexBuffer(tempVBO);
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
    if (!m_renderEngine) return;
    
    // Render bounds as a colored wireframe box
    Rendering::Color boundsColor(0.2f, 0.8f, 1.0f, 0.8f); // Light blue
    renderBox(bounds, boundsColor, 2.0f);
    
    // Additionally, render corner markers for better visibility
    const float markerSize = 0.05f * bounds.getDiagonalLength();
    Math::Vector3f corners[8] = {
        Math::Vector3f(bounds.min.x, bounds.min.y, bounds.min.z),
        Math::Vector3f(bounds.max.x, bounds.min.y, bounds.min.z),
        Math::Vector3f(bounds.max.x, bounds.max.y, bounds.min.z),
        Math::Vector3f(bounds.min.x, bounds.max.y, bounds.min.z),
        Math::Vector3f(bounds.min.x, bounds.min.y, bounds.max.z),
        Math::Vector3f(bounds.max.x, bounds.min.y, bounds.max.z),
        Math::Vector3f(bounds.max.x, bounds.max.y, bounds.max.z),
        Math::Vector3f(bounds.min.x, bounds.max.y, bounds.max.z)
    };
    
    // Render small spheres at each corner
    for (const auto& corner : corners) {
        renderSphere(corner, markerSize, boundsColor);
    }
}

void SelectionRenderer::renderStats(const SelectionStats& stats, const Math::Matrix4f& viewProj) {
    if (!m_renderEngine) return;
    
    // TODO: This would typically render text overlay showing selection stats
    // Since text rendering is complex and requires font systems, we'll log the stats for now
    
    // For now, render a small indicator showing the selection count
    // This could be expanded to proper text rendering when font support is added
    
    if (stats.voxelCount > 0) {
        // Render a small sphere at the selection center as an indicator
        // The size could represent the selection count
        float indicatorSize = 0.1f + 0.01f * std::log(static_cast<float>(stats.voxelCount));
        Rendering::Color indicatorColor(1.0f, 0.8f, 0.0f, 0.8f); // Golden yellow
        
        // Calculate selection center
        Math::Vector3f center = m_selectionManager ? 
            m_selectionManager->getSelection().getBounds().getCenter() : 
            Math::Vector3f(0, 0, 0);
            
        renderSphere(center, indicatorSize, indicatorColor);
        
        // Log the actual stats
        static float lastLogTime = 0.0f;
        if (m_animationTime - lastLogTime > 2.0f) { // Log every 2 seconds
            Logging::Logger::getInstance().info("Selection stats: " + 
                                              std::to_string(stats.voxelCount) + " voxels, volume: " +
                                              std::to_string(stats.totalVolume));
            lastLogTime = m_animationTime;
        }
    }
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
    if (!m_renderEngine) return;
    
    // Vertex shader for selection rendering
    const char* vertexShader = R"(
#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_viewProj;
uniform mat4 u_model;

out vec3 v_worldPos;
out vec3 v_normal;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_worldPos = worldPos.xyz;
    v_normal = normalize((u_model * vec4(a_normal, 0.0)).xyz);
    gl_Position = u_viewProj * worldPos;
}
)";
    
    // Fragment shader for selection rendering
    const char* fragmentShader = R"(
#version 330 core

in vec3 v_worldPos;
in vec3 v_normal;

uniform vec4 u_color;
uniform float u_time;
uniform bool u_animated;

out vec4 fragColor;

void main() {
    vec4 color = u_color;
    
    // Add animation effect if enabled
    if (u_animated) {
        float pulse = sin(u_time * 2.0) * 0.5 + 0.5;
        color.a *= 0.5 + pulse * 0.5;
        
        // Add rim lighting effect
        vec3 viewDir = normalize(-v_worldPos);
        float rim = 1.0 - max(0.0, dot(v_normal, viewDir));
        color.rgb += vec3(0.2, 0.3, 0.4) * rim * pulse;
    }
    
    fragColor = color;
}
)";
    
    // Create shader program
    auto shaderManager = m_renderEngine->getShaderManager();
    if (shaderManager) {
        try {
            // Try to create shader through the engine's own methods
            m_resources->selectionShader = m_renderEngine->loadShader(
                "SelectionShader", "selection.vert", "selection.frag");
                
            if (m_resources->selectionShader != Rendering::InvalidId) {
                Logging::Logger::getInstance().info("SelectionRenderer: Created selection shader successfully");
            } else {
                Logging::Logger::getInstance().error("SelectionRenderer: Failed to create selection shader");
            }
        } catch (const std::exception& e) {
            Logging::Logger::getInstance().error("SelectionRenderer: Shader creation exception: {}", e.what());
        }
    } else {
        Logging::Logger::getInstance().warning("SelectionRenderer: No shader manager available");
    }
}

}
}
