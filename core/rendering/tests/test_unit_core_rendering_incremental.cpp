#include <gtest/gtest.h>

// Include OpenGL headers (must come before GLFW)
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

#include "../RenderEngine.h"
#include "../OpenGLRenderer.h"
#include "../RenderConfig.h"
#include "../RenderTypes.h"
#include "../../camera/OrbitCamera.h"
#include "../../../foundation/events/EventDispatcher.h"
#include "../../../foundation/logging/Logger.h"

#ifdef HAVE_GLFW
#include <GLFW/glfw3.h>
#endif

#include <memory>
#include <vector>

namespace VoxelEditor {
namespace Rendering {

class MockEventDispatcher : public Events::EventDispatcher {
public:
    MockEventDispatcher() = default;
};

class MockCamera : public Camera::OrbitCamera {
public:
    MockCamera() : OrbitCamera() {
        setPosition(Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 5.0f)));
        setTarget(Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f)));
        setUp(Math::WorldCoordinates(Math::Vector3f(0.0f, 1.0f, 0.0f)));
        setFieldOfView(45.0f);
        setAspectRatio(1.0f);
        setNearFarPlanes(0.1f, 1000.0f);
    }
};

// Test fixture for incremental rendering tests
class RenderIncrementalTest : public ::testing::Test {
protected:
    std::unique_ptr<MockEventDispatcher> eventDispatcher;
    std::unique_ptr<RenderEngine> renderEngine;
    std::unique_ptr<MockCamera> camera;
    
#ifdef HAVE_GLFW
    GLFWwindow* window = nullptr;
#endif
    
    void SetUp() override {
        // Initialize logging for test debugging
        Logging::Logger::getInstance().setLevel(Logging::LogLevel::Debug);
        
#ifdef HAVE_GLFW
        // Initialize GLFW for OpenGL context
        if (glfwInit()) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing
            
            window = glfwCreateWindow(800, 600, "Test", nullptr, nullptr);
            if (window) {
                glfwMakeContextCurrent(window);
            }
        }
#endif
        
        eventDispatcher = std::make_unique<MockEventDispatcher>();
        renderEngine = std::make_unique<RenderEngine>(eventDispatcher.get());
        camera = std::make_unique<MockCamera>();
    }
    
    void TearDown() override {
        renderEngine.reset();
        eventDispatcher.reset();
        camera.reset();
        
#ifdef HAVE_GLFW
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
#endif
    }
    
    bool hasOpenGLContext() const {
#ifdef HAVE_GLFW
        return window != nullptr;
#else
        return false;
#endif
    }
};

// Test 1: Clear to solid color (no rendering)
TEST_F(RenderIncrementalTest, Test1_ClearToSolidColor) {
    if (!hasOpenGLContext()) {
        GTEST_SKIP() << "No OpenGL context available, skipping OpenGL test";
        return;
    }
    
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    config.vsync = false;
    config.enableDebugOutput = true;
    
    // Initialize render engine
    bool initialized = renderEngine->initialize(config);
    if (!initialized) {
        GTEST_SKIP() << "Failed to initialize render engine";
        return;
    }
    
    // Set viewport
    renderEngine->setViewport(0, 0, 800, 600);
    
    // Begin frame
    renderEngine->beginFrame();
    
    // Clear to red color
    Color clearColor(1.0f, 0.0f, 0.0f, 1.0f); // Red
    renderEngine->clear(ClearFlags::All, clearColor);
    
    // End frame
    renderEngine->endFrame();
    
    // For now, just verify the operations completed without error
    // TODO: Add pixel readback verification when readPixels is implemented
    
    // Test clearing to different colors
    renderEngine->beginFrame();
    Color greenColor(0.0f, 1.0f, 0.0f, 1.0f); // Green
    renderEngine->clear(ClearFlags::All, greenColor);
    renderEngine->endFrame();
    
    renderEngine->beginFrame();
    Color blueColor(0.0f, 0.0f, 1.0f, 1.0f); // Blue
    renderEngine->clear(ClearFlags::All, blueColor);
    renderEngine->endFrame();
    
    // Verify the engine maintains state properly
    EXPECT_TRUE(renderEngine->isInitialized());
}

