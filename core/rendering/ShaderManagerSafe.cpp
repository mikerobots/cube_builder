#include "ShaderManagerSafe.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <iostream>

namespace VoxelEditor {
namespace Rendering {

// ProductionShaderLogger implementation
void ProductionShaderLogger::logInfo(const std::string& message) {
    try {
        VoxelEditor::Logging::Logger::getInstance().info(message);
    } catch (...) {
        // Fallback to standard output if logger fails
        std::cout << "[INFO] " << message << std::endl;
    }
}

void ProductionShaderLogger::logDebug(const std::string& message) {
    try {
        VoxelEditor::Logging::Logger::getInstance().debug(message);
    } catch (...) {
        // Fallback to standard output if logger fails
        std::cout << "[DEBUG] " << message << std::endl;
    }
}

void ProductionShaderLogger::logError(const std::string& message) {
    try {
        VoxelEditor::Logging::Logger::getInstance().error(message);
    } catch (...) {
        // Fallback to standard error if logger fails
        std::cerr << "[ERROR] " << message << std::endl;
    }
}

void ProductionShaderLogger::logWarning(const std::string& message) {
    try {
        VoxelEditor::Logging::Logger::getInstance().warning(message);
    } catch (...) {
        // Fallback to standard output if logger fails
        std::cout << "[WARNING] " << message << std::endl;
    }
}

// ShaderManagerSafe implementation
ShaderManagerSafe::ShaderManagerSafe(std::unique_ptr<IShaderLogger> logger) 
    : m_logger(logger ? std::move(logger) : std::make_unique<NullShaderLogger>()) {
}

ShaderManagerSafe::~ShaderManagerSafe() {
    cleanup();
}

ShaderId ShaderManagerSafe::getShader(const std::string& name) {
    auto it = m_shadersByName.find(name);
    if (it != m_shadersByName.end()) {
        return it->second;
    }
    return InvalidId;
}

ShaderId ShaderManagerSafe::createShaderFromSource(const std::string& name, 
                                                   const std::string& vertexSource, 
                                                   const std::string& fragmentSource,
                                                   OpenGLRenderer* renderer) {
    if (!renderer) {
        m_logger->logError("ShaderManagerSafe::createShaderFromSource - null renderer provided");
        return InvalidId;
    }
    
    m_logger->logInfo("Compiling shader program: " + name);
    
    // Count lines safely
    auto vertexLines = std::count(vertexSource.begin(), vertexSource.end(), '\n');
    auto fragmentLines = std::count(fragmentSource.begin(), fragmentSource.end(), '\n');
    
    m_logger->logDebug("Vertex shader source lines: " + std::to_string(static_cast<int>(vertexLines)));
    m_logger->logDebug("Fragment shader source lines: " + std::to_string(static_cast<int>(fragmentLines)));
    
    // Create vertex shader
    ShaderId vertexShader = renderer->createShader(ShaderType::Vertex, vertexSource);
    if (vertexShader == InvalidId) {
        m_logger->logError("Failed to compile vertex shader: " + name);
        m_logger->logDebug("Vertex shader source:\n" + vertexSource);
        return InvalidId;
    }
    m_logger->logDebug("Successfully compiled vertex shader for: " + name);
    
    // Create fragment shader
    ShaderId fragmentShader = renderer->createShader(ShaderType::Fragment, fragmentSource);
    if (fragmentShader == InvalidId) {
        m_logger->logError("Failed to compile fragment shader: " + name);
        m_logger->logDebug("Fragment shader source:\n" + fragmentSource);
        renderer->deleteShader(vertexShader);
        return InvalidId;
    }
    m_logger->logDebug("Successfully compiled fragment shader for: " + name);
    
    // Create program
    std::vector<ShaderId> shaders = {vertexShader, fragmentShader};
    ShaderId program = renderer->createProgram(shaders);
    
    // Clean up individual shaders
    renderer->deleteShader(vertexShader);
    renderer->deleteShader(fragmentShader);
    
    if (program == InvalidId) {
        m_logger->logError("Failed to link shader program: " + name);
        m_logger->logDebug("Make sure vertex outputs match fragment inputs (varyings)");
        return InvalidId;
    }
    
    // Store in our map
    m_shadersByName[name] = program;
    
    m_logger->logInfo("Successfully created shader program: " + name + " (ID: " + std::to_string(program) + ")");
    return program;
}

void ShaderManagerSafe::cleanup() {
    m_shadersByName.clear();
}

// Factory methods
std::unique_ptr<ShaderManagerSafe> ShaderManagerSafe::createForProduction() {
    return std::make_unique<ShaderManagerSafe>(std::make_unique<ProductionShaderLogger>());
}

std::unique_ptr<ShaderManagerSafe> ShaderManagerSafe::createForTesting() {
    return std::make_unique<ShaderManagerSafe>(std::make_unique<TestShaderLogger>());
}

std::unique_ptr<ShaderManagerSafe> ShaderManagerSafe::createSilent() {
    return std::make_unique<ShaderManagerSafe>(std::make_unique<NullShaderLogger>());
}

}
}