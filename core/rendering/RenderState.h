#pragma once

#include "RenderTypes.h"
#include <array>

namespace VoxelEditor {
namespace Rendering {

class RenderState {
public:
    RenderState();
    ~RenderState() = default;
    
    // State management
    void setDepthTest(bool enabled);
    void setDepthWrite(bool enabled);
    void setBlending(bool enabled, BlendMode mode = BlendMode::Alpha);
    void setCulling(bool enabled, CullMode mode = CullMode::Back);
    void setPolygonMode(bool wireframe);
    void setLineWidth(float width);
    void setPointSize(float size);
    
    // Texture and shader binding
    void bindShader(ShaderId id);
    void bindTexture(TextureId textureId, int slot);
    void bindVertexArray(uint32_t vaoId);
    void bindVertexBuffer(BufferId bufferId);
    void bindIndexBuffer(BufferId bufferId);
    
    // Viewport state
    void setViewport(int x, int y, int width, int height);
    
    // Clear state
    void setClearColor(const Color& color);
    
    // State forcing and flushing
    void forceStateChange(); // Forces next state change to apply
    void flush();           // Apply all pending state changes
    void reset();           // Reset to default state
    
    // Statistics
    uint32_t getStateChanges() const { return m_stateChanges; }
    uint32_t getShaderSwitches() const { return m_shaderSwitches; }
    uint32_t getTextureBinds() const { return m_textureBinds; }
    void resetStatistics();
    
    // State queries - return pending state since that's what will be applied
    bool isDepthTestEnabled() const { return m_pending.depthTest; }
    bool isDepthWriteEnabled() const { return m_pending.depthWrite; }
    bool isBlendingEnabled() const { return m_pending.blending; }
    BlendMode getBlendMode() const { return m_pending.blendMode; }
    bool isCullingEnabled() const { return m_pending.culling; }
    CullMode getCullMode() const { return m_pending.cullMode; }
    ShaderId getBoundShader() const { return m_pending.boundShader; }
    
private:
    struct StateBlock {
        // Depth state
        bool depthTest = true;
        bool depthWrite = true;
        
        // Blending state
        bool blending = false;
        BlendMode blendMode = BlendMode::Opaque;
        
        // Culling state
        bool culling = true;
        CullMode cullMode = CullMode::Back;
        
        // Polygon state
        bool wireframe = false;
        float lineWidth = 1.0f;
        float pointSize = 1.0f;
        
        // Bound resources
        ShaderId boundShader = 0;
        std::array<TextureId, 16> boundTextures = {}; // Support up to 16 texture units
        uint32_t boundVertexArray = 0;
        BufferId boundVertexBuffer = 0;
        BufferId boundIndexBuffer = 0;
        
        // Viewport state
        int viewportX = 0;
        int viewportY = 0;
        int viewportWidth = 0;
        int viewportHeight = 0;
        
        // Clear state
        Color clearColor = Color::Black();
        
        bool operator==(const StateBlock& other) const {
            return depthTest == other.depthTest &&
                   depthWrite == other.depthWrite &&
                   blending == other.blending &&
                   blendMode == other.blendMode &&
                   culling == other.culling &&
                   cullMode == other.cullMode &&
                   wireframe == other.wireframe &&
                   lineWidth == other.lineWidth &&
                   pointSize == other.pointSize &&
                   boundShader == other.boundShader &&
                   boundTextures == other.boundTextures &&
                   boundVertexArray == other.boundVertexArray &&
                   boundVertexBuffer == other.boundVertexBuffer &&
                   boundIndexBuffer == other.boundIndexBuffer &&
                   viewportX == other.viewportX &&
                   viewportY == other.viewportY &&
                   viewportWidth == other.viewportWidth &&
                   viewportHeight == other.viewportHeight &&
                   clearColor.r == other.clearColor.r &&
                   clearColor.g == other.clearColor.g &&
                   clearColor.b == other.clearColor.b &&
                   clearColor.a == other.clearColor.a;
        }
        
        bool operator!=(const StateBlock& other) const {
            return !(*this == other);
        }
    };
    
    StateBlock m_current;
    StateBlock m_pending;
    
    // State change tracking
    uint32_t m_stateChanges;
    uint32_t m_shaderSwitches;
    uint32_t m_textureBinds;
    bool m_stateDirty;
    bool m_forceNextChange;
    
    // State application methods
    void applyDepthState();
    void applyBlendingState();
    void applyCullingState();
    void applyPolygonState();
    void applyShaderState();
    void applyTextureState();
    void applyBufferState();
    void applyViewportState();
    void applyClearState();
    
    // Helper methods
    void markStateDirty();
    bool needsDepthStateUpdate() const;
    bool needsBlendingStateUpdate() const;
    bool needsCullingStateUpdate() const;
    bool needsPolygonStateUpdate() const;
    bool needsShaderStateUpdate() const;
    bool needsTextureStateUpdate() const;
    bool needsBufferStateUpdate() const;
    bool needsViewportStateUpdate() const;
    bool needsClearStateUpdate() const;
    
    // OpenGL state application
    void applyDepthTestGL(bool enabled);
    void applyDepthWriteGL(bool enabled);
    void applyBlendingGL(bool enabled, BlendMode mode);
    void applyCullingGL(bool enabled, CullMode mode);
    void applyPolygonModeGL(bool wireframe);
    void applyLineWidthGL(float width);
    void applyPointSizeGL(float size);
    void applyShaderGL(ShaderId id);
    void applyTextureGL(TextureId textureId, int slot);
    void applyVertexArrayGL(uint32_t vaoId);
    void applyVertexBufferGL(BufferId bufferId);
    void applyIndexBufferGL(BufferId bufferId);
    void applyViewportGL(int x, int y, int width, int height);
    void applyClearColorGL(const Color& color);
};

// RAII helper for temporary state changes
class ScopedRenderState {
public:
    explicit ScopedRenderState(RenderState& state) : m_state(state) {
        // Save current state
        m_savedDepthTest = state.isDepthTestEnabled();
        m_savedDepthWrite = state.isDepthWriteEnabled();
        m_savedBlending = state.isBlendingEnabled();
        m_savedBlendMode = state.getBlendMode();
        m_savedCulling = state.isCullingEnabled();
        m_savedCullMode = state.getCullMode();
        m_savedShader = state.getBoundShader();
    }
    
    ~ScopedRenderState() {
        // Restore saved state
        m_state.setDepthTest(m_savedDepthTest);
        m_state.setDepthWrite(m_savedDepthWrite);
        m_state.setBlending(m_savedBlending, m_savedBlendMode);
        m_state.setCulling(m_savedCulling, m_savedCullMode);
        m_state.bindShader(m_savedShader);
        m_state.flush();
    }
    
private:
    RenderState& m_state;
    bool m_savedDepthTest;
    bool m_savedDepthWrite;
    bool m_savedBlending;
    BlendMode m_savedBlendMode;
    bool m_savedCulling;
    CullMode m_savedCullMode;
    ShaderId m_savedShader;
};

} // namespace Rendering
} // namespace VoxelEditor