#include <gtest/gtest.h>
#include "../RenderConfig.h"

using namespace VoxelEditor::Rendering;
using namespace VoxelEditor::Math;

class RenderConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RenderConfigTest, DefaultConfiguration) {
    RenderConfig config;
    
    // Window settings
    EXPECT_EQ(config.windowWidth, 1920);
    EXPECT_EQ(config.windowHeight, 1080);
    EXPECT_FALSE(config.fullscreen);
    
    // Graphics settings
    EXPECT_EQ(config.samples, 4);
    EXPECT_TRUE(config.vsync);
    EXPECT_FALSE(config.debugContext);
    EXPECT_EQ(config.colorFormat, ColorFormat::RGBA8);
    EXPECT_EQ(config.depthFormat, DepthFormat::Depth24Stencil8);
    
    // Performance settings
    EXPECT_TRUE(config.frustumCulling);
    EXPECT_FALSE(config.occlusionCulling);
    EXPECT_EQ(config.maxLights, 8);
    EXPECT_FALSE(config.shadowMapping);
    
    // Quality settings
    EXPECT_FLOAT_EQ(config.anisotropicFiltering, 16.0f);
    EXPECT_TRUE(config.mipmapping);
    
    // Debug settings
    EXPECT_FALSE(config.wireframeOverlay);
    EXPECT_FALSE(config.showNormals);
    EXPECT_FALSE(config.showBounds);
    EXPECT_FALSE(config.performanceOverlay);
    
    // Validation
    EXPECT_TRUE(config.isValid());
    EXPECT_FLOAT_EQ(config.getAspectRatio(), 1920.0f / 1080.0f);
}

TEST_F(RenderConfigTest, PresetConfigurations) {
    // Default preset
    RenderConfig defaultConfig = RenderConfig::Default();
    EXPECT_EQ(defaultConfig.samples, 4);
    EXPECT_TRUE(defaultConfig.vsync);
    EXPECT_FALSE(defaultConfig.shadowMapping);
    EXPECT_TRUE(defaultConfig.isValid());
    
    // High quality preset
    RenderConfig highQuality = RenderConfig::HighQuality();
    EXPECT_EQ(highQuality.samples, 8);
    EXPECT_TRUE(highQuality.shadowMapping);
    EXPECT_FLOAT_EQ(highQuality.anisotropicFiltering, 16.0f);
    EXPECT_EQ(highQuality.colorFormat, ColorFormat::RGBA16F);
    EXPECT_TRUE(highQuality.isValid());
    
    // Performance preset
    RenderConfig performance = RenderConfig::Performance();
    EXPECT_EQ(performance.samples, 0);
    EXPECT_FALSE(performance.shadowMapping);
    EXPECT_FLOAT_EQ(performance.anisotropicFiltering, 4.0f);
    EXPECT_TRUE(performance.mipmapping);
    EXPECT_TRUE(performance.frustumCulling);
    EXPECT_TRUE(performance.occlusionCulling);
    EXPECT_TRUE(performance.isValid());
    
    // VR preset
    RenderConfig vr = RenderConfig::VR();
    EXPECT_EQ(vr.samples, 2);
    EXPECT_FALSE(vr.vsync);
    EXPECT_FALSE(vr.shadowMapping);
    EXPECT_FLOAT_EQ(vr.anisotropicFiltering, 8.0f);
    EXPECT_TRUE(vr.frustumCulling);
    EXPECT_TRUE(vr.occlusionCulling);
    EXPECT_TRUE(vr.isValid());
    
    // Debug preset
    RenderConfig debug = RenderConfig::Debug();
    EXPECT_TRUE(debug.debugContext);
    EXPECT_TRUE(debug.wireframeOverlay);
    EXPECT_TRUE(debug.showNormals);
    EXPECT_TRUE(debug.showBounds);
    EXPECT_TRUE(debug.performanceOverlay);
    EXPECT_TRUE(debug.isValid());
}

