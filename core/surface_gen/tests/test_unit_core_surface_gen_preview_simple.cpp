#include <gtest/gtest.h>
#include "../SurfaceTypes.h"

using namespace VoxelEditor::SurfaceGen;

class PreviewQualitySimpleTest : public ::testing::Test {
protected:
};

// Test preview quality enum values
TEST_F(PreviewQualitySimpleTest, PreviewQualityEnumValues) {
    EXPECT_EQ(static_cast<int>(PreviewQuality::Disabled), 0);
    EXPECT_EQ(static_cast<int>(PreviewQuality::Fast), 1);
    EXPECT_EQ(static_cast<int>(PreviewQuality::Balanced), 2);
    EXPECT_EQ(static_cast<int>(PreviewQuality::HighQuality), 3);
}

// Test preview quality settings creation
TEST_F(PreviewQualitySimpleTest, PreviewQualitySettings) {
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
TEST_F(PreviewQualitySimpleTest, PreviewQualityEqualityAndHash) {
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
TEST_F(PreviewQualitySimpleTest, BackwardCompatibility) {
    SurfaceSettings settings = SurfaceSettings::Default();
    settings.usePreviewQuality = true;  // Old way
    
    // Should still work with new system
    EXPECT_EQ(settings.previewQuality, PreviewQuality::Disabled); // Should be disabled until explicitly set
    EXPECT_TRUE(settings.usePreviewQuality); // But old flag should still be true
}

// Test that different preview qualities produce different settings
TEST_F(PreviewQualitySimpleTest, DifferentPreviewQualitiesProduceDifferentSettings) {
    SurfaceSettings fast = SurfaceSettings::FastPreview();
    SurfaceSettings balanced = SurfaceSettings::BalancedPreview();
    SurfaceSettings high = SurfaceSettings::HighQualityPreview();
    
    // They should be different
    EXPECT_NE(fast, balanced);
    EXPECT_NE(balanced, high);
    EXPECT_NE(fast, high);
    
    // They should have different hashes
    EXPECT_NE(fast.hash(), balanced.hash());
    EXPECT_NE(balanced.hash(), high.hash());
    EXPECT_NE(fast.hash(), high.hash());
    
    // Fast should be most aggressive
    EXPECT_LT(fast.simplificationRatio, balanced.simplificationRatio);
    EXPECT_LT(balanced.simplificationRatio, high.simplificationRatio);
    
    // Fast should disable features for performance
    EXPECT_FALSE(fast.generateNormals);
    EXPECT_FALSE(fast.preserveTopology);
    
    // High should enable features for quality
    EXPECT_TRUE(high.generateNormals);
    EXPECT_TRUE(high.preserveTopology);
}

// Test settings validation
TEST_F(PreviewQualitySimpleTest, SettingsValidation) {
    // All preview quality settings should be valid
    SurfaceSettings fast = SurfaceSettings::FastPreview();
    SurfaceSettings balanced = SurfaceSettings::BalancedPreview();
    SurfaceSettings high = SurfaceSettings::HighQualityPreview();
    
    // Basic validation checks
    EXPECT_GE(fast.smoothingLevel, 0);
    EXPECT_GE(balanced.smoothingLevel, 0);
    EXPECT_GE(high.smoothingLevel, 0);
    
    EXPECT_GT(fast.minFeatureSize, 0.0f);
    EXPECT_GT(balanced.minFeatureSize, 0.0f);
    EXPECT_GT(high.minFeatureSize, 0.0f);
    
    EXPECT_GE(fast.simplificationRatio, 0.0f);
    EXPECT_LE(fast.simplificationRatio, 1.0f);
    
    EXPECT_GE(balanced.simplificationRatio, 0.0f);
    EXPECT_LE(balanced.simplificationRatio, 1.0f);
    
    EXPECT_GE(high.simplificationRatio, 0.0f);
    EXPECT_LE(high.simplificationRatio, 1.0f);
}