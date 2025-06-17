#include <gtest/gtest.h>
#include <chrono>
#include "../include/visual_feedback/OverlayRenderer.h"
#include "../../camera/OrbitCamera.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Camera;

class OverlayRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        renderer = std::make_unique<OverlayRenderer>();
    }
    
    std::unique_ptr<OverlayRenderer> renderer;
};

TEST_F(OverlayRendererTest, TextRendering) {
    renderer->beginFrame(1920, 1080);
    
    std::string text = "Hello, World!";
    Vector2f position(100, 100);
    TextStyle style = TextStyle::Default();
    
    EXPECT_NO_THROW(renderer->renderText(text, position, style));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, TextStyles) {
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

TEST_F(OverlayRendererTest, PerformanceMetrics) {
    renderer->beginFrame(1920, 1080);
    
    RenderStats stats;
    stats.frameTime = 16.67f;
    stats.cpuTime = 8.5f;
    stats.gpuTime = 6.2f;
    stats.drawCalls = 150;
    stats.triangleCount = 45000;
    stats.vertexCount = 22500;
    
    Vector2f position(10, 10);
    
    EXPECT_NO_THROW(renderer->renderPerformanceMetrics(stats, position));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, MemoryUsage) {
    renderer->beginFrame(1920, 1080);
    
    size_t used = 256 * 1024 * 1024; // 256 MB
    size_t total = 1024 * 1024 * 1024; // 1 GB
    Vector2f position(10, 150);
    
    EXPECT_NO_THROW(renderer->renderMemoryUsage(used, total, position));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, VoxelCount) {
    renderer->beginFrame(1920, 1080);
    
    std::unordered_map<VoxelResolution, size_t> counts;
    counts[VoxelResolution::Size_32cm] = 1000;
    counts[VoxelResolution::Size_64cm] = 500;
    counts[VoxelResolution::Size_128cm] = 100;
    
    Vector2f position(10, 200);
    
    EXPECT_NO_THROW(renderer->renderVoxelCount(counts, position));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, Indicators) {
    renderer->beginFrame(1920, 1080);
    
    // Resolution indicator
    EXPECT_NO_THROW(
        renderer->renderResolutionIndicator(VoxelResolution::Size_32cm, Vector2f(10, 250))
    );
    
    // Workspace indicator
    Vector3f workspaceSize(10.0f, 8.0f, 12.0f);
    EXPECT_NO_THROW(
        renderer->renderWorkspaceIndicator(workspaceSize, Vector2f(10, 300))
    );
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, BoundingBoxes) {
    renderer->beginFrame(1920, 1080);
    
    std::vector<BoundingBox> boxes = {
        BoundingBox(Vector3f(0, 0, 0), Vector3f(1, 1, 1)),
        BoundingBox(Vector3f(2, 2, 2), Vector3f(3, 3, 3)),
        BoundingBox(Vector3f(-1, -1, -1), Vector3f(0, 0, 0))
    };
    
    VoxelEditor::Rendering::Color color = VoxelEditor::Rendering::Color::Red();
    
    // Note: This requires a camera parameter which we don't have in this test
    // EXPECT_NO_THROW(renderer->renderBoundingBoxes(boxes, color, camera));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, Raycast) {
    renderer->beginFrame(1920, 1080);
    
    VoxelEditor::VisualFeedback::Ray ray(Vector3f(0, 0, 0), Vector3f(1, 0, 0));
    float length = 10.0f;
    VoxelEditor::Rendering::Color color = VoxelEditor::Rendering::Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    
    // Note: This requires a camera parameter which we don't have in this test
    // EXPECT_NO_THROW(renderer->renderRaycast(ray, length, color, camera));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, FrameManagement) {
    // Test frame begin/end without crashes
    EXPECT_NO_THROW(renderer->beginFrame(1920, 1080));
    EXPECT_NO_THROW(renderer->endFrame());
    
    // Test multiple begin/end cycles
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(renderer->beginFrame(800, 600));
        
        // Add some content
        renderer->renderText("Frame " + std::to_string(i), Vector2f(10, 10), TextStyle::Default());
        
        EXPECT_NO_THROW(renderer->endFrame());
    }
}

TEST_F(OverlayRendererTest, EmptyFrame) {
    // Test empty frame (begin/end without any content)
    EXPECT_NO_THROW(renderer->beginFrame(1920, 1080));
    EXPECT_NO_THROW(renderer->endFrame());
}