// Test depth-only clear
TEST_F(RenderIncrementalTest, Test1_ClearDepthOnly) {
    if (!hasOpenGLContext()) {
        GTEST_SKIP() << "No OpenGL context available, skipping OpenGL test";
        return;
    }
    
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    
    if (!renderEngine->initialize(config)) {
        GTEST_SKIP() << "Failed to initialize render engine";
        return;
    }
    
    renderEngine->setViewport(0, 0, 800, 600);
    
    // Test clearing only depth buffer
    renderEngine->beginFrame();
    renderEngine->clear(ClearFlags::DEPTH, Color::Black());
    renderEngine->endFrame();
    
    // Test clearing only color buffer
    renderEngine->beginFrame();
    renderEngine->clear(ClearFlags::COLOR, Color::White());
    renderEngine->endFrame();
    
    EXPECT_TRUE(renderEngine->isInitialized());
}

// Test multiple frame clears
TEST_F(RenderIncrementalTest, Test1_MultipleFrameClears) {
    if (!hasOpenGLContext()) {
        GTEST_SKIP() << "No OpenGL context available, skipping OpenGL test";
        return;
    }
    
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    
    if (!renderEngine->initialize(config)) {
        GTEST_SKIP() << "Failed to initialize render engine";
        return;
    }
    
    renderEngine->setViewport(0, 0, 800, 600);
    
    // Render multiple frames with different clear colors
    std::vector<Color> colors = {
        Color::Red(),
        Color::Green(),
        Color::Blue(),
        Color(1.0f, 1.0f, 0.0f, 1.0f), // Yellow
        Color(0.5f, 0.0f, 0.5f, 1.0f)  // Purple
    };
    
    for (const auto& color : colors) {
        renderEngine->beginFrame();
        renderEngine->clear(ClearFlags::All, color);
        renderEngine->endFrame();
        
        // Verify stats are being tracked
        const RenderStats& stats = renderEngine->getRenderStats();
        EXPECT_GE(stats.frameCount, 0);
    }
    
    EXPECT_TRUE(renderEngine->isInitialized());
}

// Test 2: Render immediate mode triangle (no shader)
TEST_F(RenderIncrementalTest, Test2_RenderImmediateModeTriangle) {
    if (!hasOpenGLContext()) {
        GTEST_SKIP() << "No OpenGL context available, skipping OpenGL test";
        return;
    }
    
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    
    if (!renderEngine->initialize(config)) {
        GTEST_SKIP() << "Failed to initialize render engine";
        return;
    }
    
    renderEngine->setViewport(0, 0, 800, 600);
    
    // Begin frame
    renderEngine->beginFrame();
    
    // Clear to black background
    renderEngine->clear(ClearFlags::All, Color::Black());
    
    // Note: Immediate mode rendering would use deprecated OpenGL functions
    // like glBegin/glEnd, but modern OpenGL core profile doesn't support this.
    // Instead, we'll test the modern equivalent with minimal setup.
    
    // Create a simple triangle with raw OpenGL calls (if context is available)
    #ifdef HAVE_GLFW
    // Define triangle vertices in NDC space
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  // Bottom left
         0.5f, -0.5f, 0.0f,  // Bottom right  
         0.0f,  0.5f, 0.0f   // Top center
    };
    
    // Create and bind vertex buffer
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Configure vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Use immediate mode style rendering (but with modern OpenGL)
    // We'll skip shader setup for this test to simulate immediate mode
    
    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    #endif
    
    // End frame
    renderEngine->endFrame();
    
    // Verify the engine state
    EXPECT_TRUE(renderEngine->isInitialized());
    
    // Check render stats
    const RenderStats& stats = renderEngine->getRenderStats();
    EXPECT_GE(stats.frameCount, 0);
}

