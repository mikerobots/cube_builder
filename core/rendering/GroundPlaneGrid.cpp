#include "GroundPlaneGrid.h"
#include "OpenGLRenderer.h"
#include "ShaderManager.h"
#include "../../foundation/logging/Logger.h"
#include <vector>
#include <cmath>
#include <fstream>

// Include OpenGL headers
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

namespace VoxelEditor {
namespace Rendering {

using namespace Math;
using namespace Logging;

GroundPlaneGrid::GroundPlaneGrid(ShaderManager* shaderManager, OpenGLRenderer* glRenderer)
    : m_vao(InvalidId)
    , m_vbo(InvalidId)
    , m_shader(InvalidId)
    , m_lineCount(0)
    , m_shaderManager(shaderManager)
    , m_glRenderer(glRenderer)
    , m_initialized(false)
    , m_visible(true)
    , m_currentWorkspaceSize(0.0f, 0.0f, 0.0f)
    , m_baseOpacity(0.35f)
    , m_nearOpacity(0.65f)
    , m_transitionSpeed(5.0f)
    , m_cursorPosition(0.0f, 0.0f, 0.0f)
    , m_smoothedCursorPosition(WorldCoordinates::zero())
    , m_currentOpacity(0.35f)
    , m_targetOpacity(0.35f)
    , m_forceMaxOpacity(false) {
}

GroundPlaneGrid::~GroundPlaneGrid() {
    cleanup();
}

bool GroundPlaneGrid::initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Load grid shader
    if (!loadShader()) {
        Logging::Logger::getInstance().error("Failed to load ground plane grid shader");
        return false;
    }
    
    // Create VAO
    m_vao = m_glRenderer->createVertexArray();
    if (m_vao == 0) {  // createVertexArray returns uint32_t, not InvalidId
        Logging::Logger::getInstance().error("Failed to create VAO for ground plane grid");
        cleanup();
        return false;
    }
    
