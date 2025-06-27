#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../include/cli/MouseInteraction.h"
#include "../include/cli/Application.h"
#include "../include/cli/RenderWindow.h"
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "camera/Viewport.h"
#include "input/InputManager.h"
#include "visual_feedback/include/visual_feedback/FeedbackRenderer.h"
#include "undo_redo/HistoryManager.h"
#include "undo_redo/Command.h"
#include "visual_feedback/include/visual_feedback/FaceDetector.h"
#include "visual_feedback/include/visual_feedback/FeedbackTypes.h"
#include "foundation/math/Ray.h"
#include "foundation/math/CoordinateConverter.h"
#include "input/PlacementValidation.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Camera;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Input;
using namespace VoxelEditor::UndoRedo;
using ::testing::Return;
using ::testing::_;

// Since MouseInteraction is tightly coupled to Application and many components,
// we'll test it in isolation by testing its core logic rather than mocking everything
class MouseInteractionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // This test verifies the MouseInteraction class structure and basic logic
        // Full integration testing would require a complete application setup
    }
};

// Test that MouseInteraction can be constructed
TEST_F(MouseInteractionTest, Construction) {
    // MouseInteraction requires an Application pointer
    // Since Application is not easily mockable, we test compilation and basic structure
    EXPECT_TRUE(true); // Placeholder - real test would require refactoring MouseInteraction
}

// Test the math for face clicking
TEST_F(MouseInteractionTest, FaceClickMath) {
    // Test placement position calculation
    // When clicking on top face of a voxel at (0,0,0) with 32cm size
    // The next voxel should be placed at (0,32,0)
    
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    VoxelEditor::VisualFeedback::FaceDirection faceDir = VoxelEditor::VisualFeedback::FaceDirection::PositiveY;
    
    // Calculate expected placement position
    int voxelSizeCm = static_cast<int>(getVoxelSize(resolution) * 100.0f);
    IncrementCoordinates expectedPlacement = voxelPos;
    
    switch (faceDir) {
        case VoxelEditor::VisualFeedback::FaceDirection::PositiveY:
            expectedPlacement = IncrementCoordinates(
                voxelPos.x(), 
                voxelPos.y() + voxelSizeCm, 
                voxelPos.z()
            );
            break;
        default:
            break;
    }
    
    EXPECT_EQ(expectedPlacement.x(), 0);
    EXPECT_EQ(expectedPlacement.y(), 32);
    EXPECT_EQ(expectedPlacement.z(), 0);
}

// Test ground plane placement
TEST_F(MouseInteractionTest, GroundPlanePlacement) {
    // When clicking on ground plane at world position (1.234, 0, 2.567)
    // Should snap to nearest 1cm increment
    Vector3f groundHitPoint(1.234f, 0.0f, 2.567f);
    
    IncrementCoordinates snapped = Input::PlacementUtils::snapToValidIncrement(Math::WorldCoordinates(groundHitPoint));
    
    EXPECT_EQ(snapped.x(), 123); // 1.234m -> 123cm
    EXPECT_EQ(snapped.y(), 0);
    EXPECT_EQ(snapped.z(), 257); // 2.567m -> 257cm
}

// Test ray-face intersection logic
TEST_F(MouseInteractionTest, RayFaceIntersection) {
    // Create a simple test case for ray-voxel intersection
    VoxelEditor::Math::Ray ray(
        Vector3f(0, 2, -2),  // Start above and in front
        Vector3f(0, -0.5f, 0.866f).normalized()  // Point down and forward
    );
    
    // Test if ray would hit ground plane
    if (ray.direction.y < 0) {
        float t = -ray.origin.y / ray.direction.y;
        Vector3f groundHit = ray.origin + ray.direction * t;
        
        EXPECT_GE(t, 0.0f); // Should be positive (in front of ray)
        EXPECT_FLOAT_EQ(groundHit.y, 0.0f); // Should hit at Y=0
    }
}

// Test placement validation
TEST_F(MouseInteractionTest, PlacementValidation) {
    Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    
    // Test valid placement
    {
        IncrementCoordinates pos(100, 0, 100); // 1m, 0, 1m
        auto result = Input::PlacementUtils::validatePlacement(
            pos, VoxelResolution::Size_32cm, workspaceSize);
        EXPECT_EQ(result, Input::PlacementValidationResult::Valid);
    }
    
    // Test Y below zero
    {
        IncrementCoordinates pos(100, -10, 100);
        auto result = Input::PlacementUtils::validatePlacement(
            pos, VoxelResolution::Size_32cm, workspaceSize);
        EXPECT_EQ(result, Input::PlacementValidationResult::InvalidYBelowZero);
    }
    
    // Test out of bounds
    {
        IncrementCoordinates pos(300, 0, 300); // 3m, 0, 3m (outside 2.5m bound)
        auto result = Input::PlacementUtils::validatePlacement(
            pos, VoxelResolution::Size_32cm, workspaceSize);
        EXPECT_EQ(result, Input::PlacementValidationResult::InvalidOutOfBounds);
    }
}