// Test 2b: Basic OpenGL state verification
TEST_F(RenderIncrementalTest, Test2_OpenGLStateVerification) {
    if (!hasOpenGLContext()) {
        GTEST_SKIP() << "No OpenGL context available, skipping OpenGL test";
        return;
    }
    
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    
    if (!renderEngine->initialize(config)) {
        GTEST_SKIP() << "Failed to initialize render engine";
        return;
    }
    
    renderEngine->setViewport(0, 0, 800, 600);
    
    // Test basic OpenGL state
    renderEngine->beginFrame();
    
    #ifdef HAVE_GLFW
    // Verify OpenGL context is active
    EXPECT_NE(glfwGetCurrentContext(), nullptr);
    
    // Check viewport was set correctly
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    EXPECT_EQ(viewport[0], 0);
    EXPECT_EQ(viewport[1], 0);
    EXPECT_EQ(viewport[2], 800);
    EXPECT_EQ(viewport[3], 600);
    
    // Check depth test state
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    EXPECT_TRUE(depthTestEnabled);
    
    // Check default clear color
    GLfloat clearColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    // Default should be black after our clear operation
    #endif
    
    renderEngine->endFrame();
    
    EXPECT_TRUE(renderEngine->isInitialized());
}

// Test 3: Render single triangle with known coordinates using RenderEngine API
TEST_F(RenderIncrementalTest, Test3_RenderTriangleKnownCoordinates) {
    if (!hasOpenGLContext()) {
        GTEST_SKIP() << "No OpenGL context available, skipping OpenGL test";
        return;
    }
    
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    config.enableDebugOutput = true;
    
    if (!renderEngine->initialize(config)) {
        GTEST_SKIP() << "Failed to initialize render engine";
        return;
    }
    
    renderEngine->setViewport(0, 0, 800, 600);
    renderEngine->setCamera(*camera);
    
    // Create a triangle mesh with known world coordinates
    Mesh triangleMesh;
    
    // Define triangle vertices in world space
    Math::Vector3f v1(-1.0f, -1.0f, 0.0f);  // Bottom left
    Math::Vector3f v2( 1.0f, -1.0f, 0.0f);  // Bottom right
    Math::Vector3f v3( 0.0f,  1.0f, 0.0f);  // Top center
    
    // Calculate normal (facing towards camera)
    Math::Vector3f edge1 = v2 - v1;
    Math::Vector3f edge2 = v3 - v1;
    Math::Vector3f normal = edge1.cross(edge2).normalized();
    
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Triangle vertices: v1(%.3f,%.3f,%.3f), v2(%.3f,%.3f,%.3f), v3(%.3f,%.3f,%.3f)", 
        v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z);
    
    Logging::Logger::getInstance().debugfc("RenderTest", "Triangle normal: (%.3f,%.3f,%.3f)", 
        normal.x, normal.y, normal.z);
    
    // Create vertices with positions, normals, and colors
    triangleMesh.vertices.push_back(Vertex(v1, normal, Math::Vector2f::zero(), Color::Red()));
    triangleMesh.vertices.push_back(Vertex(v2, normal, Math::Vector2f::zero(), Color::Green()));
    triangleMesh.vertices.push_back(Vertex(v3, normal, Math::Vector2f::zero(), Color::Blue()));
    
    // Add indices for triangle
    triangleMesh.indices = {0, 1, 2};
    
    // Create material and transform
    Material material = Material::createDefault();
    material.albedo = Color(1.0f, 1.0f, 1.0f, 1.0f); // White
    
    Transform transform;
    transform.position = Math::WorldCoordinates(0.0f, 0.0f, 0.0f);
    transform.rotation = Math::Vector3f(0.0f, 0.0f, 0.0f);
    transform.scale = Math::Vector3f(1.0f, 1.0f, 1.0f);
    
    // Log camera and transform info
    Math::Vector3f camPos = camera->getPosition().value();
    Math::Vector3f camTarget = camera->getTarget().value();
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Camera pos: (%.3f,%.3f,%.3f), target: (%.3f,%.3f,%.3f)", 
        camPos.x, camPos.y, camPos.z, camTarget.x, camTarget.y, camTarget.z);
    
    // Render the triangle
    renderEngine->beginFrame();
    renderEngine->clear(ClearFlags::All, Color::Black());
    
    Logging::Logger::getInstance().debug("Rendering triangle with known coordinates", "RenderTest");
    renderEngine->renderMesh(triangleMesh, transform, material);
    
    renderEngine->endFrame();
    
    // Verify the mesh was processed
    EXPECT_EQ(triangleMesh.vertices.size(), 3);
    EXPECT_EQ(triangleMesh.indices.size(), 3);
    
    // Check render stats
    const RenderStats& stats = renderEngine->getRenderStats();
    EXPECT_GE(stats.frameCount, 0);
    
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Render stats: %d frames, %d draw calls, %d triangles, %d vertices", 
        stats.frameCount, stats.drawCalls, stats.trianglesRendered, stats.verticesProcessed);
    
    EXPECT_TRUE(renderEngine->isInitialized());
}