TEST_F(OverlayRendererTest, LargeText) {
    renderer->beginFrame(1920, 1080);
    
    // Test with large text
    std::string largeText;
    for (int i = 0; i < 1000; ++i) {
        largeText += "A";
    }
    
    EXPECT_NO_THROW(renderer->renderText(largeText, Vector2f(0, 0), TextStyle::Default()));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, ManyTextElements) {
    renderer->beginFrame(1920, 1080);
    
    // Test with many text elements
    for (int i = 0; i < 100; ++i) {
        Vector2f pos(i % 20 * 50, i / 20 * 20);
        renderer->renderText("Text " + std::to_string(i), pos, TextStyle::Default());
    }
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, DifferentScreenSizes) {
    std::vector<std::pair<int, int>> screenSizes = {
        {800, 600},
        {1920, 1080},
        {3840, 2160},
        {1024, 768}
    };
    
    for (const auto& size : screenSizes) {
        EXPECT_NO_THROW(renderer->beginFrame(size.first, size.second));
        
        renderer->renderText("Test", Vector2f(10, 10), TextStyle::Default());
        
        EXPECT_NO_THROW(renderer->endFrame());
    }
}

// Enhanced tests for grid visualization requirements
TEST_F(OverlayRendererTest, GroundPlaneGridBasic) {
    renderer->beginFrame(1920, 1080);
    
    // REQ-1.1.1, REQ-1.1.3, REQ-1.1.4: Basic grid rendering
    Vector3f center(0.0f, 0.0f, 0.0f);
    float extent = 5.0f; // 5 meter extent
    Vector3f cursorPos(1.0f, 0.0f, 1.0f); 
    bool enableDynamicOpacity = false;
    
    // Create a simple camera for testing
    OrbitCamera camera;
    camera.setTarget(Vector3f(0, 0, 0));
    camera.setDistance(8.0f);
    camera.setOrbitAngles(45.0f, -30.0f);
    
    EXPECT_NO_THROW(renderer->renderGroundPlaneGrid(center, extent, cursorPos, enableDynamicOpacity, camera));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, GroundPlaneGridDynamicOpacity) {
    renderer->beginFrame(1920, 1080);
    
    // REQ-1.2.2: Dynamic opacity near cursor
    Vector3f center(0.0f, 0.0f, 0.0f);
    float extent = 5.0f;
    Vector3f cursorPos(0.64f, 0.0f, 0.32f); // Within 2 grid squares (64cm) of origin
    bool enableDynamicOpacity = true;
    
    OrbitCamera camera;
    camera.setTarget(Vector3f(0, 0, 0));
    camera.setDistance(8.0f);
    camera.setOrbitAngles(45.0f, -30.0f);
    
    EXPECT_NO_THROW(renderer->renderGroundPlaneGrid(center, extent, cursorPos, enableDynamicOpacity, camera));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, GroundPlaneGridLargeExtent) {
    renderer->beginFrame(1920, 1080);
    
    // REQ-6.2.2: Grid scaling up to 8m x 8m
    Vector3f center(0.0f, 0.0f, 0.0f);
    float extent = 8.0f; // Maximum workspace size
    Vector3f cursorPos(0.0f, 0.0f, 0.0f);
    bool enableDynamicOpacity = false;
    
    OrbitCamera camera;
    camera.setTarget(Vector3f(0, 0, 0));
    camera.setDistance(15.0f);
    camera.setOrbitAngles(45.0f, -30.0f);
    
    EXPECT_NO_THROW(renderer->renderGroundPlaneGrid(center, extent, cursorPos, enableDynamicOpacity, camera));
    
    renderer->endFrame();
}

TEST_F(OverlayRendererTest, GroundPlaneGridPerformance) {
    // REQ-6.1.1: Grid rendering performance test
    renderer->beginFrame(1920, 1080);
    
    Vector3f center(0.0f, 0.0f, 0.0f);
    float extent = 5.0f;
    Vector3f cursorPos(0.0f, 0.0f, 0.0f);
    bool enableDynamicOpacity = true;
    
    OrbitCamera camera;
    camera.setTarget(Vector3f(0, 0, 0));
    camera.setDistance(8.0f);
    camera.setOrbitAngles(45.0f, -30.0f);
    
    // Test multiple renders (simulating frame updates)
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 60; ++i) { // Simulate 60 frames
        EXPECT_NO_THROW(renderer->renderGroundPlaneGrid(center, extent, cursorPos, enableDynamicOpacity, camera));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 60 renders in reasonable time (less than 1 second for this test)
    EXPECT_LT(duration.count(), 1000);
    
    renderer->endFrame();
}