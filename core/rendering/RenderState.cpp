#include "RenderState.h"

namespace VoxelEditor {
namespace Rendering {

RenderState::RenderState()
    : m_stateChanges(0)
    , m_shaderSwitches(0)
    , m_textureBinds(0)
    , m_stateDirty(false)
    , m_forceNextChange(false) {
    // Initialize to default OpenGL state
    reset();
}

void RenderState::reset() {
    m_current = StateBlock();
    m_pending = StateBlock();
    m_stateDirty = false;
    m_forceNextChange = true;
}

void RenderState::setDepthTest(bool enabled) {
    if (m_pending.depthTest != enabled) {
        m_pending.depthTest = enabled;
        markStateDirty();
    }
}

void RenderState::setDepthWrite(bool enabled) {
    if (m_pending.depthWrite != enabled) {
        m_pending.depthWrite = enabled;
        markStateDirty();
    }
}

void RenderState::setBlending(bool enabled, BlendMode mode) {
    if (m_pending.blending != enabled || m_pending.blendMode != mode) {
        m_pending.blending = enabled;
        m_pending.blendMode = mode;
        markStateDirty();
    }
}

void RenderState::setCulling(bool enabled, CullMode mode) {
    if (m_pending.culling != enabled || m_pending.cullMode != mode) {
        m_pending.culling = enabled;
        m_pending.cullMode = mode;
        markStateDirty();
    }
}

void RenderState::setPolygonMode(bool wireframe) {
    if (m_pending.wireframe != wireframe) {
        m_pending.wireframe = wireframe;
        markStateDirty();
    }
}

void RenderState::setLineWidth(float width) {
    if (m_pending.lineWidth != width) {
        m_pending.lineWidth = width;
        markStateDirty();
    }
}

void RenderState::setPointSize(float size) {
    if (m_pending.pointSize != size) {
        m_pending.pointSize = size;
        markStateDirty();
    }
}

void RenderState::bindShader(ShaderId id) {
    if (m_pending.boundShader != id) {
        m_pending.boundShader = id;
        markStateDirty();
    }
}

void RenderState::bindTexture(TextureId textureId, int slot) {
    if (slot >= 0 && slot < static_cast<int>(m_pending.boundTextures.size())) {
        if (m_pending.boundTextures[slot] != textureId) {
            m_pending.boundTextures[slot] = textureId;
            markStateDirty();
        }
    }
}

void RenderState::bindVertexArray(uint32_t vaoId) {
    if (m_pending.boundVertexArray != vaoId) {
        m_pending.boundVertexArray = vaoId;
        markStateDirty();
    }
}

void RenderState::bindVertexBuffer(BufferId bufferId) {
    if (m_pending.boundVertexBuffer != bufferId) {
        m_pending.boundVertexBuffer = bufferId;
        markStateDirty();
    }
}

void RenderState::bindIndexBuffer(BufferId bufferId) {
    if (m_pending.boundIndexBuffer != bufferId) {
        m_pending.boundIndexBuffer = bufferId;
        markStateDirty();
    }
}

void RenderState::setViewport(int x, int y, int width, int height) {
    if (m_pending.viewportX != x || m_pending.viewportY != y ||
        m_pending.viewportWidth != width || m_pending.viewportHeight != height) {
        m_pending.viewportX = x;
        m_pending.viewportY = y;
        m_pending.viewportWidth = width;
        m_pending.viewportHeight = height;
        markStateDirty();
    }
}

void RenderState::setClearColor(const Color& color) {
    if (m_pending.clearColor.r != color.r || m_pending.clearColor.g != color.g ||
        m_pending.clearColor.b != color.b || m_pending.clearColor.a != color.a) {
        m_pending.clearColor = color;
        markStateDirty();
    }
}

void RenderState::forceStateChange() {
    m_forceNextChange = true;
}

void RenderState::flush() {
    if (!m_stateDirty && !m_forceNextChange) {
        return;
    }
    
    // Apply all state changes
    if (needsDepthStateUpdate()) applyDepthState();
    if (needsBlendingStateUpdate()) applyBlendingState();
    if (needsCullingStateUpdate()) applyCullingState();
    if (needsPolygonStateUpdate()) applyPolygonState();
    if (needsShaderStateUpdate()) applyShaderState();
    if (needsTextureStateUpdate()) applyTextureState();
    if (needsBufferStateUpdate()) applyBufferState();
    if (needsViewportStateUpdate()) applyViewportState();
    if (needsClearStateUpdate()) applyClearState();
    
    // Update current state
    m_current = m_pending;
    m_stateDirty = false;
    m_forceNextChange = false;
}

void RenderState::resetStatistics() {
    m_stateChanges = 0;
    m_shaderSwitches = 0;
    m_textureBinds = 0;
}

// Private helper methods
void RenderState::markStateDirty() {
    m_stateDirty = true;
}

bool RenderState::needsDepthStateUpdate() const {
    return m_forceNextChange || 
           m_current.depthTest != m_pending.depthTest ||
           m_current.depthWrite != m_pending.depthWrite;
}

bool RenderState::needsBlendingStateUpdate() const {
    return m_forceNextChange || 
           m_current.blending != m_pending.blending ||
           m_current.blendMode != m_pending.blendMode;
}

bool RenderState::needsCullingStateUpdate() const {
    return m_forceNextChange || 
           m_current.culling != m_pending.culling ||
           m_current.cullMode != m_pending.cullMode;
}

bool RenderState::needsPolygonStateUpdate() const {
    return m_forceNextChange || 
           m_current.wireframe != m_pending.wireframe ||
           m_current.lineWidth != m_pending.lineWidth ||
           m_current.pointSize != m_pending.pointSize;
}

bool RenderState::needsShaderStateUpdate() const {
    return m_forceNextChange || 
           m_current.boundShader != m_pending.boundShader;
}

bool RenderState::needsTextureStateUpdate() const {
    if (m_forceNextChange) return true;
    
    for (size_t i = 0; i < m_current.boundTextures.size(); ++i) {
        if (m_current.boundTextures[i] != m_pending.boundTextures[i]) {
            return true;
        }
    }
    return false;
}

bool RenderState::needsBufferStateUpdate() const {
    return m_forceNextChange || 
           m_current.boundVertexArray != m_pending.boundVertexArray ||
           m_current.boundVertexBuffer != m_pending.boundVertexBuffer ||
           m_current.boundIndexBuffer != m_pending.boundIndexBuffer;
}

bool RenderState::needsViewportStateUpdate() const {
    return m_forceNextChange || 
           m_current.viewportX != m_pending.viewportX ||
           m_current.viewportY != m_pending.viewportY ||
           m_current.viewportWidth != m_pending.viewportWidth ||
           m_current.viewportHeight != m_pending.viewportHeight;
}

bool RenderState::needsClearStateUpdate() const {
    return m_forceNextChange || 
           m_current.clearColor.r != m_pending.clearColor.r ||
           m_current.clearColor.g != m_pending.clearColor.g ||
           m_current.clearColor.b != m_pending.clearColor.b ||
           m_current.clearColor.a != m_pending.clearColor.a;
}

// State application stubs - these would normally call OpenGL functions
void RenderState::applyDepthState() {
    applyDepthTestGL(m_pending.depthTest);
    applyDepthWriteGL(m_pending.depthWrite);
    m_stateChanges++;
}

void RenderState::applyBlendingState() {
    applyBlendingGL(m_pending.blending, m_pending.blendMode);
    m_stateChanges++;
}

void RenderState::applyCullingState() {
    applyCullingGL(m_pending.culling, m_pending.cullMode);
    m_stateChanges++;
}

void RenderState::applyPolygonState() {
    applyPolygonModeGL(m_pending.wireframe);
    applyLineWidthGL(m_pending.lineWidth);
    applyPointSizeGL(m_pending.pointSize);
    m_stateChanges++;
}

void RenderState::applyShaderState() {
    applyShaderGL(m_pending.boundShader);
    m_shaderSwitches++;
    m_stateChanges++;
}

void RenderState::applyTextureState() {
    for (size_t i = 0; i < m_pending.boundTextures.size(); ++i) {
        if (m_current.boundTextures[i] != m_pending.boundTextures[i]) {
            applyTextureGL(m_pending.boundTextures[i], static_cast<int>(i));
            m_textureBinds++;
        }
    }
    m_stateChanges++;
}

void RenderState::applyBufferState() {
    if (m_current.boundVertexArray != m_pending.boundVertexArray) {
        applyVertexArrayGL(m_pending.boundVertexArray);
    }
    if (m_current.boundVertexBuffer != m_pending.boundVertexBuffer) {
        applyVertexBufferGL(m_pending.boundVertexBuffer);
    }
    if (m_current.boundIndexBuffer != m_pending.boundIndexBuffer) {
        applyIndexBufferGL(m_pending.boundIndexBuffer);
    }
    m_stateChanges++;
}

void RenderState::applyViewportState() {
    applyViewportGL(m_pending.viewportX, m_pending.viewportY, 
                    m_pending.viewportWidth, m_pending.viewportHeight);
    m_stateChanges++;
}

void RenderState::applyClearState() {
    applyClearColorGL(m_pending.clearColor);
    m_stateChanges++;
}

// OpenGL state application stubs
void RenderState::applyDepthTestGL(bool enabled) {
    // Would call glEnable/glDisable(GL_DEPTH_TEST)
}

void RenderState::applyDepthWriteGL(bool enabled) {
    // Would call glDepthMask
}

void RenderState::applyBlendingGL(bool enabled, BlendMode mode) {
    // Would call glEnable/glDisable(GL_BLEND) and glBlendFunc
}

void RenderState::applyCullingGL(bool enabled, CullMode mode) {
    // Would call glEnable/glDisable(GL_CULL_FACE) and glCullFace
}

void RenderState::applyPolygonModeGL(bool wireframe) {
    // Would call glPolygonMode
}

void RenderState::applyLineWidthGL(float width) {
    // Would call glLineWidth
}

void RenderState::applyPointSizeGL(float size) {
    // Would call glPointSize
}

void RenderState::applyShaderGL(ShaderId id) {
    // Would call glUseProgram
}

void RenderState::applyTextureGL(TextureId textureId, int slot) {
    // Would call glActiveTexture and glBindTexture
}

void RenderState::applyVertexArrayGL(uint32_t vaoId) {
    // Would call glBindVertexArray
}

void RenderState::applyVertexBufferGL(BufferId bufferId) {
    // Would call glBindBuffer(GL_ARRAY_BUFFER, ...)
}

void RenderState::applyIndexBufferGL(BufferId bufferId) {
    // Would call glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)
}

void RenderState::applyViewportGL(int x, int y, int width, int height) {
    // Would call glViewport
}

void RenderState::applyClearColorGL(const Color& color) {
    // Would call glClearColor
}

} // namespace Rendering
} // namespace VoxelEditor