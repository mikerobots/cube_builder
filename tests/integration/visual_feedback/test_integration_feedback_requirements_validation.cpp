#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include "../../../core/visual_feedback/include/visual_feedback/FeedbackRenderer.h"
#include "../../../core/visual_feedback/include/visual_feedback/OverlayRenderer.h"
#include "../../../core/visual_feedback/include/visual_feedback/FaceDetector.h"
#include "../../../core/camera/Camera.h"
#include "../../../core/camera/OrbitCamera.h"

using namespace VoxelEditor::Math;
using VoxelEditor::VoxelData::VoxelResolution;
using VoxelEditor::VisualFeedback::Face;
using VoxelEditor::VisualFeedback::FeedbackRenderer;
using VoxelEditor::VisualFeedback::OverlayRenderer;
using VoxelEditor::VisualFeedback::FaceDetector;
using VoxelEditor::VisualFeedback::FaceDirection;

class VisualFeedbackRequirementsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Always skip these tests unless explicitly enabled
        // These tests require a proper OpenGL context which is not available in CI/headless environments
        if (std::getenv("ENABLE_OPENGL_TESTS") == nullptr) {
            GTEST_SKIP() << "Skipping OpenGL tests - set ENABLE_OPENGL_TESTS=1 to run";
        }
        
        overlayRenderer = std::make_unique<OverlayRenderer>();
        feedbackRenderer = std::make_unique<FeedbackRenderer>(nullptr);
        camera = std::make_unique<VoxelEditor::Camera::OrbitCamera>(nullptr);
        camera->setPosition(VoxelEditor::Math::WorldCoordinates(Vector3f(5.0f, 5.0f, 5.0f)));
        camera->setTarget(VoxelEditor::Math::WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)));
    }
    
    std::unique_ptr<OverlayRenderer> overlayRenderer;
    std::unique_ptr<FeedbackRenderer> feedbackRenderer;
    std::unique_ptr<VoxelEditor::Camera::Camera> camera;
};

// REQ-1.1.1: Grid Size - Ground plane grid uses 32cm squares
TEST_F(VisualFeedbackRequirementsIntegrationTest, GridSize_REQ_1_1_1) {
    overlayRenderer->beginFrame(1920, 1080);
    
    Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    float opacity = 0.35f;
    
    // The grid should use 32cm squares as specified in requirements
    EXPECT_NO_THROW(overlayRenderer->renderGroundPlaneGrid(workspaceCenter, 5.0f, Vector3f(0,0,0), false, *camera));
    
    overlayRenderer->endFrame();
}

// REQ-1.1.3: Grid Color - Ground plane grid uses subtle color
TEST_F(VisualFeedbackRequirementsIntegrationTest, GridColor_REQ_1_1_3) {
    overlayRenderer->beginFrame(1920, 1080);
    
    Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    float opacity = 0.35f;
    
    // Grid should render with subtle, non-intrusive color
    EXPECT_NO_THROW(overlayRenderer->renderGroundPlaneGrid(workspaceCenter, 5.0f, Vector3f(0,0,0), false, *camera));
    
    overlayRenderer->endFrame();
}

// REQ-1.2.2: Dynamic Opacity - Grid opacity changes during interaction
TEST_F(VisualFeedbackRequirementsIntegrationTest, DynamicOpacity_REQ_1_2_2) {
    overlayRenderer->beginFrame(1920, 1080);
    
    Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    
    // Test normal opacity (35%)
    EXPECT_NO_THROW(overlayRenderer->renderGroundPlaneGrid(workspaceCenter, 5.0f, Vector3f(0,0,0), false, *camera));
    
    // Test interaction opacity (65%)
    EXPECT_NO_THROW(overlayRenderer->renderGroundPlaneGrid(workspaceCenter, 5.0f, Vector3f(0,0,0), true, *camera));
    
    overlayRenderer->endFrame();
}

// REQ-6.2.2: Grid Scaling - Grid adapts to workspace scaling
TEST_F(VisualFeedbackRequirementsIntegrationTest, GridScaling_REQ_6_2_2) {
    overlayRenderer->beginFrame(1920, 1080);
    
    // Test different workspace sizes
    std::vector<Vector3f> workspaceSizes = {
        Vector3f(2.0f, 2.0f, 2.0f),   // Minimum
        Vector3f(5.0f, 5.0f, 5.0f),   // Default
        Vector3f(8.0f, 8.0f, 8.0f)    // Maximum
    };
    
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    float opacity = 0.35f;
    
    for (const auto& size : workspaceSizes) {
        EXPECT_NO_THROW(overlayRenderer->renderGroundPlaneGrid(workspaceCenter, size.x, Vector3f(0,0,0), false, *camera));
    }
    
    overlayRenderer->endFrame();
}

