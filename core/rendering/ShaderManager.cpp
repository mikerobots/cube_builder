#include "ShaderManager.h"
#include "../../foundation/logging/Logger.h"
#include "../../foundation/config/BuildConfig.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cassert>

namespace VoxelEditor {
namespace Rendering {

ShaderManager::ShaderManager() 
    : m_nextShaderId(1)
    , m_hotReloadEnabled(false)
    , m_renderer(nullptr) {
}

ShaderManager::ShaderManager(OpenGLRenderer* renderer)
    : m_nextShaderId(1)
    , m_hotReloadEnabled(false)
    , m_renderer(renderer) {
}

ShaderManager::~ShaderManager() {
    cleanup();
}

ShaderId ShaderManager::getShader(const std::string& name) {
    auto it = m_shadersByName.find(name);
    if (it != m_shadersByName.end()) {
        return it->second;
    }
    return InvalidId;
}

ShaderId ShaderManager::loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
    if (!m_renderer) {
        try {
            Logging::Logger::getInstance().error("ShaderManager::loadShaderFromFile - no renderer set");
        } catch (...) {}
        return InvalidId;
    }
    
    // Read vertex shader file
    std::ifstream vertexFile(vertexPath);
    if (!vertexFile.is_open()) {
        std::string errorMsg = "Failed to open vertex shader file: " + vertexPath;
        try {
            Logging::Logger::getInstance().error(errorMsg);
        } catch (...) {}
        VOXEL_ASSERT_SHADER_FILE(false, errorMsg);
        return InvalidId;
    }
    
    std::stringstream vertexStream;
    vertexStream << vertexFile.rdbuf();
    std::string vertexSource = vertexStream.str();
    vertexFile.close();
    
    // Read fragment shader file
    std::ifstream fragmentFile(fragmentPath);
    if (!fragmentFile.is_open()) {
        std::string errorMsg = "Failed to open fragment shader file: " + fragmentPath;
        try {
            Logging::Logger::getInstance().error(errorMsg);
        } catch (...) {}
        VOXEL_ASSERT_SHADER_FILE(false, errorMsg);
        return InvalidId;
    }
    
    std::stringstream fragmentStream;
    fragmentStream << fragmentFile.rdbuf();
    std::string fragmentSource = fragmentStream.str();
    fragmentFile.close();
    
