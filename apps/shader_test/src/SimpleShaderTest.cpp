extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Add GL_SILENCE_DEPRECATION for macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

struct ShaderTestResult {
    std::string name;
    bool compilationSuccess;
    bool linkingSuccess;
    std::string errorLog;
};

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int compileShader(GLenum type, const std::string& source, std::string& errorLog) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        errorLog = infoLog;
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

unsigned int linkProgram(unsigned int vertexShader, unsigned int fragmentShader, std::string& errorLog) {
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        errorLog = infoLog;
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

ShaderTestResult testShaderPair(const std::string& name, 
                               const std::string& vertexPath, 
                               const std::string& fragmentPath) {
    ShaderTestResult result;
    result.name = name;
    result.compilationSuccess = false;
    result.linkingSuccess = false;
    
    // Read shader files
    std::string vertexSource = readFile(vertexPath);
    std::string fragmentSource = readFile(fragmentPath);
    
    if (vertexSource.empty() || fragmentSource.empty()) {
        result.errorLog = "Failed to read shader files";
        return result;
    }
    
    // Compile vertex shader
    std::string vertexError;
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, vertexError);
    if (vertexShader == 0) {
        result.errorLog = "Vertex shader compilation failed: " + vertexError;
        return result;
    }
    
    // Compile fragment shader
    std::string fragmentError;
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, fragmentError);
    if (fragmentShader == 0) {
        result.errorLog = "Fragment shader compilation failed: " + fragmentError;
        glDeleteShader(vertexShader);
        return result;
    }
    
    result.compilationSuccess = true;
    
    // Link program
    std::string linkError;
    unsigned int program = linkProgram(vertexShader, fragmentShader, linkError);
    if (program == 0) {
        result.errorLog = "Program linking failed: " + linkError;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return result;
    }
    
    result.linkingSuccess = true;
    
    // Query uniforms and attributes
    int numUniforms;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
    std::cout << "    Uniforms (" << numUniforms << "): ";
    for (int i = 0; i < numUniforms; ++i) {
        char name[256];
        int size;
        GLenum type;
        glGetActiveUniform(program, i, sizeof(name), nullptr, &size, &type, name);
        std::cout << name << " ";
    }
    std::cout << std::endl;
    
    int numAttributes;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numAttributes);
    std::cout << "    Attributes (" << numAttributes << "): ";
    for (int i = 0; i < numAttributes; ++i) {
        char name[256];
        int size;
        GLenum type;
        glGetActiveAttrib(program, i, sizeof(name), nullptr, &size, &type, name);
        std::cout << name << " ";
    }
    std::cout << std::endl;
    
    // Clean up
    glDeleteProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return result;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Shader Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    
    std::cout << "=== Simple Shader Validation Test ===" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << std::endl;
    
    // Determine shader directory
    std::string shaderDir = "bin/core/rendering/shaders/";
    std::ifstream testFile(shaderDir + "basic_voxel.vert");
    if (!testFile.is_open()) {
        shaderDir = "build_ninja/bin/core/rendering/shaders/";
        testFile.open(shaderDir + "basic_voxel.vert");
        if (!testFile.is_open()) {
            shaderDir = "core/rendering/shaders/";
        }
    }
    testFile.close();
    
    // Test shader pairs
    std::vector<ShaderTestResult> results;
    
    // Test basic voxel shaders
    std::cout << "Testing basic_voxel shaders..." << std::endl;
    results.push_back(testShaderPair("basic_voxel", 
                                    shaderDir + "basic_voxel.vert",
                                    shaderDir + "basic_voxel.frag"));
    
    std::cout << "Testing basic_voxel_gl33 shaders..." << std::endl;
    results.push_back(testShaderPair("basic_voxel_gl33", 
                                    shaderDir + "basic_voxel_gl33.vert",
                                    shaderDir + "basic_voxel_gl33.frag"));
    
    std::cout << "Testing flat_voxel shader..." << std::endl;
    results.push_back(testShaderPair("flat_voxel", 
                                    shaderDir + "basic_voxel.vert",
                                    shaderDir + "flat_voxel.frag"));
    
    std::cout << "Testing enhanced_voxel shader..." << std::endl;
    results.push_back(testShaderPair("enhanced_voxel", 
                                    shaderDir + "basic_voxel.vert",
                                    shaderDir + "enhanced_voxel.frag"));
    
    std::cout << "Testing test_fixed_color_gl33 shaders..." << std::endl;
    results.push_back(testShaderPair("test_fixed_color_gl33", 
                                    shaderDir + "test_fixed_color_gl33.vert",
                                    shaderDir + "test_fixed_color_gl33.frag"));
    
    // Legacy shader tests removed - we only support OpenGL 3.3 Core Profile
    
    // Summary
    std::cout << std::endl;
    std::cout << "=== Summary ===" << std::endl;
    int passed = 0;
    int failed = 0;
    
    for (const auto& result : results) {
        if (result.compilationSuccess && result.linkingSuccess) {
            std::cout << "✅ " << result.name << " - PASSED" << std::endl;
            passed++;
        } else {
            std::cout << "❌ " << result.name << " - FAILED: " << result.errorLog << std::endl;
            failed++;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Total: " << (passed + failed) << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    // Clean up
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return failed > 0 ? 1 : 0;
}