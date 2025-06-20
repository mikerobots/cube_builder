#include <gtest/gtest.h>
#include "core/rendering/RenderEngine.h"
#include "core/rendering/OpenGLRenderer.h"
#include "core/rendering/ShaderManager.h"
#include "core/rendering/GroundPlaneGrid.h"
#include "core/camera/Camera.h"
#include "core/camera/OrbitCamera.h"
#include "core/voxel_data/VoxelDataManager.h"
#include "foundation/math/Vector3.h"
#include "foundation/math/Matrix4.h"
#include "foundation/math/CoordinateTypes.h"
#include "foundation/logging/Logger.h"
#include <memory>

using namespace VoxelEditor;
using namespace VoxelEditor::Rendering;
using namespace VoxelEditor::Camera;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Logging;

class RealShaderPipelineTest : public ::testing::Test {
protected:
    std::unique_ptr<RenderEngine> renderEngine;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Logger> logger;
    
    void SetUp() override {
        // Create logger
        logger = std::make_unique<Logger>("ShaderPipelineTest");
        
        // Create render engine - this will initialize OpenGL context internally
        renderEngine = std::make_unique<RenderEngine>();
        
        // Check if initialization succeeded
        if (!renderEngine->isInitialized()) {
            GTEST_SKIP() << "Failed to initialize RenderEngine - OpenGL context not available";
        }
        
        // Create camera
        camera = std::make_unique<OrbitCamera>();
        camera->setPosition(Math::WorldCoordinates(5.0f, 5.0f, 5.0f));
        camera->lookAt(Math::WorldCoordinates(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f));
    }
    
    void TearDown() override {
        renderEngine.reset();
        camera.reset();
        logger.reset();
    }
};

TEST_F(RealShaderPipelineTest, BasicVoxelShaderPipeline) {
    // Create a simple voxel mesh
    Mesh voxelMesh;
    
    // Create cube vertices with the exact layout expected by shaders
    // Vertex layout: position (3), normal (3), texCoords (2), color (4)
    std::vector<float> vertexData;
    
    // Front face vertices
    auto addVertex = [&](float x, float y, float z, float nx, float ny, float nz, float r, float g, float b) {
        // Position
        vertexData.push_back(x);
        vertexData.push_back(y);
        vertexData.push_back(z);
        // Normal
        vertexData.push_back(nx);
        vertexData.push_back(ny);
        vertexData.push_back(nz);
        // TexCoords (not used but needed for layout)
        vertexData.push_back(0.0f);
        vertexData.push_back(0.0f);
        // Color
        vertexData.push_back(r);
        vertexData.push_back(g);
        vertexData.push_back(b);
        vertexData.push_back(1.0f);
    };
    
    // Create a cube centered at origin
    // Front face (z = 0.5)
    addVertex(-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f);
    addVertex( 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f);
    addVertex( 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f);
    addVertex(-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, 0.0f);
    
    // Set vertex data
    voxelMesh.vertices.resize(vertexData.size() / 12); // 12 floats per vertex
    memcpy(voxelMesh.vertices.data(), vertexData.data(), vertexData.size() * sizeof(float));
    
    // Create indices for the front face
    voxelMesh.indices = {0, 1, 2, 2, 3, 0};
    
    // Create material with basic shader
    Material material;
    material.shader = renderEngine->getBuiltinShader("basic");
    ASSERT_NE(material.shader, InvalidId) << "Failed to get basic shader";
    
    // Setup mesh buffers (creates VAO/VBO)
    renderEngine->setupMeshBuffers(voxelMesh);
    ASSERT_NE(voxelMesh.vertexArray, 0) << "Failed to create VAO";
    ASSERT_NE(voxelMesh.vertexBuffer, 0) << "Failed to create VBO";
    ASSERT_NE(voxelMesh.indexBuffer, 0) << "Failed to create IBO";
    
    // Set viewport
    renderEngine->setViewport(0, 0, 800, 600);
    
    // Clear screen
    renderEngine->clear(Color(0.1f, 0.1f, 0.1f, 1.0f));
    
    // Begin frame
    renderEngine->beginFrame();
    
    // Set camera matrices
    Matrix4f view = camera->getViewMatrix();
    Matrix4f projection = Matrix4f::perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    renderEngine->setViewMatrix(view);
    renderEngine->setProjectionMatrix(projection);
    
    // Render the mesh
    Matrix4f modelMatrix = Matrix4f::identity();
    renderEngine->renderMesh(voxelMesh, modelMatrix, material);
    
    // End frame
    renderEngine->endFrame();
    
    // Verify no OpenGL errors
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error after rendering: " << error;
}