    m_initialized = true;
    Logging::Logger::getInstance().info("Ground plane grid renderer initialized");
    return true;
}

void GroundPlaneGrid::updateGridMesh(const Vector3f& workspaceSize) {
    if (workspaceSize == m_currentWorkspaceSize) {
        return; // No change needed
    }
    
    m_currentWorkspaceSize = workspaceSize;
    
    // Only generate mesh if we're initialized (have OpenGL context)
    if (m_initialized) {
        generateGridMesh(workspaceSize);
    }
}

void GroundPlaneGrid::update(float deltaTime) {
    // Skip opacity calculations if forced to max opacity
    if (m_forceMaxOpacity) {
        m_currentOpacity = 1.0f;
        m_targetOpacity = 1.0f;
        return;
    }
    
    // Smooth cursor position for less jittery transitions
    Vector3f cursorDelta = (m_cursorPosition.value() - m_smoothedCursorPosition.value()) * 
                          std::min(CursorSmoothingFactor * deltaTime, 1.0f);
    m_smoothedCursorPosition = WorldCoordinates(m_smoothedCursorPosition.value() + cursorDelta);
    
    // Calculate target opacity based on smoothed cursor distance to grid
    // We only care about XZ distance since grid is at Y=0
    float distanceToGrid = std::abs(m_smoothedCursorPosition.y());
    
    // Also consider XZ position - if cursor is over the grid area
    const float gridRadius = std::max(m_currentWorkspaceSize.x, m_currentWorkspaceSize.z) * 0.5f;
    float xzDistance = std::sqrt(m_smoothedCursorPosition.x() * m_smoothedCursorPosition.x() + 
                                 m_smoothedCursorPosition.z() * m_smoothedCursorPosition.z());
    
    // Calculate opacity based on proximity
    if (distanceToGrid < ProximityRadius * getGridCellSize() && xzDistance <= gridRadius) {
        // Close to grid plane and within grid bounds
        float proximityFactor = 1.0f - (distanceToGrid / (ProximityRadius * getGridCellSize()));
        m_targetOpacity = m_baseOpacity + (m_nearOpacity - m_baseOpacity) * proximityFactor;
    } else {
        m_targetOpacity = m_baseOpacity;
    }
    
    // Smooth opacity transition
    float opacityDelta = m_targetOpacity - m_currentOpacity;
    m_currentOpacity += opacityDelta * std::min(m_transitionSpeed * deltaTime, 1.0f);
}

void GroundPlaneGrid::setCursorPosition(const WorldCoordinates& cursorWorldPos) {
    m_cursorPosition = cursorWorldPos;
}

void GroundPlaneGrid::setOpacityParameters(float baseOpacity, float nearOpacity, float transitionSpeed) {
    m_baseOpacity = baseOpacity;
    m_nearOpacity = nearOpacity;
    m_transitionSpeed = transitionSpeed;
}

void GroundPlaneGrid::setForceMaxOpacity(bool forceMaxOpacity) {
    m_forceMaxOpacity = forceMaxOpacity;
    if (forceMaxOpacity) {
        m_currentOpacity = 1.0f;
        m_targetOpacity = 1.0f;
    }
}

void GroundPlaneGrid::render(const Matrix4f& viewMatrix, 
                             const Matrix4f& projMatrix) {
    if (!m_initialized || !m_visible || m_lineCount == 0 || m_shader == InvalidId) {
        return;
    }
    
    
    // Use shader
    m_glRenderer->useProgram(m_shader);
    
    // Set uniforms
    Matrix4f mvpMatrix = projMatrix * viewMatrix;
    
    // MVP matrix calculated for ground plane rendering
    
    m_glRenderer->setUniform("mvpMatrix", UniformValue(mvpMatrix));
    m_glRenderer->setUniform("minorLineColor", UniformValue(MinorLineColor()));
    m_glRenderer->setUniform("majorLineColor", UniformValue(MajorLineColor()));
    m_glRenderer->setUniform("opacity", UniformValue(m_currentOpacity));
    
    // Set line width for better visibility
    glLineWidth(DefaultLineWidth);
    
    // Enable blending for transparency
    m_glRenderer->setBlending(true, BlendMode::Alpha);
    
    // Disable depth write for transparent grid (but keep depth test)
    m_glRenderer->setDepthWrite(false);
    
    // Render grid lines
    m_glRenderer->bindVertexArray(m_vao);
    
    m_glRenderer->drawArrays(PrimitiveType::Lines, 0, m_lineCount * 2);
    
    // Check for OpenGL errors (only log first few frames to avoid spam)
    static int errorLogCount = 0;
    GLenum error = glGetError();
    if (error != GL_NO_ERROR && errorLogCount < 5) {
        const char* errorStr = nullptr;
        switch (error) {
            case GL_INVALID_ENUM: errorStr = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: errorStr = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: errorStr = "GL_INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_OUT_OF_MEMORY: errorStr = "GL_OUT_OF_MEMORY"; break;
            default: errorStr = "Unknown error"; break;
        }
        Logging::Logger::getInstance().error("GroundPlaneGrid GL error: " + std::string(errorStr));
        errorLogCount++;
    }
    
    // Restore state
    m_glRenderer->setDepthWrite(true);
    m_glRenderer->setBlending(false);
    m_glRenderer->useProgram(InvalidId);
}

void GroundPlaneGrid::generateGridMesh(const Vector3f& workspaceSize) {
    std::vector<GridVertex> vertices;
    
    const float cellSize = getGridCellSize();
    const float majorInterval = getMajorLineInterval();
    
    // Calculate grid bounds (centered at origin)
    float halfSizeX = workspaceSize.x * 0.5f;
    float halfSizeZ = workspaceSize.z * 0.5f;
    
    // Round to nearest cell boundary
    int cellsX = static_cast<int>(std::ceil(halfSizeX / cellSize));
    int cellsZ = static_cast<int>(std::ceil(halfSizeZ / cellSize));
    
    // Actual grid bounds
    float minX = -cellsX * cellSize;
    float maxX = cellsX * cellSize;
    float minZ = -cellsZ * cellSize;
    float maxZ = cellsZ * cellSize;
    
    // Generate lines parallel to X axis (varying Z)
    for (int i = -cellsZ; i <= cellsZ; ++i) {
        float z = i * cellSize;
        bool isMajor = (i % MajorLineInterval) == 0; // Major line every 5 cells (160cm)
        
        // Start and end points of line
        vertices.emplace_back(Vector3f(minX, 0.0f, z), isMajor);
        vertices.emplace_back(Vector3f(maxX, 0.0f, z), isMajor);
    }
    
    // Generate lines parallel to Z axis (varying X)
    for (int i = -cellsX; i <= cellsX; ++i) {
        float x = i * cellSize;
        bool isMajor = (i % MajorLineInterval) == 0; // Major line every 5 cells (160cm)
        
        // Start and end points of line
        vertices.emplace_back(Vector3f(x, 0.0f, minZ), isMajor);
        vertices.emplace_back(Vector3f(x, 0.0f, maxZ), isMajor);
    }
    
    m_lineCount = vertices.size() / 2;
    
    // Note: Vertex count logging for debugging purposes (can be removed in production)
    
    // Upload to GPU
    if (m_vbo != InvalidId) {
        m_glRenderer->deleteBuffer(m_vbo);
    }
    
    m_vbo = m_glRenderer->createVertexBuffer(vertices.data(), 
                                             vertices.size() * sizeof(GridVertex),
                                             BufferUsage::Static);
    
    // Setup vertex attributes manually using OpenGL
    m_glRenderer->bindVertexArray(m_vao);
    m_glRenderer->bindVertexBuffer(m_vbo);
    
    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertex), 
                         reinterpret_cast<const void*>(offsetof(GridVertex, position)));
    
    // IsMajorLine attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(GridVertex),
                         reinterpret_cast<const void*>(offsetof(GridVertex, isMajorLine)));
    
    // Unbind VAO to preserve state
    m_glRenderer->bindVertexArray(0);
    // Note: VBO unbinding is done after VAO unbinding to preserve the VAO state
    m_glRenderer->bindVertexBuffer(0);
    
    Logging::Logger::getInstance().info("Generated ground plane grid mesh: " + 
                                        std::to_string(m_lineCount) + " lines, workspace size: (" +
                                        std::to_string(workspaceSize.x) + ", " +
                                        std::to_string(workspaceSize.y) + ", " +
                                        std::to_string(workspaceSize.z) + ")");
    
    // Grid mesh generation completed successfully
}

