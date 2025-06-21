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

TEST_F(Matrix4fTest, LookAt_DetailedValidation) {
    // Test 1: Looking down negative Z (standard view)
    {
        Vector3f eye(0.0f, 0.0f, 5.0f);
        Vector3f center(0.0f, 0.0f, 0.0f);
        Vector3f up(0.0f, 1.0f, 0.0f);
        
        Matrix4f view = Matrix4f::lookAt(eye, center, up);
        
        // Right vector should be (1, 0, 0)
        EXPECT_NEAR(view.m[0], 1.0f, 1e-6f);
        EXPECT_NEAR(view.m[1], 0.0f, 1e-6f);
        EXPECT_NEAR(view.m[2], 0.0f, 1e-6f);
        
        // Up vector should be (0, 1, 0)
        EXPECT_NEAR(view.m[4], 0.0f, 1e-6f);
        EXPECT_NEAR(view.m[5], 1.0f, 1e-6f);
        EXPECT_NEAR(view.m[6], 0.0f, 1e-6f);
        
        // Forward vector should be (0, 0, 1) - looking down -Z
        EXPECT_NEAR(view.m[8], 0.0f, 1e-6f);
        EXPECT_NEAR(view.m[9], 0.0f, 1e-6f);
        EXPECT_NEAR(view.m[10], 1.0f, 1e-6f);
        
        // Translation should be negative dot products
        EXPECT_NEAR(view.m[3], 0.0f, 1e-6f);    // -dot(right, eye)
        EXPECT_NEAR(view.m[7], 0.0f, 1e-6f);    // -dot(up, eye)
        EXPECT_NEAR(view.m[11], -5.0f, 1e-6f);  // -dot(forward, eye)
    }
    
    // Test 2: Looking from an angle
    {
        Vector3f eye(3.0f, 4.0f, 5.0f);
        Vector3f center(0.0f, 0.0f, 0.0f);
        Vector3f up(0.0f, 1.0f, 0.0f);
        
        Matrix4f view = Matrix4f::lookAt(eye, center, up);
        
        // Transform the eye position - should go to origin
        Vector3f transformedEye = view * eye;
        EXPECT_NEAR(transformedEye.x, 0.0f, 1e-5f);
        EXPECT_NEAR(transformedEye.y, 0.0f, 1e-5f);
        EXPECT_NEAR(transformedEye.z, 0.0f, 1e-5f);
        
        // Transform the center - should go to negative Z
        Vector3f transformedCenter = view * center;
        float expectedZ = -sqrtf(3*3 + 4*4 + 5*5); // Distance from eye to center
        EXPECT_NEAR(transformedCenter.x, 0.0f, 1e-5f);
        EXPECT_NEAR(transformedCenter.y, 0.0f, 1e-5f);
        EXPECT_NEAR(transformedCenter.z, expectedZ, 1e-5f);
    }
    
    // Test 3: Isometric-style view (45 degree rotation around Y, then tilt)
    {
        float dist = 10.0f;
        float angleY = M_PI / 4.0f; // 45 degrees
        float angleX = M_PI / 6.0f; // 30 degrees
        
        Vector3f eye(
            dist * sinf(angleY) * cosf(angleX),
            dist * sinf(angleX),
            dist * cosf(angleY) * cosf(angleX)
        );
        Vector3f center(0.0f, 0.0f, 0.0f);
        Vector3f up(0.0f, 1.0f, 0.0f);
        
        Matrix4f view = Matrix4f::lookAt(eye, center, up);
        
        // View matrix should be orthogonal (inverse = transpose for rotation part)
        Matrix4f viewInv = view.inverted();
        Matrix4f viewTranspose = view.transposed();
        
        // Compare rotation parts (ignore translation)
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                int idx = i * 4 + j;
                EXPECT_NEAR(viewInv.m[idx], viewTranspose.m[idx], 1e-5f);
            }
        }
    }
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

TEST_F(Matrix4fTest, MatrixMultiplicationOrder_MVP) {
    // Test that MVP multiplication order is correct for OpenGL
    // In OpenGL: MVP = Projection * View * Model
    // This means we transform vertices as: v' = MVP * v
    
    // Create a simple model matrix (translate by 2 on X)
    Matrix4f model = Matrix4f::translation(Vector3f(2.0f, 0.0f, 0.0f));
    
    // Create a simple view matrix (translate camera back by 5)
    Vector3f eye(0.0f, 0.0f, 5.0f);
    Vector3f center(0.0f, 0.0f, 0.0f);
    Vector3f up(0.0f, 1.0f, 0.0f);
    Matrix4f view = Matrix4f::lookAt(eye, center, up);
    
    // Create a simple projection matrix
    Matrix4f projection = Matrix4f::perspective(M_PI/4.0f, 1.0f, 0.1f, 100.0f);
    
    // Test correct order: P * V * M
    Matrix4f mvp_correct = projection * view * model;
    
    // Test incorrect order: M * V * P
    Matrix4f mvp_wrong = model * view * projection;
    
    // Transform a point at origin
    Vector3f point(0.0f, 0.0f, 0.0f);
    Vector3f result_correct = mvp_correct * point;
    Vector3f result_wrong = mvp_wrong * point;
    
    // Results should be different
    EXPECT_NE(result_correct, result_wrong);
    
    // With correct order, the point should be translated by model matrix first
    // then transformed by view and projection
    // The model translates (0,0,0) to (2,0,0)
    // The view looks from (0,0,5) to origin, so (2,0,0) is still visible
    // This is the expected behavior in OpenGL
}