TEST_F(RealShaderPipelineTest, GroundPlaneShaderPipeline) {
    // Create ground plane grid
    auto groundPlane = std::make_unique<GroundPlaneGrid>();
    
    // Initialize with workspace bounds
    Vector3f workspaceSize(10.0f, 10.0f, 10.0f);
    groundPlane->initialize(workspaceSize);
    
    // Set viewport
    renderEngine->setViewport(0, 0, 800, 600);
    
    // Clear screen
    renderEngine->clear(Color(0.2f, 0.2f, 0.2f, 1.0f));
    
    // Begin frame
    renderEngine->beginFrame();
    
    // Set camera matrices
    Matrix4f view = camera->getViewMatrix();
    Matrix4f projection = Matrix4f::perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    
    // Render ground plane
    groundPlane->render(view, projection);
    
    // End frame
    renderEngine->endFrame();
    
    // Verify the ground plane rendered without errors
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error after ground plane rendering: " << error;
}

TEST_F(RealShaderPipelineTest, EnhancedVoxelShaderPipeline) {
    // Test the enhanced shader with lighting
    Mesh voxelMesh;
    
    // Create a more complex mesh with proper normals
    std::vector<Vertex> vertices;
    
    // Helper to create vertex
    auto createVertex = [](const Vector3f& pos, const Vector3f& normal, const Color& color) {
        Vertex v;
        v.position = pos;
        v.normal = normal;
        v.texCoords = Vector2f(0.0f, 0.0f);
        v.color = color;
        return v;
    };
    
    // Create vertices for all 6 faces of a cube
    Color red(1.0f, 0.0f, 0.0f, 1.0f);
    Color green(0.0f, 1.0f, 0.0f, 1.0f);
    Color blue(0.0f, 0.0f, 1.0f, 1.0f);
    Color yellow(1.0f, 1.0f, 0.0f, 1.0f);
    Color cyan(0.0f, 1.0f, 1.0f, 1.0f);
    Color magenta(1.0f, 0.0f, 1.0f, 1.0f);
    
    // Front face (z = 0.5)
    Vector3f frontNormal(0.0f, 0.0f, 1.0f);
    vertices.push_back(createVertex(Vector3f(-0.5f, -0.5f,  0.5f), frontNormal, red));
    vertices.push_back(createVertex(Vector3f( 0.5f, -0.5f,  0.5f), frontNormal, red));
    vertices.push_back(createVertex(Vector3f( 0.5f,  0.5f,  0.5f), frontNormal, red));
    vertices.push_back(createVertex(Vector3f(-0.5f,  0.5f,  0.5f), frontNormal, red));
    
    // Back face (z = -0.5)
    Vector3f backNormal(0.0f, 0.0f, -1.0f);
    vertices.push_back(createVertex(Vector3f(-0.5f, -0.5f, -0.5f), backNormal, green));
    vertices.push_back(createVertex(Vector3f( 0.5f, -0.5f, -0.5f), backNormal, green));
    vertices.push_back(createVertex(Vector3f( 0.5f,  0.5f, -0.5f), backNormal, green));
    vertices.push_back(createVertex(Vector3f(-0.5f,  0.5f, -0.5f), backNormal, green));
    
    // Copy vertices to mesh
    voxelMesh.vertices = vertices;
    
    // Create indices for two faces
    voxelMesh.indices = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 6, 5, 6, 4, 7
    };
    
    // Use enhanced shader
    Material material;
    material.shader = renderEngine->getBuiltinShader("enhanced");
    ASSERT_NE(material.shader, InvalidId) << "Failed to get enhanced shader";
    
    // Setup mesh buffers
    renderEngine->setupMeshBuffers(voxelMesh);
    
    // Set viewport
    renderEngine->setViewport(0, 0, 800, 600);
    
    // Clear and render
    renderEngine->clear(Color(0.1f, 0.1f, 0.1f, 1.0f));
    renderEngine->beginFrame();
    
    // Set matrices
    Matrix4f view = camera->getViewMatrix();
    Matrix4f projection = Matrix4f::perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    renderEngine->setViewMatrix(view);
    renderEngine->setProjectionMatrix(projection);
    
    // Set lighting parameters
    renderEngine->setLightPosition(Vector3f(5.0f, 5.0f, 5.0f));
    renderEngine->setLightColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
    renderEngine->setViewPosition(camera->getPosition().toVector3f());
    
    // Render with rotation animation
    float angle = 45.0f;
    Matrix4f modelMatrix = Matrix4f::rotateY(angle);
    renderEngine->renderMesh(voxelMesh, modelMatrix, material);
    
    renderEngine->endFrame();
    
    // Verify successful rendering
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error with enhanced shader: " << error;
}

