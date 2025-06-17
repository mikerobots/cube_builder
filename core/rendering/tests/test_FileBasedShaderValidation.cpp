#include <gtest/gtest.h>
extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <vector>

// Test framework for validating file-based shaders
class FileBasedShaderTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::string shaderDir;
    
    void SetUp() override {
        if (!glfwInit()) {
            FAIL() << "Failed to initialize GLFW";
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        window = glfwCreateWindow(1, 1, "Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            FAIL() << "Failed to create GLFW window";
        }
        
        glfwMakeContextCurrent(window);
        
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            FAIL() << "Failed to initialize GLAD";
        }
        
        // Find shader directory relative to build directory
        // Tests run from the project root, so shaders are in core/rendering/shaders
        std::filesystem::path currentPath = std::filesystem::current_path();
        shaderDir = currentPath / "core" / "rendering" / "shaders";
        
        // Check if shader directory exists, if not try from build_ninja
        if (!std::filesystem::exists(shaderDir)) {
            shaderDir = currentPath.parent_path() / "core" / "rendering" / "shaders";
        }
        
        std::cout << "Looking for shaders in: " << shaderDir << std::endl;
    }
    
    void TearDown() override {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    std::string loadShaderFile(const std::string& filename) {
        std::filesystem::path fullPath = std::filesystem::path(shaderDir) / filename;
        std::ifstream file(fullPath.string());
        if (!file.is_open()) {
            ADD_FAILURE() << "Failed to open shader file: " << fullPath.string();
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    bool compileShader(GLuint shader, const std::string& source) {
        const char* sourcePtr = source.c_str();
        glShaderSource(shader, 1, &sourcePtr, nullptr);
        glCompileShader(shader);
        
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            ADD_FAILURE() << "Shader compilation failed: " << infoLog;
        }
        
        return success == GL_TRUE;
    }
    
    bool linkProgram(GLuint program) {
        glLinkProgram(program);
        
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            ADD_FAILURE() << "Program linking failed: " << infoLog;
        }
        
        return success == GL_TRUE;
    }
    
    struct ShaderProgramInfo {
        GLuint program;
        bool success;
        std::vector<std::string> uniforms;
        std::vector<std::string> attributes;
    };
    
    ShaderProgramInfo loadAndValidateShaderProgram(
        const std::string& vertexFile,
        const std::string& fragmentFile
    ) {
        ShaderProgramInfo info{0, false, {}, {}};
        
        std::string vertexSource = loadShaderFile(vertexFile);
        std::string fragmentSource = loadShaderFile(fragmentFile);
        
        if (vertexSource.empty() || fragmentSource.empty()) {
            return info;
        }
        
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        info.program = glCreateProgram();
        
        if (compileShader(vertexShader, vertexSource) &&
            compileShader(fragmentShader, fragmentSource)) {
            
            glAttachShader(info.program, vertexShader);
            glAttachShader(info.program, fragmentShader);
            
            if (linkProgram(info.program)) {
                info.success = true;
                
                // Get active uniforms
                GLint numUniforms;
                glGetProgramiv(info.program, GL_ACTIVE_UNIFORMS, &numUniforms);
                for (GLint i = 0; i < numUniforms; ++i) {
                    char name[256];
                    GLsizei length;
                    GLint size;
                    GLenum type;
                    glGetActiveUniform(info.program, i, 256, &length, &size, &type, name);
                    info.uniforms.push_back(name);
                }
                
                // Get active attributes
                GLint numAttributes;
                glGetProgramiv(info.program, GL_ACTIVE_ATTRIBUTES, &numAttributes);
                for (GLint i = 0; i < numAttributes; ++i) {
                    char name[256];
                    GLsizei length;
                    GLint size;
                    GLenum type;
                    glGetActiveAttrib(info.program, i, 256, &length, &size, &type, name);
                    info.attributes.push_back(name);
                }
            }
        }
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return info;
    }
};

// Test flat voxel shader (previously untested)
TEST_F(FileBasedShaderTest, FlatVoxelShader) {
    // Skip this test if running in a headless environment
    if (!glfwGetCurrentContext()) {
        GTEST_SKIP() << "Skipping test that requires OpenGL context";
    }
    
    auto info = loadAndValidateShaderProgram("basic_voxel.vert", "flat_voxel.frag");
    
    ASSERT_TRUE(info.success) << "Failed to compile/link flat voxel shader";
    
    // Check for required uniforms
    auto hasUniform = [&](const std::string& name) {
        return std::find(info.uniforms.begin(), info.uniforms.end(), name) != info.uniforms.end();
    };
    
    // Check for vertex shader uniforms
    EXPECT_TRUE(hasUniform("model")) << "Missing model uniform";
    EXPECT_TRUE(hasUniform("view")) << "Missing view uniform";
    EXPECT_TRUE(hasUniform("projection")) << "Missing projection uniform";
    
    // Note: flat_voxel.frag uniforms might be optimized out if not used
    // The shader uses face normals for brightness, not these uniforms
    
    // Check for required attributes
    auto hasAttribute = [&](const std::string& name) {
        return std::find(info.attributes.begin(), info.attributes.end(), name) != info.attributes.end();
    };
    
    EXPECT_TRUE(hasAttribute("aPos")) << "Missing aPos attribute";
    EXPECT_TRUE(hasAttribute("aNormal")) << "Missing aNormal attribute";
    EXPECT_TRUE(hasAttribute("aColor")) << "Missing aColor attribute";
    
    if (info.program) {
        glDeleteProgram(info.program);
    }
}