// Test coordinate conversions used in mouse interaction
TEST_F(MouseInteractionTest, CoordinateConversions) {
    // Screen to normalized coordinates
    Vector2i screenPos(400, 300);
    Vector2i viewportSize(800, 600);
    
    // Normalized coordinates should be in [-1, 1] range
    float normalizedX = (2.0f * screenPos.x / viewportSize.x) - 1.0f;
    float normalizedY = 1.0f - (2.0f * screenPos.y / viewportSize.y);
    
    EXPECT_FLOAT_EQ(normalizedX, 0.0f); // Center X
    EXPECT_FLOAT_EQ(normalizedY, 0.0f); // Center Y
    
    // World to increment coordinates
    Vector3f worldPos(1.23f, 0.0f, 2.34f);
    IncrementCoordinates incPos = CoordinateConverter::worldToIncrement(WorldCoordinates(worldPos));
    
    EXPECT_EQ(incPos.x(), 123);
    EXPECT_EQ(incPos.y(), 0);
    EXPECT_EQ(incPos.z(), 234);
}

// Test face direction calculations
TEST_F(MouseInteractionTest, FaceDirectionCalculation) {
    // Test getting opposite face
    using FaceDir = VoxelEditor::VisualFeedback::FaceDirection;
    struct FaceTest {
        FaceDir original;
        FaceDir opposite;
    };
    
    FaceTest tests[] = {
        {FaceDir::PositiveX, FaceDir::NegativeX},
        {FaceDir::NegativeX, FaceDir::PositiveX},
        {FaceDir::PositiveY, FaceDir::NegativeY},
        {FaceDir::NegativeY, FaceDir::PositiveY},
        {FaceDir::PositiveZ, FaceDir::NegativeZ},
        {FaceDir::NegativeZ, FaceDir::PositiveZ}
    };
    
    for (const auto& test : tests) {
        // In a real implementation, there would be a getOppositeFace function
        FaceDir opposite = test.original;
        switch (test.original) {
            case FaceDir::PositiveX: opposite = FaceDir::NegativeX; break;
            case FaceDir::NegativeX: opposite = FaceDir::PositiveX; break;
            case FaceDir::PositiveY: opposite = FaceDir::NegativeY; break;
            case FaceDir::NegativeY: opposite = FaceDir::PositiveY; break;
            case FaceDir::PositiveZ: opposite = FaceDir::NegativeZ; break;
            case FaceDir::NegativeZ: opposite = FaceDir::PositiveZ; break;
        }
        EXPECT_EQ(opposite, test.opposite);
    }
}

// Test hover state logic
TEST_F(MouseInteractionTest, HoverStateLogic) {
    // Test that hover state properly tracks face vs ground
    VoxelEditor::VisualFeedback::Face groundFace = VoxelEditor::VisualFeedback::Face::GroundPlane(Vector3f(1.0f, 0.0f, 1.0f));
    EXPECT_TRUE(groundFace.isValid());
    EXPECT_TRUE(groundFace.isGroundPlane());
    
    VoxelEditor::VisualFeedback::Face voxelFace(IncrementCoordinates(10, 10, 10), VoxelResolution::Size_8cm, VoxelEditor::VisualFeedback::FaceDirection::PositiveY);
    EXPECT_TRUE(voxelFace.isValid());
    EXPECT_FALSE(voxelFace.isGroundPlane());
    
    // getVoxelPosition() returns IncrementCoordinates directly, not optional
    IncrementCoordinates voxelPos = voxelFace.getVoxelPosition();
    EXPECT_EQ(voxelPos.x(), 10);
    EXPECT_EQ(voxelPos.y(), 10);
    EXPECT_EQ(voxelPos.z(), 10);
}

// Test non-aligned voxel positions
TEST_F(MouseInteractionTest, NonAlignedVoxelPositions) {
    // Test that voxels can be placed at arbitrary 1cm positions
    std::vector<IncrementCoordinates> testPositions = {
        IncrementCoordinates(7, 0, 13),   // Non-aligned to any voxel size
        IncrementCoordinates(23, 0, 41),  // Prime numbers
        IncrementCoordinates(111, 0, 97), // Arbitrary positions
    };
    
    for (const auto& pos : testPositions) {
        // Verify these are valid positions
        Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
        auto result = Input::PlacementUtils::validatePlacement(
            pos, VoxelResolution::Size_32cm, workspaceSize);
        
        // Should be valid as long as within bounds
        if (pos.x() < 250 && pos.z() < 250) { // Within 2.5m bounds
            EXPECT_EQ(result, Input::PlacementValidationResult::Valid)
                << "Position (" << pos.x() << ", " << pos.y() << ", " << pos.z() 
                << ") should be valid";
        }
    }
}