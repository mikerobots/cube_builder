#include <gtest/gtest.h>
#include "../include/visual_feedback/FaceDetector.h"
#include "../../voxel_data/VoxelGrid.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class FaceDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        workspaceSize = Vector3f(10.0f, 10.0f, 10.0f);
        resolution = VoxelResolution::Size_32cm;
        testGrid = std::make_unique<VoxelGrid>(resolution, workspaceSize);
        detector = std::make_unique<FaceDetector>();
        
        // Add some test voxels
        testGrid->setVoxel(Vector3i(5, 5, 5), true);
        testGrid->setVoxel(Vector3i(6, 5, 5), true);
        testGrid->setVoxel(Vector3i(5, 6, 5), true);
    }
    
    Vector3f workspaceSize;
    VoxelResolution resolution;
    std::unique_ptr<VoxelGrid> testGrid;
    std::unique_ptr<FaceDetector> detector;
};

TEST_F(FaceDetectorTest, RayMiss) {
    // Ray that doesn't hit any voxels
    VoxelEditor::VisualFeedback::Ray ray(Vector3f(0, 0, 0), Vector3f(0, 0, 1));
    
    Face face = detector->detectFace(ray, *testGrid, resolution);
    
    EXPECT_FALSE(face.isValid());
}

TEST_F(FaceDetectorTest, RayHit) {
    // Ray that hits the test voxel
    // Grid position (5,5,5) maps to world position (-3.4, 1.6, -3.4)
    Vector3f voxelWorldPos = testGrid->gridToWorld(Vector3i(5, 5, 5));
    float voxelSize = getVoxelSize(resolution);
    Vector3f voxelCenter = voxelWorldPos + Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    
    // Ray from in front of the voxel looking in +Z direction
    Vector3f rayOrigin = Vector3f(voxelCenter.x, voxelCenter.y, voxelCenter.z - 2.0f);
    VoxelEditor::VisualFeedback::Ray ray(rayOrigin, Vector3f(0, 0, 1));
    
    Face face = detector->detectFace(ray, *testGrid, resolution);
    
    EXPECT_TRUE(face.isValid());
    EXPECT_EQ(face.getVoxelPosition(), Vector3i(5, 5, 5));
    EXPECT_EQ(face.getDirection(), VoxelEditor::VisualFeedback::FaceDirection::NegativeZ);
}

// Enhancement Tests
TEST_F(FaceDetectorTest, GroundPlaneDetection) {
    // Ray pointing down at ground plane
    VoxelEditor::VisualFeedback::Ray ray(Vector3f(2.5f, 1.0f, 3.5f), Vector3f(0, -1, 0));
    
    Face face = detector->detectGroundPlane(ray);
    
    EXPECT_TRUE(face.isValid());
    EXPECT_TRUE(face.isGroundPlane());
    EXPECT_EQ(face.getDirection(), VoxelEditor::VisualFeedback::FaceDirection::PositiveY);
    EXPECT_FLOAT_EQ(face.getGroundPlaneHitPoint().y, 0.0f);
    EXPECT_FLOAT_EQ(face.getGroundPlaneHitPoint().x, 2.5f);
    EXPECT_FLOAT_EQ(face.getGroundPlaneHitPoint().z, 3.5f);
}

TEST_F(FaceDetectorTest, GroundPlaneNoHit_ParallelRay) {
    // Ray parallel to ground plane
    VoxelEditor::VisualFeedback::Ray ray(Vector3f(0, 1.0f, 0), Vector3f(1, 0, 0));
    
    Face face = detector->detectGroundPlane(ray);
    
    EXPECT_FALSE(face.isValid());
}

TEST_F(FaceDetectorTest, GroundPlaneNoHit_UpwardRay) {
    // Ray pointing upward
    VoxelEditor::VisualFeedback::Ray ray(Vector3f(0, 1.0f, 0), Vector3f(0, 1, 0));
    
    Face face = detector->detectGroundPlane(ray);
    
    EXPECT_FALSE(face.isValid());
}

TEST_F(FaceDetectorTest, DetectFaceOrGround_HitsVoxel) {
    // Ray that hits a voxel
    Vector3f voxelWorldPos = testGrid->gridToWorld(Vector3i(5, 5, 5));
    float voxelSize = getVoxelSize(resolution);
    Vector3f voxelCenter = voxelWorldPos + Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    
    // Ray from in front of the voxel looking in +Z direction
    Vector3f rayOrigin = Vector3f(voxelCenter.x, voxelCenter.y, voxelCenter.z - 2.0f);
    VoxelEditor::VisualFeedback::Ray ray(rayOrigin, Vector3f(0, 0, 1));
    
    Face face = detector->detectFaceOrGround(ray, *testGrid, resolution);
    
    EXPECT_TRUE(face.isValid());
    EXPECT_FALSE(face.isGroundPlane());
    EXPECT_EQ(face.getVoxelPosition(), Vector3i(5, 5, 5));
}

