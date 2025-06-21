#include <gtest/gtest.h>
#include "../OpenGLRenderer.h"
#include "../RenderTypes.h"
#include "../../foundation/math/Matrix4f.h"
#include "../../foundation/math/Vector3f.h"
#include <memory>
#include <vector>

// Silence OpenGL deprecation warnings on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>

namespace VoxelEditor {
namespace Rendering {

// Test fixture for shader uniform tests with proper OpenGL context
class ShaderUniformsTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<OpenGLRenderer> renderer;
    
    void SetUp() override {
        // Initialize GLFW
        if (!glfwInit()) {
            GTEST_SKIP() << "Failed to initialize GLFW - skipping OpenGL tests";
            return;
        }
        
        // Configure GLFW for OpenGL 3.3 Core
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        // Create window
        window = glfwCreateWindow(640, 480, "ShaderUniformsTest", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            GTEST_SKIP() << "Failed to create GLFW window - skipping OpenGL tests";
            return;
        }
        
        // Make context current
        glfwMakeContextCurrent(window);
        
        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glfwDestroyWindow(window);
            glfwTerminate();
            GTEST_SKIP() << "Failed to initialize GLAD - skipping OpenGL tests";
            return;
        }
        
        // Create renderer
        renderer = std::make_unique<OpenGLRenderer>();
        
        RenderConfig config;
        config.windowWidth = 640;
        config.windowHeight = 480;
        config.vsync = false;
        config.enableDebugOutput = true;
        
        if (!renderer->initializeContext(config)) {
            GTEST_SKIP() << "Failed to initialize OpenGL context - skipping tests";
        }
    }
    
    void TearDown() override {
        renderer.reset();
        
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        
        glfwTerminate();
    }
    
    // Helper to create a simple test shader program
    ShaderId createTestProgram(bool useOldNames = false) {
        std::string vertexSource;
        std::string fragmentSource;
        
        if (useOldNames) {
            // Shader with old naming convention (u_model, u_view, u_projection)
            vertexSource = R"(
                #version 330 core
                layout(location = 0) in vec3 aPosition;
                
                uniform mat4 u_model;
                uniform mat4 u_view;
                uniform mat4 u_projection;
                
                void main() {
                    gl_Position = u_projection * u_view * u_model * vec4(aPosition, 1.0);
                }
            )";
            
            fragmentSource = R"(
                #version 330 core
                out vec4 FragColor;
                
                uniform vec4 u_color;
                
                void main() {
                    FragColor = u_color;
                }
            )";
        } else {
            // Shader with new naming convention (model, view, projection)
            vertexSource = R"(
                #version 330 core
                layout(location = 0) in vec3 aPosition;
                
                uniform mat4 model;
                uniform mat4 view;
                uniform mat4 projection;
                
                void main() {
                    gl_Position = projection * view * model * vec4(aPosition, 1.0);
                }
            )";
            
            fragmentSource = R"(
                #version 330 core
                out vec4 FragColor;
                
                uniform vec4 color;
                uniform float brightness;
                uniform vec3 lightDir;
                uniform int useTexture;
                
                void main() {
                    FragColor = color * brightness;
                }
            )";
        }
        
        ShaderId vs = renderer->createShader(ShaderType::Vertex, vertexSource);
        ShaderId fs = renderer->createShader(ShaderType::Fragment, fragmentSource);
        
        if (vs == InvalidId || fs == InvalidId) {
            return InvalidId;
        }
        
        std::vector<ShaderId> shaders = {vs, fs};
        return renderer->createProgram(shaders);
    }
};

// Test basic uniform setting and retrieval
TEST_F(ShaderUniformsTest, SetAndGetUniforms) {
    if (!renderer->isContextValid()) {
        GTEST_SKIP() << "No valid OpenGL context";
    }
    
    ShaderId program = createTestProgram(false);
    ASSERT_NE(program, InvalidId);
    
    renderer->useProgram(program);
    
    // Test matrix uniforms
    Math::Matrix4f modelMatrix = Math::Matrix4f::translation(Math::Vector3f(1.0f, 2.0f, 3.0f));
    
    Math::Matrix4f viewMatrix = Math::Matrix4f::lookAt(
        Math::Vector3f(0, 0, 5), 
        Math::Vector3f(0, 0, 0), 
        Math::Vector3f(0, 1, 0)
    );
    
    Math::Matrix4f projectionMatrix = Math::Matrix4f::perspective(45.0f, 1.333f, 0.1f, 100.0f);
    
    // Set uniforms
    renderer->setUniform("model", UniformValue(modelMatrix));
    renderer->setUniform("view", UniformValue(viewMatrix));
    renderer->setUniform("projection", UniformValue(projectionMatrix));
    
    // Get uniform locations to verify they exist
    int modelLoc = renderer->getUniformLocation(program, "model");
    int viewLoc = renderer->getUniformLocation(program, "view");
    int projLoc = renderer->getUniformLocation(program, "projection");
    
    EXPECT_NE(modelLoc, -1) << "Model uniform not found";
    EXPECT_NE(viewLoc, -1) << "View uniform not found";
    EXPECT_NE(projLoc, -1) << "Projection uniform not found";
    
    // Verify we can query back the values using OpenGL directly
    float queriedMatrix[16];
    glGetUniformfv(renderer->getProgramInfo(program)->glHandle, modelLoc, queriedMatrix);
    
    // Check that at least some values match (accounting for potential precision differences)
    EXPECT_NEAR(queriedMatrix[12], 1.0f, 0.001f) << "Model matrix translation X incorrect";
    EXPECT_NEAR(queriedMatrix[13], 2.0f, 0.001f) << "Model matrix translation Y incorrect";
    EXPECT_NEAR(queriedMatrix[14], 3.0f, 0.001f) << "Model matrix translation Z incorrect";
}

