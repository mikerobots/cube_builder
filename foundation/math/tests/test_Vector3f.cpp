#include <gtest/gtest.h>
#include "../Vector3f.h"

using namespace VoxelEditor::Math;

class Vector3fTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Vector3fTest, DefaultConstructor) {
    Vector3f v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST_F(Vector3fTest, ParameterConstructor) {
    Vector3f v(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
}

TEST_F(Vector3fTest, SingleValueConstructor) {
    Vector3f v(5.0f);
    EXPECT_FLOAT_EQ(v.x, 5.0f);
    EXPECT_FLOAT_EQ(v.y, 5.0f);
    EXPECT_FLOAT_EQ(v.z, 5.0f);
}

TEST_F(Vector3fTest, Addition) {
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(4.0f, 5.0f, 6.0f);
    Vector3f result = a + b;
    
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 7.0f);
    EXPECT_FLOAT_EQ(result.z, 9.0f);
}

TEST_F(Vector3fTest, Subtraction) {
    Vector3f a(4.0f, 5.0f, 6.0f);
    Vector3f b(1.0f, 2.0f, 3.0f);
    Vector3f result = a - b;
    
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST_F(Vector3fTest, ScalarMultiplication) {
    Vector3f v(1.0f, 2.0f, 3.0f);
    Vector3f result = v * 2.0f;
    
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
    EXPECT_FLOAT_EQ(result.z, 6.0f);
}

TEST_F(Vector3fTest, ScalarDivision) {
    Vector3f v(2.0f, 4.0f, 6.0f);
    Vector3f result = v / 2.0f;
    
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST_F(Vector3fTest, Negation) {
    Vector3f v(1.0f, -2.0f, 3.0f);
    Vector3f result = -v;
    
    EXPECT_FLOAT_EQ(result.x, -1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, -3.0f);
}

TEST_F(Vector3fTest, DotProduct) {
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(4.0f, 5.0f, 6.0f);
    float result = a.dot(b);
    
    EXPECT_FLOAT_EQ(result, 32.0f); // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
}

TEST_F(Vector3fTest, CrossProduct) {
    Vector3f a(1.0f, 0.0f, 0.0f);
    Vector3f b(0.0f, 1.0f, 0.0f);
    Vector3f result = a.cross(b);
    
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 1.0f);
}

TEST_F(Vector3fTest, Length) {
    Vector3f v(3.0f, 4.0f, 0.0f);
    float length = v.length();
    
    EXPECT_FLOAT_EQ(length, 5.0f);
}

TEST_F(Vector3fTest, LengthSquared) {
    Vector3f v(3.0f, 4.0f, 0.0f);
    float lengthSq = v.lengthSquared();
    
    EXPECT_FLOAT_EQ(lengthSq, 25.0f);
}

TEST_F(Vector3fTest, Normalization) {
    Vector3f v(3.0f, 4.0f, 0.0f);
    Vector3f normalized = v.normalized();
    
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
    EXPECT_FLOAT_EQ(normalized.z, 0.0f);
    EXPECT_NEAR(normalized.length(), 1.0f, 1e-6f);
}

TEST_F(Vector3fTest, NormalizeInPlace) {
    Vector3f v(3.0f, 4.0f, 0.0f);
    v.normalize();
    
    EXPECT_FLOAT_EQ(v.x, 0.6f);
    EXPECT_FLOAT_EQ(v.y, 0.8f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
    EXPECT_NEAR(v.length(), 1.0f, 1e-6f);
}

TEST_F(Vector3fTest, ZeroVectorNormalization) {
    Vector3f v(0.0f, 0.0f, 0.0f);
    Vector3f normalized = v.normalized();
    
    EXPECT_FLOAT_EQ(normalized.x, 1.0f);
    EXPECT_FLOAT_EQ(normalized.y, 0.0f);
    EXPECT_FLOAT_EQ(normalized.z, 0.0f);
}

TEST_F(Vector3fTest, StaticConstants) {
    Vector3f zero = Vector3f::Zero();
    Vector3f one = Vector3f::One();
    Vector3f unitX = Vector3f::UnitX();
    Vector3f unitY = Vector3f::UnitY();
    Vector3f unitZ = Vector3f::UnitZ();
    
    EXPECT_EQ(zero, Vector3f(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(one, Vector3f(1.0f, 1.0f, 1.0f));
    EXPECT_EQ(unitX, Vector3f(1.0f, 0.0f, 0.0f));
    EXPECT_EQ(unitY, Vector3f(0.0f, 1.0f, 0.0f));
    EXPECT_EQ(unitZ, Vector3f(0.0f, 0.0f, 1.0f));
}

TEST_F(Vector3fTest, Distance) {
    Vector3f a(0.0f, 0.0f, 0.0f);
    Vector3f b(3.0f, 4.0f, 0.0f);
    float distance = Vector3f::distance(a, b);
    
    EXPECT_FLOAT_EQ(distance, 5.0f);
}

TEST_F(Vector3fTest, DistanceSquared) {
    Vector3f a(0.0f, 0.0f, 0.0f);
    Vector3f b(3.0f, 4.0f, 0.0f);
    float distanceSq = Vector3f::distanceSquared(a, b);
    
    EXPECT_FLOAT_EQ(distanceSq, 25.0f);
}

TEST_F(Vector3fTest, Lerp) {
    Vector3f a(0.0f, 0.0f, 0.0f);
    Vector3f b(10.0f, 20.0f, 30.0f);
    Vector3f result = Vector3f::lerp(a, b, 0.5f);
    
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
    EXPECT_FLOAT_EQ(result.z, 15.0f);
}

TEST_F(Vector3fTest, MinMax) {
    Vector3f a(1.0f, 5.0f, 3.0f);
    Vector3f b(4.0f, 2.0f, 6.0f);
    
    Vector3f min = Vector3f::min(a, b);
    Vector3f max = Vector3f::max(a, b);
    
    EXPECT_EQ(min, Vector3f(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(max, Vector3f(4.0f, 5.0f, 6.0f));
}

TEST_F(Vector3fTest, Clamp) {
    Vector3f value(5.0f, -2.0f, 10.0f);
    Vector3f min(0.0f, 0.0f, 0.0f);
    Vector3f max(8.0f, 8.0f, 8.0f);
    
    Vector3f result = Vector3f::clamp(value, min, max);
    
    EXPECT_EQ(result, Vector3f(5.0f, 0.0f, 8.0f));
}

TEST_F(Vector3fTest, Equality) {
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(1.0f, 2.0f, 3.0f);
    Vector3f c(1.0f, 2.0f, 3.1f);
    
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

TEST_F(Vector3fTest, IndexOperator) {
    Vector3f v(1.0f, 2.0f, 3.0f);
    
    EXPECT_FLOAT_EQ(v[0], 1.0f);
    EXPECT_FLOAT_EQ(v[1], 2.0f);
    EXPECT_FLOAT_EQ(v[2], 3.0f);
    
    v[0] = 10.0f;
    EXPECT_FLOAT_EQ(v.x, 10.0f);
}

TEST_F(Vector3fTest, CompoundAssignment) {
    Vector3f v(1.0f, 2.0f, 3.0f);
    
    v += Vector3f(1.0f, 1.0f, 1.0f);
    EXPECT_EQ(v, Vector3f(2.0f, 3.0f, 4.0f));
    
    v -= Vector3f(1.0f, 1.0f, 1.0f);
    EXPECT_EQ(v, Vector3f(1.0f, 2.0f, 3.0f));
    
    v *= 2.0f;
    EXPECT_EQ(v, Vector3f(2.0f, 4.0f, 6.0f));
    
    v /= 2.0f;
    EXPECT_EQ(v, Vector3f(1.0f, 2.0f, 3.0f));
}