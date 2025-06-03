#include "OpenGLRenderer.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// Silence OpenGL deprecation warnings on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <glad/glad.h>

namespace VoxelEditor {
namespace Rendering {

// Static debug callback
void GLAPIENTRY OpenGLRenderer::debugCallback(GLenum source, GLenum type, GLuint id, 
                         GLenum severity, GLsizei length, 
                         const GLchar* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
    
    std::cerr << "GL Debug: ";
    
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: std::cerr << "[HIGH] "; break;
        case GL_DEBUG_SEVERITY_MEDIUM: std::cerr << "[MEDIUM] "; break;
        case GL_DEBUG_SEVERITY_LOW: std::cerr << "[LOW] "; break;
        default: std::cerr << "[INFO] "; break;
    }
    
    std::cerr << message << std::endl;
}

OpenGLRenderer::OpenGLRenderer() 
    : m_contextValid(false)
    , m_renderState(nullptr)
    , m_nextBufferId(1)
    , m_nextTextureId(1)
    , m_nextShaderId(1)
    , m_supportsAnisotropicFiltering(false)
    , m_supportsDebugOutput(false)
    , m_supportsTimestampQueries(false)
    , m_maxAnisotropy(1.0f)
    , m_maxTextureSize(2048)
    , m_maxTextureUnits(16)
    , m_maxVertexAttributes(16) {
}

OpenGLRenderer::~OpenGLRenderer() {
    if (m_contextValid) {
        destroyContext();
    }
}

bool OpenGLRenderer::initializeContext(const RenderConfig& config) {
    if (m_contextValid) {
        return false;
    }
    
    // Query OpenGL capabilities
    queryCapabilities();
    
    // Set up debug output if available
    if (m_supportsDebugOutput && config.enableDebugOutput) {
        setupDebugOutput();
    }
    
    // Get renderer info
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    
    std::stringstream ss;
    ss << "OpenGL " << version << " - " << renderer << " (" << vendor << ")";
    m_rendererInfo = ss.str();
    
    // Set default OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    // Enable seamless cubemap filtering
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
    m_contextValid = true;
    
    return true;
}

void OpenGLRenderer::destroyContext() {
    // Clean up all resources
    for (auto& pair : m_buffers) {
        glDeleteBuffers(1, &pair.second.glHandle);
    }
    m_buffers.clear();
    
    for (auto& pair : m_textures) {
        glDeleteTextures(1, &pair.second.glHandle);
    }
    m_textures.clear();
    
    for (auto& pair : m_shaders) {
        glDeleteShader(pair.second.glHandle);
    }
    m_shaders.clear();
    
    for (auto& pair : m_programs) {
        glDeleteProgram(pair.second.glHandle);
    }
    m_programs.clear();
    
    m_contextValid = false;
}

BufferId OpenGLRenderer::createVertexBuffer(const void* data, size_t size, BufferUsage usage) {
    BufferId id = m_nextBufferId++;
    BufferInfo info;
    info.id = id;
    info.usage = usage;
    info.size = size;
    info.isIndexBuffer = false;
    
    // Create OpenGL buffer
    glGenBuffers(1, &info.glHandle);
    glBindBuffer(GL_ARRAY_BUFFER, info.glHandle);
    glBufferData(GL_ARRAY_BUFFER, size, data, translateBufferUsage(usage));
    
    checkGLError("createVertexBuffer");
    
    m_buffers[id] = info;
    return id;
}

BufferId OpenGLRenderer::createIndexBuffer(const uint32_t* indices, size_t count, BufferUsage usage) {
    BufferId id = m_nextBufferId++;
    BufferInfo info;
    info.id = id;
    info.usage = usage;
    info.size = count * sizeof(uint32_t);
    info.isIndexBuffer = true;
    
    // Create OpenGL buffer
    glGenBuffers(1, &info.glHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, info.glHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, info.size, indices, translateBufferUsage(usage));
    
    checkGLError("createIndexBuffer");
    
    m_buffers[id] = info;
    return id;
}

void OpenGLRenderer::updateBuffer(BufferId bufferId, const void* data, size_t size, size_t offset) {
    auto it = m_buffers.find(bufferId);
    if (it == m_buffers.end()) return;
    
    const BufferInfo& info = it->second;
    GLenum target = info.isIndexBuffer ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    
    glBindBuffer(target, info.glHandle);
    glBufferSubData(target, offset, size, data);
    
    checkGLError("updateBuffer");
}

void OpenGLRenderer::bindVertexBuffer(BufferId bufferId) {
    auto it = m_buffers.find(bufferId);
    if (it == m_buffers.end()) return;
    
    glBindBuffer(GL_ARRAY_BUFFER, it->second.glHandle);
}

void OpenGLRenderer::bindIndexBuffer(BufferId bufferId) {
    auto it = m_buffers.find(bufferId);
    if (it == m_buffers.end()) return;
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.glHandle);
}

