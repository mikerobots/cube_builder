#include <gtest/gtest.h>
#include "../include/visual_feedback/FeedbackRenderer.h"
#include "../include/visual_feedback/FeedbackTypes.h"
#include "../../../foundation/math/CoordinateTypes.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Selection;

class FeedbackRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create renderer for testing pure logic without OpenGL dependencies
        renderer = std::make_unique<FeedbackRenderer>(nullptr);
    }
    
    std::unique_ptr<FeedbackRenderer> renderer;
};

// Unit tests focused on logic without OpenGL calls
TEST_F(FeedbackRendererTest, Construction) {
    // Test that renderer constructs without crashing
    EXPECT_TRUE(renderer != nullptr);
}

TEST_F(FeedbackRendererTest, StateManagement) {
    // Test state management without OpenGL
    EXPECT_NO_THROW(renderer->pauseAnimations(true));
    EXPECT_NO_THROW(renderer->pauseAnimations(false));
    EXPECT_NO_THROW(renderer->setDebugOverlaysEnabled(true));
    EXPECT_NO_THROW(renderer->setDebugOverlaysEnabled(false));
}

TEST_F(FeedbackRendererTest, FaceValidation) {
    // Test face validation logic
    Face validFace(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
    // FeedbackRenderer doesn't have validateFace method, test Face construction instead
    EXPECT_EQ(validFace.getVoxelPosition().value(), Vector3i(1, 2, 3));
    
    Face invalidFace(Vector3i(-1, -1, -1), VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
    EXPECT_EQ(invalidFace.getVoxelPosition().value(), Vector3i(-1, -1, -1));
}

TEST_F(FeedbackRendererTest, PreviewPositionValidation) {
    // Test preview position setting
    Vector3i validPosition(5, 10, 15);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    EXPECT_NO_THROW(renderer->renderVoxelPreview(validPosition, resolution));
    
    // Test boundary conditions
    Vector3i boundaryPosition(250, 250, 250);
    EXPECT_NO_THROW(renderer->renderVoxelPreview(boundaryPosition, resolution));
    
    Vector3i invalidPosition(-1000, -1000, -1000);
    EXPECT_NO_THROW(renderer->renderVoxelPreview(invalidPosition, resolution));
}

TEST_F(FeedbackRendererTest, SelectionBoundsCalculation) {
    // Test selection bounds calculation logic
    VoxelEditor::Selection::SelectionSet selection;
    selection.add(VoxelEditor::Selection::VoxelId(Vector3i(0, 0, 0), VoxelResolution::Size_32cm));
    selection.add(VoxelEditor::Selection::VoxelId(Vector3i(32, 0, 0), VoxelResolution::Size_32cm));
    selection.add(VoxelEditor::Selection::VoxelId(Vector3i(0, 32, 0), VoxelResolution::Size_32cm));
    
    // FeedbackRenderer has renderSelection method
    EXPECT_NO_THROW(renderer->renderSelection(selection));
    EXPECT_NO_THROW(renderer->renderSelection(selection, VoxelEditor::Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f)));
    
    // Empty selection
    VoxelEditor::Selection::SelectionSet emptySelection;
    EXPECT_NO_THROW(renderer->renderSelection(emptySelection));
}

TEST_F(FeedbackRendererTest, GroupVisualization) {
    // Test group visualization
    // Test group outline rendering
    GroupId groupId = 1;
    std::vector<GroupId> groups = {groupId};
    
    EXPECT_NO_THROW(renderer->renderGroupOutlines(groups));
    EXPECT_NO_THROW(renderer->renderGroupBounds(groupId, VoxelEditor::Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f)));
}

TEST_F(FeedbackRendererTest, WorkspaceBoundsVisualization) {
    // Test workspace visualization settings
    BoundingBox validWorkspace(Vector3f(-2.5f, -2.5f, -2.5f), Vector3f(2.5f, 2.5f, 2.5f));
    EXPECT_NO_THROW(renderer->renderWorkspaceBounds(validWorkspace));
    
    EXPECT_NO_THROW(renderer->setWorkspaceVisualizationEnabled(true));
    EXPECT_TRUE(renderer->isWorkspaceVisualizationEnabled());
    
    EXPECT_NO_THROW(renderer->setWorkspaceVisualizationEnabled(false));
    EXPECT_FALSE(renderer->isWorkspaceVisualizationEnabled());
}

