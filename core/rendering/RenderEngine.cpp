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
#include <fstream>
#include <cstring>

// Include OpenGL headers for glFlush
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include "MacOSGLLoader.h"
#else
#include <glad/glad.h>
#endif

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
    
    Logging::Logger::getInstance().debugfc("RenderEngine", "Begin frame %d", m_stats.frameCount);
    
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
    
    Logging::Logger::getInstance().debugfc("RenderEngine", 
        "End frame %d: %.2fms, %.1f FPS, %d draws, %d triangles", 
        m_stats.frameCount, m_stats.frameTime, m_stats.fps, 
        m_stats.drawCalls, m_stats.trianglesRendered);
    
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
    
    Logging::Logger::getInstance().debugfc("RenderEngine", 
        "Clear: flags=%d, color=(%.3f,%.3f,%.3f,%.3f), depth=%.3f, stencil=%d", 
        static_cast<int>(flags), color.r, color.g, color.b, color.a, depth, stencil);
    
    m_glRenderer->clear(flags, color, depth, stencil);
}

// Basic rendering
void RenderEngine::renderMesh(const Mesh& mesh, const Transform& transform, const Material& material) {
    if (!m_initialized || mesh.isEmpty()) return;
    
    // Ensure mesh has VAO and GPU buffers
    if (const_cast<Mesh&>(mesh).vertexArray == InvalidId || 
        const_cast<Mesh&>(mesh).vertexBuffer == InvalidId ||
        const_cast<Mesh&>(mesh).indexBuffer == InvalidId) {
        // Use setupMeshBuffers which properly configures the VAO
        setupMeshBuffers(const_cast<Mesh&>(mesh));
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
        
        m_glRenderer->setUniform("model", UniformValue(modelMatrix));
        m_glRenderer->setUniform("view", UniformValue(m_currentCamera->getViewMatrix()));
        m_glRenderer->setUniform("projection", UniformValue(m_currentCamera->getProjectionMatrix()));
        
        // Set lighting uniforms
        // Default light position above and to the side of the scene
        Math::Vector3f lightPos(5.0f, 10.0f, 5.0f);
        Math::Vector3f lightColor(1.0f, 1.0f, 1.0f);  // White light
        Math::Vector3f viewPos = m_currentCamera->getPosition();
        
        m_glRenderer->setUniform("lightPos", UniformValue(lightPos));
        m_glRenderer->setUniform("lightColor", UniformValue(lightColor));
        m_glRenderer->setUniform("viewPos", UniformValue(viewPos));
        
        // Debug: Print matrices and test transform
        static int transformCount = 0;
        if (transformCount < 3) {
            const auto& view = m_currentCamera->getViewMatrix();
            const auto& proj = m_currentCamera->getProjectionMatrix();
            
            if (transformCount == 0) {
                std::cout << "Camera target: " << m_currentCamera->getTarget().x << ", " 
                          << m_currentCamera->getTarget().y << ", " 
                          << m_currentCamera->getTarget().z << std::endl;
                std::cout << "Camera position: " << m_currentCamera->getPosition().x << ", " 
                          << m_currentCamera->getPosition().y << ", " 
                          << m_currentCamera->getPosition().z << std::endl;
                          
                // Print first row of view matrix
                std::cout << "View matrix first row: " << view.m[0] << ", " << view.m[1] 
                          << ", " << view.m[2] << ", " << view.m[3] << std::endl;
                          
                // Check workspace bounds
                Math::Vector3f wsSize = Math::Vector3f(10, 10, 10);
                std::cout << "Workspace: 0,0,0 to " << wsSize.x << "," << wsSize.y << "," << wsSize.z << std::endl;
            }
            
            // Test transform vertices from the mesh
            if (mesh.vertices.size() >= 3) {
                float minX = 9999, maxX = -9999, minY = 9999, maxY = -9999;
                
                for (int i = 0; i < 3; i++) {
                    Math::Vector4f testPos(mesh.vertices[i].position.x, 
                                          mesh.vertices[i].position.y,
                                          mesh.vertices[i].position.z, 1.0f);
                    
                    Math::Vector4f worldPos = modelMatrix * testPos;
                    Math::Vector4f viewPos = view * worldPos;
                    Math::Vector4f clipPos = proj * viewPos;
                    
                    if (i == 0) {
                        std::cout << "Vertex 0 transform: world(" << worldPos.x << "," << worldPos.y << "," << worldPos.z 
                                  << ") -> clip(" << clipPos.x << "," << clipPos.y << "," << clipPos.z << "," << clipPos.w << ")";
                    }
                    
                    if (std::abs(clipPos.w) > 0.001f) {
                        float ndcX = clipPos.x / clipPos.w;
                        float ndcY = clipPos.y / clipPos.w;
                        float ndcZ = clipPos.z / clipPos.w;
                        
                        // Convert to screen coordinates
                        float screenX = (ndcX + 1.0f) * 0.5f * 1280;
                        float screenY = (1.0f - ndcY) * 0.5f * 720;
                        
                        minX = std::min(minX, screenX);
                        maxX = std::max(maxX, screenX);
                        minY = std::min(minY, screenY);
                        maxY = std::max(maxY, screenY);
                        
                        if (i == 0) {
                            std::cout << " -> NDC(" << ndcX << "," << ndcY << "," << ndcZ << ")";
                            std::cout << " -> Screen(" << screenX << "," << screenY << ")";
                            
                            if (ndcX >= -1 && ndcX <= 1 && ndcY >= -1 && ndcY <= 1 && ndcZ >= -1 && ndcZ <= 1) {
                                std::cout << " [VISIBLE]";
                            } else {
                                std::cout << " [OUTSIDE]";
                            }
                        }
                    }
                }
                
                std::cout << std::endl;
                std::cout << "  Triangle bounds: (" << minX << "," << minY << ") to (" << maxX << "," << maxY << ")";
                std::cout << " Size: " << (maxX - minX) << " x " << (maxY - minY) << " pixels" << std::endl;
            }
            
            transformCount++;
        }
        
        // Normal matrix (inverse transpose of model-view)
        Math::Matrix4f normalMatrix = (m_currentCamera->getViewMatrix() * modelMatrix).inverse().transposed();
        // Note: The shader doesn't use u_normalMatrix, it calculates the normal matrix inline
        // m_glRenderer->setUniform("u_normalMatrix", UniformValue(normalMatrix));
    }
    
    // Bind VAO - the VAO already has all the vertex attributes configured
    m_glRenderer->bindVertexArray(mesh.vertexArray);
    
    // Draw the mesh
    static int meshDrawCount = 0;
    if (meshDrawCount < 5) {
        std::cout << "Drawing mesh with " << mesh.indices.size() << " indices (" 
                  << mesh.indices.size() / 3 << " triangles)" << std::endl;
        meshDrawCount++;
    }
    
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

void RenderEngine::renderMeshAsLines(const Mesh& mesh, const Transform& transform, const Material& material) {
    if (!m_glRenderer || !m_shaderManager || mesh.isEmpty()) return;
    
    // Clear any pending GL errors before starting
    while (glGetError() != GL_NO_ERROR) {}
    
    // Set up mesh buffers if needed
    if (mesh.vertexArray == 0 || mesh.vertexBuffer == InvalidId) {
        setupMeshBuffers(const_cast<Mesh&>(mesh));
    }
    
    // Select shader
    ShaderId shaderId = material.shader;
    if (shaderId == InvalidId) {
        shaderId = getBuiltinShader("basic");
    }
    
    // Bind shader
    m_glRenderer->useProgram(shaderId);
    
    // Bind vertex array
    m_glRenderer->bindVertexArray(mesh.vertexArray);
    
    // Set up transform matrices
    Math::Matrix4f modelMatrix = Math::Matrix4f::Identity();
    modelMatrix = modelMatrix * Math::Matrix4f::translation(transform.position);
    // For simplicity, we'll skip rotation for lines - could be enhanced later
    // modelMatrix = modelMatrix * Math::Matrix4f::rotation(transform.rotation);
    modelMatrix = modelMatrix * Math::Matrix4f::scale(transform.scale);
    
    m_glRenderer->setUniform("model", UniformValue(modelMatrix));
    m_glRenderer->setUniform("view", UniformValue(m_currentCamera->getViewMatrix()));
    m_glRenderer->setUniform("projection", UniformValue(m_currentCamera->getProjectionMatrix()));
    
    // Set lighting uniforms for basic shader
    Math::Vector3f lightPos(5.0f, 10.0f, 5.0f);
    Math::Vector3f lightColor(1.0f, 1.0f, 1.0f);  // White light
    Math::Vector3f viewPos = m_currentCamera->getPosition();
    
    m_glRenderer->setUniform("lightPos", UniformValue(lightPos));
    m_glRenderer->setUniform("lightColor", UniformValue(lightColor));
    m_glRenderer->setUniform("viewPos", UniformValue(viewPos));
    
    // Clear any pending GL errors before drawing
    while (glGetError() != GL_NO_ERROR) {}
    
    // Draw as lines
    m_glRenderer->drawElements(PrimitiveType::Lines, 
                               static_cast<int>(mesh.indices.size()),
                               IndexType::UInt32);
    
    // Update stats
    m_stats.drawCalls++;
    m_stats.verticesProcessed += mesh.getVertexCount();
}

// Viewport and camera
void RenderEngine::setViewport(int x, int y, int width, int height) {
    if (!m_glRenderer) return;
    
    Logging::Logger::getInstance().debugfc("RenderEngine", 
        "Viewport: x=%d, y=%d, width=%d, height=%d (aspect=%.3f)", 
        x, y, width, height, width > 0 ? static_cast<float>(width) / height : 0.0f);
    
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
    
    // Create VAO for the mesh
    if (mesh.vertexArray == 0) {
        mesh.vertexArray = m_glRenderer->createVertexArray();
    }
    
    // Bind VAO before setting up buffers
    m_glRenderer->bindVertexArray(mesh.vertexArray);
    
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
    
    // Setup vertex attributes while VAO is bound
    m_glRenderer->bindVertexBuffer(mesh.vertexBuffer);
    m_glRenderer->bindIndexBuffer(mesh.indexBuffer);
    
    // Configure vertex attributes for the VAO
    std::vector<VertexAttribute> attributes = {
        VertexAttribute::Position,
        VertexAttribute::Normal,
        VertexAttribute::TexCoord0,
        VertexAttribute::Color
    };
    m_glRenderer->setupVertexAttributes(attributes);
    
    // Unbind VAO
    m_glRenderer->bindVertexArray(0);
    
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
    
    if (mesh.vertexArray != InvalidId) {
        m_glRenderer->deleteVertexArray(mesh.vertexArray);
        mesh.vertexArray = InvalidId;
    }
    
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
    
    // Set up culling - DISABLED for debugging
    // m_glRenderer->setCulling(material.cullMode != CullMode::None && !material.doubleSided, material.cullMode);
    m_glRenderer->setCulling(false, CullMode::Back);
    
    // Enable depth test
    m_glRenderer->setDepthTest(true);
    
    // Set up polygon mode based on render settings
    m_glRenderer->setPolygonMode(m_currentSettings.renderMode == RenderMode::Wireframe);
}

void RenderEngine::bindMaterial(const Material& material) {
    if (!m_glRenderer) return;
    
    // Use material shader or default
    ShaderId shader = material.shader != InvalidId ? material.shader : getBuiltinShader("basic");
    static int bindCount = 0;
    if (bindCount < 5) {
        std::cout << "bindMaterial: Using shader ID " << shader << " (material shader: " << material.shader << ")" << std::endl;
        bindCount++;
    }
    m_glRenderer->useProgram(shader);
    
    // Set material uniforms
    // NOTE: The current basic shader doesn't use these uniforms - it uses vertex colors
    // Uncomment when we implement a proper PBR shader
    // m_glRenderer->setUniform("u_albedo", UniformValue(material.albedo));
    // m_glRenderer->setUniform("u_metallic", UniformValue(material.metallic));
    // m_glRenderer->setUniform("u_roughness", UniformValue(material.roughness));
    // m_glRenderer->setUniform("u_emission", UniformValue(material.emission));
    
    // No texture binding for now - shader doesn't use textures
}

void RenderEngine::updatePerFrameUniforms() {
    if (!m_glRenderer) return;
    
    // Update lighting uniforms - but we need to set these for each shader program
    // NOTE: The current basic shader doesn't use lighting uniforms
    // m_glRenderer->setUniform("u_enableLighting", UniformValue(0));
}

void RenderEngine::loadBuiltinShaders() {
    if (!m_shaderManager || !m_glRenderer) return;
    
    // Common vertex shader for all voxel shaders
    const std::string voxelVertex = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;

void main() {
    FragPos = vec3(model * vec4(a_position, 1.0));
    Normal = mat3(transpose(inverse(model))) * a_normal;
    Color = a_color;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
    )";
    
    // Basic shader (original lighting)
    const std::string basicFragment = R"(
#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine results
    vec3 result = (ambient + diffuse + specular) * Color.rgb;
    FragColor = vec4(result, 1.0);
}
    )";
    
    // Enhanced shader with better face distinction
    const std::string enhancedFragment = R"(
