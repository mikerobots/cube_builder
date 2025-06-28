#include <gtest/gtest.h>
#include <chrono>
#include "../VoxelDataManager.h"
#include "../../../foundation/math/CoordinateTypes.h"
#include "../../../foundation/math/CoordinateConverter.h"
#include "../../../foundation/math/BoundingBox.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

class VoxelDataRegionOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        voxelManager = std::make_unique<VoxelDataManager>();
        // Set default workspace size to 5m (500cm)
        voxelManager->resizeWorkspace(5.0f);
    }
    
    std::unique_ptr<VoxelDataManager> voxelManager;
};

// Test FillResult struct
TEST_F(VoxelDataRegionOperationsTest, FillResultDefaultConstruction) {
    FillResult result;
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.voxelsFilled, 0u);
    EXPECT_EQ(result.voxelsSkipped, 0u);
    EXPECT_EQ(result.totalPositions, 0u);
    EXPECT_TRUE(result.errorMessage.empty());
    EXPECT_EQ(result.failedBelowGround, 0u);
    EXPECT_EQ(result.failedOutOfBounds, 0u);
    EXPECT_EQ(result.failedOverlap, 0u);
    EXPECT_EQ(result.failedNotAligned, 0u);
}

// Test RegionQuery struct
TEST_F(VoxelDataRegionOperationsTest, RegionQueryDefaultConstruction) {
    RegionQuery query;
    EXPECT_EQ(query.voxelCount, 0u);
    EXPECT_TRUE(query.isEmpty);
    EXPECT_TRUE(query.voxels.empty());
}

// Test fillRegion method
TEST_F(VoxelDataRegionOperationsTest, FillRegion_EmptyRegion) {
    // Fill a very small region for fast testing
    BoundingBox region(Vector3f(-0.02f, 0.0f, -0.02f), Vector3f(0.02f, 0.02f, 0.02f));
    auto result = voxelManager->fillRegion(region, VoxelResolution::Size_1cm);
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.voxelsFilled, 0u);
    EXPECT_EQ(result.voxelsSkipped, 0u);
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(VoxelDataRegionOperationsTest, FillRegion_LargerVoxels) {
    // Fill with 4cm voxels - smaller region for fast testing
    BoundingBox region(Vector3f(-0.04f, 0.0f, -0.04f), Vector3f(0.04f, 0.04f, 0.04f));
    auto result = voxelManager->fillRegion(region, VoxelResolution::Size_4cm);
    
    EXPECT_TRUE(result.success);
    // Should fill some voxels (exact count depends on grid alignment)
    EXPECT_GT(result.voxelsFilled, 0u);
    EXPECT_LT(result.voxelsFilled, 100u);  // Reasonable upper bound
}

TEST_F(VoxelDataRegionOperationsTest, FillRegion_BelowGroundPlane) {
    // Try to fill below ground
    BoundingBox region(Vector3f(-0.1f, -0.1f, -0.1f), Vector3f(0.1f, 0.0f, 0.1f));
    auto result = voxelManager->fillRegion(region, VoxelResolution::Size_1cm);
    
    // Should partially succeed (only Y=0 voxels)
    EXPECT_TRUE(result.success);  // Partial success
    EXPECT_GT(result.voxelsFilled, 0u);
    EXPECT_GT(result.failedBelowGround, 0u);
}

TEST_F(VoxelDataRegionOperationsTest, FillRegion_OutsideWorkspace) {
    // Try to fill outside workspace bounds
    BoundingBox region(Vector3f(3.0f, 0.0f, 3.0f), Vector3f(4.0f, 1.0f, 4.0f));
    auto result = voxelManager->fillRegion(region, VoxelResolution::Size_1cm);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.voxelsFilled, 0u);
    EXPECT_GT(result.failedOutOfBounds, 0u);
}

TEST_F(VoxelDataRegionOperationsTest, FillRegion_WithOverlap) {
    // First fill a region
    BoundingBox region1(Vector3f(-0.05f, 0.0f, -0.05f), Vector3f(0.05f, 0.05f, 0.05f));
    auto result1 = voxelManager->fillRegion(region1, VoxelResolution::Size_1cm);
    EXPECT_TRUE(result1.success);
    
    // Try to fill overlapping region
    BoundingBox region2(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.1f, 0.1f, 0.1f));
    auto result2 = voxelManager->fillRegion(region2, VoxelResolution::Size_1cm);
    
    // Should partially succeed (non-overlapping voxels only)
    EXPECT_TRUE(result2.success);
    EXPECT_GT(result2.voxelsFilled, 0u);
    EXPECT_GT(result2.voxelsSkipped, 0u);  // Already filled voxels
}

