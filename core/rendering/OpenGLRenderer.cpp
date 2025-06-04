#include "OpenGLRenderer.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// Silence OpenGL deprecation warnings on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include "MacOSGLLoader.h"
#endif

#include <glad/glad.h>

// Define missing GL constants
#ifndef GL_VERTEX_ARRAY_BINDING
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#endif

namespace VoxelEditor {
namespace Rendering {

// Static debug callback
void OpenGLRenderer::debugCallback(uint32_t source, uint32_t type, uint32_t id, 
                         uint32_t severity, int length, 
                         const char* message, const void* userParam) {
    // Suppress unused parameter warnings
    (void)source;
    (void)type;
    (void)id;
    (void)length;
    (void)userParam;
    
    // Simple severity check
    if (severity == 0) return; // Skip notifications
    
    std::cerr << "GL Debug: ";
    
    switch (severity) {
        case 3: std::cerr << "[HIGH] "; break;
        case 2: std::cerr << "[MEDIUM] "; break;
        case 1: std::cerr << "[LOW] "; break;
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
    , m_maxVertexAttributes(16)
    , m_defaultVAO(0) {
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
    
#ifdef __APPLE__
    // Load OpenGL extensions on macOS
    if (!LoadOpenGLExtensions()) {
        std::cerr << "Failed to load OpenGL extensions on macOS" << std::endl;
        return false;
    }
#endif
    
    // Query OpenGL capabilities (skip in test environments)
    try {
        queryCapabilities();
    } catch (...) {
        // Ignore failures in test environments
    }
    
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
    glDepthMask(GL_TRUE);  // Ensure depth writing is enabled
    glDisable(GL_CULL_FACE);   // Keep disabled for debugging
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    // Create and bind a default VAO for OpenGL 3.3 core profile
    // This is REQUIRED on macOS
#ifdef __APPLE__
    if (glGenVertexArrays && glBindVertexArray) {
        glGenVertexArrays(1, &m_defaultVAO);
        if (m_defaultVAO != 0) {
            glBindVertexArray(m_defaultVAO);
            std::cout << "Successfully created and bound default VAO: " << m_defaultVAO << std::endl;
        } else {
            std::cerr << "Failed to create default VAO" << std::endl;
            return false;
        }
    } else {
        std::cerr << "VAO functions not available - cannot continue on macOS" << std::endl;
        return false;
    }
#else
    glGenVertexArrays(1, &m_defaultVAO);
    glBindVertexArray(m_defaultVAO);
#endif
    
    // Ensure depth range is correct
    glDepthRange(0.0, 1.0);
    
    // Disable alpha blending by default
    glDisable(GL_BLEND);
    
    // Disable scissor test
    glDisable(GL_SCISSOR_TEST);
    
    // Enable seamless cubemap filtering
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
    m_contextValid = true;
    
    return true;
}

void OpenGLRenderer::destroyContext() {
    // Clean up default VAO
    if (m_defaultVAO != 0) {
#ifdef __APPLE__
        if (glDeleteVertexArrays) {
            glDeleteVertexArrays(1, &m_defaultVAO);
        }
#else
        glDeleteVertexArrays(1, &m_defaultVAO);
#endif
        m_defaultVAO = 0;
    }
    
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
    uint32_t vao = 0;
#ifdef __APPLE__
    if (glGenVertexArrays) {
        glGenVertexArrays(1, &vao);
    }
#else
    glGenVertexArrays(1, &vao);
#endif
    return vao;
}

void OpenGLRenderer::bindVertexArray(uint32_t vaoId) {
    // If vaoId is 0, bind the default VAO
    if (vaoId == 0) {
        vaoId = m_defaultVAO;
    }
#ifdef __APPLE__
    if (glBindVertexArray) {
        glBindVertexArray(vaoId);
    }
#else
    glBindVertexArray(vaoId);
#endif
}

void OpenGLRenderer::deleteVertexArray(uint32_t vaoId) {
    if (vaoId != 0 && vaoId != m_defaultVAO) {
#ifdef __APPLE__
        if (glDeleteVertexArrays) {
            glDeleteVertexArrays(1, &vaoId);
        }
#else
        glDeleteVertexArrays(1, &vaoId);
#endif
    }
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
    
    // First disable all attributes
    for (int i = 0; i < 4; i++) {
        glDisableVertexAttribArray(i);
    }
    
    // Enable and set up requested attributes
    for (const auto& layout : standardLayout) {
        for (const auto& attr : attributes) {
            if (attr == layout.attr) {
                glEnableVertexAttribArray(layout.location);
                glVertexAttribPointer(layout.location, layout.size, layout.type, 
                                      layout.normalized, layout.stride, 
                                      reinterpret_cast<const void*>(layout.offset));
                
                // Debug output
                static int setupCount = 0;
                if (setupCount < 5) {
                    std::cout << "Enabled attribute " << layout.location << " for " 
                              << (attr == VertexAttribute::Position ? "Position" :
                                  attr == VertexAttribute::Normal ? "Normal" :
                                  attr == VertexAttribute::Color ? "Color" : "Other")
                              << " at offset " << layout.offset << std::endl;
                }
            }
        }
    }
    
    // Static counter removed - was unused
    
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
    
    static int uniformCount = 0;
    if (uniformCount < 20 && name.find("u_") == 0) {
        std::cout << "Setting uniform '" << name << "' at location " << location << std::endl;
        uniformCount++;
    }
    
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
            // Our matrices use row-major order, but OpenGL expects column-major
            glUniformMatrix3fv(location, 1, GL_TRUE, value.data.mat3);
            break;
        case UniformValue::Mat4:
            if (uniformCount < 20 && name == "u_model") {
                std::cout << "  u_model matrix:" << std::endl;
                for (int i = 0; i < 4; i++) {
                    std::cout << "    ";
                    for (int j = 0; j < 4; j++) {
                        std::cout << value.data.mat4[i*4+j] << " ";
                    }
                    std::cout << std::endl;
                }
            }
            // Our Matrix4f uses row-major order, but OpenGL expects column-major
            // So we need to transpose when uploading
            glUniformMatrix4fv(location, 1, GL_TRUE, value.data.mat4);
            break;
        case UniformValue::Sampler2D:
            glUniform1i(location, value.data.sampler);
            break;
    }
}

void OpenGLRenderer::drawElements(PrimitiveType type, int count, IndexType indexType, int offset) {
    GLenum glPrimType = translatePrimitiveType(type);
    GLenum glIndexType = translateIndexType(indexType);
    
    static int drawCount = 0;
    if (drawCount < 5) {
        std::cout << "drawElements called: count=" << count 
                  << ", type=" << glPrimType 
                  << ", offset=" << offset 
                  << " (expected " << count/3 << " triangles)" << std::endl;
        
        // Check current program
        GLint prog;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
        std::cout << "  Current program: " << prog << std::endl;
        
        // Check vertex array and buffers
        GLint vao, vbo, ibo;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ibo);
        std::cout << "  VAO=" << vao << ", VBO=" << vbo << ", IBO=" << ibo << std::endl;
        
        // Check if position attribute is enabled
        GLint posEnabled;
        glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &posEnabled);
        std::cout << "  Position attrib 0 enabled: " << posEnabled << std::endl;
        
        // Check depth test state
        GLboolean depthTest;
        glGetBooleanv(GL_DEPTH_TEST, &depthTest);
        std::cout << "  Depth test: " << (depthTest ? "ON" : "OFF") << std::endl;
        
        // Check blend state
        GLboolean blend;
        glGetBooleanv(GL_BLEND, &blend);
        std::cout << "  Blending: " << (blend ? "ON" : "OFF") << std::endl;
        
        // Check cull face
        GLboolean cullFace;
        glGetBooleanv(GL_CULL_FACE, &cullFace);
        std::cout << "  Face culling: " << (cullFace ? "ON" : "OFF") << std::endl;
        
        drawCount++;
    }
    