void OpenGLRenderer::deleteBuffer(BufferId bufferId) {
    auto it = m_buffers.find(bufferId);
    if (it == m_buffers.end()) return;
    
    glDeleteBuffers(1, &it->second.glHandle);
    m_buffers.erase(it);
}

uint32_t OpenGLRenderer::createVertexArray() {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    return vao;
}

void OpenGLRenderer::bindVertexArray(uint32_t vaoId) {
    glBindVertexArray(vaoId);
}

void OpenGLRenderer::deleteVertexArray(uint32_t vaoId) {
    glDeleteVertexArrays(1, &vaoId);
}

void OpenGLRenderer::setupVertexAttributes(const std::vector<VertexAttribute>& attributes) {
    // Standard vertex attribute layout
    const struct {
        VertexAttribute attr;
        GLuint location;
        GLint size;
        GLenum type;
        GLboolean normalized;
        GLsizei stride;
        size_t offset;
    } standardLayout[] = {
        { VertexAttribute::Position, 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position) },
        { VertexAttribute::Normal, 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal) },
        { VertexAttribute::TexCoord0, 2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texCoords) },
        { VertexAttribute::Color, 3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, color) }
    };
    
    for (const auto& layout : standardLayout) {
        for (const auto& attr : attributes) {
            if (attr == layout.attr) {
                glEnableVertexAttribArray(layout.location);
                glVertexAttribPointer(layout.location, layout.size, layout.type, 
                                      layout.normalized, layout.stride, 
                                      reinterpret_cast<const void*>(layout.offset));
            }
        }
    }
    
    checkGLError("setupVertexAttributes");
}

void OpenGLRenderer::useProgram(ShaderId programId) {
    auto it = m_programs.find(programId);
    if (it == m_programs.end()) return;
    
    glUseProgram(it->second.glHandle);
}

void OpenGLRenderer::setUniform(const std::string& name, const UniformValue& value) {
    GLint location;
    glGetIntegerv(GL_CURRENT_PROGRAM, &location);
    if (location == 0) return;
    
    GLuint program = static_cast<GLuint>(location);
    location = glGetUniformLocation(program, name.c_str());
    if (location == -1) return;
    
    switch (value.type) {
        case UniformValue::Float:
            glUniform1f(location, value.data.f);
            break;
        case UniformValue::Vec2:
            glUniform2fv(location, 1, value.data.vec2);
            break;
        case UniformValue::Vec3:
            glUniform3fv(location, 1, value.data.vec3);
            break;
        case UniformValue::Vec4:
            glUniform4fv(location, 1, value.data.vec4);
            break;
        case UniformValue::Int:
            glUniform1i(location, value.data.i);
            break;
        case UniformValue::IVec2:
            glUniform2iv(location, 1, value.data.ivec2);
            break;
        case UniformValue::IVec3:
            glUniform3iv(location, 1, value.data.ivec3);
            break;
        case UniformValue::IVec4:
            glUniform4iv(location, 1, value.data.ivec4);
            break;
        case UniformValue::Mat3:
            glUniformMatrix3fv(location, 1, GL_FALSE, value.data.mat3);
            break;
        case UniformValue::Mat4:
            glUniformMatrix4fv(location, 1, GL_FALSE, value.data.mat4);
            break;
        case UniformValue::Sampler2D:
            glUniform1i(location, value.data.sampler);
            break;
    }
}