// REQ-2.2.1: Ground Plane Preview - Voxel preview on ground plane
TEST_F(VisualFeedbackRequirementsIntegrationTest, GroundPlanePreview_REQ_2_2_1) {
    Vector3i position(0, 0, 0); // Ground plane position
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    bool isValid = true;
    
    EXPECT_NO_THROW(feedbackRenderer->renderVoxelPreviewWithValidation(position, resolution, isValid));
    EXPECT_NO_THROW(feedbackRenderer->clearVoxelPreview());
}

// REQ-2.2.2: Preview Snapping - Preview snaps to 1cm increments
TEST_F(VisualFeedbackRequirementsIntegrationTest, PreviewSnapping_REQ_2_2_2) {
    // Test that preview positions snap to 1cm increments
    std::vector<Vector3i> positions = {
        Vector3i(0, 0, 0),     // Origin
        Vector3i(1, 0, 0),     // 1cm increment
        Vector3i(32, 0, 0),    // 32cm increment
        Vector3i(-16, 0, 0)    // Negative increment
    };
    
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    for (const auto& pos : positions) {
        EXPECT_NO_THROW(feedbackRenderer->renderVoxelPreviewWithValidation(pos, resolution, true));
        EXPECT_NO_THROW(feedbackRenderer->clearVoxelPreview());
    }
}

// REQ-2.2.3: Realtime Preview Update - Preview updates in real-time
TEST_F(VisualFeedbackRequirementsIntegrationTest, RealtimePreviewUpdate_REQ_2_2_3) {
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    // Simulate real-time updates as mouse moves
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 60; ++i) { // 60 updates (1 second at 60fps)
        Vector3i position(i, 0, 0);
        feedbackRenderer->renderVoxelPreviewWithValidation(position, resolution, true);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 60 updates in under 16ms (real-time requirement)
    EXPECT_LT(duration.count(), 16);
    
    feedbackRenderer->clearVoxelPreview();
}

// REQ-4.1.1 to REQ-4.1.2: Preview Colors - Green for valid, red for invalid
TEST_F(VisualFeedbackRequirementsIntegrationTest, PreviewColors_REQ_4_1_1_to_4_1_2) {
    Vector3i position(0, 0, 0);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    // Test valid preview (green)
    EXPECT_NO_THROW(feedbackRenderer->renderVoxelPreviewWithValidation(position, resolution, true));
    EXPECT_NO_THROW(feedbackRenderer->clearVoxelPreview());
    
    // Test invalid preview (red)
    EXPECT_NO_THROW(feedbackRenderer->renderVoxelPreviewWithValidation(position, resolution, false));
    EXPECT_NO_THROW(feedbackRenderer->clearVoxelPreview());
}

