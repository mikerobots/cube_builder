#include <gtest/gtest.h>
#include "core/selection/FloodFillSelector.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Selection;

class FloodFillSelectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        selector = std::make_unique<FloodFillSelector>();
        
        // Set smaller max voxels for faster testing
        selector->setMaxVoxels(100);
        
        // Create test voxels
        seed = VoxelId(Math::IncrementCoordinates(Math::Vector3i(5, 5, 5)), VoxelData::VoxelResolution::Size_4cm);
    }
    
    std::unique_ptr<FloodFillSelector> selector;
    VoxelId seed;
};

// Basic Tests
TEST_F(FloodFillSelectorTest, DefaultConfiguration) {
    // Create fresh selector to test defaults
    auto freshSelector = std::make_unique<FloodFillSelector>();
    EXPECT_EQ(freshSelector->getMaxVoxels(), 1000000u);
    EXPECT_FALSE(freshSelector->getDiagonalConnectivity());
    EXPECT_EQ(freshSelector->getConnectivityMode(), FloodFillSelector::ConnectivityMode::Face6);
}

TEST_F(FloodFillSelectorTest, SetConfiguration) {
    selector->setMaxVoxels(5000);
    selector->setDiagonalConnectivity(true);
    selector->setConnectivityMode(FloodFillSelector::ConnectivityMode::Vertex26);
    
    EXPECT_EQ(selector->getMaxVoxels(), 5000u);
    EXPECT_TRUE(selector->getDiagonalConnectivity());
    EXPECT_EQ(selector->getConnectivityMode(), FloodFillSelector::ConnectivityMode::Vertex26);
}

// Basic Flood Fill Tests
TEST_F(FloodFillSelectorTest, SelectFloodFill_SingleVoxel) {
    // REQ: FloodFillSelector for different selection methods
    // Without voxel manager, assumes all voxels exist
    SelectionSet result = selector->selectFloodFill(seed, FloodFillCriteria::Connected6);
    
    // Should at least contain the seed
    EXPECT_GE(result.size(), 1u);
    EXPECT_TRUE(result.contains(seed));
}

TEST_F(FloodFillSelectorTest, SelectFloodFill_Connected) {
    selector->setConnectivityMode(FloodFillSelector::ConnectivityMode::Face6);
    
    SelectionSet result = selector->selectFloodFill(seed, FloodFillCriteria::Connected6);
    
    // Should contain seed and its face-connected neighbors
    EXPECT_TRUE(result.contains(seed));
    
    // Without voxel manager, it will fill many voxels
    EXPECT_GT(result.size(), 1u);
}

TEST_F(FloodFillSelectorTest, SelectFloodFill_SameResolution) {
    VoxelId mixedResSeed(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_8cm);
    
    SelectionSet result = selector->selectFloodFill(mixedResSeed, FloodFillCriteria::SameResolution);
    
    // All selected voxels should have same resolution
    for (const auto& voxel : result) {
        EXPECT_EQ(voxel.resolution, VoxelData::VoxelResolution::Size_8cm);
    }
}

// Custom Predicate Tests
TEST_F(FloodFillSelectorTest, SelectFloodFillCustom_PositiveCoordinates) {
    VoxelId originSeed(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm);
    
    // Only select voxels with all positive coordinates
    auto predicate = [](const VoxelId& voxel) {
        return voxel.position.x() >= 0 && voxel.position.y() >= 0 && voxel.position.z() >= 0;
    };
    
    SelectionSet result = selector->selectFloodFillCustom(originSeed, predicate);
    
    // Check all selected voxels meet criteria
    for (const auto& voxel : result) {
        EXPECT_GE(voxel.position.x(), 0);
        EXPECT_GE(voxel.position.y(), 0);
        EXPECT_GE(voxel.position.z(), 0);
    }
}

TEST_F(FloodFillSelectorTest, SelectFloodFillCustom_MaxDistance) {
    VoxelId originSeed(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm);
    const int maxDist = 3;
    
    // Only select voxels within Manhattan distance of 3
    auto predicate = [&originSeed, maxDist](const VoxelId& voxel) {
        Math::Vector3i diff = voxel.position.value() - originSeed.position.value();
        int manhattan = std::abs(diff.x) + std::abs(diff.y) + std::abs(diff.z);
        return manhattan <= maxDist;
    };
    
    SelectionSet result = selector->selectFloodFillCustom(originSeed, predicate);
    
    // Check all selected voxels are within distance
    for (const auto& voxel : result) {
        Math::Vector3i diff = voxel.position.value() - originSeed.position.value();
        int manhattan = std::abs(diff.x) + std::abs(diff.y) + std::abs(diff.z);
        EXPECT_LE(manhattan, maxDist);
    }
}

