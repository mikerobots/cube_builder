#include <gtest/gtest.h>
#include "../include/visual_feedback/HighlightManager.h"
#include "../include/visual_feedback/FeedbackTypes.h"
#include "../../voxel_data/VoxelTypes.h"

namespace VoxelEditor {
namespace VisualFeedback {
namespace Tests {

using namespace Math;
using namespace VoxelData;

class HighlightManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_manager = std::make_unique<HighlightManager>();
    }
    
    std::unique_ptr<HighlightManager> m_manager;
};

// Test single face highlighting constraint
TEST_F(HighlightManagerTest, SingleFaceHighlight) {
    // Create two different faces
    Face face1(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveY);
    Face face2(Vector3i(1, 0, 0), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveY);
    
    // Initially no face highlighted
    EXPECT_FALSE(m_manager->hasFaceHighlight());
    
    // Highlight first face
    m_manager->setHighlightedFace(face1);
    EXPECT_TRUE(m_manager->hasFaceHighlight());
    EXPECT_EQ(m_manager->getCurrentFace(), face1);
    
    // Highlight second face - should replace first
    m_manager->setHighlightedFace(face2);
    EXPECT_TRUE(m_manager->hasFaceHighlight());
    EXPECT_EQ(m_manager->getCurrentFace(), face2);
    EXPECT_NE(m_manager->getCurrentFace(), face1);
}

// Test clearing highlights
TEST_F(HighlightManagerTest, ClearHighlight) {
    Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveY);
    
    // Set and clear
    m_manager->setHighlightedFace(face);
    EXPECT_TRUE(m_manager->hasFaceHighlight());
    
    m_manager->clearFaceHighlight();
    EXPECT_FALSE(m_manager->hasFaceHighlight());
}

// Test highlighting same face multiple times
TEST_F(HighlightManagerTest, HighlightSameFace) {
    Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveY);
    
    // Highlight same face multiple times - should be no-op
    m_manager->setHighlightedFace(face);
    m_manager->setHighlightedFace(face);
    m_manager->setHighlightedFace(face);
    
    EXPECT_TRUE(m_manager->hasFaceHighlight());
    EXPECT_EQ(m_manager->getCurrentFace(), face);
}

// Test invalid face handling
TEST_F(HighlightManagerTest, InvalidFace) {
    Face validFace(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveY);
    Face invalidFace; // Default constructed = invalid
    
    // Set valid face
    m_manager->setHighlightedFace(validFace);
    EXPECT_TRUE(m_manager->hasFaceHighlight());
    
    // Set invalid face - should clear highlight
    m_manager->setHighlightedFace(invalidFace);
    EXPECT_FALSE(m_manager->hasFaceHighlight());
}

// Test animation update
TEST_F(HighlightManagerTest, AnimationUpdate) {
    Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveY);
    
    m_manager->setHighlightedFace(face);
    
    // Update animation - should not crash
    m_manager->update(0.016f); // 16ms = ~60fps
    m_manager->update(0.016f);
    m_manager->update(0.016f);
    
    EXPECT_TRUE(m_manager->hasFaceHighlight());
}

// Test animation enable/disable
TEST_F(HighlightManagerTest, AnimationControl) {
    // Should not crash
    m_manager->setAnimationEnabled(false);
    m_manager->setAnimationEnabled(true);
    
    Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveY);
    m_manager->setHighlightedFace(face);
    
    m_manager->update(0.016f);
}

// Test face transitions
TEST_F(HighlightManagerTest, FaceTransitions) {
    Face face1(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveY);
    Face face2(Vector3i(1, 0, 0), VoxelResolution::Size_32cm, VisualFeedback::FaceDirection::PositiveY);
    
    // Rapid face changes
    m_manager->setHighlightedFace(face1);
    m_manager->update(0.008f); // Half frame
    m_manager->setHighlightedFace(face2);
    m_manager->update(0.008f);
    
    EXPECT_EQ(m_manager->getCurrentFace(), face2);
}

// Test ground plane face highlighting (if supported)
TEST_F(HighlightManagerTest, GroundPlaneFace) {
    // Create a ground plane face
    Face groundFace = Face::GroundPlane(Vector3f(1.0f, 0.0f, 1.0f));
    
    m_manager->setHighlightedFace(groundFace);
    EXPECT_TRUE(m_manager->hasFaceHighlight());
    EXPECT_TRUE(m_manager->getCurrentFace().isGroundPlane());
}

} // namespace Tests
} // namespace VisualFeedback
} // namespace VoxelEditor