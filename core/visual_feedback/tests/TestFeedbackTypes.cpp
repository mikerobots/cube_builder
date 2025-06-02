#include <gtest/gtest.h>
#include "../include/visual_feedback/FeedbackTypes.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

class FeedbackTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test setup
    }
};

TEST_F(FeedbackTypesTest, FaceConstruction) {
    Vector3i voxelPos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    FaceDirection direction = FaceDirection::PositiveX;
    
    Face face(voxelPos, resolution, direction);
    
    EXPECT_TRUE(face.isValid());
    EXPECT_EQ(face.getVoxelPosition(), voxelPos);
    EXPECT_EQ(face.getResolution(), resolution);
    EXPECT_EQ(face.getDirection(), direction);
}

TEST_F(FeedbackTypesTest, FaceId) {
    Face face1(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    Face face2(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    Face face3(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, FaceDirection::PositiveY);
    
    EXPECT_EQ(face1.getId(), face2.getId());
    EXPECT_NE(face1.getId(), face3.getId());
}

TEST_F(FeedbackTypesTest, FaceWorldPosition) {
    Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    
    Vector3f worldPos = face.getWorldPosition();
    float voxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    
    EXPECT_FLOAT_EQ(worldPos.x, voxelSize);
    EXPECT_FLOAT_EQ(worldPos.y, voxelSize * 0.5f);
    EXPECT_FLOAT_EQ(worldPos.z, voxelSize * 0.5f);
}

TEST_F(FeedbackTypesTest, FaceNormal) {
    Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    
    Vector3f normal = face.getNormal();
    
    EXPECT_FLOAT_EQ(normal.x, 1.0f);
    EXPECT_FLOAT_EQ(normal.y, 0.0f);
    EXPECT_FLOAT_EQ(normal.z, 0.0f);
}

TEST_F(FeedbackTypesTest, FaceCorners) {
    Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    
    auto corners = face.getCorners();
    
    EXPECT_EQ(corners.size(), 4);
    
    float voxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    
    // All corners should have same X coordinate (face on positive X)
    for (const auto& corner : corners) {
        EXPECT_FLOAT_EQ(corner.x, voxelSize);
    }
}

TEST_F(FeedbackTypesTest, FaceArea) {
    Face face(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    
    float area = face.getArea();
    float voxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    float expectedArea = voxelSize * voxelSize;
    
    EXPECT_FLOAT_EQ(area, expectedArea);
}

TEST_F(FeedbackTypesTest, FaceEquality) {
    Face face1(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    Face face2(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, FaceDirection::PositiveX);
    Face face3(Vector3i(1, 2, 3), VoxelResolution::Size_32cm, FaceDirection::PositiveY);
    
    EXPECT_EQ(face1, face2);
    EXPECT_NE(face1, face3);
}

TEST_F(FeedbackTypesTest, RayConstruction) {
    Vector3f origin(1, 2, 3);
    Vector3f direction(0, 1, 0);
    
    VoxelEditor::VisualFeedback::Ray ray(origin, direction);
    
    EXPECT_EQ(ray.origin, origin);
    EXPECT_FLOAT_EQ(ray.direction.length(), 1.0f); // Should be normalized
}

TEST_F(FeedbackTypesTest, RayPointAt) {
    VoxelEditor::VisualFeedback::Ray ray(Vector3f(0, 0, 0), Vector3f(1, 0, 0));
    
    Vector3f point = ray.pointAt(5.0f);
    
    EXPECT_FLOAT_EQ(point.x, 5.0f);
    EXPECT_FLOAT_EQ(point.y, 0.0f);
    EXPECT_FLOAT_EQ(point.z, 0.0f);
}

TEST_F(FeedbackTypesTest, TransformMatrix) {
    Transform transform;
    transform.position = Vector3f(1, 2, 3);
    transform.scale = Vector3f(2, 2, 2);
    
    Matrix4f matrix = transform.toMatrix();
    
    // Test translation components
    EXPECT_FLOAT_EQ(matrix.m[12], 1.0f);
    EXPECT_FLOAT_EQ(matrix.m[13], 2.0f);
    EXPECT_FLOAT_EQ(matrix.m[14], 3.0f);
}

TEST_F(FeedbackTypesTest, HighlightStyleFactories) {
    auto faceStyle = HighlightStyle::Face();
    auto selectionStyle = HighlightStyle::Selection();
    auto groupStyle = HighlightStyle::Group();
    auto previewStyle = HighlightStyle::Preview();
    
    EXPECT_TRUE(faceStyle.animated);
    EXPECT_TRUE(selectionStyle.animated);
    EXPECT_FALSE(groupStyle.animated);
    EXPECT_FALSE(previewStyle.animated);
    
    EXPECT_FALSE(faceStyle.wireframe);
    EXPECT_TRUE(groupStyle.wireframe);
    EXPECT_TRUE(previewStyle.wireframe);
}

TEST_F(FeedbackTypesTest, OutlineStyleFactories) {
    auto voxelStyle = OutlineStyle::VoxelPreview();
    auto groupStyle = OutlineStyle::GroupBoundary();
    auto selectionStyle = OutlineStyle::SelectionBox();
    auto workspaceStyle = OutlineStyle::WorkspaceBounds();
    
    EXPECT_EQ(voxelStyle.pattern, LinePattern::Solid);
    EXPECT_EQ(groupStyle.pattern, LinePattern::Dashed);
    EXPECT_EQ(selectionStyle.pattern, LinePattern::Solid);
    EXPECT_EQ(workspaceStyle.pattern, LinePattern::Dotted);
}

TEST_F(FeedbackTypesTest, TextStyleFactories) {
    auto defaultStyle = TextStyle::Default();
    auto headerStyle = TextStyle::Header();
    auto debugStyle = TextStyle::Debug();
    auto warningStyle = TextStyle::Warning();
    auto errorStyle = TextStyle::Error();
    
    EXPECT_FALSE(defaultStyle.background);
    EXPECT_TRUE(headerStyle.background);
    EXPECT_TRUE(debugStyle.background);
    EXPECT_TRUE(warningStyle.background);
    EXPECT_TRUE(errorStyle.background);
    
    EXPECT_GT(headerStyle.size, defaultStyle.size);
    EXPECT_GT(errorStyle.size, warningStyle.size);
}

TEST_F(FeedbackTypesTest, FaceDirectionUtils) {
    auto normal = faceDirectionToNormal(FaceDirection::PositiveX);
    EXPECT_EQ(normal, Vector3f(1, 0, 0));
    
    auto opposite = oppositeDirection(FaceDirection::PositiveX);
    EXPECT_EQ(opposite, FaceDirection::NegativeX);
    
    opposite = oppositeDirection(FaceDirection::NegativeY);
    EXPECT_EQ(opposite, FaceDirection::PositiveY);
}