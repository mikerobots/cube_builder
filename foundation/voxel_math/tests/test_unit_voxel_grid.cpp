#include <gtest/gtest.h>
#include "../include/voxel_math/VoxelGrid.h"
#include "../../math/CoordinateConverter.h"
#include <cmath>

using namespace VoxelEditor;
using namespace VoxelEditor::Math;

class VoxelGridTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test data
    }
};

// Test snap to increment grid
TEST_F(VoxelGridTest, SnapToIncrementGrid) {
    // Test exact positions
    {
        WorldCoordinates world(Vector3f(1.0f, 2.0f, 3.0f));
        IncrementCoordinates result = VoxelGrid::snapToIncrementGrid(world);
        EXPECT_EQ(result.x(), 100);
        EXPECT_EQ(result.y(), 200);
        EXPECT_EQ(result.z(), 300);
    }
    
    // Test positions that need rounding
    {
        WorldCoordinates world(Vector3f(1.234f, 2.567f, 3.891f));
        IncrementCoordinates result = VoxelGrid::snapToIncrementGrid(world);
        EXPECT_EQ(result.x(), 123);  // Rounds to nearest cm
        EXPECT_EQ(result.y(), 257);
        EXPECT_EQ(result.z(), 389);
    }
    
    // Test negative positions
    {
        WorldCoordinates world(Vector3f(-1.234f, -2.567f, -3.891f));
        IncrementCoordinates result = VoxelGrid::snapToIncrementGrid(world);
        EXPECT_EQ(result.x(), -123);
        EXPECT_EQ(result.y(), -257);
        EXPECT_EQ(result.z(), -389);
    }
    
    // Test very small positions
    {
        WorldCoordinates world(Vector3f(0.005f, 0.006f, 0.004f));
        IncrementCoordinates result = VoxelGrid::snapToIncrementGrid(world);
        EXPECT_EQ(result.x(), 1);  // 0.5cm rounds to 1cm
        EXPECT_EQ(result.y(), 1);  // 0.6cm rounds to 1cm
        EXPECT_EQ(result.z(), 0);  // 0.4cm rounds to 0cm
    }
}

// Test snap to voxel grid from world coordinates
TEST_F(VoxelGridTest, SnapToVoxelGridFromWorld) {
    // Test 16cm voxel grid
    {
        WorldCoordinates world(Vector3f(0.25f, 0.30f, 0.40f));  // 25cm, 30cm, 40cm
        IncrementCoordinates result = VoxelGrid::snapToVoxelGrid(world, VoxelData::VoxelResolution::Size_16cm);
        EXPECT_EQ(result.x(), 16);   // 25cm snaps to 16cm
        EXPECT_EQ(result.y(), 32);   // 30cm snaps to 32cm  
        EXPECT_EQ(result.z(), 32);   // 40cm snaps to 32cm
    }
    
    // Test 32cm voxel grid
    {
        WorldCoordinates world(Vector3f(0.50f, 0.60f, 0.70f));  // 50cm, 60cm, 70cm
        IncrementCoordinates result = VoxelGrid::snapToVoxelGrid(world, VoxelData::VoxelResolution::Size_32cm);
        EXPECT_EQ(result.x(), 32);   // 50cm snaps to 32cm
        EXPECT_EQ(result.y(), 64);   // 60cm snaps to 64cm
        EXPECT_EQ(result.z(), 64);   // 70cm snaps to 64cm
    }
}

// Test snap to voxel grid from increment coordinates
TEST_F(VoxelGridTest, SnapToVoxelGridFromIncrement) {
    // Test 16cm voxel grid
    {
        IncrementCoordinates increment(25, 30, 40);
        IncrementCoordinates result = VoxelGrid::snapToVoxelGrid(increment, VoxelData::VoxelResolution::Size_16cm);
        EXPECT_EQ(result.x(), 16);   // 25cm snaps to 16cm
        EXPECT_EQ(result.y(), 32);   // 30cm snaps to 32cm  
        EXPECT_EQ(result.z(), 32);   // 40cm snaps to 32cm
    }
    
    // Test negative coordinates
    {
        IncrementCoordinates increment(-25, -30, -40);
        IncrementCoordinates result = VoxelGrid::snapToVoxelGrid(increment, VoxelData::VoxelResolution::Size_16cm);
        EXPECT_EQ(result.x(), -32);  // -25cm snaps to -32cm
        EXPECT_EQ(result.y(), -32);  // -30cm snaps to -32cm
        EXPECT_EQ(result.z(), -48);  // -40cm snaps to -48cm
    }
    
    // Test already aligned positions
    {
        IncrementCoordinates increment(32, 64, 96);
        IncrementCoordinates result = VoxelGrid::snapToVoxelGrid(increment, VoxelData::VoxelResolution::Size_32cm);
        EXPECT_EQ(result.x(), 32);   // Already aligned
        EXPECT_EQ(result.y(), 64);   // Already aligned
        EXPECT_EQ(result.z(), 96);   // Already aligned
    }
}