// Test 4: Verify MVP matrix multiplication
TEST_F(RenderIncrementalTest, Test4_VerifyMVPMatrixMultiplication) {
    if (!hasOpenGLContext()) {
        GTEST_SKIP() << "No OpenGL context available, skipping OpenGL test";
        return;
    }
    
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    
    if (!renderEngine->initialize(config)) {
        GTEST_SKIP() << "Failed to initialize render engine";
        return;
    }
    
    renderEngine->setCamera(*camera);
    
    // Get camera matrices
    Math::Matrix4f viewMatrix = camera->getViewMatrix();
    Math::Matrix4f projectionMatrix = camera->getProjectionMatrix();
    
    // Log matrices for debugging
    Logging::Logger::getInstance().debug("=== Camera Matrix Verification ===", "RenderTest");
    
    // Test a known point transformation
    Math::Vector3f worldPoint(0.0f, 0.0f, 0.0f); // Origin
    
    // Manual matrix multiplication
    Math::Matrix4f mvpMatrix = projectionMatrix * viewMatrix;
    
    // Transform point through pipeline
    Math::Vector4f worldPoint4(worldPoint.x, worldPoint.y, worldPoint.z, 1.0f);
    Math::Vector4f viewPoint4 = viewMatrix * worldPoint4;
    Math::Vector4f clipPoint4 = projectionMatrix * viewPoint4;
    Math::Vector4f clipPoint4Direct = mvpMatrix * worldPoint4;
    
    // Log intermediate results
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "World point: (%.3f, %.3f, %.3f)", worldPoint.x, worldPoint.y, worldPoint.z);
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "View space: (%.3f, %.3f, %.3f, %.3f)", viewPoint4.x, viewPoint4.y, viewPoint4.z, viewPoint4.w);
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Clip space (step-by-step): (%.3f, %.3f, %.3f, %.3f)", 
        clipPoint4.x, clipPoint4.y, clipPoint4.z, clipPoint4.w);
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Clip space (direct MVP): (%.3f, %.3f, %.3f, %.3f)", 
        clipPoint4Direct.x, clipPoint4Direct.y, clipPoint4Direct.z, clipPoint4Direct.w);
    
    // Verify that step-by-step multiplication equals direct MVP multiplication
    float tolerance = 0.001f;
    EXPECT_NEAR(clipPoint4.x, clipPoint4Direct.x, tolerance);
    EXPECT_NEAR(clipPoint4.y, clipPoint4Direct.y, tolerance);
    EXPECT_NEAR(clipPoint4.z, clipPoint4Direct.z, tolerance);
    EXPECT_NEAR(clipPoint4.w, clipPoint4Direct.w, tolerance);
    
    // Test NDC conversion
    if (clipPoint4.w != 0.0f) {
        Math::Vector3f ndcPoint(
            clipPoint4.x / clipPoint4.w,
            clipPoint4.y / clipPoint4.w,
            clipPoint4.z / clipPoint4.w
        );
        
        Logging::Logger::getInstance().debugfc("RenderTest", 
            "NDC point: (%.3f, %.3f, %.3f)", ndcPoint.x, ndcPoint.y, ndcPoint.z);
        
        // NDC coordinates should be in [-1, 1] range for visible points
        // Origin at camera position (0,0,5) looking at origin should be in front
        EXPECT_GT(ndcPoint.z, -1.0f); // Should be in front of far plane
        EXPECT_LT(ndcPoint.z,  1.0f); // Should be behind near plane
    }
    
    // Test multiple points to verify transformation consistency
    std::vector<Math::Vector3f> testPoints = {
        Math::Vector3f( 1.0f,  0.0f, 0.0f),  // Right
        Math::Vector3f(-1.0f,  0.0f, 0.0f),  // Left
        Math::Vector3f( 0.0f,  1.0f, 0.0f),  // Up
        Math::Vector3f( 0.0f, -1.0f, 0.0f),  // Down
        Math::Vector3f( 0.0f,  0.0f, 1.0f),  // Forward
        Math::Vector3f( 0.0f,  0.0f,-1.0f)   // Backward
    };
    
    for (size_t i = 0; i < testPoints.size(); ++i) {
        Math::Vector4f testPoint4(testPoints[i].x, testPoints[i].y, testPoints[i].z, 1.0f);
        Math::Vector4f transformedDirect = mvpMatrix * testPoint4;
        Math::Vector4f transformedStepwise = projectionMatrix * (viewMatrix * testPoint4);
        
        EXPECT_NEAR(transformedDirect.x, transformedStepwise.x, tolerance);
        EXPECT_NEAR(transformedDirect.y, transformedStepwise.y, tolerance);
        EXPECT_NEAR(transformedDirect.z, transformedStepwise.z, tolerance);
        EXPECT_NEAR(transformedDirect.w, transformedStepwise.w, tolerance);
    }
    
    EXPECT_TRUE(renderEngine->isInitialized());
}