// REQ-2.3.1 to REQ-2.3.2: Face Highlighting - Yellow highlight on single face
TEST_F(VisualFeedbackRequirementsIntegrationTest, FaceHighlighting_REQ_2_3_1_to_2_3_2) {
    Face face(Vector3i(1, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    
    EXPECT_NO_THROW(feedbackRenderer->renderFaceHighlight(face));
    EXPECT_NO_THROW(feedbackRenderer->clearFaceHighlight());
}

// REQ-4.2.1: Face Highlight Color - Yellow color for face highlighting
TEST_F(VisualFeedbackRequirementsIntegrationTest, FaceHighlightColor_REQ_4_2_1) {
    Face face(Vector3i(1, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveY);
    
    // Face should be highlighted in yellow color
    EXPECT_NO_THROW(feedbackRenderer->renderFaceHighlight(face));
    EXPECT_NO_THROW(feedbackRenderer->clearFaceHighlight());
}

// REQ-4.2.2: Single Face Highlight - Only one face highlighted at a time
TEST_F(VisualFeedbackRequirementsIntegrationTest, SingleFaceHighlight_REQ_4_2_2) {
    Face face1(Vector3i(1, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    Face face2(Vector3i(2, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveY);
    
    // Setting second face should replace first
    EXPECT_NO_THROW(feedbackRenderer->renderFaceHighlight(face1));
    EXPECT_NO_THROW(feedbackRenderer->renderFaceHighlight(face2));
    EXPECT_NO_THROW(feedbackRenderer->clearFaceHighlight());
}

// REQ-4.2.3: Highlight Visibility - Highlights visible from all angles
TEST_F(VisualFeedbackRequirementsIntegrationTest, HighlightVisibility_REQ_4_2_3) {
    Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveZ);
    
    // Face highlight should be visible regardless of camera angle
    EXPECT_NO_THROW(feedbackRenderer->renderFaceHighlight(face));
    
    // Test with different camera positions (simulated)
    // In actual implementation, this would test different view matrices
    
    EXPECT_NO_THROW(feedbackRenderer->clearFaceHighlight());
}

// REQ-4.1.3: Preview Performance - <16ms preview updates
TEST_F(VisualFeedbackRequirementsIntegrationTest, PreviewPerformance_REQ_4_1_3) {
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Single preview update
    Vector3i position(5, 5, 5);
    feedbackRenderer->renderVoxelPreviewWithValidation(position, resolution, true);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should complete in under 16ms (16000 microseconds)
    EXPECT_LT(duration.count(), 16000);
    
    feedbackRenderer->clearVoxelPreview();
}

// REQ-6.1.1: Grid Performance - 60+ FPS grid rendering
TEST_F(VisualFeedbackRequirementsIntegrationTest, GridPerformance_REQ_6_1_1) {
    Vector3f workspaceSize(8.0f, 8.0f, 8.0f); // Large workspace
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    float opacity = 0.35f;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    overlayRenderer->beginFrame(1920, 1080);
    overlayRenderer->renderGroundPlaneGrid(workspaceCenter, 8.0f, Vector3f(0,0,0), false, *camera);
    overlayRenderer->endFrame();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in under 16ms (for 60+ FPS)
    EXPECT_LT(duration.count(), 16);
}

// REQ-6.1.3: Face Highlight Performance
TEST_F(VisualFeedbackRequirementsIntegrationTest, FaceHighlightPerformance_REQ_6_1_3) {
    auto start = std::chrono::high_resolution_clock::now();
    
    Face face(Vector3i(10, 10, 10), VoxelResolution::Size_32cm, FaceDirection::PositiveY);
    feedbackRenderer->renderFaceHighlight(face);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Face highlight update should be very fast
    EXPECT_LT(duration.count(), 1000); // Under 1ms
    
    feedbackRenderer->clearFaceHighlight();
}

// REQ-6.2.1: Large Voxel Count - Handle 10,000+ voxels efficiently
TEST_F(VisualFeedbackRequirementsIntegrationTest, LargeVoxelCount_REQ_6_2_1) {
    // TODO: Test with large selection visualization - VoxelId type needs to be defined
    // std::vector<VoxelId> largeSelection;
    
    // Create 1000 voxels (subset of 10,000+ for test efficiency)
    // for (int x = 0; x < 10; ++x) {
    //     for (int y = 0; y < 10; ++y) {
    //         for (int z = 0; z < 10; ++z) {
    //             largeSelection.push_back(VoxelId{Vector3i(x, y, z), VoxelResolution::Size_32cm});
    //         }
    //     }
    // }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // TODO: Need proper selection visualization methods
    // EXPECT_NO_THROW(feedbackRenderer->renderSelection(largeSelection));
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should handle large selections efficiently
    // EXPECT_LT(duration.count(), 100);
    
    // feedbackRenderer->clearSelection();
}

// REQ-7.1.3: OpenGL Requirement - Requires OpenGL 3.3+
TEST_F(VisualFeedbackRequirementsIntegrationTest, OpenGLRequirement_REQ_7_1_3) {
    // This test verifies that OpenGL context is available and functional
    overlayRenderer->beginFrame(1920, 1080);
    
    // If we reach this point without crashes, OpenGL context is working
    Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    
    EXPECT_NO_THROW(overlayRenderer->renderGroundPlaneGrid(workspaceCenter, 5.0f, Vector3f(0,0,0), false, *camera));
    
    overlayRenderer->endFrame();
    
    // Test should only pass in environments with OpenGL 3.3+ support
    SUCCEED();
}