void OpenGLRenderer::drawElements(PrimitiveType type, int count, IndexType indexType, int offset) {
    GLenum glPrimType = translatePrimitiveType(type);
    GLenum glIndexType = translateIndexType(indexType);
    
    glDrawElements(glPrimType, count, glIndexType, reinterpret_cast<const void*>(static_cast<size_t>(offset)));
    
    checkGLError("drawElements");
}

void OpenGLRenderer::clear(ClearFlags flags, const Color& color, float depth, int stencil) {
    glClearColor(color.r, color.g, color.b, color.a);
    
    GLbitfield clearMask = 0;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ClearFlags::COLOR)) clearMask |= GL_COLOR_BUFFER_BIT;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ClearFlags::DEPTH)) clearMask |= GL_DEPTH_BUFFER_BIT;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ClearFlags::STENCIL)) clearMask |= GL_STENCIL_BUFFER_BIT;
    
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ClearFlags::DEPTH)) {
        glClearDepth(depth);
    }
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ClearFlags::STENCIL)) {
        glClearStencil(stencil);
    }
    
    glClear(clearMask);
}

void OpenGLRenderer::setClearColor(const Color& color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLRenderer::setViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

// Shader operations
ShaderId OpenGLRenderer::createShader(ShaderType type, const std::string& source) {
    ShaderId id = m_nextShaderId++;
    ShaderInfo info;
    info.id = id;
    info.type = type;
    info.source = source;
    
    info.glHandle = glCreateShader(translateShaderType(type));
    const char* src = source.c_str();
    glShaderSource(info.glHandle, 1, &src, nullptr);
    
    compileShaderInternal(info);
    
    m_shaders[id] = info;
    return id;
}

ShaderId OpenGLRenderer::createProgram(const std::vector<ShaderId>& shaders) {
    ShaderId id = m_nextShaderId++;
    ProgramInfo info;
    info.id = id;
    info.shaders = shaders;
    
    info.glHandle = glCreateProgram();
    
    // Attach all shaders
    for (ShaderId shaderId : shaders) {
        auto it = m_shaders.find(shaderId);
        if (it != m_shaders.end() && it->second.compiled) {
            glAttachShader(info.glHandle, it->second.glHandle);
        }
    }
    
    linkProgramInternal(info);
    
    // Detach shaders after linking
    for (ShaderId shaderId : shaders) {
        auto it = m_shaders.find(shaderId);
        if (it != m_shaders.end()) {
            glDetachShader(info.glHandle, it->second.glHandle);
        }
    }
    
    m_programs[id] = info;
    return id;
}

void OpenGLRenderer::deleteShader(ShaderId shaderId) {
    auto it = m_shaders.find(shaderId);
    if (it == m_shaders.end()) return;
    
    glDeleteShader(it->second.glHandle);
    m_shaders.erase(it);
}

void OpenGLRenderer::deleteProgram(ShaderId programId) {
    auto it = m_programs.find(programId);
    if (it == m_programs.end()) return;
    
    glDeleteProgram(it->second.glHandle);
    m_programs.erase(it);
}

void OpenGLRenderer::setUniform(ShaderId programId, const std::string& name, const UniformValue& value) {
    auto it = m_programs.find(programId);
    if (it == m_programs.end()) return;
    
    GLuint oldProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&oldProgram));
    
    glUseProgram(it->second.glHandle);
    setUniform(name, value);
    
    glUseProgram(oldProgram);
}