TEST_F(VoxelDataRegionOperationsTest, FillRegion_Clear) {
    // First fill a region
    BoundingBox region(Vector3f(-0.05f, 0.0f, -0.05f), Vector3f(0.05f, 0.05f, 0.05f));
    auto result1 = voxelManager->fillRegion(region, VoxelResolution::Size_1cm, true);
    EXPECT_TRUE(result1.success);
    
    size_t filledCount = result1.voxelsFilled;
    
    // Now clear the same region
    auto result2 = voxelManager->fillRegion(region, VoxelResolution::Size_1cm, false);
    EXPECT_TRUE(result2.success);
    EXPECT_EQ(result2.voxelsFilled, filledCount);  // Should clear same number
}

// Test canFillRegion method
TEST_F(VoxelDataRegionOperationsTest, CanFillRegion_ValidRegion) {
    BoundingBox region(Vector3f(-0.1f, 0.0f, -0.1f), Vector3f(0.1f, 0.1f, 0.1f));
    EXPECT_TRUE(voxelManager->canFillRegion(region, VoxelResolution::Size_1cm));
}

TEST_F(VoxelDataRegionOperationsTest, CanFillRegion_BelowGround) {
    BoundingBox region(Vector3f(-0.1f, -0.2f, -0.1f), Vector3f(0.1f, -0.1f, 0.1f));
    EXPECT_FALSE(voxelManager->canFillRegion(region, VoxelResolution::Size_1cm));
}

TEST_F(VoxelDataRegionOperationsTest, CanFillRegion_OutsideBounds) {
    BoundingBox region(Vector3f(5.0f, 0.0f, 5.0f), Vector3f(6.0f, 1.0f, 6.0f));
    EXPECT_FALSE(voxelManager->canFillRegion(region, VoxelResolution::Size_1cm));
}

// Test isRegionEmpty method
TEST_F(VoxelDataRegionOperationsTest, IsRegionEmpty_EmptyRegion) {
    BoundingBox region(Vector3f(-0.1f, 0.0f, -0.1f), Vector3f(0.1f, 0.1f, 0.1f));
    EXPECT_TRUE(voxelManager->isRegionEmpty(region));
}

TEST_F(VoxelDataRegionOperationsTest, IsRegionEmpty_AfterFilling) {
    BoundingBox region(Vector3f(-0.1f, 0.0f, -0.1f), Vector3f(0.1f, 0.1f, 0.1f));
    
    // Fill the region
    voxelManager->fillRegion(region, VoxelResolution::Size_1cm);
    
    // Should no longer be empty
    EXPECT_FALSE(voxelManager->isRegionEmpty(region));
    
    // Adjacent region should still be empty
    BoundingBox adjacentRegion(Vector3f(0.2f, 0.0f, 0.2f), Vector3f(0.3f, 0.1f, 0.3f));
    EXPECT_TRUE(voxelManager->isRegionEmpty(adjacentRegion));
}

TEST_F(VoxelDataRegionOperationsTest, IsRegionEmpty_PartialOverlap) {
    // Place a single voxel
    voxelManager->setVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, true);
    
    // Region that partially contains the voxel
    BoundingBox region(Vector3f(-0.005f, -0.005f, -0.005f), Vector3f(0.1f, 0.1f, 0.1f));
    EXPECT_FALSE(voxelManager->isRegionEmpty(region));
}

// Test queryRegion method
TEST_F(VoxelDataRegionOperationsTest, QueryRegion_Empty) {
    BoundingBox region(Vector3f(-0.1f, 0.0f, -0.1f), Vector3f(0.1f, 0.1f, 0.1f));
    auto query = voxelManager->queryRegion(region);
    
    EXPECT_TRUE(query.isEmpty);
    EXPECT_EQ(query.voxelCount, 0u);
    EXPECT_TRUE(query.voxels.empty());
}

TEST_F(VoxelDataRegionOperationsTest, QueryRegion_WithVoxels) {
    // Place some voxels
    voxelManager->setVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, true);
    voxelManager->setVoxel(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm, true);
    voxelManager->setVoxel(IncrementCoordinates(0, 0, 1), VoxelResolution::Size_1cm, true);
    
    BoundingBox region(Vector3f(-0.01f, -0.01f, -0.01f), Vector3f(0.02f, 0.02f, 0.02f));
    auto query = voxelManager->queryRegion(region, true);  // Include voxel list
    
    EXPECT_FALSE(query.isEmpty);
    EXPECT_EQ(query.voxelCount, 3u);
    EXPECT_EQ(query.voxels.size(), 3u);
    
    // Check actual bounds
    EXPECT_TRUE(query.actualBounds.isValid());
}

TEST_F(VoxelDataRegionOperationsTest, QueryRegion_MixedResolutions) {
    // Place voxels of different sizes
    voxelManager->setVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, true);
    voxelManager->setVoxel(IncrementCoordinates(4, 0, 0), VoxelResolution::Size_4cm, true);
    
    BoundingBox region(Vector3f(-0.05f, -0.01f, -0.05f), Vector3f(0.1f, 0.1f, 0.05f));
    auto query = voxelManager->queryRegion(region);
    
    EXPECT_FALSE(query.isEmpty);
    EXPECT_EQ(query.voxelCount, 2u);  // Both voxels
}