// Test compatibility with old uniform names
TEST_F(ShaderUniformsTest, OldUniformNameCompatibility) {
    if (!renderer->isContextValid()) {
        GTEST_SKIP() << "No valid OpenGL context";
    }
    
    ShaderId program = createTestProgram(true);
    ASSERT_NE(program, InvalidId);
    
    renderer->useProgram(program);
    
    // Test with old naming convention
    Math::Matrix4f modelMatrix = Math::Matrix4f::scale(Math::Vector3f(2.0f, 2.0f, 2.0f));
    
    Math::Matrix4f viewMatrix = Math::Matrix4f::Identity();
    
    Math::Matrix4f projectionMatrix = Math::Matrix4f::orthographic(-1, 1, -1, 1, -1, 1);
    
    // Set uniforms using old names
    renderer->setUniform("u_model", UniformValue(modelMatrix));
    renderer->setUniform("u_view", UniformValue(viewMatrix));
    renderer->setUniform("u_projection", UniformValue(projectionMatrix));
    
    // Verify locations
    int modelLoc = renderer->getUniformLocation(program, "u_model");
    int viewLoc = renderer->getUniformLocation(program, "u_view");
    int projLoc = renderer->getUniformLocation(program, "u_projection");
    
    EXPECT_NE(modelLoc, -1) << "u_model uniform not found";
    EXPECT_NE(viewLoc, -1) << "u_view uniform not found";
    EXPECT_NE(projLoc, -1) << "u_projection uniform not found";
    
    // Test color uniform
    Color testColor(1.0f, 0.5f, 0.25f, 1.0f);
    renderer->setUniform("u_color", UniformValue(testColor));
    
    int colorLoc = renderer->getUniformLocation(program, "u_color");
    EXPECT_NE(colorLoc, -1) << "u_color uniform not found";
    
    // Verify color values
    float queriedColor[4];
    glGetUniformfv(renderer->getProgramInfo(program)->glHandle, colorLoc, queriedColor);
    
    EXPECT_NEAR(queriedColor[0], 1.0f, 0.001f) << "Color R incorrect";
    EXPECT_NEAR(queriedColor[1], 0.5f, 0.001f) << "Color G incorrect";
    EXPECT_NEAR(queriedColor[2], 0.25f, 0.001f) << "Color B incorrect";
    EXPECT_NEAR(queriedColor[3], 1.0f, 0.001f) << "Color A incorrect";
}

// Test various uniform types
TEST_F(ShaderUniformsTest, VariousUniformTypes) {
    if (!renderer->isContextValid()) {
        GTEST_SKIP() << "No valid OpenGL context";
    }
    
    ShaderId program = createTestProgram(false);
    ASSERT_NE(program, InvalidId);
    
    renderer->useProgram(program);
    
    // Test float uniform
    float brightness = 0.75f;
    renderer->setUniform("brightness", UniformValue(brightness));
    
    int brightnessLoc = renderer->getUniformLocation(program, "brightness");
    if (brightnessLoc != -1) {
        float queriedBrightness;
        glGetUniformfv(renderer->getProgramInfo(program)->glHandle, brightnessLoc, &queriedBrightness);
        EXPECT_NEAR(queriedBrightness, brightness, 0.001f) << "Float uniform incorrect";
    }
    
    // Test vec3 uniform
    Math::Vector3f lightDir(0.0f, -1.0f, 0.0f);
    renderer->setUniform("lightDir", UniformValue(lightDir));
    
    int lightDirLoc = renderer->getUniformLocation(program, "lightDir");
    if (lightDirLoc != -1) {
        float queriedLightDir[3];
        glGetUniformfv(renderer->getProgramInfo(program)->glHandle, lightDirLoc, queriedLightDir);
        EXPECT_NEAR(queriedLightDir[0], 0.0f, 0.001f) << "Vec3 X incorrect";
        EXPECT_NEAR(queriedLightDir[1], -1.0f, 0.001f) << "Vec3 Y incorrect";
        EXPECT_NEAR(queriedLightDir[2], 0.0f, 0.001f) << "Vec3 Z incorrect";
    }
    
    // Test int uniform
    int useTexture = 1;
    renderer->setUniform("useTexture", UniformValue(useTexture));
    
    int useTextureLoc = renderer->getUniformLocation(program, "useTexture");
    if (useTextureLoc != -1) {
        int queriedUseTexture;
        glGetUniformiv(renderer->getProgramInfo(program)->glHandle, useTextureLoc, &queriedUseTexture);
        EXPECT_EQ(queriedUseTexture, useTexture) << "Int uniform incorrect";
    }
    
    // Test vec4/color uniform
    Color color(0.2f, 0.4f, 0.6f, 0.8f);
    renderer->setUniform("color", UniformValue(color));
    
    int colorLoc = renderer->getUniformLocation(program, "color");
    if (colorLoc != -1) {
        float queriedColor[4];
        glGetUniformfv(renderer->getProgramInfo(program)->glHandle, colorLoc, queriedColor);
        EXPECT_NEAR(queriedColor[0], 0.2f, 0.001f) << "Vec4 R incorrect";
        EXPECT_NEAR(queriedColor[1], 0.4f, 0.001f) << "Vec4 G incorrect";
        EXPECT_NEAR(queriedColor[2], 0.6f, 0.001f) << "Vec4 B incorrect";
        EXPECT_NEAR(queriedColor[3], 0.8f, 0.001f) << "Vec4 A incorrect";
    }
}