#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    // Enhanced lighting with better face distinction
    vec3 norm = normalize(Normal);
    
    // Multiple light sources for better face visibility
    // Primary light from above-front-right
    vec3 lightDir1 = normalize(vec3(1.0, 2.0, 1.0));
    float diff1 = max(dot(norm, lightDir1), 0.0);
    
    // Secondary light from left
    vec3 lightDir2 = normalize(vec3(-1.0, 0.5, 0.5));
    float diff2 = max(dot(norm, lightDir2), 0.0);
    
    // Rim light from behind for edge definition
    vec3 viewDir = normalize(viewPos - FragPos);
    float rimLight = 1.0 - max(dot(norm, viewDir), 0.0);
    rimLight = pow(rimLight, 2.0) * 0.3;
    
    // Face-dependent base lighting to ensure each face is distinct
    float faceBrightness = 0.0;
    
    // Top face (Y+) - brightest
    if (abs(norm.y - 1.0) < 0.01) {
        faceBrightness = 1.0;
    }
    // Bottom face (Y-) - darkest
    else if (abs(norm.y + 1.0) < 0.01) {
        faceBrightness = 0.3;
    }
    // Right face (X+) - bright
    else if (abs(norm.x - 1.0) < 0.01) {
        faceBrightness = 0.85;
    }
    // Left face (X-) - medium dark
    else if (abs(norm.x + 1.0) < 0.01) {
        faceBrightness = 0.5;
    }
    // Front face (Z+) - medium bright
    else if (abs(norm.z - 1.0) < 0.01) {
        faceBrightness = 0.7;
    }
    // Back face (Z-) - medium
    else if (abs(norm.z + 1.0) < 0.01) {
        faceBrightness = 0.6;
    }
    
    // Combine lighting components
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
    
    // Primary and secondary diffuse
    vec3 diffuse = (diff1 * 0.7 + diff2 * 0.3) * lightColor;
    
    // Apply face-dependent brightness
    vec3 faceLight = faceBrightness * lightColor * 0.4;
    
    // Subtle specular for material definition
    vec3 reflectDir = reflect(-lightDir1, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 specular = 0.2 * spec * lightColor;
    
    // Combine all lighting
    vec3 result = (ambient + diffuse + faceLight + specular + rimLight) * Color.rgb;
    
    // Subtle edge darkening based on viewing angle
    // This creates a gentle darkening effect at grazing angles
    // viewDir already declared above, reuse it
    float fresnel = 1.0 - abs(dot(norm, viewDir));
    fresnel = pow(fresnel, 3.0); // Make the effect more subtle
    
    // Apply subtle edge darkening
    result *= (1.0 - fresnel * 0.3);
    
    // Add slight darkening based on face to enhance separation
    // This is a simple way to make edges more visible
    float faceDarkening = 0.0;
    
    // Apply different subtle darkening to each face edge
    if (abs(norm.y - 1.0) < 0.01) {
        // Top face - no extra darkening
        faceDarkening = 0.0;
    } else if (abs(norm.y + 1.0) < 0.01) {
        // Bottom face - slight darkening
        faceDarkening = 0.1;
    } else {
        // Side faces - very slight darkening
        faceDarkening = 0.05;
    }
    
    result *= (1.0 - faceDarkening);
    
    // Slight contrast boost
    result = pow(result, vec3(0.95));
    
    FragColor = vec4(result, 1.0);
}
    )";
    
    // Flat shader for maximum face distinction
    const std::string flatFragment = R"(