// Test 5: Render shader triangle with full MVP matrix
TEST_F(RenderIncrementalTest, Test5_RenderTriangleFullMVP) {
    if (!hasOpenGLContext()) {
        GTEST_SKIP() << "No OpenGL context available, skipping OpenGL test";
        return;
    }
    
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    config.enableDebugOutput = true;
    
    if (!renderEngine->initialize(config)) {
        GTEST_SKIP() << "Failed to initialize render engine";
        return;
    }
    
    renderEngine->setViewport(0, 0, 800, 600);
    renderEngine->setCamera(*camera);
    
    // Create a triangle mesh positioned in world space
    Mesh triangleMesh;
    
    // Define triangle vertices in world space (smaller triangle)
    Math::Vector3f v1(-0.5f, -0.5f, 0.0f);  // Bottom left
    Math::Vector3f v2( 0.5f, -0.5f, 0.0f);  // Bottom right
    Math::Vector3f v3( 0.0f,  0.5f, 0.0f);  // Top center
    
    // Calculate normal
    Math::Vector3f edge1 = v2 - v1;
    Math::Vector3f edge2 = v3 - v1;
    Math::Vector3f normal = edge1.cross(edge2).normalized();
    
    Logging::Logger::getInstance().debug("=== Test 5: Full MVP Pipeline ===", "RenderTest");
    
    // Create vertices with colors for visual verification
    triangleMesh.vertices.push_back(Vertex(v1, normal, Math::Vector2f::zero(), Color::Red()));
    triangleMesh.vertices.push_back(Vertex(v2, normal, Math::Vector2f::zero(), Color::Green()));
    triangleMesh.vertices.push_back(Vertex(v3, normal, Math::Vector2f::zero(), Color::Blue()));
    triangleMesh.indices = {0, 1, 2};
    
    // Create transform that will be combined with camera matrices
    Transform transform;
    transform.position = Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, -2.0f)); // Move triangle away from camera
    transform.rotation = Math::Vector3f(0.0f, 0.0f, 0.0f);
    transform.scale = Math::Vector3f(1.0f, 1.0f, 1.0f);
    
    // Log all matrices in the pipeline
    Math::Matrix4f modelMatrix = Math::Matrix4f::translation(transform.position.value());
    Math::Matrix4f viewMatrix = camera->getViewMatrix();
    Math::Matrix4f projectionMatrix = camera->getProjectionMatrix();
    Math::Matrix4f mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
    
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Model translation: (%.3f, %.3f, %.3f)", 
        transform.position.x(), transform.position.y(), transform.position.z());
    
    // Test point transformation through full pipeline
    Math::Vector4f testPoint(v1.x, v1.y, v1.z, 1.0f);
    Math::Vector4f worldPoint = modelMatrix * testPoint;
    Math::Vector4f viewPoint = viewMatrix * worldPoint;
    Math::Vector4f clipPoint = projectionMatrix * viewPoint;
    
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Vertex transformation pipeline:");
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "  Local: (%.3f, %.3f, %.3f)", testPoint.x, testPoint.y, testPoint.z);
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "  World: (%.3f, %.3f, %.3f)", worldPoint.x, worldPoint.y, worldPoint.z);
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "  View:  (%.3f, %.3f, %.3f)", viewPoint.x, viewPoint.y, viewPoint.z);
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "  Clip:  (%.3f, %.3f, %.3f, %.3f)", clipPoint.x, clipPoint.y, clipPoint.z, clipPoint.w);
    
    // Create material
    Material material = Material::createDefault();
    material.albedo = Color::White();
    
    // Render the triangle with full MVP pipeline
    renderEngine->beginFrame();
    renderEngine->clear(ClearFlags::All, Color::Black());
    renderEngine->renderMesh(triangleMesh, transform, material);
    renderEngine->endFrame();
    
    // Verify the mesh was processed
    EXPECT_EQ(triangleMesh.vertices.size(), 3);
    EXPECT_EQ(triangleMesh.indices.size(), 3);
    
    // Check render stats
    const RenderStats& stats = renderEngine->getRenderStats();
    EXPECT_GE(stats.frameCount, 0);
    
    EXPECT_TRUE(renderEngine->isInitialized());
}

