#include <gtest/gtest.h>
#include "../SurfaceGenerator.h"
#include "../MeshSmoother.h"
#include "../MeshValidator.h"
#include "../../voxel_data/VoxelGrid.h"
#include "../../voxel_data/VoxelTypes.h"

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor;

// Test integration without expensive mesh generation
class MockSmoothingIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

// Test that all smoothing components can be created
TEST_F(MockSmoothingIntegrationTest, SmoothingComponentsCreation) {
    // Test MeshSmoother can be created
    try {
        MeshSmoother smoother;
        SUCCEED();
    } catch (...) {
        FAIL() << "MeshSmoother creation should not throw";
    }
    
    // Test MeshValidator can be created
    try {
        MeshValidator validator;
        SUCCEED();
    } catch (...) {
        FAIL() << "MeshValidator creation should not throw";
    }
    
    // Test SurfaceGenerator can be created
    try {
        SurfaceGenerator generator;
        SUCCEED();
    } catch (...) {
        FAIL() << "SurfaceGenerator creation should not throw";
    }
}

// Test smoothing configuration structures
TEST_F(MockSmoothingIntegrationTest, SmoothingConfigurationStructures) {
    // Test MeshSmoother::SmoothingConfig
    MeshSmoother::SmoothingConfig config;
    config.smoothingLevel = 5;
    config.algorithm = MeshSmoother::Algorithm::Taubin;
    config.preserveTopology = true;
    config.preserveBoundaries = true;
    config.minFeatureSize = 1.0f;
    config.usePreviewQuality = false;
    
    EXPECT_EQ(config.smoothingLevel, 5);
    EXPECT_EQ(config.algorithm, MeshSmoother::Algorithm::Taubin);
    EXPECT_TRUE(config.preserveTopology);
    EXPECT_TRUE(config.preserveBoundaries);
    EXPECT_FLOAT_EQ(config.minFeatureSize, 1.0f);
    EXPECT_FALSE(config.usePreviewQuality);
}

// Test SurfaceSettings integration
TEST_F(MockSmoothingIntegrationTest, SurfaceSettingsIntegration) {
    // Test default settings
    SurfaceSettings defaultSettings = SurfaceSettings::Default();
    EXPECT_EQ(defaultSettings.smoothingLevel, 0);
    EXPECT_EQ(defaultSettings.smoothingAlgorithm, SmoothingAlgorithm::Auto);
    EXPECT_TRUE(defaultSettings.preserveTopology);
    EXPECT_FLOAT_EQ(defaultSettings.minFeatureSize, 1.0f);
    EXPECT_FALSE(defaultSettings.usePreviewQuality);
    
    // Test preview settings
    SurfaceSettings previewSettings = SurfaceSettings::Preview();
    EXPECT_EQ(previewSettings.smoothingLevel, 3);
    EXPECT_EQ(previewSettings.smoothingAlgorithm, SmoothingAlgorithm::Auto);
    EXPECT_TRUE(previewSettings.preserveTopology);
    EXPECT_FLOAT_EQ(previewSettings.minFeatureSize, 2.0f);
    EXPECT_TRUE(previewSettings.usePreviewQuality);
    
    // Test export settings
    SurfaceSettings exportSettings = SurfaceSettings::Export();
    EXPECT_EQ(exportSettings.smoothingLevel, 5);
    EXPECT_EQ(exportSettings.smoothingAlgorithm, SmoothingAlgorithm::Auto);
    EXPECT_TRUE(exportSettings.preserveTopology);
    EXPECT_FLOAT_EQ(exportSettings.minFeatureSize, 1.0f);
    EXPECT_FALSE(exportSettings.usePreviewQuality);
}

// Test enum mappings
TEST_F(MockSmoothingIntegrationTest, EnumMappings) {
    // Test SmoothingAlgorithm enum values
    EXPECT_EQ(static_cast<int>(SmoothingAlgorithm::Auto), 0);
    EXPECT_EQ(static_cast<int>(SmoothingAlgorithm::Laplacian), 1);
    EXPECT_EQ(static_cast<int>(SmoothingAlgorithm::Taubin), 2);
    EXPECT_EQ(static_cast<int>(SmoothingAlgorithm::BiLaplacian), 3);
    
    // Test MeshSmoother::Algorithm enum values
    EXPECT_EQ(static_cast<int>(MeshSmoother::Algorithm::None), 0);
    EXPECT_EQ(static_cast<int>(MeshSmoother::Algorithm::Laplacian), 1);
    EXPECT_EQ(static_cast<int>(MeshSmoother::Algorithm::Taubin), 2);
    EXPECT_EQ(static_cast<int>(MeshSmoother::Algorithm::BiLaplacian), 3);
}

// Test VoxelGrid creation (without expensive operations)
TEST_F(MockSmoothingIntegrationTest, VoxelGridCreation) {
    try {
        VoxelGrid grid(VoxelResolution::Size_4cm, 5.0f);
        
        // Basic operations should work - empty grid is expected initially
        // Grid starts empty until voxels are added
        
        Math::Vector3i dims = grid.getGridDimensions();
        EXPECT_GT(dims.x, 0);
        EXPECT_GT(dims.y, 0);
        EXPECT_GT(dims.z, 0);
        
        SUCCEED();
    } catch (...) {
        FAIL() << "VoxelGrid creation should not throw";
    }
}

// Test progress callback types
TEST_F(MockSmoothingIntegrationTest, ProgressCallbackTypes) {
    SurfaceGenerator generator;
    
    bool callbackSet = false;
    generator.setProgressCallback([&](float progress, const std::string& status) {
        callbackSet = true;
        EXPECT_GE(progress, 0.0f);
        EXPECT_LE(progress, 1.0f);
        EXPECT_FALSE(status.empty());
    });
    
    // Just test that callback can be set without error
    SUCCEED();
}

// Test cancellation functionality
TEST_F(MockSmoothingIntegrationTest, CancellationFunctionality) {
    SurfaceGenerator generator;
    
    EXPECT_FALSE(generator.isCancelled());
    
    generator.cancelGeneration();
    
    EXPECT_TRUE(generator.isCancelled());
}

// Test settings equality and hashing
TEST_F(MockSmoothingIntegrationTest, SettingsEqualityAndHashing) {
    SurfaceSettings settings1 = SurfaceSettings::Default();
    SurfaceSettings settings2 = SurfaceSettings::Default();
    
    // Same settings should be equal
    EXPECT_EQ(settings1, settings2);
    EXPECT_EQ(settings1.hash(), settings2.hash());
    
    // Different smoothing levels should not be equal
    settings2.smoothingLevel = 5;
    EXPECT_NE(settings1, settings2);
    EXPECT_NE(settings1.hash(), settings2.hash());
    
    // Different algorithms should not be equal
    settings2 = settings1;
    settings2.smoothingAlgorithm = SmoothingAlgorithm::Taubin;
    EXPECT_NE(settings1, settings2);
    EXPECT_NE(settings1.hash(), settings2.hash());
    
    // Different topology preservation should not be equal
    settings2 = settings1;
    settings2.preserveTopology = false;
    EXPECT_NE(settings1, settings2);
    EXPECT_NE(settings1.hash(), settings2.hash());
    
    // Different feature sizes should not be equal
    settings2 = settings1;
    settings2.minFeatureSize = 2.0f;
    EXPECT_NE(settings1, settings2);
    EXPECT_NE(settings1.hash(), settings2.hash());
    
    // Different preview quality should not be equal
    settings2 = settings1;
    settings2.usePreviewQuality = true;
    EXPECT_NE(settings1, settings2);
    EXPECT_NE(settings1.hash(), settings2.hash());
}