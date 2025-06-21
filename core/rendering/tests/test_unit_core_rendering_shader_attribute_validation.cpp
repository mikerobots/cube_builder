#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "../RenderEngine.h"
#include "../ShaderManager.h"
#include "../OpenGLRenderer.h"
#include "../../foundation/events/EventDispatcher.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace VoxelEditor {
namespace Rendering {

class ShaderAttributeValidationTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<RenderEngine> renderEngine;
    
    void SetUp() override {
        // Initialize GLFW
        if (!glfwInit()) {
            GTEST_SKIP() << "Failed to initialize GLFW";
        }
        
        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        // Create window
        window = glfwCreateWindow(800, 600, "Shader Validation Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            GTEST_SKIP() << "Failed to create GLFW window";
        }
        
        // Make context current
        glfwMakeContextCurrent(window);
        
        // Initialize render engine
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        renderEngine = std::make_unique<RenderEngine>(eventDispatcher.get());
        
        RenderConfig config;
        config.windowWidth = 800;
        config.windowHeight = 600;
        
        if (!renderEngine->initialize(config)) {
            GTEST_SKIP() << "Failed to initialize render engine";
        }
    }
    
    void TearDown() override {
        renderEngine.reset();
        eventDispatcher.reset();
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    std::string readShaderFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

TEST_F(ShaderAttributeValidationTest, ValidateBasicVoxelShaderAttributes) {
    // Get the basic shader
    ShaderId basicShader = renderEngine->getBuiltinShader("basic");
    ASSERT_NE(basicShader, InvalidId) << "Failed to get basic shader";
    
    // Use the shader
    renderEngine->useProgram(basicShader);
    
    // Get current program
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    ASSERT_NE(currentProgram, 0) << "No shader program is active";
    
    // Check attribute locations
    // The shaders use layout qualifiers, so we can check by location
    // The actual names might vary between shaders
    GLint posLoc = glGetAttribLocation(currentProgram, "aPos");
    GLint normalLoc = glGetAttribLocation(currentProgram, "aNormal");
    GLint colorLoc = glGetAttribLocation(currentProgram, "aColor");
    
    // If names don't match, check alternate names
    if (posLoc == -1) posLoc = glGetAttribLocation(currentProgram, "a_position");
    if (normalLoc == -1) normalLoc = glGetAttribLocation(currentProgram, "a_normal");
    if (colorLoc == -1) colorLoc = glGetAttribLocation(currentProgram, "a_color");
    
    // Validate expected locations based on shader layout qualifiers
    // With layout qualifiers, the location is fixed regardless of the name
    EXPECT_EQ(posLoc, 0) << "Position attribute should be at location 0";
    EXPECT_EQ(normalLoc, 1) << "Normal attribute should be at location 1";
    EXPECT_EQ(colorLoc, 2) << "Color attribute should be at location 2";
    
    // Check that the shader compiled without errors
    GLint linkStatus;
    glGetProgramiv(currentProgram, GL_LINK_STATUS, &linkStatus);
    EXPECT_EQ(linkStatus, GL_TRUE) << "Shader program failed to link";
    
    // Validate the program
    glValidateProgram(currentProgram);
    GLint validateStatus;
    glGetProgramiv(currentProgram, GL_VALIDATE_STATUS, &validateStatus);
    
    if (validateStatus != GL_TRUE) {
        GLint logLength;
        glGetProgramiv(currentProgram, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            std::vector<char> log(logLength);
            glGetProgramInfoLog(currentProgram, logLength, nullptr, log.data());
            std::cerr << "Shader validation log: " << log.data() << std::endl;
        }
    }
}

TEST_F(ShaderAttributeValidationTest, ValidateVertexBufferBinding) {
    // Create a test mesh
    Mesh testMesh;
    
    // Add vertices with all attributes
    testMesh.vertices.push_back(Vertex(Math::Vector3f(0, 0, 0), Math::Vector3f(0, 0, 1), 
                                       Math::Vector2f(0, 0), Color(1, 0, 0, 1)));
    testMesh.vertices.push_back(Vertex(Math::Vector3f(1, 0, 0), Math::Vector3f(0, 0, 1), 
                                       Math::Vector2f(1, 0), Color(0, 1, 0, 1)));
    testMesh.vertices.push_back(Vertex(Math::Vector3f(0, 1, 0), Math::Vector3f(0, 0, 1), 
                                       Math::Vector2f(0, 1), Color(0, 0, 1, 1)));
    
    testMesh.indices = {0, 1, 2};
    
    // Setup mesh buffers
    renderEngine->setupMeshBuffers(testMesh);
    
    // Get basic shader
    ShaderId basicShader = renderEngine->getBuiltinShader("basic");
    ASSERT_NE(basicShader, InvalidId);
    
    // Use shader
    renderEngine->useProgram(basicShader);
    
    // Bind the vertex array
    renderEngine->bindVertexArray(testMesh.vertexArray);
    
    // Check that vertex attributes are properly enabled
    for (int i = 0; i < 3; ++i) {  // Check first 3 attributes (pos, normal, color)
        GLint enabled;
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
        EXPECT_EQ(enabled, GL_TRUE) << "Vertex attribute " << i << " should be enabled";
        
        // Get attribute info
        GLint size, type, normalized, stride;
        GLvoid* pointer;
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
        glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointer);
        
        // Log the attribute setup
        std::cout << "Attribute " << i << ": "
                  << "size=" << size << ", "
                  << "type=" << std::hex << type << std::dec << ", "
                  << "stride=" << stride << ", "
                  << "offset=" << reinterpret_cast<size_t>(pointer) << std::endl;
        
        // Validate expected values
        if (i == 0 || i == 1) {  // Position and Normal
            EXPECT_EQ(size, 3) << "Position/Normal should have 3 components";
        } else if (i == 2) {  // Color
            EXPECT_EQ(size, 4) << "Color should have 4 components";
        }
        
        EXPECT_EQ(type, GL_FLOAT) << "All attributes should be float type";
        EXPECT_EQ(stride, sizeof(Vertex)) << "Stride should match vertex size";
    }
    
    // Clean up
    renderEngine->cleanupMeshBuffers(testMesh);
}

TEST_F(ShaderAttributeValidationTest, ValidateShaderUniformBinding) {
    // Clear any previous OpenGL errors
    while (glGetError() != GL_NO_ERROR) {}
    
    // Get basic shader
    ShaderId basicShader = renderEngine->getBuiltinShader("basic");
    ASSERT_NE(basicShader, InvalidId);
    
    // Use shader
    renderEngine->useProgram(basicShader);
    
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    
    // Check required uniforms
    GLint modelLoc = glGetUniformLocation(currentProgram, "model");
    GLint viewLoc = glGetUniformLocation(currentProgram, "view");
    GLint projectionLoc = glGetUniformLocation(currentProgram, "projection");
    
    EXPECT_NE(modelLoc, -1) << "model uniform not found";
    EXPECT_NE(viewLoc, -1) << "view uniform not found";
    EXPECT_NE(projectionLoc, -1) << "projection uniform not found";
    
    // Set uniforms and verify no errors
    Math::Matrix4f identity = Math::Matrix4f::Identity();
    
    renderEngine->setUniform("model", UniformValue(identity));
    EXPECT_EQ(glGetError(), GL_NO_ERROR) << "Error setting model uniform";
    
    renderEngine->setUniform("view", UniformValue(identity));
    EXPECT_EQ(glGetError(), GL_NO_ERROR) << "Error setting view uniform";
    
    renderEngine->setUniform("projection", UniformValue(identity));
    EXPECT_EQ(glGetError(), GL_NO_ERROR) << "Error setting projection uniform";
}

TEST_F(ShaderAttributeValidationTest, ValidateShaderRenderPipeline) {
    // This test validates the full render pipeline with proper attribute binding
    
    // Create a simple test mesh
    Mesh testMesh;
    testMesh.vertices.push_back(Vertex(Math::Vector3f(-0.5f, -0.5f, 0), 
                                      Math::Vector3f(0, 0, 1), 
                                      Math::Vector2f(0, 0), 
                                      Color(1, 1, 0, 1)));
    testMesh.vertices.push_back(Vertex(Math::Vector3f(0.5f, -0.5f, 0), 
                                      Math::Vector3f(0, 0, 1), 
                                      Math::Vector2f(1, 0), 
                                      Color(1, 1, 0, 1)));
    testMesh.vertices.push_back(Vertex(Math::Vector3f(0, 0.5f, 0), 
                                      Math::Vector3f(0, 0, 1), 
                                      Math::Vector2f(0.5f, 1), 
                                      Color(1, 1, 0, 1)));
    testMesh.indices = {0, 1, 2};
    
    // Setup mesh
    renderEngine->setupMeshBuffers(testMesh);
    
    // Begin frame
    renderEngine->beginFrame();
    renderEngine->clear(ClearFlags::All, Color(0.2f, 0.2f, 0.2f, 1.0f));
    
    // Setup camera matrices
    Math::Matrix4f model = Math::Matrix4f::Identity();
    Math::Matrix4f view = Math::Matrix4f::lookAt(
        Math::Vector3f(0, 0, 3),  // eye
        Math::Vector3f(0, 0, 0),  // target
        Math::Vector3f(0, 1, 0)   // up
    );
    Math::Matrix4f projection = Math::Matrix4f::perspective(
        45.0f * 3.14159f / 180.0f,  // fov in radians
        800.0f / 600.0f,            // aspect
        0.1f,                       // near
        100.0f                      // far
    );
    
    // Render with proper attribute binding
    Transform transform;
    Material material;
    material.albedo = Color(1, 1, 0, 1);
    material.shader = renderEngine->getBuiltinShader("basic");
    
    // Clear any GL errors before rendering
    while (glGetError() != GL_NO_ERROR) {}
    
    // Render the mesh
    renderEngine->renderMesh(testMesh, transform, material);
    
    // Check for GL errors after rendering
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error after rendering: " << error;
    
    // End frame
    renderEngine->endFrame();
    renderEngine->present();
    
    // Clean up
    renderEngine->cleanupMeshBuffers(testMesh);
}

} // namespace Rendering
} // namespace VoxelEditor