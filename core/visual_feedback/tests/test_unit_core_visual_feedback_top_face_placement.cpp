#include <gtest/gtest.h>
#include "visual_feedback/FaceDetector.h"
#include "visual_feedback/GeometricFaceDetector.h"
#include "voxel_data/VoxelDataManager.h"
#include "math/CoordinateConverter.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;

class TopFacePlacementTest : public ::testing::Test {
protected:
    void SetUp() override {
        resolution = VoxelData::VoxelResolution::Size_32cm;
        voxelSize = VoxelData::getVoxelSize(resolution);
        
        // Place a single voxel at a known position
        voxelPos = IncrementCoordinates(64, 32, 96);
        bool placed = voxelManager.setVoxel(voxelPos, resolution, true);
        ASSERT_TRUE(placed) << "Failed to place initial voxel";
        
        grid = voxelManager.getGrid(resolution);
        ASSERT_NE(grid, nullptr) << "No grid found for resolution";
        
        // Calculate voxel world position and top face center
        WorldCoordinates voxelWorld = CoordinateConverter::incrementToWorld(voxelPos);
        topFaceCenter = Vector3f(
            voxelWorld.value().x + voxelSize / 2.0f,  // X center
            voxelWorld.value().y + voxelSize,         // Y top
            voxelWorld.value().z + voxelSize / 2.0f   // Z center
        );
        
        std::cout << "Voxel at: (" << voxelPos.x() << ", " << voxelPos.y() << ", " << voxelPos.z() << ")" << std::endl;
        std::cout << "Voxel world pos: (" << voxelWorld.value().x << ", " << voxelWorld.value().y << ", " << voxelWorld.value().z << ")" << std::endl;
        std::cout << "Top face center: (" << topFaceCenter.x << ", " << topFaceCenter.y << ", " << topFaceCenter.z << ")" << std::endl;
        std::cout << "Voxel size: " << voxelSize << " meters" << std::endl;
    }

    VoxelData::VoxelDataManager voxelManager;
    VoxelData::VoxelResolution resolution;
    const VoxelData::VoxelGrid* grid;
    IncrementCoordinates voxelPos;
    float voxelSize;
    Vector3f topFaceCenter;
};

