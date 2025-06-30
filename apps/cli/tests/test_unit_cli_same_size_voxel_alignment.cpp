#include <gtest/gtest.h>
#include <memory>
#include "cli/MouseInteraction.h"
#include "voxel_data/VoxelDataManager.h"
#include "visual_feedback/FaceDetector.h"
#include "foundation/math/CoordinateConverter.h"
#include "foundation/logging/Logger.h"

namespace VoxelEditor {
namespace Tests {

class SameSizeVoxelAlignmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up minimal logging
        Logging::Logger::getInstance().setLevel(Logging::LogLevel::Warning);
        
        // Create event dispatcher and voxel manager
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>(eventDispatcher.get());
        voxelManager->resizeWorkspace(Math::Vector3f(10.0f, 10.0f, 10.0f));
    }
    
    void TearDown() override {
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    // Helper to calculate expected top face position
    Math::Vector3i getExpectedTopFacePosition(const Math::Vector3i& voxelPos, VoxelData::VoxelResolution resolution) {
        int voxelSize_cm = static_cast<int>(VoxelData::getVoxelSize(resolution) * 100.0f);
        return Math::Vector3i(voxelPos.x, voxelPos.y + voxelSize_cm, voxelPos.z);
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
};

// Test same-size voxel placement alignment
TEST_F(SameSizeVoxelAlignmentTest, TestTopFacePlacementAlignment) {
    // Test with different resolutions
    std::vector<VoxelData::VoxelResolution> resolutions = {
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_8cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_32cm,
        VoxelData::VoxelResolution::Size_64cm
    };
    
    for (auto resolution : resolutions) {
        // Clear voxels for each test
        voxelManager->clear();
        voxelManager->setActiveResolution(resolution);
        
        float voxelSizeMeters = VoxelData::getVoxelSize(resolution);
        int voxelSize_cm = static_cast<int>(voxelSizeMeters * 100.0f);
        
        // Place initial voxel at ground level (Y=0)
        Math::IncrementCoordinates basePos(0, 0, 0);
        bool placed = voxelManager->setVoxel(basePos.value(), resolution, true);
        ASSERT_TRUE(placed) << "Failed to place base voxel for " << voxelSize_cm << "cm resolution";
        
        // Calculate where the top face click should place the next voxel
        Math::Vector3i expectedTopPos = getExpectedTopFacePosition(basePos.value(), resolution);
        
        // Test the adjacent position calculation
        Math::Vector3i adjacentPos = voxelManager->getAdjacentPosition(
            basePos.value(), 
            VoxelData::FaceDirection::PosY,  // Top face
            resolution,  // Source resolution
            resolution   // Target resolution (same size)
        );
        
        EXPECT_EQ(adjacentPos, expectedTopPos) 
            << "Adjacent position calculation incorrect for " << voxelSize_cm << "cm voxels. "
            << "Expected: (" << expectedTopPos.x << "," << expectedTopPos.y << "," << expectedTopPos.z << "), "
            << "Got: (" << adjacentPos.x << "," << adjacentPos.y << "," << adjacentPos.z << ")";
        
        // Place voxel at calculated position
        bool placedTop = voxelManager->setVoxel(adjacentPos, resolution, true);
        ASSERT_TRUE(placedTop) << "Failed to place top voxel for " << voxelSize_cm << "cm resolution";
        
        // Verify the voxels are properly aligned (no gap, no overlap)
        EXPECT_TRUE(voxelManager->hasVoxel(basePos.value(), resolution)) 
            << "Base voxel missing after placement";
        EXPECT_TRUE(voxelManager->hasVoxel(adjacentPos, resolution)) 
            << "Top voxel missing after placement";
        
        // VERTEX ALIGNMENT VALIDATION
        // Calculate world coordinates for vertex validation
        Math::WorldCoordinates baseWorldPos = Math::CoordinateConverter::incrementToWorld(basePos);
        Math::WorldCoordinates topWorldPos = Math::CoordinateConverter::incrementToWorld(Math::IncrementCoordinates(adjacentPos));
        
        // Base voxel vertices (bottom-center coordinate system)
        // Top face of base voxel
        float baseTopY = baseWorldPos.value().y + voxelSizeMeters;
        Math::Vector3f baseTopVertices[4] = {
            Math::Vector3f(baseWorldPos.value().x - voxelSizeMeters/2, baseTopY, baseWorldPos.value().z - voxelSizeMeters/2),
            Math::Vector3f(baseWorldPos.value().x + voxelSizeMeters/2, baseTopY, baseWorldPos.value().z - voxelSizeMeters/2),
            Math::Vector3f(baseWorldPos.value().x + voxelSizeMeters/2, baseTopY, baseWorldPos.value().z + voxelSizeMeters/2),
            Math::Vector3f(baseWorldPos.value().x - voxelSizeMeters/2, baseTopY, baseWorldPos.value().z + voxelSizeMeters/2)
        };
        
        // Bottom face of top voxel
        float topBottomY = topWorldPos.value().y;
        Math::Vector3f topBottomVertices[4] = {
            Math::Vector3f(topWorldPos.value().x - voxelSizeMeters/2, topBottomY, topWorldPos.value().z - voxelSizeMeters/2),
            Math::Vector3f(topWorldPos.value().x + voxelSizeMeters/2, topBottomY, topWorldPos.value().z - voxelSizeMeters/2),
            Math::Vector3f(topWorldPos.value().x + voxelSizeMeters/2, topBottomY, topWorldPos.value().z + voxelSizeMeters/2),
            Math::Vector3f(topWorldPos.value().x - voxelSizeMeters/2, topBottomY, topWorldPos.value().z + voxelSizeMeters/2)
        };
        
        // Validate that corresponding vertices align perfectly
        const float epsilon = 0.0001f; // Small tolerance for floating point comparison
        for (int i = 0; i < 4; i++) {
            EXPECT_NEAR(baseTopVertices[i].x, topBottomVertices[i].x, epsilon) 
                << "X coordinate mismatch at vertex " << i << " for " << voxelSize_cm << "cm voxels";
            EXPECT_NEAR(baseTopVertices[i].y, topBottomVertices[i].y, epsilon) 
                << "Y coordinate mismatch at vertex " << i << " for " << voxelSize_cm << "cm voxels. "
                << "Base top Y: " << baseTopVertices[i].y << ", Top bottom Y: " << topBottomVertices[i].y;
            EXPECT_NEAR(baseTopVertices[i].z, topBottomVertices[i].z, epsilon) 
                << "Z coordinate mismatch at vertex " << i << " for " << voxelSize_cm << "cm voxels";
        }
        
        // Additional check: The Y coordinates should match exactly
        EXPECT_FLOAT_EQ(baseTopY, topBottomY) 
            << "Top face of base voxel should align exactly with bottom face of top voxel. "
            << "Base top Y: " << baseTopY << ", Top bottom Y: " << topBottomY;
        
        // Verify no overlap by checking the space between
        for (int y = 1; y < voxelSize_cm; y++) {
            Math::Vector3i betweenPos(0, y, 0);
            // These positions should be occupied by the base voxel
            bool hasBase = voxelManager->wouldOverlap(betweenPos, VoxelData::VoxelResolution::Size_1cm);
            EXPECT_TRUE(hasBase) << "Gap detected at Y=" << y << " for " << voxelSize_cm << "cm voxels";
        }
        
        // The position exactly at the top face should be the start of the new voxel
        bool topOccupied = voxelManager->wouldOverlap(expectedTopPos, VoxelData::VoxelResolution::Size_1cm);
        EXPECT_TRUE(topOccupied) << "Top voxel not properly aligned at Y=" << expectedTopPos.y;
    }
}

// Test face detection for same-size voxel placement
TEST_F(SameSizeVoxelAlignmentTest, TestFaceDetectionForPlacement) {
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    voxelManager->setActiveResolution(resolution);
    
    // Place a voxel
    Math::IncrementCoordinates voxelPos(0, 0, 0);
    voxelManager->setVoxel(voxelPos.value(), resolution, true);
    
    // Create face detector
    VisualFeedback::FaceDetector faceDetector;
    
    // Get the voxel's world position and size
    Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(voxelPos);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Create a ray hitting the top face center
    Math::Vector3f topFaceCenter = worldPos.value() + Math::Vector3f(voxelSize/2, voxelSize, voxelSize/2);
    Math::Vector3f rayOrigin = topFaceCenter + Math::Vector3f(0, 1.0f, 0); // Above the face
    Math::Vector3f rayDir(0, -1, 0); // Pointing down
    
    VisualFeedback::Ray ray(rayOrigin, rayDir);
    const VoxelData::VoxelGrid* grid = voxelManager->getGrid(resolution);
    
    VisualFeedback::Face face = faceDetector.detectFace(ray, *grid, resolution);
    
    ASSERT_TRUE(face.isValid()) << "Failed to detect top face";
    EXPECT_EQ(face.getDirection(), VisualFeedback::FaceDirection::PositiveY) 
        << "Wrong face direction detected";
    EXPECT_EQ(face.getVoxelPosition(), voxelPos) 
        << "Wrong voxel position detected";
    
    // Calculate placement position
    Math::IncrementCoordinates placementPos = faceDetector.calculatePlacementPosition(face);
    Math::Vector3i expectedPos = getExpectedTopFacePosition(voxelPos.value(), resolution);
    
    EXPECT_EQ(placementPos.value(), expectedPos)
        << "Placement position incorrect. Expected: (" 
        << expectedPos.x << "," << expectedPos.y << "," << expectedPos.z << "), Got: ("
        << placementPos.x() << "," << placementPos.y() << "," << placementPos.z() << ")";
}

// Test sequential vertical stacking
TEST_F(SameSizeVoxelAlignmentTest, TestVerticalStacking) {
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    voxelManager->setActiveResolution(resolution);
    
    int voxelSize_cm = 16;
    int numVoxels = 5;
    
    // Place voxels in a vertical stack
    for (int i = 0; i < numVoxels; i++) {
        Math::Vector3i pos(0, i * voxelSize_cm, 0);
        bool placed = voxelManager->setVoxel(pos, resolution, true);
        ASSERT_TRUE(placed) << "Failed to place voxel " << i << " at Y=" << pos.y;
    }
    
    // Verify all voxels are present and properly aligned
    for (int i = 0; i < numVoxels; i++) {
        Math::Vector3i pos(0, i * voxelSize_cm, 0);
        EXPECT_TRUE(voxelManager->hasVoxel(pos, resolution))
            << "Voxel " << i << " missing at Y=" << pos.y;
        
        // Check that adjacent position calculation is correct
        if (i < numVoxels - 1) {
            Math::Vector3i nextPos = voxelManager->getAdjacentPosition(
                pos, VoxelData::FaceDirection::PosY, resolution, resolution);
            Math::Vector3i expectedNext(0, (i + 1) * voxelSize_cm, 0);
            EXPECT_EQ(nextPos, expectedNext)
                << "Adjacent position incorrect for voxel " << i;
        }
    }
    
    // Verify no gaps in the stack
    for (int y = 0; y < numVoxels * voxelSize_cm; y++) {
        Math::Vector3i checkPos(0, y, 0);
        bool occupied = voxelManager->wouldOverlap(checkPos, VoxelData::VoxelResolution::Size_1cm);
        EXPECT_TRUE(occupied) << "Gap detected in stack at Y=" << y;
    }
}

// Test vertex alignment for all six faces
TEST_F(SameSizeVoxelAlignmentTest, TestAllFacesVertexAlignment) {
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    voxelManager->setActiveResolution(resolution);
    
    float voxelSizeMeters = 0.32f;
    int voxelSize_cm = 32;
    
    // Place center voxel
    Math::IncrementCoordinates centerPos(64, 32, 64); // Away from edges
    voxelManager->setVoxel(centerPos.value(), resolution, true);
    
    struct FaceTest {
        VoxelData::FaceDirection face;
        Math::Vector3i expectedOffset;
        std::string name;
        int faceVertexIndices[4]; // Which vertices of the center voxel form this face
    };
    
    // Define which vertices form each face (for a cube with vertices 0-7)
    // 0: (-x,-y,-z), 1: (+x,-y,-z), 2: (+x,-y,+z), 3: (-x,-y,+z)
    // 4: (-x,+y,-z), 5: (+x,+y,-z), 6: (+x,+y,+z), 7: (-x,+y,+z)
    std::vector<FaceTest> faceTests = {
        {VoxelData::FaceDirection::PosX, Math::Vector3i(32, 0, 0), "PositiveX", {1, 2, 6, 5}},
        {VoxelData::FaceDirection::NegX, Math::Vector3i(-32, 0, 0), "NegativeX", {0, 4, 7, 3}},
        {VoxelData::FaceDirection::PosY, Math::Vector3i(0, 32, 0), "PositiveY", {4, 5, 6, 7}},
        {VoxelData::FaceDirection::NegY, Math::Vector3i(0, -32, 0), "NegativeY", {0, 1, 2, 3}},
        {VoxelData::FaceDirection::PosZ, Math::Vector3i(0, 0, 32), "PositiveZ", {2, 3, 7, 6}},
        {VoxelData::FaceDirection::NegZ, Math::Vector3i(0, 0, -32), "NegativeZ", {0, 1, 5, 4}}
    };
    
    for (const auto& test : faceTests) {
        // Calculate adjacent position
        Math::Vector3i adjacentPos = voxelManager->getAdjacentPosition(
            centerPos.value(), test.face, resolution, resolution);
        
        // Place adjacent voxel
        bool placed = voxelManager->setVoxel(adjacentPos, resolution, true);
        ASSERT_TRUE(placed) << "Failed to place adjacent voxel on " << test.name << " face";
        
        // Get world coordinates
        Math::WorldCoordinates centerWorld = Math::CoordinateConverter::incrementToWorld(centerPos);
        Math::WorldCoordinates adjacentWorld = Math::CoordinateConverter::incrementToWorld(Math::IncrementCoordinates(adjacentPos));
        
        // Calculate all 8 vertices for center voxel
        Math::Vector3f centerVertices[8];
        float cx = centerWorld.value().x;
        float cy = centerWorld.value().y;
        float cz = centerWorld.value().z;
        float h = voxelSizeMeters / 2;
        
        centerVertices[0] = Math::Vector3f(cx - h, cy, cz - h);          // -x, -y, -z
        centerVertices[1] = Math::Vector3f(cx + h, cy, cz - h);          // +x, -y, -z
        centerVertices[2] = Math::Vector3f(cx + h, cy, cz + h);          // +x, -y, +z
        centerVertices[3] = Math::Vector3f(cx - h, cy, cz + h);          // -x, -y, +z
        centerVertices[4] = Math::Vector3f(cx - h, cy + voxelSizeMeters, cz - h); // -x, +y, -z
        centerVertices[5] = Math::Vector3f(cx + h, cy + voxelSizeMeters, cz - h); // +x, +y, -z
        centerVertices[6] = Math::Vector3f(cx + h, cy + voxelSizeMeters, cz + h); // +x, +y, +z
        centerVertices[7] = Math::Vector3f(cx - h, cy + voxelSizeMeters, cz + h); // -x, +y, +z
        
        // Calculate all 8 vertices for adjacent voxel
        Math::Vector3f adjacentVertices[8];
        float ax = adjacentWorld.value().x;
        float ay = adjacentWorld.value().y;
        float az = adjacentWorld.value().z;
        
        adjacentVertices[0] = Math::Vector3f(ax - h, ay, az - h);
        adjacentVertices[1] = Math::Vector3f(ax + h, ay, az - h);
        adjacentVertices[2] = Math::Vector3f(ax + h, ay, az + h);
        adjacentVertices[3] = Math::Vector3f(ax - h, ay, az + h);
        adjacentVertices[4] = Math::Vector3f(ax - h, ay + voxelSizeMeters, az - h);
        adjacentVertices[5] = Math::Vector3f(ax + h, ay + voxelSizeMeters, az - h);
        adjacentVertices[6] = Math::Vector3f(ax + h, ay + voxelSizeMeters, az + h);
        adjacentVertices[7] = Math::Vector3f(ax - h, ay + voxelSizeMeters, az + h);
        
        // Determine which vertices should align based on face
        int alignmentPairs[4][2]; // [center vertex index, adjacent vertex index]
        
        switch (test.face) {
            case VoxelData::FaceDirection::PosX:
                // Center's +X face aligns with adjacent's -X face
                alignmentPairs[0][0] = 1; alignmentPairs[0][1] = 0;
                alignmentPairs[1][0] = 2; alignmentPairs[1][1] = 3;
                alignmentPairs[2][0] = 6; alignmentPairs[2][1] = 7;
                alignmentPairs[3][0] = 5; alignmentPairs[3][1] = 4;
                break;
            case VoxelData::FaceDirection::NegX:
                // Center's -X face aligns with adjacent's +X face
                alignmentPairs[0][0] = 0; alignmentPairs[0][1] = 1;
                alignmentPairs[1][0] = 3; alignmentPairs[1][1] = 2;
                alignmentPairs[2][0] = 7; alignmentPairs[2][1] = 6;
                alignmentPairs[3][0] = 4; alignmentPairs[3][1] = 5;
                break;
            case VoxelData::FaceDirection::PosY:
                // Center's +Y face aligns with adjacent's -Y face
                alignmentPairs[0][0] = 4; alignmentPairs[0][1] = 0;
                alignmentPairs[1][0] = 5; alignmentPairs[1][1] = 1;
                alignmentPairs[2][0] = 6; alignmentPairs[2][1] = 2;
                alignmentPairs[3][0] = 7; alignmentPairs[3][1] = 3;
                break;
            case VoxelData::FaceDirection::NegY:
                // Center's -Y face aligns with adjacent's +Y face
                alignmentPairs[0][0] = 0; alignmentPairs[0][1] = 4;
                alignmentPairs[1][0] = 1; alignmentPairs[1][1] = 5;
                alignmentPairs[2][0] = 2; alignmentPairs[2][1] = 6;
                alignmentPairs[3][0] = 3; alignmentPairs[3][1] = 7;
                break;
            case VoxelData::FaceDirection::PosZ:
                // Center's +Z face aligns with adjacent's -Z face
                alignmentPairs[0][0] = 2; alignmentPairs[0][1] = 1;
                alignmentPairs[1][0] = 3; alignmentPairs[1][1] = 0;
                alignmentPairs[2][0] = 7; alignmentPairs[2][1] = 4;
                alignmentPairs[3][0] = 6; alignmentPairs[3][1] = 5;
                break;
            case VoxelData::FaceDirection::NegZ:
                // Center's -Z face aligns with adjacent's +Z face
                alignmentPairs[0][0] = 0; alignmentPairs[0][1] = 3;
                alignmentPairs[1][0] = 1; alignmentPairs[1][1] = 2;
                alignmentPairs[2][0] = 5; alignmentPairs[2][1] = 6;
                alignmentPairs[3][0] = 4; alignmentPairs[3][1] = 7;
                break;
        }
        
        // Validate vertex alignment
        const float epsilon = 0.0001f;
        for (int i = 0; i < 4; i++) {
            int centerIdx = alignmentPairs[i][0];
            int adjacentIdx = alignmentPairs[i][1];
            
            EXPECT_NEAR(centerVertices[centerIdx].x, adjacentVertices[adjacentIdx].x, epsilon)
                << test.name << " face: X mismatch at vertex pair " << i;
            EXPECT_NEAR(centerVertices[centerIdx].y, adjacentVertices[adjacentIdx].y, epsilon)
                << test.name << " face: Y mismatch at vertex pair " << i;
            EXPECT_NEAR(centerVertices[centerIdx].z, adjacentVertices[adjacentIdx].z, epsilon)
                << test.name << " face: Z mismatch at vertex pair " << i;
        }
        
        // Clean up adjacent voxel for next test
        voxelManager->setVoxel(adjacentPos, resolution, false);
    }
}

// Test edge cases
TEST_F(SameSizeVoxelAlignmentTest, TestEdgeCases) {
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_8cm;
    voxelManager->setActiveResolution(resolution);
    
    // Test 1: Non-aligned starting position (should still work per new requirements)
    Math::IncrementCoordinates nonAlignedPos(3, 0, 5); // 3cm, 0cm, 5cm
    bool placed = voxelManager->setVoxel(nonAlignedPos.value(), resolution, true);
    ASSERT_TRUE(placed) << "Failed to place voxel at non-aligned position";
    
    // Adjacent position should maintain the non-aligned X and Z
    Math::Vector3i topPos = voxelManager->getAdjacentPosition(
        nonAlignedPos.value(), VoxelData::FaceDirection::PosY, resolution, resolution);
    EXPECT_EQ(topPos.x, 3) << "X coordinate should remain non-aligned";
    EXPECT_EQ(topPos.y, 8) << "Y should be offset by voxel size";
    EXPECT_EQ(topPos.z, 5) << "Z coordinate should remain non-aligned";
    
    // Test 2: Negative coordinates (valid in centered system)
    Math::IncrementCoordinates negativePos(-16, 0, -16);
    placed = voxelManager->setVoxel(negativePos.value(), resolution, true);
    ASSERT_TRUE(placed) << "Failed to place voxel at negative position";
    
    Math::Vector3i negTopPos = voxelManager->getAdjacentPosition(
        negativePos.value(), VoxelData::FaceDirection::PosY, resolution, resolution);
    EXPECT_EQ(negTopPos, Math::Vector3i(-16, 8, -16))
        << "Adjacent position incorrect for negative coordinates";
}

} // namespace Tests
} // namespace VoxelEditor