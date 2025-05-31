#pragma once

#include "RenderTypes.h"

namespace VoxelEditor {
namespace Rendering {

struct RenderConfig {
    // Window settings
    int windowWidth = 1920;
    int windowHeight = 1080;
    bool fullscreen = false;
    
    // Graphics settings
    int samples = 4;                    // MSAA samples
    bool vsync = true;
    bool debugContext = false;
    ColorFormat colorFormat = ColorFormat::RGBA8;
    DepthFormat depthFormat = DepthFormat::Depth24Stencil8;
    
    // Performance settings
    bool frustumCulling = true;
    bool occlusionCulling = false;
    int maxLights = 8;
    bool shadowMapping = false;
    
    // Quality settings
    float anisotropicFiltering = 16.0f;
    bool mipmapping = true;
    
    // Debug settings
    bool wireframeOverlay = false;
    bool showNormals = false;
    bool showBounds = false;
    bool performanceOverlay = false;
    
    RenderConfig() = default;
    
    static RenderConfig Default() {
        return RenderConfig{};
    }
    
    static RenderConfig HighQuality() {
        RenderConfig config;
        config.samples = 8;
        config.shadowMapping = true;
        config.anisotropicFiltering = 16.0f;
        config.colorFormat = ColorFormat::RGBA16F;
        return config;
    }
    
    static RenderConfig Performance() {
        RenderConfig config;
        config.samples = 0;
        config.shadowMapping = false;
        config.anisotropicFiltering = 4.0f;
        config.mipmapping = true;
        config.frustumCulling = true;
        config.occlusionCulling = true;
        return config;
    }
    
    static RenderConfig VR() {
        RenderConfig config;
        config.samples = 2;
        config.vsync = false;
        config.shadowMapping = false;
        config.anisotropicFiltering = 8.0f;
        config.frustumCulling = true;
        config.occlusionCulling = true;
        return config;
    }
    
    static RenderConfig Debug() {
        RenderConfig config;
        config.debugContext = true;
        config.wireframeOverlay = true;
        config.showNormals = true;
        config.showBounds = true;
        config.performanceOverlay = true;
        return config;
    }
    
    bool isValid() const {
        return windowWidth > 0 && windowHeight > 0 && 
               samples >= 0 && samples <= 16 &&
               maxLights > 0 && maxLights <= 32 &&
               anisotropicFiltering >= 1.0f;
    }
    
    float getAspectRatio() const {
        return static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    }
};

struct RenderSettings {
    RenderMode renderMode = RenderMode::Solid;
    bool wireframeEnabled = false;
    bool solidEnabled = true;
    Color wireframeColor = Color(0.5f, 0.5f, 0.5f, 1.0f);
    float wireframeWidth = 1.0f;
    
    // Lighting settings
    bool lightingEnabled = true;
    Math::Vector3f lightDirection = Math::Vector3f(-0.5f, -0.7f, -0.5f).normalized();
    Color lightColor = Color::White();
    float lightIntensity = 1.0f;
    Color ambientColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
    
    // Material overrides
    bool useVertexColors = false;
    Material defaultMaterial = Material::createDefault();
    
    RenderSettings() = default;
    
    static RenderSettings Wireframe() {
        RenderSettings settings;
        settings.renderMode = RenderMode::Wireframe;
        settings.wireframeEnabled = true;
        settings.solidEnabled = false;
        return settings;
    }
    
    static RenderSettings Solid() {
        RenderSettings settings;
        settings.renderMode = RenderMode::Solid;
        settings.wireframeEnabled = false;
        settings.solidEnabled = true;
        return settings;
    }
    
    static RenderSettings Combined() {
        RenderSettings settings;
        settings.renderMode = RenderMode::Combined;
        settings.wireframeEnabled = true;
        settings.solidEnabled = true;
        return settings;
    }
};

}
}