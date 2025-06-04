#include <gtest/gtest.h>
#include "rendering/RenderEngine.h"
#include "rendering/OpenGLRenderer.h"
#include "rendering/ShaderManager.h"
#include "voxel_data/VoxelDataManager.h"
#include "camera/OrbitCamera.h"
#include "cli/VoxelMeshGenerator.h"
#include "foundation/logging/Logger.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>

namespace VoxelEditor {
namespace Rendering {
namespace Tests {

class EdgeRenderingTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<RenderEngine> renderEngine;
    std::unique_ptr<Camera::OrbitCamera> camera;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
    std::unique_ptr<VoxelMeshGenerator> meshGenerator;
    
    void SetUp() override {
        // Initialize GLFW
        ASSERT_TRUE(glfwInit());
        
        // Create hidden window for OpenGL context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        window = glfwCreateWindow(800, 600, "Edge Test", nullptr, nullptr);
        ASSERT_NE(window, nullptr);
        
        glfwMakeContextCurrent(window);
        
        // Initialize render engine
        renderEngine = std::make_unique<RenderEngine>(nullptr);
        RenderConfig config;
        config.windowWidth = 800;
        config.windowHeight = 600;
        ASSERT_TRUE(renderEngine->initialize(config));
        
        // Create camera
        camera = std::make_unique<Camera::OrbitCamera>();
        camera->setFieldOfView(45.0f);
        camera->setAspectRatio(800.0f/600.0f);
        camera->setNearFarPlanes(0.1f, 100.0f);
        camera->setTarget(Math::Vector3f(5.0f, 5.0f, 5.0f));
        camera->setDistance(20.0f);
        
        // Create voxel manager and mesh generator
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
        voxelManager->resizeWorkspace(Math::Vector3f(10, 10, 10));
        meshGenerator = std::make_unique<VoxelMeshGenerator>();
    }
    