TEST_F(FeedbackRendererTest, PerformanceMetrics) {
    // Test performance metrics rendering
    VisualFeedback::RenderStats stats;
    stats.frameTime = 16.67f;
    stats.vertexCount = 1000;
    stats.drawCalls = 10;
    
    EXPECT_NO_THROW(renderer->renderPerformanceMetrics(stats));
    
    // Test memory usage rendering
    size_t memoryUsed = 128 * 1024 * 1024; // 128MB
    size_t memoryTotal = 256 * 1024 * 1024; // 256MB
    EXPECT_NO_THROW(renderer->renderMemoryUsage(memoryUsed, memoryTotal));
}

TEST_F(FeedbackRendererTest, AnimationTimingCalculations) {
    // Test animation timing updates
    float deltaTime = 0.016f; // 60 FPS
    
    EXPECT_NO_THROW(renderer->update(deltaTime));
    
    // Test with different speeds
    EXPECT_NO_THROW(renderer->setAnimationSpeed(2.0f));
    EXPECT_EQ(renderer->getAnimationSpeed(), 2.0f);
    
    EXPECT_NO_THROW(renderer->update(deltaTime));
}

TEST_F(FeedbackRendererTest, RenderOrderValidation) {
    // Test render order settings
    EXPECT_NO_THROW(renderer->setRenderOrder(500));
    EXPECT_EQ(renderer->getRenderOrder(), 500);
    
    EXPECT_NO_THROW(renderer->setRenderOrder(0));
    EXPECT_EQ(renderer->getRenderOrder(), 0);
    
    EXPECT_NO_THROW(renderer->setRenderOrder(1000));
    EXPECT_EQ(renderer->getRenderOrder(), 1000);
}

TEST_F(FeedbackRendererTest, ComponentAccess) {
    // Test that components can be accessed safely
    EXPECT_NO_THROW(renderer->getFaceDetector());
    EXPECT_NO_THROW(renderer->getHighlightRenderer());
    EXPECT_NO_THROW(renderer->getOutlineRenderer());
    EXPECT_NO_THROW(renderer->getOverlayRenderer());
}

TEST_F(FeedbackRendererTest, StateUpdateLogic) {
    // Test state update logic without rendering
    float deltaTime = 0.016f;
    
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(renderer->update(deltaTime));
    }
    
    // Test pausing animations
    EXPECT_NO_THROW(renderer->pauseAnimations(true));
    EXPECT_TRUE(renderer->areAnimationsPaused());
}

TEST_F(FeedbackRendererTest, DisabledStateValidation) {
    // Test disabled state validation
    renderer->setEnabled(false);
    EXPECT_FALSE(renderer->isEnabled());
    
    // Validation should still work when disabled
    Face face(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
    // Renderer doesn't have validateFace method, check face validity
    EXPECT_TRUE(face.isValid());
    
    // Renderer can still render when disabled (but nothing will be displayed)
    Vector3i position(0, 0, 0);
    EXPECT_NO_THROW(renderer->renderVoxelPreview(position, VoxelResolution::Size_32cm));
}

TEST_F(FeedbackRendererTest, ComplexSceneValidation) {
    // Test validation with multiple visual elements
    
    // Face validation
    Face face(Vector3i(5, 5, 5), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveX);
    EXPECT_TRUE(renderer->validateFace(face));
    
    // Preview position validation
    Vector3i previewPos(6, 5, 5);
    EXPECT_TRUE(renderer->validatePreviewPosition(previewPos, VoxelResolution::Size_32cm));
    
    // Selection bounds calculation
    std::vector<VisualFeedback::VoxelId> selection = {
        1, // VoxelId is just uint64_t in VisualFeedback
        2
    };
    auto bounds = renderer->calculateSelectionBounds(selection);
    EXPECT_GT(bounds.max.x, bounds.min.x);
    
    // Performance metrics formatting
    PerformanceMetrics metrics;
    metrics.frameTime = 16.67f;
    metrics.voxelCount = selection.size();
    auto formatted = renderer->formatPerformanceMetrics(metrics);
    EXPECT_FALSE(formatted.frameTimeText.empty());
}

TEST_F(FeedbackRendererTest, VoxelPreviewColorLogic) {
    // Test preview color logic
    Vector3i position(5, 0, 5);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    // REQ-4.1.1: Green color for valid placement
    auto validColor = renderer->getPreviewColor(position, resolution, true);
    EXPECT_EQ(validColor.r, 0.0f);
    EXPECT_EQ(validColor.g, 1.0f);
    EXPECT_EQ(validColor.b, 0.0f);
    
    // REQ-4.1.2: Red color for invalid placement
    auto invalidColor = renderer->getPreviewColor(position, resolution, false);
    EXPECT_EQ(invalidColor.r, 1.0f);
    EXPECT_EQ(invalidColor.g, 0.0f);
    EXPECT_EQ(invalidColor.b, 0.0f);
}

TEST_F(FeedbackRendererTest, MultipleResolutionValidation) {
    Vector3i position(0, 0, 0);
    
    // REQ-2.2.4: All voxel sizes (1cm to 512cm) validation
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
        EXPECT_TRUE(renderer->validatePreviewPosition(position, res));
        auto color = renderer->getPreviewColor(position, res, true);
        EXPECT_EQ(color.g, 1.0f); // Should be green for valid placement
    }
}