TEST_F(FaceDetectorTest, DetectFaceOrGround_HitsGround) {
    // Ray that misses voxels but hits ground
    VoxelEditor::VisualFeedback::Ray ray(Vector3f(0, 2.0f, 0), Vector3f(0, -1, 0));
    
    Face face = detector->detectFaceOrGround(ray, *testGrid, resolution);
    
    EXPECT_TRUE(face.isValid());
    EXPECT_TRUE(face.isGroundPlane());
    EXPECT_FLOAT_EQ(face.getGroundPlaneHitPoint().y, 0.0f);
}

TEST_F(FaceDetectorTest, CalculatePlacementPosition_GroundPlane) {
    // Create a ground plane face
    Face groundFace = Face::GroundPlane(Vector3f(1.234f, 0.0f, 2.567f));
    
    Vector3i placementPos = detector->calculatePlacementPosition(groundFace);
    
    // Should snap to nearest 1cm increment
    EXPECT_EQ(placementPos.x, 123); // 1.234m = 123.4cm, rounds to 123
    EXPECT_EQ(placementPos.y, 0);
    EXPECT_EQ(placementPos.z, 257); // 2.567m = 256.7cm, rounds to 257
}

TEST_F(FaceDetectorTest, FaceDirection_AllDirections) {
    // Test that we correctly identify face directions
    testGrid->setVoxel(Vector3i(10, 10, 10), true);
    
    // Test each face direction
    struct TestCase {
        Vector3f rayOrigin;
        Vector3f rayDir;
        VoxelEditor::VisualFeedback::FaceDirection expectedDir;
    };
    
    float voxelSize = getVoxelSize(resolution);
    Vector3f voxelWorldPos = testGrid->gridToWorld(Vector3i(10, 10, 10));
    Vector3f voxelCenter = voxelWorldPos + Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    
    TestCase testCases[] = {
        {voxelCenter + Vector3f(-2 * voxelSize, 0, 0), Vector3f(1, 0, 0), VoxelEditor::VisualFeedback::FaceDirection::NegativeX},
        {voxelCenter + Vector3f(2 * voxelSize, 0, 0), Vector3f(-1, 0, 0), VoxelEditor::VisualFeedback::FaceDirection::PositiveX},
        {voxelCenter + Vector3f(0, -2 * voxelSize, 0), Vector3f(0, 1, 0), VoxelEditor::VisualFeedback::FaceDirection::NegativeY},
        {voxelCenter + Vector3f(0, 2 * voxelSize, 0), Vector3f(0, -1, 0), VoxelEditor::VisualFeedback::FaceDirection::PositiveY},
        {voxelCenter + Vector3f(0, 0, -2 * voxelSize), Vector3f(0, 0, 1), VoxelEditor::VisualFeedback::FaceDirection::NegativeZ},
        {voxelCenter + Vector3f(0, 0, 2 * voxelSize), Vector3f(0, 0, -1), VoxelEditor::VisualFeedback::FaceDirection::PositiveZ}
    };
    
    for (const auto& test : testCases) {
        VoxelEditor::VisualFeedback::Ray ray(test.rayOrigin, test.rayDir);
        Face face = detector->detectFace(ray, *testGrid, resolution);
        
        EXPECT_TRUE(face.isValid());
        EXPECT_EQ(face.getDirection(), test.expectedDir);
    }
}

TEST_F(FaceDetectorTest, ValidFaceForPlacement) {
    Face face(Vector3i(5, 5, 5), resolution, VoxelEditor::VisualFeedback::FaceDirection::PositiveZ);
    
    bool isValid = detector->isValidFaceForPlacement(face, *testGrid);
    
    EXPECT_TRUE(isValid); // Should be valid since adjacent voxel is empty
}

