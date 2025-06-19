#include <gtest/gtest.h>
#include "../GroundPlaneGrid.h"
#include "../OpenGLRenderer.h"
#include "../ShaderManager.h"
#include "../RenderConfig.h"
#include <memory>
#include <fstream>

extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>

using namespace VoxelEditor;

class GroundPlaneShaderFileTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<Rendering::OpenGLRenderer> renderer;
    std::unique_ptr<Rendering::ShaderManager> shaderManager;
    
    void SetUp() override {
        // Initialize GLFW
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
        
        window = glfwCreateWindow(100, 100, "Shader Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            FAIL() << "Failed to create GLFW window";
        }
        
        glfwMakeContextCurrent(window);
        
        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            FAIL() << "Failed to initialize GLAD";
        }
        
        // Initialize renderer and shader manager
        renderer = std::make_unique<Rendering::OpenGLRenderer>();
        Rendering::RenderConfig config;
        ASSERT_TRUE(renderer->initializeContext(config));
        
        shaderManager = std::make_unique<Rendering::ShaderManager>(renderer.get());
    }
    
    void TearDown() override {
        shaderManager.reset();
        renderer.reset();
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
};

TEST_F(GroundPlaneShaderFileTest, LoadsShaderFromFiles) {
    // Create ground plane grid
    auto groundPlane = std::make_unique<Rendering::GroundPlaneGrid>(shaderManager.get(), renderer.get());
    
    // Initialize should load shaders from files
    ASSERT_TRUE(groundPlane->initialize());
    
    // The shader should be loaded and registered
    auto shaderId = shaderManager->getShader("ground_plane");
    EXPECT_NE(shaderId, Rendering::InvalidId) << "Shader should be registered after loading from files";
}

TEST_F(GroundPlaneShaderFileTest, ShaderFilesExist) {
    // Check that shader files exist in expected locations
    std::vector<std::string> possiblePaths = {
        "core/rendering/shaders/ground_plane.vert",
        "../core/rendering/shaders/ground_plane.vert",
        "bin/core/rendering/shaders/ground_plane.vert",
        "../../../bin/core/rendering/shaders/ground_plane.vert",  // From CTest working dir
        "../../../../core/rendering/shaders/ground_plane.vert"     // From CTest to source
    };
    
    bool vertexFound = false;
    bool fragmentFound = false;
    std::string foundPath;
    
    for (const auto& basePath : possiblePaths) {
        std::string vertPath = basePath;
        std::string fragPath = basePath;
        fragPath.replace(fragPath.find(".vert"), 5, ".frag");
        
        std::ifstream vertFile(vertPath);
        std::ifstream fragFile(fragPath);
        
        if (vertFile.is_open() && fragFile.is_open()) {
            vertexFound = true;
            fragmentFound = true;
            foundPath = basePath.substr(0, basePath.find_last_of('/'));
            vertFile.close();
            fragFile.close();
            break;
        }
    }
    
    EXPECT_TRUE(vertexFound) << "Ground plane vertex shader file not found in any expected location";
    EXPECT_TRUE(fragmentFound) << "Ground plane fragment shader file not found in any expected location";
    
    if (vertexFound && fragmentFound) {
        std::cout << "Shader files found at: " << foundPath << std::endl;
    }
}

TEST_F(GroundPlaneShaderFileTest, ShaderContentIsValid) {
    // Load and validate shader content
    std::string vertPath = "../core/rendering/shaders/ground_plane.vert";
    std::string fragPath = "../core/rendering/shaders/ground_plane.frag";
    
    // Try alternate path if not found
    std::ifstream testVert(vertPath);
    if (!testVert.is_open()) {
        vertPath = "core/rendering/shaders/ground_plane.vert";
        fragPath = "core/rendering/shaders/ground_plane.frag";
    } else {
        testVert.close();
    }
    
    // Read vertex shader
    std::ifstream vertFile(vertPath);
    if (vertFile.is_open()) {
        std::string vertContent((std::istreambuf_iterator<char>(vertFile)),
                               std::istreambuf_iterator<char>());
        
        // Check for required elements
        EXPECT_NE(vertContent.find("#version"), std::string::npos) 
            << "Vertex shader should have #version directive";
        EXPECT_NE(vertContent.find("layout(location = 0) in vec3 position"), std::string::npos) 
            << "Vertex shader should have position attribute";
        EXPECT_NE(vertContent.find("uniform mat4 mvpMatrix"), std::string::npos) 
            << "Vertex shader should have mvpMatrix uniform";
        
        vertFile.close();
    }
    
    // Read fragment shader
    std::ifstream fragFile(fragPath);
    if (fragFile.is_open()) {
        std::string fragContent((std::istreambuf_iterator<char>(fragFile)),
                               std::istreambuf_iterator<char>());
        
        // Check for required elements
        EXPECT_NE(fragContent.find("#version"), std::string::npos) 
            << "Fragment shader should have #version directive";
        EXPECT_NE(fragContent.find("uniform vec3 minorLineColor"), std::string::npos) 
            << "Fragment shader should have minorLineColor uniform";
        EXPECT_NE(fragContent.find("uniform vec3 majorLineColor"), std::string::npos) 
            << "Fragment shader should have majorLineColor uniform";
        EXPECT_NE(fragContent.find("uniform float opacity"), std::string::npos) 
            << "Fragment shader should have opacity uniform";
        
        fragFile.close();
    }
}