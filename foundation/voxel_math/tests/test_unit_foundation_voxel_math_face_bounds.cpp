#include <gtest/gtest.h>
#include "voxel_math/VoxelPlacementMath.h"
#include "math/CoordinateTypes.h"
#include "math/CoordinateConverter.h"
#include "voxel_data/VoxelTypes.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;

class FaceBoundsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
};

// Test isWithinFaceBounds for all face directions
TEST_F(FaceBoundsTest, IsWithinFaceBounds_AllFaces) {
    // 32cm voxel at origin
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    // Voxel bounds: X[-16,16], Y[0,32], Z[-16,16] in cm
    
    struct TestCase {
        VoxelData::FaceDirection faceDir;
        WorldCoordinates testPoint;
        bool expectedResult;
        const char* description;
    };
    
    TestCase testCases[] = {
        // Top face (PosY) - check XZ bounds
        {VoxelData::FaceDirection::PosY, WorldCoordinates(0.0f, 0.32f, 0.0f), true, "Top face center"},
        {VoxelData::FaceDirection::PosY, WorldCoordinates(0.15f, 0.32f, 0.15f), true, "Top face near corner"},
        {VoxelData::FaceDirection::PosY, WorldCoordinates(0.17f, 0.32f, 0.0f), false, "Top face outside X"},
        {VoxelData::FaceDirection::PosY, WorldCoordinates(0.0f, 0.32f, 0.17f), false, "Top face outside Z"},
        
        // Bottom face (NegY) - check XZ bounds
        {VoxelData::FaceDirection::NegY, WorldCoordinates(0.0f, 0.0f, 0.0f), true, "Bottom face center"},
        {VoxelData::FaceDirection::NegY, WorldCoordinates(-0.15f, 0.0f, -0.15f), true, "Bottom face near corner"},
        {VoxelData::FaceDirection::NegY, WorldCoordinates(-0.17f, 0.0f, 0.0f), false, "Bottom face outside X"},
        
        // Right face (PosX) - check YZ bounds
        {VoxelData::FaceDirection::PosX, WorldCoordinates(0.16f, 0.16f, 0.0f), true, "Right face center"},
        {VoxelData::FaceDirection::PosX, WorldCoordinates(0.16f, 0.31f, 0.15f), true, "Right face near corner"},
        {VoxelData::FaceDirection::PosX, WorldCoordinates(0.16f, 0.33f, 0.0f), false, "Right face outside Y"},
        {VoxelData::FaceDirection::PosX, WorldCoordinates(0.16f, 0.16f, 0.17f), false, "Right face outside Z"},
        
        // Left face (NegX) - check YZ bounds
        {VoxelData::FaceDirection::NegX, WorldCoordinates(-0.16f, 0.16f, 0.0f), true, "Left face center"},
        {VoxelData::FaceDirection::NegX, WorldCoordinates(-0.16f, -0.01f, 0.0f), false, "Left face below Y"},
        
        // Back face (PosZ) - check XY bounds
        {VoxelData::FaceDirection::PosZ, WorldCoordinates(0.0f, 0.16f, 0.16f), true, "Back face center"},
        {VoxelData::FaceDirection::PosZ, WorldCoordinates(0.15f, 0.31f, 0.16f), true, "Back face near corner"},
        {VoxelData::FaceDirection::PosZ, WorldCoordinates(0.17f, 0.16f, 0.16f), false, "Back face outside X"},
        
        // Front face (NegZ) - check XY bounds
        {VoxelData::FaceDirection::NegZ, WorldCoordinates(0.0f, 0.16f, -0.16f), true, "Front face center"},
        {VoxelData::FaceDirection::NegZ, WorldCoordinates(0.0f, 0.33f, -0.16f), false, "Front face outside Y"},
    };
    
    for (const auto& testCase : testCases) {
        bool result = VoxelPlacementMath::isWithinFaceBounds(
            testCase.testPoint, voxelPos, resolution, testCase.faceDir);
        
        EXPECT_EQ(result, testCase.expectedResult) << testCase.description;
    }
}

