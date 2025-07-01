#include <gtest/gtest.h>
#include <memory>
#include "voxel_data/VoxelDataManager.h"
#include "visual_feedback/FaceDetector.h"
#include "input/PlacementValidation.h"
#include "math/CoordinateConverter.h"
#include "math/Vector3f.h"
#include "events/EventDispatcher.h"

namespace VoxelEditor {

// Integration test for REQ-3.1.1: Same-size voxel automatic edge alignment
// Tests that when placing a same-size voxel on an existing voxel's face,
// the new voxel automatically aligns so its edges match perfectly with the clicked face
class FaceToFaceAlignmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>(eventDispatcher.get());
        voxelManager->resizeWorkspace(Math::Vector3f(10.0f, 10.0f, 10.0f));
        detector = std::make_unique<VisualFeedback::FaceDetector>();
    }
    
    void TearDown() override {
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    // Helper to place a voxel and verify it was placed
    Math::IncrementCoordinates placeVoxel(const Math::IncrementCoordinates& pos, 
                                         VoxelData::VoxelResolution resolution) {
        bool success = voxelManager->setVoxel(pos.value(), resolution, true);
        EXPECT_TRUE(success) << "Failed to place voxel at (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
        return pos;
    }
    
    // Helper to check if two same-size voxels have perfectly aligned edges
    bool haveAlignedEdges(const Math::IncrementCoordinates& pos1, 
                         const Math::IncrementCoordinates& pos2,
                         VoxelData::VoxelResolution resolution) {
        // For same-size voxels to have aligned edges, they must be exactly one voxel size apart
        // on exactly one axis, with the same position on the other two axes
        int voxelSize_cm = static_cast<int>(VoxelData::getVoxelSize(resolution) * 100.0f);
        Math::Vector3i diff = pos2.value() - pos1.value();
        
        // Count how many axes have non-zero differences
        int nonZeroCount = 0;
        bool perfectAlignment = false;
        
        if (diff.x != 0) {
            nonZeroCount++;
            perfectAlignment = (std::abs(diff.x) == voxelSize_cm && diff.y == 0 && diff.z == 0);
        }
        if (diff.y != 0) {
            nonZeroCount++;
            perfectAlignment = perfectAlignment || (std::abs(diff.y) == voxelSize_cm && diff.x == 0 && diff.z == 0);
        }
        if (diff.z != 0) {
            nonZeroCount++;
            perfectAlignment = perfectAlignment || (std::abs(diff.z) == voxelSize_cm && diff.x == 0 && diff.y == 0);
        }
        
        // Must be adjacent on exactly one axis with perfect voxel size distance
        return nonZeroCount == 1 && perfectAlignment;
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
    std::unique_ptr<VisualFeedback::FaceDetector> detector;
};

// REQ-3.1.1: Test automatic edge alignment for same-size voxels placed via face detection
TEST_F(FaceToFaceAlignmentTest, SameSizeVoxelAutomaticEdgeAlignment) {
    // Test with different voxel sizes to ensure alignment works regardless of size
    std::vector<VoxelData::VoxelResolution> testResolutions = {
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_32cm,
        VoxelData::VoxelResolution::Size_64cm
    };
    
    for (auto resolution : testResolutions) {
        // Clear all voxels for each test
        voxelManager->clearAll();
        
        // Place a voxel at a non-aligned position (to test alignment independence)
        Math::IncrementCoordinates basePos(17, 32, 23); // Arbitrary non-aligned position
        placeVoxel(basePos, resolution);
        
        // Test alignment on all 6 faces
        struct FaceTest {
            VisualFeedback::FaceDirection direction;
            Math::Vector3i expectedOffset;
            std::string description;
        };
        
        int voxelSize_cm = static_cast<int>(VoxelData::getVoxelSize(resolution) * 100.0f);
        std::vector<FaceTest> faceTests = {
            {VisualFeedback::FaceDirection::PositiveX, Math::Vector3i(voxelSize_cm, 0, 0), "Positive X face"},
            {VisualFeedback::FaceDirection::NegativeX, Math::Vector3i(-voxelSize_cm, 0, 0), "Negative X face"},
            {VisualFeedback::FaceDirection::PositiveY, Math::Vector3i(0, voxelSize_cm, 0), "Positive Y face"},
            {VisualFeedback::FaceDirection::NegativeY, Math::Vector3i(0, -voxelSize_cm, 0), "Negative Y face"},
            {VisualFeedback::FaceDirection::PositiveZ, Math::Vector3i(0, 0, voxelSize_cm), "Positive Z face"},
            {VisualFeedback::FaceDirection::NegativeZ, Math::Vector3i(0, 0, -voxelSize_cm), "Negative Z face"}
        };
        
        for (const auto& faceTest : faceTests) {
            // Skip negative Y face if it would place voxel below ground
            if (faceTest.direction == VisualFeedback::FaceDirection::NegativeY && 
                basePos.y() + faceTest.expectedOffset.y < 0) {
                continue;
            }
            
            // Create a face on the base voxel
            VisualFeedback::Face face(basePos, resolution, faceTest.direction);
            
            // Calculate where a same-size voxel should be placed for perfect edge alignment
            Math::IncrementCoordinates calculatedPos = detector->calculatePlacementPosition(face);
            Math::IncrementCoordinates expectedPos(basePos.value() + faceTest.expectedOffset);
            
            // REQ-3.1.1: The calculated position should ensure perfect edge alignment
            EXPECT_EQ(calculatedPos, expectedPos) 
                << "For " << static_cast<int>(resolution) << "cm voxel on " << faceTest.description 
                << ": calculated position should ensure perfect edge alignment";
            
            // Verify that this placement would indeed create perfect edge alignment
            EXPECT_TRUE(haveAlignedEdges(basePos, calculatedPos, resolution))
                << "Calculated placement position should create perfect edge alignment";
            
            // Verify the calculated position is valid for placement
            EXPECT_TRUE(voxelManager->isValidPosition(calculatedPos, resolution))
                << "Calculated position should be valid for placement";
            
            // Verify no overlap would occur
            EXPECT_FALSE(voxelManager->wouldOverlap(calculatedPos, resolution))
                << "Calculated position should not overlap with existing voxels";
        }
    }
}

// REQ-3.1.1: Test that edge alignment works regardless of base voxel position
TEST_F(FaceToFaceAlignmentTest, EdgeAlignmentIndependentOfBasePosition) {
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    // Test with various non-aligned base positions
    std::vector<Math::IncrementCoordinates> basePositions = {
        Math::IncrementCoordinates(1, 0, 1),     // Minimal non-aligned
        Math::IncrementCoordinates(15, 16, 31),  // Mid-range non-aligned
        Math::IncrementCoordinates(7, 32, 19),   // Mixed aligned/non-aligned
        Math::IncrementCoordinates(63, 48, 47)   // Large non-aligned
    };
    
    for (const auto& basePos : basePositions) {
        voxelManager->clearAll();
        
        // Place base voxel at non-aligned position
        placeVoxel(basePos, resolution);
        
        // Test placement on positive X face (representative test)
        VisualFeedback::Face face(basePos, resolution, VisualFeedback::FaceDirection::PositiveX);
        Math::IncrementCoordinates calculatedPos = detector->calculatePlacementPosition(face);
        
        // REQ-3.1.1: Regardless of base position, edges should align perfectly
        EXPECT_TRUE(haveAlignedEdges(basePos, calculatedPos, resolution))
            << "Edge alignment should work regardless of base voxel position";
        
        // The aligned position should be exactly one voxel size away on X axis
        Math::Vector3i diff = calculatedPos.value() - basePos.value();
        EXPECT_EQ(diff, Math::Vector3i(32, 0, 0))
            << "Same-size voxel should be placed exactly one voxel size away for perfect alignment";
    }
}


// REQ-3.1.1: Test multiple same-size voxels forming a perfectly aligned chain
TEST_F(FaceToFaceAlignmentTest, PerfectlyAlignedVoxelChain) {
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    // Start with a voxel at non-aligned position
    Math::IncrementCoordinates firstPos(13, 32, 17);
    placeVoxel(firstPos, resolution);
    
    // Build a chain of perfectly aligned voxels using face detection
    std::vector<Math::IncrementCoordinates> chainPositions = {firstPos};
    
    for (int i = 0; i < 3; ++i) {
        // Get the last voxel position
        Math::IncrementCoordinates lastPos = chainPositions.back();
        
        // Create face on positive X side
        VisualFeedback::Face face(lastPos, resolution, VisualFeedback::FaceDirection::PositiveX);
        
        // Calculate placement position for perfect alignment
        Math::IncrementCoordinates nextPos = detector->calculatePlacementPosition(face);
        
        // Place the next voxel
        placeVoxel(nextPos, resolution);
        chainPositions.push_back(nextPos);
        
        // REQ-3.1.1: Each pair should have perfect edge alignment
        EXPECT_TRUE(haveAlignedEdges(lastPos, nextPos, resolution))
            << "Voxels " << i << " and " << (i+1) << " should have perfect edge alignment";
    }
    
    // Verify the entire chain maintains perfect alignment
    for (size_t i = 1; i < chainPositions.size(); ++i) {
        Math::Vector3i diff = chainPositions[i].value() - chainPositions[i-1].value();
        EXPECT_EQ(diff, Math::Vector3i(32, 0, 0))
            << "All voxels in chain should be exactly 32cm apart on X axis";
    }
    
    // Verify no overlaps exist
    for (const auto& pos : chainPositions) {
        EXPECT_TRUE(voxelManager->hasVoxel(pos.value(), resolution))
            << "All voxels in chain should exist";
    }
}

// REQ-3.1.1: Test that alignment works for different voxel sizes
TEST_F(FaceToFaceAlignmentTest, AlignmentWorksForAllVoxelSizes) {
    std::vector<VoxelData::VoxelResolution> resolutions = {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_2cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_8cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_32cm,
        VoxelData::VoxelResolution::Size_64cm
    };
    
    for (auto resolution : resolutions) {
        voxelManager->clearAll();
        
        // Place a voxel at position that's not aligned to this resolution's grid
        Math::IncrementCoordinates basePos(7, 32, 11); // 7cm, 32cm, 11cm
        placeVoxel(basePos, resolution);
        
        // Test placement on positive Z face
        VisualFeedback::Face face(basePos, resolution, VisualFeedback::FaceDirection::PositiveZ);
        Math::IncrementCoordinates calculatedPos = detector->calculatePlacementPosition(face);
        
        // REQ-3.1.1: Should create perfect edge alignment for any voxel size
        EXPECT_TRUE(haveAlignedEdges(basePos, calculatedPos, resolution))
            << "Edge alignment should work for " << static_cast<int>(VoxelData::getVoxelSize(resolution) * Math::CoordinateConverter::METERS_TO_CM) << "cm voxels";
        
        // Verify the distance is exactly one voxel size
        int voxelSize_cm = static_cast<int>(VoxelData::getVoxelSize(resolution) * 100.0f);
        Math::Vector3i diff = calculatedPos.value() - basePos.value();
        EXPECT_EQ(diff, Math::Vector3i(0, 0, voxelSize_cm))
            << "Distance should be exactly " << voxelSize_cm << "cm on Z axis";
    }
}

} // namespace VoxelEditor