// Test getVoxelsInRegion method
TEST_F(VoxelDataRegionOperationsTest, GetVoxelsInRegion_Empty) {
    BoundingBox region(Vector3f(-0.1f, 0.0f, -0.1f), Vector3f(0.1f, 0.1f, 0.1f));
    auto voxels = voxelManager->getVoxelsInRegion(region);
    
    EXPECT_TRUE(voxels.empty());
}

TEST_F(VoxelDataRegionOperationsTest, GetVoxelsInRegion_WithVoxels) {
    // Fill a region
    BoundingBox fillRegion(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.04f, 0.04f, 0.04f));
    voxelManager->fillRegion(fillRegion, VoxelResolution::Size_1cm);
    
    // Query a larger region
    BoundingBox queryRegion(Vector3f(-0.01f, -0.01f, -0.01f), Vector3f(0.05f, 0.05f, 0.05f));
    auto voxels = voxelManager->getVoxelsInRegion(queryRegion);
    
    EXPECT_FALSE(voxels.empty());
    
    // Verify all returned voxels are actually in the region
    for (const auto& voxel : voxels) {
        Vector3f voxelMin, voxelMax;
        voxel.getWorldBounds(voxelMin, voxelMax);
        BoundingBox voxelBounds(voxelMin, voxelMax);
        EXPECT_TRUE(queryRegion.intersects(voxelBounds));
    }
}

// Test edge cases
TEST_F(VoxelDataRegionOperationsTest, EdgeCase_TinyRegion) {
    // Region smaller than a voxel
    BoundingBox region(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.005f, 0.005f, 0.005f));
    auto result = voxelManager->fillRegion(region, VoxelResolution::Size_1cm);
    
    // Should fill some voxels (depends on grid alignment)
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.voxelsFilled, 0u);
    EXPECT_LT(result.voxelsFilled, 20u);  // Reasonable upper bound
}

TEST_F(VoxelDataRegionOperationsTest, EdgeCase_ExactVoxelBounds) {
    // Region exactly matching voxel bounds
    BoundingBox region(Vector3f(-0.005f, 0.0f, -0.005f), Vector3f(0.005f, 0.01f, 0.005f));
    auto result = voxelManager->fillRegion(region, VoxelResolution::Size_1cm);
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.voxelsFilled, 0u);
    EXPECT_LT(result.voxelsFilled, 20u);  // Reasonable upper bound
}

TEST_F(VoxelDataRegionOperationsTest, EdgeCase_LargeRegion) {
    // Fill a medium region with larger voxels for performance
    BoundingBox region(Vector3f(-0.5f, 0.0f, -0.5f), Vector3f(0.5f, 0.5f, 0.5f));
    
    // Use larger voxels for performance
    auto result = voxelManager->fillRegion(region, VoxelResolution::Size_16cm);
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.voxelsFilled, 0u);
    
    // Verify no voxels failed
    EXPECT_EQ(result.failedBelowGround, 0u);
    EXPECT_EQ(result.failedOutOfBounds, 0u);
}

// Test alignment issues
TEST_F(VoxelDataRegionOperationsTest, FillRegion_UnalignedBounds) {
    // Region with unaligned bounds for 4cm voxels
    BoundingBox region(Vector3f(0.01f, 0.0f, 0.01f), Vector3f(0.09f, 0.08f, 0.09f));
    auto result = voxelManager->fillRegion(region, VoxelResolution::Size_4cm);
    
    // Should align and fill properly
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.voxelsFilled, 0u);
}

// Performance test (basic)
TEST_F(VoxelDataRegionOperationsTest, Performance_MediumRegionFill) {
    // Fill a small region for quick performance testing
    BoundingBox region(Vector3f(-0.2f, 0.0f, -0.2f), Vector3f(0.2f, 0.2f, 0.2f));
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = voxelManager->fillRegion(region, VoxelResolution::Size_4cm);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.voxelsFilled, 0u);
    
    // Should complete quickly
    EXPECT_LT(duration.count(), 100);  // Less than 100ms
}

// Test clear operations
TEST_F(VoxelDataRegionOperationsTest, FillRegion_ClearSpecificRegion) {
    // Fill two separate regions
    BoundingBox region1(Vector3f(-0.1f, 0.0f, -0.1f), Vector3f(-0.05f, 0.05f, -0.05f));
    BoundingBox region2(Vector3f(0.05f, 0.0f, 0.05f), Vector3f(0.1f, 0.05f, 0.1f));
    
    voxelManager->fillRegion(region1, VoxelResolution::Size_1cm);
    voxelManager->fillRegion(region2, VoxelResolution::Size_1cm);
    
    // Clear only region1
    auto clearResult = voxelManager->fillRegion(region1, VoxelResolution::Size_1cm, false);
    EXPECT_TRUE(clearResult.success);
    
    // Verify region1 is empty but region2 is not
    EXPECT_TRUE(voxelManager->isRegionEmpty(region1));
    EXPECT_FALSE(voxelManager->isRegionEmpty(region2));
}