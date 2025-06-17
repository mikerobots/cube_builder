#include <gtest/gtest.h>
#include <chrono>
#include "../include/visual_feedback/FeedbackRenderer.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Rendering;

class FeedbackRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Note: In a real implementation, renderEngine would be a valid pointer
        renderer = std::make_unique<FeedbackRenderer>(nullptr);
    }
    
    std::unique_ptr<FeedbackRenderer> renderer;
};

TEST_F(FeedbackRendererTest, Construction) {
    EXPECT_TRUE(renderer->isEnabled());
    EXPECT_TRUE(renderer->isFaceHighlightEnabled());
    EXPECT_TRUE(renderer->isVoxelPreviewEnabled());
    EXPECT_TRUE(renderer->isSelectionAnimationEnabled());
    EXPECT_TRUE(renderer->isGroupVisualizationEnabled());
    EXPECT_TRUE(renderer->isWorkspaceVisualizationEnabled());
    EXPECT_FALSE(renderer->areDebugOverlaysEnabled());
    EXPECT_FALSE(renderer->areAnimationsPaused());
}

TEST_F(FeedbackRendererTest, EnableDisable) {
    renderer->setEnabled(false);
    EXPECT_FALSE(renderer->isEnabled());
    
    renderer->setEnabled(true);
    EXPECT_TRUE(renderer->isEnabled());
}

