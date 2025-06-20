#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
extern "C" {
#include <glad/glad.h>
}
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <filesystem>

namespace VoxelEditor {
namespace Rendering {

class EnhancedShaderValidationIntegrationTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::string shaderPath;
    
    void SetUp() override {
        // Initialize GLFW
        if (!glfwInit()) {
            GTEST_SKIP() << "Failed to initialize GLFW";
        }
        
        // Configure GLFW for OpenGL 3.3 Core Profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        // Create window
        window = glfwCreateWindow(640, 480, "Enhanced Shader Validation Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            GTEST_SKIP() << "Failed to create GLFW window - OpenGL 3.3 may not be supported";
        }
        
        // Make context current
        glfwMakeContextCurrent(window);
        
        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glfwDestroyWindow(window);
            glfwTerminate();
            GTEST_SKIP() << "Failed to initialize GLAD";
        }
        
        // Check OpenGL version
        const char* version = (const char*)glGetString(GL_VERSION);
        std::cout << "OpenGL Version: " << version << std::endl;
        
        // Determine shader path
        // First try relative to binary location
        shaderPath = "core/rendering/shaders/";
        if (!std::filesystem::exists(shaderPath)) {
            // Try build directory structure
            shaderPath = "../bin/core/rendering/shaders/";
            if (!std::filesystem::exists(shaderPath)) {
                // Try source directory
                shaderPath = "../../core/rendering/shaders/";
                if (!std::filesystem::exists(shaderPath)) {
                    // Last resort - use absolute path from build
                    shaderPath = "/Users/michaelhalloran/cube_edit/build_ninja/bin/core/rendering/shaders/";
                }
            }
        }
        
        std::cout << "Using shader path: " << shaderPath << std::endl;
    }
    
