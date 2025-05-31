#pragma once

#include "RenderTypes.h"
#include "RenderConfig.h"
#include "RenderStats.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace VoxelEditor {
namespace Rendering {

// Forward declarations
class RenderState;
enum class VertexAttribute;

// Resource information structures
struct BufferInfo {
    BufferId id;
    BufferUsage usage;
    size_t size;
    uint32_t glHandle;
    bool isIndexBuffer;
    
    BufferInfo() : id(0), usage(BufferUsage::Static), size(0), glHandle(0), isIndexBuffer(false) {}
};

struct TextureInfo {
    TextureId id;
    TextureFormat format;
    int width;
    int height;
    uint32_t glHandle;
    size_t memorySize;
    
    TextureInfo() : id(0), format(TextureFormat::RGBA8), width(0), height(0), glHandle(0), memorySize(0) {}
};

struct ShaderInfo {
    ShaderId id;
    ShaderType type;
    uint32_t glHandle;
    std::string source;
    bool compiled;
    std::string errorLog;
    
    ShaderInfo() : id(0), type(ShaderType::Vertex), glHandle(0), compiled(false) {}
};

struct ProgramInfo {
    ShaderId id;
    uint32_t glHandle;
    std::vector<ShaderId> shaders;
    std::unordered_map<std::string, int> uniformLocations;
    bool linked;
    std::string errorLog;
    
    ProgramInfo() : id(0), glHandle(0), linked(false) {}
};

// Uniform value variant
struct UniformValue {
    enum Type {
        Float,
        Vec2,
        Vec3,
        Vec4,
        Int,
        IVec2,
        IVec3,
        IVec4,
        Mat3,
        Mat4,
        Sampler2D
    } type;
    
    union {
        float f;
        float vec2[2];
        float vec3[3];
        float vec4[4];
        int i;
        int ivec2[2];
        int ivec3[3];
        int ivec4[4];
        float mat3[9];
        float mat4[16];
        int sampler;
    } data;
    
    UniformValue(float value) : type(Float) { data.f = value; }
    UniformValue(const Math::Vector2f& value) : type(Vec2) { 
        data.vec2[0] = value.x; data.vec2[1] = value.y; 
    }
    UniformValue(const Math::Vector3f& value) : type(Vec3) { 
        data.vec3[0] = value.x; data.vec3[1] = value.y; data.vec3[2] = value.z; 
    }
    UniformValue(const Color& value) : type(Vec4) { 
        data.vec4[0] = value.r; data.vec4[1] = value.g; data.vec4[2] = value.b; data.vec4[3] = value.a; 
    }
    UniformValue(int value) : type(Int) { data.i = value; }
    UniformValue(const Math::Matrix4f& value) : type(Mat4) {
        for (int i = 0; i < 16; ++i) {
            data.mat4[i] = value.m[i];
        }
    }
};

class OpenGLRenderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer();
    
    // Context management
    bool initializeContext(const RenderConfig& config);
    void destroyContext();
    bool isContextValid() const { return m_contextValid; }
    const std::string& getRendererInfo() const { return m_rendererInfo; }
    
    // Buffer operations
    BufferId createVertexBuffer(const void* data, size_t size, BufferUsage usage = BufferUsage::Static);
    BufferId createIndexBuffer(const uint32_t* indices, size_t count, BufferUsage usage = BufferUsage::Static);
    void updateBuffer(BufferId bufferId, const void* data, size_t size, size_t offset = 0);
    void bindVertexBuffer(BufferId bufferId);
    void bindIndexBuffer(BufferId bufferId);
    void deleteBuffer(BufferId bufferId);
    
    // Vertex array objects
    uint32_t createVertexArray();
    void bindVertexArray(uint32_t vaoId);
    void deleteVertexArray(uint32_t vaoId);
    void setupVertexAttributes(const std::vector<VertexAttribute>& attributes);
    
    // Texture operations
    TextureId createTexture2D(int width, int height, TextureFormat format, const void* data = nullptr);
    TextureId createTextureCube(int size, TextureFormat format, const void* data[6] = nullptr);
    void updateTexture(TextureId textureId, int x, int y, int width, int height, const void* data);
    void bindTexture(TextureId textureId, int slot = 0);
    void deleteTexture(TextureId textureId);
    void generateMipmaps(TextureId textureId);
    void setTextureParameters(TextureId textureId, bool mipmapping, float anisotropy);
    
    // Shader operations
    ShaderId createShader(ShaderType type, const std::string& source);
    ShaderId createProgram(const std::vector<ShaderId>& shaders);
    void useProgram(ShaderId programId);
    void deleteShader(ShaderId shaderId);
    void deleteProgram(ShaderId programId);
    
    // Uniform operations
    void setUniform(ShaderId programId, const std::string& name, const UniformValue& value);
    void setUniform(const std::string& name, const UniformValue& value); // Uses currently bound program
    int getUniformLocation(ShaderId programId, const std::string& name);
    
    // Drawing operations
    void drawArrays(PrimitiveType type, int first, int count);
    void drawElements(PrimitiveType type, int count, IndexType indexType = IndexType::UInt32, int offset = 0);
    void drawElementsInstanced(PrimitiveType type, int count, int instanceCount, IndexType indexType = IndexType::UInt32);
    
    // State management
    void setDepthTest(bool enabled);
    void setDepthWrite(bool enabled);
    void setBlending(bool enabled, BlendMode mode = BlendMode::Alpha);
    void setCulling(bool enabled, CullMode mode = CullMode::Back);
    void setViewport(int x, int y, int width, int height);
    void setPolygonMode(bool wireframe);
    void setLineWidth(float width);
    void setPointSize(float size);
    
