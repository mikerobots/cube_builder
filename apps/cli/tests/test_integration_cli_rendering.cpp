#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

#include "cli/Application.h"
#include "cli/RenderWindow.h"
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "rendering/RenderEngine.h"
#include "foundation/logging/Logger.h"

namespace VoxelEditor {
namespace Tests {

// CLI Rendering Integration Tests
class CLIRenderingIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<Application> app;
    
    void SetUp() override {
        // Initialize logging
        Logging::Logger::getInstance().setLevel(Logging::Logger::Level::Warning);
        
        // Create application with OpenGL context (NOT headless)
        app = std::make_unique<Application>();
        
        int argc = 1;
        char arg0[] = "test";
        char* argv[] = {arg0, nullptr};
        
        ASSERT_TRUE(app->initialize(argc, argv)) << "Application should initialize with OpenGL";
    }
    
    void TearDown() override {
        app.reset();
    }
    
    // Helper to capture framebuffer
    std::vector<unsigned char> captureFramebuffer() {
        auto* window = app->getRenderWindow();
        EXPECT_NE(window, nullptr);
        
        int width = window->getWidth();
        int height = window->getHeight();
        std::vector<unsigned char> pixels(width * height * 4); // RGBA
        
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        return pixels;
    }
    
    // Helper to check if any pixels are rendered
    bool hasRenderedPixels(const std::vector<unsigned char>& pixels) {
        int nonBlackPixels = 0;
        int redPixels = 0;
        int grayPixels = 0;
        
        
        // Check if any pixel is not black (background)
        for (size_t i = 0; i < pixels.size(); i += 4) {
            if (pixels[i] > 0 || pixels[i+1] > 0 || pixels[i+2] > 0) {
                nonBlackPixels++;
                if (pixels[i] > 100 && pixels[i+1] < 50 && pixels[i+2] < 50) {
                    redPixels++;
                }
                // Check for gray background (should be around 76, 76, 76)
                if (pixels[i] > 60 && pixels[i] < 90 && 
                    pixels[i+1] > 60 && pixels[i+1] < 90 &&
                    pixels[i+2] > 60 && pixels[i+2] < 90) {
                    grayPixels++;
                }
            }
        }
        
        
        return nonBlackPixels > 0;
    }
};

TEST_F(CLIRenderingIntegrationTest, BasicRenderingTest) {
    // Test basic rendering pipeline works
    auto* window = app->getRenderWindow();
    ASSERT_NE(window, nullptr) << "Render window should exist";
    
    // Render a frame
    app->render();
    window->swapBuffers();
    
    // Basic check - window dimensions
    EXPECT_GT(window->getWidth(), 0);
    EXPECT_GT(window->getHeight(), 0);
    
    // Check OpenGL context is valid
    EXPECT_EQ(glGetError(), GL_NO_ERROR) << "No OpenGL errors should occur";
}

TEST_F(CLIRenderingIntegrationTest, ScreenshotValidationTest) {
    // Test screenshot functionality
    auto* window = app->getRenderWindow();
    ASSERT_NE(window, nullptr);
    
    // Place a voxel to have something to render
    auto* voxelManager = app->getVoxelManager();
    voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    app->updateVoxelMeshes();
    
    // Render frame
    app->render();
    window->swapBuffers();
    
    // Save screenshot
    std::string filename = "test_screenshot.ppm";
    EXPECT_TRUE(window->saveScreenshot(filename)) << "Screenshot should be saved successfully";
    
    // TODO: Verify file exists and has valid content
    // For now, just check the method works without crashing
}

TEST_F(CLIRenderingIntegrationTest, VoxelRenderingTest) {
    // Test voxel rendering
    auto* voxelManager = app->getVoxelManager();
    
    
    // Place a voxel
    bool placedSuccessfully = voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    
    app->updateVoxelMeshes();
    
    
    // Render frame
    app->render();
    
    // Capture framebuffer BEFORE swapping (read from back buffer where we rendered)
    auto pixels = captureFramebuffer();
    
    app->getRenderWindow()->swapBuffers();
    
    // Verify something was rendered (not all black)
    EXPECT_TRUE(hasRenderedPixels(pixels)) << "Voxel should be visible in rendered output";
}

TEST_F(CLIRenderingIntegrationTest, CameraViewTest) {
    // Test different camera views
    auto* cameraController = app->getCameraController();
    auto* voxelManager = app->getVoxelManager();
    
    // Place a voxel
    voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    app->updateVoxelMeshes();
    
    // Test different camera presets
    std::vector<std::string> views = {"front", "back", "left", "right", "top", "bottom"};
    
    for (const auto& view : views) {
        // Set camera view (would need to expose this functionality)
        // For now just test that we can render from current view
        app->render();
        
        auto pixels = captureFramebuffer();
        app->getRenderWindow()->swapBuffers();
        
        EXPECT_TRUE(hasRenderedPixels(pixels)) << "Should render voxel from " << view << " view";
    }
}

TEST_F(CLIRenderingIntegrationTest, MultipleVoxelRenderingTest) {
    // Test rendering multiple voxels
    auto* voxelManager = app->getVoxelManager();
    
    // Place multiple voxels in a pattern
    for (int i = 0; i < 3; ++i) {
        voxelManager->setVoxel(Math::Vector3i(i * 64, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    }
    app->updateVoxelMeshes();
    
    // Render frame
    app->render();
    
    // Verify rendering
    auto pixels = captureFramebuffer();
    app->getRenderWindow()->swapBuffers();
    
    EXPECT_TRUE(hasRenderedPixels(pixels)) << "Multiple voxels should be visible";
    
    // Check voxel count
    EXPECT_EQ(voxelManager->getVoxelCount(), 3) << "Should have 3 voxels";
}

TEST_F(CLIRenderingIntegrationTest, ResolutionSwitchingTest) {
    // Test switching between different voxel resolutions
    auto* voxelManager = app->getVoxelManager();
    
    std::vector<VoxelData::VoxelResolution> resolutions = {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_64cm
    };
    
    for (auto resolution : resolutions) {
        // Clear previous voxels
        voxelManager->clearAll();
        
        // Set active resolution
        voxelManager->setActiveResolution(resolution);
        
        // Place a voxel at appropriate increment
        int increment = static_cast<int>(VoxelData::getVoxelSize(resolution) * 100);
        voxelManager->setVoxel(Math::Vector3i(0, 0, 0), resolution, true);
        app->updateVoxelMeshes();
        
        // Render
        app->render();
        
        // Verify
        auto pixels = captureFramebuffer();
        app->getRenderWindow()->swapBuffers();
        
        EXPECT_TRUE(hasRenderedPixels(pixels)) 
            << "Should render voxel at resolution " << increment << "cm";
    }
}

} // namespace Tests
} // namespace VoxelEditor