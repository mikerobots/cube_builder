#include <gtest/gtest.h>

// Include OpenGL headers
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>
#include <memory>
#include <cstdlib>
#include "visual_feedback/OutlineRenderer.h"
#include "visual_feedback/FeedbackTypes.h"
#include "camera/QuaternionOrbitCamera.h"
#include "rendering/OpenGLRenderer.h"
#include "math/Vector3i.h"
#include "math/BoundingBox.h"
#include "voxel_data/VoxelTypes.h"
#include "logging/Logger.h"

namespace VoxelEditor {

class OutlineRendererIntegrationTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    std::unique_ptr<VisualFeedback::OutlineRenderer> renderer;
    std::unique_ptr<Camera::QuaternionOrbitCamera> camera;
    
    void SetUp() override {
        // Skip in CI environment
        if (std::getenv("CI") != nullptr) {
            GTEST_SKIP() << "Skipping OpenGL tests in CI environment";
        }
        
        // Setup logging
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::FileOutput>("outline_renderer_integration_test.log", "TestLog", false));
        
        // Initialize GLFW
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        
        // Configure OpenGL context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        // Create window
        window = glfwCreateWindow(800, 600, "OutlineRenderer Integration Test", nullptr, nullptr);
        ASSERT_NE(window, nullptr) << "Failed to create GLFW window";
        
        // Make context current
        glfwMakeContextCurrent(window);
        
        // Load OpenGL functions
#ifndef __APPLE__
        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) << "Failed to initialize GLAD";
#endif
        
        // Clear any existing GL errors
        while (glGetError() != GL_NO_ERROR) {}
        
        // Create renderer and camera
        renderer = std::make_unique<VisualFeedback::OutlineRenderer>();
        camera = std::make_unique<Camera::QuaternionOrbitCamera>();
        camera->setViewPreset(Camera::ViewPreset::ISOMETRIC);
    }
    
    void TearDown() override {
        if (window) {
            renderer.reset();
            camera.reset();
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }
    
    void checkGLError(const std::string& operation) {
        GLenum error = glGetError();
        EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error " << error << " during " << operation;
    }
};

TEST_F(OutlineRendererIntegrationTest, RenderSingleVoxelOutline) {
    // Test rendering a single voxel outline
    renderer->beginBatch();
    
    Math::Vector3i position(0, 0, 0);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    VisualFeedback::OutlineStyle style = VisualFeedback::OutlineStyle::VoxelPreview();
    
    renderer->renderVoxelOutline(position, resolution, style);
    renderer->endBatch();
    
    // Render the batch
    EXPECT_NO_THROW(renderer->renderBatch(*camera));
    checkGLError("renderBatch single voxel");
    
    renderer->clearBatch();
}

TEST_F(OutlineRendererIntegrationTest, RenderMultipleVoxelOutlines) {
    // Test rendering multiple voxel outlines in a batch
    renderer->beginBatch();
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    VisualFeedback::OutlineStyle style = VisualFeedback::OutlineStyle::VoxelPreview();
    
    // Add multiple voxels
    for (int x = 0; x < 3; ++x) {
        for (int z = 0; z < 3; ++z) {
            Math::Vector3i position(x, 0, z);
            renderer->renderVoxelOutline(position, resolution, style);
        }
    }
    
    renderer->endBatch();
    
    // Render the batch
    EXPECT_NO_THROW(renderer->renderBatch(*camera));
    checkGLError("renderBatch multiple voxels");
    
    renderer->clearBatch();
}

TEST_F(OutlineRendererIntegrationTest, RenderBoxOutline) {
    // Test rendering a box outline
    renderer->beginBatch();
    
    Math::BoundingBox box(Math::Vector3f(0, 0, 0), Math::Vector3f(1, 1, 1));
    VisualFeedback::OutlineStyle style = VisualFeedback::OutlineStyle::SelectionBox();
    
    renderer->renderBoxOutline(box, style);
    renderer->endBatch();
    
    // Render the batch
    EXPECT_NO_THROW(renderer->renderBatch(*camera));
    checkGLError("renderBatch box outline");
    
    renderer->clearBatch();
}