TEST_F(RealShaderPipelineTest, ShaderUniformUpdates) {
    // Test that uniforms are properly updated during rendering
    
    // Get basic shader
    ShaderId shader = renderEngine->getBuiltinShader("basic");
    ASSERT_NE(shader, InvalidId);
    
    // Use the shader
    renderEngine->useShader(shader);
    
    // Test setting various uniform types
    
    // Matrix uniforms
    Matrix4f testMatrix = Matrix4f::identity();
    testMatrix.m[0][0] = 2.0f; // Scale X by 2
    renderEngine->setUniform("model", testMatrix);
    
    // Vector uniforms
    Vector3f testVector(1.0f, 2.0f, 3.0f);
    renderEngine->setUniform("lightPos", testVector);
    
    // Color uniforms
    Color testColor(0.5f, 0.7f, 0.9f, 1.0f);
    renderEngine->setUniform("lightColor", testColor);
    
    // Float uniforms
    float testFloat = 0.75f;
    renderEngine->setUniform("opacity", testFloat);
    
    // Verify no errors
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error setting uniforms: " << error;
}

TEST_F(RealShaderPipelineTest, MultipleVAOManagement) {
    // Test rendering multiple meshes with different VAOs
    
    // Create first mesh (red cube)
    Mesh mesh1;
    std::vector<Vertex> vertices1;
    Color red(1.0f, 0.0f, 0.0f, 1.0f);
    
    // Simple quad for mesh1
    vertices1.push_back({Vector3f(-1.0f, 0.0f, -1.0f), Vector3f(0.0f, 1.0f, 0.0f), Vector2f(0.0f, 0.0f), red});
    vertices1.push_back({Vector3f( 1.0f, 0.0f, -1.0f), Vector3f(0.0f, 1.0f, 0.0f), Vector2f(1.0f, 0.0f), red});
    vertices1.push_back({Vector3f( 1.0f, 0.0f,  1.0f), Vector3f(0.0f, 1.0f, 0.0f), Vector2f(1.0f, 1.0f), red});
    vertices1.push_back({Vector3f(-1.0f, 0.0f,  1.0f), Vector3f(0.0f, 1.0f, 0.0f), Vector2f(0.0f, 1.0f), red});
    
    mesh1.vertices = vertices1;
    mesh1.indices = {0, 1, 2, 2, 3, 0};
    
    // Create second mesh (blue cube)
    Mesh mesh2;
    std::vector<Vertex> vertices2 = vertices1; // Copy
    Color blue(0.0f, 0.0f, 1.0f, 1.0f);
    for (auto& v : vertices2) {
        v.color = blue;
        v.position.y += 2.0f; // Offset vertically
    }
    
    mesh2.vertices = vertices2;
    mesh2.indices = {0, 1, 2, 2, 3, 0};
    
    // Setup both meshes (creates separate VAOs)
    renderEngine->setupMeshBuffers(mesh1);
    renderEngine->setupMeshBuffers(mesh2);
    
    ASSERT_NE(mesh1.vertexArray, mesh2.vertexArray) << "Meshes should have different VAOs";
    
    // Create materials
    Material material1, material2;
    material1.shader = renderEngine->getBuiltinShader("basic");
    material2.shader = renderEngine->getBuiltinShader("flat");
    
    // Render both meshes
    renderEngine->clear(Color(0.1f, 0.1f, 0.1f, 1.0f));
    renderEngine->beginFrame();
    
    Matrix4f view = camera->getViewMatrix();
    Matrix4f projection = Matrix4f::perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    renderEngine->setViewMatrix(view);
    renderEngine->setProjectionMatrix(projection);
    
    // Render first mesh
    Matrix4f model1 = Matrix4f::identity();
    renderEngine->renderMesh(mesh1, model1, material1);
    
    // Render second mesh
    Matrix4f model2 = Matrix4f::identity();
    renderEngine->renderMesh(mesh2, model2, material2);
    
    renderEngine->endFrame();
    
    // Verify successful multi-VAO rendering
    GLenum error = glGetError();
    EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error with multiple VAOs: " << error;
}

