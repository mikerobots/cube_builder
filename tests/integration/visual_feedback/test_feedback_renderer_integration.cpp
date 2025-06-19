#include <gtest/gtest.h>
#include <chrono>
#include "../../../core/visual_feedback/include/visual_feedback/FeedbackRenderer.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
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
    Face face(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    
    EXPECT_NO_THROW(renderer->setFaceHighlight(face));
    EXPECT_NO_THROW(renderer->clearFaceHighlight());
}

TEST_F(FeedbackRendererIntegrationTest, VoxelPreview) {
    Vector3i position(5, 10, 15);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    bool isValid = true;
    
    EXPECT_NO_THROW(renderer->setVoxelPreview(position, resolution, isValid));
    EXPECT_NO_THROW(renderer->clearVoxelPreview());
}

TEST_F(FeedbackRendererIntegrationTest, SelectionVisualization) {
    std::vector<VoxelId> selection = {
        VoxelId{Vector3i(0, 0, 0), VoxelResolution::Size_32cm},
        VoxelId{Vector3i(1, 0, 0), VoxelResolution::Size_32cm},
        VoxelId{Vector3i(0, 1, 0), VoxelResolution::Size_32cm}
    };
    
    EXPECT_NO_THROW(renderer->setSelectionVisualization(selection));
    EXPECT_NO_THROW(renderer->clearSelectionVisualization());
}

TEST_F(FeedbackRendererIntegrationTest, GroupVisualization) {
    std::vector<VoxelId> group = {
        VoxelId{Vector3i(2, 2, 2), VoxelResolution::Size_32cm},
        VoxelId{Vector3i(3, 2, 2), VoxelResolution::Size_32cm}
    };
    
    EXPECT_NO_THROW(renderer->setGroupVisualization(group));
    EXPECT_NO_THROW(renderer->clearGroupVisualization());
}

TEST_F(FeedbackRendererIntegrationTest, WorkspaceVisualization) {
    Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    
    EXPECT_NO_THROW(renderer->setWorkspaceVisualization(workspaceSize, workspaceCenter));
    EXPECT_NO_THROW(renderer->clearWorkspaceVisualization());
}

TEST_F(FeedbackRendererIntegrationTest, PerformanceOverlays) {
    PerformanceMetrics metrics;
    metrics.frameTime = 16.7f; // 60 FPS
    metrics.voxelCount = 1000;
    metrics.memoryUsage = 50.0f; // MB
    
    EXPECT_NO_THROW(renderer->setPerformanceOverlay(metrics));
    EXPECT_NO_THROW(renderer->clearPerformanceOverlay());
}

TEST_F(FeedbackRendererIntegrationTest, AnimationControl) {
    // Test animation enable/disable
    EXPECT_NO_THROW(renderer->setAnimationEnabled(true));
    EXPECT_TRUE(renderer->isAnimationEnabled());
    
    EXPECT_NO_THROW(renderer->setAnimationEnabled(false));
    EXPECT_FALSE(renderer->isAnimationEnabled());
}

TEST_F(FeedbackRendererIntegrationTest, RenderOrder) {
    // Test that multiple visual elements can be rendered without conflicts
    Face face(Vector3i(1, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    Vector3i previewPos(2, 0, 0);
    
    // Set multiple visual elements
    EXPECT_NO_THROW(renderer->setFaceHighlight(face));
    EXPECT_NO_THROW(renderer->setVoxelPreview(previewPos, VoxelResolution::Size_32cm, true));
    
    // Clear in different order
    EXPECT_NO_THROW(renderer->clearVoxelPreview());
    EXPECT_NO_THROW(renderer->clearFaceHighlight());
}

TEST_F(FeedbackRendererIntegrationTest, MultipleUpdates) {
    // Test rapid updates don't cause issues
    for (int i = 0; i < 100; ++i) {
        Vector3i position(i % 10, 0, 0);
        Face face(position, VoxelResolution::Size_32cm, FaceDirection::PositiveX);
        
        EXPECT_NO_THROW(renderer->setFaceHighlight(face));
        EXPECT_NO_THROW(renderer->setVoxelPreview(position, VoxelResolution::Size_32cm, true));
    }
    
    EXPECT_NO_THROW(renderer->clearAll());
}

TEST_F(FeedbackRendererIntegrationTest, ComplexScene) {
    // Test complex scene with multiple visual elements
    
    // Face highlight
    Face face(Vector3i(5, 5, 5), VoxelResolution::Size_32cm, FaceDirection::PositiveY);
    EXPECT_NO_THROW(renderer->setFaceHighlight(face));
    
    // Voxel preview
    Vector3i previewPos(6, 5, 5);
    EXPECT_NO_THROW(renderer->setVoxelPreview(previewPos, VoxelResolution::Size_32cm, true));
    
    // Selection
    std::vector<VoxelId> selection;
    for (int i = 0; i < 10; ++i) {
        selection.push_back(VoxelId{Vector3i(i, 0, 0), VoxelResolution::Size_32cm});
    }
    EXPECT_NO_THROW(renderer->setSelectionVisualization(selection));
    
    // Workspace bounds
    Vector3f workspaceSize(10.0f, 10.0f, 10.0f);
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    EXPECT_NO_THROW(renderer->setWorkspaceVisualization(workspaceSize, workspaceCenter));
    
    // Performance overlay
    PerformanceMetrics metrics;
    metrics.frameTime = 16.7f;
    metrics.voxelCount = selection.size();
    metrics.memoryUsage = 75.0f;
    EXPECT_NO_THROW(renderer->setPerformanceOverlay(metrics));
    
    // Clear everything
    EXPECT_NO_THROW(renderer->clearAll());
}

TEST_F(FeedbackRendererIntegrationTest, VoxelPreviewMultipleResolutions) {
    // Test preview with different voxel resolutions
    std::vector<VoxelResolution> resolutions = {
        VoxelResolution::Size_1cm,
        VoxelResolution::Size_8cm,
        VoxelResolution::Size_32cm,
        VoxelResolution::Size_128cm
    };
    
    Vector3i basePos(0, 0, 0);
    
    for (size_t i = 0; i < resolutions.size(); ++i) {
        Vector3i position(static_cast<int>(i), 0, 0);
        EXPECT_NO_THROW(renderer->setVoxelPreview(position, resolutions[i], true));
        EXPECT_NO_THROW(renderer->clearVoxelPreview());
    }
}

TEST_F(FeedbackRendererIntegrationTest, PreviewUpdatePerformance) {
    // Test that rapid preview updates maintain good performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        Vector3i position(i % 100, (i / 100) % 10, 0);
        bool isValid = (i % 2 == 0);
        
        renderer->setVoxelPreview(position, VoxelResolution::Size_32cm, isValid);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Performance requirement: 1000 preview updates should complete in under 100ms
    EXPECT_LT(duration.count(), 100);
    
    renderer->clearVoxelPreview();
}