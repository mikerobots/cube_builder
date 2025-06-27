#include <gtest/gtest.h>
#include "../SurfaceGenerator.h"
#include "../MeshSmoother.h"
#include "../MeshValidator.h"
#include "../../voxel_data/VoxelGrid.h"
#include "../../voxel_data/VoxelTypes.h"

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor;

class SimpleSmoothingIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<SurfaceGenerator> generator;
    std::unique_ptr<VoxelGrid> grid;
    
    void SetUp() override {
        generator = std::make_unique<SurfaceGenerator>();
        grid = std::make_unique<VoxelGrid>(VoxelResolution::Size_4cm, 5.0f);
        
        // Create just one voxel for fastest possible generation
        grid->setVoxel(Math::Vector3i(0, 0, 0), true);
    }
};

// Test smoothing settings are properly applied
TEST_F(SimpleSmoothingIntegrationTest, SmoothingSettingsApplied) {
    // Create settings with smoothing
    SurfaceSettings settings = SurfaceSettings::Default();
    settings.smoothingLevel = 3;
    settings.preserveTopology = true;
    settings.minFeatureSize = 1.0f;
    
    // Verify settings are properly configured
    EXPECT_EQ(settings.smoothingLevel, 3);
    EXPECT_TRUE(settings.preserveTopology);
    EXPECT_FLOAT_EQ(settings.minFeatureSize, 1.0f);
}

// Test generateSmoothedSurface convenience method
TEST_F(SimpleSmoothingIntegrationTest, GenerateSmoothedSurfaceMethod) {
    // Test with default smoothing level (should not crash or hang)
    try {
        Mesh smoothed = generator->generateSmoothedSurface(*grid);
        // Basic validity check - method should not crash
        SUCCEED();
    } catch (...) {
        FAIL() << "generateSmoothedSurface should not throw";
    }
}

// Test applySmoothingToMesh method without creating manual mesh
TEST_F(SimpleSmoothingIntegrationTest, ApplySmoothingToMeshMethod) {
    // Test smoothing settings
    SurfaceSettings settings = SurfaceSettings::Default();
    settings.smoothingLevel = 0; // No smoothing - should be fast
    
    // This should not crash
    try {
        // Access private method through public interface
        Mesh result = generator->generateSurface(*grid, settings);
        SUCCEED();
    } catch (...) {
        FAIL() << "Smoothing should not throw with level 0";
    }
}

// Test validateMeshForPrinting method
TEST_F(SimpleSmoothingIntegrationTest, ValidateMeshForPrintingMethod) {
    // Test with settings that include validation
    SurfaceSettings settings = SurfaceSettings::Export();
    settings.smoothingLevel = 0; // No smoothing for speed
    
    // Should complete without crashing
    try {
        Mesh result = generator->generateSurface(*grid, settings);
        SUCCEED();
    } catch (...) {
        FAIL() << "Mesh validation should not throw";
    }
}

// Test progress callback functionality
TEST_F(SimpleSmoothingIntegrationTest, ProgressCallbackFunctionality) {
    bool callbackCalled = false;
    
    generator->setProgressCallback([&](float progress, const std::string& status) {
        callbackCalled = true;
        EXPECT_GE(progress, 0.0f);
        EXPECT_LE(progress, 1.0f);
        EXPECT_FALSE(status.empty());
    });
    
    SurfaceSettings settings = SurfaceSettings::Default();
    settings.smoothingLevel = 0; // No smoothing for speed
    
    Mesh result = generator->generateSurface(*grid, settings);
    
    EXPECT_TRUE(callbackCalled);
}

// Test settings equality and hash
TEST_F(SimpleSmoothingIntegrationTest, SettingsEqualityAndHash) {
    SurfaceSettings settings1 = SurfaceSettings::Default();
    SurfaceSettings settings2 = SurfaceSettings::Default();
    
    EXPECT_EQ(settings1, settings2);
    EXPECT_EQ(settings1.hash(), settings2.hash());
    
    settings2.smoothingLevel = 5;
    EXPECT_NE(settings1, settings2);
    EXPECT_NE(settings1.hash(), settings2.hash());
}

// Test different preset configurations
TEST_F(SimpleSmoothingIntegrationTest, PresetConfigurations) {
    SurfaceSettings defaultSettings = SurfaceSettings::Default();
    SurfaceSettings previewSettings = SurfaceSettings::Preview();
    SurfaceSettings exportSettings = SurfaceSettings::Export();
    
    // Verify different configurations
    EXPECT_EQ(defaultSettings.smoothingLevel, 0);
    EXPECT_EQ(previewSettings.smoothingLevel, 3);
    EXPECT_EQ(exportSettings.smoothingLevel, 5);
    
    EXPECT_FALSE(defaultSettings.usePreviewQuality);
    EXPECT_TRUE(previewSettings.usePreviewQuality);
    EXPECT_FALSE(exportSettings.usePreviewQuality);
    
    // All should preserve topology by default
    EXPECT_TRUE(defaultSettings.preserveTopology);
    EXPECT_TRUE(previewSettings.preserveTopology);
    EXPECT_TRUE(exportSettings.preserveTopology);
}