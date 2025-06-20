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

// PPM image structure for screenshot validation
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
    
    bool load(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return false;
        
        // Read header
        std::string magic;
        file >> magic;
        if (magic != "P6") return false;
        
        file >> width >> height;
        int maxval;
        file >> maxval;
        if (maxval != 255) return false;
        
        // Skip single whitespace after header
        file.ignore(1);
        
        // Read pixel data
        pixels.resize(width * height * 3);
        file.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
        
        return file.good();
    }
    
    // Compare two images with tolerance for platform differences
    bool compare(const PPMImage& other, int tolerance = 5) const {
        if (width != other.width || height != other.height) return false;
        if (pixels.size() != other.pixels.size()) return false;
        
        for (size_t i = 0; i < pixels.size(); ++i) {
            int diff = std::abs(static_cast<int>(pixels[i]) - static_cast<int>(other.pixels[i]));
            if (diff > tolerance) {
                return false;
            }
        }
        
        return true;
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
    
    // Check if a region contains a specific color
    bool hasColorInRegion(int x, int y, int w, int h, 
                         uint8_t r, uint8_t g, uint8_t b, 
                         int tolerance = 5) const {
        for (int py = y; py < y + h && py < height; ++py) {
            for (int px = x; px < x + w && px < width; ++px) {
                int idx = (py * width + px) * 3;
                if (idx + 2 < pixels.size()) {
                    int dr = std::abs(pixels[idx] - r);
                    int dg = std::abs(pixels[idx + 1] - g);
                    int db = std::abs(pixels[idx + 2] - b);
                    
                    if (dr <= tolerance && dg <= tolerance && db <= tolerance) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
};

class CLIRenderingTest : public ::testing::Test {
protected:
    void SetUp() override {
        app = std::make_unique<Application>();
        
        // Initialize with render window (not headless)
        int argc = 1;
        char arg0[] = "test";
        char* argv[] = {arg0, nullptr};
        
        initialized = app->initialize(argc, argv);
        ASSERT_TRUE(initialized) << "Application should initialize with rendering";
        
        // Cache system pointers
        renderWindow = app->getRenderWindow();
        voxelManager = app->getVoxelManager();
        cameraController = app->getCameraController();
        renderEngine = app->getRenderEngine();
        
        ASSERT_NE(renderWindow, nullptr);
        ASSERT_NE(renderEngine, nullptr);
        
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
        
        int width, height;
        renderWindow->getFramebufferSize(width, height);
        
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
        renderWindow->swapBuffers();
        
        // Small delay to ensure rendering completes
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        
        return captureScreenshot();
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
        // Center the cube around the origin
        int offset = size / 2;
        for (int x = 0; x < size; ++x) {
            for (int y = 0; y < size; ++y) {
                for (int z = 0; z < size; ++z) {
                    voxelManager->setVoxel(Math::Vector3i(x - offset, y, z - offset), 
                                         VoxelData::VoxelResolution::Size_8cm, true);
                }
            }
        }
        app->updateVoxelMeshes();
    }
    
    void createVoxelPlane(int width = 5, int depth = 5) {
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
        // Center the plane around the origin
        int offsetX = width / 2;
        int offsetZ = depth / 2;
        for (int x = 0; x < width; ++x) {
            for (int z = 0; z < depth; ++z) {
                voxelManager->setVoxel(Math::Vector3i(x - offsetX, 0, z - offsetZ), 
                                     VoxelData::VoxelResolution::Size_8cm, true);
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
    bool cleanupTestFiles = true;
};

// ============================================================================
// Basic Rendering Tests
// ============================================================================

TEST_F(CLIRenderingTest, WindowCreation) {
    // Verify window and OpenGL context are created
    EXPECT_TRUE(renderWindow->isValid());
    
    int width, height;
    renderWindow->getFramebufferSize(width, height);
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
    
    // Verify OpenGL context is current
    renderWindow->makeContextCurrent();
    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    EXPECT_GE(maxTextureSize, 1024); // Reasonable minimum
}

TEST_F(CLIRenderingTest, ClearColorRendering) {
    // Test clear color rendering
    renderEngine->clear(Rendering::ClearFlags::All, 
                       Rendering::Color(0.2f, 0.3f, 0.4f, 1.0f));
    
    PPMImage screenshot = captureScreenshot();
    
    // Verify the clear color (convert from float to byte)
    uint8_t expectedR = static_cast<uint8_t>(0.2f * 255);
    uint8_t expectedG = static_cast<uint8_t>(0.3f * 255);
    uint8_t expectedB = static_cast<uint8_t>(0.4f * 255);
    
    auto avgColor = screenshot.getAverageColor();
    EXPECT_NEAR(avgColor[0], expectedR, 5);
    EXPECT_NEAR(avgColor[1], expectedG, 5);
    EXPECT_NEAR(avgColor[2], expectedB, 5);
    
    // Save for visual inspection
    screenshot.save(testOutputDir + "/clear_color.ppm");
}

TEST_F(CLIRenderingTest, ViewportSizing) {
    // Test different viewport sizes
    std::vector<std::pair<int, int>> sizes = {
        {640, 480},
        {800, 600},
        {1024, 768}
    };
    
    for (const auto& size : sizes) {
        renderWindow->setSize(size.first, size.second);
        
        // Wait for resize to take effect
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        int width, height;
        renderWindow->getFramebufferSize(width, height);
        
        // Account for high-DPI displays
        EXPECT_GE(width, size.first);
        EXPECT_GE(height, size.second);
    }
}

// ============================================================================
// Voxel Rendering Tests
// ============================================================================

TEST_F(CLIRenderingTest, SingleVoxelRendering) {
    // Clear to dark background
    renderEngine->setClearColor(Rendering::Color(0.1f, 0.1f, 0.1f, 1.0f));
    
    // Create single voxel at origin
    createSingleVoxel();
    
    // Render and capture
    PPMImage screenshot = renderAndCapture();
    
    // The voxel should be visible (lighter than background)
    auto avgColor = screenshot.getAverageColor();
    EXPECT_GT(avgColor[0], 25); // Should be brighter than background
    
    // Save for inspection
    screenshot.save(testOutputDir + "/single_voxel.ppm");
}

TEST_F(CLIRenderingTest, MultipleVoxelPositions) {
    renderEngine->setClearColor(Rendering::Color(0.1f, 0.1f, 0.1f, 1.0f));
    
    // Create voxels at different positions (centered around origin)
    std::vector<Math::Vector3i> positions = {
        {0, 0, 0},    // Center
        {-1, 0, 1},   // Negative X, positive Z
        {1, 1, 0},    // Positive X and Y
        {0, 0, -1},   // Negative Z
        {-1, 2, -1}   // Mixed negative coordinates
    };
    
    for (const auto& pos : positions) {
        createSingleVoxel(pos);
    }
    
    PPMImage screenshot = renderAndCapture();
    
    // Should see multiple voxels
    auto avgColor = screenshot.getAverageColor();
    EXPECT_GT(avgColor[0], 25);
    
    screenshot.save(testOutputDir + "/multiple_voxels.ppm");
}

TEST_F(CLIRenderingTest, DifferentResolutionVoxels) {
    renderEngine->setClearColor(Rendering::Color(0.1f, 0.1f, 0.1f, 1.0f));
    
    // Create voxels at different resolutions
    createSingleVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_8cm);
    createSingleVoxel(Math::Vector3i(2, 0, 0), VoxelData::VoxelResolution::Size_16cm);
    createSingleVoxel(Math::Vector3i(0, 2, 0), VoxelData::VoxelResolution::Size_32cm);
    
    PPMImage screenshot = renderAndCapture();
    screenshot.save(testOutputDir + "/multi_resolution.ppm");
    
    // Verify something is rendered
    auto avgColor = screenshot.getAverageColor();
    EXPECT_GT(avgColor[0], 20);
}

// ============================================================================
// Camera View Tests
// ============================================================================

TEST_F(CLIRenderingTest, DefaultCameraView) {
    createVoxelCube(3);
    
    // Reset to default view
    cameraController->resetView();
    
    PPMImage screenshot = renderAndCapture();
    screenshot.save(testOutputDir + "/camera_default.ppm");
    
    // Should see the cube
    auto avgColor = screenshot.getAverageColor();
    EXPECT_GT(avgColor[0], 20);
}

TEST_F(CLIRenderingTest, AllPresetViews) {
    createVoxelCube(3);
    
    std::vector<std::pair<Camera::ViewPreset, std::string>> presets = {
        {Camera::ViewPreset::Front, "front"},
        {Camera::ViewPreset::Back, "back"},
        {Camera::ViewPreset::Left, "left"},
        {Camera::ViewPreset::Right, "right"},
        {Camera::ViewPreset::Top, "top"},
        {Camera::ViewPreset::Bottom, "bottom"},
        {Camera::ViewPreset::Isometric, "iso"}
    };
    
    for (const auto& preset : presets) {
        cameraController->setViewPreset(preset.first);
        
        // Wait for animation
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        PPMImage screenshot = renderAndCapture();
        screenshot.save(testOutputDir + "/camera_" + preset.second + ".ppm");
        
        // Verify rendering
        auto avgColor = screenshot.getAverageColor();
        EXPECT_GT(avgColor[0], 15) << "View preset " << preset.second << " should show voxels";
    }
}

TEST_F(CLIRenderingTest, CameraZoomLevels) {
    createVoxelCube(3);
    
    std::vector<float> zoomLevels = {0.5f, 1.0f, 2.0f, 5.0f};
    
    for (float zoom : zoomLevels) {
        auto camera = cameraController->getCamera();
        camera->setDistance(5.0f / zoom); // Closer = more zoom
        
        PPMImage screenshot = renderAndCapture();
        screenshot.save(testOutputDir + "/zoom_" + std::to_string(zoom) + ".ppm");
    }
}

// ============================================================================
// Selection Rendering Tests
// ============================================================================

TEST_F(CLIRenderingTest, SelectedVoxelHighlight) {
    createVoxelCube(3);
    
    // Select center voxel
    Selection::VoxelId voxelId(Math::Vector3i(1, 1, 1), VoxelData::VoxelResolution::Size_8cm);
    app->getSelectionManager()->selectVoxel(voxelId);
    
    PPMImage screenshot = renderAndCapture();
    screenshot.save(testOutputDir + "/selected_voxel.ppm");
    
    // Should show selection highlight
    auto avgColor = screenshot.getAverageColor();
    EXPECT_GT(avgColor[0], 25);
}

TEST_F(CLIRenderingTest, BoxSelectionOutline) {
    createVoxelPlane(5, 5);
    
    // Select box region (centered around origin)
    Math::BoundingBox box(
        Math::Vector3f(-0.12f, 0.0f, -0.12f),   // Start from centered position
        Math::Vector3f(0.12f, 0.08f, 0.12f)     // 3x1x3 voxels centered
    );
    
    app->getSelectionManager()->selectBox(box, VoxelData::VoxelResolution::Size_8cm);
    
    PPMImage screenshot = renderAndCapture();
    screenshot.save(testOutputDir + "/box_selection.ppm");
}

// ============================================================================
// Visual Feedback Tests
// ============================================================================

TEST_F(CLIRenderingTest, GreenOutlinePreview) {
    // Set hover position for green outline
    app->setHoverPosition(Math::Vector3i(2, 0, 2));
    
    PPMImage screenshot = renderAndCapture();
    screenshot.save(testOutputDir + "/green_outline.ppm");
    
    // Look for green color in the image
    bool hasGreen = false;
    for (size_t i = 0; i < screenshot.pixels.size(); i += 3) {
        uint8_t r = screenshot.pixels[i];
        uint8_t g = screenshot.pixels[i + 1];
        uint8_t b = screenshot.pixels[i + 2];
        
        // Check for predominantly green pixels
        if (g > r && g > b && g > 100) {
            hasGreen = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasGreen) << "Green outline should be visible";
}

// ============================================================================
// Complex Scene Tests
// ============================================================================

TEST_F(CLIRenderingTest, LargeVoxelCount) {
    // Create 10x10x10 cube (1000 voxels)
    createVoxelCube(10);
    
    PPMImage screenshot = renderAndCapture();
    screenshot.save(testOutputDir + "/large_voxel_count.ppm");
    
    // Verify rendering completed
    auto avgColor = screenshot.getAverageColor();
    EXPECT_GT(avgColor[0], 20);
}

TEST_F(CLIRenderingTest, MixedResolutionScene) {
    // Create voxels of different sizes (centered around origin)
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    for (int i = -2; i <= 2; ++i) {
        voxelManager->setVoxel(Math::Vector3i(i, 0, 0), VoxelData::VoxelResolution::Size_8cm, true);
    }
    
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_16cm);
    for (int i = -1; i <= 1; ++i) {
        voxelManager->setVoxel(Math::Vector3i(i, 1, 0), VoxelData::VoxelResolution::Size_16cm, true);
    }
    
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_32cm);
    voxelManager->setVoxel(Math::Vector3i(0, 2, 0), VoxelData::VoxelResolution::Size_32cm, true);
    
    app->updateVoxelMeshes();
    
    PPMImage screenshot = renderAndCapture();
    screenshot.save(testOutputDir + "/mixed_resolution.ppm");
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(CLIRenderingTest, EmptySceneRendering) {
    // Render empty scene
    PPMImage screenshot = renderAndCapture();
    screenshot.save(testOutputDir + "/empty_scene.ppm");
    
    // Should just show background color
    auto avgColor = screenshot.getAverageColor();
    uint8_t expectedGray = static_cast<uint8_t>(0.2f * 255);
    EXPECT_NEAR(avgColor[0], expectedGray, 10);
    EXPECT_NEAR(avgColor[1], expectedGray, 10);
    EXPECT_NEAR(avgColor[2], expectedGray, 10);
}

TEST_F(CLIRenderingTest, ExtremeCameraPositions) {
    createVoxelCube(3);
    
    auto camera = cameraController->getCamera();
    
    // Very close
    camera->setDistance(0.5f);
    PPMImage screenshot1 = renderAndCapture();
    screenshot1.save(testOutputDir + "/camera_very_close.ppm");
    
    // Very far
    camera->setDistance(50.0f);
    PPMImage screenshot2 = renderAndCapture();
    screenshot2.save(testOutputDir + "/camera_very_far.ppm");
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(CLIRenderingTest, RenderingPerformance) {
    createVoxelCube(5); // 125 voxels
    
    auto start = std::chrono::high_resolution_clock::now();
    
    const int frameCount = 60;
    for (int i = 0; i < frameCount; ++i) {
        app->render();
        renderWindow->swapBuffers();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    float fps = (frameCount * 1000.0f) / duration.count();
    std::cout << "Rendering performance: " << fps << " FPS" << std::endl;
    
    // Should achieve at least 30 FPS for small scenes
    EXPECT_GT(fps, 30.0f);
}

} // namespace Tests
} // namespace VoxelEditor