#include <gtest/gtest.h>
#include "../SurfaceTypes.h"

using namespace VoxelEditor::SurfaceGen;

class SurfaceSettingsTest : public ::testing::Test {
protected:
};

// Test new smoothing fields
TEST_F(SurfaceSettingsTest, SmoothingFieldsDefault) {
    SurfaceSettings settings = SurfaceSettings::Default();
    
    EXPECT_EQ(settings.smoothingLevel, 0);
    EXPECT_EQ(settings.smoothingAlgorithm, SmoothingAlgorithm::Auto);
    EXPECT_TRUE(settings.preserveTopology);
    EXPECT_FLOAT_EQ(settings.minFeatureSize, 1.0f);
    EXPECT_FALSE(settings.usePreviewQuality);
}

// Test preview settings
TEST_F(SurfaceSettingsTest, SmoothingFieldsPreview) {
    SurfaceSettings settings = SurfaceSettings::Preview();
    
    EXPECT_EQ(settings.smoothingLevel, 3);
    EXPECT_EQ(settings.smoothingAlgorithm, SmoothingAlgorithm::Auto);
    EXPECT_TRUE(settings.preserveTopology);
    EXPECT_FLOAT_EQ(settings.minFeatureSize, 2.0f);
    EXPECT_TRUE(settings.usePreviewQuality);
}

// Test export settings
TEST_F(SurfaceSettingsTest, SmoothingFieldsExport) {
    SurfaceSettings settings = SurfaceSettings::Export();
    
    EXPECT_EQ(settings.smoothingLevel, 5);
    EXPECT_EQ(settings.smoothingAlgorithm, SmoothingAlgorithm::Auto);
    EXPECT_TRUE(settings.preserveTopology);
    EXPECT_FLOAT_EQ(settings.minFeatureSize, 1.0f);
    EXPECT_FALSE(settings.usePreviewQuality);
}

// Test equality operator with new fields
TEST_F(SurfaceSettingsTest, EqualityWithSmoothingFields) {
    SurfaceSettings settings1 = SurfaceSettings::Default();
    SurfaceSettings settings2 = SurfaceSettings::Default();
    
    EXPECT_EQ(settings1, settings2);
    
    // Change smoothing level
    settings2.smoothingLevel = 5;
    EXPECT_NE(settings1, settings2);
    
    // Reset and change algorithm
    settings2 = settings1;
    settings2.smoothingAlgorithm = SmoothingAlgorithm::Taubin;
    EXPECT_NE(settings1, settings2);
    
    // Reset and change topology preservation
    settings2 = settings1;
    settings2.preserveTopology = false;
    EXPECT_NE(settings1, settings2);
    
    // Reset and change min feature size
    settings2 = settings1;
    settings2.minFeatureSize = 2.0f;
    EXPECT_NE(settings1, settings2);
    
    // Reset and change preview quality
    settings2 = settings1;
    settings2.usePreviewQuality = true;
    EXPECT_NE(settings1, settings2);
}

// Test hash function includes new fields
TEST_F(SurfaceSettingsTest, HashIncludesSmoothingFields) {
    SurfaceSettings settings1 = SurfaceSettings::Default();
    SurfaceSettings settings2 = SurfaceSettings::Default();
    
    size_t hash1 = settings1.hash();
    size_t hash2 = settings2.hash();
    
    EXPECT_EQ(hash1, hash2);
    
    // Change smoothing level should change hash
    settings2.smoothingLevel = 5;
    size_t hash3 = settings2.hash();
    EXPECT_NE(hash1, hash3);
    
    // Different settings should likely have different hashes
    SurfaceSettings preview = SurfaceSettings::Preview();
    SurfaceSettings exportSettings = SurfaceSettings::Export();
    
    EXPECT_NE(preview.hash(), exportSettings.hash());
}