TEST_F(RenderConfigTest, ValidationTests) {
    RenderConfig config;
    
    // Valid configuration
    EXPECT_TRUE(config.isValid());
    
    // Invalid window dimensions
    config.windowWidth = 0;
    EXPECT_FALSE(config.isValid());
    config.windowWidth = 1920;
    EXPECT_TRUE(config.isValid());
    
    config.windowHeight = -100;
    EXPECT_FALSE(config.isValid());
    config.windowHeight = 1080;
    EXPECT_TRUE(config.isValid());
    
    // Invalid MSAA samples
    config.samples = -1;
    EXPECT_FALSE(config.isValid());
    config.samples = 0;
    EXPECT_TRUE(config.isValid());
    
    config.samples = 20; // Too many samples
    EXPECT_FALSE(config.isValid());
    config.samples = 4;
    EXPECT_TRUE(config.isValid());
    
    // Invalid max lights
    config.maxLights = 0;
    EXPECT_FALSE(config.isValid());
    config.maxLights = 8;
    EXPECT_TRUE(config.isValid());
    
    config.maxLights = 50; // Too many lights
    EXPECT_FALSE(config.isValid());
    config.maxLights = 8;
    EXPECT_TRUE(config.isValid());
    
    // Invalid anisotropic filtering
    config.anisotropicFiltering = 0.5f;
    EXPECT_FALSE(config.isValid());
    config.anisotropicFiltering = 16.0f;
    EXPECT_TRUE(config.isValid());
}

TEST_F(RenderConfigTest, AspectRatioCalculation) {
    RenderConfig config;
    
    // Standard 16:9
    config.windowWidth = 1920;
    config.windowHeight = 1080;
    EXPECT_FLOAT_EQ(config.getAspectRatio(), 16.0f / 9.0f);
    
    // 4:3
    config.windowWidth = 1024;
    config.windowHeight = 768;
    EXPECT_FLOAT_EQ(config.getAspectRatio(), 4.0f / 3.0f);
    
    // Square
    config.windowWidth = 800;
    config.windowHeight = 800;
    EXPECT_FLOAT_EQ(config.getAspectRatio(), 1.0f);
    
    // Ultra-wide
    config.windowWidth = 3440;
    config.windowHeight = 1440;
    EXPECT_FLOAT_EQ(config.getAspectRatio(), 3440.0f / 1440.0f);
}

class RenderSettingsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RenderSettingsTest, DefaultSettings) {
    RenderSettings settings;
    
    EXPECT_EQ(settings.renderMode, RenderMode::Solid);
    EXPECT_FALSE(settings.wireframeEnabled);
    EXPECT_TRUE(settings.solidEnabled);
    EXPECT_FLOAT_EQ(settings.wireframeColor.r, 0.5f);
    EXPECT_FLOAT_EQ(settings.wireframeColor.g, 0.5f);
    EXPECT_FLOAT_EQ(settings.wireframeColor.b, 0.5f);
    EXPECT_FLOAT_EQ(settings.wireframeColor.a, 1.0f);
    EXPECT_FLOAT_EQ(settings.wireframeWidth, 1.0f);
    
    // Lighting settings
    EXPECT_TRUE(settings.enableLighting);
    Vector3f expectedLightDir = Vector3f(-0.5f, -0.7f, -0.5f).normalized();
    EXPECT_NEAR(settings.lightDirection.x, expectedLightDir.x, 0.001f);
    EXPECT_NEAR(settings.lightDirection.y, expectedLightDir.y, 0.001f);
    EXPECT_NEAR(settings.lightDirection.z, expectedLightDir.z, 0.001f);
    EXPECT_FLOAT_EQ(settings.lightColor.r, 1.0f);
    EXPECT_FLOAT_EQ(settings.lightColor.g, 1.0f);
    EXPECT_FLOAT_EQ(settings.lightColor.b, 1.0f);
    EXPECT_FLOAT_EQ(settings.lightIntensity, 1.0f);
    EXPECT_FLOAT_EQ(settings.ambientLight.r, 0.2f);
    EXPECT_FLOAT_EQ(settings.ambientLight.g, 0.2f);
    EXPECT_FLOAT_EQ(settings.ambientLight.b, 0.2f);
    
    // Material settings
    EXPECT_FALSE(settings.useVertexColors);
}

TEST_F(RenderSettingsTest, PresetSettings) {
    // Wireframe preset
    RenderSettings wireframe = RenderSettings::Wireframe();
    EXPECT_EQ(wireframe.renderMode, RenderMode::Wireframe);
    EXPECT_TRUE(wireframe.wireframeEnabled);
    EXPECT_FALSE(wireframe.solidEnabled);
    
    // Solid preset
    RenderSettings solid = RenderSettings::Solid();
    EXPECT_EQ(solid.renderMode, RenderMode::Solid);
    EXPECT_FALSE(solid.wireframeEnabled);
    EXPECT_TRUE(solid.solidEnabled);
    
    // Combined preset
    RenderSettings combined = RenderSettings::Combined();
    EXPECT_EQ(combined.renderMode, RenderMode::Combined);
    EXPECT_TRUE(combined.wireframeEnabled);
    EXPECT_TRUE(combined.solidEnabled);
}