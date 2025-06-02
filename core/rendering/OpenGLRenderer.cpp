#include "OpenGLRenderer.h"
#include <iostream>

// Silence OpenGL deprecation warnings on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <glad/glad.h>

namespace VoxelEditor {
namespace Rendering {

OpenGLRenderer::OpenGLRenderer() 
    : m_contextValid(false)
    , m_nextBufferId(1)
    , m_nextTextureId(1)
    , m_nextShaderId(1)
    , m_supportsAnisotropicFiltering(false)
    , m_supportsDebugOutput(false)
    , m_supportsTimestampQueries(false)
    , m_maxAnisotropy(1.0f)
    , m_maxTextureSize(2048)
    , m_maxTextureUnits(16)
    , m_maxVertexAttributes(16) {
}

OpenGLRenderer::~OpenGLRenderer() {
    if (m_contextValid) {
        destroyContext();
    }
}

bool OpenGLRenderer::initializeContext(const RenderConfig& config) {
    if (m_contextValid) {
        return false;
    }
    
    // TODO: Initialize OpenGL context
    m_contextValid = true;
    m_rendererInfo = "OpenGL Renderer (Stub)";
    
    return true;
}

void OpenGLRenderer::destroyContext() {
    m_contextValid = false;
}

BufferId OpenGLRenderer::createVertexBuffer(const void* data, size_t size, BufferUsage usage) {
    BufferId id = m_nextBufferId++;
    BufferInfo info;
    info.id = id;
    info.usage = usage;
    info.size = size;
    info.glHandle = id; // TODO: Create actual GL buffer
    info.isIndexBuffer = false;
    m_buffers[id] = info;
    return id;
}

BufferId OpenGLRenderer::createIndexBuffer(const uint32_t* indices, size_t count, BufferUsage usage) {
    BufferId id = m_nextBufferId++;
    BufferInfo info;
    info.id = id;
    info.usage = usage;
    info.size = count * sizeof(uint32_t);
    info.glHandle = id; // TODO: Create actual GL buffer
    info.isIndexBuffer = true;
    m_buffers[id] = info;
    return id;
}

void OpenGLRenderer::bindVertexBuffer(BufferId bufferId) {
    // TODO: Implement buffer binding
}

void OpenGLRenderer::bindIndexBuffer(BufferId bufferId) {
    // TODO: Implement buffer binding
}

uint32_t OpenGLRenderer::createVertexArray() {
    // TODO: Create actual VAO
    return m_nextBufferId++;
}

void OpenGLRenderer::bindVertexArray(uint32_t vaoId) {
    // TODO: Implement VAO binding
}

void OpenGLRenderer::setupVertexAttributes(const std::vector<VertexAttribute>& attributes) {
    // TODO: Implement vertex attribute setup
}

void OpenGLRenderer::useProgram(ShaderId programId) {
    // TODO: Implement shader program binding
}

void OpenGLRenderer::setUniform(const std::string& name, const UniformValue& value) {
    // TODO: Implement uniform setting
}

void OpenGLRenderer::drawElements(PrimitiveType type, int count, IndexType indexType, int offset) {
    // TODO: Implement element drawing
}

void OpenGLRenderer::clear(ClearFlags flags, const Color& color, float depth, int stencil) {
    glClearColor(color.r, color.g, color.b, color.a);
    
    GLbitfield clearMask = 0;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ClearFlags::COLOR)) clearMask |= GL_COLOR_BUFFER_BIT;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ClearFlags::DEPTH)) clearMask |= GL_DEPTH_BUFFER_BIT;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ClearFlags::STENCIL)) clearMask |= GL_STENCIL_BUFFER_BIT;
    
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ClearFlags::DEPTH)) {
        glClearDepth(depth);
    }
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ClearFlags::STENCIL)) {
        glClearStencil(stencil);
    }
    
    glClear(clearMask);
}

void OpenGLRenderer::setClearColor(const Color& color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLRenderer::setViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

} // namespace Rendering
} // namespace VoxelEditor