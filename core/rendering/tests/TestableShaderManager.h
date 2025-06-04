#pragma once
#include "../RenderTypes.h"
#include "../OpenGLRenderer.h"
#include "../../../foundation/logging/Logger.h"
#include "TestLogger.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>

namespace VoxelEditor {
namespace Rendering {
namespace Test {

// Interface for logging to allow dependency injection
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void info(const std::string& message) = 0;
    virtual void debug(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
    virtual void warning(const std::string& message) = 0;
};

// Adapter for the production logger
class ProductionLoggerAdapter : public ILogger {
public:
    void info(const std::string& message) override {
        try {
            VoxelEditor::Logging::Logger::getInstance().info(message);
        } catch (...) {}
    }
    
    void debug(const std::string& message) override {
        try {
            VoxelEditor::Logging::Logger::getInstance().debug(message);
        } catch (...) {}
    }
    
    void error(const std::string& message) override {
        try {
            VoxelEditor::Logging::Logger::getInstance().error(message);
        } catch (...) {}
    }
    
    void warning(const std::string& message) override {
        try {
            VoxelEditor::Logging::Logger::getInstance().warning(message);
        } catch (...) {}
    }
};

// Test logger adapter
class TestLoggerAdapter : public ILogger {
public:
    void info(const std::string& message) override {
        TestLogger::info(message);
    }
    
    void debug(const std::string& message) override {
        TestLogger::debug(message);
    }
    
    void error(const std::string& message) override {
        TestLogger::error(message);
    }
    
    void warning(const std::string& message) override {
        TestLogger::warning(message);
    }
};

// Null logger for tests that don't want any logging
class NullLoggerAdapter : public ILogger {
public:
    void info(const std::string&) override {}
    void debug(const std::string&) override {}
    void error(const std::string&) override {}
    void warning(const std::string&) override {}
};

// Testable version of ShaderManager with dependency injection
class TestableShaderManager {
public:
    explicit TestableShaderManager(std::unique_ptr<ILogger> logger = nullptr) 
        : m_logger(logger ? std::move(logger) : std::make_unique<NullLoggerAdapter>()) {}
    
    ShaderId getShader(const std::string& name) {
        auto it = m_shadersByName.find(name);
        if (it != m_shadersByName.end()) {
            return it->second;
        }
        return InvalidId;
    }
    
    ShaderId createShaderFromSource(const std::string& name, 
                                   const std::string& vertexSource, 
                                   const std::string& fragmentSource,
                                   OpenGLRenderer* renderer) {
        if (!renderer) {
            m_logger->error("TestableShaderManager::createShaderFromSource - null renderer provided");
            return InvalidId;
        }
        
        m_logger->info("Compiling shader program: " + name);
        
        // Count lines safely
        auto vertexLines = std::count(vertexSource.begin(), vertexSource.end(), '\n');
        auto fragmentLines = std::count(fragmentSource.begin(), fragmentSource.end(), '\n');
        
        m_logger->debug("Vertex shader source lines: " + std::to_string(static_cast<int>(vertexLines)));
        m_logger->debug("Fragment shader source lines: " + std::to_string(static_cast<int>(fragmentLines)));
        
        // Create vertex shader
        ShaderId vertexShader = renderer->createShader(ShaderType::Vertex, vertexSource);
        if (vertexShader == InvalidId) {
            m_logger->error("Failed to compile vertex shader: " + name);
            m_logger->debug("Vertex shader source:\n" + vertexSource);
            return InvalidId;
        }
        m_logger->debug("Successfully compiled vertex shader for: " + name);
        
        // Create fragment shader
        ShaderId fragmentShader = renderer->createShader(ShaderType::Fragment, fragmentSource);
        if (fragmentShader == InvalidId) {
            m_logger->error("Failed to compile fragment shader: " + name);
            m_logger->debug("Fragment shader source:\n" + fragmentSource);
            renderer->deleteShader(vertexShader);
            return InvalidId;
        }
        m_logger->debug("Successfully compiled fragment shader for: " + name);
        
        // Create program
        std::vector<ShaderId> shaders = {vertexShader, fragmentShader};
        ShaderId program = renderer->createProgram(shaders);
        
        // Clean up individual shaders
        renderer->deleteShader(vertexShader);
        renderer->deleteShader(fragmentShader);
        
        if (program == InvalidId) {
            m_logger->error("Failed to link shader program: " + name);
            m_logger->debug("Make sure vertex outputs match fragment inputs (varyings)");
            return InvalidId;
        }
        
        // Store in our map
        m_shadersByName[name] = program;
        
        m_logger->info("Successfully created shader program: " + name + " (ID: " + std::to_string(program) + ")");
        return program;
    }
    
    void cleanup() {
        m_shadersByName.clear();
    }

private:
    std::unique_ptr<ILogger> m_logger;
    std::unordered_map<std::string, ShaderId> m_shadersByName;
};

}
}
}