TEST_F(Matrix4fTest, MatrixMultiplicationOrder_Associativity) {
    // Test that matrix multiplication is associative but not commutative
    Matrix4f a = Matrix4f::translation(Vector3f(1.0f, 0.0f, 0.0f));
    Matrix4f b = Matrix4f::rotationY(M_PI / 4.0f);
    Matrix4f c = Matrix4f::scale(2.0f);
    
    // Test associativity: (A * B) * C == A * (B * C)
    Matrix4f left_assoc = (a * b) * c;
    Matrix4f right_assoc = a * (b * c);
    expectMatrixEqual(left_assoc, right_assoc);
    
    // Test non-commutativity: A * B != B * A
    Matrix4f ab = a * b;
    Matrix4f ba = b * a;
    
    // Transform a test point to verify they produce different results
    Vector3f testPoint(1.0f, 0.0f, 0.0f);
    Vector3f result_ab = ab * testPoint;
    Vector3f result_ba = ba * testPoint;
    
    // Results should be different, proving non-commutativity
    EXPECT_NE(result_ab, result_ba);
}

TEST_F(Matrix4fTest, MatrixStorageOrder) {
    // Verify matrix storage order
    // Our Matrix4f uses row-major order internally
    // The constructor takes parameters row by row
    
    Matrix4f mat(
        1, 2, 3, 4,      // First row 
        5, 6, 7, 8,      // Second row
        9, 10, 11, 12,   // Third row
        13, 14, 15, 16   // Fourth row
    );
    
    // In row-major storage:
    // m[0-3] is the first row (1, 2, 3, 4)
    EXPECT_FLOAT_EQ(mat.m[0], 1.0f);
    EXPECT_FLOAT_EQ(mat.m[1], 2.0f);
    EXPECT_FLOAT_EQ(mat.m[2], 3.0f);
    EXPECT_FLOAT_EQ(mat.m[3], 4.0f);
    
    // m[4-7] is the second row (5, 6, 7, 8)
    EXPECT_FLOAT_EQ(mat.m[4], 5.0f);
    EXPECT_FLOAT_EQ(mat.m[5], 6.0f);
    EXPECT_FLOAT_EQ(mat.m[6], 7.0f);
    EXPECT_FLOAT_EQ(mat.m[7], 8.0f);
    
    // Translation components in row-major are in m[3], m[7], m[11]
    Matrix4f translation = Matrix4f::translation(Vector3f(10.0f, 20.0f, 30.0f));
    EXPECT_FLOAT_EQ(translation.m[3], 10.0f);   // X translation
    EXPECT_FLOAT_EQ(translation.m[7], 20.0f);   // Y translation
    EXPECT_FLOAT_EQ(translation.m[11], 30.0f);  // Z translation
    
    // When passing to OpenGL, we need to transpose or use glUniformMatrix4fv with transpose=GL_TRUE
}

TEST_F(Matrix4fTest, PerspectiveProjection_KnownValues) {
    // Test perspective projection with specific known values
    // Using standard OpenGL parameters
    float fov = M_PI / 2.0f; // 90 degrees
    float aspect = 1.0f; // Square aspect ratio
    float nearPlane = 1.0f;
    float farPlane = 100.0f;
    
    Matrix4f proj = Matrix4f::perspective(fov, aspect, nearPlane, farPlane);
    
    // For 90 degree FOV and aspect=1:
    // m[0] = 1/tan(45°) = 1
    // m[5] = 1/tan(45°) = 1
    float expectedM0 = 1.0f / tanf(fov / 2.0f);
    float expectedM5 = expectedM0;
    
    EXPECT_NEAR(proj.m[0], expectedM0, 1e-6f);
    EXPECT_NEAR(proj.m[5], expectedM5, 1e-6f);
    
    // Test the Z mapping
    // m[10] = -(far + near) / (far - near)
    // m[11] = -2 * far * near / (far - near)
    float expectedM10 = -(farPlane + nearPlane) / (farPlane - nearPlane);
    float expectedM11 = -2.0f * farPlane * nearPlane / (farPlane - nearPlane);
    
    EXPECT_NEAR(proj.m[10], expectedM10, 1e-6f);
    EXPECT_NEAR(proj.m[11], expectedM11, 1e-6f);
    
    // Standard perspective matrix values
    EXPECT_FLOAT_EQ(proj.m[14], -1.0f); // Perspective divide
    EXPECT_FLOAT_EQ(proj.m[15], 0.0f);
    
    // Test transformation of points
    // Point at near plane center should map to Z=-1
    Vector4f nearPoint(0.0f, 0.0f, -nearPlane, 1.0f);
    Vector4f nearResult = proj * nearPoint;
    float nearNDC = nearResult.z / nearResult.w;
    EXPECT_NEAR(nearNDC, -1.0f, 1e-5f);
    
    // Point at far plane center should map to Z=1
    Vector4f farPoint(0.0f, 0.0f, -farPlane, 1.0f);
    Vector4f farResult = proj * farPoint;
    float farNDC = farResult.z / farResult.w;
    EXPECT_NEAR(farNDC, 1.0f, 1e-5f);
}