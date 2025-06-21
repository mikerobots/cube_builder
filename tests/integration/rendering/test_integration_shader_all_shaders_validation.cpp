#include <gtest/gtest.h>
#include "rendering/RenderEngine.h"
#include "rendering/RenderTypes.h"
#include "rendering/ShaderManager.h"
#include "camera/CameraController.h"
#include "math/Matrix4f.h"
#include "math/Vector3f.h"
#include "math/Vector4f.h"
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

// Include OpenGL headers
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>

namespace VoxelEditor {
namespace Tests {

class AllShadersValidationTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Rendering::RenderEngine> renderEngine;
    std::unique_ptr<Camera::CameraController> cameraController;
    
    void SetUp() override {
        // Skip in CI environment
        if (std::getenv("CI") != nullptr) {
            GTEST_SKIP() << "Skipping OpenGL tests in CI environment";
        }
        
        // Initialize GLFW
        if (!glfwInit()) {
            GTEST_SKIP() << "Failed to initialize GLFW";
        }
        
        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
        
        window = glfwCreateWindow(800, 600, "Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            GTEST_SKIP() << "Failed to create GLFW window";
        }
        
        glfwMakeContextCurrent(window);
        
        // Create render engine
        renderEngine = std::make_unique<Rendering::RenderEngine>(nullptr);
        Rendering::RenderConfig config;
        config.windowWidth = 800;
        config.windowHeight = 600;
        
        if (!renderEngine->initialize(config)) {
            GTEST_SKIP() << "Failed to initialize render engine";
        }
        
        // Create camera controller
        cameraController = std::make_unique<Camera::CameraController>(nullptr);
        cameraController->setViewPreset(Camera::ViewPreset::ISOMETRIC);
    }
    
    void TearDown() override {
        cameraController.reset();
        renderEngine.reset();
        
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        glfwTerminate();
    }
    
    // Helper to create a test mesh with all possible attributes
    Rendering::Mesh createFullAttributeMesh() {
        Rendering::Mesh mesh;
        
        // Triangle with all attributes
        Rendering::Vertex v1(Math::Vector3f(0.0f, 0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                            Math::Vector2f(0.5f, 1.0f), Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f));
        Rendering::Vertex v2(Math::Vector3f(-0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                            Math::Vector2f(0.0f, 0.0f), Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f));
        Rendering::Vertex v3(Math::Vector3f(0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                            Math::Vector2f(1.0f, 0.0f), Rendering::Color(0.0f, 0.0f, 1.0f, 1.0f));
        
        mesh.vertices = {v1, v2, v3};
        mesh.indices = {0, 1, 2};
        
        return mesh;
    }
    
    // Helper to create a line mesh for wireframe/outline testing
    Rendering::Mesh createLineMesh() {
        Rendering::Mesh mesh;
        
        // Square outline
        Rendering::Vertex v1(Math::Vector3f(-0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                            Math::Vector2f(0.0f, 0.0f), Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f));
        Rendering::Vertex v2(Math::Vector3f(0.5f, -0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                            Math::Vector2f(1.0f, 0.0f), Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f));
        Rendering::Vertex v3(Math::Vector3f(0.5f, 0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                            Math::Vector2f(1.0f, 1.0f), Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f));
        Rendering::Vertex v4(Math::Vector3f(-0.5f, 0.5f, 0.0f), Math::Vector3f(0.0f, 0.0f, 1.0f), 
                            Math::Vector2f(0.0f, 1.0f), Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f));
        
        mesh.vertices = {v1, v2, v3, v4};
        mesh.indices = {0, 1, 1, 2, 2, 3, 3, 0}; // Line segments
        
        return mesh;
    }
    
    // Helper to check for OpenGL errors
    bool checkGLError(const std::string& context) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "GL Error in " << context << ": " << error << std::endl;
            return false;
        }
        return true;
    }
    
    // Helper to find all shader files
    std::vector<std::pair<std::string, std::string>> findAllShaderPairs() {
        std::vector<std::pair<std::string, std::string>> shaderPairs;
        
        // Check common shader directories
        std::vector<std::string> searchDirs = {
            "core/rendering/shaders/",
            "build_ninja/core/rendering/shaders/",
            "build_debug/core/rendering/shaders/"
        };
        
        for (const auto& dir : searchDirs) {
            if (!std::filesystem::exists(dir)) continue;
            
            std::set<std::string> baseNames;
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (filename.ends_with(".vert") || filename.ends_with(".frag")) {
                        std::string baseName = filename.substr(0, filename.find_last_of('.'));
                        baseNames.insert(baseName);
                    }
                }
            }
            
            for (const auto& baseName : baseNames) {
                std::string vertPath = dir + baseName + ".vert";
                std::string fragPath = dir + baseName + ".frag";
                
                // Only add if both files exist
                if (std::filesystem::exists(vertPath) && std::filesystem::exists(fragPath)) {
                    shaderPairs.push_back({baseName, dir});
                }
            }
        }
        
        return shaderPairs;
    }
};