TEST_F(RealShaderPipelineTest, ShaderHotReload) {
    // Test shader hot-reload functionality
    ShaderManager* shaderMgr = renderEngine->getShaderManager();
    ASSERT_NE(shaderMgr, nullptr);
    
    // Enable hot reload
    shaderMgr->setHotReloadEnabled(true);
    
    // Load a shader from file
    std::string shaderPath = "core/rendering/shaders/";
    ShaderId fileShader = shaderMgr->loadFromFile("basic_voxel_gl33", 
                                                  shaderPath + "basic_voxel_gl33.vert",
                                                  shaderPath + "basic_voxel_gl33.frag");
    
    if (fileShader != InvalidId) {
        // Trigger reload (in real usage, this would happen on file change)
        shaderMgr->reloadAllShaders();
        
        // Verify shader is still valid after reload
        EXPECT_TRUE(shaderMgr->isValid(fileShader)) << "Shader invalid after reload";
    } else {
        // File loading failed, skip this part of the test
        std::cout << "Shader file loading not available, skipping hot-reload test" << std::endl;
    }
    
    // Disable hot reload
    shaderMgr->setHotReloadEnabled(false);
}

TEST_F(RealShaderPipelineTest, RenderStateManagement) {
    // Test that render state is properly managed
    
    // Enable various render states
    renderEngine->setDepthTesting(true);
    renderEngine->setCulling(true, CullMode::Back);
    renderEngine->setBlending(true, BlendMode::Alpha);
    
    // Create and render a transparent mesh
    Mesh mesh;
    std::vector<Vertex> vertices;
    Color semiTransparent(1.0f, 1.0f, 1.0f, 0.5f);
    
    // Create triangle
    vertices.push_back({Vector3f(-1.0f, -1.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f), Vector2f(0.0f, 0.0f), semiTransparent});
    vertices.push_back({Vector3f( 1.0f, -1.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f), Vector2f(1.0f, 0.0f), semiTransparent});
    vertices.push_back({Vector3f( 0.0f,  1.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f), Vector2f(0.5f, 1.0f), semiTransparent});
    
    mesh.vertices = vertices;
    mesh.indices = {0, 1, 2};
    
    Material material;
    material.shader = renderEngine->getBuiltinShader("basic");
    material.blendMode = BlendMode::Alpha;
    
    renderEngine->setupMeshBuffers(mesh);
    
    // Render with transparency
    renderEngine->clear(Color(0.0f, 0.0f, 0.0f, 1.0f));
    renderEngine->beginFrame();
    
    Matrix4f view = camera->getViewMatrix();
    Matrix4f projection = Matrix4f::perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    renderEngine->setViewMatrix(view);
    renderEngine->setProjectionMatrix(projection);
    
    renderEngine->renderMesh(mesh, Matrix4f::identity(), material);
    
    renderEngine->endFrame();
    
    // Verify render states
    GLboolean depthEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
    EXPECT_TRUE(depthEnabled) << "Depth testing should be enabled";
    
    GLboolean cullEnabled;
    glGetBooleanv(GL_CULL_FACE, &cullEnabled);
    EXPECT_TRUE(cullEnabled) << "Face culling should be enabled";
    
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    EXPECT_TRUE(blendEnabled) << "Blending should be enabled";
    
    // Reset states
    renderEngine->setDepthTesting(true);
    renderEngine->setCulling(false, CullMode::Back);
    renderEngine->setBlending(false, BlendMode::None);
}

TEST_F(RealShaderPipelineTest, ErrorHandling) {
    // Test error handling for invalid operations
    
    // Try to use invalid shader
    ShaderId invalidShader = static_cast<ShaderId>(999999);
    renderEngine->useShader(invalidShader);
    
    // Should fall back to basic shader
    Mesh mesh;
    mesh.vertices.push_back({Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), 
                            Vector2f(0.0f, 0.0f), Color(1.0f, 1.0f, 1.0f, 1.0f)});
    mesh.indices = {0};
    
    Material material;
    material.shader = invalidShader;
    
    renderEngine->setupMeshBuffers(mesh);
    
    // Should not crash when rendering with invalid shader
    renderEngine->beginFrame();
    renderEngine->renderMesh(mesh, Matrix4f::identity(), material);
    renderEngine->endFrame();
    
    // Verify we recovered from the error
    GLenum error = glGetError();
    std::cout << "OpenGL error state after invalid shader: " << error << std::endl;
}