    // Always check GL error before
    GLenum errorBefore = glGetError();
    if (errorBefore != GL_NO_ERROR) {
        std::cerr << "GL Error before drawElements: " << errorBefore << std::endl;
    }
    
    glDrawElements(glPrimType, count, glIndexType, reinterpret_cast<const void*>(static_cast<size_t>(offset)));
    
    // Check error after
    GLenum errorAfter = glGetError();
    if (errorAfter != GL_NO_ERROR) {
        std::cerr << "GL Error after drawElements: " << errorAfter << std::endl;
    }
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
    
    // Bind standard attribute locations before linking
    // Try both naming conventions to support different shaders
    glBindAttribLocation(info.glHandle, 0, "a_position");
    glBindAttribLocation(info.glHandle, 0, "aPos");
    glBindAttribLocation(info.glHandle, 1, "a_normal");
    glBindAttribLocation(info.glHandle, 1, "aNormal");
    glBindAttribLocation(info.glHandle, 2, "a_texCoord");
    glBindAttribLocation(info.glHandle, 2, "aColor");  // Note: shader has color at location 2
    glBindAttribLocation(info.glHandle, 3, "a_color");
    
    linkProgramInternal(info);
    
    // Debug: Check actual attribute locations after linking
    if (info.linked) {
        GLint posLoc = glGetAttribLocation(info.glHandle, "a_position");
        GLint normLoc = glGetAttribLocation(info.glHandle, "a_normal");
        GLint colorLoc = glGetAttribLocation(info.glHandle, "a_color");
        
        std::cout << "Shader attribute locations after linking: pos=" << posLoc 
                  << " normal=" << normLoc << " color=" << colorLoc << std::endl;
    }
    
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
#ifndef __APPLE__
        case ShaderType::Geometry: return GL_GEOMETRY_SHADER;
        case ShaderType::Compute: return GL_COMPUTE_SHADER;
#else
        case ShaderType::Geometry: return GL_VERTEX_SHADER; // Not supported on macOS
        case ShaderType::Compute: return GL_VERTEX_SHADER; // Not supported on macOS
#endif
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
    // Check for anisotropic filtering (safely handle missing context)
    m_supportsAnisotropicFiltering = false;
    try {
        const GLubyte* extensions = glGetString(GL_EXTENSIONS);
        if (extensions) {
            std::string extStr(reinterpret_cast<const char*>(extensions));
            m_supportsAnisotropicFiltering = extStr.find("GL_EXT_texture_filter_anisotropic") != std::string::npos;
        }
        
        if (m_supportsAnisotropicFiltering) {
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &m_maxAnisotropy);
        }
    } catch (...) {
        // Ignore OpenGL errors in test environments
        m_supportsAnisotropicFiltering = false;
    }
    