int OpenGLRenderer::getUniformLocation(ShaderId programId, const std::string& name) {
    auto it = m_programs.find(programId);
    if (it == m_programs.end()) return -1;
    
    auto locIt = it->second.uniformLocations.find(name);
    if (locIt != it->second.uniformLocations.end()) {
        return locIt->second;
    }
    
    int location = glGetUniformLocation(it->second.glHandle, name.c_str());
    it->second.uniformLocations[name] = location;
    return location;
}

// Drawing operations
void OpenGLRenderer::drawArrays(PrimitiveType type, int first, int count) {
    glDrawArrays(translatePrimitiveType(type), first, count);
    checkGLError("drawArrays");
}

void OpenGLRenderer::drawElementsInstanced(PrimitiveType type, int count, int instanceCount, IndexType indexType) {
    glDrawElementsInstanced(translatePrimitiveType(type), count, translateIndexType(indexType), nullptr, instanceCount);
    checkGLError("drawElementsInstanced");
}

// State management
void OpenGLRenderer::setDepthTest(bool enabled) {
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void OpenGLRenderer::setDepthWrite(bool enabled) {
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}

void OpenGLRenderer::setBlending(bool enabled, BlendMode mode) {
    if (enabled) {
        glEnable(GL_BLEND);
        GLenum srcFactor, dstFactor;
        translateBlendMode(mode, srcFactor, dstFactor);
        glBlendFunc(srcFactor, dstFactor);
    } else {
        glDisable(GL_BLEND);
    }
}

void OpenGLRenderer::setCulling(bool enabled, CullMode mode) {
    if (enabled) {
        glEnable(GL_CULL_FACE);
        glCullFace(translateCullMode(mode));
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void OpenGLRenderer::setPolygonMode(bool wireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
}

void OpenGLRenderer::setLineWidth(float width) {
    glLineWidth(width);
}

void OpenGLRenderer::setPointSize(float size) {
    glPointSize(size);
}

// Resource queries
const BufferInfo* OpenGLRenderer::getBufferInfo(BufferId bufferId) const {
    auto it = m_buffers.find(bufferId);
    return it != m_buffers.end() ? &it->second : nullptr;
}

const TextureInfo* OpenGLRenderer::getTextureInfo(TextureId textureId) const {
    auto it = m_textures.find(textureId);
    return it != m_textures.end() ? &it->second : nullptr;
}

const ShaderInfo* OpenGLRenderer::getShaderInfo(ShaderId shaderId) const {
    auto it = m_shaders.find(shaderId);
    return it != m_shaders.end() ? &it->second : nullptr;
}

const ProgramInfo* OpenGLRenderer::getProgramInfo(ShaderId programId) const {
    auto it = m_programs.find(programId);
    return it != m_programs.end() ? &it->second : nullptr;
}

// Memory statistics
size_t OpenGLRenderer::getTotalBufferMemory() const {
    size_t total = 0;
    for (const auto& pair : m_buffers) {
        total += pair.second.size;
    }
    return total;
}

size_t OpenGLRenderer::getTotalTextureMemory() const {
    size_t total = 0;
    for (const auto& pair : m_textures) {
        total += pair.second.memorySize;
    }
    return total;
}

// Helper methods
uint32_t OpenGLRenderer::translateBufferUsage(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Static: return GL_STATIC_DRAW;
        case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream: return GL_STREAM_DRAW;
        default: return GL_STATIC_DRAW;
    }
}

uint32_t OpenGLRenderer::translatePrimitiveType(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Triangles: return GL_TRIANGLES;
        case PrimitiveType::Lines: return GL_LINES;
        case PrimitiveType::Points: return GL_POINTS;
        case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveType::LineStrip: return GL_LINE_STRIP;
        default: return GL_TRIANGLES;
    }
}

uint32_t OpenGLRenderer::translateIndexType(IndexType type) {
    switch (type) {
        case IndexType::UInt16: return GL_UNSIGNED_SHORT;
        case IndexType::UInt32: return GL_UNSIGNED_INT;
        default: return GL_UNSIGNED_INT;
    }
}

uint32_t OpenGLRenderer::translateShaderType(ShaderType type) {
    switch (type) {
        case ShaderType::Vertex: return GL_VERTEX_SHADER;
        case ShaderType::Fragment: return GL_FRAGMENT_SHADER;
        case ShaderType::Geometry: return GL_GEOMETRY_SHADER;
        case ShaderType::Compute: return GL_COMPUTE_SHADER;
        default: return GL_VERTEX_SHADER;
    }
}

uint32_t OpenGLRenderer::translateBlendMode(BlendMode mode, uint32_t& srcFactor, uint32_t& dstFactor) {
    switch (mode) {
        case BlendMode::Opaque:
            srcFactor = GL_ONE;
            dstFactor = GL_ZERO;
            break;
        case BlendMode::Alpha:
            srcFactor = GL_SRC_ALPHA;
            dstFactor = GL_ONE_MINUS_SRC_ALPHA;
            break;
        case BlendMode::Additive:
            srcFactor = GL_ONE;
            dstFactor = GL_ONE;
            break;
        case BlendMode::Multiply:
            srcFactor = GL_DST_COLOR;
            dstFactor = GL_ZERO;
            break;
        default:
            srcFactor = GL_ONE;
            dstFactor = GL_ZERO;
            break;
    }
    return 0;
}

uint32_t OpenGLRenderer::translateCullMode(CullMode mode) {
    switch (mode) {
        case CullMode::None: return GL_NONE;
        case CullMode::Front: return GL_FRONT;
        case CullMode::Back: return GL_BACK;
        default: return GL_BACK;
    }
}

void OpenGLRenderer::queryCapabilities() {
    // Check for anisotropic filtering
    m_supportsAnisotropicFiltering = false;
    const GLubyte* extensions = glGetString(GL_EXTENSIONS);
    if (extensions) {
        std::string extStr(reinterpret_cast<const char*>(extensions));
        m_supportsAnisotropicFiltering = extStr.find("GL_EXT_texture_filter_anisotropic") != std::string::npos;
    }
    
    if (m_supportsAnisotropicFiltering) {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &m_maxAnisotropy);
    }
    
    // Check for debug output
    GLint contextFlags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
    m_supportsDebugOutput = (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0;
    
    // Check for timestamp queries
    m_supportsTimestampQueries = false; // Requires GL_ARB_timer_query
    
    // Query limits
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_maxTextureUnits);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_maxVertexAttributes);
}