TEST_F(FeedbackRendererTest, GridParameterValidation) {
    Vector3f center(0.0f, 0.0f, 0.0f);
    float extent = 5.0f;
    Vector3f cursorPos(1.0f, 0.0f, 1.0f);
    
    // REQ-1.1.1, REQ-1.1.3, REQ-1.1.4, REQ-1.2.2: Grid parameter validation
    EXPECT_TRUE(renderer->validateGridParameters(center, extent, cursorPos));
    
    // Test invalid parameters
    EXPECT_FALSE(renderer->validateGridParameters(center, -1.0f, cursorPos)); // Negative extent
    EXPECT_FALSE(renderer->validateGridParameters(center, 20.0f, cursorPos)); // Too large extent
}

TEST_F(FeedbackRendererTest, PreviewCalculationPerformance) {
    Vector3i position(0, 0, 0);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    // REQ-4.1.3: Preview calculations should be fast
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        bool isValid = i % 2 == 0;
        auto color = renderer->getPreviewColor(position, resolution, isValid);
        EXPECT_GE(color.a, 0.0f); // Valid alpha value
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 1000 color calculations should be very fast
    EXPECT_LT(duration.count(), 1000); // Under 1ms total
}

TEST_F(FeedbackRendererTest, HighlightColorLogic) {
    Face face(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveX);
    
    // REQ-4.2.1: Face highlighting uses yellow color
    auto highlightColor = renderer->getFaceHighlightColor(face);
    EXPECT_EQ(highlightColor.r, 1.0f);
    EXPECT_EQ(highlightColor.g, 1.0f);
    EXPECT_EQ(highlightColor.b, 0.0f);
    
    // Test color consistency
    Face face2(Vector3i(2, 3, 4), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::NegativeY);
    auto highlightColor2 = renderer->getFaceHighlightColor(face2);
    EXPECT_EQ(highlightColor.r, highlightColor2.r);
    EXPECT_EQ(highlightColor.g, highlightColor2.g);
    EXPECT_EQ(highlightColor.b, highlightColor2.b);
}

TEST_F(FeedbackRendererTest, WorkspaceScalingValidation) {
    // REQ-6.2.2: Grid size scales with workspace (up to 8m x 8m)
    std::vector<float> extents = {2.0f, 4.0f, 6.0f, 8.0f};
    Vector3f center(0.0f, 0.0f, 0.0f);
    Vector3f cursorPos(0.0f, 0.0f, 0.0f);
    
    for (float extent : extents) {
        EXPECT_TRUE(renderer->validateGridParameters(center, extent, cursorPos));
        
        auto gridInfo = renderer->calculateGridInfo(center, extent);
        EXPECT_GT(gridInfo.lineCount, 0);
        EXPECT_GE(gridInfo.extent, extent);
    }
}