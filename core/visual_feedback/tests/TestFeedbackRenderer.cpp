#include <gtest/gtest.h>
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
    Face face(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    Color color = Color::Yellow();
    
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
    Selection::SelectionSet selection;
    Color color = Color::Cyan();
    
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
    EXPECT_NO_THROW(renderer->renderGroupBounds(1, Color::Orange()));
    
    // Test enable/disable
    renderer->setGroupVisualizationEnabled(false);
    EXPECT_FALSE(renderer->isGroupVisualizationEnabled());
    
    renderer->setGroupVisualizationEnabled(true);
    EXPECT_TRUE(renderer->isGroupVisualizationEnabled());
}

TEST_F(FeedbackRendererTest, WorkspaceVisualization) {
    BoundingBox workspace(Vector3f(0, 0, 0), Vector3f(10, 10, 10));
    Color color = Color::Gray();
    
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
    
    Face face(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    renderer->renderFaceHighlight(face, Color::Yellow());
    
    renderer->renderVoxelPreview(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, Color::Green());
    
    Selection::SelectionSet selection;
    renderer->renderSelection(selection, Color::Cyan());
    
    // Should not crash when updating disabled renderer
    EXPECT_NO_THROW(renderer->update(0.016f));
}

TEST_F(FeedbackRendererTest, ComplexScene) {
    // Test with multiple types of feedback active
    
    // Face highlight
    Face face(Vector3i(5, 5, 5), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    renderer->renderFaceHighlight(face, Color::Yellow());
    
    // Voxel preview
    renderer->renderVoxelPreview(Vector3i(6, 5, 5), VoxelResolution::Size_32cm, Color::Green());
    
    // Selection
    Selection::SelectionSet selection;
    renderer->renderSelection(selection, Color::Cyan());
    
    // Group outlines
    std::vector<GroupId> groups = {1, 2, 3};
    renderer->renderGroupOutlines(groups);
    
    // Workspace bounds
    BoundingBox workspace(Vector3f(0, 0, 0), Vector3f(20, 20, 20));
    renderer->renderWorkspaceBounds(workspace, Color::Gray());
    
    // Performance metrics
    RenderStats stats;
    stats.frameTime = 16.67f;
    renderer->renderPerformanceMetrics(stats);
    
    // Enable debug overlays
    renderer->setDebugOverlaysEnabled(true);
    
    // Update with complex scene
    EXPECT_NO_THROW(renderer->update(0.016f));
}