void OpenGLRenderer::setupDebugOutput() {
    if (!m_supportsDebugOutput) return;
    
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(debugCallback), this);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
}

bool OpenGLRenderer::compileShaderInternal(ShaderInfo& info) {
    glCompileShader(info.glHandle);
    
    GLint status;
    glGetShaderiv(info.glHandle, GL_COMPILE_STATUS, &status);
    info.compiled = (status == GL_TRUE);
    
    GLint logLength;
    glGetShaderiv(info.glHandle, GL_INFO_LOG_LENGTH, &logLength);
    
    if (logLength > 1) {
        std::vector<char> log(logLength);
        glGetShaderInfoLog(info.glHandle, logLength, nullptr, log.data());
        info.errorLog = std::string(log.data());
    }
    
    return info.compiled;
}

bool OpenGLRenderer::linkProgramInternal(ProgramInfo& info) {
    glLinkProgram(info.glHandle);
    
    GLint status;
    glGetProgramiv(info.glHandle, GL_LINK_STATUS, &status);
    info.linked = (status == GL_TRUE);
    
    GLint logLength;
    glGetProgramiv(info.glHandle, GL_INFO_LOG_LENGTH, &logLength);
    
    if (logLength > 1) {
        std::vector<char> log(logLength);
        glGetProgramInfoLog(info.glHandle, logLength, nullptr, log.data());
        info.errorLog = std::string(log.data());
    }
    
    if (info.linked) {
        updateUniformLocations(info);
    }
    
    return info.linked;
}