#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    // Simple flat shading with face-based coloring
    vec3 norm = normalize(Normal);
    
    // Base color with face-dependent tinting
    vec3 faceColor = Color.rgb;
    
    // Apply different brightness to each face based on normal
    float brightness = 0.5;
    
    // Top face - brightest
    if (abs(norm.y - 1.0) < 0.01) {
        brightness = 1.0;
    }
    // Bottom face - darkest
    else if (abs(norm.y + 1.0) < 0.01) {
        brightness = 0.25;
    }
    // Right face
    else if (abs(norm.x - 1.0) < 0.01) {
        brightness = 0.8;
    }
    // Left face
    else if (abs(norm.x + 1.0) < 0.01) {
        brightness = 0.4;
    }
    // Front face
    else if (abs(norm.z - 1.0) < 0.01) {
        brightness = 0.65;
    }
    // Back face
    else if (abs(norm.z + 1.0) < 0.01) {
        brightness = 0.5;
    }
    
    // Apply brightness
    vec3 result = faceColor * brightness;
    
    FragColor = vec4(result, 1.0);
}
    )";
    
    // Create all shader variants
    auto basicShaderId = m_shaderManager->createShaderFromSource("basic", voxelVertex, basicFragment, m_glRenderer.get());
    std::cout << "Created basic shader with ID: " << basicShaderId << std::endl;
    
    auto enhancedShaderId = m_shaderManager->createShaderFromSource("enhanced", voxelVertex, enhancedFragment, m_glRenderer.get());
    std::cout << "Created enhanced shader with ID: " << enhancedShaderId << std::endl;
    
    auto flatShaderId = m_shaderManager->createShaderFromSource("flat", voxelVertex, flatFragment, m_glRenderer.get());
    std::cout << "Created flat shader with ID: " << flatShaderId << std::endl;
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

