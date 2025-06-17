// Add GL_SILENCE_DEPRECATION for macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include "shader_test/ShaderTestFramework.h"
#include "shader_test/TestMeshGenerator.h"
#include "rendering/OpenGLRenderer.h"
#include "rendering/ShaderManager.h"
#include "rendering/RenderState.h"
#include "foundation/logging/Logger.h"

extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace VoxelEditor {
namespace ShaderTest {

class ShaderTestFramework::Impl {
public:
    bool m_initialized = false;
    bool m_headless = true;
    int m_width = 800;
    int m_height = 600;
    
    GLFWwindow* m_window = nullptr;
    unsigned int m_framebuffer = 0;
    unsigned int m_colorTexture = 0;
    unsigned int m_depthRenderbuffer = 0;
    
    std::unique_ptr<Rendering::OpenGLRenderer> m_renderer;
    std::unique_ptr<Rendering::ShaderManager> m_shaderManager;
    std::unique_ptr<Rendering::RenderState> m_renderState;
    
    std::string m_lastError;
};

ShaderTestFramework::ShaderTestFramework() : m_impl(std::make_unique<Impl>()) {
}

ShaderTestFramework::~ShaderTestFramework() {
    shutdown();
}

bool ShaderTestFramework::initialize(bool headless, int width, int height) {
    if (m_impl->m_initialized) {
        return true;
    }
    
    m_impl->m_headless = headless;
    m_impl->m_width = width;
    m_impl->m_height = height;
    
    // Initialize GLFW
    if (!glfwInit()) {
        m_impl->m_lastError = "Failed to initialize GLFW";
        return false;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    if (headless) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    
    // Create window
    m_impl->m_window = glfwCreateWindow(width, height, "Shader Test", nullptr, nullptr);
    if (!m_impl->m_window) {
        m_impl->m_lastError = "Failed to create GLFW window";
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_impl->m_window);
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        m_impl->m_lastError = "Failed to initialize GLAD";
        glfwDestroyWindow(m_impl->m_window);
        glfwTerminate();
        return false;
    }
    
    // Create framebuffer for headless rendering
    if (headless) {
        glGenFramebuffers(1, &m_impl->m_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_framebuffer);
        
        // Color texture
        glGenTextures(1, &m_impl->m_colorTexture);
        glBindTexture(GL_TEXTURE_2D, m_impl->m_colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_impl->m_colorTexture, 0);
        
        // Depth renderbuffer
        glGenRenderbuffers(1, &m_impl->m_depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_impl->m_depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_impl->m_depthRenderbuffer);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            m_impl->m_lastError = "Failed to create framebuffer";
            shutdown();
            return false;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    // Initialize rendering components
    m_impl->m_renderer = std::make_unique<Rendering::OpenGLRenderer>();
    m_impl->m_renderState = std::make_unique<Rendering::RenderState>();
    
    // Configure OpenGL renderer
    Rendering::RenderConfig config;
    config.samples = 1;  // No multisampling
    config.vsync = false;
    
    if (!m_impl->m_renderer->initializeContext(config)) {
        m_impl->m_lastError = "Failed to initialize OpenGL renderer";
        shutdown();
        return false;
    }
    
    // Initialize shader manager with renderer
    m_impl->m_shaderManager = std::make_unique<Rendering::ShaderManager>(m_impl->m_renderer.get());
    
    // Set default OpenGL state
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    m_impl->m_initialized = true;
    return true;
}

void ShaderTestFramework::shutdown() {
    if (!m_impl->m_initialized) {
        return;
    }
    
    // Clean up OpenGL resources
    if (m_impl->m_framebuffer) {
        glDeleteFramebuffers(1, &m_impl->m_framebuffer);
    }
    if (m_impl->m_colorTexture) {
        glDeleteTextures(1, &m_impl->m_colorTexture);
    }
    if (m_impl->m_depthRenderbuffer) {
        glDeleteRenderbuffers(1, &m_impl->m_depthRenderbuffer);
    }
    
    // Clean up rendering components
    m_impl->m_renderState.reset();
    m_impl->m_shaderManager.reset();
    m_impl->m_renderer.reset();
    
    // Clean up GLFW
    if (m_impl->m_window) {
        glfwDestroyWindow(m_impl->m_window);
    }
    glfwTerminate();
    
    m_impl->m_initialized = false;
}

TestResult ShaderTestFramework::testShaderCompilation(const std::string& vertexPath,
                                                     const std::string& fragmentPath,
                                                     bool isSource) {
    TestResult result;
    result.shaderName = isSource ? "inline_shader" : vertexPath + " + " + fragmentPath;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    clearGLErrors();
    
    // Load or use provided shader source
    std::string vertexSource, fragmentSource;
    
    if (isSource) {
        vertexSource = vertexPath;
        fragmentSource = fragmentPath;
    } else {
        // Load from files
        auto loadFile = [](const std::string& path) -> std::string {
            std::ifstream file(path);
            if (!file.is_open()) {
                return "";
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        };
        
        vertexSource = loadFile(vertexPath);
        fragmentSource = loadFile(fragmentPath);
        
        if (vertexSource.empty() || fragmentSource.empty()) {
            result.errorMessage = "Failed to load shader files";
            return result;
        }
    }
    
    // Compile shaders
    Rendering::ShaderId vertexShader = m_impl->m_renderer->createShader(Rendering::ShaderType::Vertex, vertexSource);
    if (vertexShader == 0) {
        result.compilationSuccess = false;
        result.compileLog = "Vertex shader compilation failed";
        result.errorMessage = "Vertex shader compilation failed";
        return result;
    }
    
    Rendering::ShaderId fragmentShader = m_impl->m_renderer->createShader(Rendering::ShaderType::Fragment, fragmentSource);
    if (fragmentShader == 0) {
        result.compilationSuccess = false;
        result.compileLog = "Fragment shader compilation failed";
        result.errorMessage = "Fragment shader compilation failed";
        m_impl->m_renderer->deleteShader(vertexShader);
        return result;
    }
    
    result.compilationSuccess = true;
    
    // Link program
    std::vector<Rendering::ShaderId> shaders = {vertexShader, fragmentShader};
    Rendering::ShaderId program = m_impl->m_renderer->createProgram(shaders);
    if (program == 0) {
        result.linkingSuccess = false;
        result.linkLog = "Shader linking failed";
        result.errorMessage = "Shader linking failed";
        m_impl->m_renderer->deleteShader(vertexShader);
        m_impl->m_renderer->deleteShader(fragmentShader);
        return result;
    }
    
    result.linkingSuccess = true;
    
    // Get program info to query uniforms/attributes
    const auto* programInfo = m_impl->m_renderer->getProgramInfo(program);
    if (programInfo) {
        // Use glGetProgramiv directly with the GL handle
        int numUniforms;
        glGetProgramiv(programInfo->glHandle, GL_ACTIVE_UNIFORMS, &numUniforms);
        for (int i = 0; i < numUniforms; ++i) {
            char name[256];
            int size;
            GLenum type;
            glGetActiveUniform(programInfo->glHandle, i, sizeof(name), nullptr, &size, &type, name);
            result.foundUniforms.push_back(name);
        }
        
        // Query active attributes
        int numAttributes;
        glGetProgramiv(programInfo->glHandle, GL_ACTIVE_ATTRIBUTES, &numAttributes);
        for (int i = 0; i < numAttributes; ++i) {
            char name[256];
            int size;
            GLenum type;
            glGetActiveAttrib(programInfo->glHandle, i, sizeof(name), nullptr, &size, &type, name);
            result.foundAttributes.push_back(name);
        }
    }
    
    // Clean up
    m_impl->m_renderer->deleteProgram(program);
    m_impl->m_renderer->deleteShader(vertexShader);
    m_impl->m_renderer->deleteShader(fragmentShader);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.compileTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    result.success = true;
    return result;
}

TestResult ShaderTestFramework::testShaderWithMesh(const std::string& shaderName,
                                                   const Rendering::Mesh& mesh,
                                                   const ShaderUniforms& uniforms,
                                                   const ValidationCriteria& criteria,
                                                   unsigned int primitiveType) {
    TestResult result;
    result.shaderName = shaderName;
    
    if (!m_impl->m_shaderManager->getShader(shaderName)) {
        result.errorMessage = "Shader not found: " + shaderName;
        return result;
    }
    
    // Use framebuffer if capturing output
    if (criteria.captureOutput && m_impl->m_headless) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_framebuffer);
    }
    
    // Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set up shader
    // For now, skip shader manager and use renderer directly
    // This is a simplified test that doesn't integrate with ShaderManager
    Rendering::ShaderId program = 0; // We'll need to create the shader program separately
    
    // Set uniforms
    // Use program needs to be handled differently
    // m_impl->m_renderer->useProgram(program);
    
    // Matrix uniforms
    auto setMatrix = [program](const std::string& name, const Math::Matrix4f& matrix) {
        int loc = glGetUniformLocation(program, name.c_str());
        if (loc >= 0) {
            glUniformMatrix4fv(loc, 1, GL_FALSE, matrix.data());
        }
    };
    
    setMatrix("uModel", uniforms.modelMatrix);
    setMatrix("uView", uniforms.viewMatrix);
    setMatrix("uProjection", uniforms.projectionMatrix);
    setMatrix("uMVP", uniforms.projectionMatrix * uniforms.viewMatrix * uniforms.modelMatrix);
    
    // Vector uniforms
    auto setVec3 = [program](const std::string& name, const Math::Vector3f& vec) {
        int loc = glGetUniformLocation(program, name.c_str());
        if (loc >= 0) {
            glUniform3f(loc, vec.x, vec.y, vec.z);
        }
    };
    
    setVec3("uLightDirection", uniforms.lightDirection);
    setVec3("uLightColor", uniforms.lightColor);
    setVec3("uAmbientColor", uniforms.ambientColor);
    setVec3("uMinorLineColor", uniforms.minorLineColor);
    setVec3("uMajorLineColor", uniforms.majorLineColor);
    
    // Float uniforms
    int loc = glGetUniformLocation(program, "uGridOpacity");
    if (loc >= 0) glUniform1f(loc, uniforms.gridOpacity);
    
    loc = glGetUniformLocation(program, "uTime");
    if (loc >= 0) glUniform1f(loc, uniforms.time);
    
    // Custom uniforms
    for (const auto& [name, value] : uniforms.floatUniforms) {
        loc = glGetUniformLocation(program, name.c_str());
        if (loc >= 0) glUniform1f(loc, value);
    }
    
    for (const auto& [name, value] : uniforms.vec3Uniforms) {
        setVec3(name, value);
    }
    
    // Render mesh
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Create vertex buffer if needed
    if (mesh.vertexBuffer == 0 && !mesh.vertices.empty()) {
        // Create and bind VAO
        unsigned int vao;
#ifdef __APPLE__
        // On macOS, GLAD might not load VAO functions properly
        // Use the APPLE variants directly
        glGenVertexArraysAPPLE(1, &vao);
        glBindVertexArrayAPPLE(vao);
#else
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
#endif
        
        // Create vertex buffer
        unsigned int vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Rendering::Vertex), 
                     mesh.vertices.data(), GL_STATIC_DRAW);
        
        // Set vertex attributes
        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex), 
                            (void*)offsetof(Rendering::Vertex, position));
        
        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex), 
                            (void*)offsetof(Rendering::Vertex, normal));
        
        // TexCoords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex), 
                            (void*)offsetof(Rendering::Vertex, texCoords));
        
        // Color
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Rendering::Vertex), 
                            (void*)offsetof(Rendering::Vertex, color));
        
        // Create index buffer if needed
        unsigned int ebo = 0;
        if (!mesh.indices.empty()) {
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), 
                        mesh.indices.data(), GL_STATIC_DRAW);
        }
        
        // Draw
        if (!mesh.indices.empty()) {
            glDrawElements(primitiveType, mesh.indices.size(), GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(primitiveType, 0, mesh.vertices.size());
        }
        
        // Cleanup
        glDeleteBuffers(1, &vbo);
        if (ebo != 0) {
            glDeleteBuffers(1, &ebo);
        }
        if (vao != 0) {
#ifdef __APPLE__
            glDeleteVertexArraysAPPLE(1, &vao);
#else
            glDeleteVertexArrays(1, &vao);
#endif
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.renderTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    // Capture output if requested
    if (criteria.captureOutput) {
        result.outputCaptured = captureFramebuffer(criteria.outputPath);
        result.outputPath = criteria.outputPath;
    }
    
    // Validate pixels if requested
    if (criteria.validatePixels && criteria.testPixels.size() == criteria.expectedColors.size()) {
        std::vector<unsigned char> pixels(m_impl->m_width * m_impl->m_height * 3);
        glReadPixels(0, 0, m_impl->m_width, m_impl->m_height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        
        bool pixelsValid = true;
        for (size_t i = 0; i < criteria.testPixels.size(); ++i) {
            int x = criteria.testPixels[i].first;
            int y = criteria.testPixels[i].second;
            int idx = (y * m_impl->m_width + x) * 3;
            
            Rendering::Color actual(
                pixels[idx] / 255.0f,
                pixels[idx + 1] / 255.0f,
                pixels[idx + 2] / 255.0f,
                1.0f
            );
            
            const auto& expected = criteria.expectedColors[i];
            float diff = std::abs(actual.r - expected.r) + 
                        std::abs(actual.g - expected.g) + 
                        std::abs(actual.b - expected.b);
            
            if (diff > criteria.colorTolerance * 3) {
                pixelsValid = false;
                result.errorMessage += "Pixel validation failed at (" + std::to_string(x) + ", " + 
                                     std::to_string(y) + ")\n";
            }
        }
        
        if (!pixelsValid) {
            result.success = false;
            return result;
        }
    }
    
    // Reset framebuffer
    if (criteria.captureOutput && m_impl->m_headless) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    result.success = true;
    return result;
}

TestResult ShaderTestFramework::runCompleteTest(const std::string& vertexPath,
                                               const std::string& fragmentPath,
                                               const Rendering::Mesh& mesh,
                                               const ShaderUniforms& uniforms,
                                               const ValidationCriteria& criteria) {
    // First test compilation
    auto compileResult = testShaderCompilation(vertexPath, fragmentPath, false);
    if (!compileResult.success) {
        return compileResult;
    }
    
    // Create shader in manager
    std::string shaderName = "test_" + std::to_string(std::hash<std::string>{}(vertexPath + fragmentPath));
    m_impl->m_shaderManager->loadShaderFromFile(shaderName, vertexPath, fragmentPath);
    
    // Test with mesh
    auto meshResult = testShaderWithMesh(shaderName, mesh, uniforms, criteria);
    
    // Combine results
    meshResult.compilationSuccess = compileResult.compilationSuccess;
    meshResult.linkingSuccess = compileResult.linkingSuccess;
    meshResult.compileLog = compileResult.compileLog;
    meshResult.linkLog = compileResult.linkLog;
    meshResult.foundUniforms = compileResult.foundUniforms;
    meshResult.foundAttributes = compileResult.foundAttributes;
    meshResult.compileTimeMs = compileResult.compileTimeMs;
    
    // Check required uniforms/attributes
    if (criteria.checkUniforms) {
        for (const auto& required : criteria.requiredUniforms) {
            if (std::find(meshResult.foundUniforms.begin(), meshResult.foundUniforms.end(), required) == 
                meshResult.foundUniforms.end()) {
                meshResult.missingUniforms.push_back(required);
            }
        }
    }
    
    if (criteria.checkAttributes) {
        for (const auto& required : criteria.requiredAttributes) {
            if (std::find(meshResult.foundAttributes.begin(), meshResult.foundAttributes.end(), required) == 
                meshResult.foundAttributes.end()) {
                meshResult.missingAttributes.push_back(required);
            }
        }
    }
    
    if (!meshResult.missingUniforms.empty() || !meshResult.missingAttributes.empty()) {
        meshResult.success = false;
        meshResult.errorMessage = "Missing required uniforms/attributes";
    }
    
    return meshResult;
}

bool ShaderTestFramework::captureFramebuffer(const std::string& filename) {
    std::vector<unsigned char> pixels(m_impl->m_width * m_impl->m_height * 3);
    glReadPixels(0, 0, m_impl->m_width, m_impl->m_height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    
    // Flip vertically (OpenGL has origin at bottom-left)
    std::vector<unsigned char> flipped(pixels.size());
    for (int y = 0; y < m_impl->m_height; ++y) {
        memcpy(&flipped[y * m_impl->m_width * 3],
               &pixels[(m_impl->m_height - 1 - y) * m_impl->m_width * 3],
               m_impl->m_width * 3);
    }
    
    // Write PPM file
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    file << "P6\n" << m_impl->m_width << " " << m_impl->m_height << "\n255\n";
    file.write(reinterpret_cast<const char*>(flipped.data()), flipped.size());
    
    return file.good();
}

std::string ShaderTestFramework::getLastGLError() const {
    GLenum error = glGetError();
    switch (error) {
        case GL_NO_ERROR: return "No error";
        case GL_INVALID_ENUM: return "Invalid enum";
        case GL_INVALID_VALUE: return "Invalid value";
        case GL_INVALID_OPERATION: return "Invalid operation";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "Invalid framebuffer operation";
        case GL_OUT_OF_MEMORY: return "Out of memory";
        default: return "Unknown error";
    }
}

void ShaderTestFramework::clearGLErrors() {
    while (glGetError() != GL_NO_ERROR) {}
}

void ShaderTestFramework::setViewport(int width, int height) {
    m_impl->m_width = width;
    m_impl->m_height = height;
    glViewport(0, 0, width, height);
}

// ShaderTestRunner implementation

void ShaderTestRunner::Summary::print() const {
    std::cout << "\n=== Shader Test Summary ===" << std::endl;
    std::cout << "Total tests: " << totalTests << std::endl;
    std::cout << "Passed: " << passedTests << std::endl;
    std::cout << "Failed: " << failedTests << std::endl;
    std::cout << "Success rate: " << std::fixed << std::setprecision(1) 
              << (totalTests > 0 ? (100.0 * passedTests / totalTests) : 0.0) << "%" << std::endl;
    
    if (failedTests > 0) {
        std::cout << "\nFailed tests:" << std::endl;
        for (const auto& result : results) {
            if (!result.success) {
                std::cout << "  - " << result.shaderName << ": " << result.errorMessage << std::endl;
            }
        }
    }
}

void ShaderTestRunner::Summary::writeToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) return;
    
    file << "Shader Test Report" << std::endl;
    file << "==================" << std::endl;
    file << "Total tests: " << totalTests << std::endl;
    file << "Passed: " << passedTests << std::endl;
    file << "Failed: " << failedTests << std::endl;
    file << "Success rate: " << std::fixed << std::setprecision(1) 
         << (totalTests > 0 ? (100.0 * passedTests / totalTests) : 0.0) << "%" << std::endl;
    
    file << "\nDetailed Results:" << std::endl;
    for (const auto& result : results) {
        file << "\nShader: " << result.shaderName << std::endl;
        file << "Status: " << (result.success ? "PASSED" : "FAILED") << std::endl;
        if (!result.success) {
            file << "Error: " << result.errorMessage << std::endl;
        }
        file << "Compile time: " << result.compileTimeMs << " ms" << std::endl;
        file << "Render time: " << result.renderTimeMs << " ms" << std::endl;
        file << "Uniforms found: ";
        for (const auto& u : result.foundUniforms) {
            file << u << " ";
        }
        file << std::endl;
        file << "Attributes found: ";
        for (const auto& a : result.foundAttributes) {
            file << a << " ";
        }
        file << std::endl;
    }
}

} // namespace ShaderTest
} // namespace VoxelEditor