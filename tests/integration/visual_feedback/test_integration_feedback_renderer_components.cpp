#include <gtest/gtest.h>
#include <chrono>
#include "../../../core/visual_feedback/include/visual_feedback/FeedbackRenderer.h"
#include "../../../core/selection/SelectionSet.h"
#include "../../../foundation/math/BoundingBox.h"
#include "../../../core/rendering/RenderTypes.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Rendering;

class FeedbackRendererIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Note: In a real implementation, renderEngine would be a valid pointer
        renderer = std::make_unique<FeedbackRenderer>(nullptr);
    }
    
    std::unique_ptr<FeedbackRenderer> renderer;
};

// These tests require OpenGL context and should only run in environments with display support
TEST_F(FeedbackRendererIntegrationTest, FaceHighlight) {
    Face face(Vector3i(1, 2, 3), VoxelEditor::VoxelData::VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
    
    EXPECT_NO_THROW(renderer->renderFaceHighlight(face));
    EXPECT_NO_THROW(renderer->clearFaceHighlight());
}

TEST_F(FeedbackRendererIntegrationTest, VoxelPreview) {
    Vector3i position(5, 10, 15);
    VoxelEditor::VoxelData::VoxelResolution resolution = VoxelEditor::VoxelData::VoxelResolution::Size_32cm;
    bool isValid = true;
    
    EXPECT_NO_THROW(renderer->renderVoxelPreviewWithValidation(position, resolution, isValid));
    EXPECT_NO_THROW(renderer->clearVoxelPreview());
}

TEST_F(FeedbackRendererIntegrationTest, SelectionVisualization) {
    VoxelEditor::Selection::SelectionSet selection;
    selection.add(VoxelEditor::Selection::VoxelId{Vector3i(0, 0, 0), VoxelEditor::VoxelData::VoxelResolution::Size_32cm});
    selection.add(VoxelEditor::Selection::VoxelId{Vector3i(1, 0, 0), VoxelEditor::VoxelData::VoxelResolution::Size_32cm});
    selection.add(VoxelEditor::Selection::VoxelId{Vector3i(0, 1, 0), VoxelEditor::VoxelData::VoxelResolution::Size_32cm});
    
    EXPECT_NO_THROW(renderer->renderSelection(selection));
    EXPECT_NO_THROW(renderer->setSelectionAnimationEnabled(false));
}

TEST_F(FeedbackRendererIntegrationTest, GroupVisualization) {
    std::vector<GroupId> groups = {1, 2, 3};
    
    EXPECT_NO_THROW(renderer->renderGroupOutlines(groups));
    EXPECT_NO_THROW(renderer->renderGroupBounds(1, Color::Red()));
}

TEST_F(FeedbackRendererIntegrationTest, WorkspaceVisualization) {
    BoundingBox workspace(Vector3f(-2.5f, 0.0f, -2.5f), Vector3f(2.5f, 5.0f, 2.5f));
    
    EXPECT_NO_THROW(renderer->renderWorkspaceBounds(workspace));
    EXPECT_NO_THROW(renderer->renderGridLines(VoxelEditor::VoxelData::VoxelResolution::Size_32cm, 0.35f));
}

TEST_F(FeedbackRendererIntegrationTest, PerformanceOverlays) {
    RenderStats stats;
    stats.drawCalls = 100;
    stats.triangleCount = 5000;
    stats.frameTime = 16.7f;
    
    EXPECT_NO_THROW(renderer->renderPerformanceMetrics(stats));
    EXPECT_NO_THROW(renderer->renderMemoryUsage(50 * 1024 * 1024, 100 * 1024 * 1024)); // 50MB used, 100MB total
}

TEST_F(FeedbackRendererIntegrationTest, AnimationControl) {
    // Test animation pause/resume
    EXPECT_NO_THROW(renderer->pauseAnimations(true));
    EXPECT_TRUE(renderer->areAnimationsPaused());
    
    EXPECT_NO_THROW(renderer->pauseAnimations(false));
    EXPECT_FALSE(renderer->areAnimationsPaused());
    
    // Test animation speed
    EXPECT_NO_THROW(renderer->setAnimationSpeed(2.0f));
    EXPECT_FLOAT_EQ(renderer->getAnimationSpeed(), 2.0f);
}

TEST_F(FeedbackRendererIntegrationTest, RenderOrder) {
    // Test that multiple visual elements can be rendered without conflicts
    Face face(Vector3i(1, 0, 0), VoxelEditor::VoxelData::VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
    Vector3i previewPos(2, 0, 0);
    
    // Set multiple visual elements
    EXPECT_NO_THROW(renderer->renderFaceHighlight(face));
    EXPECT_NO_THROW(renderer->renderVoxelPreviewWithValidation(previewPos, VoxelEditor::VoxelData::VoxelResolution::Size_32cm, true));
    
    // Clear in different order
    EXPECT_NO_THROW(renderer->clearVoxelPreview());
    EXPECT_NO_THROW(renderer->clearFaceHighlight());
}

TEST_F(FeedbackRendererIntegrationTest, MultipleUpdates) {
    // Test rapid updates don't cause issues
    for (int i = 0; i < 100; ++i) {
        Vector3i position(i % 10, 0, 0);
        Face face(position, VoxelEditor::VoxelData::VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
        
        EXPECT_NO_THROW(renderer->renderFaceHighlight(face));
        EXPECT_NO_THROW(renderer->renderVoxelPreviewWithValidation(position, VoxelEditor::VoxelData::VoxelResolution::Size_32cm, true));
    }
    
    // Clear individual elements (no clearAll method)
    EXPECT_NO_THROW(renderer->clearFaceHighlight());
    EXPECT_NO_THROW(renderer->clearVoxelPreview());
}

TEST_F(FeedbackRendererIntegrationTest, ComplexScene) {
    // Test complex scene with multiple visual elements
    
    // Face highlight
    Face face(Vector3i(5, 5, 5), VoxelEditor::VoxelData::VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveY);
    EXPECT_NO_THROW(renderer->renderFaceHighlight(face));
    
    // Voxel preview
    Vector3i previewPos(6, 5, 5);
    EXPECT_NO_THROW(renderer->renderVoxelPreviewWithValidation(previewPos, VoxelEditor::VoxelData::VoxelResolution::Size_32cm, true));
    
    // Selection
    std::vector<VoxelEditor::Selection::VoxelId> selection;
    for (int i = 0; i < 10; ++i) {
        selection.push_back(VoxelEditor::Selection::VoxelId{Vector3i(i, 0, 0), VoxelEditor::VoxelData::VoxelResolution::Size_32cm});
    }
    VoxelEditor::Selection::SelectionSet selectionSet;
    for (const auto& voxel : selection) {
        selectionSet.add(voxel);
    }
    EXPECT_NO_THROW(renderer->renderSelection(selectionSet));
    
    // Workspace bounds
    BoundingBox workspace(Vector3f(-5.0f, 0.0f, -5.0f), Vector3f(5.0f, 10.0f, 5.0f));
    EXPECT_NO_THROW(renderer->renderWorkspaceBounds(workspace));
    
    // Performance overlay
    RenderStats stats;
    stats.drawCalls = 200;
    stats.triangleCount = 10000;
    stats.frameTime = 16.7f;
    EXPECT_NO_THROW(renderer->renderPerformanceMetrics(stats));
    
    // Clear everything
    // Clear individual elements (no clearAll method)
    EXPECT_NO_THROW(renderer->clearFaceHighlight());
    EXPECT_NO_THROW(renderer->clearVoxelPreview());
}

TEST_F(FeedbackRendererIntegrationTest, VoxelPreviewMultipleResolutions) {
    // Test preview with different voxel resolutions
    std::vector<VoxelEditor::VoxelData::VoxelResolution> resolutions = {
        VoxelEditor::VoxelData::VoxelResolution::Size_1cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_8cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_32cm,
        VoxelEditor::VoxelData::VoxelResolution::Size_128cm
    };
    
    Vector3i basePos(0, 0, 0);
    
    for (size_t i = 0; i < resolutions.size(); ++i) {
        Vector3i position(static_cast<int>(i), 0, 0);
        EXPECT_NO_THROW(renderer->renderVoxelPreviewWithValidation(position, resolutions[i], true));
        EXPECT_NO_THROW(renderer->clearVoxelPreview());
    }
}

TEST_F(FeedbackRendererIntegrationTest, PreviewUpdatePerformance) {
    // Test that rapid preview updates maintain good performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        Vector3i position(i % 100, (i / 100) % 10, 0);
        bool isValid = (i % 2 == 0);
        
        renderer->renderVoxelPreviewWithValidation(position, VoxelEditor::VoxelData::VoxelResolution::Size_32cm, isValid);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Performance requirement: 1000 preview updates should complete in under 100ms
    EXPECT_LT(duration.count(), 100);
    
    renderer->clearVoxelPreview();
}