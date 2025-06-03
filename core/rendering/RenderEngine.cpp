#include "RenderEngine.h"
#include "OpenGLRenderer.h"
#include "ShaderManager.h"
#include "RenderState.h"
#include "../voxel_data/VoxelGrid.h"
#include "../../foundation/logging/Logger.h"
#include "../../foundation/math/Matrix4f.h"
#include <sstream>
#include <algorithm>
#include <cmath>

// Include OpenGL headers for glFlush
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <glad/glad.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace VoxelEditor {
namespace Rendering {

RenderEngine::RenderEngine(Events::EventDispatcher* eventDispatcher) 
    : m_initialized(false)
    , m_debugMode(false)
    , m_currentCamera(nullptr)
    , m_eventDispatcher(eventDispatcher) {
    
    // Initialize render settings with defaults
    m_currentSettings.renderMode = RenderMode::Solid;
    m_currentSettings.enableLighting = true;
    m_currentSettings.enableShadows = false;
    m_currentSettings.ambientLight = Color(0.2f, 0.2f, 0.2f, 1.0f);
    m_currentSettings.lightDirection = Math::Vector3f(0.3f, -1.0f, 0.5f).normalized();
    m_currentSettings.lightColor = Color::White();
}

RenderEngine::~RenderEngine() {
    if (m_initialized) {
        shutdown();
    }
}

bool RenderEngine::initialize(const RenderConfig& config) {
    if (m_initialized) {
        return false;
    }
    
    m_config = config;
    
    // Create OpenGL renderer
    m_glRenderer = std::make_unique<OpenGLRenderer>();
    if (!m_glRenderer->initializeContext(config)) {
        Logging::Logger::getInstance().error("Failed to initialize OpenGL renderer");
        return false;
    }
    
    // Create shader manager
    m_shaderManager = std::make_unique<ShaderManager>();
    
    // Create render state manager
    m_renderState = std::make_unique<RenderState>();
    
    // Load built-in shaders
    loadBuiltinShaders();
    
    // Initialize stats
    m_stats.reset();
    
    m_initialized = true;
    
    Logging::Logger::getInstance().info("RenderEngine initialized successfully");
    Logging::Logger::getInstance().info("Renderer: " + m_glRenderer->getRendererInfo());
    
    return true;
}

void RenderEngine::shutdown() {
    if (!m_initialized) return;
    
    // Clean up in reverse order
    m_renderState.reset();
    m_shaderManager.reset();
    m_glRenderer.reset();
    
    m_initialized = false;
    
    Logging::Logger::getInstance().info("RenderEngine shut down");
}

void RenderEngine::updateConfig(const RenderConfig& config) {
    m_config = config;
    
    // Update renderer settings
    if (m_glRenderer) {
        // Apply new configuration
        setViewport(0, 0, config.windowWidth, config.windowHeight);
    }
}

// Frame management
void RenderEngine::beginFrame() {
    if (!m_initialized) return;
    
    m_frameTimer.start();
    m_stats.frameCount++;
    
    // Clear frame stats
    m_stats.drawCalls = 0;
    m_stats.trianglesRendered = 0;
    m_stats.verticesProcessed = 0;
    
    updatePerFrameUniforms();
}

void RenderEngine::endFrame() {
    if (!m_initialized) return;
    
    // Timer is read in the next line
    
    // Update timing stats
    m_stats.frameTime = m_frameTimer.getElapsedMs();
    m_stats.fps = 1000.0f / m_stats.frameTime;
    
    if (m_debugMode) {
        renderDebugInfo();
    }
}

void RenderEngine::present() {
    // In a windowed context, this would swap buffers
    // For now, just flush GL commands
    if (m_glRenderer) {
        glFlush();
    }
}

void RenderEngine::clear(ClearFlags flags, const Color& color, float depth, int stencil) {
    if (!m_glRenderer) return;
    
    m_glRenderer->clear(flags, color, depth, stencil);
}

// Basic rendering
void RenderEngine::renderMesh(const Mesh& mesh, const Transform& transform, const Material& material) {
    if (!m_initialized || mesh.isEmpty()) return;
    
    // Ensure mesh has GPU buffers
    if (const_cast<Mesh&>(mesh).vertexBuffer == InvalidId) {
        const_cast<Mesh&>(mesh).vertexBuffer = m_glRenderer->createVertexBuffer(
            mesh.vertices.data(), 
            mesh.vertices.size() * sizeof(Vertex), 
            BufferUsage::Static);
    }
    
    if (const_cast<Mesh&>(mesh).indexBuffer == InvalidId) {
        const_cast<Mesh&>(mesh).indexBuffer = m_glRenderer->createIndexBuffer(
            mesh.indices.data(), 
            mesh.indices.size(), 
            BufferUsage::Static);
    }
    
    renderMeshInternal(mesh, transform, material);
}

void RenderEngine::renderMeshInternal(const Mesh& mesh, const Transform& transform, const Material& material) {
    // Set up render state
    setupRenderState(material);
    
    // Bind material
    bindMaterial(material);
    
    // Set transform uniforms
    if (m_currentCamera) {
        // Build model matrix from transform
        Math::Matrix4f modelMatrix = Math::Matrix4f::translation(transform.position);
        
        // Apply rotations (Euler angles)
        if (transform.rotation.x != 0.0f) {
            modelMatrix = modelMatrix * Math::Matrix4f::rotationX(transform.rotation.x * M_PI / 180.0f);
        }
        if (transform.rotation.y != 0.0f) {
            modelMatrix = modelMatrix * Math::Matrix4f::rotationY(transform.rotation.y * M_PI / 180.0f);
        }
        if (transform.rotation.z != 0.0f) {
            modelMatrix = modelMatrix * Math::Matrix4f::rotationZ(transform.rotation.z * M_PI / 180.0f);
        }
        
        modelMatrix = modelMatrix * Math::Matrix4f::scale(transform.scale);
        
        m_glRenderer->setUniform("u_model", UniformValue(modelMatrix));
        m_glRenderer->setUniform("u_view", UniformValue(m_currentCamera->getViewMatrix()));
        m_glRenderer->setUniform("u_projection", UniformValue(m_currentCamera->getProjectionMatrix()));
        
        // Normal matrix (inverse transpose of model-view)
        Math::Matrix4f normalMatrix = (m_currentCamera->getViewMatrix() * modelMatrix).inverse().transposed();
        m_glRenderer->setUniform("u_normalMatrix", UniformValue(normalMatrix));
    }
    
    // Bind buffers and draw
    m_glRenderer->bindVertexBuffer(mesh.vertexBuffer);
    m_glRenderer->bindIndexBuffer(mesh.indexBuffer);
    
    // Set up vertex attributes
    std::vector<VertexAttribute> attributes = {
        VertexAttribute::Position,
        VertexAttribute::Normal,
        VertexAttribute::TexCoord0,
        VertexAttribute::Color
    };
    m_glRenderer->setupVertexAttributes(attributes);
    
    // Draw the mesh
    m_glRenderer->drawElements(PrimitiveType::Triangles, 
                               static_cast<int>(mesh.indices.size()),
                               IndexType::UInt32);
    
    // Update stats
    m_stats.drawCalls++;
    m_stats.trianglesRendered += mesh.getTriangleCount();
    m_stats.verticesProcessed += mesh.getVertexCount();
    
    // Debug rendering
    if (m_debugMode) {
        if (m_currentSettings.renderMode == RenderMode::Combined) {
            renderWireframeOverlay(mesh, transform);
        }
        if (m_currentSettings.showNormals) {
            renderNormals(mesh, transform);
        }
        if (m_currentSettings.showBounds) {
            renderBounds(transform);
        }
    }
}

// Viewport and camera
void RenderEngine::setViewport(int x, int y, int width, int height) {
    if (!m_glRenderer) return;
    
    m_glRenderer->setViewport(x, y, width, height);
    
    // Update aspect ratio in camera if needed
    onViewportChanged();
}

void RenderEngine::setCamera(const Camera::Camera& camera) {
    m_currentCamera = &camera;
}

// Resource management
BufferId RenderEngine::createVertexBuffer(const void* data, size_t size, BufferUsage usage) {
    if (!m_glRenderer) return InvalidId;
    return m_glRenderer->createVertexBuffer(data, size, usage);
}

BufferId RenderEngine::createIndexBuffer(const uint32_t* indices, size_t count, BufferUsage usage) {
    if (!m_glRenderer) return InvalidId;
    return m_glRenderer->createIndexBuffer(indices, count, usage);
}

TextureId RenderEngine::createTexture(int width, int height, TextureFormat format, const void* data) {
    if (!m_glRenderer) return InvalidId;
    return m_glRenderer->createTexture2D(width, height, format, data);
}

void RenderEngine::updateBuffer(BufferId bufferId, const void* data, size_t size, size_t offset) {
    if (!m_glRenderer) return;
    m_glRenderer->updateBuffer(bufferId, data, size, offset);
}

void RenderEngine::updateTexture(TextureId textureId, int x, int y, int width, int height, const void* data) {
    if (!m_glRenderer) return;
    m_glRenderer->updateTexture(textureId, x, y, width, height, data);
}

void RenderEngine::deleteBuffer(BufferId bufferId) {
    if (!m_glRenderer) return;
    m_glRenderer->deleteBuffer(bufferId);
}

void RenderEngine::deleteTexture(TextureId textureId) {
    if (!m_glRenderer) return;
    m_glRenderer->deleteTexture(textureId);
}

// State management
void RenderEngine::setRenderMode(RenderMode mode) {
    m_currentSettings.renderMode = mode;
    onRenderModeChanged();
}

void RenderEngine::setRenderSettings(const RenderSettings& settings) {
    m_currentSettings = settings;
    onRenderModeChanged();
}

void RenderEngine::setBlendMode(BlendMode mode) {
    if (!m_glRenderer) return;
    m_glRenderer->setBlending(mode != BlendMode::Opaque, mode);
}

void RenderEngine::setDepthWrite(bool enabled) {
    if (!m_glRenderer) return;
    m_glRenderer->setDepthWrite(enabled);
}

void RenderEngine::setCullMode(CullMode mode) {
    if (!m_glRenderer) return;
    m_glRenderer->setCulling(mode != CullMode::None, mode);
}

void RenderEngine::setDepthTest(bool enabled) {
    if (!m_glRenderer) return;
    m_glRenderer->setDepthTest(enabled);
}

void RenderEngine::setLineWidth(float width) {
    if (!m_glRenderer) return;
    m_glRenderer->setLineWidth(width);
}

// Shader management
ShaderId RenderEngine::getBuiltinShader(const std::string& name) {
    if (!m_shaderManager) return InvalidId;
    return m_shaderManager->getShader(name);
}

ShaderId RenderEngine::loadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
    if (!m_shaderManager) return InvalidId;
    return m_shaderManager->loadShader(name, vertexPath, fragmentPath);
}

void RenderEngine::reloadShaders() {
    if (!m_shaderManager) return;
    m_shaderManager->reloadAllShaders();
}

// Mesh utilities
void RenderEngine::setupMeshBuffers(Mesh& mesh) {
    if (!m_glRenderer || mesh.isEmpty()) return;
    
    if (mesh.vertexBuffer == InvalidId) {
        mesh.vertexBuffer = m_glRenderer->createVertexBuffer(
            mesh.vertices.data(),
            mesh.vertices.size() * sizeof(Vertex),
            BufferUsage::Static
        );
    }
    
    if (mesh.indexBuffer == InvalidId) {
        mesh.indexBuffer = m_glRenderer->createIndexBuffer(
            mesh.indices.data(),
            mesh.indices.size(),
            BufferUsage::Static
        );
    }
    
    mesh.dirty = false;
}

void RenderEngine::uploadMeshData(const Mesh& mesh) {
    if (!m_glRenderer || mesh.isEmpty()) return;
    
    if (mesh.vertexBuffer != InvalidId) {
        m_glRenderer->updateBuffer(mesh.vertexBuffer, 
                                   mesh.vertices.data(), 
                                   mesh.vertices.size() * sizeof(Vertex));
    }
    
    if (mesh.indexBuffer != InvalidId) {
        m_glRenderer->updateBuffer(mesh.indexBuffer, 
                                   mesh.indices.data(), 
                                   mesh.indices.size() * sizeof(uint32_t));
    }
}

void RenderEngine::cleanupMeshBuffers(Mesh& mesh) {
    if (!m_glRenderer) return;
    
    if (mesh.vertexBuffer != InvalidId) {
        m_glRenderer->deleteBuffer(mesh.vertexBuffer);
        mesh.vertexBuffer = InvalidId;
    }
    
    if (mesh.indexBuffer != InvalidId) {
        m_glRenderer->deleteBuffer(mesh.indexBuffer);
        mesh.indexBuffer = InvalidId;
    }
}

// Private helper methods
void RenderEngine::setupRenderState(const Material& material) {
    if (!m_glRenderer) return;
    
    // Set up blending
    m_glRenderer->setBlending(material.blendMode != BlendMode::Opaque, material.blendMode);
    
    // Set up culling
    m_glRenderer->setCulling(material.cullMode != CullMode::None && !material.doubleSided, material.cullMode);
    
    // Set up polygon mode based on render settings
    m_glRenderer->setPolygonMode(m_currentSettings.renderMode == RenderMode::Wireframe);
}

void RenderEngine::bindMaterial(const Material& material) {
    if (!m_glRenderer) return;
    
    // Use material shader or default
    ShaderId shader = material.shader != InvalidId ? material.shader : getBuiltinShader("basic");
    m_glRenderer->useProgram(shader);
    
    // Set material uniforms
    m_glRenderer->setUniform("u_albedo", UniformValue(material.albedo));
    m_glRenderer->setUniform("u_metallic", UniformValue(material.metallic));
    m_glRenderer->setUniform("u_roughness", UniformValue(material.roughness));
    m_glRenderer->setUniform("u_emission", UniformValue(material.emission));
    
    // Bind textures
    if (material.albedoTexture != InvalidId) {
        m_glRenderer->bindTexture(material.albedoTexture, 0);
        m_glRenderer->setUniform("u_albedoTexture", UniformValue(0));
        m_glRenderer->setUniform("u_hasAlbedoTexture", UniformValue(1));
    } else {
        m_glRenderer->setUniform("u_hasAlbedoTexture", UniformValue(0));
    }
}

void RenderEngine::updatePerFrameUniforms() {
    if (!m_glRenderer) return;
    
    // Update lighting uniforms
    m_glRenderer->setUniform("u_ambientLight", UniformValue(m_currentSettings.ambientLight));
    m_glRenderer->setUniform("u_lightDirection", UniformValue(m_currentSettings.lightDirection));
    m_glRenderer->setUniform("u_lightColor", UniformValue(m_currentSettings.lightColor));
    m_glRenderer->setUniform("u_enableLighting", UniformValue(m_currentSettings.enableLighting ? 1 : 0));
}

void RenderEngine::loadBuiltinShaders() {
    if (!m_shaderManager || !m_glRenderer) return;
    
    // Load basic shader
    const std::string basicVertex = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_normal;
        layout(location = 2) in vec2 a_texCoord;
        layout(location = 3) in vec4 a_color;
        
        uniform mat4 u_model;
        uniform mat4 u_view;
        uniform mat4 u_projection;
        uniform mat4 u_normalMatrix;
        
        out vec3 v_worldPos;
        out vec3 v_normal;
        out vec2 v_texCoord;
        out vec4 v_color;
        
        void main() {
            vec4 worldPos = u_model * vec4(a_position, 1.0);
            v_worldPos = worldPos.xyz;
            v_normal = mat3(u_normalMatrix) * a_normal;
            v_texCoord = a_texCoord;
            v_color = a_color;
            
            gl_Position = u_projection * u_view * worldPos;
        }
    )";
    
    const std::string basicFragment = R"(
        #version 330 core
        in vec3 v_worldPos;
        in vec3 v_normal;
        in vec2 v_texCoord;
        in vec4 v_color;
        
        uniform vec4 u_albedo;
        uniform float u_metallic;
        uniform float u_roughness;
        uniform float u_emission;
        
        uniform vec4 u_ambientLight;
        uniform vec3 u_lightDirection;
        uniform vec4 u_lightColor;
        uniform int u_enableLighting;
        
        uniform sampler2D u_albedoTexture;
        uniform int u_hasAlbedoTexture;
        
        out vec4 FragColor;
        
        void main() {
            vec4 albedo = u_albedo * v_color;
            if (u_hasAlbedoTexture > 0) {
                albedo *= texture(u_albedoTexture, v_texCoord);
            }
            
            if (u_enableLighting > 0) {
                vec3 normal = normalize(v_normal);
                float NdotL = max(dot(normal, -u_lightDirection), 0.0);
                
                vec3 diffuse = albedo.rgb * u_lightColor.rgb * NdotL;
                vec3 ambient = albedo.rgb * u_ambientLight.rgb;
                vec3 emission = albedo.rgb * u_emission;
                
                FragColor = vec4(ambient + diffuse + emission, albedo.a);
            } else {
                FragColor = albedo;
            }
        }
    )";
    
    m_shaderManager->createShaderFromSource("basic", basicVertex, basicFragment, m_glRenderer.get());
}