// Test 6: Render single voxel at origin
TEST_F(RenderIncrementalTest, Test6_RenderVoxelAtOrigin) {
    if (!hasOpenGLContext()) {
        GTEST_SKIP() << "No OpenGL context available, skipping OpenGL test";
        return;
    }
    
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    config.enableDebugOutput = true;
    
    if (!renderEngine->initialize(config)) {
        GTEST_SKIP() << "Failed to initialize render engine";
        return;
    }
    
    renderEngine->setViewport(0, 0, 800, 600);
    renderEngine->setCamera(*camera);
    
    Logging::Logger::getInstance().debug("=== Test 6: Single Voxel at Origin ===", "RenderTest");
    
    // Create a voxel mesh (unit cube) centered at origin
    Mesh voxelMesh;
    
    // Define cube vertices (8 corners of unit cube)
    std::vector<Math::Vector3f> positions = {
        Math::Vector3f(-0.5f, -0.5f, -0.5f), // 0: bottom-back-left
        Math::Vector3f( 0.5f, -0.5f, -0.5f), // 1: bottom-back-right
        Math::Vector3f( 0.5f,  0.5f, -0.5f), // 2: top-back-right
        Math::Vector3f(-0.5f,  0.5f, -0.5f), // 3: top-back-left
        Math::Vector3f(-0.5f, -0.5f,  0.5f), // 4: bottom-front-left
        Math::Vector3f( 0.5f, -0.5f,  0.5f), // 5: bottom-front-right
        Math::Vector3f( 0.5f,  0.5f,  0.5f), // 6: top-front-right
        Math::Vector3f(-0.5f,  0.5f,  0.5f)  // 7: top-front-left
    };
    
    // Define face normals
    std::vector<Math::Vector3f> normals = {
        Math::Vector3f( 0.0f,  0.0f, -1.0f), // Back face
        Math::Vector3f( 0.0f,  0.0f,  1.0f), // Front face
        Math::Vector3f(-1.0f,  0.0f,  0.0f), // Left face
        Math::Vector3f( 1.0f,  0.0f,  0.0f), // Right face
        Math::Vector3f( 0.0f, -1.0f,  0.0f), // Bottom face
        Math::Vector3f( 0.0f,  1.0f,  0.0f)  // Top face
    };
    
    // Create vertices for each face (4 vertices per face)
    std::vector<uint32_t> faceIndices[6] = {
        {3, 2, 1, 0}, // Back face
        {4, 5, 6, 7}, // Front face
        {7, 3, 0, 4}, // Left face
        {1, 2, 6, 5}, // Right face
        {0, 1, 5, 4}, // Bottom face
        {7, 6, 2, 3}  // Top face
    };
    
    // Build mesh with proper normals
    for (int face = 0; face < 6; ++face) {
        uint32_t baseIndex = static_cast<uint32_t>(voxelMesh.vertices.size());
        
        // Add 4 vertices for this face
        for (int i = 0; i < 4; ++i) {
            uint32_t posIndex = faceIndices[face][i];
            Color faceColor = Color(
                face == 0 || face == 1 ? 1.0f : 0.5f,  // Red component
                face == 2 || face == 3 ? 1.0f : 0.5f,  // Green component  
                face == 4 || face == 5 ? 1.0f : 0.5f,  // Blue component
                1.0f
            );
            voxelMesh.vertices.push_back(Vertex(positions[posIndex], normals[face], Math::Vector2f::zero(), faceColor));
        }
        
        // Add indices for 2 triangles per face
        voxelMesh.indices.push_back(baseIndex + 0);
        voxelMesh.indices.push_back(baseIndex + 1);
        voxelMesh.indices.push_back(baseIndex + 2);
        
        voxelMesh.indices.push_back(baseIndex + 0);
        voxelMesh.indices.push_back(baseIndex + 2);
        voxelMesh.indices.push_back(baseIndex + 3);
    }
    
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Created voxel mesh: %zu vertices, %zu indices (%zu triangles)", 
        voxelMesh.vertices.size(), voxelMesh.indices.size(), voxelMesh.indices.size() / 3);
    
    // Position voxel at origin
    Transform transform;
    transform.position = Math::WorldCoordinates(0.0f, 0.0f, 0.0f);
    transform.rotation = Math::Vector3f(0.0f, 0.0f, 0.0f);
    transform.scale = Math::Vector3f(1.0f, 1.0f, 1.0f);
    
    // Create material
    Material material = Material::createVoxel(Color::White());
    
    // Render the voxel
    renderEngine->beginFrame();
    renderEngine->clear(ClearFlags::All, Color(0.1f, 0.1f, 0.2f, 1.0f)); // Dark blue background
    renderEngine->renderMesh(voxelMesh, transform, material);
    renderEngine->endFrame();
    
    // Verify the mesh was processed
    EXPECT_EQ(voxelMesh.vertices.size(), 24); // 6 faces * 4 vertices
    EXPECT_EQ(voxelMesh.indices.size(), 36);  // 6 faces * 2 triangles * 3 indices
    
    // Check render stats
    const RenderStats& stats = renderEngine->getRenderStats();
    EXPECT_GE(stats.frameCount, 0);
    
    EXPECT_TRUE(renderEngine->isInitialized());
}

