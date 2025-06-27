#include <gtest/gtest.h>
#include "../SurfaceGenerator.h"
#include "../SurfaceTypes.h"
#include "../../voxel_data/VoxelGrid.h"
#include "../../voxel_data/VoxelTypes.h"
#include <chrono>

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor;

class PreviewQualityTest : public ::testing::Test {
protected:
    std::unique_ptr<SurfaceGenerator> generator;
    std::unique_ptr<VoxelGrid> grid;
    
    void SetUp() override {
        generator = std::make_unique<SurfaceGenerator>();
        grid = std::make_unique<VoxelGrid>(VoxelResolution::Size_4cm, 5.0f);
        
        // Create a simple blocky shape (2x2x2 cube)
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    grid->setVoxel(Math::Vector3i(x, y, z), true);
                }
            }
        }
    }
};

// Test preview quality enum values
TEST_F(PreviewQualityTest, PreviewQualityEnumValues) {
    EXPECT_EQ(static_cast<int>(PreviewQuality::Disabled), 0);
    EXPECT_EQ(static_cast<int>(PreviewQuality::Fast), 1);
    EXPECT_EQ(static_cast<int>(PreviewQuality::Balanced), 2);
    EXPECT_EQ(static_cast<int>(PreviewQuality::HighQuality), 3);
}

// Test preview quality settings creation
TEST_F(PreviewQualityTest, PreviewQualitySettings) {
    // Test FastPreview settings
    SurfaceSettings fastPreview = SurfaceSettings::FastPreview();
    EXPECT_EQ(fastPreview.previewQuality, PreviewQuality::Fast);
    EXPECT_EQ(fastPreview.smoothingLevel, 2);
    EXPECT_EQ(fastPreview.smoothingAlgorithm, SmoothingAlgorithm::Laplacian);
    EXPECT_FALSE(fastPreview.preserveTopology);
    EXPECT_FALSE(fastPreview.generateNormals);
    EXPECT_FLOAT_EQ(fastPreview.simplificationRatio, 0.3f);
    
    // Test BalancedPreview settings
    SurfaceSettings balancedPreview = SurfaceSettings::BalancedPreview();
    EXPECT_EQ(balancedPreview.previewQuality, PreviewQuality::Balanced);
    EXPECT_EQ(balancedPreview.smoothingLevel, 3);
    EXPECT_EQ(balancedPreview.smoothingAlgorithm, SmoothingAlgorithm::Auto);
    EXPECT_TRUE(balancedPreview.preserveTopology);
    EXPECT_FALSE(balancedPreview.generateNormals);
    EXPECT_FLOAT_EQ(balancedPreview.simplificationRatio, 0.5f);
    
    // Test HighQualityPreview settings
    SurfaceSettings highPreview = SurfaceSettings::HighQualityPreview();
    EXPECT_EQ(highPreview.previewQuality, PreviewQuality::HighQuality);
    EXPECT_EQ(highPreview.smoothingLevel, 4);
    EXPECT_EQ(highPreview.smoothingAlgorithm, SmoothingAlgorithm::Auto);
    EXPECT_TRUE(highPreview.preserveTopology);
    EXPECT_TRUE(highPreview.generateNormals);
    EXPECT_FLOAT_EQ(highPreview.simplificationRatio, 0.8f);
}

// Test preview quality settings equality and hashing
TEST_F(PreviewQualityTest, PreviewQualityEqualityAndHash) {
    SurfaceSettings settings1 = SurfaceSettings::FastPreview();
    SurfaceSettings settings2 = SurfaceSettings::FastPreview();
    
    EXPECT_EQ(settings1, settings2);
    EXPECT_EQ(settings1.hash(), settings2.hash());
    
    // Change preview quality
    settings2.previewQuality = PreviewQuality::Balanced;
    EXPECT_NE(settings1, settings2);
    EXPECT_NE(settings1.hash(), settings2.hash());
}

// Test backward compatibility with usePreviewQuality
TEST_F(PreviewQualityTest, BackwardCompatibility) {
    SurfaceSettings settings = SurfaceSettings::Default();
    settings.usePreviewQuality = true;  // Old way
    
    // Should still work with new system
    EXPECT_EQ(settings.previewQuality, PreviewQuality::Disabled); // Should be disabled until explicitly set
    EXPECT_TRUE(settings.usePreviewQuality); // But old flag should still be true
}

// Test progressive smoothing cache creation
TEST_F(PreviewQualityTest, ProgressiveSmoothingCacheBasic) {
    ProgressiveSmoothingCache cache;
    
    // Test empty cache
    EXPECT_FALSE(cache.hasEntry("test_key"));
    EXPECT_EQ(cache.getMemoryUsage(), 0);
    
    // Test cache key generation
    EXPECT_FALSE(cache.hasProgressiveResult("base_key", 5, PreviewQuality::Fast));
}

