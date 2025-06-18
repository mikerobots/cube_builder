#include <gtest/gtest.h>
#include "../CoordinateTypes.h"

using namespace VoxelEditor::Math;

class CoordinateTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==================== WorldCoordinates Tests ====================

TEST_F(CoordinateTypesTest, WorldCoordinates_DefaultConstructor) {
    WorldCoordinates coord;
    EXPECT_FLOAT_EQ(coord.x(), 0.0f);
    EXPECT_FLOAT_EQ(coord.y(), 0.0f);
    EXPECT_FLOAT_EQ(coord.z(), 0.0f);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_ParameterConstructor) {
    WorldCoordinates coord(1.5f, 2.5f, 3.5f);
    EXPECT_FLOAT_EQ(coord.x(), 1.5f);
    EXPECT_FLOAT_EQ(coord.y(), 2.5f);
    EXPECT_FLOAT_EQ(coord.z(), 3.5f);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_Vector3fConstructor) {
    Vector3f vec(2.0f, 3.0f, 4.0f);
    WorldCoordinates coord(vec);
    EXPECT_FLOAT_EQ(coord.x(), 2.0f);
    EXPECT_FLOAT_EQ(coord.y(), 3.0f);
    EXPECT_FLOAT_EQ(coord.z(), 4.0f);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_Addition) {
    WorldCoordinates a(1.0f, 2.0f, 3.0f);
    WorldCoordinates b(4.0f, 5.0f, 6.0f);
    WorldCoordinates result = a + b;
    
    EXPECT_FLOAT_EQ(result.x(), 5.0f);
    EXPECT_FLOAT_EQ(result.y(), 7.0f);
    EXPECT_FLOAT_EQ(result.z(), 9.0f);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_Subtraction) {
    WorldCoordinates a(4.0f, 5.0f, 6.0f);
    WorldCoordinates b(1.0f, 2.0f, 3.0f);
    WorldCoordinates result = a - b;
    
    EXPECT_FLOAT_EQ(result.x(), 3.0f);
    EXPECT_FLOAT_EQ(result.y(), 3.0f);
    EXPECT_FLOAT_EQ(result.z(), 3.0f);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_ScalarMultiplication) {
    WorldCoordinates coord(2.0f, 3.0f, 4.0f);
    WorldCoordinates result = coord * 2.5f;
    
    EXPECT_FLOAT_EQ(result.x(), 5.0f);
    EXPECT_FLOAT_EQ(result.y(), 7.5f);
    EXPECT_FLOAT_EQ(result.z(), 10.0f);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_ScalarMultiplication_Reverse) {
    WorldCoordinates coord(2.0f, 3.0f, 4.0f);
    WorldCoordinates result = 2.5f * coord;
    
    EXPECT_FLOAT_EQ(result.x(), 5.0f);
    EXPECT_FLOAT_EQ(result.y(), 7.5f);
    EXPECT_FLOAT_EQ(result.z(), 10.0f);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_ScalarDivision) {
    WorldCoordinates coord(6.0f, 9.0f, 12.0f);
    WorldCoordinates result = coord / 3.0f;
    
    EXPECT_FLOAT_EQ(result.x(), 2.0f);
    EXPECT_FLOAT_EQ(result.y(), 3.0f);
    EXPECT_FLOAT_EQ(result.z(), 4.0f);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_CompoundAssignment) {
    WorldCoordinates coord(1.0f, 2.0f, 3.0f);
    
    coord += WorldCoordinates(4.0f, 5.0f, 6.0f);
    EXPECT_FLOAT_EQ(coord.x(), 5.0f);
    EXPECT_FLOAT_EQ(coord.y(), 7.0f);
    EXPECT_FLOAT_EQ(coord.z(), 9.0f);
    
    coord -= WorldCoordinates(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(coord.x(), 4.0f);
    EXPECT_FLOAT_EQ(coord.y(), 5.0f);
    EXPECT_FLOAT_EQ(coord.z(), 6.0f);
    
    coord *= 2.0f;
    EXPECT_FLOAT_EQ(coord.x(), 8.0f);
    EXPECT_FLOAT_EQ(coord.y(), 10.0f);
    EXPECT_FLOAT_EQ(coord.z(), 12.0f);
    
    coord /= 4.0f;
    EXPECT_FLOAT_EQ(coord.x(), 2.0f);
    EXPECT_FLOAT_EQ(coord.y(), 2.5f);
    EXPECT_FLOAT_EQ(coord.z(), 3.0f);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_Comparison) {
    WorldCoordinates a(1.0f, 2.0f, 3.0f);
    WorldCoordinates b(1.0f, 2.0f, 3.0f);
    WorldCoordinates c(4.0f, 5.0f, 6.0f);
    
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_VectorOperations) {
    WorldCoordinates a(3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(a.length(), 5.0f);
    EXPECT_FLOAT_EQ(a.lengthSquared(), 25.0f);
    
    WorldCoordinates b(1.0f, 0.0f, 0.0f);
    WorldCoordinates c(0.0f, 1.0f, 0.0f);
    EXPECT_FLOAT_EQ(b.dot(c), 0.0f);
    
    WorldCoordinates cross = b.cross(c);
    EXPECT_FLOAT_EQ(cross.x(), 0.0f);
    EXPECT_FLOAT_EQ(cross.y(), 0.0f);
    EXPECT_FLOAT_EQ(cross.z(), 1.0f);
}

TEST_F(CoordinateTypesTest, WorldCoordinates_StaticConstants) {
    WorldCoordinates zero = WorldCoordinates::zero();
    EXPECT_FLOAT_EQ(zero.x(), 0.0f);
    EXPECT_FLOAT_EQ(zero.y(), 0.0f);
    EXPECT_FLOAT_EQ(zero.z(), 0.0f);
    
    WorldCoordinates unitX = WorldCoordinates::unitX();
    EXPECT_FLOAT_EQ(unitX.x(), 1.0f);
    EXPECT_FLOAT_EQ(unitX.y(), 0.0f);
    EXPECT_FLOAT_EQ(unitX.z(), 0.0f);
}

// ==================== IncrementCoordinates Tests ====================

TEST_F(CoordinateTypesTest, IncrementCoordinates_DefaultConstructor) {
    IncrementCoordinates coord;
    EXPECT_EQ(coord.x(), 0);
    EXPECT_EQ(coord.y(), 0);
    EXPECT_EQ(coord.z(), 0);
}

TEST_F(CoordinateTypesTest, IncrementCoordinates_ParameterConstructor) {
    IncrementCoordinates coord(10, 20, 30);
    EXPECT_EQ(coord.x(), 10);
    EXPECT_EQ(coord.y(), 20);
    EXPECT_EQ(coord.z(), 30);
}

TEST_F(CoordinateTypesTest, IncrementCoordinates_Arithmetic) {
    IncrementCoordinates a(10, 20, 30);
    IncrementCoordinates b(5, 10, 15);
    
    IncrementCoordinates sum = a + b;
    EXPECT_EQ(sum.x(), 15);
    EXPECT_EQ(sum.y(), 30);
    EXPECT_EQ(sum.z(), 45);
    
    IncrementCoordinates diff = a - b;
    EXPECT_EQ(diff.x(), 5);
    EXPECT_EQ(diff.y(), 10);
    EXPECT_EQ(diff.z(), 15);
}

TEST_F(CoordinateTypesTest, IncrementCoordinates_Hash) {
    IncrementCoordinates a(10, 20, 30);
    IncrementCoordinates b(10, 20, 30);
    IncrementCoordinates c(40, 50, 60);
    
    EXPECT_EQ(a.hash(), b.hash());
    EXPECT_NE(a.hash(), c.hash());
    
    // Test std::hash specialization
    std::hash<IncrementCoordinates> hasher;
    EXPECT_EQ(hasher(a), hasher(b));
    EXPECT_NE(hasher(a), hasher(c));
}

TEST_F(CoordinateTypesTest, IncrementCoordinates_CenteredCoordinateSystem) {
    // Test that increment coordinates support negative values for centered system
    IncrementCoordinates negative(-100, 0, -50);
    EXPECT_EQ(negative.x(), -100);
    EXPECT_EQ(negative.y(), 0);
    EXPECT_EQ(negative.z(), -50);
    
    IncrementCoordinates positive(100, 250, 50);
    EXPECT_EQ(positive.x(), 100);
    EXPECT_EQ(positive.y(), 250);
    EXPECT_EQ(positive.z(), 50);
    
    // Test operations with negative coordinates
    IncrementCoordinates sum = negative + positive;
    EXPECT_EQ(sum.x(), 0);
    EXPECT_EQ(sum.y(), 250);
    EXPECT_EQ(sum.z(), 0);
}

// ==================== ScreenCoordinates Tests ====================

TEST_F(CoordinateTypesTest, ScreenCoordinates_DefaultConstructor) {
    ScreenCoordinates coord;
    EXPECT_FLOAT_EQ(coord.x(), 0.0f);
    EXPECT_FLOAT_EQ(coord.y(), 0.0f);
}

TEST_F(CoordinateTypesTest, ScreenCoordinates_ParameterConstructor) {
    ScreenCoordinates coord(640.0f, 480.0f);
    EXPECT_FLOAT_EQ(coord.x(), 640.0f);
    EXPECT_FLOAT_EQ(coord.y(), 480.0f);
}

TEST_F(CoordinateTypesTest, ScreenCoordinates_Vector2fConstructor) {
    Vector2f vec(800.0f, 600.0f);
    ScreenCoordinates coord(vec);
    EXPECT_FLOAT_EQ(coord.x(), 800.0f);
    EXPECT_FLOAT_EQ(coord.y(), 600.0f);
}

TEST_F(CoordinateTypesTest, ScreenCoordinates_Arithmetic) {
    ScreenCoordinates a(100.0f, 200.0f);
    ScreenCoordinates b(300.0f, 400.0f);
    
    ScreenCoordinates sum = a + b;
    EXPECT_FLOAT_EQ(sum.x(), 400.0f);
    EXPECT_FLOAT_EQ(sum.y(), 600.0f);
    
    ScreenCoordinates diff = b - a;
    EXPECT_FLOAT_EQ(diff.x(), 200.0f);
    EXPECT_FLOAT_EQ(diff.y(), 200.0f);
    
    ScreenCoordinates scaled = a * 2.5f;
    EXPECT_FLOAT_EQ(scaled.x(), 250.0f);
    EXPECT_FLOAT_EQ(scaled.y(), 500.0f);
}

TEST_F(CoordinateTypesTest, ScreenCoordinates_VectorOperations) {
    ScreenCoordinates a(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(a.length(), 5.0f);
    EXPECT_FLOAT_EQ(a.lengthSquared(), 25.0f);
    
    ScreenCoordinates b(1.0f, 0.0f);
    ScreenCoordinates c(0.0f, 1.0f);
    EXPECT_FLOAT_EQ(b.dot(c), 0.0f);
}

TEST_F(CoordinateTypesTest, ScreenCoordinates_StaticConstants) {
    ScreenCoordinates zero = ScreenCoordinates::zero();
    EXPECT_FLOAT_EQ(zero.x(), 0.0f);
    EXPECT_FLOAT_EQ(zero.y(), 0.0f);
    
    ScreenCoordinates unitX = ScreenCoordinates::unitX();
    EXPECT_FLOAT_EQ(unitX.x(), 1.0f);
    EXPECT_FLOAT_EQ(unitX.y(), 0.0f);
}

// ==================== Type Safety Tests ====================

TEST_F(CoordinateTypesTest, TypeSafety_CannotMixTypes) {
    // These should not compile - testing that types are distinct
    // If you uncomment these lines, compilation should fail:
    
    // WorldCoordinates world(1.0f, 2.0f, 3.0f);
    // IncrementCoordinates increment(1, 2, 3);
    // WorldCoordinates result = world + increment;  // Should not compile
    // bool equal = (world == increment);             // Should not compile
}

TEST_F(CoordinateTypesTest, ValueAccess_Mutable) {
    WorldCoordinates world(1.0f, 2.0f, 3.0f);
    world.x() = 10.0f;
    world.y() = 20.0f;
    world.z() = 30.0f;
    
    EXPECT_FLOAT_EQ(world.x(), 10.0f);
    EXPECT_FLOAT_EQ(world.y(), 20.0f);
    EXPECT_FLOAT_EQ(world.z(), 30.0f);
    
    IncrementCoordinates increment(1, 2, 3);
    increment.x() = 100;
    increment.y() = 200;
    increment.z() = 300;
    
    EXPECT_EQ(increment.x(), 100);
    EXPECT_EQ(increment.y(), 200);
    EXPECT_EQ(increment.z(), 300);
}