    // Clear operations
    void clear(ClearFlags flags, const Color& color = Color::Black(), float depth = 1.0f, int stencil = 0);
    void setClearColor(const Color& color);
    
    // Resource queries
    const BufferInfo* getBufferInfo(BufferId bufferId) const;
    const TextureInfo* getTextureInfo(TextureId textureId) const;
    const ShaderInfo* getShaderInfo(ShaderId shaderId) const;
    const ProgramInfo* getProgramInfo(ShaderId programId) const;
    
    // Memory and statistics
    size_t getTotalBufferMemory() const;
    size_t getTotalTextureMemory() const;
    uint32_t getActiveBufferCount() const { return static_cast<uint32_t>(m_buffers.size()); }
    uint32_t getActiveTextureCount() const { return static_cast<uint32_t>(m_textures.size()); }
    uint32_t getActiveShaderCount() const { return static_cast<uint32_t>(m_shaders.size()) + static_cast<uint32_t>(m_programs.size()); }
    
    // Debug operations
    void pushDebugGroup(const std::string& name);
    void popDebugGroup();
    void setObjectLabel(uint32_t glHandle, const std::string& label);
    bool checkGLError(const std::string& operation = "");
    
    // Capability queries
    bool supportsAnisotropicFiltering() const { return m_supportsAnisotropicFiltering; }
    bool supportsDebugOutput() const { return m_supportsDebugOutput; }
    bool supportsTimestampQueries() const { return m_supportsTimestampQueries; }
    float getMaxAnisotropy() const { return m_maxAnisotropy; }
    int getMaxTextureSize() const { return m_maxTextureSize; }
    int getMaxTextureUnits() const { return m_maxTextureUnits; }
    
    // Performance monitoring
    void beginTimestampQuery(const std::string& name);
    void endTimestampQuery(const std::string& name);
    float getQueryTime(const std::string& name) const;
    
private:
    // Context state
    bool m_contextValid;
    std::string m_rendererInfo;
    RenderState* m_renderState;
    
    // Resource management
    std::unordered_map<BufferId, BufferInfo> m_buffers;
    std::unordered_map<TextureId, TextureInfo> m_textures;
    std::unordered_map<ShaderId, ShaderInfo> m_shaders;
    std::unordered_map<ShaderId, ProgramInfo> m_programs;
    
    // ID generation
    BufferId m_nextBufferId;
    TextureId m_nextTextureId;
    ShaderId m_nextShaderId;
    
    // OpenGL capabilities
    bool m_supportsAnisotropicFiltering;
    bool m_supportsDebugOutput;
    bool m_supportsTimestampQueries;
    float m_maxAnisotropy;
    int m_maxTextureSize;
    int m_maxTextureUnits;
    int m_maxVertexAttributes;
    
    // Performance monitoring
    struct TimestampQuery {
        uint32_t startQuery;
        uint32_t endQuery;
        bool active;
        float lastResult;
    };
    std::unordered_map<std::string, TimestampQuery> m_timestampQueries;
    
    // Helper methods
    uint32_t translateTextureFormat(TextureFormat format, uint32_t& internalFormat, uint32_t& type);
    uint32_t translateBufferUsage(BufferUsage usage);
    uint32_t translatePrimitiveType(PrimitiveType type);
    uint32_t translateIndexType(IndexType type);
    uint32_t translateShaderType(ShaderType type);
    uint32_t translateBlendMode(BlendMode mode, uint32_t& srcFactor, uint32_t& dstFactor);
    uint32_t translateCullMode(CullMode mode);
    
    void queryCapabilities();
    void setupDebugOutput();
    bool compileShaderInternal(ShaderInfo& info);
    bool linkProgramInternal(ProgramInfo& info);
    void updateUniformLocations(ProgramInfo& info);
    size_t calculateTextureMemory(int width, int height, TextureFormat format);
    void cleanupDeletedResources();
    
    // Debug callback
    static void debugCallback(uint32_t source, uint32_t type, uint32_t id, 
                             uint32_t severity, int length, 
                             const char* message, const void* userParam);
};

// Vertex attribute structure
struct VertexAttributeInfo {
    int location;
    int size;           // Number of components (1-4)
    uint32_t type;      // GL_FLOAT, GL_INT, etc.
    bool normalized;
    int stride;
    size_t offset;
    
    VertexAttributeInfo(int loc, int sz, uint32_t tp, bool norm, int str, size_t off)
        : location(loc), size(sz), type(tp), normalized(norm), stride(str), offset(off) {}
    
    // Common vertex attributes
    static VertexAttributeInfo Position(int stride, size_t offset = 0) {
        return VertexAttributeInfo(0, 3, 0x1406 /* GL_FLOAT */, false, stride, offset); // Position
    }
    
    static VertexAttributeInfo Normal(int stride, size_t offset) {
        return VertexAttributeInfo(1, 3, 0x1406 /* GL_FLOAT */, false, stride, offset); // Normal
    }
    
    static VertexAttributeInfo TexCoord(int stride, size_t offset) {
        return VertexAttributeInfo(2, 2, 0x1406 /* GL_FLOAT */, false, stride, offset); // TexCoord
    }
    
    static VertexAttributeInfo Color(int stride, size_t offset) {
        return VertexAttributeInfo(3, 4, 0x1406 /* GL_FLOAT */, false, stride, offset); // Color
    }
};

}
}