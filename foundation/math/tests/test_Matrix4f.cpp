#include <gtest/gtest.h>
#include "../Matrix4f.h"
#include <cmath>

using namespace VoxelEditor::Math;

class Matrix4fTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    
    void expectMatrixEqual(const Matrix4f& a, const Matrix4f& b, float tolerance = 1e-6f) {
        for (int i = 0; i < 16; ++i) {
            EXPECT_NEAR(a.m[i], b.m[i], tolerance) << "Matrices differ at index " << i;
        }
    }
};

TEST_F(Matrix4fTest, DefaultConstructor) {
    Matrix4f mat;
    
    EXPECT_FLOAT_EQ(mat.m[0], 1.0f);
    EXPECT_FLOAT_EQ(mat.m[5], 1.0f);
    EXPECT_FLOAT_EQ(mat.m[10], 1.0f);
    EXPECT_FLOAT_EQ(mat.m[15], 1.0f);
    
    for (int i = 0; i < 16; ++i) {
        if (i != 0 && i != 5 && i != 10 && i != 15) {
            EXPECT_FLOAT_EQ(mat.m[i], 0.0f);
        }
    }
}

TEST_F(Matrix4fTest, ArrayConstructor) {
    float data[16] = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    };
    
    Matrix4f mat(data);
    
    for (int i = 0; i < 16; ++i) {
        EXPECT_FLOAT_EQ(mat.m[i], data[i]);
    }
}

TEST_F(Matrix4fTest, ParameterConstructor) {
    Matrix4f mat(
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    );
    
    EXPECT_FLOAT_EQ(mat.m[0], 1.0f);
    EXPECT_FLOAT_EQ(mat.m[1], 2.0f);
    EXPECT_FLOAT_EQ(mat.m[15], 16.0f);
}

TEST_F(Matrix4fTest, Identity) {
    Matrix4f mat;
    mat.m[0] = 5.0f; // Modify to test identity reset
    mat.identity();
    
    Matrix4f expected = Matrix4f::Identity();
    expectMatrixEqual(mat, expected);
}

TEST_F(Matrix4fTest, MatrixMultiplication) {
    Matrix4f a(
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    );
    
    Matrix4f b = Matrix4f::Identity();
    Matrix4f result = a * b;
    
    expectMatrixEqual(result, a);
}

TEST_F(Matrix4fTest, VectorTransformation) {
    Matrix4f translation = Matrix4f::translation(Vector3f(1.0f, 2.0f, 3.0f));
    Vector3f point(0.0f, 0.0f, 0.0f);
    Vector3f result = translation * point;
    
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST_F(Matrix4fTest, DirectionTransformation) {
    Matrix4f rotation = Matrix4f::rotationZ(M_PI / 2.0f); // 90 degrees
    Vector3f direction(1.0f, 0.0f, 0.0f);
    Vector3f result = rotation.transformDirection(direction);
    
    EXPECT_NEAR(result.x, 0.0f, 1e-6f);
    EXPECT_NEAR(result.y, 1.0f, 1e-6f);
    EXPECT_NEAR(result.z, 0.0f, 1e-6f);
}

TEST_F(Matrix4fTest, Transpose) {
    Matrix4f mat(
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    );
    
    Matrix4f transposed = mat.transposed();
    
    EXPECT_FLOAT_EQ(transposed.m[0], 1.0f);
    EXPECT_FLOAT_EQ(transposed.m[1], 5.0f);
    EXPECT_FLOAT_EQ(transposed.m[2], 9.0f);
    EXPECT_FLOAT_EQ(transposed.m[3], 13.0f);
    EXPECT_FLOAT_EQ(transposed.m[4], 2.0f);
}

TEST_F(Matrix4fTest, Determinant) {
    Matrix4f identity = Matrix4f::Identity();
    float det = identity.determinant();
    
    EXPECT_FLOAT_EQ(det, 1.0f);
}

TEST_F(Matrix4fTest, Inversion) {
    Matrix4f translation = Matrix4f::translation(Vector3f(1.0f, 2.0f, 3.0f));
    Matrix4f inverted = translation.inverted();
    Matrix4f result = translation * inverted;
    
    Matrix4f identity = Matrix4f::Identity();
    expectMatrixEqual(result, identity, 1e-5f);
}

TEST_F(Matrix4fTest, TranslationMatrix) {
    Vector3f translation(1.0f, 2.0f, 3.0f);
    Matrix4f mat = Matrix4f::translation(translation);
    
    EXPECT_FLOAT_EQ(mat.m[3], 1.0f);
    EXPECT_FLOAT_EQ(mat.m[7], 2.0f);
    EXPECT_FLOAT_EQ(mat.m[11], 3.0f);
    
    Vector3f extractedTranslation = mat.getTranslation();
    EXPECT_EQ(extractedTranslation, translation);
}

TEST_F(Matrix4fTest, RotationX) {
    Matrix4f rotation = Matrix4f::rotationX(M_PI / 2.0f); // 90 degrees
    Vector3f point(0.0f, 1.0f, 0.0f);
    Vector3f result = rotation * point;
    
    EXPECT_NEAR(result.x, 0.0f, 1e-6f);
    EXPECT_NEAR(result.y, 0.0f, 1e-6f);
    EXPECT_NEAR(result.z, 1.0f, 1e-6f);
}

TEST_F(Matrix4fTest, RotationY) {
    Matrix4f rotation = Matrix4f::rotationY(M_PI / 2.0f); // 90 degrees
    Vector3f point(1.0f, 0.0f, 0.0f);
    Vector3f result = rotation * point;
    
    EXPECT_NEAR(result.x, 0.0f, 1e-6f);
    EXPECT_NEAR(result.y, 0.0f, 1e-6f);
    EXPECT_NEAR(result.z, -1.0f, 1e-6f);
}

TEST_F(Matrix4fTest, RotationZ) {
    Matrix4f rotation = Matrix4f::rotationZ(M_PI / 2.0f); // 90 degrees
    Vector3f point(1.0f, 0.0f, 0.0f);
    Vector3f result = rotation * point;
    
    EXPECT_NEAR(result.x, 0.0f, 1e-6f);
    EXPECT_NEAR(result.y, 1.0f, 1e-6f);
    EXPECT_NEAR(result.z, 0.0f, 1e-6f);
}

TEST_F(Matrix4fTest, ArbitraryAxisRotation) {
    Vector3f axis(0.0f, 0.0f, 1.0f);
    Matrix4f rotation = Matrix4f::rotation(axis, M_PI / 2.0f); // 90 degrees around Z
    Vector3f point(1.0f, 0.0f, 0.0f);
    Vector3f result = rotation * point;
    
    EXPECT_NEAR(result.x, 0.0f, 1e-6f);
    EXPECT_NEAR(result.y, 1.0f, 1e-6f);
    EXPECT_NEAR(result.z, 0.0f, 1e-6f);
}

TEST_F(Matrix4fTest, ScaleMatrix) {
    Vector3f scaleVec(2.0f, 3.0f, 4.0f);
    Matrix4f scale = Matrix4f::scale(scaleVec);
    Vector3f point(1.0f, 1.0f, 1.0f);
    Vector3f result = scale * point;
    
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
}

TEST_F(Matrix4fTest, UniformScale) {
    Matrix4f scale = Matrix4f::scale(2.0f);
    Vector3f point(1.0f, 1.0f, 1.0f);
    Vector3f result = scale * point;
    
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 2.0f);
}