bool GroundPlaneGrid::loadShader() {
    Logging::Logger::getInstance().info("GroundPlaneGrid: Loading ground plane grid shader from files...");
    
    // Try to load from shader files first
    // Check multiple possible paths for the shader files
    std::vector<std::string> possibleVertPaths = {
        "core/rendering/shaders/ground_plane.vert",           // From build directory
        "../core/rendering/shaders/ground_plane.vert",       // From one level up
        "../../core/rendering/shaders/ground_plane.vert",    // From test directories
        "bin/core/rendering/shaders/ground_plane.vert",       // Direct bin path
        "../../../bin/core/rendering/shaders/ground_plane.vert",  // From CTest working dir
        "../../../../core/rendering/shaders/ground_plane.vert"     // From CTest to source
    };
    
    std::string vertPath, fragPath;
    bool foundShader = false;
    
    for (const auto& testPath : possibleVertPaths) {
        std::ifstream testFile(testPath);
        if (testFile.is_open()) {
            testFile.close();
            vertPath = testPath;
            // Replace .vert with .frag for fragment shader
            fragPath = testPath;
            size_t pos = fragPath.find(".vert");
            if (pos != std::string::npos) {
                fragPath.replace(pos, 5, ".frag");
            }
            foundShader = true;
            break;
        }
    }
    
    if (!foundShader) {
        Logging::Logger::getInstance().warning("Could not find ground plane shader files in any expected location");
    } else {
        m_shader = m_shaderManager->loadShaderFromFile("ground_plane", vertPath, fragPath);
    }
    
    if (m_shader == InvalidId) {
        Logging::Logger::getInstance().warning("Failed to load ground plane shader from files, using inline version");
        
        // Fallback to inline shaders
        const char* vertexSource = R"(
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in float isMajorLine;

uniform mat4 mvpMatrix;

out float vIsMajorLine;

void main() {
    gl_Position = mvpMatrix * vec4(position, 1.0);
    vIsMajorLine = isMajorLine;
}
)";
        
        const char* fragmentSource = R"(
#version 330 core

in float vIsMajorLine;

uniform vec3 minorLineColor;
uniform vec3 majorLineColor;
uniform float opacity;

out vec4 fragColor;

const float MajorLineVisibilityMultiplier = 1.2;

void main() {
    // Select color based on line type
    vec3 lineColor = mix(minorLineColor, majorLineColor, vIsMajorLine);
    
    // Apply line width effect for major lines (simulated with opacity)
    float finalOpacity = opacity;
    if (vIsMajorLine > 0.5) {
        finalOpacity *= MajorLineVisibilityMultiplier; // Make major lines slightly more visible
        finalOpacity = min(finalOpacity, 1.0);
    }
    
    fragColor = vec4(lineColor, finalOpacity);
}
)";
        
        // Compile shader from source
        m_shader = m_shaderManager->createShaderFromSource("GroundPlaneGrid", 
                                                          vertexSource, 
                                                          fragmentSource,
                                                          m_glRenderer);
    }
    
    if (m_shader == InvalidId) {
        Logging::Logger::getInstance().error("Failed to compile ground plane grid shader");
        return false;
    }
    
    Logging::Logger::getInstance().info("GroundPlaneGrid: Shader created successfully with ID: " + std::to_string(m_shader));
    return true;
}

void GroundPlaneGrid::cleanup() {
    if (m_vbo != InvalidId) {
        m_glRenderer->deleteBuffer(m_vbo);
        m_vbo = InvalidId;
    }
    
    if (m_vao != 0) {
        m_glRenderer->deleteVertexArray(m_vao);
        m_vao = 0;
    }
    
    // Shader is managed by ShaderManager, no need to delete
    m_shader = InvalidId;
    
    m_initialized = false;
}

} // namespace Rendering
} // namespace VoxelEditor