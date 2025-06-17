#pragma once

#include "rendering/RenderTypes.h"
#include "rendering/ShaderManager.h"
#include "math/Matrix4f.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// Forward declarations
struct GLFWwindow;

namespace VoxelEditor {
namespace ShaderTest {

/**
 * @brief Validation criteria for shader output
 */
struct ValidationCriteria {
    bool checkCompilation = true;
    bool checkLinking = true;
    bool checkUniforms = true;
    bool checkAttributes = true;
    bool captureOutput = false;
    std::string outputPath;
    
    // Expected uniforms
    std::vector<std::string> requiredUniforms;
    
    // Expected attributes  
    std::vector<std::string> requiredAttributes;
    
    // Pixel validation (if captureOutput is true)
    bool validatePixels = false;
    std::vector<std::pair<int, int>> testPixels; // Positions to check
    std::vector<Rendering::Color> expectedColors; // Expected colors
    float colorTolerance = 0.01f;
};

/**
 * @brief Shader uniform data for testing
 */
struct ShaderUniforms {
    Math::Matrix4f modelMatrix;  // Default constructor creates identity
    Math::Matrix4f viewMatrix;
    Math::Matrix4f projectionMatrix;
    
    // Common uniforms
    Math::Vector3f lightDirection = Math::Vector3f(0.0f, -0.707f, -0.707f); // Pre-normalized
    Math::Vector3f lightColor = Math::Vector3f(1.0f, 1.0f, 1.0f);
    Math::Vector3f ambientColor = Math::Vector3f(0.2f, 0.2f, 0.2f);
    
    // Grid-specific uniforms
    Math::Vector3f minorLineColor = Math::Vector3f(0.7f, 0.7f, 0.7f);
    Math::Vector3f majorLineColor = Math::Vector3f(0.8f, 0.8f, 0.8f);
    float gridOpacity = 0.5f;
    
    // Animation uniforms
    float time = 0.0f;
    float phase = 0.0f;
    
    // Custom uniforms
    std::unordered_map<std::string, float> floatUniforms;
    std::unordered_map<std::string, Math::Vector3f> vec3Uniforms;
    std::unordered_map<std::string, Math::Vector4f> vec4Uniforms;
};

/**
 * @brief Test result information
 */
struct TestResult {
    bool success = false;
    std::string shaderName;
    std::string errorMessage;
    
    // Compilation/linking info
    bool compilationSuccess = false;
    bool linkingSuccess = false;
    std::string compileLog;
    std::string linkLog;
    
    // Uniform/attribute info
    std::vector<std::string> foundUniforms;
    std::vector<std::string> foundAttributes;
    std::vector<std::string> missingUniforms;
    std::vector<std::string> missingAttributes;
    
    // Performance info
    double compileTimeMs = 0.0;
    double renderTimeMs = 0.0;
    
    // Output capture
    bool outputCaptured = false;
    std::string outputPath;
};

/**
 * @brief Main shader testing framework
 */
class ShaderTestFramework {
public:
    ShaderTestFramework();
    ~ShaderTestFramework();
    
    /**
     * @brief Initialize the test framework
     * @param headless If true, use offscreen rendering
     * @param width Window/framebuffer width
     * @param height Window/framebuffer height
     * @return True if initialization succeeded
     */
    bool initialize(bool headless = true, int width = 800, int height = 600);
    
    /**
     * @brief Shutdown the framework and release resources
     */
    void shutdown();
    
    /**
     * @brief Test shader compilation and linking
     * @param vertexPath Path to vertex shader or source code
     * @param fragmentPath Path to fragment shader or source code
     * @param isSource If true, paths contain source code rather than file paths
     * @return Test result with compilation details
     */
    TestResult testShaderCompilation(const std::string& vertexPath,
                                    const std::string& fragmentPath,
                                    bool isSource = false);
    
    /**
     * @brief Test shader with a specific mesh and uniforms
     * @param shaderName Name for the shader program
     * @param mesh Test mesh to render
     * @param uniforms Shader uniforms to set
     * @param criteria Validation criteria
     * @return Test result with validation details
     */
    TestResult testShaderWithMesh(const std::string& shaderName,
                                 const Rendering::Mesh& mesh,
                                 const ShaderUniforms& uniforms,
                                 const ValidationCriteria& criteria,
                                 unsigned int primitiveType = 0x0004 /* GL_TRIANGLES */);
    
    /**
     * @brief Run a complete shader test
     * @param vertexPath Vertex shader path
     * @param fragmentPath Fragment shader path
     * @param mesh Test mesh
     * @param uniforms Shader uniforms
     * @param criteria Validation criteria
     * @return Complete test result
     */
    TestResult runCompleteTest(const std::string& vertexPath,
                              const std::string& fragmentPath,
                              const Rendering::Mesh& mesh,
                              const ShaderUniforms& uniforms,
                              const ValidationCriteria& criteria);
    
    /**
     * @brief Capture current framebuffer to file
     * @param filename Output filename (PPM format)
     * @return True if capture succeeded
     */
    bool captureFramebuffer(const std::string& filename);
    
    /**
     * @brief Get last OpenGL error as string
     */
    std::string getLastGLError() const;
    
    /**
     * @brief Clear any OpenGL errors
     */
    void clearGLErrors();
    
    /**
     * @brief Set viewport size
     */
    void setViewport(int width, int height);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Utility to run tests on all shaders in a directory
 */
class ShaderTestRunner {
public:
    /**
     * @brief Test result summary
     */
    struct Summary {
        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;
        std::vector<TestResult> results;
        
        void print() const;
        void writeToFile(const std::string& filename) const;
    };
    
    /**
     * @brief Run tests on all shaders
     * @return Test summary
     */
    static Summary runAllTests();
    
    /**
     * @brief Run tests on built-in shaders
     */
    static Summary testBuiltInShaders();
    
    /**
     * @brief Run tests on file-based shaders
     */
    static Summary testFileShaders();
    
private:
    static void testBasicVoxelShaders(ShaderTestFramework& framework, Summary& summary);
    static void testEnhancedVoxelShaders(ShaderTestFramework& framework, Summary& summary);
    static void testGridShaders(ShaderTestFramework& framework, Summary& summary);
    static void testTestShaders(ShaderTestFramework& framework, Summary& summary);
};

} // namespace ShaderTest
} // namespace VoxelEditor