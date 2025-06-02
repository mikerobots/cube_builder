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
    float voxelSize = getVoxelSize(resolution);
    Vector3f rayOrigin = Vector3f(5 * voxelSize, 5 * voxelSize, 0);
    VoxelEditor::VisualFeedback::Ray ray(rayOrigin, Vector3f(0, 0, 1));
    
    Face face = detector->detectFace(ray, *testGrid, resolution);
    
    EXPECT_TRUE(face.isValid());
    EXPECT_EQ(face.getVoxelPosition(), Vector3i(5, 5, 5));
    EXPECT_EQ(face.getDirection(), FaceDirection::NegativeZ);
}

TEST_F(FaceDetectorTest, ValidFaceForPlacement) {
    Face face(Vector3i(5, 5, 5), resolution, FaceDirection::PositiveZ);
    
    bool isValid = detector->isValidFaceForPlacement(face, *testGrid);
    
    EXPECT_TRUE(isValid); // Should be valid since adjacent voxel is empty
}

TEST_F(FaceDetectorTest, InvalidFaceForPlacement) {
    Face face(Vector3i(5, 5, 5), resolution, FaceDirection::PositiveX);
    
    bool isValid = detector->isValidFaceForPlacement(face, *testGrid);
    
    EXPECT_FALSE(isValid); // Should be invalid since adjacent voxel (6,5,5) is occupied
}

TEST_F(FaceDetectorTest, PlacementPosition) {
    Face face(Vector3i(5, 5, 5), resolution, FaceDirection::PositiveZ);
    
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
    Vector3f rayOrigin = Vector3f(5 * voxelSize + voxelSize * 0.5f, 
                                 5 * voxelSize + voxelSize * 0.5f, 
                                 5 * voxelSize + voxelSize * 0.5f);
    VoxelEditor::VisualFeedback::Ray ray(rayOrigin, Vector3f(1, 0, 0));
    
    Face face = detector->detectFace(ray, *testGrid, resolution);
    
    // Should detect the exit face
    EXPECT_TRUE(face.isValid());
    EXPECT_EQ(face.getDirection(), FaceDirection::PositiveX);
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
    Vector3f rayOrigin = Vector3f(4 * voxelSize, 5 * voxelSize + voxelSize * 0.5f, 5 * voxelSize + voxelSize * 0.5f);
    VoxelEditor::VisualFeedback::Ray ray(rayOrigin, Vector3f(1, 0, 0));
    
    Face face = detector->detectFace(ray, *testGrid, resolution);
    
    EXPECT_TRUE(face.isValid());
    EXPECT_EQ(face.getVoxelPosition(), Vector3i(5, 5, 5)); // Should hit first voxel
    EXPECT_EQ(face.getDirection(), FaceDirection::NegativeX);
}