TEST_F(TopFacePlacementTest, TopFaceQuadrantClicking) {
    std::cout << "\n=== TESTING TOP FACE QUADRANT CLICKING ===" << std::endl;
    
    FaceDetector faceDetector;
    
    // Define 4 quadrants of the top face
    struct QuadrantTest {
        std::string name;
        Vector3f clickPoint;
        std::string description;
    };
    
    float quadrantOffset = voxelSize * 0.25f;  // 25% from center toward edge
    
    std::vector<QuadrantTest> quadrants = {
        {"TopLeft", Vector3f(topFaceCenter.x - quadrantOffset, topFaceCenter.y, topFaceCenter.z - quadrantOffset), "Top-left quadrant"},
        {"TopRight", Vector3f(topFaceCenter.x + quadrantOffset, topFaceCenter.y, topFaceCenter.z + quadrantOffset), "Top-right quadrant"}, 
        {"BottomLeft", Vector3f(topFaceCenter.x - quadrantOffset, topFaceCenter.y, topFaceCenter.z + quadrantOffset), "Bottom-left quadrant"},
        {"BottomRight", Vector3f(topFaceCenter.x + quadrantOffset, topFaceCenter.y, topFaceCenter.z - quadrantOffset), "Bottom-right quadrant"}
    };
    
    int successCount = 0;
    
    for (const auto& quadrant : quadrants) {
        std::cout << "\n--- Testing " << quadrant.name << " (" << quadrant.description << ") ---" << std::endl;
        std::cout << "Click point: (" << quadrant.clickPoint.x << ", " << quadrant.clickPoint.y << ", " << quadrant.clickPoint.z << ")" << std::endl;
        
        // Create ray from above the voxel pointing down toward the click point
        Vector3f rayOrigin(quadrant.clickPoint.x, topFaceCenter.y + 2.0f, quadrant.clickPoint.z);
        Vector3f rayDir = (quadrant.clickPoint - rayOrigin).normalized();
        VisualFeedback::Ray ray = {rayOrigin, rayDir};
        
        std::cout << "Ray origin: (" << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << ")" << std::endl;
        std::cout << "Ray direction: (" << rayDir.x << ", " << rayDir.y << ", " << rayDir.z << ")" << std::endl;
        
        // Detect face
        Face detectedFace = faceDetector.detectFaceAcrossAllResolutions(ray, voxelManager);
        
        // Validate face detection
        ASSERT_TRUE(detectedFace.isValid()) << "Failed to detect any face for " << quadrant.name;
        ASSERT_FALSE(detectedFace.isGroundPlane()) << "Should detect voxel face, not ground plane for " << quadrant.name;
        
        std::cout << "Detected face - Voxel: (" << detectedFace.getVoxelPosition().x() << ", " 
                  << detectedFace.getVoxelPosition().y() << ", " << detectedFace.getVoxelPosition().z() 
                  << "), Direction: " << static_cast<int>(detectedFace.getDirection()) << std::endl;
        
        // Validate it's the correct voxel and face
        EXPECT_EQ(detectedFace.getVoxelPosition(), voxelPos) << "Should detect the correct voxel for " << quadrant.name;
        EXPECT_EQ(detectedFace.getDirection(), FaceDirection::PositiveY) << "Should detect +Y (top) face for " << quadrant.name;
        
        // Calculate placement position
        IncrementCoordinates placementPos = faceDetector.calculatePlacementPosition(detectedFace);
        std::cout << "Placement position: (" << placementPos.x() << ", " << placementPos.y() << ", " << placementPos.z() << ")" << std::endl;
        
        // Expected placement should be one voxel above the original
        IncrementCoordinates expectedPlacement(voxelPos.x(), voxelPos.y() + 32, voxelPos.z());  // +32cm in Y
        EXPECT_EQ(placementPos, expectedPlacement) << "Placement should be directly above original voxel for " << quadrant.name;
        
        // Validate placement is valid (position not occupied)
        bool isValidPlacement = faceDetector.isValidFaceForPlacement(detectedFace, *grid);
        EXPECT_TRUE(isValidPlacement) << "Placement position should be valid (empty) for " << quadrant.name;
        
        // Verify no voxel exists at placement position
        bool isOccupied = grid->getVoxel(placementPos);
        EXPECT_FALSE(isOccupied) << "Placement position should be empty for " << quadrant.name;
        
        if (detectedFace.isValid() && !detectedFace.isGroundPlane() && 
            detectedFace.getVoxelPosition() == voxelPos && 
            detectedFace.getDirection() == FaceDirection::PositiveY &&
            placementPos == expectedPlacement && isValidPlacement && !isOccupied) {
            successCount++;
            std::cout << "✓ SUCCESS: " << quadrant.name << " quadrant test passed" << std::endl;
        } else {
            std::cout << "✗ FAILED: " << quadrant.name << " quadrant test failed" << std::endl;
        }
    }
    
    std::cout << "\n=== RESULTS ===" << std::endl;
    std::cout << "Successful quadrants: " << successCount << " / " << quadrants.size() << std::endl;
    
    // All quadrants should pass
    EXPECT_EQ(successCount, 4) << "All 4 quadrants of the top face should work correctly";
    
    if (successCount == 4) {
        std::cout << "✓ ALL TESTS PASSED: Top face clicking works correctly in all quadrants" << std::endl;
    } else {
        std::cout << "✗ SOME TESTS FAILED: Top face clicking has issues in " << (4 - successCount) << " quadrant(s)" << std::endl;
    }
}

