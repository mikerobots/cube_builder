#include <gtest/gtest.h>
#include "input/PlacementValidation.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "math/CoordinateTypes.h"
#include "math/Vector3f.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Input;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class PlacementValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a VoxelDataManager for comparison
        m_workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);  // 5m x 5m x 5m workspace
        m_voxelManager = std::make_unique<VoxelDataManager>();
        
        // Set the workspace size
        bool sizeSet = m_voxelManager->getWorkspaceManager()->setSize(m_workspaceSize);
        ASSERT_TRUE(sizeSet) << "Failed to set workspace size";
    }

    Vector3f m_workspaceSize;
    std::unique_ptr<VoxelDataManager> m_voxelManager;
};

// Test that boundary validation matches between PlacementValidation and VoxelDataManager
TEST_F(PlacementValidationTest, BoundaryValidationConsistency) {
    VoxelResolution resolution = VoxelResolution::Size_32cm;  // Large voxel for clear boundary testing
    
    struct TestCase {
        IncrementCoordinates position;
        std::string description;
    };
    
    // Test positions near workspace boundaries
    std::vector<TestCase> testCases = {
        // Valid positions
        { IncrementCoordinates(0, 0, 0), "Center position" },
        { IncrementCoordinates(100, 0, 100), "Valid interior position" },
        { IncrementCoordinates(-100, 0, -100), "Valid negative position" },
        { IncrementCoordinates(0, 100, 0), "Valid elevated position" },
        
        // Boundary cases for 32cm voxels (16cm half-size)
        // Workspace is 5m = 500cm, so bounds are ±250cm
        { IncrementCoordinates(234, 0, 0), "Valid near right boundary (234cm + 16cm = 250cm)" },
        { IncrementCoordinates(235, 0, 0), "Invalid beyond right boundary (235cm + 16cm = 251cm)" },
        { IncrementCoordinates(-234, 0, 0), "Valid near left boundary (-234cm - 16cm = -250cm)" },
        { IncrementCoordinates(-235, 0, 0), "Invalid beyond left boundary (-235cm - 16cm = -251cm)" },
        
        // Y boundary cases (height is 500cm)
        { IncrementCoordinates(0, 468, 0), "Valid near top boundary (468cm + 32cm = 500cm)" },
        { IncrementCoordinates(0, 469, 0), "Invalid beyond top boundary (469cm + 32cm = 501cm)" },
        { IncrementCoordinates(0, -1, 0), "Invalid below ground plane" },
        
        // Z boundary cases
        { IncrementCoordinates(0, 0, 234), "Valid near front boundary" },
        { IncrementCoordinates(0, 0, 235), "Invalid beyond front boundary" },
        { IncrementCoordinates(0, 0, -234), "Valid near back boundary" },
        { IncrementCoordinates(0, 0, -235), "Invalid beyond back boundary" },
    };
    
    for (const auto& testCase : testCases) {
        // Check PlacementValidation result
        PlacementValidationResult placementResult = PlacementUtils::validatePlacement(
            testCase.position, resolution, m_workspaceSize);
        bool placementValid = (placementResult == PlacementValidationResult::Valid);
        
        // Check VoxelDataManager result (both position and overlap)
        bool managerPositionValid = m_voxelManager->isValidPosition(testCase.position, resolution);
        bool managerOverlapValid = !m_voxelManager->wouldOverlap(testCase.position, resolution);
        bool managerValid = managerPositionValid && managerOverlapValid;
        
        // Both should agree
        EXPECT_EQ(placementValid, managerValid) 
            << "Validation mismatch for " << testCase.description 
            << " at position (" << testCase.position.x() << ", " << testCase.position.y() << ", " << testCase.position.z() << ")"
            << "\nPlacementValidation: " << (placementValid ? "valid" : "invalid")
            << "\nVoxelDataManager: " << (managerValid ? "valid" : "invalid")
            << " (position: " << (managerPositionValid ? "valid" : "invalid") 
            << ", overlap: " << (managerOverlapValid ? "valid" : "invalid") << ")";
    }
}

