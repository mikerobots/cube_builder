#pragma once

#include "RenderTypes.h"
#include "RenderConfig.h"
#include "RenderStats.h"
#include "OpenGLRenderer.h"  // For UniformValue
#include "../../foundation/events/EventDispatcher.h"
#include "../camera/Camera.h"
#include <memory>
#include <string>

namespace VoxelEditor {

// Forward declarations
namespace VoxelData {
    class VoxelGrid;
    enum class VoxelResolution : uint8_t;
}

namespace Rendering {

// Forward declarations
class OpenGLRenderer;
class ShaderManager;
class RenderState;
class FrameBuffer;
class GroundPlaneGrid;

class RenderEngine {
public:
    explicit RenderEngine(Events::EventDispatcher* eventDispatcher = nullptr);
    ~RenderEngine();
    
    // Disable copy/move for singleton-like behavior
    RenderEngine(const RenderEngine&) = delete;
    RenderEngine& operator=(const RenderEngine&) = delete;
    RenderEngine(RenderEngine&&) = delete;
    RenderEngine& operator=(RenderEngine&&) = delete;
    
    // Initialization and lifecycle
    bool initialize(const RenderConfig& config);
    void shutdown();
    bool isInitialized() const { return m_initialized; }
    
    // Configuration
    const RenderConfig& getConfig() const { return m_config; }
    void updateConfig(const RenderConfig& config);
    
    // Frame rendering
    void beginFrame();
    void endFrame();
    void present();
    void clear(ClearFlags flags = ClearFlags::All, 
               const Color& color = Color::Black(), 
               float depth = 1.0f, 
               int stencil = 0);
    
    // Basic rendering
    void renderMesh(const Mesh& mesh, const Transform& transform, const Material& material);
    void renderMeshAsLines(const Mesh& mesh, const Transform& transform, const Material& material);
    void renderMeshInstanced(const Mesh& mesh, const std::vector<Transform>& transforms, const Material& material);
    
    // Voxel rendering
    void renderVoxels(const VoxelData::VoxelGrid& grid, 
                     VoxelData::VoxelResolution resolution, 
                     const RenderSettings& settings);
    
    // Render modes and settings
    void setRenderMode(RenderMode mode);
    RenderMode getRenderMode() const { return m_currentSettings.renderMode; }
    void setRenderSettings(const RenderSettings& settings);
    const RenderSettings& getRenderSettings() const { return m_currentSettings; }
    
    // Viewport and camera
    void setViewport(int x, int y, int width, int height);
    void setCamera(const Camera::Camera& camera);
    
    // Render targets and framebuffers (TODO: Implement)
    // void setRenderTarget(FrameBuffer* target);
    // void setDefaultRenderTarget();
    
    // Resource management
    BufferId createVertexBuffer(const void* data, size_t size, BufferUsage usage = BufferUsage::Static);
    BufferId createIndexBuffer(const uint32_t* indices, size_t count, BufferUsage usage = BufferUsage::Static);
    VertexArrayId createVertexArray(const VertexLayout& layout);
    TextureId createTexture(int width, int height, TextureFormat format, const void* data = nullptr);
    void updateBuffer(BufferId bufferId, const void* data, size_t size, size_t offset = 0);
    void updateVertexBuffer(VertexBufferId bufferId, const void* data, size_t size, size_t offset = 0);
    void updateIndexBuffer(IndexBufferId bufferId, const void* data, size_t size, size_t offset = 0);
    void updateTexture(TextureId textureId, int x, int y, int width, int height, const void* data);
    void deleteBuffer(BufferId bufferId);
    void deleteVertexBuffer(VertexBufferId bufferId);
    void deleteIndexBuffer(IndexBufferId bufferId);
    void deleteVertexArray(VertexArrayId vaoId);
    void deleteTexture(TextureId textureId);
    
    // Binding operations
    void bindVertexBuffer(VertexArrayId vaoId, VertexBufferId vboId, int bindingIndex);
    void bindIndexBuffer(VertexArrayId vaoId, IndexBufferId iboId);
    void setVertexArray(VertexArrayId vaoId);
    
    // Drawing operations
    void drawIndexed(PrimitiveType primitive, size_t indexCount, IndexType indexType, size_t offset = 0);
    void drawArrays(PrimitiveType primitive, size_t vertexCount, size_t offset = 0);
    void drawElements(PrimitiveType primitive, int count, IndexType indexType);
    
    // Direct OpenGL access (for advanced rendering)
    void bindVertexArray(VertexArrayId vao);
    void useProgram(ShaderId program);
    void setUniform(const std::string& name, const UniformValue& value);
    
    // State management
    void setBlendMode(BlendMode mode);
    void setDepthWrite(bool enabled);
    void setCullMode(CullMode mode);
    void setDepthTest(bool enabled);
    void setLineWidth(float width);
    
    // Shader management
    ShaderId getBuiltinShader(const std::string& name);
    ShaderId loadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    void reloadShaders();
    ShaderManager* getShaderManager() { return m_shaderManager.get(); }
    
    // Debug and profiling
    void setDebugMode(bool enabled);
    bool isDebugMode() const { return m_debugMode; }
    const RenderStats& getRenderStats() const { return m_stats; }
    void captureFrame(const std::string& filename);
    
    // Ground plane grid
    void setGroundPlaneGridVisible(bool visible);
    bool isGroundPlaneGridVisible() const;
    void updateGroundPlaneGrid(const Math::Vector3f& workspaceSize);
    void updateGroundPlaneGridAnimation(float deltaTime);
    void renderGroundPlaneGrid(const Math::Vector3f& cursorWorldPos);
    
    // Utility functions
    void setupMeshBuffers(Mesh& mesh);
    void uploadMeshData(const Mesh& mesh);
    void cleanupMeshBuffers(Mesh& mesh);
    
private:
    // Helper methods
    std::string findShaderDirectory() const;
    
    // Core components
    std::unique_ptr<OpenGLRenderer> m_glRenderer;
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<RenderState> m_renderState;
    std::unique_ptr<GroundPlaneGrid> m_groundPlaneGrid;
    // std::unique_ptr<FrameBuffer> m_defaultFrameBuffer; // TODO: Implement
    // FrameBuffer* m_currentRenderTarget; // TODO: Implement
    
    // Configuration and state
    RenderConfig m_config;
    RenderSettings m_currentSettings;
    RenderStats m_stats;
    mutable RenderTimer m_frameTimer;
    bool m_initialized;
    bool m_debugMode;
    
    // Camera state
    const Camera::Camera* m_currentCamera;
    
    // Event system
    Events::EventDispatcher* m_eventDispatcher;
    
    // Internal rendering methods
    void renderMeshInternal(const Mesh& mesh, const Transform& transform, const Material& material);
    void setupRenderState(const Material& material);
    void bindMaterial(const Material& material);
    void updatePerFrameUniforms();
    void updateStats();
    
    // Voxel rendering helpers
    Mesh generateVoxelMesh(const VoxelData::VoxelGrid& grid, 
                          VoxelData::VoxelResolution resolution,
                          const RenderSettings& settings);
    Material createVoxelMaterial(VoxelData::VoxelResolution resolution, const RenderSettings& settings);
    
    // Built-in shader loading
    void loadBuiltinShaders();
    
    // Event handlers
    void onRenderModeChanged();
    void onViewportChanged();
    
    // Debug rendering
    void renderDebugInfo();
    void renderWireframeOverlay(const Mesh& mesh, const Transform& transform);
    void renderNormals(const Mesh& mesh, const Transform& transform);
    void renderBounds(const Transform& transform);
};

}
}