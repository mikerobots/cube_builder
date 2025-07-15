#include <gtest/gtest.h>
#include "../VoxelDataManager.h"

using namespace VoxelEditor::VoxelData;
// Don't import Math namespace to avoid VoxelResolution ambiguity
// using namespace VoxelEditor::Math;

// REQ-5.2.1: Voxels shall not overlap with existing voxels
// REQ-4.3.1: System shall prevent overlapping voxel placements
// Updated for REQ-2.1.1: Test exact position placement and overlap detection
TEST(CollisionSimple, BasicOverlapAtExactPositions) {
    VoxelDataManager manager;
    
    // Place a 1cm voxel at exact 1cm increment position
    VoxelEditor::Math::IncrementCoordinates pos1(10, 0, 10);
    ASSERT_TRUE(manager.setVoxel(pos1, VoxelResolution::Size_1cm, true));
    
    // Verify the voxel was placed at the exact position
    EXPECT_TRUE(manager.getVoxel(pos1, VoxelResolution::Size_1cm));
    
    // Try to place another voxel at the exact same position - should fail due to overlap
    EXPECT_FALSE(manager.setVoxel(pos1, VoxelResolution::Size_1cm, true));
    
    // Place a voxel at an adjacent position - should succeed
    VoxelEditor::Math::IncrementCoordinates pos2(11, 0, 10);
    EXPECT_TRUE(manager.setVoxel(pos2, VoxelResolution::Size_1cm, true));
    
    // Verify both voxels exist at their exact positions
    EXPECT_TRUE(manager.getVoxel(pos1, VoxelResolution::Size_1cm));
    EXPECT_TRUE(manager.getVoxel(pos2, VoxelResolution::Size_1cm));
    
    // Verify that a position with no voxel returns false
    VoxelEditor::Math::IncrementCoordinates pos3(12, 0, 10);
    EXPECT_FALSE(manager.getVoxel(pos3, VoxelResolution::Size_1cm));
}

// REQ-2.1.1: Test that different voxel sizes can be placed at exact positions without snapping
TEST(CollisionSimple, DifferentSizesAtExactPositions) {
    VoxelDataManager manager;
    
    // Place voxels of different sizes at exact 1cm increment positions
    VoxelEditor::Math::IncrementCoordinates pos1cm(5, 0, 5);    // 1cm voxel
    VoxelEditor::Math::IncrementCoordinates pos4cm(10, 0, 10);  // 4cm voxel
    VoxelEditor::Math::IncrementCoordinates pos16cm(20, 0, 20); // 16cm voxel
    
    // All should succeed at their exact positions
    ASSERT_TRUE(manager.setVoxel(pos1cm, VoxelResolution::Size_1cm, true));
    ASSERT_TRUE(manager.setVoxel(pos4cm, VoxelResolution::Size_4cm, true));
    ASSERT_TRUE(manager.setVoxel(pos16cm, VoxelResolution::Size_16cm, true));
    
    // Verify all are stored at their exact positions (no snapping occurred)
    EXPECT_TRUE(manager.getVoxel(pos1cm, VoxelResolution::Size_1cm));
    EXPECT_TRUE(manager.getVoxel(pos4cm, VoxelResolution::Size_4cm));
    EXPECT_TRUE(manager.getVoxel(pos16cm, VoxelResolution::Size_16cm));
    
    // Verify that placing at the same positions fails due to overlap
    EXPECT_FALSE(manager.setVoxel(pos1cm, VoxelResolution::Size_1cm, true));   // Already exists
    EXPECT_FALSE(manager.setVoxel(pos4cm, VoxelResolution::Size_4cm, true));   // Already exists  
    EXPECT_FALSE(manager.setVoxel(pos16cm, VoxelResolution::Size_16cm, true)); // Already exists
}

