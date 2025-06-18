#include <gtest/gtest.h>
#include "core/selection/SelectionSet.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Selection;

class SelectionSetTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create some test voxels - use 4cm increments for 4cm voxels to ensure they're distinct
        voxel1 = VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), VoxelData::VoxelResolution::Size_4cm);
        voxel2 = VoxelId(Math::IncrementCoordinates(Math::Vector3i(4, 0, 0)), VoxelData::VoxelResolution::Size_4cm);
        voxel3 = VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 4, 0)), VoxelData::VoxelResolution::Size_4cm);
        voxel4 = VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 0, 4)), VoxelData::VoxelResolution::Size_4cm);
        voxel5 = VoxelId(Math::IncrementCoordinates(Math::Vector3i(8, 8, 8)), VoxelData::VoxelResolution::Size_8cm);
    }
    
    VoxelId voxel1, voxel2, voxel3, voxel4, voxel5;
};

// Construction Tests
TEST_F(SelectionSetTest, DefaultConstruction) {
    // REQ: SelectionSet manages collections of selected voxels
    SelectionSet set;
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0u);
}

TEST_F(SelectionSetTest, VectorConstruction) {
    std::vector<VoxelId> voxels = {voxel1, voxel2, voxel3};
    SelectionSet set(voxels);
    
    EXPECT_EQ(set.size(), 3u);
    EXPECT_TRUE(set.contains(voxel1));
    EXPECT_TRUE(set.contains(voxel2));
    EXPECT_TRUE(set.contains(voxel3));
    EXPECT_FALSE(set.contains(voxel4));
}

TEST_F(SelectionSetTest, InitializerListConstruction) {
    SelectionSet set = {voxel1, voxel2, voxel3};
    
    EXPECT_EQ(set.size(), 3u);
    EXPECT_TRUE(set.contains(voxel1));
    EXPECT_TRUE(set.contains(voxel2));
    EXPECT_TRUE(set.contains(voxel3));
}

// Basic Operations Tests
TEST_F(SelectionSetTest, AddAndContains) {
    SelectionSet set;
    
    set.add(voxel1);
    EXPECT_TRUE(set.contains(voxel1));
    EXPECT_FALSE(set.contains(voxel2));
    EXPECT_EQ(set.size(), 1u);
    
    set.add(voxel2);
    EXPECT_TRUE(set.contains(voxel1));
    EXPECT_TRUE(set.contains(voxel2));
    EXPECT_EQ(set.size(), 2u);
    
    // Adding duplicate should not increase size
    set.add(voxel1);
    EXPECT_EQ(set.size(), 2u);
}

TEST_F(SelectionSetTest, Remove) {
    SelectionSet set = {voxel1, voxel2, voxel3};
    
    set.remove(voxel2);
    EXPECT_TRUE(set.contains(voxel1));
    EXPECT_FALSE(set.contains(voxel2));
    EXPECT_TRUE(set.contains(voxel3));
    EXPECT_EQ(set.size(), 2u);
    
    // Removing non-existent voxel should not change size
    set.remove(voxel4);
    EXPECT_EQ(set.size(), 2u);
}

TEST_F(SelectionSetTest, Clear) {
    SelectionSet set = {voxel1, voxel2, voxel3};
    EXPECT_FALSE(set.empty());
    
    set.clear();
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0u);
    EXPECT_FALSE(set.contains(voxel1));
}

// Bulk Operations Tests
TEST_F(SelectionSetTest, AddRange) {
    SelectionSet set;
    std::vector<VoxelId> voxels = {voxel1, voxel2, voxel3};
    
    set.addRange(voxels);
    EXPECT_EQ(set.size(), 3u);
    EXPECT_TRUE(set.contains(voxel1));
    EXPECT_TRUE(set.contains(voxel2));
    EXPECT_TRUE(set.contains(voxel3));
}

TEST_F(SelectionSetTest, RemoveRange) {
    SelectionSet set = {voxel1, voxel2, voxel3, voxel4};
    std::vector<VoxelId> toRemove = {voxel2, voxel3};
    
    set.removeRange(toRemove);
    EXPECT_EQ(set.size(), 2u);
    EXPECT_TRUE(set.contains(voxel1));
    EXPECT_FALSE(set.contains(voxel2));
    EXPECT_FALSE(set.contains(voxel3));
    EXPECT_TRUE(set.contains(voxel4));
}

