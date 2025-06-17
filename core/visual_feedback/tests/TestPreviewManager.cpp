#include <gtest/gtest.h>
#include "../include/visual_feedback/PreviewManager.h"
#include "../include/visual_feedback/OutlineRenderer.h"
#include "../include/visual_feedback/FeedbackRenderer.h"
#include "../../rendering/RenderTypes.h"
#include "../../rendering/RenderEngine.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;

class PreviewManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_previewManager = std::make_unique<PreviewManager>();
    }
    
    std::unique_ptr<PreviewManager> m_previewManager;
};

// Test initial state
TEST_F(PreviewManagerTest, InitialState) {
    EXPECT_FALSE(m_previewManager->hasPreview());
    EXPECT_TRUE(m_previewManager->isValid());
    EXPECT_EQ(m_previewManager->getPreviewPosition(), Vector3i(0, 0, 0));
    EXPECT_EQ(m_previewManager->getPreviewResolution(), VoxelData::VoxelResolution::Size_1cm);
}

// Test setting preview position
TEST_F(PreviewManagerTest, SetPreviewPosition) {
    Vector3i position(10, 20, 30);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    m_previewManager->setPreviewPosition(position, resolution);
    
    EXPECT_TRUE(m_previewManager->hasPreview());
    EXPECT_EQ(m_previewManager->getPreviewPosition(), position);
    EXPECT_EQ(m_previewManager->getPreviewResolution(), resolution);
}

// Test validation result handling
TEST_F(PreviewManagerTest, ValidationResultHandling) {
    // Set valid result
    m_previewManager->setValidationResult(Input::PlacementValidationResult::Valid);
    EXPECT_TRUE(m_previewManager->isValid());
    
    // Set invalid Y below zero
    m_previewManager->setValidationResult(Input::PlacementValidationResult::InvalidYBelowZero);
    EXPECT_FALSE(m_previewManager->isValid());
    
    // Set invalid overlap
    m_previewManager->setValidationResult(Input::PlacementValidationResult::InvalidOverlap);
    EXPECT_FALSE(m_previewManager->isValid());
    
    // Set invalid out of bounds
    m_previewManager->setValidationResult(Input::PlacementValidationResult::InvalidOutOfBounds);
    EXPECT_FALSE(m_previewManager->isValid());
}

// Test clearing preview
TEST_F(PreviewManagerTest, ClearPreview) {
    // Set a preview
    m_previewManager->setPreviewPosition(Vector3i(5, 5, 5), VoxelData::VoxelResolution::Size_16cm);
    EXPECT_TRUE(m_previewManager->hasPreview());
    
    // Clear it
    m_previewManager->clearPreview();
    EXPECT_FALSE(m_previewManager->hasPreview());
}

// Test color configuration
TEST_F(PreviewManagerTest, ColorConfiguration) {
    Rendering::Color customValid(0.5f, 1.0f, 0.5f, 1.0f);
    Rendering::Color customInvalid(1.0f, 0.5f, 0.5f, 1.0f);
    
    m_previewManager->setValidColor(customValid);
    m_previewManager->setInvalidColor(customInvalid);
    
    // No direct getter for colors, but we can verify they're stored
    // by checking the outline style creation behavior
    EXPECT_NO_THROW(m_previewManager->setValidColor(customValid));
    EXPECT_NO_THROW(m_previewManager->setInvalidColor(customInvalid));
}

// Test animation settings
TEST_F(PreviewManagerTest, AnimationSettings) {
    m_previewManager->setAnimated(true);
    m_previewManager->setAnimationSpeed(2.0f);
    
    // Update with some delta time
    m_previewManager->update(0.016f);
    
    // Animation should work without errors
    EXPECT_NO_THROW(m_previewManager->update(0.016f));
}

// Test auto-clear functionality
TEST_F(PreviewManagerTest, AutoClear) {
    // Set a preview
    m_previewManager->setPreviewPosition(Vector3i(1, 1, 1), VoxelData::VoxelResolution::Size_1cm);
    EXPECT_TRUE(m_previewManager->hasPreview());
    
    // Update for more than 1 second without setting new position
    for (int i = 0; i < 70; ++i) {
        m_previewManager->update(0.016f); // ~1.12 seconds
    }
    
    // Preview should be cleared
    EXPECT_FALSE(m_previewManager->hasPreview());
}