// Direct OpenGL access methods
void RenderEngine::drawElements(PrimitiveType primitive, int count, IndexType indexType) {
    if (!m_glRenderer) return;
    m_glRenderer->drawElements(primitive, count, indexType);
    
    // Update stats
    m_stats.drawCalls++;
    if (primitive == PrimitiveType::Triangles) {
        m_stats.trianglesRendered += count / 3;
    }
}

void RenderEngine::bindVertexArray(VertexArrayId vao) {
    if (!m_glRenderer) return;
    m_glRenderer->bindVertexArray(vao);
}

void RenderEngine::useProgram(ShaderId program) {
    if (!m_glRenderer) return;
    m_glRenderer->useProgram(program);
}

void RenderEngine::setUniform(const std::string& name, const UniformValue& value) {
    if (!m_glRenderer) return;
    m_glRenderer->setUniform(name, value);
}

// Screenshot functionality
void RenderEngine::captureFrame(const std::string& filename) {
    if (!m_glRenderer || !m_initialized) {
        Logging::Logger::getInstance().error("Cannot capture frame: RenderEngine not initialized");
        return;
    }
    
    // Get current viewport dimensions
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int width = viewport[2];
    int height = viewport[3];
    
    // Allocate buffer for pixels (RGB format)
    std::vector<unsigned char> pixels(width * height * 3);
    
    // Ensure all OpenGL commands are finished
    glFlush();
    glFinish();
    
    // Read pixels from framebuffer
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    
    // OpenGL gives us the image upside down, so we need to flip it
    std::vector<unsigned char> flipped(width * height * 3);
    for (int y = 0; y < height; ++y) {
        std::memcpy(&flipped[y * width * 3], 
                   &pixels[(height - 1 - y) * width * 3], 
                   width * 3);
    }
    
    // Determine output format and save
    std::string actualFilename = filename;
    
    // If filename ends with .png, replace with .ppm for now
    size_t dotPos = actualFilename.find_last_of('.');
    if (dotPos != std::string::npos && actualFilename.substr(dotPos) == ".png") {
        actualFilename = actualFilename.substr(0, dotPos) + ".ppm";
    } else if (dotPos == std::string::npos) {
        actualFilename += ".ppm";
    }
    
    // Write PPM file
    std::ofstream file(actualFilename, std::ios::binary);
    if (!file) {
        Logging::Logger::getInstance().error("Failed to open file for screenshot: " + actualFilename);
        return;
    }
    
    // PPM header
    file << "P6\n" << width << " " << height << "\n255\n";
    
    // Write pixel data
    file.write(reinterpret_cast<const char*>(flipped.data()), flipped.size());
    file.close();
    
    // Log that we saved as PPM instead of PNG
    if (filename.find(".png") != std::string::npos) {
        Logging::Logger::getInstance().info("Screenshot saved as " + actualFilename + " (PPM format)");
    } else {
        Logging::Logger::getInstance().info("Screenshot saved: " + actualFilename);
    }
}

} // namespace Rendering
} // namespace VoxelEditor