TEST_F(SelectionSetTest, AddSet) {
    SelectionSet set1 = {voxel1, voxel2};
    SelectionSet set2 = {voxel3, voxel4};
    
    set1.addSet(set2);
    EXPECT_EQ(set1.size(), 4u);
    EXPECT_TRUE(set1.contains(voxel1));
    EXPECT_TRUE(set1.contains(voxel2));
    EXPECT_TRUE(set1.contains(voxel3));
    EXPECT_TRUE(set1.contains(voxel4));
}

TEST_F(SelectionSetTest, RemoveSet) {
    SelectionSet set1 = {voxel1, voxel2, voxel3, voxel4};
    SelectionSet set2 = {voxel2, voxel3};
    
    set1.removeSet(set2);
    EXPECT_EQ(set1.size(), 2u);
    EXPECT_TRUE(set1.contains(voxel1));
    EXPECT_FALSE(set1.contains(voxel2));
    EXPECT_FALSE(set1.contains(voxel3));
    EXPECT_TRUE(set1.contains(voxel4));
}

// Set Operations Tests
TEST_F(SelectionSetTest, UnionWith) {
    SelectionSet set1 = {voxel1, voxel2};
    SelectionSet set2 = {voxel2, voxel3};
    
    SelectionSet result = set1.unionWith(set2);
    EXPECT_EQ(result.size(), 3u);
    EXPECT_TRUE(result.contains(voxel1));
    EXPECT_TRUE(result.contains(voxel2));
    EXPECT_TRUE(result.contains(voxel3));
}

TEST_F(SelectionSetTest, IntersectWith) {
    SelectionSet set1 = {voxel1, voxel2, voxel3};
    SelectionSet set2 = {voxel2, voxel3, voxel4};
    
    SelectionSet result = set1.intersectWith(set2);
    EXPECT_EQ(result.size(), 2u);
    EXPECT_FALSE(result.contains(voxel1));
    EXPECT_TRUE(result.contains(voxel2));
    EXPECT_TRUE(result.contains(voxel3));
    EXPECT_FALSE(result.contains(voxel4));
}

TEST_F(SelectionSetTest, Subtract) {
    SelectionSet set1 = {voxel1, voxel2, voxel3};
    SelectionSet set2 = {voxel2, voxel3, voxel4};
    
    SelectionSet result = set1.subtract(set2);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_TRUE(result.contains(voxel1));
    EXPECT_FALSE(result.contains(voxel2));
    EXPECT_FALSE(result.contains(voxel3));
}

TEST_F(SelectionSetTest, SymmetricDifference) {
    SelectionSet set1 = {voxel1, voxel2, voxel3};
    SelectionSet set2 = {voxel2, voxel3, voxel4};
    
    SelectionSet result = set1.symmetricDifference(set2);
    EXPECT_EQ(result.size(), 2u);
    EXPECT_TRUE(result.contains(voxel1));
    EXPECT_FALSE(result.contains(voxel2));
    EXPECT_FALSE(result.contains(voxel3));
    EXPECT_TRUE(result.contains(voxel4));
}

// In-place Set Operations Tests
TEST_F(SelectionSetTest, Unite) {
    SelectionSet set1 = {voxel1, voxel2};
    SelectionSet set2 = {voxel2, voxel3};
    
    set1.unite(set2);
    EXPECT_EQ(set1.size(), 3u);
    EXPECT_TRUE(set1.contains(voxel1));
    EXPECT_TRUE(set1.contains(voxel2));
    EXPECT_TRUE(set1.contains(voxel3));
}

TEST_F(SelectionSetTest, Intersect) {
    SelectionSet set1 = {voxel1, voxel2, voxel3};
    SelectionSet set2 = {voxel2, voxel3, voxel4};
    
    set1.intersect(set2);
    EXPECT_EQ(set1.size(), 2u);
    EXPECT_FALSE(set1.contains(voxel1));
    EXPECT_TRUE(set1.contains(voxel2));
    EXPECT_TRUE(set1.contains(voxel3));
}