// Test mouse position tracking
TEST_F(PreviewManagerTest, MousePositionTracking) {
    // Set a preview
    m_previewManager->setPreviewPosition(Vector3i(1, 1, 1), VoxelData::VoxelResolution::Size_1cm);
    
    // Set auto-clear distance
    m_previewManager->setAutoClearDistance(100.0f);
    
    // Small mouse movement - should not clear
    m_previewManager->updateMousePosition(Vector2f(0.0f, 0.0f));
    m_previewManager->updateMousePosition(Vector2f(50.0f, 50.0f));
    EXPECT_TRUE(m_previewManager->hasPreview());
    
    // Large mouse movement - should clear
    m_previewManager->updateMousePosition(Vector2f(200.0f, 200.0f));
    EXPECT_FALSE(m_previewManager->hasPreview());
}

// Test multiple resolution preview positions
TEST_F(PreviewManagerTest, MultipleResolutions) {
    struct TestCase {
        VoxelData::VoxelResolution resolution;
        Vector3i position;
    };
    
    TestCase testCases[] = {
        { VoxelData::VoxelResolution::Size_1cm, Vector3i(100, 50, 75) },
        { VoxelData::VoxelResolution::Size_4cm, Vector3i(25, 12, 18) },
        { VoxelData::VoxelResolution::Size_16cm, Vector3i(6, 3, 4) },
        { VoxelData::VoxelResolution::Size_32cm, Vector3i(3, 1, 2) },
        { VoxelData::VoxelResolution::Size_64cm, Vector3i(1, 0, 1) }
    };
    
    for (const auto& test : testCases) {
        m_previewManager->setPreviewPosition(test.position, test.resolution);
        EXPECT_EQ(m_previewManager->getPreviewPosition(), test.position);
        EXPECT_EQ(m_previewManager->getPreviewResolution(), test.resolution);
    }
}

// Test rendering settings
TEST_F(PreviewManagerTest, RenderingSettings) {
    m_previewManager->setLineWidth(5.0f);
    m_previewManager->setAnimated(false);
    
    // Set a preview
    m_previewManager->setPreviewPosition(Vector3i(1, 1, 1), VoxelData::VoxelResolution::Size_1cm);
    
    // These calls should not throw
    EXPECT_NO_THROW(m_previewManager->setLineWidth(2.0f));
    EXPECT_NO_THROW(m_previewManager->setAnimated(true));
    EXPECT_NO_THROW(m_previewManager->setAnimationSpeed(3.0f));
}

// Mock test for outline generation without OpenGL
TEST_F(PreviewManagerTest, OutlineGenerationMock) {
    // Set preview with valid position
    m_previewManager->setPreviewPosition(Vector3i(10, 20, 30), VoxelData::VoxelResolution::Size_32cm);
    m_previewManager->setValidationResult(Input::PlacementValidationResult::Valid);
    
    // The render method would be called but we can't test actual rendering without OpenGL
    // We can test that it doesn't crash
    OutlineRenderer mockRenderer;
    EXPECT_NO_THROW(m_previewManager->render(mockRenderer));
    
    // Test with invalid position
    m_previewManager->setValidationResult(Input::PlacementValidationResult::InvalidOverlap);
    EXPECT_NO_THROW(m_previewManager->render(mockRenderer));
}

// Test state transitions
TEST_F(PreviewManagerTest, StateTransitions) {
    // No preview -> Has preview
    EXPECT_FALSE(m_previewManager->hasPreview());
    m_previewManager->setPreviewPosition(Vector3i(1, 1, 1), VoxelData::VoxelResolution::Size_1cm);
    EXPECT_TRUE(m_previewManager->hasPreview());
    
    // Valid -> Invalid
    EXPECT_TRUE(m_previewManager->isValid());
    m_previewManager->setValidationResult(Input::PlacementValidationResult::InvalidYBelowZero);
    EXPECT_FALSE(m_previewManager->isValid());
    
    // Invalid -> Valid
    m_previewManager->setValidationResult(Input::PlacementValidationResult::Valid);
    EXPECT_TRUE(m_previewManager->isValid());
    
    // Has preview -> No preview
    m_previewManager->clearPreview();
    EXPECT_FALSE(m_previewManager->hasPreview());
}