#include <gtest/gtest.h>
#include "../Quaternion.h"
#include "../Vector3f.h"
#include "../MathUtils.h"
#include <cmath>

namespace VoxelEditor {
namespace Math {

class QuaternionTest : public ::testing::Test {
protected:
    static constexpr float EPSILON = 1e-6f;
    
    bool quaternionsEqual(const Quaternion& a, const Quaternion& b, float epsilon = EPSILON) {
        return std::abs(a.x - b.x) < epsilon &&
               std::abs(a.y - b.y) < epsilon &&
               std::abs(a.z - b.z) < epsilon &&
               std::abs(a.w - b.w) < epsilon;
    }
    
    bool vectorsEqual(const Vector3f& a, const Vector3f& b, float epsilon = EPSILON) {
        return std::abs(a.x - b.x) < epsilon &&
               std::abs(a.y - b.y) < epsilon &&
               std::abs(a.z - b.z) < epsilon;
    }
};

// Test basic construction and identity
TEST_F(QuaternionTest, ConstructionAndIdentity) {
    // Default constructor creates identity quaternion
    Quaternion q1;
    EXPECT_FLOAT_EQ(q1.x, 0.0f);
    EXPECT_FLOAT_EQ(q1.y, 0.0f);
    EXPECT_FLOAT_EQ(q1.z, 0.0f);
    EXPECT_FLOAT_EQ(q1.w, 1.0f);
    
    // Component constructor
    Quaternion q2(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(q2.x, 1.0f);
    EXPECT_FLOAT_EQ(q2.y, 2.0f);
    EXPECT_FLOAT_EQ(q2.z, 3.0f);
    EXPECT_FLOAT_EQ(q2.w, 4.0f);
    
    // Identity quaternion static method
    Quaternion identity = Quaternion::identity();
    EXPECT_FLOAT_EQ(identity.x, 0.0f);
    EXPECT_FLOAT_EQ(identity.y, 0.0f);
    EXPECT_FLOAT_EQ(identity.z, 0.0f);
    EXPECT_FLOAT_EQ(identity.w, 1.0f);
}

// Test axis-angle construction
TEST_F(QuaternionTest, AxisAngleConstruction) {
    // 90 degree rotation around Y axis
    Vector3f axis(0.0f, 1.0f, 0.0f);
    float angle = PI / 2.0f; // 90 degrees
    
    // Constructor version
    Quaternion q(axis, angle);
    
    // Expected values for 90 degree rotation around Y
    float expectedW = std::cos(angle / 2.0f);
    float expectedY = std::sin(angle / 2.0f);
    
    EXPECT_NEAR(q.x, 0.0f, EPSILON);
    EXPECT_NEAR(q.y, expectedY, EPSILON);
    EXPECT_NEAR(q.z, 0.0f, EPSILON);
    EXPECT_NEAR(q.w, expectedW, EPSILON);
    
    // Static method version
    Quaternion q2 = Quaternion::fromAxisAngle(axis, angle);
    EXPECT_TRUE(quaternionsEqual(q, q2));
}

// Test quaternion multiplication
TEST_F(QuaternionTest, Multiplication) {
    // Two 90 degree rotations around Y axis should equal 180 degrees
    Quaternion q1 = Quaternion::fromAxisAngle(Vector3f::UnitY(), PI / 2.0f);
    Quaternion q2 = Quaternion::fromAxisAngle(Vector3f::UnitY(), PI / 2.0f);
    Quaternion result = q1 * q2;
    
    // Should be equivalent to 180 degree rotation around Y
    Quaternion expected = Quaternion::fromAxisAngle(Vector3f::UnitY(), PI);
    EXPECT_TRUE(quaternionsEqual(result, expected, 1e-5f));
}

// Test quaternion normalization
TEST_F(QuaternionTest, Normalization) {
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
    
    // Test normalized() method (returns new quaternion)
    Quaternion normalized = q.normalized();
    EXPECT_NEAR(normalized.length(), 1.0f, EPSILON);
    
    // Test normalize() method (modifies in place)
    q.normalize();
    EXPECT_NEAR(q.length(), 1.0f, EPSILON);
}

// Test quaternion conjugate
TEST_F(QuaternionTest, Conjugate) {
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion conj = q.conjugate();
    
    EXPECT_FLOAT_EQ(conj.x, -1.0f);
    EXPECT_FLOAT_EQ(conj.y, -2.0f);
    EXPECT_FLOAT_EQ(conj.z, -3.0f);
    EXPECT_FLOAT_EQ(conj.w, 4.0f);
}

// Test quaternion inverse
TEST_F(QuaternionTest, Inverse) {
    Quaternion q = Quaternion::fromAxisAngle(Vector3f::UnitY(), PI / 4.0f);
    Quaternion inv = q.inverse();
    
    // q * q^-1 should equal identity
    Quaternion result = q * inv;
    Quaternion identity = Quaternion::identity();
    EXPECT_TRUE(quaternionsEqual(result, identity));
}

// Test vector rotation
TEST_F(QuaternionTest, VectorRotation) {
    // 90 degree rotation around Y axis
    Quaternion q = Quaternion::fromAxisAngle(Vector3f::UnitY(), PI / 2.0f);
    
    // Rotate unit X vector 90 degrees around Y
    // Should transform X to -Z
    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f rotated = q.rotate(v);
    
    EXPECT_NEAR(rotated.x, 0.0f, EPSILON);
    EXPECT_NEAR(rotated.y, 0.0f, EPSILON);
    EXPECT_NEAR(rotated.z, -1.0f, EPSILON);
}

// Test Euler angles basic functionality
TEST_F(QuaternionTest, EulerAnglesBasic) {
    // Create a simple rotation
    Quaternion q = Quaternion::fromAxisAngle(Vector3f::UnitY(), PI / 4.0f);
    
    // Test that quaternion has unit length
    EXPECT_NEAR(q.length(), 1.0f, EPSILON);
    
    // Test that we can extract Euler angles
    Vector3f euler = q.getEulerAngles();
    
    // Verify the angles are in valid range
    EXPECT_GE(euler.x, -PI);
    EXPECT_LE(euler.x, PI);
    EXPECT_GE(euler.y, -PI);
    EXPECT_LE(euler.y, PI);
    EXPECT_GE(euler.z, -PI);
    EXPECT_LE(euler.z, PI);
}

// Test SLERP (Spherical Linear Interpolation)
TEST_F(QuaternionTest, Slerp) {
    Quaternion q1 = Quaternion::identity();
    Quaternion q2 = Quaternion::fromAxisAngle(Vector3f::UnitY(), PI / 2.0f);
    
    // Test endpoints
    Quaternion result0 = Quaternion::slerp(q1, q2, 0.0f);
    EXPECT_TRUE(quaternionsEqual(result0, q1));
    
    Quaternion result1 = Quaternion::slerp(q1, q2, 1.0f);
    EXPECT_TRUE(quaternionsEqual(result1, q2));
    
    // Test midpoint
    Quaternion result05 = Quaternion::slerp(q1, q2, 0.5f);
    Quaternion expected = Quaternion::fromAxisAngle(Vector3f::UnitY(), PI / 4.0f);
    EXPECT_TRUE(quaternionsEqual(result05, expected, 1e-5f));
}

// Test lookRotation basic functionality
TEST_F(QuaternionTest, LookRotationBasic) {
    // Create a quaternion that looks in X direction with Y up
    Vector3f forward(1.0f, 0.0f, 0.0f);
    Vector3f up(0.0f, 1.0f, 0.0f);
    
    Quaternion q = Quaternion::lookRotation(forward, up);
    
    // Verify the quaternion is normalized
    EXPECT_NEAR(q.length(), 1.0f, EPSILON);
    
    // The quaternion should create a valid rotation
    Vector3f testVec(1.0f, 1.0f, 1.0f);
    Vector3f rotated = q.rotate(testVec);
    
    // Check that rotation preserves vector length
    EXPECT_NEAR(testVec.length(), rotated.length(), EPSILON);
}

// Test quaternion at potential gimbal lock angle
TEST_F(QuaternionTest, QuaternionAtGimbalLock) {
    // Create quaternion from Euler angles at 90 degree pitch
    Quaternion q = Quaternion::fromEulerAngles(PI / 2.0f, 0.0f, 0.0f);
    
    // Quaternion should still be valid
    EXPECT_NEAR(q.length(), 1.0f, EPSILON);
    
    // Should be able to rotate vectors
    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f rotated = q.rotate(v);
    
    // Rotation should preserve length
    EXPECT_NEAR(v.length(), rotated.length(), EPSILON);
}

// Test quaternion addition and subtraction
TEST_F(QuaternionTest, AdditionSubtraction) {
    Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion q2(5.0f, 6.0f, 7.0f, 8.0f);
    
    Quaternion sum = q1 + q2;
    EXPECT_FLOAT_EQ(sum.x, 6.0f);
    EXPECT_FLOAT_EQ(sum.y, 8.0f);
    EXPECT_FLOAT_EQ(sum.z, 10.0f);
    EXPECT_FLOAT_EQ(sum.w, 12.0f);
    
    Quaternion diff = q2 - q1;
    EXPECT_FLOAT_EQ(diff.x, 4.0f);
    EXPECT_FLOAT_EQ(diff.y, 4.0f);
    EXPECT_FLOAT_EQ(diff.z, 4.0f);
    EXPECT_FLOAT_EQ(diff.w, 4.0f);
}

// Test scalar multiplication
TEST_F(QuaternionTest, ScalarMultiplication) {
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
    float scalar = 2.0f;
    
    Quaternion result = q * scalar;
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
    EXPECT_FLOAT_EQ(result.z, 6.0f);
    EXPECT_FLOAT_EQ(result.w, 8.0f);
}

// Test dot product
TEST_F(QuaternionTest, DotProduct) {
    Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion q2(5.0f, 6.0f, 7.0f, 8.0f);
    
    float dot = q1.dot(q2);
    float expected = 1*5 + 2*6 + 3*7 + 4*8; // 5 + 12 + 21 + 32 = 70
    EXPECT_FLOAT_EQ(dot, expected);
}

// Test equality operators
TEST_F(QuaternionTest, EqualityOperators) {
    Quaternion q1(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion q2(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion q3(1.0f, 2.0f, 3.0f, 4.1f);
    
    EXPECT_TRUE(q1 == q2);
    EXPECT_FALSE(q1 == q3);
    EXPECT_FALSE(q1 != q2);
    EXPECT_TRUE(q1 != q3);
}

// Test compound rotation order
TEST_F(QuaternionTest, CompoundRotationOrder) {
    // Rotate 45 degrees around Y, then 45 degrees around X
    Quaternion yRot = Quaternion::fromAxisAngle(Vector3f::UnitY(), PI / 4.0f);
    Quaternion xRot = Quaternion::fromAxisAngle(Vector3f::UnitX(), PI / 4.0f);
    
    // Combined rotation (order matters!)
    Quaternion combined = xRot * yRot;
    
    // Test by rotating a vector
    Vector3f v(0.0f, 0.0f, 1.0f);
    Vector3f rotated = combined.rotate(v);
    
    // Apply rotations separately
    Vector3f step1 = yRot.rotate(v);
    Vector3f step2 = xRot.rotate(step1);
    
    EXPECT_TRUE(vectorsEqual(rotated, step2));
}

// Test lookRotation with various directions
TEST_F(QuaternionTest, LookRotationDirections) {
    // Test several different look directions
    struct TestCase {
        Vector3f forward;
        Vector3f up;
    };
    
    TestCase cases[] = {
        {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // Look +X
        {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // Look +Y
        {{0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},  // Look +Z
        {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // Look diagonal
    };
    
    for (const auto& testCase : cases) {
        Quaternion q = Quaternion::lookRotation(testCase.forward, testCase.up);
        
        // Should produce normalized quaternion
        EXPECT_NEAR(q.length(), 1.0f, EPSILON);
        
        // Should produce valid rotation
        Vector3f v(1.0f, 0.0f, 0.0f);
        Vector3f rotated = q.rotate(v);
        EXPECT_NEAR(v.length(), rotated.length(), EPSILON);
    }
}

// Test length and length squared
TEST_F(QuaternionTest, LengthOperations) {
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
    
    float lengthSq = q.lengthSquared();
    float expectedLengthSq = 1*1 + 2*2 + 3*3 + 4*4; // 1 + 4 + 9 + 16 = 30
    EXPECT_FLOAT_EQ(lengthSq, expectedLengthSq);
    
    float length = q.length();
    EXPECT_FLOAT_EQ(length, std::sqrt(expectedLengthSq));
}

// Test edge cases
TEST_F(QuaternionTest, EdgeCases) {
    // Zero quaternion normalization
    Quaternion zero(0.0f, 0.0f, 0.0f, 0.0f);
    Quaternion normalized = zero.normalized();
    // Should return identity when normalizing zero quaternion
    EXPECT_TRUE(quaternionsEqual(normalized, Quaternion::identity()));
    
    // Very small quaternion
    Quaternion tiny(1e-10f, 1e-10f, 1e-10f, 1e-10f);
    Quaternion tinyNorm = tiny.normalized();
    EXPECT_NEAR(tinyNorm.length(), 1.0f, EPSILON);
}

} // namespace Math
} // namespace VoxelEditor