TEST_F(FaceDetectorTest, InvalidFaceForPlacement) {
    Face face(Vector3i(5, 5, 5), resolution, VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
    
    bool isValid = detector->isValidFaceForPlacement(face, *testGrid);
    
    EXPECT_FALSE(isValid); // Should be invalid since adjacent voxel (6,5,5) is occupied
}

TEST_F(FaceDetectorTest, PlacementPosition) {
    Face face(Vector3i(5, 5, 5), resolution, VoxelEditor::VisualFeedback::FaceDirection::PositiveZ);
    
    Vector3i placementPos = detector->calculatePlacementPosition(face);
    
    EXPECT_EQ(placementPos, Vector3i(5, 5, 6));
}

TEST_F(FaceDetectorTest, FacesInRegion) {
    BoundingBox region(Vector3f(4.5f * getVoxelSize(resolution), 4.5f * getVoxelSize(resolution), 4.5f * getVoxelSize(resolution)),
                      Vector3f(6.5f * getVoxelSize(resolution), 6.5f * getVoxelSize(resolution), 5.5f * getVoxelSize(resolution)));
    
    std::vector<Face> faces = detector->detectFacesInRegion(region, *testGrid, resolution);
    
    EXPECT_GT(faces.size(), 0);
    
    // Check that all returned faces are valid for placement
    for (const auto& face : faces) {
        EXPECT_TRUE(detector->isValidFaceForPlacement(face, *testGrid));
    }
}

TEST_F(FaceDetectorTest, MaxRayDistance) {
    detector->setMaxRayDistance(1.0f);
    EXPECT_FLOAT_EQ(detector->getMaxRayDistance(), 1.0f);
    
    // Ray that would hit but is too far
    float voxelSize = getVoxelSize(resolution);
    Vector3f rayOrigin = Vector3f(5 * voxelSize, 5 * voxelSize, -2.0f);
    VoxelEditor::VisualFeedback::Ray ray(rayOrigin, Vector3f(0, 0, 1));
    
    Face face = detector->detectFace(ray, *testGrid, resolution);
    
    EXPECT_FALSE(face.isValid()); // Should miss due to distance limit
}

TEST_F(FaceDetectorTest, RayFromInside) {
    // Ray starting inside a voxel
    float voxelSize = getVoxelSize(resolution);
    Vector3f voxelWorldPos = testGrid->gridToWorld(Vector3i(5, 5, 5));
    Vector3f rayOrigin = voxelWorldPos + Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    VoxelEditor::VisualFeedback::Ray ray(rayOrigin, Vector3f(1, 0, 0));
    
    Face face = detector->detectFace(ray, *testGrid, resolution);
    
    // Should detect the exit face
    EXPECT_TRUE(face.isValid());
    EXPECT_EQ(face.getDirection(), VoxelEditor::VisualFeedback::FaceDirection::PositiveX);
}

TEST_F(FaceDetectorTest, EmptyGrid) {
    VoxelGrid emptyGrid(resolution, workspaceSize);
    
    VoxelEditor::VisualFeedback::Ray ray(Vector3f(0, 0, 0), Vector3f(1, 1, 1));
    Face face = detector->detectFace(ray, emptyGrid, resolution);
    
    EXPECT_FALSE(face.isValid());
    
    BoundingBox region(Vector3f(0, 0, 0), Vector3f(10, 10, 10));
    std::vector<Face> faces = detector->detectFacesInRegion(region, emptyGrid, resolution);
    
    EXPECT_EQ(faces.size(), 0);
}

TEST_F(FaceDetectorTest, GridBoundaryRay) {
    // Ray that starts outside grid bounds
    Vector3f rayOrigin = Vector3f(-1, -1, -1);
    VoxelEditor::VisualFeedback::Ray ray(rayOrigin, Vector3f(1, 1, 1).normalized());
    
    Face face = detector->detectFace(ray, *testGrid, resolution);
    
    // Should be able to hit voxels even starting from outside
    if (face.isValid()) {
        EXPECT_TRUE(testGrid->getVoxel(face.getVoxelPosition()));
    }
}

TEST_F(FaceDetectorTest, MultipleVoxelRay) {
    // Add more voxels in a line
    testGrid->setVoxel(Vector3i(7, 5, 5), true);
    testGrid->setVoxel(Vector3i(8, 5, 5), true);
    
    // Ray that passes through multiple voxels
    float voxelSize = getVoxelSize(resolution);
    Vector3f voxelWorldPos = testGrid->gridToWorld(Vector3i(5, 5, 5));
    Vector3f voxelCenter = voxelWorldPos + Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    
    // Ray from left of the voxels looking right
    Vector3f rayOrigin = Vector3f(voxelCenter.x - 2.0f, voxelCenter.y, voxelCenter.z);
    VoxelEditor::VisualFeedback::Ray ray(rayOrigin, Vector3f(1, 0, 0));
    
    Face face = detector->detectFace(ray, *testGrid, resolution);
    
    EXPECT_TRUE(face.isValid());
    EXPECT_EQ(face.getVoxelPosition(), Vector3i(5, 5, 5)); // Should hit first voxel
    EXPECT_EQ(face.getDirection(), VoxelEditor::VisualFeedback::FaceDirection::NegativeX);
}