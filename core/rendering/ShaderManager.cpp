#include "ShaderManager.h"
#include "../../foundation/logging/Logger.h"

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
    Logging::Logger::getInstance().warning("ShaderManager::loadShader not implemented");
    (void)name;
    (void)vertexPath;
    (void)fragmentPath;
    return InvalidId;
}

void ShaderManager::reloadAllShaders() {
    // TODO: Implement shader reloading
    Logging::Logger::getInstance().warning("ShaderManager::reloadAllShaders not implemented");
}

ShaderId ShaderManager::createShaderFromSource(const std::string& name, 
                                               const std::string& vertexSource, 
                                               const std::string& fragmentSource,
                                               OpenGLRenderer* renderer) {
    if (!renderer) return InvalidId;
    
    // Create vertex shader
    ShaderId vertexShader = renderer->createShader(ShaderType::Vertex, vertexSource);
    if (vertexShader == InvalidId) {
        Logging::Logger::getInstance().error("Failed to compile vertex shader: " + name);
        return InvalidId;
    }
    
    // Create fragment shader
    ShaderId fragmentShader = renderer->createShader(ShaderType::Fragment, fragmentSource);
    if (fragmentShader == InvalidId) {
        Logging::Logger::getInstance().error("Failed to compile fragment shader: " + name);
        renderer->deleteShader(vertexShader);
        return InvalidId;
    }
    
    // Create program
    std::vector<ShaderId> shaders = {vertexShader, fragmentShader};
    ShaderId program = renderer->createProgram(shaders);
    
    // Clean up individual shaders (they're linked into the program now)
    renderer->deleteShader(vertexShader);
    renderer->deleteShader(fragmentShader);
    
    if (program == InvalidId) {
        Logging::Logger::getInstance().error("Failed to link shader program: " + name);
        return InvalidId;
    }
    
    // Store in our map
    m_shadersByName[name] = program;
    
    Logging::Logger::getInstance().info("Created shader program: " + name);
    return program;
}

void ShaderManager::cleanup() {
    m_shadersByName.clear();
}

} // namespace Rendering
} // namespace VoxelEditor