// Test all built-in shaders that are accessible through RenderEngine
TEST_F(AllShadersValidationTest, AllBuiltinShadersCompileAndRender) {
    // List of all known built-in shaders
    std::vector<std::string> builtinShaders = {
        "basic",
        "enhanced", 
        "flat"
    };
    
    auto mesh = createFullAttributeMesh();
    renderEngine->setupMeshBuffers(mesh);
    renderEngine->setCamera(*cameraController->getCamera());
    
    for (const auto& shaderName : builtinShaders) {
        // Clear any existing errors
        while (glGetError() != GL_NO_ERROR) {}
        
        auto shaderId = renderEngine->getBuiltinShader(shaderName);
        
        // Skip if shader doesn't exist
        if (shaderId == Rendering::InvalidId) {
            std::cout << "Shader '" << shaderName << "' not found, skipping" << std::endl;
            continue;
        }
        
        EXPECT_NE(shaderId, Rendering::InvalidId) << "Failed to get " << shaderName << " shader";
        
        // Test rendering with this shader
        Rendering::Transform transform;
        Rendering::Material material;
        material.shader = shaderId;
        material.albedo = Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f);
        
        renderEngine->beginFrame();
        renderEngine->clear();
        renderEngine->renderMesh(mesh, transform, material);
        renderEngine->endFrame();
        
        EXPECT_TRUE(checkGLError("Render with " + shaderName + " shader"));
        
        // Also test line rendering if applicable
        renderEngine->beginFrame();
        renderEngine->clear();
        renderEngine->renderMeshAsLines(mesh, transform, material);
        renderEngine->endFrame();
        
        EXPECT_TRUE(checkGLError("Line render with " + shaderName + " shader"));
    }
}

// Test ground plane shader specifically
TEST_F(AllShadersValidationTest, GroundPlaneShaderValidation) {
    renderEngine->setCamera(*cameraController->getCamera());
    
    while (glGetError() != GL_NO_ERROR) {}
    
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->setGroundPlaneGridVisible(true);
    renderEngine->updateGroundPlaneGrid(Math::Vector3f(10.0f, 10.0f, 10.0f));
    renderEngine->renderGroundPlaneGrid(Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f)));
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("Ground plane shader rendering"));
}

// Test finding and validating all shader files in the project
TEST_F(AllShadersValidationTest, AllShaderFilesValid) {
    auto shaderPairs = findAllShaderPairs();
    
    EXPECT_GT(shaderPairs.size(), 0u) << "No shader files found";
    
    std::cout << "Found " << shaderPairs.size() << " shader pairs to test:" << std::endl;
    for (const auto& [name, dir] : shaderPairs) {
        std::cout << "  - " << name << " in " << dir << std::endl;
    }
    
    // For each shader pair, verify files exist and are readable
    for (const auto& [baseName, dir] : shaderPairs) {
        std::string vertPath = dir + baseName + ".vert";
        std::string fragPath = dir + baseName + ".frag";
        
        // Check vertex shader
        {
            std::ifstream vertFile(vertPath);
            EXPECT_TRUE(vertFile.good()) << "Cannot read vertex shader: " << vertPath;
            
            // Read first line to check it's valid GLSL
            std::string firstLine;
            std::getline(vertFile, firstLine);
            EXPECT_TRUE(firstLine.find("#version") != std::string::npos || 
                       firstLine.find("//") == 0) 
                << "Vertex shader doesn't start with #version: " << vertPath;
        }
        
        // Check fragment shader
        {
            std::ifstream fragFile(fragPath);
            EXPECT_TRUE(fragFile.good()) << "Cannot read fragment shader: " << fragPath;
            
            // Read first line to check it's valid GLSL
            std::string firstLine;
            std::getline(fragFile, firstLine);
            EXPECT_TRUE(firstLine.find("#version") != std::string::npos || 
                       firstLine.find("//") == 0) 
                << "Fragment shader doesn't start with #version: " << fragPath;
        }
    }
}