// Test grid alignment check
TEST_F(VoxelGridTest, IsAlignedToGrid) {
    // Test aligned positions
    EXPECT_TRUE(VoxelGrid::isAlignedToGrid(IncrementCoordinates(16, 32, 48), 
                                          VoxelData::VoxelResolution::Size_16cm));
    EXPECT_TRUE(VoxelGrid::isAlignedToGrid(IncrementCoordinates(0, 0, 0), 
                                          VoxelData::VoxelResolution::Size_32cm));
    EXPECT_TRUE(VoxelGrid::isAlignedToGrid(IncrementCoordinates(64, 128, 192), 
                                          VoxelData::VoxelResolution::Size_64cm));
    
    // Test non-aligned positions
    EXPECT_FALSE(VoxelGrid::isAlignedToGrid(IncrementCoordinates(15, 32, 48), 
                                           VoxelData::VoxelResolution::Size_16cm));
    EXPECT_FALSE(VoxelGrid::isAlignedToGrid(IncrementCoordinates(16, 31, 48), 
                                           VoxelData::VoxelResolution::Size_16cm));
    EXPECT_FALSE(VoxelGrid::isAlignedToGrid(IncrementCoordinates(16, 32, 47), 
                                           VoxelData::VoxelResolution::Size_16cm));
    
    // Test negative aligned positions
    EXPECT_TRUE(VoxelGrid::isAlignedToGrid(IncrementCoordinates(-16, -32, -48), 
                                          VoxelData::VoxelResolution::Size_16cm));
}

// Test increment grid check
TEST_F(VoxelGridTest, IsOnIncrementGrid) {
    // Test exact positions
    EXPECT_TRUE(VoxelGrid::isOnIncrementGrid(WorldCoordinates(Vector3f(1.0f, 2.0f, 3.0f))));
    EXPECT_TRUE(VoxelGrid::isOnIncrementGrid(WorldCoordinates(Vector3f(0.01f, 0.02f, 0.03f))));
    
    // Test non-grid positions
    EXPECT_FALSE(VoxelGrid::isOnIncrementGrid(WorldCoordinates(Vector3f(1.005f, 2.0f, 3.0f))));
    EXPECT_FALSE(VoxelGrid::isOnIncrementGrid(WorldCoordinates(Vector3f(1.0f, 2.015f, 3.0f))));
    
    // Test negative positions
    EXPECT_TRUE(VoxelGrid::isOnIncrementGrid(WorldCoordinates(Vector3f(-1.0f, -2.0f, -3.0f))));
    EXPECT_FALSE(VoxelGrid::isOnIncrementGrid(WorldCoordinates(Vector3f(-1.005f, -2.0f, -3.0f))));
}

// Test voxel size getters
TEST_F(VoxelGridTest, VoxelSizeGetters) {
    // Test meters
    EXPECT_FLOAT_EQ(VoxelGrid::getVoxelSizeMeters(VoxelData::VoxelResolution::Size_1cm), 0.01f);
    EXPECT_FLOAT_EQ(VoxelGrid::getVoxelSizeMeters(VoxelData::VoxelResolution::Size_16cm), 0.16f);
    EXPECT_FLOAT_EQ(VoxelGrid::getVoxelSizeMeters(VoxelData::VoxelResolution::Size_32cm), 0.32f);
    EXPECT_FLOAT_EQ(VoxelGrid::getVoxelSizeMeters(VoxelData::VoxelResolution::Size_512cm), 5.12f);
    
    // Test centimeters
    EXPECT_EQ(VoxelGrid::getVoxelSizeCm(VoxelData::VoxelResolution::Size_1cm), 1);
    EXPECT_EQ(VoxelGrid::getVoxelSizeCm(VoxelData::VoxelResolution::Size_16cm), 16);
    EXPECT_EQ(VoxelGrid::getVoxelSizeCm(VoxelData::VoxelResolution::Size_32cm), 32);
    EXPECT_EQ(VoxelGrid::getVoxelSizeCm(VoxelData::VoxelResolution::Size_512cm), 512);
}