TEST_F(OutlineRendererIntegrationTest, RenderCustomOutline) {
    // Test rendering a custom outline
    renderer->beginBatch();
    
    std::vector<Math::Vector3f> points = {
        Math::Vector3f(0, 0, 0),
        Math::Vector3f(1, 0, 0),
        Math::Vector3f(1, 1, 0),
        Math::Vector3f(0, 1, 0)
    };
    
    VisualFeedback::OutlineStyle style = VisualFeedback::OutlineStyle::GroupBoundary();
    renderer->renderCustomOutline(points, style, true); // closed outline
    
    renderer->endBatch();
    
    // Render the batch
    EXPECT_NO_THROW(renderer->renderBatch(*camera));
    checkGLError("renderBatch custom outline");
    
    renderer->clearBatch();
}

TEST_F(OutlineRendererIntegrationTest, RenderEmptyBatch) {
    // Test rendering an empty batch (should not crash or produce errors)
    renderer->beginBatch();
    renderer->endBatch();
    
    EXPECT_NO_THROW(renderer->renderBatch(*camera));
    checkGLError("renderBatch empty");
    
    renderer->clearBatch();
}

TEST_F(OutlineRendererIntegrationTest, RenderWithDifferentLinePatterns) {
    // Test different line patterns
    std::vector<VisualFeedback::LinePattern> patterns = {
        VisualFeedback::LinePattern::Solid,
        VisualFeedback::LinePattern::Dashed,
        VisualFeedback::LinePattern::Dotted,
        VisualFeedback::LinePattern::DashDot
    };
    
    for (auto pattern : patterns) {
        renderer->beginBatch();
        
        Math::Vector3i position(0, 0, 0);
        VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
        VisualFeedback::OutlineStyle style = VisualFeedback::OutlineStyle::VoxelPreview();
        style.pattern = pattern;
        
        renderer->renderVoxelOutline(position, resolution, style);
        renderer->endBatch();
        
        EXPECT_NO_THROW(renderer->renderBatch(*camera));
        checkGLError("renderBatch pattern " + std::to_string(static_cast<int>(pattern)));
        
        renderer->clearBatch();
    }
}

TEST_F(OutlineRendererIntegrationTest, RenderWithAnimation) {
    // Test animated outlines
    renderer->beginBatch();
    
    Math::BoundingBox box(Math::Vector3f(0, 0, 0), Math::Vector3f(1, 1, 1));
    VisualFeedback::OutlineStyle style = VisualFeedback::OutlineStyle::SelectionBox();
    style.animated = true;
    style.animationSpeed = 2.0f;
    
    renderer->renderBoxOutline(box, style);
    renderer->endBatch();
    
    // Update animation
    renderer->update(0.016f);
    
    // Render the batch
    EXPECT_NO_THROW(renderer->renderBatch(*camera));
    checkGLError("renderBatch animated");
    
    renderer->clearBatch();
}

TEST_F(OutlineRendererIntegrationTest, StressTestManyOutlines) {
    // Stress test with many outlines
    renderer->beginBatch();
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    VisualFeedback::OutlineStyle style = VisualFeedback::OutlineStyle::VoxelPreview();
    
    // Add 100 voxel outlines
    for (int i = 0; i < 100; ++i) {
        Math::Vector3i position(i % 10, 0, i / 10);
        renderer->renderVoxelOutline(position, resolution, style);
    }
    
    renderer->endBatch();
    
    // Render the batch
    EXPECT_NO_THROW(renderer->renderBatch(*camera));
    checkGLError("renderBatch stress test");
    
    renderer->clearBatch();
}

TEST_F(OutlineRendererIntegrationTest, MultipleRenderCalls) {
    // Test multiple render calls without clearing
    renderer->beginBatch();
    renderer->renderVoxelOutline(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_32cm, 
                                 VisualFeedback::OutlineStyle::VoxelPreview());
    renderer->endBatch();
    
    // Render multiple times
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(renderer->renderBatch(*camera));
        checkGLError("renderBatch iteration " + std::to_string(i));
    }
    
    renderer->clearBatch();
}

} // namespace VoxelEditor