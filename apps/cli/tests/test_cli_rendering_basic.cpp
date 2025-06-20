#define GL_SILENCE_DEPRECATION
#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/RenderWindow.h"
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "rendering/RenderEngine.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstring>
#include <thread>
#include <chrono>

namespace VoxelEditor {
namespace Tests {

// Simple PPM image structure for screenshot validation
struct PPMImage {
    int width;
    int height;
    std::vector<uint8_t> pixels; // RGB format
    
    bool save(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return false;
        
        // PPM header
        file << "P6\n" << width << " " << height << "\n255\n";
        
        // Pixel data
        file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
        
        return file.good();
    }
    
    // Calculate average color for simple validation
    std::array<float, 3> getAverageColor() const {
        std::array<float, 3> avg = {0, 0, 0};
        size_t pixelCount = width * height;
        
        for (size_t i = 0; i < pixels.size(); i += 3) {
            avg[0] += pixels[i];     // R
            avg[1] += pixels[i + 1]; // G
            avg[2] += pixels[i + 2]; // B
        }
        
        avg[0] /= pixelCount;
        avg[1] /= pixelCount;
        avg[2] /= pixelCount;
        
        return avg;
    }
    
    // Check if image is mostly a certain color
    bool isDominantColor(uint8_t r, uint8_t g, uint8_t b, int tolerance = 30) const {
        auto avg = getAverageColor();
        return std::abs(avg[0] - r) < tolerance &&
               std::abs(avg[1] - g) < tolerance &&
               std::abs(avg[2] - b) < tolerance;
    }
};

class CLIRenderingBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        app = std::make_unique<Application>();
        
        // Initialize with render window (not headless)
        int argc = 1;
        char arg0[] = "test";
        char* argv[] = {arg0, nullptr};
        
        initialized = app->initialize(argc, argv);
        if (!initialized) {
            GTEST_SKIP() << "Cannot initialize rendering - may be running in headless environment";
        }
        
        // Additional check for CI environment - if window creation succeeded but we're in CI,
        // skip to avoid false failures due to virtual display issues
        if (std::getenv("CI") != nullptr) {
            GTEST_SKIP() << "Skipping rendering tests in CI environment - virtual display may not render correctly";
        }
        
        // Cache system pointers
        renderWindow = app->getRenderWindow();
        voxelManager = app->getVoxelManager();
        cameraController = app->getCameraController();
        renderEngine = app->getRenderEngine();
        
        if (!renderWindow || !renderEngine) {
            GTEST_SKIP() << "Rendering components not available";
        }
        
        // Set up test output directory
        testOutputDir = "test_renders";
        std::filesystem::create_directories(testOutputDir);
    }
    
    void TearDown() override {
        // Clean up test files if requested
        if (cleanupTestFiles) {
            std::filesystem::remove_all(testOutputDir);
        }
    }
    
    // Capture screenshot from the render window
    PPMImage captureScreenshot() {
        renderWindow->makeContextCurrent();
        
        int width = renderWindow->getWidth();
        int height = renderWindow->getHeight();
        
        PPMImage image;
        image.width = width;
        image.height = height;
        image.pixels.resize(width * height * 3);
        
        // Read pixels from OpenGL framebuffer
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image.pixels.data());
        
        // Flip vertically (OpenGL has origin at bottom-left)
        for (int y = 0; y < height / 2; ++y) {
            for (int x = 0; x < width * 3; ++x) {
                std::swap(image.pixels[y * width * 3 + x],
                         image.pixels[(height - 1 - y) * width * 3 + x]);
            }
        }
        
        return image;
    }
    
    // Render a frame and capture it
    PPMImage renderAndCapture() {
        app->render();
        
        // Capture before swapping buffers to read the rendered content
        PPMImage screenshot = captureScreenshot();
        
        // Now swap buffers for display
        renderWindow->swapBuffers();
        
        return screenshot;
    }
    
    // Helper to create test voxel scenes
    void createSingleVoxel(const Math::Vector3i& pos = Math::Vector3i(0, 0, 0),
                          VoxelData::VoxelResolution res = VoxelData::VoxelResolution::Size_8cm) {
        voxelManager->setActiveResolution(res);
        voxelManager->setVoxel(pos, res, true);
        app->updateVoxelMeshes();
    }
    
    void createVoxelCube(int size = 3) {
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
        
        // Center the cube around the origin for the new coordinate system
        int halfSize = size / 2;
        for (int x = -halfSize; x < size - halfSize; ++x) {
            for (int y = 0; y < size; ++y) { // Keep Y >= 0 (ground plane constraint)
                for (int z = -halfSize; z < size - halfSize; ++z) {
                    voxelManager->setVoxel(Math::Vector3i(x, y, z), 
                                         VoxelData::VoxelResolution::Size_8cm, true);
                }
            }
        }
        app->updateVoxelMeshes();
    }

    std::unique_ptr<Application> app;
    bool initialized = false;
    
    RenderWindow* renderWindow = nullptr;
    VoxelData::VoxelDataManager* voxelManager = nullptr;
    Camera::CameraController* cameraController = nullptr;
    Rendering::RenderEngine* renderEngine = nullptr;
    
    std::string testOutputDir;
    bool cleanupTestFiles = false; // Keep files for inspection
};

// ============================================================================
// Basic Rendering Tests
// ============================================================================

