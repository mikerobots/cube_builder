#include <gtest/gtest.h>
#include <unordered_set>
#include "core/selection/SelectionTypes.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Selection;

class SelectionTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// VoxelId Tests
TEST_F(SelectionTypesTest, VoxelId_DefaultConstruction) {
    VoxelId voxel;
    EXPECT_EQ(voxel.position, Math::IncrementCoordinates(Math::Vector3i::Zero()));
    EXPECT_EQ(voxel.resolution, VoxelData::VoxelResolution::Size_1cm);
}

TEST_F(SelectionTypesTest, VoxelId_ParameterizedConstruction) {
    Math::Vector3i pos(10, 20, 30);
    VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_8cm;
    
    VoxelId voxel(Math::IncrementCoordinates(pos), res);
    EXPECT_EQ(voxel.position.value(), pos);
    EXPECT_EQ(voxel.resolution, res);
}

TEST_F(SelectionTypesTest, VoxelId_Equality) {
    VoxelId voxel1(Math::IncrementCoordinates(Math::Vector3i(1, 2, 3)), VoxelData::VoxelResolution::Size_4cm);
    VoxelId voxel2(Math::IncrementCoordinates(Math::Vector3i(1, 2, 3)), VoxelData::VoxelResolution::Size_4cm);
    VoxelId voxel3(Math::IncrementCoordinates(Math::Vector3i(1, 2, 4)), VoxelData::VoxelResolution::Size_4cm);
    VoxelId voxel4(Math::IncrementCoordinates(Math::Vector3i(1, 2, 3)), VoxelData::VoxelResolution::Size_8cm);
    
    EXPECT_EQ(voxel1, voxel2);
    EXPECT_NE(voxel1, voxel3);
    EXPECT_NE(voxel1, voxel4);
}

TEST_F(SelectionTypesTest, VoxelId_Comparison) {
    VoxelId voxel1(Math::IncrementCoordinates(Math::Vector3i(1, 2, 3)), VoxelData::VoxelResolution::Size_1cm);
    VoxelId voxel2(Math::IncrementCoordinates(Math::Vector3i(1, 2, 3)), VoxelData::VoxelResolution::Size_2cm);
    VoxelId voxel3(Math::IncrementCoordinates(Math::Vector3i(2, 2, 3)), VoxelData::VoxelResolution::Size_1cm);
    
    // Resolution takes precedence
    EXPECT_LT(voxel1, voxel2);
    EXPECT_LT(voxel1, voxel3);
}

TEST_F(SelectionTypesTest, VoxelId_Hash) {
    VoxelId voxel1(Math::IncrementCoordinates(Math::Vector3i(1, 2, 3)), VoxelData::VoxelResolution::Size_4cm);
    VoxelId voxel2(Math::IncrementCoordinates(Math::Vector3i(1, 2, 3)), VoxelData::VoxelResolution::Size_4cm);
    VoxelId voxel3(Math::IncrementCoordinates(Math::Vector3i(1, 2, 4)), VoxelData::VoxelResolution::Size_4cm);
    
    EXPECT_EQ(voxel1.hash(), voxel2.hash());
    EXPECT_NE(voxel1.hash(), voxel3.hash());
}

TEST_F(SelectionTypesTest, VoxelId_GetWorldPosition) {
    // Test 1cm voxel at origin - placed on ground plane
    VoxelId voxel1(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), VoxelData::VoxelResolution::Size_1cm);
    // Bottom at Y=0, center at Y=0.005m
    EXPECT_EQ(voxel1.getWorldPosition(), Math::Vector3f(0.0f, 0.005f, 0.0f));
    
    // Test 8cm voxel at origin - placed on ground plane
    VoxelId voxel2(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), VoxelData::VoxelResolution::Size_8cm);
    // Bottom at Y=0, center at Y=0.04m
    EXPECT_EQ(voxel2.getWorldPosition(), Math::Vector3f(0.0f, 0.04f, 0.0f));
    
    // Test 8cm voxel at increment position (8, 16, 24)
    // Position in meters: (0.08, 0.16, 0.24), with center at Y + half voxel size
    VoxelId voxel3(Math::IncrementCoordinates(Math::Vector3i(8, 16, 24)), VoxelData::VoxelResolution::Size_8cm);
    EXPECT_EQ(voxel3.getWorldPosition(), Math::Vector3f(0.08f, 0.20f, 0.24f));
}