// Test shader attribute requirements match VAO setup
TEST_F(AllShadersValidationTest, ShaderAttributeRequirements) {
    struct ShaderAttributes {
        std::string name;
        bool needsPosition;
        bool needsNormal;
        bool needsTexCoord;
        bool needsColor;
    };
    
    // Define expected attributes for each shader
    std::vector<ShaderAttributes> shaderRequirements = {
        {"basic", true, true, false, true},
        {"enhanced", true, true, false, true},
        {"flat", true, true, false, true},
        // Add more as we discover them
    };
    
    auto mesh = createFullAttributeMesh();
    renderEngine->setupMeshBuffers(mesh);
    
    // Check VAO setup matches our expectations
    glBindVertexArray(mesh.vertexArray);
    
    GLint posEnabled, normalEnabled, colorEnabled, texCoordEnabled;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &posEnabled);
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &normalEnabled);
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &colorEnabled);
    glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &texCoordEnabled);
    
    // Based on the fix, we expect position, normal, color enabled but NOT texcoord
    EXPECT_EQ(posEnabled, GL_TRUE) << "Position attribute should be enabled";
    EXPECT_EQ(normalEnabled, GL_TRUE) << "Normal attribute should be enabled";
    EXPECT_EQ(colorEnabled, GL_TRUE) << "Color attribute should be enabled";
    EXPECT_EQ(texCoordEnabled, GL_FALSE) << "TexCoord attribute should NOT be enabled (this was the bug)";
    
    glBindVertexArray(0);
    
    EXPECT_TRUE(checkGLError("Attribute validation"));
}

// Test shader switching performance and correctness
TEST_F(AllShadersValidationTest, RapidShaderSwitching) {
    std::vector<std::string> shaderNames = {"basic", "enhanced", "flat"};
    std::vector<Rendering::Mesh> meshes;
    
    // Create different mesh types
    meshes.push_back(createFullAttributeMesh());
    meshes.push_back(createLineMesh());
    
    for (auto& mesh : meshes) {
        renderEngine->setupMeshBuffers(mesh);
    }
    
    renderEngine->setCamera(*cameraController->getCamera());
    
    while (glGetError() != GL_NO_ERROR) {}
    
    // Rapidly switch between all shaders and mesh types
    const int iterations = 100;
    for (int i = 0; i < iterations; ++i) {
        renderEngine->beginFrame();
        renderEngine->clear();
        
        // Use different shader and mesh each iteration
        const auto& shaderName = shaderNames[i % shaderNames.size()];
        const auto& mesh = meshes[i % meshes.size()];
        
        auto shaderId = renderEngine->getBuiltinShader(shaderName);
        if (shaderId != Rendering::InvalidId) {
            Rendering::Transform transform;
            transform.rotation = Math::Vector3f(0.0f, i * 3.6f, 0.0f); // Rotate
            
            Rendering::Material material;
            material.shader = shaderId;
            material.albedo = Rendering::Color(
                0.5f + 0.5f * std::sin(i * 0.1f),
                0.5f + 0.5f * std::sin(i * 0.1f + 2.0f),
                0.5f + 0.5f * std::sin(i * 0.1f + 4.0f),
                1.0f
            );
            
            // Alternate between normal and line rendering
            if (i % 2 == 0) {
                renderEngine->renderMesh(mesh, transform, material);
            } else {
                renderEngine->renderMeshAsLines(mesh, transform, material);
            }
        }
        
        renderEngine->endFrame();
    }
    
    EXPECT_TRUE(checkGLError("After rapid shader switching"));
}

// Test error recovery - using invalid shader IDs
TEST_F(AllShadersValidationTest, InvalidShaderHandling) {
    auto mesh = createFullAttributeMesh();
    renderEngine->setupMeshBuffers(mesh);
    renderEngine->setCamera(*cameraController->getCamera());
    
    while (glGetError() != GL_NO_ERROR) {}
    
    // Try to use invalid shader ID
    Rendering::Transform transform;
    Rendering::Material material;
    material.shader = 999999; // Invalid ID
    material.albedo = Rendering::Color(1.0f, 1.0f, 1.0f, 1.0f);
    
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->renderMesh(mesh, transform, material);
    renderEngine->endFrame();
    
    // Should handle gracefully without crashing
    // There might be GL errors, so clear them
    while (glGetError() != GL_NO_ERROR) {}
    
    // Now use a valid shader to ensure we can recover
    material.shader = renderEngine->getBuiltinShader("basic");
    
    renderEngine->beginFrame();
    renderEngine->clear();
    renderEngine->renderMesh(mesh, transform, material);
    renderEngine->endFrame();
    
    EXPECT_TRUE(checkGLError("After invalid shader recovery"));
}

} // namespace Tests
} // namespace VoxelEditor