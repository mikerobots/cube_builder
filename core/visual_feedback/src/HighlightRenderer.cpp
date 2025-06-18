#include "../include/visual_feedback/HighlightRenderer.h"
#include "../../rendering/OpenGLRenderer.h"
#include "../../rendering/RenderTypes.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <glad/glad.h>
#endif

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
    , m_maxHighlights(1000)
    , m_vao(0)
    , m_vbo(0)
    , m_ibo(0)
    , m_initialized(false) {
    
    // Delay OpenGL resource creation until first use
    // createHighlightMeshes();
}

HighlightRenderer::~HighlightRenderer() {
    // Clean up GPU resources
    if (m_vao != 0) {
#ifdef __APPLE__
        glDeleteVertexArrays(1, &m_vao);
#else
        glDeleteVertexArrays(1, &m_vao);
#endif
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_ibo != 0) {
        glDeleteBuffers(1, &m_ibo);
    }
    if (m_instanceBuffer != 0) {
        glDeleteBuffers(1, &m_instanceBuffer);
    }
    if (m_highlightShader != 0) {
        glDeleteProgram(m_highlightShader);
    }
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
    
    // Initialize OpenGL resources on first use
    if (!m_initialized) {
        createHighlightMeshes();
        if (!m_initialized) {
            std::cerr << "HighlightRenderer: Failed to initialize OpenGL resources" << std::endl;
            return;
        }
    }
    
    // Clear any existing GL errors
    while (glGetError() != GL_NO_ERROR) {}
    
    if (m_instancingEnabled && m_instanceBuffer != 0) {
        renderInstanced(camera);
    } else {
        renderImmediate(camera);
    }
    
    // Check for GL errors after rendering
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "HighlightRenderer GL Error after render: " << error << std::endl;
    }
}

void HighlightRenderer::createHighlightMeshes() {
    createBoxMesh();
    createFaceMesh();
    
    // Create shader program
    createHighlightShader();
    m_initialized = true;
}

void HighlightRenderer::createBoxMesh() {
    // Create unit cube mesh for voxel highlights
    float vertices[] = {
        // Position (3), Normal (3)
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        // Right face
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        // Left face
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f
    };
    
    uint32_t indices[] = {
        0,  1,  2,  0,  2,  3,   // front
        4,  5,  6,  4,  6,  7,   // back
        8,  9, 10,  8, 10, 11,   // top
        12, 13, 14, 12, 14, 15,  // bottom
        16, 17, 18, 16, 18, 19,  // right
        20, 21, 22, 20, 22, 23   // left
    };
    
    // Create VAO
#ifdef __APPLE__
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
#else
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
#endif
    
    // Create VBO
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Create IBO
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Set vertex attributes
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(1); // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    
    // Unbind
#ifdef __APPLE__
    glBindVertexArray(0);
#else
    glBindVertexArray(0);
#endif
    
    m_highlightMesh = m_vao;
    m_meshIndexCount = 36;
}

void HighlightRenderer::createFaceMesh() {
    // Create quad mesh for face highlights
    float vertices[] = {
        // Position (3), Normal (3)
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f
    };
    
    uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };
    
    // Create face mesh VAO/VBO/IBO if needed
    // For now, we'll reuse the box mesh for simplicity
    m_faceMesh = m_highlightMesh;
}

void HighlightRenderer::updateInstanceBuffer() {
    if (m_highlights.empty()) {
        return;
    }
    
    // TODO: Update GPU instance buffer with highlight data
}

void HighlightRenderer::renderInstanced(const Camera::Camera& camera) {
    if (!m_initialized || m_highlightShader == 0) return;
    
    updateInstanceBuffer();
    
    glUseProgram(m_highlightShader);
    
    // Set shader uniforms
    Math::Matrix4f viewMatrix = camera.getViewMatrix();
    Math::Matrix4f projMatrix = camera.getProjectionMatrix();
    
    GLint viewLoc = glGetUniformLocation(m_highlightShader, "u_view");
    GLint projLoc = glGetUniformLocation(m_highlightShader, "u_projection");
    GLint timeLoc = glGetUniformLocation(m_highlightShader, "u_time");
    
    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix.data());
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix.data());
    if (timeLoc != -1) glUniform1f(timeLoc, m_globalTime);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    
    // Bind VAO and render
#ifdef __APPLE__
    glBindVertexArray(m_highlightMesh);
#else
    glBindVertexArray(m_highlightMesh);
#endif
    
    // Render all instances
    glDrawElementsInstanced(GL_TRIANGLES, m_meshIndexCount, GL_UNSIGNED_INT, 0, 
                           static_cast<GLsizei>(m_highlights.size()));
    
    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    
#ifdef __APPLE__
    glBindVertexArray(0);
#else
    glBindVertexArray(0);
#endif
    
    // Restore shader program
    glUseProgram(0);
}