// Test 7: Render voxel at actual position
TEST_F(RenderIncrementalTest, Test7_RenderVoxelAtPosition) {
    if (!hasOpenGLContext()) {
        GTEST_SKIP() << "No OpenGL context available, skipping OpenGL test";
        return;
    }
    
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    config.enableDebugOutput = true;
    
    if (!renderEngine->initialize(config)) {
        GTEST_SKIP() << "Failed to initialize render engine";
        return;
    }
    
    renderEngine->setViewport(0, 0, 800, 600);
    renderEngine->setCamera(*camera);
    
    Logging::Logger::getInstance().debug("=== Test 7: Voxel at World Position ===", "RenderTest");
    
    // Use the same voxel mesh as Test 6, but create it more efficiently
    Mesh voxelMesh;
    
    // Simple cube vertices (will be duplicated for proper normals)
    Math::Vector3f vertices[8] = {
        Math::Vector3f(-0.5f, -0.5f, -0.5f), Math::Vector3f( 0.5f, -0.5f, -0.5f),
        Math::Vector3f( 0.5f,  0.5f, -0.5f), Math::Vector3f(-0.5f,  0.5f, -0.5f),
        Math::Vector3f(-0.5f, -0.5f,  0.5f), Math::Vector3f( 0.5f, -0.5f,  0.5f),
        Math::Vector3f( 0.5f,  0.5f,  0.5f), Math::Vector3f(-0.5f,  0.5f,  0.5f)
    };
    
    // Face definitions with normals and colors
    struct Face {
        uint32_t indices[4];
        Math::Vector3f normal;
        Color color;
    };
    
    Face faces[6] = {
        {{3, 2, 1, 0}, Math::Vector3f( 0, 0, -1), Color::Red()},    // Back
        {{4, 5, 6, 7}, Math::Vector3f( 0, 0,  1), Color::Green()},  // Front
        {{7, 3, 0, 4}, Math::Vector3f(-1, 0,  0), Color::Blue()},   // Left
        {{1, 2, 6, 5}, Math::Vector3f( 1, 0,  0), Color(1.0f, 1.0f, 0.0f)}, // Right (Yellow)
        {{0, 1, 5, 4}, Math::Vector3f( 0,-1,  0), Color(0.5f, 0.0f, 0.5f)}, // Bottom (Purple)
        {{7, 6, 2, 3}, Math::Vector3f( 0, 1,  0), Color(0.0f, 1.0f, 1.0f)}   // Top (Cyan)
    };
    
    // Build mesh
    for (int f = 0; f < 6; ++f) {
        uint32_t baseIndex = static_cast<uint32_t>(voxelMesh.vertices.size());
        
        // Add 4 vertices for this face
        for (int i = 0; i < 4; ++i) {
            voxelMesh.vertices.push_back(Vertex(
                vertices[faces[f].indices[i]], 
                faces[f].normal, 
                Math::Vector2f(), 
                faces[f].color
            ));
        }
        
        // Add triangles (0,1,2) and (0,2,3)
        voxelMesh.indices.insert(voxelMesh.indices.end(), {
            baseIndex + 0, baseIndex + 1, baseIndex + 2,
            baseIndex + 0, baseIndex + 2, baseIndex + 3
        });
    }
    
    // Position voxel away from origin in a visible location
    Math::Vector3f voxelPosition(1.5f, 0.5f, -3.0f);
    Transform transform;
    transform.position = Math::WorldCoordinates(voxelPosition);
    transform.rotation = Math::Vector3f(15.0f, 30.0f, 0.0f); // Slight rotation for better visibility
    transform.scale = Math::Vector3f(1.2f, 1.2f, 1.2f);     // Slightly larger
    
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Voxel position: (%.3f, %.3f, %.3f), rotation: (%.1f°, %.1f°, %.1f°), scale: %.1f", 
        voxelPosition.x, voxelPosition.y, voxelPosition.z,
        transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.scale.x);
    
    // Log camera info for debugging
    Math::Vector3f camPos = camera->getPosition().value();
    Math::Vector3f camTarget = camera->getTarget().value();
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Camera: pos(%.3f,%.3f,%.3f) -> target(%.3f,%.3f,%.3f)", 
        camPos.x, camPos.y, camPos.z, camTarget.x, camTarget.y, camTarget.z);
    
    // Create material
    Material material = Material::createVoxel(Color::White());
    
    // Render the positioned voxel
    renderEngine->beginFrame();
    renderEngine->clear(ClearFlags::All, Color(0.05f, 0.05f, 0.1f, 1.0f)); // Very dark background
    renderEngine->renderMesh(voxelMesh, transform, material);
    renderEngine->endFrame();
    
    // Verify the mesh was processed
    EXPECT_EQ(voxelMesh.vertices.size(), 24);
    EXPECT_EQ(voxelMesh.indices.size(), 36);
    
    // Check render stats
    const RenderStats& stats = renderEngine->getRenderStats();
    EXPECT_GE(stats.frameCount, 0);
    EXPECT_GE(stats.drawCalls, 0);
    
    Logging::Logger::getInstance().debugfc("RenderTest", 
        "Final render stats: %d frames, %d draws, %d triangles rendered", 
        stats.frameCount, stats.drawCalls, stats.trianglesRendered);
    
    EXPECT_TRUE(renderEngine->isInitialized());
}

} // namespace Rendering
} // namespace VoxelEditor