TEST_F(FeedbackRendererTest, FaceHighlight) {
    Face face(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
    Color color = Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    
    EXPECT_NO_THROW(renderer->renderFaceHighlight(face, color));
    EXPECT_NO_THROW(renderer->clearFaceHighlight());
    
    // Test enable/disable
    renderer->setFaceHighlightEnabled(false);
    EXPECT_FALSE(renderer->isFaceHighlightEnabled());
    
    renderer->setFaceHighlightEnabled(true);
    EXPECT_TRUE(renderer->isFaceHighlightEnabled());
}

TEST_F(FeedbackRendererTest, VoxelPreview) {
    Vector3i position(5, 10, 15);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    Color color = Color::Green();
    
    EXPECT_NO_THROW(renderer->renderVoxelPreview(position, resolution, color));
    EXPECT_NO_THROW(renderer->clearVoxelPreview());
    
    // Test enable/disable
    renderer->setVoxelPreviewEnabled(false);
    EXPECT_FALSE(renderer->isVoxelPreviewEnabled());
    
    renderer->setVoxelPreviewEnabled(true);
    EXPECT_TRUE(renderer->isVoxelPreviewEnabled());
}

TEST_F(FeedbackRendererTest, SelectionVisualization) {
    VoxelEditor::Selection::SelectionSet selection;
    Color color = Color(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
    
    EXPECT_NO_THROW(renderer->renderSelection(selection, color));
    
    BoundingBox bounds(Vector3f(0, 0, 0), Vector3f(5, 5, 5));
    EXPECT_NO_THROW(renderer->renderSelectionBounds(bounds, color));
    
    // Test animation enable/disable
    renderer->setSelectionAnimationEnabled(false);
    EXPECT_FALSE(renderer->isSelectionAnimationEnabled());
    
    renderer->setSelectionAnimationEnabled(true);
    EXPECT_TRUE(renderer->isSelectionAnimationEnabled());
}

TEST_F(FeedbackRendererTest, GroupVisualization) {
    std::vector<GroupId> groups = {1, 2, 3};
    
    EXPECT_NO_THROW(renderer->renderGroupOutlines(groups));
    EXPECT_NO_THROW(renderer->renderGroupBounds(1, Color(1.0f, 0.5f, 0.0f, 1.0f))); // Orange
    
    // Test enable/disable
    renderer->setGroupVisualizationEnabled(false);
    EXPECT_FALSE(renderer->isGroupVisualizationEnabled());
    
    renderer->setGroupVisualizationEnabled(true);
    EXPECT_TRUE(renderer->isGroupVisualizationEnabled());
}

TEST_F(FeedbackRendererTest, WorkspaceVisualization) {
    BoundingBox workspace(Vector3f(0, 0, 0), Vector3f(10, 10, 10));
    Color color = Color(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    
    EXPECT_NO_THROW(renderer->renderWorkspaceBounds(workspace, color));
    EXPECT_NO_THROW(renderer->renderGridLines(VoxelResolution::Size_32cm, 0.5f));
    
    // Test enable/disable
    renderer->setWorkspaceVisualizationEnabled(false);
    EXPECT_FALSE(renderer->isWorkspaceVisualizationEnabled());
    
    renderer->setWorkspaceVisualizationEnabled(true);
    EXPECT_TRUE(renderer->isWorkspaceVisualizationEnabled());
}

TEST_F(FeedbackRendererTest, PerformanceOverlays) {
    RenderStats stats;
    stats.frameTime = 16.67f;
    stats.cpuTime = 8.5f;
    stats.gpuTime = 6.2f;
    stats.drawCalls = 150;
    stats.triangleCount = 45000;
    stats.vertexCount = 22500;
    
    EXPECT_NO_THROW(renderer->renderPerformanceMetrics(stats));
    
    size_t used = 256 * 1024 * 1024; // 256 MB
    size_t total = 1024 * 1024 * 1024; // 1 GB
    
    EXPECT_NO_THROW(renderer->renderMemoryUsage(used, total));
    
    // Test enable/disable
    renderer->setDebugOverlaysEnabled(true);
    EXPECT_TRUE(renderer->areDebugOverlaysEnabled());
    
    renderer->setDebugOverlaysEnabled(false);
    EXPECT_FALSE(renderer->areDebugOverlaysEnabled());
}

TEST_F(FeedbackRendererTest, AnimationControl) {
    EXPECT_NO_THROW(renderer->update(0.016f)); // 60 FPS frame time
    
    // Test animation speed
    renderer->setAnimationSpeed(2.0f);
    EXPECT_FLOAT_EQ(renderer->getAnimationSpeed(), 2.0f);
    
    // Test pause/unpause
    renderer->pauseAnimations(true);
    EXPECT_TRUE(renderer->areAnimationsPaused());
    
    renderer->pauseAnimations(false);
    EXPECT_FALSE(renderer->areAnimationsPaused());
    
    EXPECT_NO_THROW(renderer->update(0.016f));
}

TEST_F(FeedbackRendererTest, RenderOrder) {
    renderer->setRenderOrder(500);
    EXPECT_EQ(renderer->getRenderOrder(), 500);
}

TEST_F(FeedbackRendererTest, ComponentAccess) {
    // Test component access
    EXPECT_NO_THROW(renderer->getFaceDetector());
    EXPECT_NO_THROW(renderer->getHighlightRenderer());
    EXPECT_NO_THROW(renderer->getOutlineRenderer());
    EXPECT_NO_THROW(renderer->getOverlayRenderer());
    
    // Test const access
    const FeedbackRenderer& constRenderer = *renderer;
    EXPECT_NO_THROW(constRenderer.getFaceDetector());
}

TEST_F(FeedbackRendererTest, MultipleUpdates) {
    // Test multiple animation updates
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(renderer->update(0.016f));
    }
    
    // Add some content and continue updating
    renderer->renderVoxelPreview(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, Color::Green());
    
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(renderer->update(0.016f));
    }
}

TEST_F(FeedbackRendererTest, DisabledRenderer) {
    // Disable renderer and test that operations don't crash
    renderer->setEnabled(false);
    
    Face face(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
    renderer->renderFaceHighlight(face, Color(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
    
    renderer->renderVoxelPreview(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, Color::Green());
    
    VoxelEditor::Selection::SelectionSet selection;
    renderer->renderSelection(selection, Color(0.0f, 1.0f, 1.0f, 1.0f)); // Cyan
    
    // Should not crash when updating disabled renderer
    EXPECT_NO_THROW(renderer->update(0.016f));
}

TEST_F(FeedbackRendererTest, ComplexScene) {
    // Test with multiple types of feedback active
    
    // Face highlight
    Face face(Vector3i(5, 5, 5), VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
    renderer->renderFaceHighlight(face, Color(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
    
    // Voxel preview
    renderer->renderVoxelPreview(Vector3i(6, 5, 5), VoxelResolution::Size_32cm, Color::Green());
    
    // Selection
    VoxelEditor::Selection::SelectionSet selection;
    renderer->renderSelection(selection, Color(0.0f, 1.0f, 1.0f, 1.0f)); // Cyan
    
    // Group outlines
    std::vector<GroupId> groups = {1, 2, 3};
    renderer->renderGroupOutlines(groups);
    
    // Workspace bounds
    BoundingBox workspace(Vector3f(0, 0, 0), Vector3f(20, 20, 20));
    renderer->renderWorkspaceBounds(workspace, Color(0.5f, 0.5f, 0.5f, 1.0f)); // Gray
    
    // Performance metrics
    RenderStats stats;
    stats.frameTime = 16.67f;
    renderer->renderPerformanceMetrics(stats);
    
    // Enable debug overlays
    renderer->setDebugOverlaysEnabled(true);
    
    // Update with complex scene
    EXPECT_NO_THROW(renderer->update(0.016f));
}

// Enhanced tests for preview rendering requirements
TEST_F(FeedbackRendererTest, VoxelPreviewWithValidation) {
    Vector3i position(5, 0, 5);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    // REQ-4.1.1: Green outline for valid placement
    EXPECT_NO_THROW(renderer->renderVoxelPreviewWithValidation(position, resolution, true));
    
    // REQ-4.1.2: Red outline for invalid placement  
    EXPECT_NO_THROW(renderer->renderVoxelPreviewWithValidation(position, resolution, false));
    
    // Clear and test again
    EXPECT_NO_THROW(renderer->clearVoxelPreview());
    EXPECT_NO_THROW(renderer->renderVoxelPreviewWithValidation(position, resolution, true));
}

TEST_F(FeedbackRendererTest, VoxelPreviewMultipleResolutions) {
    Vector3i position(0, 0, 0);
    
    // REQ-2.2.4: All voxel sizes (1cm to 512cm) placeable
    std::vector<VoxelResolution> resolutions = {
        VoxelResolution::Size_1cm,
        VoxelResolution::Size_2cm,
        VoxelResolution::Size_4cm,
        VoxelResolution::Size_8cm,
        VoxelResolution::Size_16cm,
        VoxelResolution::Size_32cm,
        VoxelResolution::Size_64cm,
        VoxelResolution::Size_128cm,
        VoxelResolution::Size_256cm,
        VoxelResolution::Size_512cm
    };
    
    for (const auto& res : resolutions) {
        EXPECT_NO_THROW(renderer->renderVoxelPreviewWithValidation(position, res, true));
        EXPECT_NO_THROW(renderer->clearVoxelPreview());
    }
}

TEST_F(FeedbackRendererTest, GroundPlaneGridEnhanced) {
    Vector3f center(0.0f, 0.0f, 0.0f);
    float extent = 5.0f;
    Vector3f cursorPos(1.0f, 0.0f, 1.0f);
    
    // REQ-1.1.1, REQ-1.1.3, REQ-1.1.4, REQ-1.2.2: Enhanced grid rendering
    EXPECT_NO_THROW(renderer->renderGroundPlaneGridEnhanced(center, extent, cursorPos, true));
    EXPECT_NO_THROW(renderer->renderGroundPlaneGridEnhanced(center, extent, cursorPos, false));
}

TEST_F(FeedbackRendererTest, PreviewUpdatePerformance) {
    Vector3i position(0, 0, 0);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    // REQ-4.1.3: Preview updates should be smooth and responsive (< 16ms)
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        renderer->renderVoxelPreviewWithValidation(position, resolution, i % 2 == 0);
        renderer->clearVoxelPreview();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 100 updates should complete in reasonable time
    // Each update should average much less than 16ms (16000 microseconds)
    float avgPerUpdate = duration.count() / 100.0f;
    EXPECT_LT(avgPerUpdate, 5000.0f); // Should be much faster than 5ms per update
}

TEST_F(FeedbackRendererTest, HighlightColorValidation) {
    Face face(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
    
    // REQ-4.2.1: Face highlighting uses yellow color
    Color yellowColor(1.0f, 1.0f, 0.0f, 1.0f);
    EXPECT_NO_THROW(renderer->renderFaceHighlight(face, yellowColor));
    
    // Test default color (should be yellow)
    EXPECT_NO_THROW(renderer->renderFaceHighlight(face));
    
    // REQ-4.2.2: Only one face highlighted at a time
    Face face2(Vector3i(2, 3, 4), VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::NegativeY);
    EXPECT_NO_THROW(renderer->renderFaceHighlight(face2)); // Should replace previous highlight
    
    EXPECT_NO_THROW(renderer->clearFaceHighlight());
}

TEST_F(FeedbackRendererTest, WorkspaceScaling) {
    // REQ-6.2.2: Grid size scales with workspace (up to 8m x 8m)
    std::vector<float> extents = {2.0f, 4.0f, 6.0f, 8.0f};
    Vector3f center(0.0f, 0.0f, 0.0f);
    Vector3f cursorPos(0.0f, 0.0f, 0.0f);
    
    for (float extent : extents) {
        EXPECT_NO_THROW(renderer->renderGroundPlaneGridEnhanced(center, extent, cursorPos, false));
    }
}