    // Create shader from sources
    return createShaderFromSource(name, vertexSource, fragmentSource, m_renderer);
}

void ShaderManager::reloadAllShaders() {
    // TODO: Implement shader reloading
    try {
        Logging::Logger::getInstance().warning("ShaderManager::reloadAllShaders not implemented");
    } catch (...) {}
}

ShaderId ShaderManager::createShaderFromSource(const std::string& name, 
                                               const std::string& vertexSource, 
                                               const std::string& fragmentSource,
                                               OpenGLRenderer* renderer) {
    if (!renderer) {
        try {
            Logging::Logger::getInstance().error("ShaderManager::createShaderFromSource - null renderer provided");
        } catch (...) {}
        return InvalidId;
    }
    
    // Re-enable logging with proper error handling
    try {
        auto& logger = Logging::Logger::getInstance();
        logger.info("Compiling shader program: " + name);
        
        // Count lines safely
        auto vertexLines = std::count(vertexSource.begin(), vertexSource.end(), '\n');
        auto fragmentLines = std::count(fragmentSource.begin(), fragmentSource.end(), '\n');
        
        logger.debug("Vertex shader source lines: " + std::to_string(static_cast<int>(vertexLines)));
        logger.debug("Fragment shader source lines: " + std::to_string(static_cast<int>(fragmentLines)));
    } catch (...) {
        // If logging fails, continue without it
    }
    
    // Create vertex shader
    ShaderId vertexShader = renderer->createShader(ShaderType::Vertex, vertexSource);
    const ShaderInfo* vertexInfo = renderer->getShaderInfo(vertexShader);
    if (vertexShader == InvalidId || !vertexInfo || !vertexInfo->compiled) {
        try {
            Logging::Logger::getInstance().error("Failed to compile vertex shader: " + name);
            Logging::Logger::getInstance().debug("Vertex shader source:\n" + vertexSource);
            if (vertexInfo && !vertexInfo->errorLog.empty()) {
                Logging::Logger::getInstance().error("Vertex shader error: " + vertexInfo->errorLog);
            }
        } catch (...) {}
        // Assert when failing to ensure we are not masking problems
        assert(false && "Vertex shader compilation failed - failing hard to catch issues early");
        return InvalidId;
    }
    try {
        Logging::Logger::getInstance().debug("Successfully compiled vertex shader for: " + name);
    } catch (...) {}
    
    // Create fragment shader
    ShaderId fragmentShader = renderer->createShader(ShaderType::Fragment, fragmentSource);
    const ShaderInfo* fragmentInfo = renderer->getShaderInfo(fragmentShader);
    if (fragmentShader == InvalidId || !fragmentInfo || !fragmentInfo->compiled) {
        try {
            Logging::Logger::getInstance().error("Failed to compile fragment shader: " + name);
            Logging::Logger::getInstance().debug("Fragment shader source:\n" + fragmentSource);
            if (fragmentInfo && !fragmentInfo->errorLog.empty()) {
                Logging::Logger::getInstance().error("Fragment shader error: " + fragmentInfo->errorLog);
            }
        } catch (...) {}
        renderer->deleteShader(vertexShader);
        // Assert when failing to ensure we are not masking problems
        assert(false && "Fragment shader compilation failed - failing hard to catch issues early");
        return InvalidId;
    }
    try {
        Logging::Logger::getInstance().debug("Successfully compiled fragment shader for: " + name);
    } catch (...) {}
    
    // Create program
    std::vector<ShaderId> shaders = {vertexShader, fragmentShader};
    ShaderId program = renderer->createProgram(shaders);
    
    // Clean up individual shaders (they're linked into the program now)
    renderer->deleteShader(vertexShader);
    renderer->deleteShader(fragmentShader);
    
    if (program == InvalidId) {
        try {
            Logging::Logger::getInstance().error("Failed to link shader program: " + name);
            Logging::Logger::getInstance().debug("Make sure vertex outputs match fragment inputs (varyings)");
        } catch (...) {}
        // Assert when failing to ensure we are not masking problems
        assert(false && "Shader program linking failed - failing hard to catch issues early");
        return InvalidId;
    }
    
    // Create ShaderProgram wrapper
    ShaderEntry entry;
    entry.id = program;
    entry.name = name;
    entry.program = std::make_unique<ShaderProgram>(program, name, renderer);
    entry.program->setValid(true); // Mark as valid since linking succeeded
    entry.source = ShaderSource(vertexSource, fragmentSource);
    entry.isBuiltIn = false;
    
    // Store in our maps
    m_shaders[program] = std::move(entry);
    m_shadersByName[name] = program;
    
    try {
        Logging::Logger::getInstance().info("Successfully created shader program: " + name + " (ID: " + std::to_string(program) + ")");
    } catch (...) {}
    return program;
}

ShaderProgram* ShaderManager::getShaderProgram(ShaderId id) {
    auto it = m_shaders.find(id);
    if (it != m_shaders.end()) {
        return it->second.program.get();
    }
    return nullptr;
}

ShaderProgram* ShaderManager::getShaderProgram(const std::string& name) {
    ShaderId id = getShader(name);
    if (id != InvalidId) {
        return getShaderProgram(id);
    }
    return nullptr;
}

void ShaderManager::cleanup() {
    m_shaders.clear();
    m_shadersByName.clear();
}

// ShaderProgram implementation
ShaderProgram::ShaderProgram(ShaderId id, const std::string& name, OpenGLRenderer* renderer)
    : m_id(id)
    , m_name(name)
    , m_renderer(renderer)
    , m_valid(false)
    , m_inUse(false)
    , m_dirty(false) {
}

void ShaderProgram::setUniform(const std::string& name, const Math::Matrix4f& value) {
    if (!m_renderer || !m_valid) return;
    
    UniformValue uniformValue(value);
    m_renderer->setUniform(m_id, name, uniformValue);
}

void ShaderProgram::setUniform(const std::string& name, const Math::Vector3f& value) {
    if (!m_renderer || !m_valid) return;
    
    UniformValue uniformValue(value);
    m_renderer->setUniform(m_id, name, uniformValue);
}

void ShaderProgram::setUniform(const std::string& name, float value) {
    if (!m_renderer || !m_valid) return;
    
    UniformValue uniformValue(value);
    m_renderer->setUniform(m_id, name, uniformValue);
}

void ShaderProgram::setUniform(const std::string& name, const Math::Vector2f& value) {
    if (!m_renderer || !m_valid) return;
    
    UniformValue uniformValue(value);
    m_renderer->setUniform(m_id, name, uniformValue);
}

void ShaderProgram::setUniform(const std::string& name, const Color& value) {
    if (!m_renderer || !m_valid) return;
    
    UniformValue uniformValue(value);
    m_renderer->setUniform(m_id, name, uniformValue);
}

void ShaderProgram::setUniform(const std::string& name, int value) {
    if (!m_renderer || !m_valid) return;
    
    UniformValue uniformValue(value);
    m_renderer->setUniform(m_id, name, uniformValue);
}

void ShaderProgram::setUniform(const std::string& name, bool value) {
    if (!m_renderer || !m_valid) return;
    
    UniformValue uniformValue(value ? 1 : 0);
    m_renderer->setUniform(m_id, name, uniformValue);
}

void ShaderProgram::setUniform(const std::string& name, const UniformValue& value) {
    if (!m_renderer || !m_valid) return;
    
    m_renderer->setUniform(m_id, name, value);
}

void ShaderProgram::use() {
    if (!m_renderer || !m_valid) return;
    
    m_renderer->useProgram(m_id);
    m_inUse = true;
}

void ShaderProgram::unuse() {
    if (!m_renderer) return;
    
    m_renderer->useProgram(InvalidId);
    m_inUse = false;
}

bool ShaderProgram::hasUniform(const std::string& name) const {
    return getUniformLocation(name) >= 0;
}

int ShaderProgram::getUniformLocation(const std::string& name) const {
    if (!m_renderer || !m_valid) return -1;
    
    // Check cache first
    auto it = m_uniformLocations.find(name);
    if (it != m_uniformLocations.end()) {
        return it->second;
    }
    
    // Get location from renderer and cache it
    int location = m_renderer->getUniformLocation(m_id, name);
    if (location >= 0) {
        const_cast<ShaderProgram*>(this)->m_uniformLocations[name] = location;
    }
    
    return location;
}

std::vector<std::string> ShaderProgram::getUniformNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_uniformLocations) {
        names.push_back(pair.first);
    }
    return names;
}

bool ShaderProgram::recompile(const ShaderSource& source) {
    // TODO: Implement recompilation
    return false;
}

void ShaderProgram::cacheUniformLocations() {
    // TODO: Implement uniform location caching
}

void ShaderProgram::updateUniformLocation(const std::string& name) {
    if (!m_renderer || !m_valid) return;
    
    int location = m_renderer->getUniformLocation(m_id, name);
    m_uniformLocations[name] = location;
}

} // namespace Rendering
} // namespace VoxelEditor