void HighlightRenderer::renderImmediate(const Camera::Camera& camera) {
    if (!m_initialized || m_highlightShader == 0) return;
    
    glUseProgram(m_highlightShader);
    
    // Set common uniforms
    Math::Matrix4f viewMatrix = camera.getViewMatrix();
    Math::Matrix4f projMatrix = camera.getProjectionMatrix();
    
    GLint viewLoc = glGetUniformLocation(m_highlightShader, "u_view");
    GLint projLoc = glGetUniformLocation(m_highlightShader, "u_projection");
    GLint modelLoc = glGetUniformLocation(m_highlightShader, "u_model");
    GLint colorLoc = glGetUniformLocation(m_highlightShader, "u_color");
    GLint timeLoc = glGetUniformLocation(m_highlightShader, "u_time");
    
    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix.data());
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix.data());
    if (timeLoc != -1) glUniform1f(timeLoc, m_globalTime);
    
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    
    // Bind VAO
#ifdef __APPLE__
    glBindVertexArray(m_highlightMesh);
#else
    glBindVertexArray(m_highlightMesh);
#endif
    
    // Render each highlight individually
    for (const auto& highlight : m_highlights) {
        // Set transform
        Math::Matrix4f modelMatrix = highlight.transform.toMatrix();
        if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix.data());
        
        // Set color with animation
        Rendering::Color animColor = calculateAnimatedColor(highlight.color, 
                                                          highlight.animationPhase, 
                                                          m_globalTime);
        if (colorLoc != -1) glUniform4f(colorLoc, animColor.r, animColor.g, animColor.b, animColor.a);
        
        // Draw mesh
        glDrawElements(GL_TRIANGLES, m_meshIndexCount, GL_UNSIGNED_INT, 0);
    }
    
    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    
#ifdef __APPLE__
    glBindVertexArray(0);
#else
    glBindVertexArray(0);
#endif
    
    // Restore shader program
    glUseProgram(0);
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
    
    // Position at voxel center using centered coordinate system
    // Convert grid coordinates to world coordinates (centered workspace)
    float workspaceSize = 5.0f; // Default workspace size
    float halfWorkspace = workspaceSize * 0.5f;
    
    transform.position = Math::WorldCoordinates(Math::Vector3f(
        position.x * voxelSize - halfWorkspace + voxelSize * 0.5f,
        position.y * voxelSize + voxelSize * 0.5f,
        position.z * voxelSize - halfWorkspace + voxelSize * 0.5f
    ));
    
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

void HighlightRenderer::createHighlightShader() {
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_normal;
        
        uniform mat4 u_model;
        uniform mat4 u_view;
        uniform mat4 u_projection;
        
        out vec3 v_normal;
        out vec3 v_worldPos;
        
        void main() {
            vec4 worldPos = u_model * vec4(a_position, 1.0);
            v_worldPos = worldPos.xyz;
            v_normal = mat3(u_model) * a_normal;
            
            gl_Position = u_projection * u_view * worldPos;
        }
    )";
    
    const char* fragmentShaderSrc = R"(
        #version 330 core
        in vec3 v_normal;
        in vec3 v_worldPos;
        
        uniform vec4 u_color;
        uniform float u_time;
        
        out vec4 FragColor;
        
        void main() {
            vec3 normal = normalize(v_normal);
            float fresnel = 1.0 - abs(dot(normal, vec3(0.0, 0.0, 1.0)));
            fresnel = pow(fresnel, 2.0);
            
            vec4 color = u_color;
            // Animate the color using time
            float pulse = sin(u_time * 2.0) * 0.5 + 0.5;
            color.a *= (0.3 + 0.7 * fresnel) * (0.7 + 0.3 * pulse);
            
            FragColor = color;
        }
    )";
    
    // Create shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);
    
    // Check vertex shader compilation
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            std::vector<char> log(logLength);
            glGetShaderInfoLog(vertexShader, logLength, nullptr, log.data());
            std::cerr << "HighlightRenderer vertex shader compilation error: " << log.data() << std::endl;
        }
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);
    
    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            std::vector<char> log(logLength);
            glGetShaderInfoLog(fragmentShader, logLength, nullptr, log.data());
            std::cerr << "HighlightRenderer fragment shader compilation error: " << log.data() << std::endl;
        }
    }
    
    // Create program
    m_highlightShader = glCreateProgram();
    glAttachShader(m_highlightShader, vertexShader);
    glAttachShader(m_highlightShader, fragmentShader);
    glLinkProgram(m_highlightShader);
    
    // Check program linking
    glGetProgramiv(m_highlightShader, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetProgramiv(m_highlightShader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            std::vector<char> log(logLength);
            glGetProgramInfoLog(m_highlightShader, logLength, nullptr, log.data());
            std::cerr << "HighlightRenderer shader linking error: " << log.data() << std::endl;
        }
    }
    
    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

} // namespace VisualFeedback
} // namespace VoxelEditor