void RenderEngine::onRenderModeChanged() {
    if (!m_glRenderer) return;
    
    switch (m_currentSettings.renderMode) {
        case RenderMode::Solid:
            m_glRenderer->setPolygonMode(false);
            break;
        case RenderMode::Wireframe:
            m_glRenderer->setPolygonMode(true);
            break;
        case RenderMode::Combined:
            // Solid mode, but we'll render wireframe as overlay
            m_glRenderer->setPolygonMode(false);
            break;
        case RenderMode::Points:
            // Not fully implemented yet
            break;
    }
}

void RenderEngine::onViewportChanged() {
    // Update camera aspect ratio if needed
    if (m_eventDispatcher) {
        // Dispatch viewport changed event
    }
}

// Debug mode
void RenderEngine::setDebugMode(bool enabled) {
    m_debugMode = enabled;
}

// Debug rendering methods
void RenderEngine::renderDebugInfo() {
    // TODO: Implement debug info rendering (stats overlay, etc.)
}

void RenderEngine::renderWireframeOverlay(const Mesh& mesh, const Transform& transform) {
    if (!m_glRenderer) return;
    
    // Save current state
    bool oldDepthWrite = true; // Assume it was on
    
    // Set up wireframe rendering
    m_glRenderer->setPolygonMode(true);
    m_glRenderer->setDepthWrite(false);
    m_glRenderer->setBlending(true, BlendMode::Alpha);
    
    // Use wireframe material
    Material wireframeMat = Material::createWireframe(Color(0.0f, 1.0f, 0.0f, 0.3f));
    renderMeshInternal(mesh, transform, wireframeMat);
    
    // Restore state
    m_glRenderer->setPolygonMode(false);
    m_glRenderer->setDepthWrite(oldDepthWrite);
    m_glRenderer->setBlending(false);
}

void RenderEngine::renderNormals(const Mesh& mesh, const Transform& transform) {
    // TODO: Implement normal visualization
    (void)mesh;
    (void)transform;
}

void RenderEngine::renderBounds(const Transform& transform) {
    // TODO: Implement bounding box visualization
    (void)transform;
}

// Voxel rendering (stub for now)
void RenderEngine::renderVoxels(const VoxelData::VoxelGrid& grid, 
                               VoxelData::VoxelResolution resolution, 
                               const RenderSettings& settings) {
    // TODO: Implement voxel rendering
    (void)grid;
    (void)resolution;
    (void)settings;
}

} // namespace Rendering
} // namespace VoxelEditor