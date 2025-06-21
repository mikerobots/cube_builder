#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include "../../../core/visual_feedback/include/visual_feedback/OverlayRenderer.h"
#include "../../../core/camera/OrbitCamera.h"
#include "../../../foundation/math/CoordinateTypes.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Camera;

class OverlayRendererIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Always skip these tests unless explicitly enabled
        // These tests require a proper OpenGL context which is not available in CI/headless environments
        if (std::getenv("ENABLE_OPENGL_TESTS") == nullptr) {
            GTEST_SKIP() << "Skipping OpenGL tests - set ENABLE_OPENGL_TESTS=1 to run";
        }
        renderer = std::make_unique<OverlayRenderer>();
    }
    
    std::unique_ptr<OverlayRenderer> renderer;
};

// These tests require OpenGL context and should only run in environments with display support
TEST_F(OverlayRendererIntegrationTest, TextRendering) {
    renderer->beginFrame(1920, 1080);
    
    std::string text = "Hello, World!";
    Vector2f position(100, 100);
    TextStyle style = TextStyle::Default();
    
    EXPECT_NO_THROW(renderer->renderText(text, position, style));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererIntegrationTest, TextStyles) {
    renderer->beginFrame(1920, 1080);
    
    std::string text = "Test Text";
    Vector2f position(50, 50);
    
    // Test all text style factories
    EXPECT_NO_THROW(renderer->renderText(text, position, TextStyle::Default()));
    EXPECT_NO_THROW(renderer->renderText(text, position, TextStyle::Header()));
    EXPECT_NO_THROW(renderer->renderText(text, position, TextStyle::Debug()));
    EXPECT_NO_THROW(renderer->renderText(text, position, TextStyle::Warning()));
    EXPECT_NO_THROW(renderer->renderText(text, position, TextStyle::Error()));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererIntegrationTest, PerformanceMetrics) {
    renderer->beginFrame(1920, 1080);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        std::string text = "Performance Test " + std::to_string(i);
        Vector2f position(i * 2.0f, 50.0f);
        TextStyle style = TextStyle::Default();
        
        renderer->renderText(text, position, style);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Performance requirement: 100 text renders should complete in under 50ms
    EXPECT_LT(duration.count(), 50);
    
    renderer->endFrame();
}

TEST_F(OverlayRendererIntegrationTest, MemoryUsage) {
    // Test memory usage doesn't grow excessively with repeated rendering
    renderer->beginFrame(1920, 1080);
    
    for (int frame = 0; frame < 10; ++frame) {
        for (int i = 0; i < 50; ++i) {
            std::string text = "Frame " + std::to_string(frame) + " Text " + std::to_string(i);
            Vector2f position(i * 10.0f, frame * 20.0f);
            renderer->renderText(text, position, TextStyle::Default());
        }
        
        renderer->endFrame();
        if (frame < 9) renderer->beginFrame(1920, 1080);
    }
    
    // Test should complete without crashes or excessive memory usage
    SUCCEED();
}

TEST_F(OverlayRendererIntegrationTest, GroundPlaneGridBasic) {
    renderer->beginFrame(1920, 1080);
    
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    float extent = 5.0f;
    Vector3f cursorPos(0.0f, 0.0f, 0.0f);
    bool enableDynamicOpacity = false;
    
    // Need a camera for the method
    OrbitCamera camera(nullptr);
    camera.setPosition(WorldCoordinates(Vector3f(5.0f, 5.0f, 5.0f)));
    camera.setTarget(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)));
    
    EXPECT_NO_THROW(renderer->renderGroundPlaneGrid(workspaceCenter, extent, cursorPos, enableDynamicOpacity, camera));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererIntegrationTest, GroundPlaneGridDynamicOpacity) {
    renderer->beginFrame(1920, 1080);
    
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    float extent = 5.0f;
    Vector3f cursorPos(0.0f, 0.0f, 0.0f);
    
    // Need a camera for the method
    OrbitCamera camera(nullptr);
    camera.setPosition(WorldCoordinates(Vector3f(5.0f, 5.0f, 5.0f)));
    camera.setTarget(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)));
    
    // Test different opacity levels through enableDynamicOpacity flag
    EXPECT_NO_THROW(renderer->renderGroundPlaneGrid(workspaceCenter, extent, cursorPos, false, camera));
    EXPECT_NO_THROW(renderer->renderGroundPlaneGrid(workspaceCenter, extent, cursorPos, true, camera));
    
    renderer->endFrame();
}

// Additional integration tests for frame management, large text rendering, etc.
TEST_F(OverlayRendererIntegrationTest, FrameManagement) {
    // Test proper frame lifecycle
    EXPECT_NO_THROW(renderer->beginFrame(1920, 1080));
    EXPECT_NO_THROW(renderer->endFrame());
    
    // Test multiple frame cycles
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(renderer->beginFrame(800, 600));
        
        std::string text = "Frame " + std::to_string(i);
        Vector2f position(100, 100);
        renderer->renderText(text, position, TextStyle::Default());
        
        EXPECT_NO_THROW(renderer->endFrame());
    }
}

TEST_F(OverlayRendererIntegrationTest, DifferentScreenSizes) {
    // Test various screen resolutions
    std::vector<std::pair<int, int>> resolutions = {
        {800, 600}, {1920, 1080}, {2560, 1440}, {3840, 2160}
    };
    
    for (const auto& res : resolutions) {
        EXPECT_NO_THROW(renderer->beginFrame(res.first, res.second));
        
        std::string text = std::to_string(res.first) + "x" + std::to_string(res.second);
        Vector2f position(100, 100);
        renderer->renderText(text, position, TextStyle::Default());
        
        EXPECT_NO_THROW(renderer->endFrame());
    }
}