// Test basic voxel GL33 shader variants
TEST_F(FileBasedShaderTest, BasicVoxelGL33Shader) {
    // Skip this test if running in a headless environment
    if (!glfwGetCurrentContext()) {
        GTEST_SKIP() << "Skipping test that requires OpenGL context";
    }
    
    auto info = loadAndValidateShaderProgram("basic_voxel_gl33.vert", "basic_voxel_gl33.frag");
    
    ASSERT_TRUE(info.success) << "Failed to compile/link basic voxel GL33 shader";
    
    // Check GL 3.3 specific features
    std::string vertexSource = loadShaderFile("basic_voxel_gl33.vert");
    std::string fragmentSource = loadShaderFile("basic_voxel_gl33.frag");
    
    EXPECT_TRUE(vertexSource.find("#version 330 core") != std::string::npos) 
        << "Vertex shader should specify version 330 core";
    EXPECT_TRUE(fragmentSource.find("#version 330 core") != std::string::npos) 
        << "Fragment shader should specify version 330 core";
    
    // Check for layout qualifiers (GL 3.3 feature)
    EXPECT_TRUE(vertexSource.find("layout(location") != std::string::npos) 
        << "GL 3.3 shader should use layout qualifiers";
    
    if (info.program) {
        glDeleteProgram(info.program);
    }
}

// Test enhanced voxel shader
TEST_F(FileBasedShaderTest, EnhancedVoxelShader) {
    // Skip this test if running in a headless environment
    if (!glfwGetCurrentContext()) {
        GTEST_SKIP() << "Skipping test that requires OpenGL context";
    }
    
    auto info = loadAndValidateShaderProgram("basic_voxel.vert", "enhanced_voxel.frag");
    
    ASSERT_TRUE(info.success) << "Failed to compile/link enhanced voxel shader";
    
    // Check for enhanced features
    auto hasUniform = [&](const std::string& name) {
        return std::find(info.uniforms.begin(), info.uniforms.end(), name) != info.uniforms.end();
    };
    
    // Enhanced shader uses same uniforms as basic shader
    EXPECT_TRUE(hasUniform("model")) << "Missing model uniform";
    EXPECT_TRUE(hasUniform("view")) << "Missing view uniform";
    EXPECT_TRUE(hasUniform("projection")) << "Missing projection uniform";
    
    // Enhanced shader also has lighting uniforms (with defaults)
    // Note: uniforms with default values might be optimized out
    
    if (info.program) {
        glDeleteProgram(info.program);
    }
}

// Test ground plane shader
TEST_F(FileBasedShaderTest, GroundPlaneShader) {
    // Skip this test if running in a headless environment
    if (!glfwGetCurrentContext()) {
        GTEST_SKIP() << "Skipping test that requires OpenGL context";
    }
    
    auto info = loadAndValidateShaderProgram("ground_plane.vert", "ground_plane.frag");
    
    ASSERT_TRUE(info.success) << "Failed to compile/link ground plane shader";
    
    // Check for ground plane specific uniforms
    auto hasUniform = [&](const std::string& name) {
        return std::find(info.uniforms.begin(), info.uniforms.end(), name) != info.uniforms.end();
    };
    
    EXPECT_TRUE(hasUniform("mvpMatrix")) << "Missing mvpMatrix uniform";
    EXPECT_TRUE(hasUniform("minorLineColor")) << "Missing minorLineColor uniform";
    EXPECT_TRUE(hasUniform("majorLineColor")) << "Missing majorLineColor uniform";
    EXPECT_TRUE(hasUniform("opacity")) << "Missing opacity uniform";
    
    if (info.program) {
        glDeleteProgram(info.program);
    }
}

// Test shader compatibility between versions
TEST_F(FileBasedShaderTest, ShaderVersionCompatibility) {
    // Skip this test if running in a headless environment
    if (!glfwGetCurrentContext()) {
        GTEST_SKIP() << "Skipping test that requires OpenGL context";
    }
    
    // Test that we have both GL 2.1 and GL 3.3 compatible versions
    auto basicInfo = loadAndValidateShaderProgram("basic_voxel.vert", "basic_voxel.frag");
    auto gl33Info = loadAndValidateShaderProgram("basic_voxel_gl33.vert", "basic_voxel_gl33.frag");
    
    EXPECT_TRUE(basicInfo.success) << "Basic shader should compile";
    EXPECT_TRUE(gl33Info.success) << "GL 3.3 shader should compile";
    
    // Both should have similar uniforms (though attribute handling differs)
    auto hasUniform = [](const auto& info, const std::string& name) {
        return std::find(info.uniforms.begin(), info.uniforms.end(), name) != info.uniforms.end();
    };
    
    // Basic shader uses separate matrices
    EXPECT_TRUE(hasUniform(basicInfo, "model"));
    EXPECT_TRUE(hasUniform(basicInfo, "view"));
    EXPECT_TRUE(hasUniform(basicInfo, "projection"));
    
    // GL3.3 shader should have same uniforms
    EXPECT_TRUE(hasUniform(gl33Info, "model"));
    EXPECT_TRUE(hasUniform(gl33Info, "view"));
    EXPECT_TRUE(hasUniform(gl33Info, "projection"));
    
    if (basicInfo.program) glDeleteProgram(basicInfo.program);
    if (gl33Info.program) glDeleteProgram(gl33Info.program);
}