// Limited Flood Fill Tests
TEST_F(FloodFillSelectorTest, SelectFloodFillLimited_OneStep) {
    VoxelId originSeed(Math::Vector3i(10, 10, 10), VoxelData::VoxelResolution::Size_4cm);
    
    SelectionSet result = selector->selectFloodFillLimited(originSeed, FloodFillCriteria::Connected6, 1);
    
    // Should contain seed plus immediate neighbors (max 7 for Face6)
    EXPECT_LE(result.size(), 7u);
    EXPECT_TRUE(result.contains(originSeed));
}

TEST_F(FloodFillSelectorTest, SelectFloodFillLimited_MultipleSteps) {
    VoxelId originSeed(Math::Vector3i(20, 20, 20), VoxelData::VoxelResolution::Size_4cm);
    
    SelectionSet result = selector->selectFloodFillLimited(originSeed, FloodFillCriteria::Connected6, 3);
    
    // Should be limited to 3 steps from seed
    EXPECT_GT(result.size(), 7u); // More than 1 step
    EXPECT_TRUE(result.contains(originSeed));
    
    // Check max distance
    for (const auto& voxel : result) {
        Math::Vector3i diff = voxel.position.value() - originSeed.position.value();
        int manhattan = std::abs(diff.x) + std::abs(diff.y) + std::abs(diff.z);
        EXPECT_LE(manhattan, 3);
    }
}

// Bounded Flood Fill Tests
TEST_F(FloodFillSelectorTest, SelectFloodFillBounded_InsideBounds) {
    Math::BoundingBox bounds(
        Math::Vector3f(0.18f, 0.18f, 0.18f),  // 4.5, 4.5, 4.5 in voxel coords
        Math::Vector3f(0.22f, 0.22f, 0.22f)   // 5.5, 5.5, 5.5 in voxel coords
    );
    
    SelectionSet result = selector->selectFloodFillBounded(seed, FloodFillCriteria::Connected6, bounds);
    
    // Should only contain voxels within bounds
    for (const auto& voxel : result) {
        EXPECT_TRUE(bounds.contains(voxel.getWorldPosition()));
    }
}

TEST_F(FloodFillSelectorTest, SelectFloodFillBounded_OutsideBounds) {
    Math::BoundingBox bounds(
        Math::Vector3f(0.0f, 0.0f, 0.0f),
        Math::Vector3f(0.04f, 0.04f, 0.04f)  // Smaller bounds that exclude the seed
    );
    
    // Seed is at increment (5,5,5) which is world position ~(0.05, 0.05, 0.05) - outside these bounds
    SelectionSet result = selector->selectFloodFillBounded(seed, FloodFillCriteria::Connected6, bounds);
    
    // Should be empty since seed is outside bounds
    EXPECT_TRUE(result.empty());
}

// Planar Flood Fill Tests
TEST_F(FloodFillSelectorTest, SelectPlanarFloodFill_HorizontalPlane) {
    VoxelId planeSeed(Math::Vector3i(0, 5, 0), VoxelData::VoxelResolution::Size_4cm);
    Math::Vector3f normal(0, 1, 0); // Y-axis normal (horizontal plane)
    float tolerance = 0.01f;
    
    SelectionSet result = selector->selectPlanarFloodFill(planeSeed, normal, tolerance);
    
    // All selected voxels should be on same Y level
    float seedY = planeSeed.getWorldPosition().y;
    for (const auto& voxel : result) {
        float voxelY = voxel.getWorldPosition().y;
        EXPECT_NEAR(voxelY, seedY, tolerance);
    }
}

TEST_F(FloodFillSelectorTest, SelectPlanarFloodFill_TiltedPlane) {
    VoxelId planeSeed(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm);
    Math::Vector3f normal(1, 1, 0); // 45-degree tilted plane
    float tolerance = 0.02f;
    
    SelectionSet result = selector->selectPlanarFloodFill(planeSeed, normal, tolerance);
    
    // Verify all voxels are on the plane
    Math::Vector3f normalizedNormal = normal.normalized();
    float planeD = -normalizedNormal.dot(planeSeed.getWorldPosition());
    
    for (const auto& voxel : result) {
        float distance = std::abs(normalizedNormal.dot(voxel.getWorldPosition()) + planeD);
        EXPECT_LE(distance, tolerance);
    }
}

