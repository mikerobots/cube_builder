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
#include "math/CoordinateTypes.h"

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

TEST_F(GridOverlayTest, GroundPlaneGridVisible) {
    // Ground plane grid should be visible by default
    // Update and render to ensure initial state
    m_app->update();
    m_app->render();
    
    // Capture framebuffer - should have ground plane grid pixels
    auto pixels = captureFramebuffer();
    int gridPixels = countGridPixels(pixels);
    
    saveDebugScreenshot("test_ground_plane_grid.ppm");
    
    // Should have some grid pixels from the ground plane grid
    EXPECT_GT(gridPixels, 100) << "Ground plane grid should be visible";
}

TEST_F(GridOverlayTest, GroundPlaneGridConsistency) {
    // Test that ground plane grid remains consistent across renders
    
    // Initial render
    m_app->update();
    m_app->render();
    auto pixelsFirst = captureFramebuffer();
    int gridPixelsFirst = countGridPixels(pixelsFirst);
    
    // Render again without changes
    m_app->update();
    m_app->render();
    auto pixelsSecond = captureFramebuffer();
    int gridPixelsSecond = countGridPixels(pixelsSecond);
    
    saveDebugScreenshot("test_grid_consistency.ppm");
    
    // Grid pixel count should be consistent across renders (allow small tolerance)
    EXPECT_NEAR(gridPixelsFirst, gridPixelsSecond, 50) 
        << "Grid rendering should be consistent (first: " << gridPixelsFirst 
        << ", second: " << gridPixelsSecond << ")";
}

TEST_F(GridOverlayTest, GroundPlaneGridVisibleAtDifferentZoomLevels) {
    // Test that ground plane grid is visible at different camera distances
    
    // Ground plane grid is always visible, no need to enable
    
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

TEST_F(GridOverlayTest, GroundPlaneGridAlignsWithVoxelPlacements) {
    // Test that the ground plane grid aligns with 1cm voxel placements
    
    // Ground plane grid is always visible, just set resolution to 1cm
    m_commandProcessor->execute("resolution 1cm");
    
    // Place a voxel at origin
    m_voxelManager->setVoxel(Math::IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_1cm, true);
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
    m_voxelManager->setVoxel(Math::IncrementCoordinates(1, 0, 0), VoxelData::VoxelResolution::Size_1cm, true);
    m_voxelManager->setVoxel(Math::IncrementCoordinates(0, 1, 0), VoxelData::VoxelResolution::Size_1cm, true);
    m_voxelManager->setVoxel(Math::IncrementCoordinates(0, 0, 1), VoxelData::VoxelResolution::Size_1cm, true);
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

TEST_F(GridOverlayTest, GroundPlaneGridShowsConsistentIncrements) {
    // Test that the ground plane grid shows consistent increments regardless of current resolution
    
    // Set a larger resolution (ground plane grid should remain consistent)
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
    
    saveDebugScreenshot("test_grid_consistent_increments.ppm");
    
    EXPECT_GT(gridPixels, 1000) << "Ground plane grid should be visible even with larger voxel resolution";
}

} // namespace VoxelEditor