// REQ-2.1.1: Test that voxels can be placed at non-aligned positions (no snapping to resolution grids)
TEST(CollisionSimple, NonAlignedPositionPlacement) {
    VoxelDataManager manager;
    
    // Place a 4cm voxel at position (1,0,1) - this would have been invalid under old snapping rules
    VoxelEditor::Math::IncrementCoordinates nonAlignedPos(1, 0, 1);
    ASSERT_TRUE(manager.setVoxel(nonAlignedPos, VoxelResolution::Size_4cm, true));
    
    // Verify the voxel was placed at the exact position (no snapping to 4cm grid occurred)
    EXPECT_TRUE(manager.getVoxel(nonAlignedPos, VoxelResolution::Size_4cm));
    
    // Verify it was NOT snapped to a 4cm-aligned position like (0,0,0) or (4,0,4)
    EXPECT_FALSE(manager.getVoxel(VoxelEditor::Math::IncrementCoordinates(0, 0, 0), VoxelResolution::Size_4cm));
    EXPECT_FALSE(manager.getVoxel(VoxelEditor::Math::IncrementCoordinates(4, 0, 4), VoxelResolution::Size_4cm));
    
    // Place another 4cm voxel at different non-aligned position
    VoxelEditor::Math::IncrementCoordinates anotherPos(3, 0, 7);
    ASSERT_TRUE(manager.setVoxel(anotherPos, VoxelResolution::Size_4cm, true));
    EXPECT_TRUE(manager.getVoxel(anotherPos, VoxelResolution::Size_4cm));
    
    // Test that attempts to place at the same positions fail (redundant operation)
    EXPECT_FALSE(manager.setVoxel(nonAlignedPos, VoxelResolution::Size_4cm, true));   // Already exists
    EXPECT_FALSE(manager.setVoxel(anotherPos, VoxelResolution::Size_4cm, true));     // Already exists
}

// REQ-2.1.2: Test that all voxel sizes maintain their exact placement position
TEST(CollisionSimple, ExactPositionMaintenance) {
    VoxelDataManager manager;
    
    // Test placing voxels at exact 1cm increment positions without any resolution-based snapping
    // Ensure no overlaps between voxels:
    // 1cm at (1,0,1): bounds (0.005-0.015, 0-0.01, 0.005-0.015)
    // 2cm at (3,0,5): bounds (0.02-0.04, 0-0.02, 0.04-0.06)
    // 4cm at (7,0,11): bounds (0.05-0.09, 0-0.04, 0.09-0.13)
    // 8cm at (13,0,17): bounds (0.09-0.17, 0-0.08, 0.13-0.21)
    // 16cm at (25,0,29): bounds (0.17-0.33, 0-0.16, 0.21-0.37) - moved from x=23 to x=25 to avoid overlap
    std::vector<std::pair<VoxelEditor::Math::IncrementCoordinates, VoxelResolution>> testCases = {
        {VoxelEditor::Math::IncrementCoordinates(1, 0, 1), VoxelResolution::Size_1cm},     // 1cm at (1,0,1)
        {VoxelEditor::Math::IncrementCoordinates(3, 0, 5), VoxelResolution::Size_2cm},     // 2cm at (3,0,5)  
        {VoxelEditor::Math::IncrementCoordinates(7, 0, 11), VoxelResolution::Size_4cm},    // 4cm at (7,0,11)
        {VoxelEditor::Math::IncrementCoordinates(13, 0, 17), VoxelResolution::Size_8cm},   // 8cm at (13,0,17)
        {VoxelEditor::Math::IncrementCoordinates(25, 0, 29), VoxelResolution::Size_16cm}   // 16cm at (25,0,29)
    };
    
    // Place all voxels at their exact positions
    for (const auto& testCase : testCases) {
        const auto& pos = testCase.first;
        VoxelResolution resolution = testCase.second;
        
        
        ASSERT_TRUE(manager.setVoxel(pos, resolution, true))
            << "Failed to place " << getVoxelSizeName(resolution) << " voxel at position ("
            << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
    
    // Verify all voxels are stored at their exact positions
    for (const auto& testCase : testCases) {
        const auto& pos = testCase.first;
        VoxelResolution resolution = testCase.second;
        
        EXPECT_TRUE(manager.getVoxel(pos, resolution))
            << static_cast<int>(resolution) << "cm voxel should be at exact position ("
            << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
    
    // Verify that attempting to place at the same positions fails (already exists)
    for (const auto& testCase : testCases) {
        const auto& pos = testCase.first;
        VoxelResolution resolution = testCase.second;
        
        EXPECT_FALSE(manager.setVoxel(pos, resolution, true))
            << "Placement should fail - " << static_cast<int>(resolution) 
            << "cm voxel already exists at position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
}