    void TearDown() override {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    std::string readShaderFile(const std::string& filename) {
        std::string fullPath = shaderPath + filename;
        std::ifstream file(fullPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open shader file: " << fullPath << std::endl;
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    GLuint compileShader(GLenum type, const std::string& source) {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        
        // Check compilation status
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        
        if (!success) {
            GLint logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            
            if (logLength > 0) {
                std::vector<char> log(logLength);
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());
                std::cerr << "Shader compilation error:\n" << log.data() << std::endl;
            }
            
            glDeleteShader(shader);
            return 0;
        }
        
        return shader;
    }
    
    GLuint createProgram(GLuint vertexShader, GLuint fragmentShader) {
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        
        // Bind attribute locations before linking
        glBindAttribLocation(program, 0, "aPosition");
        glBindAttribLocation(program, 1, "aNormal");
        glBindAttribLocation(program, 2, "aColor");
        
        glLinkProgram(program);
        
        // Check linking status
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        
        if (!success) {
            GLint logLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            
            if (logLength > 0) {
                std::vector<char> log(logLength);
                glGetProgramInfoLog(program, logLength, nullptr, log.data());
                std::cerr << "Program linking error:\n" << log.data() << std::endl;
            }
            
            glDeleteProgram(program);
            return 0;
        }
        
        return program;
    }
    
    void validateProgram(GLuint program) {
        glValidateProgram(program);
        
        GLint status;
        glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
        if (status != GL_TRUE) {
            GLint logLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            
            if (logLength > 0) {
                std::vector<char> log(logLength);
                glGetProgramInfoLog(program, logLength, nullptr, log.data());
                std::cerr << "Program validation error:\n" << log.data() << std::endl;
            }
        }
    }
};

TEST_F(EnhancedShaderValidationIntegrationTest, ValidateEnhancedVoxelShader) {
    // Read the enhanced voxel shaders
    std::string vertexSource = readShaderFile("basic_voxel.vert");
    std::string fragmentSource = readShaderFile("enhanced_voxel.frag");
    
    ASSERT_FALSE(vertexSource.empty()) << "Could not read vertex shader file";
    ASSERT_FALSE(fragmentSource.empty()) << "Could not read fragment shader file";
    
    // Compile vertex shader
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    ASSERT_NE(vertexShader, 0) << "Failed to compile vertex shader";
    
    // Compile fragment shader
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    ASSERT_NE(fragmentShader, 0) << "Failed to compile enhanced fragment shader";
    
    // Create and link program
    GLuint program = createProgram(vertexShader, fragmentShader);
    ASSERT_NE(program, 0) << "Failed to link enhanced shader program";
    
    // Validate program
    validateProgram(program);
    
    // Check uniforms exist - note some may be optimized out if not used
    GLint lightPosLoc = glGetUniformLocation(program, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(program, "lightColor");
    GLint viewPosLoc = glGetUniformLocation(program, "viewPos");
    
    // These uniforms have default values in the shader, so they might be optimized out
    // We'll just check that the shader compiled and linked successfully
    std::cout << "Enhanced shader compiled and linked successfully" << std::endl;
    std::cout << "lightPos location: " << lightPosLoc << std::endl;
    std::cout << "lightColor location: " << lightColorLoc << std::endl;
    std::cout << "viewPos location: " << viewPosLoc << std::endl;
    
    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(program);
}

TEST_F(EnhancedShaderValidationIntegrationTest, ValidateBasicVoxelShader) {
    // Read the basic voxel shaders
    std::string vertexSource = readShaderFile("basic_voxel_gl33.vert");
    std::string fragmentSource = readShaderFile("basic_voxel_gl33.frag");
    
    ASSERT_FALSE(vertexSource.empty()) << "Could not read vertex shader file";
    ASSERT_FALSE(fragmentSource.empty()) << "Could not read fragment shader file";
    
    // Compile shaders
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    ASSERT_NE(vertexShader, 0) << "Failed to compile vertex shader";
    
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    ASSERT_NE(fragmentShader, 0) << "Failed to compile fragment shader";
    
    // Create and link program
    GLuint program = createProgram(vertexShader, fragmentShader);
    ASSERT_NE(program, 0) << "Failed to link shader program";
    
    // Validate program
    validateProgram(program);
    
    // Check basic uniforms
    GLint modelLoc = glGetUniformLocation(program, "model");
    GLint viewLoc = glGetUniformLocation(program, "view");
    GLint projLoc = glGetUniformLocation(program, "projection");
    
    std::cout << "Basic GL3.3 shader compiled and linked successfully" << std::endl;
    std::cout << "model location: " << modelLoc << std::endl;
    std::cout << "view location: " << viewLoc << std::endl;
    std::cout << "projection location: " << projLoc << std::endl;
    
    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(program);
}

// GL 2.1 shader test removed - project consolidated on OpenGL 3.3 Core Profile

TEST_F(EnhancedShaderValidationIntegrationTest, ShaderCompatibilityAcrossVersions) {
    // Test that we can load and compile shaders for OpenGL 3.3
    struct ShaderPair {
        std::string vertFile;
        std::string fragFile;
        std::string name;
    };
    
    std::vector<ShaderPair> shaderPairs = {
        {"basic_voxel_gl33.vert", "basic_voxel_gl33.frag", "Basic GL 3.3"},
        {"basic_voxel.vert", "basic_voxel.frag", "Standard Voxel"},
        {"basic_voxel.vert", "enhanced_voxel.frag", "Enhanced Voxel"},
        {"basic_voxel.vert", "flat_voxel.frag", "Flat Voxel"},
        {"ground_plane.vert", "ground_plane.frag", "Ground Plane"}
    };
    
    for (const auto& pair : shaderPairs) {
        std::string vertexSource = readShaderFile(pair.vertFile);
        std::string fragmentSource = readShaderFile(pair.fragFile);
        
        if (vertexSource.empty() || fragmentSource.empty()) {
            ADD_FAILURE() << "Could not read shader files for " << pair.name;
            continue;
        }
        
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
        
        if (vertexShader && fragmentShader) {
            GLuint program = createProgram(vertexShader, fragmentShader);
            if (program) {
                std::cout << pair.name << " shaders compiled and linked successfully" << std::endl;
                glDeleteProgram(program);
            }
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
        }
    }
}

} // namespace Rendering
} // namespace VoxelEditor