void OpenGLRenderer::updateUniformLocations(ProgramInfo& info) {
    info.uniformLocations.clear();
    
    GLint numUniforms;
    glGetProgramiv(info.glHandle, GL_ACTIVE_UNIFORMS, &numUniforms);
    
    for (GLint i = 0; i < numUniforms; ++i) {
        char name[256];
        GLsizei length;
        GLint size;
        GLenum type;
        
        glGetActiveUniform(info.glHandle, i, sizeof(name), &length, &size, &type, name);
        
        GLint location = glGetUniformLocation(info.glHandle, name);
        if (location != -1) {
            info.uniformLocations[std::string(name)] = location;
        }
    }
}

bool OpenGLRenderer::checkGLError(const std::string& operation) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) return false;
    
    std::cerr << "OpenGL Error in " << operation << ": ";
    switch (error) {
        case GL_INVALID_ENUM: std::cerr << "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE: std::cerr << "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: std::cerr << "GL_INVALID_OPERATION"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        case GL_OUT_OF_MEMORY: std::cerr << "GL_OUT_OF_MEMORY"; break;
        default: std::cerr << "Unknown error " << error; break;
    }
    std::cerr << std::endl;
    
    return true;
}

// Texture operations
TextureId OpenGLRenderer::createTexture2D(int width, int height, TextureFormat format, const void* data) {
    TextureId id = m_nextTextureId++;
    TextureInfo info;
    info.id = id;
    info.format = format;
    info.width = width;
    info.height = height;
    info.memorySize = calculateTextureMemory(width, height, format);
    
    glGenTextures(1, &info.glHandle);
    glBindTexture(GL_TEXTURE_2D, info.glHandle);
    
    uint32_t internalFormat, glFormat, glType;
    translateTextureFormat(format, internalFormat, glType);
    glFormat = internalFormat; // Simplified for now
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, glFormat, glType, data);
    
    // Default texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    checkGLError("createTexture2D");
    
    m_textures[id] = info;
    return id;
}

TextureId OpenGLRenderer::createTextureCube(int size, TextureFormat format, const void* data[6]) {
    TextureId id = m_nextTextureId++;
    TextureInfo info;
    info.id = id;
    info.format = format;
    info.width = size;
    info.height = size;
    info.memorySize = calculateTextureMemory(size, size, format) * 6;
    
    glGenTextures(1, &info.glHandle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, info.glHandle);
    
    uint32_t internalFormat, glFormat, glType;
    translateTextureFormat(format, internalFormat, glType);
    glFormat = internalFormat; // Simplified for now
    
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, 
                     size, size, 0, glFormat, glType, data ? data[i] : nullptr);
    }
    
    // Default cube map parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    checkGLError("createTextureCube");
    
    m_textures[id] = info;
    return id;
}

void OpenGLRenderer::updateTexture(TextureId textureId, int x, int y, int width, int height, const void* data) {
    auto it = m_textures.find(textureId);
    if (it == m_textures.end()) return;
    
    const TextureInfo& info = it->second;
    glBindTexture(GL_TEXTURE_2D, info.glHandle);
    
    uint32_t internalFormat, glFormat, glType;
    translateTextureFormat(info.format, internalFormat, glType);
    glFormat = internalFormat; // Simplified for now
    
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, glFormat, glType, data);
    
    checkGLError("updateTexture");
}

void OpenGLRenderer::bindTexture(TextureId textureId, int slot) {
    auto it = m_textures.find(textureId);
    if (it == m_textures.end()) return;
    
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, it->second.glHandle);
}

void OpenGLRenderer::deleteTexture(TextureId textureId) {
    auto it = m_textures.find(textureId);
    if (it == m_textures.end()) return;
    
    glDeleteTextures(1, &it->second.glHandle);
    m_textures.erase(it);
}