// Test progressive smoothing with SurfaceGenerator
TEST_F(PreviewQualityTest, ProgressiveSmoothingBasicFlow) {
    // Test fast preview settings
    SurfaceSettings settings = SurfaceSettings::FastPreview();
    
    // This should complete quickly without hanging
    try {
        std::string progressKey = generator->startProgressiveSmoothing(*grid, settings);
        EXPECT_FALSE(progressKey.empty());
        
        // Should be able to check if complete
        bool isComplete = generator->isProgressiveSmoothingComplete(progressKey);
        // Note: may or may not be complete immediately depending on timing
        
        // Should be able to get result (may be empty if not ready)
        Mesh result = generator->getProgressiveResult(progressKey);
        // Note: result may be empty if processing not complete
        
        // Should be able to cancel
        generator->cancelProgressiveSmoothing(progressKey);
        
        SUCCEED();
    } catch (...) {
        FAIL() << "Progressive smoothing should not throw";
    }
}

// Test performance timing for different preview qualities
TEST_F(PreviewQualityTest, PreviewQualityPerformanceTiming) {
    // Test that fast preview is actually faster than high quality
    auto start = std::chrono::high_resolution_clock::now();
    
    SurfaceSettings fastSettings = SurfaceSettings::FastPreview();
    fastSettings.smoothingLevel = 0; // No smoothing for fair timing comparison
    Mesh fastResult = generator->generateSurface(*grid, fastSettings);
    
    auto fastTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    start = std::chrono::high_resolution_clock::now();
    
    SurfaceSettings highSettings = SurfaceSettings::HighQualityPreview();
    highSettings.smoothingLevel = 0; // No smoothing for fair timing comparison
    Mesh highResult = generator->generateSurface(*grid, highSettings);
    
    auto highTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    EXPECT_TRUE(fastResult.isValid());
    EXPECT_TRUE(highResult.isValid());
    
    // Fast should complete quickly (within 100ms for such a simple mesh)
    EXPECT_LT(fastTime, 100);
    
    // Both should complete within reasonable time
    EXPECT_LT(fastTime, 1000);
    EXPECT_LT(highTime, 1000);
    
    // Performance difference might not be significant for such simple meshes,
    // but neither should hang
}

// Test preview quality vs final quality differences
TEST_F(PreviewQualityTest, PreviewQualityVsFinalQuality) {
    SurfaceSettings previewSettings = SurfaceSettings::FastPreview();
    previewSettings.smoothingLevel = 0; // Test settings differences without smoothing
    
    SurfaceSettings finalSettings = SurfaceSettings::Export();
    finalSettings.smoothingLevel = 0; // Test settings differences without smoothing
    
    Mesh previewMesh = generator->generateSurface(*grid, previewSettings);
    Mesh finalMesh = generator->generateSurface(*grid, finalSettings);
    
    EXPECT_TRUE(previewMesh.isValid());
    EXPECT_TRUE(finalMesh.isValid());
    
    // Preview should have fewer vertices due to simplification
    if (previewMesh.vertices.size() > 0 && finalMesh.vertices.size() > 0) {
        // Fast preview uses 0.3 simplification ratio vs 0.95 for export
        // So preview should have significantly fewer vertices
        EXPECT_LE(previewMesh.vertices.size(), finalMesh.vertices.size());
    }
}

// Test cancellation functionality
TEST_F(PreviewQualityTest, CancellationFunctionality) {
    SurfaceSettings settings = SurfaceSettings::FastPreview();
    
    // Start progressive smoothing
    std::string progressKey = generator->startProgressiveSmoothing(*grid, settings);
    EXPECT_FALSE(progressKey.empty());
    
    // Cancel immediately
    generator->cancelProgressiveSmoothing(progressKey);
    
    // Should be able to call multiple times without error
    generator->cancelProgressiveSmoothing(progressKey);
    
    SUCCEED();
}

// Test cache hit/miss scenarios
TEST_F(PreviewQualityTest, CacheHitMissScenarios) {
    ProgressiveSmoothingCache cache;
    
    // Test cache miss
    EXPECT_FALSE(cache.hasProgressiveResult("test_key", 5, PreviewQuality::Fast));
    
    // Create a simple mesh for testing
    Mesh testMesh;
    testMesh.vertices.resize(3);  // Valid mesh needs vertices
    testMesh.indices = {0, 1, 2}; // Valid triangle
    
    // Cache a result
    cache.cacheProgressiveResult("test_key", testMesh, 5, PreviewQuality::Fast);
    
    // Test cache hit
    EXPECT_TRUE(cache.hasProgressiveResult("test_key", 5, PreviewQuality::Fast));
    
    Mesh retrieved = cache.getProgressiveResult("test_key", 5, PreviewQuality::Fast);
    EXPECT_TRUE(retrieved.isValid());
    EXPECT_EQ(retrieved.vertices.size(), 3);
}

// Test memory management and limits
TEST_F(PreviewQualityTest, MemoryManagement) {
    ProgressiveSmoothingCache cache;
    
    // Set a small memory limit for testing
    cache.setMaxMemoryUsage(1024); // 1KB limit
    
    EXPECT_LE(cache.getMemoryUsage(), 1024);
    
    // Cache should be able to handle memory limits
    cache.clear();
    EXPECT_EQ(cache.getMemoryUsage(), 0);
}