#include <gtest/gtest.h>
#include "../include/visual_feedback/HighlightRenderer.h"
#include "../../selection/SelectionSet.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

class HighlightRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        renderer = std::make_unique<HighlightRenderer>();
    }
    
    std::unique_ptr<HighlightRenderer> renderer;
};

TEST_F(HighlightRendererTest, Construction) {
    EXPECT_TRUE(renderer->isAnimationEnabled());
    
    renderer->setGlobalAnimationEnabled(false);
    EXPECT_FALSE(renderer->isAnimationEnabled());
}

TEST_F(HighlightRendererTest, FaceHighlight) {
    Face face(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    HighlightStyle style = HighlightStyle::Face();
    
    // Should not crash when rendering face
    EXPECT_NO_THROW(renderer->renderFaceHighlight(face, style));
    
    // Should be able to clear highlights
    EXPECT_NO_THROW(renderer->clearFaceHighlights());
}

TEST_F(HighlightRendererTest, VoxelHighlight) {
    Vector3i position(5, 10, 15);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    HighlightStyle style = HighlightStyle::Preview();
    
    // Should not crash when rendering voxel
    EXPECT_NO_THROW(renderer->renderVoxelHighlight(position, resolution, style));
    
    // Should be able to clear highlights
    EXPECT_NO_THROW(renderer->clearVoxelHighlights());
}

TEST_F(HighlightRendererTest, MultiSelection) {
    Selection::SelectionSet selection;
    // TODO: Add voxels to selection once SelectionSet interface is complete
    
    HighlightStyle style = HighlightStyle::Selection();
    
    // Should not crash with empty selection
    EXPECT_NO_THROW(renderer->renderMultiSelection(selection, style));
    
    // Should be able to clear selection highlights
    EXPECT_NO_THROW(renderer->clearSelectionHighlights());
}

TEST_F(HighlightRendererTest, ClearAll) {
    // Add some highlights
    Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    renderer->renderFaceHighlight(face, HighlightStyle::Face());
    renderer->renderVoxelHighlight(Vector3i(1, 1, 1), VoxelResolution::Size_32cm, HighlightStyle::Preview());
    
    // Clear all should not crash
    EXPECT_NO_THROW(renderer->clearAll());
}

TEST_F(HighlightRendererTest, Animation) {
    // Test animation update
    EXPECT_NO_THROW(renderer->update(0.016f)); // 60 FPS frame time
    
    // Add highlight and update
    renderer->renderVoxelHighlight(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, HighlightStyle::Selection());
    EXPECT_NO_THROW(renderer->update(0.016f));
    
    // Disable animation and update
    renderer->setGlobalAnimationEnabled(false);
    EXPECT_NO_THROW(renderer->update(0.016f));
}

TEST_F(HighlightRendererTest, PerformanceSettings) {
    // Test max highlights setting
    renderer->setMaxHighlights(100);
    
    // Test instancing
    renderer->enableInstancing(true);
    renderer->enableInstancing(false);
    
    // Should not crash
    EXPECT_NO_THROW(renderer->update(0.016f));
}

TEST_F(HighlightRendererTest, MultipleHighlights) {
    HighlightStyle style = HighlightStyle::Preview();
    
    // Add multiple voxel highlights
    for (int i = 0; i < 10; ++i) {
        renderer->renderVoxelHighlight(Vector3i(i, 0, 0), VoxelResolution::Size_32cm, style);
    }
    
    // Add multiple face highlights
    for (int i = 0; i < 6; ++i) {
        Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, static_cast<FaceDirection>(i));
        renderer->renderFaceHighlight(face, HighlightStyle::Face());
    }
    
    // Update should handle multiple highlights
    EXPECT_NO_THROW(renderer->update(0.016f));
    
    // Clear should handle multiple highlights
    EXPECT_NO_THROW(renderer->clearAll());
}

TEST_F(HighlightRendererTest, DifferentResolutions) {
    std::vector<VoxelResolution> resolutions = {
        VoxelResolution::Size_1cm,
        VoxelResolution::Size_32cm,
        VoxelResolution::Size_512cm
    };
    
    for (auto res : resolutions) {
        EXPECT_NO_THROW(
            renderer->renderVoxelHighlight(Vector3i(0, 0, 0), res, HighlightStyle::Preview())
        );
    }
    
    EXPECT_NO_THROW(renderer->update(0.016f));
}

TEST_F(HighlightRendererTest, StyleVariations) {
    Vector3i pos(0, 0, 0);
    VoxelResolution res = VoxelResolution::Size_32cm;
    
    // Test different highlight styles
    EXPECT_NO_THROW(renderer->renderVoxelHighlight(pos, res, HighlightStyle::Face()));
    EXPECT_NO_THROW(renderer->renderVoxelHighlight(pos, res, HighlightStyle::Selection()));
    EXPECT_NO_THROW(renderer->renderVoxelHighlight(pos, res, HighlightStyle::Group()));
    EXPECT_NO_THROW(renderer->renderVoxelHighlight(pos, res, HighlightStyle::Preview()));
    
    EXPECT_NO_THROW(renderer->update(0.016f));
}