// Connectivity Mode Tests
TEST_F(FloodFillSelectorTest, ConnectivityMode_Face6) {
    selector->setConnectivityMode(FloodFillSelector::ConnectivityMode::Face6);
    selector->setMaxVoxels(7); // Limit to immediate neighbors
    
    VoxelId centerSeed(Math::Vector3i(50, 50, 50), VoxelData::VoxelResolution::Size_4cm);
    SelectionSet result = selector->selectFloodFill(centerSeed, FloodFillCriteria::Connected6);
    
    // Should have at most 7 voxels (center + 6 faces)
    EXPECT_LE(result.size(), 7u);
}

TEST_F(FloodFillSelectorTest, ConnectivityMode_Edge18) {
    selector->setConnectivityMode(FloodFillSelector::ConnectivityMode::Edge18);
    selector->setMaxVoxels(19); // Limit to immediate neighbors
    
    VoxelId centerSeed(Math::Vector3i(60, 60, 60), VoxelData::VoxelResolution::Size_4cm);
    SelectionSet result = selector->selectFloodFill(centerSeed, FloodFillCriteria::Connected6);
    
    // Should have at most 19 voxels (center + 6 faces + 12 edges)
    EXPECT_LE(result.size(), 19u);
}

TEST_F(FloodFillSelectorTest, ConnectivityMode_Vertex26) {
    selector->setConnectivityMode(FloodFillSelector::ConnectivityMode::Vertex26);
    selector->setMaxVoxels(27); // Limit to immediate neighbors
    
    VoxelId centerSeed(Math::Vector3i(70, 70, 70), VoxelData::VoxelResolution::Size_4cm);
    SelectionSet result = selector->selectFloodFill(centerSeed, FloodFillCriteria::Connected6);
    
    // Should have at most 27 voxels (center + all 26 neighbors)
    EXPECT_LE(result.size(), 27u);
}

// Max Voxels Test
TEST_F(FloodFillSelectorTest, MaxVoxelsLimit) {
    selector->setMaxVoxels(10);
    
    VoxelId originSeed(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm);
    SelectionSet result = selector->selectFloodFill(originSeed, FloodFillCriteria::Connected6);
    
    // Should not exceed max voxels
    EXPECT_LE(result.size(), 10u);
}

// Edge Cases
TEST_F(FloodFillSelectorTest, SelectFloodFill_NonExistentSeed) {
    // Create a selector with voxel manager that returns false for voxelExists
    // For now, this will still work as we assume all voxels exist
    VoxelId nonExistentSeed(Math::Vector3i(1000, 1000, 1000), VoxelData::VoxelResolution::Size_4cm);
    
    SelectionSet result = selector->selectFloodFill(nonExistentSeed, FloodFillCriteria::Connected6);
    
    // Without proper voxel manager, it will still select voxels
    EXPECT_GE(result.size(), 1u);
}

TEST_F(FloodFillSelectorTest, SelectFloodFillCustom_FalsePredicate) {
    // Predicate that always returns false
    auto predicate = [](const VoxelId& voxel) { return false; };
    
    SelectionSet result = selector->selectFloodFillCustom(seed, predicate);
    
    // Should be empty
    EXPECT_TRUE(result.empty());
}

TEST_F(FloodFillSelectorTest, SelectPlanarFloodFill_ZeroTolerance) {
    VoxelId planeSeed(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm);
    Math::Vector3f normal(0, 0, 1); // Z-axis normal
    float tolerance = 0.0f;
    
    SelectionSet result = selector->selectPlanarFloodFill(planeSeed, normal, tolerance);
    
    // With zero tolerance, should be very limited
    EXPECT_GE(result.size(), 1u); // At least the seed
}

// Voxel Manager Tests
TEST_F(FloodFillSelectorTest, SetVoxelManager) {
    selector->setVoxelManager(nullptr);
    
    // Selection should still work without manager (assumes all voxels exist)
    SelectionSet result = selector->selectFloodFill(seed, FloodFillCriteria::Connected6);
    EXPECT_GT(result.size(), 0u);
}