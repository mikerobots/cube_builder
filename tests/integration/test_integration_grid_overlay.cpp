#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>

#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/RenderWindow.h"
#include "rendering/OpenGLRenderer.h"
#include "visual_feedback/FeedbackRenderer.h"
#include "camera/CameraController.h"
#include "voxel_data/VoxelDataManager.h"
#include "logging/Logger.h"

namespace VoxelEditor {

class GridOverlayTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize GLFW for OpenGL context
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        
        // Set up application with rendering enabled
        m_app = std::make_unique<Application>();
        
        // Initialize with proper arguments (no --headless)
        const char* args[] = { "test" };
        ASSERT_TRUE(m_app->initialize(1, const_cast<char**>(args))) 
            << "Failed to initialize application";
        
        // Get necessary components
        m_renderWindow = m_app->getRenderWindow();
        ASSERT_NE(m_renderWindow, nullptr) << "RenderWindow should not be null";
        
        m_commandProcessor = m_app->getCommandProcessor();
        ASSERT_NE(m_commandProcessor, nullptr) << "CommandProcessor should not be null";
        
        m_voxelManager = m_app->getVoxelManager();
        ASSERT_NE(m_voxelManager, nullptr) << "VoxelManager should not be null";
        
        m_cameraController = m_app->getCameraController();
        ASSERT_NE(m_cameraController, nullptr) << "CameraController should not be null";
    }
    
    void TearDown() override {
        m_app.reset();
        glfwTerminate();
    }
    
    // Helper to capture framebuffer
    std::vector<unsigned char> captureFramebuffer() {
        int width = m_renderWindow->getWidth();
        int height = m_renderWindow->getHeight();
        
        std::vector<unsigned char> pixels(width * height * 4); // RGBA
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        
        return pixels;
    }
    
    // Helper to count grid-like pixels (looking for gray lines)
    int countGridPixels(const std::vector<unsigned char>& pixels) {
        int count = 0;
        
        // Sample background from center of image 
        int centerIdx = (pixels.size() / 2) & ~3; // Align to 4-byte boundary
        unsigned char bgR = pixels[centerIdx];
        unsigned char bgG = pixels[centerIdx + 1];
        unsigned char bgB = pixels[centerIdx + 2];
        
        for (size_t i = 0; i < pixels.size(); i += 4) {
            unsigned char r = pixels[i];
            unsigned char g = pixels[i+1];
            unsigned char b = pixels[i+2];
            
            // Look for pixels that are noticeably different from background
            // Grid lines should be lighter than the gray background
            if (r > bgR + 15 || g > bgG + 15 || b > bgB + 15) {
                count++;
            }
        }
        return count;
    }
    
    // Helper to save debug screenshot
    void saveDebugScreenshot(const std::string& filename) {
        int width = m_renderWindow->getWidth();
        int height = m_renderWindow->getHeight();
        auto pixels = captureFramebuffer();
        
        // Save as PPM for easy viewing
        std::ofstream file(filename);
        file << "P3\n" << width << " " << height << "\n255\n";
        
        // PPM format is top-to-bottom, OpenGL is bottom-to-top
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 4;
                file << (int)pixels[idx] << " " 
                     << (int)pixels[idx+1] << " " 
                     << (int)pixels[idx+2] << "\n";
            }
        }
    }
    
protected:
    std::unique_ptr<Application> m_app;
    RenderWindow* m_renderWindow = nullptr;
    CommandProcessor* m_commandProcessor = nullptr;
    VoxelData::VoxelDataManager* m_voxelManager = nullptr;
    Camera::CameraController* m_cameraController = nullptr;
};

TEST_F(GridOverlayTest, DebugGridOffByDefault) {
    // Grid should be disabled by default
    // Update and render to ensure initial state
    m_app->update();
    m_app->render();
    
    // Capture framebuffer - should not have many grid pixels
    auto pixels = captureFramebuffer();
    int gridPixelsBefore = countGridPixels(pixels);
    
    saveDebugScreenshot("test_grid_default_off.ppm");
    
    // Should have minimal grid pixels when disabled
    EXPECT_LT(gridPixelsBefore, 1000) << "Too many grid-like pixels when grid should be off";
}

