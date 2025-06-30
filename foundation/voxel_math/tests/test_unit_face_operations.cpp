#include <gtest/gtest.h>
#include "../include/voxel_math/FaceOperations.h"
#include "../include/voxel_math/VoxelBounds.h"
#include "../include/voxel_math/VoxelGrid.h"
#include <cmath>

using namespace VoxelEditor;
using namespace VoxelEditor::Math;

class FaceOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test data
    }
    
    // Helper function to check if two Vector3f are approximately equal
    bool approxEqual(const Vector3f& a, const Vector3f& b, float epsilon = 1e-5f) {
        return std::abs(a.x - b.x) < epsilon &&
               std::abs(a.y - b.y) < epsilon &&
               std::abs(a.z - b.z) < epsilon;
    }
};

// Test face normal retrieval
TEST_F(FaceOperationsTest, GetFaceNormal) {
    EXPECT_TRUE(approxEqual(FaceOperations::getFaceNormal(VoxelData::FaceDirection::PositiveX), 
                           Vector3f(1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(FaceOperations::getFaceNormal(VoxelData::FaceDirection::NegativeX), 
                           Vector3f(-1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(FaceOperations::getFaceNormal(VoxelData::FaceDirection::PositiveY), 
                           Vector3f(0.0f, 1.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(FaceOperations::getFaceNormal(VoxelData::FaceDirection::NegativeY), 
                           Vector3f(0.0f, -1.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(FaceOperations::getFaceNormal(VoxelData::FaceDirection::PositiveZ), 
                           Vector3f(0.0f, 0.0f, 1.0f)));
    EXPECT_TRUE(approxEqual(FaceOperations::getFaceNormal(VoxelData::FaceDirection::NegativeZ), 
                           Vector3f(0.0f, 0.0f, -1.0f)));
}

// Test face offset calculation
TEST_F(FaceOperationsTest, GetFaceOffset) {
    int voxelSize = 16;  // 16cm
    
    EXPECT_EQ(FaceOperations::getFaceOffset(VoxelData::FaceDirection::PositiveX, voxelSize), 
              Vector3i(16, 0, 0));
    EXPECT_EQ(FaceOperations::getFaceOffset(VoxelData::FaceDirection::NegativeX, voxelSize), 
              Vector3i(-16, 0, 0));
    EXPECT_EQ(FaceOperations::getFaceOffset(VoxelData::FaceDirection::PositiveY, voxelSize), 
              Vector3i(0, 16, 0));
    EXPECT_EQ(FaceOperations::getFaceOffset(VoxelData::FaceDirection::NegativeY, voxelSize), 
              Vector3i(0, -16, 0));
    EXPECT_EQ(FaceOperations::getFaceOffset(VoxelData::FaceDirection::PositiveZ, voxelSize), 
              Vector3i(0, 0, 16));
    EXPECT_EQ(FaceOperations::getFaceOffset(VoxelData::FaceDirection::NegativeZ, voxelSize), 
              Vector3i(0, 0, -16));
}

// Test opposite face retrieval
TEST_F(FaceOperationsTest, GetOppositeFace) {
    EXPECT_EQ(FaceOperations::getOppositeFace(VoxelData::FaceDirection::PositiveX), 
              VoxelData::FaceDirection::NegativeX);
    EXPECT_EQ(FaceOperations::getOppositeFace(VoxelData::FaceDirection::NegativeX), 
              VoxelData::FaceDirection::PositiveX);
    EXPECT_EQ(FaceOperations::getOppositeFace(VoxelData::FaceDirection::PositiveY), 
              VoxelData::FaceDirection::NegativeY);
    EXPECT_EQ(FaceOperations::getOppositeFace(VoxelData::FaceDirection::NegativeY), 
              VoxelData::FaceDirection::PositiveY);
    EXPECT_EQ(FaceOperations::getOppositeFace(VoxelData::FaceDirection::PositiveZ), 
              VoxelData::FaceDirection::NegativeZ);
    EXPECT_EQ(FaceOperations::getOppositeFace(VoxelData::FaceDirection::NegativeZ), 
              VoxelData::FaceDirection::PositiveZ);
}

// Test face determination from hit point
TEST_F(FaceOperationsTest, DetermineFaceFromHit) {
    // Create a voxel at origin with 32cm size
    VoxelBounds bounds(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)), 0.32f);
    
    // Test hitting each face
    float epsilon = 0.01f;
    
    // Hit on positive X face (right)
    WorldCoordinates hitPosX(Vector3f(0.16f, 0.16f, 0.0f));
    EXPECT_EQ(FaceOperations::determineFaceFromHit(hitPosX, bounds, epsilon), 
              VoxelData::FaceDirection::PositiveX);
    
    // Hit on negative X face (left)
    WorldCoordinates hitNegX(Vector3f(-0.16f, 0.16f, 0.0f));
    EXPECT_EQ(FaceOperations::determineFaceFromHit(hitNegX, bounds, epsilon), 
              VoxelData::FaceDirection::NegativeX);
    
    // Hit on positive Y face (top)
    WorldCoordinates hitPosY(Vector3f(0.0f, 0.32f, 0.0f));
    EXPECT_EQ(FaceOperations::determineFaceFromHit(hitPosY, bounds, epsilon), 
              VoxelData::FaceDirection::PositiveY);
    
    // Hit on negative Y face (bottom)
    WorldCoordinates hitNegY(Vector3f(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(FaceOperations::determineFaceFromHit(hitNegY, bounds, epsilon), 
              VoxelData::FaceDirection::NegativeY);
    
    // Hit on positive Z face (back)
    WorldCoordinates hitPosZ(Vector3f(0.0f, 0.16f, 0.16f));
    EXPECT_EQ(FaceOperations::determineFaceFromHit(hitPosZ, bounds, epsilon), 
              VoxelData::FaceDirection::PositiveZ);
    
    // Hit on negative Z face (front)
    WorldCoordinates hitNegZ(Vector3f(0.0f, 0.16f, -0.16f));
    EXPECT_EQ(FaceOperations::determineFaceFromHit(hitNegZ, bounds, epsilon), 
              VoxelData::FaceDirection::NegativeZ);
}

// Test face determination from ray direction
TEST_F(FaceOperationsTest, DetermineFaceFromRayDirection) {
    // Test primary directions
    EXPECT_EQ(FaceOperations::determineFaceFromRayDirection(Vector3f(1.0f, 0.0f, 0.0f)), 
              VoxelData::FaceDirection::PositiveX);
    EXPECT_EQ(FaceOperations::determineFaceFromRayDirection(Vector3f(-1.0f, 0.0f, 0.0f)), 
              VoxelData::FaceDirection::NegativeX);
    EXPECT_EQ(FaceOperations::determineFaceFromRayDirection(Vector3f(0.0f, 1.0f, 0.0f)), 
              VoxelData::FaceDirection::PositiveY);
    EXPECT_EQ(FaceOperations::determineFaceFromRayDirection(Vector3f(0.0f, -1.0f, 0.0f)), 
              VoxelData::FaceDirection::NegativeY);
    EXPECT_EQ(FaceOperations::determineFaceFromRayDirection(Vector3f(0.0f, 0.0f, 1.0f)), 
              VoxelData::FaceDirection::PositiveZ);
    EXPECT_EQ(FaceOperations::determineFaceFromRayDirection(Vector3f(0.0f, 0.0f, -1.0f)), 
              VoxelData::FaceDirection::NegativeZ);
    
    // Test diagonal directions
    Vector3f diagonal(1.0f, 0.5f, 0.5f);
    diagonal.normalize();
    EXPECT_EQ(FaceOperations::determineFaceFromRayDirection(diagonal), 
              VoxelData::FaceDirection::PositiveX);  // X is dominant
    
    Vector3f mostlyY(0.5f, 1.0f, 0.5f);
    mostlyY.normalize();
    EXPECT_EQ(FaceOperations::determineFaceFromRayDirection(mostlyY), 
              VoxelData::FaceDirection::PositiveY);  // Y is dominant
}

// Test placement position calculation
TEST_F(FaceOperationsTest, CalculatePlacementPosition) {
    IncrementCoordinates voxelPos(32, 64, 96);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    
    // Test placement on each face
    EXPECT_EQ(FaceOperations::calculatePlacementPosition(voxelPos, VoxelData::FaceDirection::PositiveX, resolution),
              IncrementCoordinates(48, 64, 96));
    EXPECT_EQ(FaceOperations::calculatePlacementPosition(voxelPos, VoxelData::FaceDirection::NegativeX, resolution),
              IncrementCoordinates(16, 64, 96));
    EXPECT_EQ(FaceOperations::calculatePlacementPosition(voxelPos, VoxelData::FaceDirection::PositiveY, resolution),
              IncrementCoordinates(32, 80, 96));
    EXPECT_EQ(FaceOperations::calculatePlacementPosition(voxelPos, VoxelData::FaceDirection::NegativeY, resolution),
              IncrementCoordinates(32, 48, 96));
    EXPECT_EQ(FaceOperations::calculatePlacementPosition(voxelPos, VoxelData::FaceDirection::PositiveZ, resolution),
              IncrementCoordinates(32, 64, 112));
    EXPECT_EQ(FaceOperations::calculatePlacementPosition(voxelPos, VoxelData::FaceDirection::NegativeZ, resolution),
              IncrementCoordinates(32, 64, 80));
}

// Test bulk face normal retrieval
TEST_F(FaceOperationsTest, GetAllFaceNormals) {
    Vector3f normals[6];
    FaceOperations::getAllFaceNormals(normals);
    
    EXPECT_TRUE(approxEqual(normals[0], Vector3f(1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(normals[1], Vector3f(-1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(normals[2], Vector3f(0.0f, 1.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(normals[3], Vector3f(0.0f, -1.0f, 0.0f)));
    EXPECT_TRUE(approxEqual(normals[4], Vector3f(0.0f, 0.0f, 1.0f)));
    EXPECT_TRUE(approxEqual(normals[5], Vector3f(0.0f, 0.0f, -1.0f)));
}

// Test bulk face offset retrieval
TEST_F(FaceOperationsTest, GetAllFaceOffsets) {
    int voxelSize = 32;  // 32cm
    Vector3i offsets[6];
    FaceOperations::getAllFaceOffsets(voxelSize, offsets);
    
    EXPECT_EQ(offsets[0], Vector3i(32, 0, 0));
    EXPECT_EQ(offsets[1], Vector3i(-32, 0, 0));
    EXPECT_EQ(offsets[2], Vector3i(0, 32, 0));
    EXPECT_EQ(offsets[3], Vector3i(0, -32, 0));
    EXPECT_EQ(offsets[4], Vector3i(0, 0, 32));
    EXPECT_EQ(offsets[5], Vector3i(0, 0, -32));
}

// Test face direction to index conversion
TEST_F(FaceOperationsTest, FaceDirectionToIndex) {
    EXPECT_EQ(FaceOperations::faceDirectionToIndex(VoxelData::FaceDirection::PositiveX), 0);
    EXPECT_EQ(FaceOperations::faceDirectionToIndex(VoxelData::FaceDirection::NegativeX), 1);
    EXPECT_EQ(FaceOperations::faceDirectionToIndex(VoxelData::FaceDirection::PositiveY), 2);
    EXPECT_EQ(FaceOperations::faceDirectionToIndex(VoxelData::FaceDirection::NegativeY), 3);
    EXPECT_EQ(FaceOperations::faceDirectionToIndex(VoxelData::FaceDirection::PositiveZ), 4);
    EXPECT_EQ(FaceOperations::faceDirectionToIndex(VoxelData::FaceDirection::NegativeZ), 5);
}

// Test index to face direction conversion
TEST_F(FaceOperationsTest, IndexToFaceDirection) {
    EXPECT_EQ(FaceOperations::indexToFaceDirection(0), VoxelData::FaceDirection::PositiveX);
    EXPECT_EQ(FaceOperations::indexToFaceDirection(1), VoxelData::FaceDirection::NegativeX);
    EXPECT_EQ(FaceOperations::indexToFaceDirection(2), VoxelData::FaceDirection::PositiveY);
    EXPECT_EQ(FaceOperations::indexToFaceDirection(3), VoxelData::FaceDirection::NegativeY);
    EXPECT_EQ(FaceOperations::indexToFaceDirection(4), VoxelData::FaceDirection::PositiveZ);
    EXPECT_EQ(FaceOperations::indexToFaceDirection(5), VoxelData::FaceDirection::NegativeZ);
}

// Test face direction names
TEST_F(FaceOperationsTest, GetFaceDirectionName) {
    EXPECT_STREQ(FaceOperations::getFaceDirectionName(VoxelData::FaceDirection::PositiveX), "PositiveX");
    EXPECT_STREQ(FaceOperations::getFaceDirectionName(VoxelData::FaceDirection::NegativeX), "NegativeX");
    EXPECT_STREQ(FaceOperations::getFaceDirectionName(VoxelData::FaceDirection::PositiveY), "PositiveY");
    EXPECT_STREQ(FaceOperations::getFaceDirectionName(VoxelData::FaceDirection::NegativeY), "NegativeY");
    EXPECT_STREQ(FaceOperations::getFaceDirectionName(VoxelData::FaceDirection::PositiveZ), "PositiveZ");
    EXPECT_STREQ(FaceOperations::getFaceDirectionName(VoxelData::FaceDirection::NegativeZ), "NegativeZ");
}

// Test edge cases
TEST_F(FaceOperationsTest, EdgeCases) {
    // Test face hit detection on corners
    VoxelBounds bounds(WorldCoordinates(Vector3f(0.0f, 0.0f, 0.0f)), 0.32f);
    
    // Hit exactly on corner - should prefer one face based on minimum distance
    WorldCoordinates cornerHit(Vector3f(0.16f, 0.32f, 0.16f));
    VoxelData::FaceDirection face = FaceOperations::determineFaceFromHit(cornerHit, bounds, 0.01f);
    // Any of the three faces (PositiveX, PositiveY, PositiveZ) would be valid
    EXPECT_TRUE(face == VoxelData::FaceDirection::PositiveX || 
                face == VoxelData::FaceDirection::PositiveY || 
                face == VoxelData::FaceDirection::PositiveZ);
    
    // Test face offset with different voxel sizes
    EXPECT_EQ(FaceOperations::getFaceOffset(VoxelData::FaceDirection::PositiveX, 1), 
              Vector3i(1, 0, 0));  // 1cm voxel
    EXPECT_EQ(FaceOperations::getFaceOffset(VoxelData::FaceDirection::PositiveX, 512), 
              Vector3i(512, 0, 0));  // 512cm voxel
}