    // Check for debug output
    m_supportsDebugOutput = false; // Debug output requires GL 4.3
#ifndef __APPLE__
    GLint contextFlags = 0;
    // glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags); // GL_CONTEXT_FLAGS requires GL 3.0+
    // m_supportsDebugOutput = (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0;
#endif
    
    // Check for timestamp queries
    m_supportsTimestampQueries = false; // Requires GL_ARB_timer_query
    
    // Query limits
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_maxTextureUnits);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_maxVertexAttributes);
}

void OpenGLRenderer::setupDebugOutput() {
    if (!m_supportsDebugOutput) return;
    
    // Debug output requires GL 4.3 or extension
    // For now, just skip on macOS
#ifndef __APPLE__
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(debugCallback), this);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif
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
        
        if (!info.compiled) {
            std::cerr << "Shader compilation failed: " << info.errorLog << std::endl;
        } else if (!info.errorLog.empty()) {
            std::cerr << "Shader compilation warning: " << info.errorLog << std::endl;
        }
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
        
        if (!info.linked) {
            std::cerr << "Program linking failed: " << info.errorLog << std::endl;
        } else if (!info.errorLog.empty()) {
            std::cerr << "Program linking warning: " << info.errorLog << std::endl;
        }
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
    // Debug groups require GL 4.3 or extension
    (void)name;
#ifndef __APPLE__
    if (m_supportsDebugOutput) {
        // glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
    }
#endif
}

void OpenGLRenderer::popDebugGroup() {
    // Debug groups require GL 4.3 or extension
#ifndef __APPLE__
    if (m_supportsDebugOutput) {
        // glPopDebugGroup();
    }
#endif
}

void OpenGLRenderer::setObjectLabel(uint32_t glHandle, const std::string& label) {
    // Object labels require GL 4.3 or extension
    (void)glHandle;
    (void)label;
#ifndef __APPLE__
    if (m_supportsDebugOutput) {
        // glObjectLabel(GL_BUFFER, glHandle, -1, label.c_str());
    }
#endif
}

// Performance monitoring stubs
void OpenGLRenderer::beginTimestampQuery(const std::string& name) {
    // TODO: Implement with GL_ARB_timer_query
    (void)name;
}

void OpenGLRenderer::endTimestampQuery(const std::string& name) {
    // TODO: Implement with GL_ARB_timer_query
    (void)name;
}

float OpenGLRenderer::getQueryTime(const std::string& name) const {
    // TODO: Implement with GL_ARB_timer_query
    (void)name;
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
            internalFormat = GL_RGB; // GL_RGB32F not available on older GL
            type = GL_FLOAT;
            return GL_RGB;
        case TextureFormat::RGBA32F:
            internalFormat = GL_RGBA; // GL_RGBA32F not available on older GL
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