TEST_F(SelectionSetTest, SubtractFrom) {
    SelectionSet set1 = {voxel1, voxel2, voxel3};
    SelectionSet set2 = {voxel2, voxel3, voxel4};
    
    set1.subtractFrom(set2);
    EXPECT_EQ(set1.size(), 1u);
    EXPECT_TRUE(set1.contains(voxel1));
    EXPECT_FALSE(set1.contains(voxel2));
    EXPECT_FALSE(set1.contains(voxel3));
}

// Query Tests
TEST_F(SelectionSetTest, ToVector) {
    SelectionSet set = {voxel1, voxel2, voxel3};
    std::vector<VoxelId> vec = set.toVector();
    
    EXPECT_EQ(vec.size(), 3u);
    // Check all voxels are present (order not guaranteed)
    EXPECT_TRUE(std::find(vec.begin(), vec.end(), voxel1) != vec.end());
    EXPECT_TRUE(std::find(vec.begin(), vec.end(), voxel2) != vec.end());
    EXPECT_TRUE(std::find(vec.begin(), vec.end(), voxel3) != vec.end());
}

TEST_F(SelectionSetTest, GetBounds) {
    // REQ: Support for selection validation and bounds checking
    SelectionSet set = {voxel1, voxel2, voxel3};
    Math::BoundingBox bounds = set.getBounds();
    
    // With 4cm voxels:
    // voxel1 at IncrementCoordinates(0,0,0) -> bounds (0,0,0) to (0.04,0.04,0.04)
    // voxel2 at IncrementCoordinates(4,0,0) -> bounds (0.04,0,0) to (0.08,0.04,0.04)
    // voxel3 at IncrementCoordinates(0,4,0) -> bounds (0,0.04,0) to (0.04,0.08,0.04)
    // Combined bounds encompass all three voxels:
    EXPECT_EQ(bounds.min, Math::Vector3f(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(bounds.max, Math::Vector3f(0.08f, 0.08f, 0.04f));
}

TEST_F(SelectionSetTest, GetCenter) {
    // Create set with voxels at (0,0,0) and (-4,0,0) - use 4cm increments for proper alignment
    VoxelId v1(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm);
    VoxelId v2(Math::Vector3i(-4, 0, 0), VoxelData::VoxelResolution::Size_4cm);
    SelectionSet set = {v1, v2};
    
    Math::Vector3f center = set.getCenter();
    // v1 world position: (0,0,0) + half_size(0.02,0.02,0.02) = (0.02, 0.02, 0.02)
    // v2 world position: (-0.04,0,0) + half_size(0.02,0.02,0.02) = (-0.02, 0.02, 0.02)
    // average: ((0.02 + (-0.02)) / 2, (0.02 + 0.02) / 2, (0.02 + 0.02) / 2) = (0, 0.02, 0.02)
    EXPECT_NEAR(center.x, 0.0f, 0.001f);
    EXPECT_NEAR(center.y, 0.02f, 0.001f);
    EXPECT_NEAR(center.z, 0.02f, 0.001f);
}

TEST_F(SelectionSetTest, GetStats) {
    SelectionSet set = {voxel1, voxel2, voxel3, voxel5};
    SelectionStats stats = set.getStats();
    
    EXPECT_EQ(stats.voxelCount, 4u);
    EXPECT_EQ(stats.countByResolution[VoxelData::VoxelResolution::Size_4cm], 3u);
    EXPECT_EQ(stats.countByResolution[VoxelData::VoxelResolution::Size_8cm], 1u);
    
    // Check volume calculation (3 * 0.04^3 + 1 * 0.08^3)
    float expectedVolume = 3 * 0.04f * 0.04f * 0.04f + 0.08f * 0.08f * 0.08f;
    EXPECT_NEAR(stats.totalVolume, expectedVolume, 0.0001f);
}

// Filtering Tests
TEST_F(SelectionSetTest, Filter) {
    // REQ: Performance optimization for large selections
    SelectionSet set = {voxel1, voxel2, voxel3, voxel4, voxel5};
    
    // Filter for 4cm resolution only
    auto predicate = [](const VoxelId& v) { 
        return v.resolution == VoxelData::VoxelResolution::Size_4cm; 
    };
    
    SelectionSet filtered = set.filter(predicate);
    EXPECT_EQ(filtered.size(), 4u);
    EXPECT_TRUE(filtered.contains(voxel1));
    EXPECT_TRUE(filtered.contains(voxel2));
    EXPECT_TRUE(filtered.contains(voxel3));
    EXPECT_TRUE(filtered.contains(voxel4));
    EXPECT_FALSE(filtered.contains(voxel5));
}

TEST_F(SelectionSetTest, FilterInPlace) {
    SelectionSet set = {voxel1, voxel2, voxel3, voxel4, voxel5};
    
    // Filter for positive x coordinates
    auto predicate = [](const VoxelId& v) { 
        return v.position.x() > 0; 
    };
    
    set.filterInPlace(predicate);
    EXPECT_EQ(set.size(), 2u);
    EXPECT_FALSE(set.contains(voxel1));
    EXPECT_TRUE(set.contains(voxel2));
    EXPECT_FALSE(set.contains(voxel3));
    EXPECT_FALSE(set.contains(voxel4));
    EXPECT_TRUE(set.contains(voxel5));
}

// Iteration Tests
TEST_F(SelectionSetTest, Iteration) {
    SelectionSet set = {voxel1, voxel2, voxel3};
    
    int count = 0;
    for (const auto& voxel : set) {
        count++;
        EXPECT_TRUE(voxel == voxel1 || voxel == voxel2 || voxel == voxel3);
    }
    EXPECT_EQ(count, 3);
}

TEST_F(SelectionSetTest, ForEach) {
    SelectionSet set = {voxel1, voxel2, voxel3};
    
    std::vector<VoxelId> visited;
    set.forEach([&visited](const VoxelId& voxel) {
        visited.push_back(voxel);
    });
    
    EXPECT_EQ(visited.size(), 3u);
}

// Comparison Tests
TEST_F(SelectionSetTest, Equality) {
    SelectionSet set1 = {voxel1, voxel2, voxel3};
    SelectionSet set2 = {voxel3, voxel1, voxel2}; // Different order
    SelectionSet set3 = {voxel1, voxel2};
    
    EXPECT_EQ(set1, set2);
    EXPECT_NE(set1, set3);
}

// Utility Function Tests
TEST_F(SelectionSetTest, MakeBoxSelection) {
    Math::BoundingBox box(Math::Vector3f(0.0f, 0.0f, 0.0f), 
                          Math::Vector3f(0.1f, 0.1f, 0.1f));
    
    SelectionSet selection = makeBoxSelection(box, VoxelData::VoxelResolution::Size_4cm);
    
    // Should contain voxels at (0,0,0), (1,0,0), (2,0,0), (0,1,0), etc.
    EXPECT_GT(selection.size(), 0u);
    EXPECT_TRUE(selection.contains(VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), VoxelData::VoxelResolution::Size_4cm)));
}

TEST_F(SelectionSetTest, MakeSphereSelection) {
    Math::Vector3f center(0.05f, 0.05f, 0.05f);
    float radius = 0.1f;
    
    SelectionSet selection = makeSphereSelection(center, radius, VoxelData::VoxelResolution::Size_4cm);
    
    EXPECT_GT(selection.size(), 0u);
    
    // Check that all selected voxels are within sphere
    for (const auto& voxel : selection) {
        float dist = (voxel.getWorldPosition() - center).length();
        EXPECT_LE(dist, radius + 0.04f); // Account for voxel size
    }
}

TEST_F(SelectionSetTest, MakeCylinderSelection) {
    Math::Vector3f base(0.0f, 0.0f, 0.0f);
    Math::Vector3f direction(0.0f, 1.0f, 0.0f);
    float radius = 0.1f;
    float height = 0.2f;
    
    SelectionSet selection = makeCylinderSelection(base, direction, radius, height, 
                                                  VoxelData::VoxelResolution::Size_4cm);
    
    EXPECT_GT(selection.size(), 0u);
}