// Test overlap detection consistency
TEST_F(PlacementValidationTest, OverlapDetectionConsistency) {
    VoxelResolution resolution = VoxelResolution::Size_16cm;
    
    // Place a voxel in the center
    IncrementCoordinates existingVoxel(0, 0, 0);
    ASSERT_TRUE(m_voxelManager->setVoxel(existingVoxel, resolution, true)) 
        << "Failed to place existing voxel for overlap test";
    
    struct TestCase {
        IncrementCoordinates position;
        bool shouldOverlap;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        // Same position - should overlap
        { IncrementCoordinates(0, 0, 0), true, "Exact same position" },
        
        // Adjacent positions that shouldn't overlap (16cm voxels have 8cm half-size)
        { IncrementCoordinates(16, 0, 0), false, "Adjacent in X direction (16cm away)" },
        { IncrementCoordinates(-16, 0, 0), false, "Adjacent in -X direction" },
        { IncrementCoordinates(0, 16, 0), false, "Adjacent in Y direction" },
        { IncrementCoordinates(0, 0, 16), false, "Adjacent in Z direction" },
        { IncrementCoordinates(0, 0, -16), false, "Adjacent in -Z direction" },
        
        // Positions that should overlap (closer than 16cm)
        { IncrementCoordinates(8, 0, 0), true, "Overlapping in X direction (8cm away)" },
        { IncrementCoordinates(0, 8, 0), true, "Overlapping in Y direction" },
        { IncrementCoordinates(0, 0, 8), true, "Overlapping in Z direction" },
        
        // Diagonal positions (16cm voxels have 8cm half-size, so they overlap if distance < 16cm in any dimension)
        { IncrementCoordinates(11, 11, 0), true, "Diagonal overlap (11cm each in X,Y)" },
        { IncrementCoordinates(16, 16, 0), false, "Diagonal no overlap (16cm each in X,Y, exactly touching)" },
        
        // Positions far away - no overlap
        { IncrementCoordinates(100, 0, 0), false, "Far away position" },
        { IncrementCoordinates(0, 100, 0), false, "Far away elevated position" },
    };
    
    for (const auto& testCase : testCases) {
        // PlacementValidation doesn't check overlap (only basic constraints)
        PlacementValidationResult placementResult = PlacementUtils::validatePlacement(
            testCase.position, resolution, m_workspaceSize);
        bool placementValid = (placementResult == PlacementValidationResult::Valid);
        
        // VoxelDataManager checks both position and overlap
        bool managerPositionValid = m_voxelManager->isValidPosition(testCase.position, resolution);
        bool managerWouldOverlap = m_voxelManager->wouldOverlap(testCase.position, resolution);
        
        // Verify overlap expectation
        EXPECT_EQ(managerWouldOverlap, testCase.shouldOverlap)
            << "Overlap detection failed for " << testCase.description
            << " at position (" << testCase.position.x() << ", " << testCase.position.y() << ", " << testCase.position.z() << ")"
            << "\nExpected overlap: " << (testCase.shouldOverlap ? "yes" : "no")
            << "\nActual overlap: " << (managerWouldOverlap ? "yes" : "no");
        
        // PlacementValidation should only fail if position is invalid (not due to overlap)
        if (!managerPositionValid) {
            EXPECT_FALSE(placementValid) 
                << "PlacementValidation should reject invalid position for " << testCase.description;
        } else {
            EXPECT_TRUE(placementValid) 
                << "PlacementValidation should accept valid position for " << testCase.description
                << " (overlap checking is handled separately by VoxelDataManager)";
        }
    }
}

// Test preview position calculation matches actual placement
TEST_F(PlacementValidationTest, PreviewMatchesActualPlacement) {
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Test various positions to ensure preview and actual placement logic align
    std::vector<IncrementCoordinates> testPositions = {
        IncrementCoordinates(0, 0, 0),
        IncrementCoordinates(10, 5, -15),
        IncrementCoordinates(-20, 0, 30),
        IncrementCoordinates(50, 25, -50),
    };
    
    for (const auto& position : testPositions) {
        // Check if PlacementValidation thinks position is valid
        PlacementValidationResult placementResult = PlacementUtils::validatePlacement(
            position, resolution, m_workspaceSize);
        bool placementExpected = (placementResult == PlacementValidationResult::Valid);
        
        // Try to actually place the voxel
        bool actualPlacement = m_voxelManager->setVoxel(position, resolution, true);
        
        // If PlacementValidation says it's valid, actual placement should succeed (assuming no overlaps)
        if (placementExpected) {
            bool wouldOverlap = m_voxelManager->wouldOverlap(position, resolution);
            if (!wouldOverlap) {
                EXPECT_TRUE(actualPlacement)
                    << "Actual placement failed despite valid PlacementValidation for position ("
                    << position.x() << ", " << position.y() << ", " << position.z() << ")";
            }
        }
        
        // Clean up for next test
        if (actualPlacement) {
            m_voxelManager->setVoxel(position, resolution, false);
        }
    }
}