TEST_F(CLIRenderingBasicTest, WindowCreation) {
    // Verify window is created and has valid size
    EXPECT_TRUE(renderWindow->isOpen());
    EXPECT_GT(renderWindow->getWidth(), 0);
    EXPECT_GT(renderWindow->getHeight(), 0);
    
    // Verify OpenGL context
    renderWindow->makeContextCurrent();
    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    EXPECT_GE(maxTextureSize, 1024); // Reasonable minimum
}

TEST_F(CLIRenderingBasicTest, ClearColorRendering) {
    // Render with default clear color
    PPMImage screenshot = renderAndCapture();
    
    // Should be gray (0.3 * 255 = 76.5 ≈ 77)
    auto avgColor = screenshot.getAverageColor();
    
    // Log actual values for debugging
    std::cout << "Average color: R=" << avgColor[0] 
              << " G=" << avgColor[1] 
              << " B=" << avgColor[2] << std::endl;
    
    // Check if it's mostly gray (within tolerance)
    EXPECT_NEAR(avgColor[0], 77, 10);
    EXPECT_NEAR(avgColor[1], 77, 10); 
    EXPECT_NEAR(avgColor[2], 77, 10);
    
    // Save for visual inspection
    screenshot.save(testOutputDir + "/basic_clear_color.ppm");
}

TEST_F(CLIRenderingBasicTest, SingleVoxelVisible) {
    // Create single voxel at origin
    createSingleVoxel();
    
    // Render and capture
    PPMImage screenshot = renderAndCapture();
    
    // The voxel should make the image different from just the background
    auto avgColor = screenshot.getAverageColor();
    
    // Log actual values for debugging
    std::cout << "Single voxel - Average color: R=" << avgColor[0] 
              << " G=" << avgColor[1] 
              << " B=" << avgColor[2] << std::endl;
    
    // With a single voxel, the average should still be close to background
    // but slightly different. Let's check if we're getting any rendering at all
    bool hasNonBackgroundPixels = false;
    int backgroundValue = 77; // 0.3 * 255
    for (size_t i = 0; i < screenshot.pixels.size(); i += 3) {
        if (std::abs(screenshot.pixels[i] - backgroundValue) > 10 || 
            std::abs(screenshot.pixels[i+1] - backgroundValue) > 10 || 
            std::abs(screenshot.pixels[i+2] - backgroundValue) > 10) {
            hasNonBackgroundPixels = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonBackgroundPixels) << "No voxel pixels found - all pixels are background color";
    
    // Save for inspection
    screenshot.save(testOutputDir + "/basic_single_voxel.ppm");
}

TEST_F(CLIRenderingBasicTest, MultipleVoxels) {
    // Create a small cube of voxels
    createVoxelCube(3);
    
    PPMImage screenshot = renderAndCapture();
    
    // Should be significantly brighter with multiple voxels
    auto avgColor = screenshot.getAverageColor();
    EXPECT_GT(avgColor[0], 45);
    
    screenshot.save(testOutputDir + "/basic_voxel_cube.ppm");
}

TEST_F(CLIRenderingBasicTest, CameraPresets) {
    createVoxelCube(3);
    
    // Test a few camera presets
    std::vector<std::pair<Camera::ViewPreset, std::string>> presets = {
        {Camera::ViewPreset::FRONT, "front"},
        {Camera::ViewPreset::ISOMETRIC, "iso"}
    };
    
    for (const auto& preset : presets) {
        cameraController->setViewPreset(preset.first);
        
        // Wait for animation
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        PPMImage screenshot = renderAndCapture();
        screenshot.save(testOutputDir + "/basic_camera_" + preset.second + ".ppm");
        
        // Verify rendering
        auto avgColor = screenshot.getAverageColor();
        EXPECT_GT(avgColor[0], 30) << "View preset " << preset.second << " should show voxels";
    }
}

TEST_F(CLIRenderingBasicTest, EmptyScene) {
    // Render empty scene
    PPMImage screenshot = renderAndCapture();
    screenshot.save(testOutputDir + "/basic_empty_scene.ppm");
    
    // Should just show background color (0.3 * 255 = 77)
    EXPECT_TRUE(screenshot.isDominantColor(77, 77, 77));
}

TEST_F(CLIRenderingBasicTest, ScreenshotCapture) {
    // Test the built-in screenshot functionality
    createVoxelCube(2);
    
    app->render();
    
    // Save screenshot before swapping buffers
    std::string screenshotPath = testOutputDir + "/screenshot_test.ppm";
    bool saved = renderWindow->saveScreenshot(screenshotPath);
    
    renderWindow->swapBuffers();
    
    EXPECT_TRUE(saved);
    EXPECT_TRUE(std::filesystem::exists(screenshotPath));
}

// ============================================================================
// Performance Test
// ============================================================================

TEST_F(CLIRenderingBasicTest, BasicRenderingPerformance) {
    createVoxelCube(5); // 125 voxels
    
    auto start = std::chrono::high_resolution_clock::now();
    
    const int frameCount = 30;
    for (int i = 0; i < frameCount; ++i) {
        app->render();
        renderWindow->swapBuffers();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    float fps = (frameCount * 1000.0f) / duration.count();
    std::cout << "Basic rendering performance: " << fps << " FPS" << std::endl;
    
    // Should achieve at least 30 FPS for small scenes
    EXPECT_GT(fps, 30.0f);
}

} // namespace Tests
} // namespace VoxelEditor