#pragma once

#include "RenderTypes.h"
#include "OpenGLRenderer.h"
#include "../camera/Camera.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>

namespace VoxelEditor {
namespace Rendering {

// Shader source structure
struct ShaderSource {
    std::string vertexSource;
    std::string fragmentSource;
    std::string geometrySource;    // Optional
    std::string computeSource;     // Optional
    std::vector<std::string> defines;
    
    ShaderSource() = default;
    
    ShaderSource(const std::string& vertex, const std::string& fragment)
        : vertexSource(vertex), fragmentSource(fragment) {}
    
    bool hasVertex() const { return !vertexSource.empty(); }
    bool hasFragment() const { return !fragmentSource.empty(); }
    bool hasGeometry() const { return !geometrySource.empty(); }
    bool hasCompute() const { return !computeSource.empty(); }
    
    void addDefine(const std::string& define) {
        defines.push_back(define);
    }
    
    void clearDefines() {
        defines.clear();
    }
};

// Shader program wrapper
class ShaderProgram {
public:
    ShaderProgram(ShaderId id, const std::string& name, OpenGLRenderer* renderer);
    ~ShaderProgram();
    
    // Program management
    ShaderId getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    bool isValid() const { return m_valid; }
    const std::string& getErrorLog() const { return m_errorLog; }
    
    // Uniform management
    void setUniform(const std::string& name, const UniformValue& value);
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, const Math::Vector2f& value);
    void setUniform(const std::string& name, const Math::Vector3f& value);
    void setUniform(const std::string& name, const Color& value);
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, const Math::Matrix4f& value);
    void setUniform(const std::string& name, bool value);
    
    // Uniform queries
    bool hasUniform(const std::string& name) const;
    int getUniformLocation(const std::string& name) const;
    std::vector<std::string> getUniformNames() const;
    
    // Program usage
    void use();
    void unuse();
    bool isInUse() const { return m_inUse; }
    
    // Recompilation
    bool recompile(const ShaderSource& source);
    void markDirty() { m_dirty = true; }
    bool isDirty() const { return m_dirty; }
    
private:
    ShaderId m_id;
    std::string m_name;
    OpenGLRenderer* m_renderer;
    bool m_valid;
    bool m_inUse;
    bool m_dirty;
    std::string m_errorLog;
    std::unordered_map<std::string, int> m_uniformLocations;
    
    void cacheUniformLocations();
    void updateUniformLocation(const std::string& name);
};

class ShaderManager {
public:
    explicit ShaderManager(OpenGLRenderer* renderer);
    ~ShaderManager();
    
    // Shader compilation and loading
    ShaderId compileShader(const std::string& name, const ShaderSource& source);
    ShaderId loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    ShaderId loadShaderFromFile(const std::string& name, const std::string& basePath); // Loads .vert and .frag
    bool reloadShader(ShaderId id);
    bool reloadShader(const std::string& name);
    void reloadAllShaders();
    
    // Shader access
    ShaderProgram* getShader(ShaderId id);
    ShaderProgram* getShader(const std::string& name);
    ShaderId findShader(const std::string& name);
    std::vector<std::string> getShaderNames() const;
    std::vector<ShaderId> getShaderIds() const;
    
    // Built-in shaders
    ShaderId getVoxelShader();
    ShaderId getWireframeShader();
    ShaderId getSelectionShader();
    ShaderId getGroupOutlineShader();
    ShaderId getDebugShader();
    ShaderId getPostProcessShader();
    
    // Shader variants
    ShaderId getShaderVariant(const std::string& baseName, const std::vector<std::string>& defines);
    void precompileVariants(const std::string& baseName, const std::vector<std::vector<std::string>>& variants);
    
    // Global uniform management
    void setGlobalUniform(const std::string& name, const UniformValue& value);
    void setGlobalUniform(const std::string& name, float value);
    void setGlobalUniform(const std::string& name, const Math::Vector3f& value);
    void setGlobalUniform(const std::string& name, const Color& value);
    void setGlobalUniform(const std::string& name, const Math::Matrix4f& value);
    void removeGlobalUniform(const std::string& name);
    void clearGlobalUniforms();
    
    // Per-frame updates
    void updatePerFrameUniforms(const Camera::Camera& camera, const RenderStats& stats);
    void updatePerFrameUniforms(const Math::Matrix4f& viewMatrix, 
                               const Math::Matrix4f& projectionMatrix,
                               const Math::Vector3f& cameraPosition,
                               const RenderStats& stats);
    
    // Hot reloading
    void enableHotReload(bool enabled);
    bool isHotReloadEnabled() const { return m_hotReloadEnabled; }
    void checkForChanges();
    void addWatchPath(const std::string& path);
    void removeWatchPath(const std::string& path);
    
    // Shader deletion
    void deleteShader(ShaderId id);
    void deleteShader(const std::string& name);
    void cleanup();
    
    // Debug and utilities
    void dumpShaderInfo(ShaderId id) const;
    void dumpAllShaderInfo() const;
    std::string getShaderSource(ShaderId id) const;
    bool validateShader(ShaderId id) const;
    
private:
    struct ShaderEntry {
        ShaderId id;
        std::string name;
        std::string vertexPath;
        std::string fragmentPath;
        std::string geometryPath;
        std::string computePath;
        std::unique_ptr<ShaderProgram> program;
        ShaderSource source;
        std::filesystem::file_time_type lastModified;
        std::vector<std::string> defines;
        bool isBuiltIn;
        
        ShaderEntry() : id(0), lastModified{}, isBuiltIn(false) {}
    };
    
    OpenGLRenderer* m_renderer;
    std::unordered_map<ShaderId, ShaderEntry> m_shaders;
    std::unordered_map<std::string, ShaderId> m_shadersByName;
    std::unordered_map<std::string, UniformValue> m_globalUniforms;
    std::vector<std::string> m_watchPaths;
    
    ShaderId m_nextShaderId;
    bool m_hotReloadEnabled;
    
    // Built-in shader IDs
    ShaderId m_voxelShaderId;
    ShaderId m_wireframeShaderId;
    ShaderId m_selectionShaderId;
    ShaderId m_groupOutlineShaderId;
    ShaderId m_debugShaderId;
    ShaderId m_postProcessShaderId;
    
    // Helper methods
    bool compileShaderProgram(ShaderEntry& entry);
    std::string loadShaderFile(const std::string& path);
    std::string preprocessShader(const std::string& source, const std::vector<std::string>& defines);
    std::filesystem::file_time_type getFileModificationTime(const std::string& path);
    bool hasFileChanged(const ShaderEntry& entry);
    void applyGlobalUniforms(ShaderProgram* program);
    void loadBuiltinShaders();
    std::string generateVariantName(const std::string& baseName, const std::vector<std::string>& defines);
    
    // Built-in shader sources
    static const char* getBuiltinVertexShader(const std::string& name);
    static const char* getBuiltinFragmentShader(const std::string& name);
    
    // Common shader preprocessor defines
    std::vector<std::string> getCommonDefines() const;
    std::string insertCommonIncludes(const std::string& source) const;
};

// Helper macros for shader compilation
#define SHADER_VERTEX(source) #source
#define SHADER_FRAGMENT(source) #source
#define SHADER_GEOMETRY(source) #source

// Built-in shader paths
namespace BuiltinShaders {
    constexpr const char* VOXEL = "voxel";
    constexpr const char* WIREFRAME = "wireframe";
    constexpr const char* SELECTION = "selection";
    constexpr const char* GROUP_OUTLINE = "group_outline";
    constexpr const char* DEBUG = "debug";
    constexpr const char* POST_PROCESS = "post_process";
}

}
}