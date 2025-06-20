#include "ShaderManager.h"
#include "../../foundation/logging/Logger.h"
#include "../../foundation/config/BuildConfig.h"
#include <algorithm>
#include <fstream>
#include <sstream>

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