TEST_F(SelectionTypesTest, VoxelId_GetVoxelSize) {
    EXPECT_FLOAT_EQ(VoxelId(Math::IncrementCoordinates(Math::Vector3i::Zero()), VoxelData::VoxelResolution::Size_1cm).getVoxelSize(), 0.01f);
    EXPECT_FLOAT_EQ(VoxelId(Math::IncrementCoordinates(Math::Vector3i::Zero()), VoxelData::VoxelResolution::Size_2cm).getVoxelSize(), 0.02f);
    EXPECT_FLOAT_EQ(VoxelId(Math::IncrementCoordinates(Math::Vector3i::Zero()), VoxelData::VoxelResolution::Size_4cm).getVoxelSize(), 0.04f);
    EXPECT_FLOAT_EQ(VoxelId(Math::IncrementCoordinates(Math::Vector3i::Zero()), VoxelData::VoxelResolution::Size_8cm).getVoxelSize(), 0.08f);
    EXPECT_FLOAT_EQ(VoxelId(Math::IncrementCoordinates(Math::Vector3i::Zero()), VoxelData::VoxelResolution::Size_16cm).getVoxelSize(), 0.16f);
    EXPECT_FLOAT_EQ(VoxelId(Math::IncrementCoordinates(Math::Vector3i::Zero()), VoxelData::VoxelResolution::Size_32cm).getVoxelSize(), 0.32f);
    EXPECT_FLOAT_EQ(VoxelId(Math::IncrementCoordinates(Math::Vector3i::Zero()), VoxelData::VoxelResolution::Size_64cm).getVoxelSize(), 0.64f);
    EXPECT_FLOAT_EQ(VoxelId(Math::IncrementCoordinates(Math::Vector3i::Zero()), VoxelData::VoxelResolution::Size_128cm).getVoxelSize(), 1.28f);
    EXPECT_FLOAT_EQ(VoxelId(Math::IncrementCoordinates(Math::Vector3i::Zero()), VoxelData::VoxelResolution::Size_256cm).getVoxelSize(), 2.56f);
    EXPECT_FLOAT_EQ(VoxelId(Math::IncrementCoordinates(Math::Vector3i::Zero()), VoxelData::VoxelResolution::Size_512cm).getVoxelSize(), 5.12f);
}

TEST_F(SelectionTypesTest, VoxelId_GetBounds) {
    // Test with a voxel at origin - placed on ground plane
    VoxelId voxel(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), VoxelData::VoxelResolution::Size_4cm);
    Math::BoundingBox bounds = voxel.getBounds();
    
    // Voxel placed on ground plane with bottom face at Y=0
    // 4cm voxel extends from -0.02 to +0.02 in X/Z, and 0 to 0.04 in Y
    Math::Vector3f expectedMin(-0.02f, 0.0f, -0.02f);
    Math::Vector3f expectedMax(0.02f, 0.04f, 0.02f);
    
    EXPECT_EQ(bounds.min, expectedMin);
    EXPECT_EQ(bounds.max, expectedMax);
}


// SelectionStats Tests
TEST_F(SelectionTypesTest, SelectionStats_DefaultConstruction) {
    SelectionStats stats;
    EXPECT_EQ(stats.voxelCount, 0u);
    EXPECT_TRUE(stats.countByResolution.empty());
    EXPECT_EQ(stats.center, Math::Vector3f::Zero());
    EXPECT_FLOAT_EQ(stats.totalVolume, 0.0f);
}

TEST_F(SelectionTypesTest, SelectionStats_Clear) {
    SelectionStats stats;
    stats.voxelCount = 100;
    stats.countByResolution[VoxelData::VoxelResolution::Size_4cm] = 50;
    stats.totalVolume = 10.0f;
    
    stats.clear();
    
    EXPECT_EQ(stats.voxelCount, 0u);
    EXPECT_TRUE(stats.countByResolution.empty());
    EXPECT_EQ(stats.center, Math::Vector3f::Zero());
    EXPECT_FLOAT_EQ(stats.totalVolume, 0.0f);
}

// SelectionStyle Tests
TEST_F(SelectionTypesTest, SelectionStyle_DefaultValues) {
    SelectionStyle style;
    
    // Check outline color (green)
    EXPECT_FLOAT_EQ(style.outlineColor.r, 0.0f);
    EXPECT_FLOAT_EQ(style.outlineColor.g, 1.0f);
    EXPECT_FLOAT_EQ(style.outlineColor.b, 0.0f);
    EXPECT_FLOAT_EQ(style.outlineColor.a, 1.0f);
    
    // Check fill color (semi-transparent green)
    EXPECT_FLOAT_EQ(style.fillColor.r, 0.0f);
    EXPECT_FLOAT_EQ(style.fillColor.g, 1.0f);
    EXPECT_FLOAT_EQ(style.fillColor.b, 0.0f);
    EXPECT_FLOAT_EQ(style.fillColor.a, 0.2f);
    
    EXPECT_FLOAT_EQ(style.outlineThickness, 2.0f);
    EXPECT_TRUE(style.animated);
    EXPECT_FLOAT_EQ(style.animationSpeed, 1.0f);
    EXPECT_TRUE(style.showBounds);
    EXPECT_TRUE(style.showCount);
}

// SelectionContext Tests
TEST_F(SelectionTypesTest, SelectionContext_DefaultValues) {
    SelectionContext context;
    EXPECT_EQ(context.mode, SelectionMode::Replace);
    EXPECT_FALSE(context.continuous);
    EXPECT_FALSE(context.preview);
    EXPECT_FALSE(context.region.has_value());
    EXPECT_FALSE(context.filter.has_value());
}

// Hash specialization test
TEST_F(SelectionTypesTest, VoxelId_StdHash) {
    std::unordered_set<VoxelId> voxelSet;
    
    VoxelId voxel1(Math::Vector3i(1, 2, 3), VoxelData::VoxelResolution::Size_4cm);
    VoxelId voxel2(Math::Vector3i(1, 2, 3), VoxelData::VoxelResolution::Size_4cm);
    VoxelId voxel3(Math::Vector3i(4, 5, 6), VoxelData::VoxelResolution::Size_8cm);
    
    voxelSet.insert(voxel1);
    voxelSet.insert(voxel2); // Should not create duplicate
    voxelSet.insert(voxel3);
    
    EXPECT_EQ(voxelSet.size(), 2u);
    EXPECT_TRUE(voxelSet.count(voxel1) > 0);
    EXPECT_TRUE(voxelSet.count(voxel3) > 0);
}