// Test calculateFaceGridOrigin for all faces
TEST_F(FaceBoundsTest, CalculateFaceGridOrigin_AllFaces) {
    // 32cm voxel at origin
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    struct TestCase {
        VoxelData::FaceDirection faceDir;
        Vector3f expectedOrigin;
        const char* description;
    };
    
    // Grid origin is the bottom-left corner when looking at each face
    TestCase testCases[] = {
        {VoxelData::FaceDirection::PosY, Vector3f(-0.16f, 0.32f, -0.16f), "Top face origin"},
        {VoxelData::FaceDirection::NegY, Vector3f(-0.16f, 0.0f, -0.16f), "Bottom face origin"},
        {VoxelData::FaceDirection::PosX, Vector3f(0.16f, 0.0f, -0.16f), "Right face origin"},
        {VoxelData::FaceDirection::NegX, Vector3f(-0.16f, 0.0f, 0.16f), "Left face origin"},
        {VoxelData::FaceDirection::PosZ, Vector3f(-0.16f, 0.0f, 0.16f), "Back face origin"},
        {VoxelData::FaceDirection::NegZ, Vector3f(0.16f, 0.0f, -0.16f), "Front face origin"},
    };
    
    for (const auto& testCase : testCases) {
        Vector3f origin = VoxelPlacementMath::calculateFaceGridOrigin(
            voxelPos, resolution, testCase.faceDir);
        
        EXPECT_FLOAT_EQ(origin.x, testCase.expectedOrigin.x) 
            << testCase.description << " - X mismatch";
        EXPECT_FLOAT_EQ(origin.y, testCase.expectedOrigin.y) 
            << testCase.description << " - Y mismatch";
        EXPECT_FLOAT_EQ(origin.z, testCase.expectedOrigin.z) 
            << testCase.description << " - Z mismatch";
    }
}

// Test bounds checking with various voxel sizes
TEST_F(FaceBoundsTest, IsWithinFaceBounds_VariousSizes) {
    struct TestCase {
        VoxelData::VoxelResolution resolution;
        float voxelSize;
        float halfSize;
    };
    
    TestCase testCases[] = {
        {VoxelData::VoxelResolution::Size_1cm, 0.01f, 0.005f},
        {VoxelData::VoxelResolution::Size_8cm, 0.08f, 0.04f},
        {VoxelData::VoxelResolution::Size_64cm, 0.64f, 0.32f},
        {VoxelData::VoxelResolution::Size_512cm, 5.12f, 2.56f},
    };
    
    for (const auto& testCase : testCases) {
        IncrementCoordinates voxelPos(0, 0, 0);
        
        // Test top face bounds
        VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosY;
        
        // Point at center of face - should be within bounds
        WorldCoordinates centerPoint(0.0f, testCase.voxelSize, 0.0f);
        EXPECT_TRUE(VoxelPlacementMath::isWithinFaceBounds(
            centerPoint, voxelPos, testCase.resolution, faceDir))
            << "Center point failed for size " << testCase.voxelSize;
        
        // Point just inside bounds
        WorldCoordinates insidePoint(
            testCase.halfSize - 0.001f, 
            testCase.voxelSize, 
            testCase.halfSize - 0.001f);
        EXPECT_TRUE(VoxelPlacementMath::isWithinFaceBounds(
            insidePoint, voxelPos, testCase.resolution, faceDir))
            << "Inside point failed for size " << testCase.voxelSize;
        
        // Point just outside bounds
        WorldCoordinates outsidePoint(
            testCase.halfSize + 0.001f, 
            testCase.voxelSize, 
            0.0f);
        EXPECT_FALSE(VoxelPlacementMath::isWithinFaceBounds(
            outsidePoint, voxelPos, testCase.resolution, faceDir))
            << "Outside point failed for size " << testCase.voxelSize;
    }
}

// Test edge cases at face boundaries
TEST_F(FaceBoundsTest, IsWithinFaceBounds_EdgeCases) {
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    float epsilon = 0.001f; // Default epsilon
    
    // Test exact boundary positions
    struct TestCase {
        VoxelData::FaceDirection faceDir;
        WorldCoordinates boundaryPoint;
        bool expectedWithDefaultEpsilon;
        const char* description;
    };
    
    TestCase testCases[] = {
        // Top face exact corners
        {VoxelData::FaceDirection::PosY, 
         WorldCoordinates(-0.08f, 0.16f, -0.08f), true, "Top face bottom-left corner"},
        {VoxelData::FaceDirection::PosY, 
         WorldCoordinates(0.08f, 0.16f, 0.08f), true, "Top face top-right corner"},
        
        // Right face exact edges
        {VoxelData::FaceDirection::PosX, 
         WorldCoordinates(0.08f, 0.0f, 0.0f), true, "Right face bottom center"},
        {VoxelData::FaceDirection::PosX, 
         WorldCoordinates(0.08f, 0.16f, 0.0f), true, "Right face top center"},
    };
    
    for (const auto& testCase : testCases) {
        bool result = VoxelPlacementMath::isWithinFaceBounds(
            testCase.boundaryPoint, voxelPos, resolution, testCase.faceDir, epsilon);
        
        EXPECT_EQ(result, testCase.expectedWithDefaultEpsilon) << testCase.description;
    }
    
    // Test with custom epsilon
    float largeEpsilon = 0.01f; // 1cm epsilon
    WorldCoordinates slightlyOutside(0.085f, 0.16f, 0.0f); // 0.5mm outside
    
    // With default epsilon, should be outside
    EXPECT_FALSE(VoxelPlacementMath::isWithinFaceBounds(
        slightlyOutside, voxelPos, resolution, VoxelData::FaceDirection::PosY, epsilon));
    
    // With large epsilon, should be inside
    EXPECT_TRUE(VoxelPlacementMath::isWithinFaceBounds(
        slightlyOutside, voxelPos, resolution, VoxelData::FaceDirection::PosY, largeEpsilon));
}