// Test face direction offset
TEST_F(VoxelGridTest, FaceDirectionOffset) {
    int voxelSize = 16;  // 16cm
    
    EXPECT_EQ(VoxelGrid::getFaceDirectionOffset(VoxelData::FaceDirection::PosX, voxelSize), 
              Vector3i(16, 0, 0));
    EXPECT_EQ(VoxelGrid::getFaceDirectionOffset(VoxelData::FaceDirection::NegX, voxelSize), 
              Vector3i(-16, 0, 0));
    EXPECT_EQ(VoxelGrid::getFaceDirectionOffset(VoxelData::FaceDirection::PosY, voxelSize), 
              Vector3i(0, 16, 0));
    EXPECT_EQ(VoxelGrid::getFaceDirectionOffset(VoxelData::FaceDirection::NegY, voxelSize), 
              Vector3i(0, -16, 0));
    EXPECT_EQ(VoxelGrid::getFaceDirectionOffset(VoxelData::FaceDirection::PosZ, voxelSize), 
              Vector3i(0, 0, 16));
    EXPECT_EQ(VoxelGrid::getFaceDirectionOffset(VoxelData::FaceDirection::NegZ, voxelSize), 
              Vector3i(0, 0, -16));
}

// Test adjacent position calculation
TEST_F(VoxelGridTest, GetAdjacentPosition) {
    IncrementCoordinates pos(32, 64, 96);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    
    // Test each direction
    EXPECT_EQ(VoxelGrid::getAdjacentPosition(pos, VoxelData::FaceDirection::PosX, resolution),
              IncrementCoordinates(48, 64, 96));
    EXPECT_EQ(VoxelGrid::getAdjacentPosition(pos, VoxelData::FaceDirection::NegX, resolution),
              IncrementCoordinates(16, 64, 96));
    EXPECT_EQ(VoxelGrid::getAdjacentPosition(pos, VoxelData::FaceDirection::PosY, resolution),
              IncrementCoordinates(32, 80, 96));
    EXPECT_EQ(VoxelGrid::getAdjacentPosition(pos, VoxelData::FaceDirection::NegY, resolution),
              IncrementCoordinates(32, 48, 96));
    EXPECT_EQ(VoxelGrid::getAdjacentPosition(pos, VoxelData::FaceDirection::PosZ, resolution),
              IncrementCoordinates(32, 64, 112));
    EXPECT_EQ(VoxelGrid::getAdjacentPosition(pos, VoxelData::FaceDirection::NegZ, resolution),
              IncrementCoordinates(32, 64, 80));
}

// Test bulk adjacent positions
TEST_F(VoxelGridTest, GetAdjacentPositionsBulk) {
    IncrementCoordinates pos(32, 64, 96);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    IncrementCoordinates adjacentPositions[6];
    
    VoxelGrid::getAdjacentPositions(pos, resolution, adjacentPositions);
    
    // Verify all 6 positions
    EXPECT_EQ(adjacentPositions[0], IncrementCoordinates(48, 64, 96));   // +X
    EXPECT_EQ(adjacentPositions[1], IncrementCoordinates(16, 64, 96));   // -X
    EXPECT_EQ(adjacentPositions[2], IncrementCoordinates(32, 80, 96));   // +Y
    EXPECT_EQ(adjacentPositions[3], IncrementCoordinates(32, 48, 96));   // -Y
    EXPECT_EQ(adjacentPositions[4], IncrementCoordinates(32, 64, 112));  // +Z
    EXPECT_EQ(adjacentPositions[5], IncrementCoordinates(32, 64, 80));   // -Z
}

// Test edge cases
TEST_F(VoxelGridTest, EdgeCases) {
    // Test very small voxels (1cm)
    {
        IncrementCoordinates pos(5, 5, 5);
        IncrementCoordinates result = VoxelGrid::snapToVoxelGrid(pos, VoxelData::VoxelResolution::Size_1cm);
        EXPECT_EQ(result, pos);  // 1cm voxels should not change position
    }
    
    // Test very large voxels (512cm)
    {
        IncrementCoordinates pos(300, 400, 500);
        IncrementCoordinates result = VoxelGrid::snapToVoxelGrid(pos, VoxelData::VoxelResolution::Size_512cm);
        EXPECT_EQ(result.x(), 0);    // 300cm snaps to 0cm (floor: 300/512=0)
        EXPECT_EQ(result.y(), 512);  // 400cm snaps to 512cm (Y rounds up)
        EXPECT_EQ(result.z(), 0);    // 500cm snaps to 0cm (floor: 500/512=0)
    }
    
    // Test origin
    {
        IncrementCoordinates origin(0, 0, 0);
        for (int i = 0; i < 10; ++i) {
            auto resolution = static_cast<VoxelData::VoxelResolution>(i);
            EXPECT_TRUE(VoxelGrid::isAlignedToGrid(origin, resolution));
        }
    }
}