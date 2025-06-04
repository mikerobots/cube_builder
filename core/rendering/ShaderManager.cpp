#include "ShaderManager.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>

namespace VoxelEditor {
namespace Rendering {

ShaderManager::ShaderManager() 
    : m_nextShaderId(1)
    , m_hotReloadEnabled(false) {
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
    // TODO: Implement shader loading from files
    try {
        Logging::Logger::getInstance().warning("ShaderManager::loadShader not implemented");
    } catch (...) {}
    (void)name;
    (void)vertexPath;
    (void)fragmentPath;
    return InvalidId;
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
    if (vertexShader == InvalidId) {
        try {
            Logging::Logger::getInstance().error("Failed to compile vertex shader: " + name);
            Logging::Logger::getInstance().debug("Vertex shader source:\n" + vertexSource);
        } catch (...) {}
        return InvalidId;
    }
    try {
        Logging::Logger::getInstance().debug("Successfully compiled vertex shader for: " + name);
    } catch (...) {}
    
    // Create fragment shader
    ShaderId fragmentShader = renderer->createShader(ShaderType::Fragment, fragmentSource);
    if (fragmentShader == InvalidId) {
        try {
            Logging::Logger::getInstance().error("Failed to compile fragment shader: " + name);
            Logging::Logger::getInstance().debug("Fragment shader source:\n" + fragmentSource);
        } catch (...) {}
        renderer->deleteShader(vertexShader);
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
        return InvalidId;
    }
    
    // Store in our map
    m_shadersByName[name] = program;
    
    try {
        Logging::Logger::getInstance().info("Successfully created shader program: " + name + " (ID: " + std::to_string(program) + ")");
    } catch (...) {}
    return program;
}

void ShaderManager::cleanup() {
    m_shadersByName.clear();
}

} // namespace Rendering
} // namespace VoxelEditor