// Test face grid origin for offset voxels
TEST_F(FaceBoundsTest, CalculateFaceGridOrigin_OffsetVoxels) {
    // Test voxels not at origin
    struct TestCase {
        IncrementCoordinates voxelPos;
        VoxelData::VoxelResolution resolution;
        VoxelData::FaceDirection faceDir;
        Vector3f expectedOrigin;
    };
    
    TestCase testCases[] = {
        // 32cm voxel at (100, 50, -50)
        {{100, 50, -50}, VoxelData::VoxelResolution::Size_32cm, 
         VoxelData::FaceDirection::PosY, 
         Vector3f(1.0f - 0.16f, 0.5f + 0.32f, -0.5f - 0.16f)},
        
        // 8cm voxel at (-20, 0, 30)
        {{-20, 0, 30}, VoxelData::VoxelResolution::Size_8cm, 
         VoxelData::FaceDirection::NegX, 
         Vector3f(-0.2f - 0.04f, 0.0f, 0.3f + 0.04f)},
    };
    
    for (const auto& testCase : testCases) {
        Vector3f origin = VoxelPlacementMath::calculateFaceGridOrigin(
            testCase.voxelPos, testCase.resolution, testCase.faceDir);
        
        EXPECT_FLOAT_EQ(origin.x, testCase.expectedOrigin.x);
        EXPECT_FLOAT_EQ(origin.y, testCase.expectedOrigin.y);
        EXPECT_FLOAT_EQ(origin.z, testCase.expectedOrigin.z);
    }
}

// Test bounds calculation consistency
TEST_F(FaceBoundsTest, VoxelWorldBounds_Consistency) {
    // Test that calculateVoxelWorldBounds is consistent with face bounds
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    
    Vector3f minCorner, maxCorner;
    VoxelPlacementMath::calculateVoxelWorldBounds(voxelPos, resolution, minCorner, maxCorner);
    
    // Test points that should be on each face
    struct FaceTest {
        VoxelData::FaceDirection faceDir;
        WorldCoordinates facePoint;
        const char* description;
    };
    
    FaceTest faceTests[] = {
        {VoxelData::FaceDirection::PosY, WorldCoordinates(0.0f, maxCorner.y, 0.0f), "Top face center"},
        {VoxelData::FaceDirection::NegY, WorldCoordinates(0.0f, minCorner.y, 0.0f), "Bottom face center"},
        {VoxelData::FaceDirection::PosX, WorldCoordinates(maxCorner.x, 0.16f, 0.0f), "Right face center"},
        {VoxelData::FaceDirection::NegX, WorldCoordinates(minCorner.x, 0.16f, 0.0f), "Left face center"},
        {VoxelData::FaceDirection::PosZ, WorldCoordinates(0.0f, 0.16f, maxCorner.z), "Back face center"},
        {VoxelData::FaceDirection::NegZ, WorldCoordinates(0.0f, 0.16f, minCorner.z), "Front face center"},
    };
    
    for (const auto& faceTest : faceTests) {
        bool result = VoxelPlacementMath::isWithinFaceBounds(
            faceTest.facePoint, voxelPos, resolution, faceTest.faceDir);
        
        EXPECT_TRUE(result) << faceTest.description << " should be within face bounds";
    }
}

// Test face bounds for extremely small voxels
TEST_F(FaceBoundsTest, IsWithinFaceBounds_SmallVoxels) {
    // 1cm voxel - test precision
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    // Very precise positions
    VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosY;
    
    // Center of 1cm voxel top face
    WorldCoordinates center(0.0f, 0.01f, 0.0f);
    EXPECT_TRUE(VoxelPlacementMath::isWithinFaceBounds(
        center, voxelPos, resolution, faceDir));
    
    // Just inside bounds (4.9mm from center)
    WorldCoordinates nearEdge(0.0049f, 0.01f, 0.0f);
    EXPECT_TRUE(VoxelPlacementMath::isWithinFaceBounds(
        nearEdge, voxelPos, resolution, faceDir));
    
    // Just outside bounds (5.1mm from center)
    WorldCoordinates outside(0.0051f, 0.01f, 0.0f);
    EXPECT_FALSE(VoxelPlacementMath::isWithinFaceBounds(
        outside, voxelPos, resolution, faceDir));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}