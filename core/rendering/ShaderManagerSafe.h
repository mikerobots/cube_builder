#pragma once
#include "RenderTypes.h"
#include "OpenGLRenderer.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

namespace VoxelEditor {
namespace Rendering {

// Interface for safe logging in tests
class IShaderLogger {
public:
    virtual ~IShaderLogger() = default;
    virtual void logInfo(const std::string& message) = 0;
    virtual void logDebug(const std::string& message) = 0;
    virtual void logError(const std::string& message) = 0;
    virtual void logWarning(const std::string& message) = 0;
};

// Production logger that uses the singleton with safety
class ProductionShaderLogger : public IShaderLogger {
public:
    void logInfo(const std::string& message) override;
    void logDebug(const std::string& message) override;
    void logError(const std::string& message) override;
    void logWarning(const std::string& message) override;
};

// Test logger that's safe for test environments
class TestShaderLogger : public IShaderLogger {
public:
    void logInfo(const std::string& message) override {
        std::cout << "[INFO] " << message << std::endl;
    }
    
    void logDebug(const std::string& message) override {
        std::cout << "[DEBUG] " << message << std::endl;
    }
    
    void logError(const std::string& message) override {
        std::cerr << "[ERROR] " << message << std::endl;
    }
    
    void logWarning(const std::string& message) override {
        std::cout << "[WARNING] " << message << std::endl;
    }
};

// Null logger for tests that don't want logging
class NullShaderLogger : public IShaderLogger {
public:
    void logInfo(const std::string&) override {}
    void logDebug(const std::string&) override {}
    void logError(const std::string&) override {}
    void logWarning(const std::string&) override {}
};

// Safe ShaderManager that allows dependency injection of logging
class ShaderManagerSafe {
public:
    explicit ShaderManagerSafe(std::unique_ptr<IShaderLogger> logger = nullptr);
    ~ShaderManagerSafe();
    
    ShaderId getShader(const std::string& name);
    
    ShaderId createShaderFromSource(const std::string& name, 
                                   const std::string& vertexSource, 
                                   const std::string& fragmentSource,
                                   OpenGLRenderer* renderer);
    
    void cleanup();
    
    // Factory methods
    static std::unique_ptr<ShaderManagerSafe> createForProduction();
    static std::unique_ptr<ShaderManagerSafe> createForTesting();
    static std::unique_ptr<ShaderManagerSafe> createSilent();

private:
    std::unique_ptr<IShaderLogger> m_logger;
    std::unordered_map<std::string, ShaderId> m_shadersByName;
};

}
}