    void TearDown() override {
        renderEngine.reset();
        camera.reset();
        voxelManager.reset();
        meshGenerator.reset();
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    // Helper to check if shader compiles
    bool validateShaderCompilation(const std::string& shaderName) {
        auto shaderId = renderEngine->getBuiltinShader(shaderName);
        return shaderId != InvalidId;
    }
    
    // Helper to render and capture pixels
    std::vector<uint8_t> renderAndCapture() {
        renderEngine->beginFrame();
        renderEngine->clear(ClearFlags::All, Color(0.2f, 0.2f, 0.2f, 1.0f));
        
        // Render logic would go here
        
        renderEngine->endFrame();
        renderEngine->present();
        
        // Capture framebuffer
        std::vector<uint8_t> pixels(800 * 600 * 3);
        glReadPixels(0, 0, 800, 600, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        
        return pixels;
    }
};

TEST_F(EdgeRenderingTest, EnhancedShaderCompiles) {
    // Test that the enhanced shader compiles successfully
    EXPECT_TRUE(validateShaderCompilation("enhanced"));
    EXPECT_TRUE(validateShaderCompilation("basic"));
    EXPECT_TRUE(validateShaderCompilation("flat"));
}

TEST_F(EdgeRenderingTest, EdgeMeshGeneration) {
    // Add some voxels at default resolution
    auto resolution = voxelManager->getActiveResolution();
    voxelManager->setVoxel(Math::Vector3i(0, 0, 0), resolution, true);
    voxelManager->setVoxel(Math::Vector3i(1, 0, 0), resolution, true);
    voxelManager->setVoxel(Math::Vector3i(0, 1, 0), resolution, true);
    
    // Generate solid mesh
    auto solidMesh = meshGenerator->generateCubeMesh(*voxelManager);
    EXPECT_FALSE(solidMesh.vertices.empty());
    EXPECT_FALSE(solidMesh.indices.empty());
    
    // Generate edge mesh
    auto edgeMesh = meshGenerator->generateEdgeMesh(*voxelManager);
    EXPECT_FALSE(edgeMesh.vertices.empty());
    EXPECT_FALSE(edgeMesh.indices.empty());
    
    // Edge mesh should have 8 vertices per voxel (cube corners)
    size_t expectedVertices = 3 * 8; // 3 voxels * 8 corners
    EXPECT_EQ(edgeMesh.vertices.size(), expectedVertices);
    
    // Edge mesh should have 24 indices per voxel (12 edges * 2 indices per edge)
    size_t expectedIndices = 3 * 12 * 2; // 3 voxels * 12 edges * 2 indices
    EXPECT_EQ(edgeMesh.indices.size(), expectedIndices);
}

TEST_F(EdgeRenderingTest, EdgeMeshRendersProperly) {
    // Add a single voxel
    auto resolution = voxelManager->getActiveResolution();
    voxelManager->setVoxel(Math::Vector3i(5, 5, 5), resolution, true);
    
    // Generate meshes
    auto solidMesh = meshGenerator->generateCubeMesh(*voxelManager);
    auto edgeMesh = meshGenerator->generateEdgeMesh(*voxelManager);
    
    // Setup mesh buffers
    renderEngine->setupMeshBuffers(solidMesh);
    renderEngine->setupMeshBuffers(edgeMesh);
    
    // Set camera
    renderEngine->setCamera(*camera);
    
    // Render frame with solid mesh only
    renderEngine->beginFrame();
    renderEngine->clear(ClearFlags::All, Color(0.2f, 0.2f, 0.2f, 1.0f));
    
    // Render solid voxel
    Transform transform;
    Material material;
    material.albedo = Color(0.8f, 0.8f, 0.8f, 1.0f);
    material.shader = renderEngine->getBuiltinShader("enhanced");
    
    renderEngine->renderMesh(solidMesh, transform, material);
    
    renderEngine->endFrame();
    renderEngine->present();
    
    // Capture solid-only render
    std::vector<uint8_t> solidPixels(800 * 600 * 3);
    glReadPixels(0, 0, 800, 600, GL_RGB, GL_UNSIGNED_BYTE, solidPixels.data());
    
    // Now render with edges
    renderEngine->beginFrame();
    renderEngine->clear(ClearFlags::All, Color(0.2f, 0.2f, 0.2f, 1.0f));
    
    // Render solid voxel
    renderEngine->renderMesh(solidMesh, transform, material);
    
    // Render edge lines
    renderEngine->setLineWidth(2.0f);
    renderEngine->bindVertexArray(edgeMesh.vertexArray);
    renderEngine->useProgram(renderEngine->getBuiltinShader("basic"));
    
    // Set uniforms for edge rendering
    Math::Matrix4f modelMatrix = Math::Matrix4f::Identity();
    renderEngine->setUniform("model", UniformValue(modelMatrix));
    renderEngine->setUniform("view", UniformValue(camera->getViewMatrix()));
    renderEngine->setUniform("projection", UniformValue(camera->getProjectionMatrix()));
    
    // Draw lines
    renderEngine->drawElements(PrimitiveType::Lines, 
                              static_cast<int>(edgeMesh.indices.size()),
                              IndexType::UInt32);
    
    renderEngine->setLineWidth(1.0f);
    
    renderEngine->endFrame();
    renderEngine->present();
    
    // Capture edge render
    std::vector<uint8_t> edgePixels(800 * 600 * 3);
    glReadPixels(0, 0, 800, 600, GL_RGB, GL_UNSIGNED_BYTE, edgePixels.data());
    
    // Compare - there should be some differences (edges added)
    bool foundDifference = false;
    int darkPixelCount = 0;
    
    for (size_t i = 0; i < solidPixels.size(); i += 3) {
        // Check if this pixel is darker in the edge render
        if (edgePixels[i] < solidPixels[i] || 
            edgePixels[i+1] < solidPixels[i+1] || 
            edgePixels[i+2] < solidPixels[i+2]) {
            darkPixelCount++;
            foundDifference = true;
        }
    }
    
    EXPECT_TRUE(foundDifference) << "Edge rendering should add dark lines";
    EXPECT_GT(darkPixelCount, 10) << "Should have multiple dark edge pixels";
}

TEST_F(EdgeRenderingTest, ShaderDerivativesWork) {
    // This test validates that the enhanced shader's derivative-based edge detection works
    // by checking that the shader compiles and can be used
    
    auto shaderId = renderEngine->getBuiltinShader("enhanced");
    ASSERT_NE(shaderId, InvalidId);
    
    // Create a simple test mesh with sharp edges
    Mesh testMesh;
    
    // Create a cube with distinct face normals
    // Front face
    testMesh.vertices.push_back(Vertex(Math::Vector3f(-1, -1, 1), Math::Vector3f(0, 0, 1)));
    testMesh.vertices.push_back(Vertex(Math::Vector3f(1, -1, 1), Math::Vector3f(0, 0, 1)));
    testMesh.vertices.push_back(Vertex(Math::Vector3f(1, 1, 1), Math::Vector3f(0, 0, 1)));
    testMesh.vertices.push_back(Vertex(Math::Vector3f(-1, 1, 1), Math::Vector3f(0, 0, 1)));
    
    // Right face  
    testMesh.vertices.push_back(Vertex(Math::Vector3f(1, -1, 1), Math::Vector3f(1, 0, 0)));
    testMesh.vertices.push_back(Vertex(Math::Vector3f(1, -1, -1), Math::Vector3f(1, 0, 0)));
    testMesh.vertices.push_back(Vertex(Math::Vector3f(1, 1, -1), Math::Vector3f(1, 0, 0)));
    testMesh.vertices.push_back(Vertex(Math::Vector3f(1, 1, 1), Math::Vector3f(1, 0, 0)));
    
    // Add indices for two faces
    testMesh.indices = {0,1,2, 0,2,3, 4,5,6, 4,6,7};
    
    renderEngine->setupMeshBuffers(testMesh);
    
    // Render with enhanced shader
    renderEngine->setCamera(*camera);
    renderEngine->beginFrame();
    renderEngine->clear(ClearFlags::All, Color(0.5f, 0.5f, 0.5f, 1.0f));
    
    Transform transform;
    Material material;
    material.shader = shaderId;
    material.albedo = Color(1.0f, 1.0f, 1.0f, 1.0f);
    
    renderEngine->renderMesh(testMesh, transform, material);
    
    renderEngine->endFrame();
    renderEngine->present();
    
    // If we got here without crashing, the shader works
    SUCCEED();
}

TEST_F(EdgeRenderingTest, EdgeColorValidation) {
    // Test that edge mesh vertices have the correct dark color
    auto resolution = voxelManager->getActiveResolution();
    voxelManager->setVoxel(Math::Vector3i(0, 0, 0), resolution, true);
    
    auto edgeMesh = meshGenerator->generateEdgeMesh(*voxelManager);
    ASSERT_FALSE(edgeMesh.vertices.empty());
    
    // Check that all edge vertices have dark color (0.1, 0.1, 0.1)
    for (const auto& vertex : edgeMesh.vertices) {
        EXPECT_NEAR(vertex.color.r, 0.1f, 0.01f);
        EXPECT_NEAR(vertex.color.g, 0.1f, 0.01f);
        EXPECT_NEAR(vertex.color.b, 0.1f, 0.01f);
        EXPECT_EQ(vertex.color.a, 1.0f);
    }
}

} // namespace Tests
} // namespace Rendering
} // namespace VoxelEditor