TEST_F(Matrix4fTest, PerspectiveProjection) {
    float fov = M_PI / 4.0f; // 45 degrees
    float aspect = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    
    Matrix4f perspective = Matrix4f::perspective(fov, aspect, nearPlane, farPlane);
    
    EXPECT_GT(perspective.m[0], 0.0f);
    EXPECT_GT(perspective.m[5], 0.0f);
    EXPECT_LT(perspective.m[10], 0.0f);
    EXPECT_LT(perspective.m[11], 0.0f);
    EXPECT_FLOAT_EQ(perspective.m[14], -1.0f);
    EXPECT_FLOAT_EQ(perspective.m[15], 0.0f);
}

TEST_F(Matrix4fTest, OrthographicProjection) {
    float left = -1.0f;
    float right = 1.0f;
    float bottom = -1.0f;
    float top = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    
    Matrix4f ortho = Matrix4f::orthographic(left, right, bottom, top, nearPlane, farPlane);
    
    EXPECT_FLOAT_EQ(ortho.m[0], 1.0f);
    EXPECT_FLOAT_EQ(ortho.m[5], 1.0f);
    EXPECT_LT(ortho.m[10], 0.0f);
    EXPECT_FLOAT_EQ(ortho.m[15], 1.0f);
}

TEST_F(Matrix4fTest, LookAt) {
    Vector3f eye(0.0f, 0.0f, 1.0f);
    Vector3f center(0.0f, 0.0f, 0.0f);
    Vector3f up(0.0f, 1.0f, 0.0f);
    
    Matrix4f lookAt = Matrix4f::lookAt(eye, center, up);
    
    // Forward direction is from eye to center, which is (0,0,-1)
    // In a right-handed view matrix, this becomes the negative Z row
    EXPECT_FLOAT_EQ(lookAt.m[8], 0.0f);   // -forward.x
    EXPECT_FLOAT_EQ(lookAt.m[9], 0.0f);   // -forward.y  
    EXPECT_FLOAT_EQ(lookAt.m[10], 1.0f);  // -forward.z (negative of -1)
}

TEST_F(Matrix4fTest, Equality) {
    Matrix4f a = Matrix4f::Identity();
    Matrix4f b = Matrix4f::Identity();
    Matrix4f c = Matrix4f::translation(Vector3f(1.0f, 0.0f, 0.0f));
    
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

TEST_F(Matrix4fTest, IndexOperator) {
    Matrix4f mat = Matrix4f::Identity();
    
    EXPECT_FLOAT_EQ(mat[0], 1.0f);
    EXPECT_FLOAT_EQ(mat[5], 1.0f);
    EXPECT_FLOAT_EQ(mat[1], 0.0f);
    
    mat[1] = 5.0f;
    EXPECT_FLOAT_EQ(mat.m[1], 5.0f);
}

TEST_F(Matrix4fTest, SetTranslation) {
    Matrix4f mat = Matrix4f::Identity();
    Vector3f newTranslation(5.0f, 6.0f, 7.0f);
    
    mat.setTranslation(newTranslation);
    
    EXPECT_EQ(mat.getTranslation(), newTranslation);
}