void OpenGLRenderer::generateMipmaps(TextureId textureId) {
    auto it = m_textures.find(textureId);
    if (it == m_textures.end()) return;
    
    glBindTexture(GL_TEXTURE_2D, it->second.glHandle);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void OpenGLRenderer::setTextureParameters(TextureId textureId, bool mipmapping, float anisotropy) {
    auto it = m_textures.find(textureId);
    if (it == m_textures.end()) return;
    
    glBindTexture(GL_TEXTURE_2D, it->second.glHandle);
    
    if (mipmapping) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        generateMipmaps(textureId);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    
    if (m_supportsAnisotropicFiltering && anisotropy > 1.0f) {
        float maxAniso = std::min(anisotropy, m_maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
    }
}

// Debug operations
void OpenGLRenderer::pushDebugGroup(const std::string& name) {
    if (m_supportsDebugOutput) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
    }
}

void OpenGLRenderer::popDebugGroup() {
    if (m_supportsDebugOutput) {
        glPopDebugGroup();
    }
}

void OpenGLRenderer::setObjectLabel(uint32_t glHandle, const std::string& label) {
    if (m_supportsDebugOutput) {
        // Note: This is simplified - in real code you'd need to know the object type
        glObjectLabel(GL_BUFFER, glHandle, -1, label.c_str());
    }
}

// Performance monitoring stubs
void OpenGLRenderer::beginTimestampQuery(const std::string& name) {
    // TODO: Implement with GL_ARB_timer_query
}

void OpenGLRenderer::endTimestampQuery(const std::string& name) {
    // TODO: Implement with GL_ARB_timer_query
}

float OpenGLRenderer::getQueryTime(const std::string& name) const {
    // TODO: Implement with GL_ARB_timer_query
    return 0.0f;
}

// Helper methods
uint32_t OpenGLRenderer::translateTextureFormat(TextureFormat format, uint32_t& internalFormat, uint32_t& type) {
    switch (format) {
        case TextureFormat::RGB8:
            internalFormat = GL_RGB8;
            type = GL_UNSIGNED_BYTE;
            return GL_RGB;
        case TextureFormat::RGBA8:
            internalFormat = GL_RGBA8;
            type = GL_UNSIGNED_BYTE;
            return GL_RGBA;
        case TextureFormat::Depth24Stencil8:
            internalFormat = GL_DEPTH24_STENCIL8;
            type = GL_UNSIGNED_INT_24_8;
            return GL_DEPTH_STENCIL;
        case TextureFormat::R32F:
            internalFormat = GL_R32F;
            type = GL_FLOAT;
            return GL_RED;
        case TextureFormat::RGB32F:
            internalFormat = GL_RGB32F;
            type = GL_FLOAT;
            return GL_RGB;
        case TextureFormat::RGBA32F:
            internalFormat = GL_RGBA32F;
            type = GL_FLOAT;
            return GL_RGBA;
        default:
            internalFormat = GL_RGBA8;
            type = GL_UNSIGNED_BYTE;
            return GL_RGBA;
    }
}

size_t OpenGLRenderer::calculateTextureMemory(int width, int height, TextureFormat format) {
    size_t bitsPerPixel = 0;
    switch (format) {
        case TextureFormat::RGB8: bitsPerPixel = 24; break;
        case TextureFormat::RGBA8: bitsPerPixel = 32; break;
        case TextureFormat::Depth24Stencil8: bitsPerPixel = 32; break;
        case TextureFormat::R32F: bitsPerPixel = 32; break;
        case TextureFormat::RGB32F: bitsPerPixel = 96; break;
        case TextureFormat::RGBA32F: bitsPerPixel = 128; break;
        default: bitsPerPixel = 32; break;
    }
    return (width * height * bitsPerPixel) / 8;
}

} // namespace Rendering
} // namespace VoxelEditor