TEST_F(TopFacePlacementTest, TopFaceCenterClicking) {
    std::cout << "\n=== TESTING TOP FACE CENTER CLICKING ===" << std::endl;
    
    FaceDetector faceDetector;
    
    // Test clicking exactly at the center of the top face
    Vector3f rayOrigin(topFaceCenter.x, topFaceCenter.y + 1.0f, topFaceCenter.z);
    Vector3f rayDir(0.0f, -1.0f, 0.0f);  // Straight down
    VisualFeedback::Ray ray = {rayOrigin, rayDir};
    
    std::cout << "Testing center click - Ray origin: (" << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << ")" << std::endl;
    
    // Detect face
    Face detectedFace = faceDetector.detectFaceAcrossAllResolutions(ray, voxelManager);
    
    // Validate face detection
    ASSERT_TRUE(detectedFace.isValid()) << "Failed to detect face at center";
    ASSERT_FALSE(detectedFace.isGroundPlane()) << "Should detect voxel face, not ground plane";
    EXPECT_EQ(detectedFace.getVoxelPosition(), voxelPos) << "Should detect the correct voxel";
    EXPECT_EQ(detectedFace.getDirection(), FaceDirection::PositiveY) << "Should detect +Y (top) face";
    
    // Calculate and validate placement
    IncrementCoordinates placementPos = faceDetector.calculatePlacementPosition(detectedFace);
    IncrementCoordinates expectedPlacement(voxelPos.x(), voxelPos.y() + 32, voxelPos.z());
    EXPECT_EQ(placementPos, expectedPlacement) << "Center click should place voxel directly above";
    
    std::cout << "✓ Center clicking test passed" << std::endl;
}

TEST_F(TopFacePlacementTest, TopFaceEdgeClicking) {
    std::cout << "\n=== TESTING TOP FACE EDGE CLICKING ===" << std::endl;
    
    FaceDetector faceDetector;
    
    // Test clicking very close to edges (but still within the face)
    struct EdgeTest {
        std::string name;
        Vector3f offset;
    };
    
    float edgeOffset = voxelSize * 0.48f;  // Very close to edge (96% from center)
    
    std::vector<EdgeTest> edges = {
        {"LeftEdge", Vector3f(-edgeOffset, 0.0f, 0.0f)},
        {"RightEdge", Vector3f(edgeOffset, 0.0f, 0.0f)},
        {"FrontEdge", Vector3f(0.0f, 0.0f, -edgeOffset)},
        {"BackEdge", Vector3f(0.0f, 0.0f, edgeOffset)}
    };
    
    int edgeSuccessCount = 0;
    
    for (const auto& edge : edges) {
        Vector3f clickPoint = topFaceCenter + edge.offset;
        Vector3f rayOrigin(clickPoint.x, topFaceCenter.y + 1.0f, clickPoint.z);
        Vector3f rayDir(0.0f, -1.0f, 0.0f);  // Straight down
        VisualFeedback::Ray ray = {rayOrigin, rayDir};
        
        std::cout << "\n--- Testing " << edge.name << " ---" << std::endl;
        std::cout << "Click point: (" << clickPoint.x << ", " << clickPoint.y << ", " << clickPoint.z << ")" << std::endl;
        
        Face detectedFace = faceDetector.detectFaceAcrossAllResolutions(ray, voxelManager);
        
        if (detectedFace.isValid() && !detectedFace.isGroundPlane() && 
            detectedFace.getVoxelPosition() == voxelPos && 
            detectedFace.getDirection() == FaceDirection::PositiveY) {
            edgeSuccessCount++;
            std::cout << "✓ SUCCESS: " << edge.name << " edge test passed" << std::endl;
        } else {
            std::cout << "✗ FAILED: " << edge.name << " edge test failed" << std::endl;
        }
    }
    
    std::cout << "\nEdge clicking results: " << edgeSuccessCount << " / " << edges.size() << " passed" << std::endl;
    EXPECT_EQ(edgeSuccessCount, 4) << "All edge clicks should work correctly";
}