// Test uniform caching in program info
TEST_F(ShaderUniformsTest, UniformLocationCaching) {
    if (!renderer->isContextValid()) {
        GTEST_SKIP() << "No valid OpenGL context";
    }
    
    ShaderId program = createTestProgram(false);
    ASSERT_NE(program, InvalidId);
    
    // Get uniform location multiple times
    int loc1 = renderer->getUniformLocation(program, "model");
    int loc2 = renderer->getUniformLocation(program, "model");
    int loc3 = renderer->getUniformLocation(program, "model");
    
    // All should return the same location
    EXPECT_EQ(loc1, loc2);
    EXPECT_EQ(loc2, loc3);
    
    // Location should be cached in program info
    const ProgramInfo* info = renderer->getProgramInfo(program);
    ASSERT_NE(info, nullptr);
    
    auto it = info->uniformLocations.find("model");
    if (it != info->uniformLocations.end()) {
        EXPECT_EQ(it->second, loc1) << "Cached location doesn't match";
    }
}

// Test error handling for invalid uniforms
TEST_F(ShaderUniformsTest, InvalidUniformHandling) {
    if (!renderer->isContextValid()) {
        GTEST_SKIP() << "No valid OpenGL context";
    }
    
    ShaderId program = createTestProgram(false);
    ASSERT_NE(program, InvalidId);
    
    renderer->useProgram(program);
    
    // Try to set a non-existent uniform
    Math::Matrix4f dummyMatrix;
    renderer->setUniform("nonExistentUniform", UniformValue(dummyMatrix));
    
    // Should not crash, location should be -1
    int loc = renderer->getUniformLocation(program, "nonExistentUniform");
    EXPECT_EQ(loc, -1) << "Non-existent uniform should return -1";
    
    // Try with invalid program
    renderer->setUniform(InvalidId, "model", UniformValue(dummyMatrix));
    // Should not crash
    
    // Try with null program (no program bound)
    renderer->useProgram(InvalidId);
    renderer->setUniform("model", UniformValue(dummyMatrix));
    // Should not crash
}

// Test setting uniforms with program ID vs current program
TEST_F(ShaderUniformsTest, ProgramSpecificVsCurrentProgram) {
    if (!renderer->isContextValid()) {
        GTEST_SKIP() << "No valid OpenGL context";
    }
    
    ShaderId program1 = createTestProgram(false);
    ShaderId program2 = createTestProgram(true);
    
    ASSERT_NE(program1, InvalidId);
    ASSERT_NE(program2, InvalidId);
    
    // Set uniform on specific program (should bind it temporarily)
    Math::Matrix4f matrix1 = Math::Matrix4f::translation(Math::Vector3f(1, 0, 0));
    renderer->setUniform(program1, "model", UniformValue(matrix1));
    
    // Use program2 and set uniform on current program
    renderer->useProgram(program2);
    Math::Matrix4f matrix2 = Math::Matrix4f::translation(Math::Vector3f(0, 1, 0));
    renderer->setUniform("u_model", UniformValue(matrix2));
    
    // Verify program2 has the correct value
    int loc = renderer->getUniformLocation(program2, "u_model");
    if (loc != -1) {
        float queriedMatrix[16];
        glGetUniformfv(renderer->getProgramInfo(program2)->glHandle, loc, queriedMatrix);
        EXPECT_NEAR(queriedMatrix[12], 0.0f, 0.001f) << "Program2 matrix X should be 0";
        EXPECT_NEAR(queriedMatrix[13], 1.0f, 0.001f) << "Program2 matrix Y should be 1";
    }
}

} // namespace Rendering
} // namespace VoxelEditor