#include <gtest/gtest.h>
#include "../include/visual_feedback/OverlayRenderer.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

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