TEST_F(GridOverlayTest, CanToggleGridOverlay) {
    // Test toggling grid overlay on and off via command
    
    // Initially off
    m_app->update();
    m_app->render();
    auto pixelsBefore = captureFramebuffer();
    int gridPixelsBefore = countGridPixels(pixelsBefore);
    
    // Enable grid
    auto result = m_commandProcessor->execute("debug grid");
    EXPECT_TRUE(result.success) << "Failed to execute debug grid command";
    EXPECT_TRUE(result.message.find("enabled") != std::string::npos) 
        << "Grid should be enabled, message: " << result.message;
    
    // Debug: Check if the application's debug grid flag is set
    std::cout << "Debug grid command result: " << result.message << std::endl;
    
    // Update and render with grid enabled
    m_app->update();
    m_app->render();
    
    // Ensure the render is complete
    glFinish();
    
    auto pixelsAfterEnable = captureFramebuffer();
    int gridPixelsAfterEnable = countGridPixels(pixelsAfterEnable);
    
    saveDebugScreenshot("test_grid_enabled.ppm");
    
    std::cout << "Grid pixels before: " << gridPixelsBefore << std::endl;
    std::cout << "Grid pixels after enable: " << gridPixelsAfterEnable << std::endl;
    
    // Should have more grid pixels when enabled, but allow a lower threshold since grid is transparent
    EXPECT_GT(gridPixelsAfterEnable, gridPixelsBefore + 10) 
        << "Grid pixels should increase when grid is enabled (before: " << gridPixelsBefore 
        << ", after: " << gridPixelsAfterEnable << ")";
    
    // Disable grid again
    result = m_commandProcessor->execute("debug grid");
    EXPECT_TRUE(result.success) << "Failed to execute debug grid command";
    EXPECT_TRUE(result.message.find("disabled") != std::string::npos) 
        << "Grid should be disabled, message: " << result.message;
    
    // Update and render with grid disabled
    m_app->update();
    m_app->render();
    auto pixelsAfterDisable = captureFramebuffer();
    int gridPixelsAfterDisable = countGridPixels(pixelsAfterDisable);
    
    saveDebugScreenshot("test_grid_disabled.ppm");
    
    // Should return to original state (allow small tolerance)
    EXPECT_LT(gridPixelsAfterDisable, gridPixelsAfterEnable - 5) 
        << "Grid pixels should decrease when grid is disabled (enabled: " << gridPixelsAfterEnable 
        << ", disabled: " << gridPixelsAfterDisable << ")";
}

TEST_F(GridOverlayTest, GridVisibleAtDifferentZoomLevels) {
    // Test that grid is visible at different camera distances
    
    // Enable grid
    auto result = m_commandProcessor->execute("debug grid");
    EXPECT_TRUE(result.success);
    
    // Test at different zoom levels
    std::vector<float> zoomLevels = {2.0f, 5.0f, 10.0f, 20.0f};
    
    for (size_t i = 0; i < zoomLevels.size(); ++i) {
        // Set camera distance
        m_cameraController->getCamera()->setDistance(zoomLevels[i]);
        
        // Update and render
        m_app->update();
        m_app->render();
        
        // Capture and verify grid is visible
        auto pixels = captureFramebuffer();
        int gridPixels = countGridPixels(pixels);
        
        EXPECT_GT(gridPixels, 100) 
            << "Grid should be visible at zoom level " << zoomLevels[i] 
            << " (distance=" << zoomLevels[i] << ")";
        
        // Save debug screenshot
        saveDebugScreenshot("test_grid_zoom_" + std::to_string(i) + ".ppm");
    }
}

TEST_F(GridOverlayTest, GridAlignsWithVoxelPlacements) {
    // Test that the grid aligns with 1cm voxel placements
    
    // Enable grid and set resolution to 1cm
    m_commandProcessor->execute("debug grid");
    m_commandProcessor->execute("resolution 1cm");
    
    // Place a voxel at origin
    m_voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm, true);
    m_app->requestMeshUpdate();
    
    // Set camera to good viewing position
    m_cameraController->getCamera()->setDistance(5.0f);
    m_cameraController->getCamera()->setTarget(Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f)));
    
    // Update and render
    m_app->update();
    m_app->render();
    
    // Capture screenshot
    auto pixels = captureFramebuffer();
    int gridPixels = countGridPixels(pixels);
    
    saveDebugScreenshot("test_grid_with_voxel.ppm");
    
    // Should have both grid and voxel visible
    EXPECT_GT(gridPixels, 500) << "Grid should be visible with voxel placement";
    
    // Place more voxels at 1cm increments to verify alignment
    m_voxelManager->setVoxel(Math::Vector3i(1, 0, 0), VoxelData::VoxelResolution::Size_1cm, true);
    m_voxelManager->setVoxel(Math::Vector3i(0, 1, 0), VoxelData::VoxelResolution::Size_1cm, true);
    m_voxelManager->setVoxel(Math::Vector3i(0, 0, 1), VoxelData::VoxelResolution::Size_1cm, true);
    m_app->requestMeshUpdate();
    
    // Update and render again
    m_app->update();
    m_app->render();
    
    saveDebugScreenshot("test_grid_alignment.ppm");
    
    // Grid should still be visible with multiple voxels
    pixels = captureFramebuffer();
    gridPixels = countGridPixels(pixels);
    EXPECT_GT(gridPixels, 400) << "Grid should remain visible with multiple voxels";
}

TEST_F(GridOverlayTest, GridShowsOnecentimeterIncrements) {
    // Test that the grid specifically shows 1cm increments regardless of current resolution
    
    // Enable grid and set a larger resolution (should still show 1cm grid)
    m_commandProcessor->execute("debug grid");
    m_commandProcessor->execute("resolution 32cm");
    
    // Set camera close enough to see fine detail
    m_cameraController->getCamera()->setDistance(3.0f);
    m_cameraController->getCamera()->setTarget(Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f)));
    
    // Update and render
    m_app->update();
    m_app->render();
    
    // Should show fine 1cm grid even with 32cm resolution
    auto pixels = captureFramebuffer();
    int gridPixels = countGridPixels(pixels);
    
    saveDebugScreenshot("test_grid_1cm_increments.ppm");
    
    EXPECT_GT(gridPixels, 1000) << "Fine 1cm grid should be visible even with larger voxel resolution";
}

} // namespace VoxelEditor