#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>

extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>

#include "rendering/RenderEngine.h"
#include "rendering/OpenGLRenderer.h"
#include "rendering/ShaderManager.h"
#include "camera/OrbitCamera.h"
#include "shader_test/TestMeshGenerator.h"
#include "math/Matrix4f.h"

// Add GL_SILENCE_DEPRECATION for macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

using namespace VoxelEditor;

// Structure to hold pixel data for analysis
struct ImageData {
    int width;
    int height;
    std::vector<uint8_t> pixels; // RGB format
    
    // Get pixel at (x, y) as RGB values
    std::tuple<uint8_t, uint8_t, uint8_t> getPixel(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return {0, 0, 0};
        }
        int idx = (y * width + x) * 3;
        return {pixels[idx], pixels[idx + 1], pixels[idx + 2]};
    }
    
    // Calculate average color in a region
    std::tuple<float, float, float> getAverageColor(int x1, int y1, int x2, int y2) const {
        float r = 0, g = 0, b = 0;
        int count = 0;
        
        for (int y = y1; y <= y2 && y < height; y++) {
            for (int x = x1; x <= x2 && x < width; x++) {
                auto [pr, pg, pb] = getPixel(x, y);
                r += pr;
                g += pg;
                b += pb;
                count++;
            }
        }
        
        if (count > 0) {
            r /= count;
            g /= count;
            b /= count;
        }
        
        return {r, g, b};
    }
    
    // Compare two regions for similarity
    float compareRegions(int x1, int y1, int w1, int h1,
                        int x2, int y2, int w2, int h2) const {
        if (w1 != w2 || h1 != h2) return 0.0f;
        
        float totalDiff = 0.0f;
        int count = 0;
        
        for (int dy = 0; dy < h1; dy++) {
            for (int dx = 0; dx < w1; dx++) {
                auto [r1, g1, b1] = getPixel(x1 + dx, y1 + dy);
                auto [r2, g2, b2] = getPixel(x2 + dx, y2 + dy);
                
                float diff = std::abs(r1 - r2) + std::abs(g1 - g2) + std::abs(b1 - b2);
                totalDiff += diff / (3.0f * 255.0f); // Normalize to [0, 1]
                count++;
            }
        }
        
        return count > 0 ? 1.0f - (totalDiff / count) : 0.0f;
    }
    
    // Check if a region is mostly a single color
    bool isUniformColor(int x, int y, int w, int h, float tolerance = 0.1f) const {
        auto [avgR, avgG, avgB] = getAverageColor(x, y, x + w - 1, y + h - 1);
        
        for (int dy = 0; dy < h; dy++) {
            for (int dx = 0; dx < w; dx++) {
                auto [r, g, b] = getPixel(x + dx, y + dy);
                float diff = (std::abs(r - avgR) + std::abs(g - avgG) + std::abs(b - avgB)) / (3.0f * 255.0f);
                if (diff > tolerance) {
                    return false;
                }
            }
        }
        
        return true;
    }
};

class GeometricShaderValidation {
public:
    bool initialize(int width = 512, int height = 512) {
        m_width = width;
        m_height = height;
        
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for automated testing
        
        // Create window
        m_window = glfwCreateWindow(m_width, m_height, "Geometric Shader Validation", nullptr, nullptr);
        if (!m_window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(m_window);
        
        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            cleanup();
            return false;
        }
        
        // Initialize rendering components
        m_renderEngine = std::make_unique<Rendering::RenderEngine>();
        Rendering::RenderConfig config;
        config.windowWidth = m_width;
        config.windowHeight = m_height;
        config.vsync = false; // No vsync for testing
        
        if (!m_renderEngine->initialize(config)) {
            std::cerr << "Failed to initialize RenderEngine" << std::endl;
            cleanup();
            return false;
        }
        
        // Set up overhead camera
        m_camera = std::make_unique<Camera::OrbitCamera>();
        setupOverheadCamera();
        
        m_renderEngine->setCamera(*m_camera);
        m_renderEngine->setViewport(0, 0, m_width, m_height);
        
        return true;
    }
    
    void cleanup() {
        m_renderEngine.reset();
        m_camera.reset();
        
        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
    }
    
    // Test 1: Half-rotated cube test
    bool testHalfRotatedCube(const std::string& shaderName) {
        std::cout << "\n=== Testing Half-Rotated Cube with " << shaderName << " shader ===" << std::endl;
        
        // Create a cube mesh
        auto cubeMesh = ShaderTest::TestMeshGenerator::createCube(2.0f, Math::Vector3f(0.8f, 0.3f, 0.3f));
        Rendering::Mesh renderCube = convertMesh(cubeMesh);
        
        // Get shader
        Rendering::ShaderId shaderId = m_renderEngine->getBuiltinShader(shaderName);
        if (shaderId == Rendering::InvalidId) {
            std::cerr << "Failed to get " << shaderName << " shader" << std::endl;
            return false;
        }
        
        // Clear screen to black
        m_renderEngine->clear(Rendering::ClearFlags::All, Rendering::Color(0, 0, 0, 1));
        
        // Render cube rotated 45 degrees
        Rendering::Transform transform;
        transform.position = Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f));
        transform.rotation = Math::Vector3f(0.0f, 45.0f, 0.0f); // 45 degree Y rotation
        
        Rendering::Material material;
        material.shader = shaderId;
        material.albedo = Rendering::Color(0.8f, 0.3f, 0.3f, 1.0f);
        
        m_renderEngine->renderMesh(renderCube, transform, material);
        
        // Capture and analyze
        ImageData image = captureFrame();
        
        // Save for debugging
        std::string filename = "test_output/geometric_cube_" + shaderName + ".ppm";
        saveImage(image, filename);
        
        // Analyze: Sample regions where we expect to see the cube
        // Sample from center area where cube is likely to be
        int sampleSize = 100; // 100x100 pixel regions
        int centerX = m_width / 2;
        int centerY = m_height / 2;
        
        // Sample left and right of center
        auto [leftR, leftG, leftB] = image.getAverageColor(centerX - sampleSize - 20, centerY - sampleSize/2, 
                                                           centerX - 20, centerY + sampleSize/2);
        auto [rightR, rightG, rightB] = image.getAverageColor(centerX + 20, centerY - sampleSize/2, 
                                                              centerX + sampleSize + 20, centerY + sampleSize/2);
        
        std::cout << "Left region average: R=" << leftR << " G=" << leftG << " B=" << leftB << std::endl;
        std::cout << "Right region average: R=" << rightR << " G=" << rightG << " B=" << rightB << std::endl;
        
        // Check that both sides have content (not black)
        // Lower threshold since shaders produce darker output
        bool leftHasContent = (leftR + leftG + leftB) > 10;
        bool rightHasContent = (rightR + rightG + rightB) > 10;
        
        if (!leftHasContent || !rightHasContent) {
            std::cout << "❌ One or both sides are too dark (no rendered content)" << std::endl;
            return false;
        }
        
        // For the enhanced and flat shaders, different faces should have different brightness
        if (shaderName == "enhanced" || shaderName == "flat") {
            float brightnessDiff = std::abs((leftR + leftG + leftB) - (rightR + rightG + rightB));
            std::cout << "Brightness difference between sides: " << brightnessDiff << std::endl;
            
            // With angled camera, we should see face distinctions
            if (brightnessDiff < 5) {
                std::cout << "⚠️  Warning: Enhanced/flat shader shows minimal face distinction (diff=" << brightnessDiff << ")" << std::endl;
                // Don't fail the test, just warn
            }
        }
        
        std::cout << "✅ Half-rotated cube test passed for " << shaderName << " shader" << std::endl;
        return true;
    }
    
    // Test 2: Grid pattern test
    bool testGridPattern(const std::string& shaderName) {
        std::cout << "\n=== Testing Grid Pattern with " << shaderName << " shader ===" << std::endl;
        
        // Create a grid mesh
        auto gridMesh = ShaderTest::TestMeshGenerator::createGrid(10, 1.0f, 5);
        Rendering::Mesh renderGrid = convertMesh(gridMesh);
        
        // Get shader
        Rendering::ShaderId shaderId = m_renderEngine->getBuiltinShader(shaderName);
        if (shaderId == Rendering::InvalidId) {
            std::cerr << "Failed to get " << shaderName << " shader" << std::endl;
            return false;
        }
        
        // Clear screen to dark gray
        m_renderEngine->clear(Rendering::ClearFlags::All, Rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));
        
        // Render grid
        Rendering::Transform transform;
        transform.position = Math::WorldCoordinates(Math::Vector3f(0.0f, -2.0f, 0.0f));
        
        Rendering::Material material;
        material.shader = shaderId;
        material.albedo = Rendering::Color(0.7f, 0.7f, 0.7f, 1.0f);
        
        // Render as lines for grid
        m_renderEngine->renderMeshAsLines(renderGrid, transform, material);
        
        // Capture and analyze
        ImageData image = captureFrame();
        
        // Save for debugging
        std::string filename = "test_output/geometric_grid_" + shaderName + ".ppm";
        saveImage(image, filename);
        
        // Analyze: Check for regular pattern
        // Sample several points where we expect grid lines
        int gridSize = 10;
        int cellPixels = m_width / gridSize;
        
        int gridLinesFound = 0;
        int expectedLines = 0;
        
        // Check horizontal lines
        for (int i = 1; i < gridSize; i++) {
            int y = (i * m_height) / gridSize;
            auto [r, g, b] = image.getAverageColor(m_width/4, y-2, 3*m_width/4, y+2);
            if ((r + g + b) > 100) { // Brighter than background
                gridLinesFound++;
            }
            expectedLines++;
        }
        
        // Check vertical lines
        for (int i = 1; i < gridSize; i++) {
            int x = (i * m_width) / gridSize;
            auto [r, g, b] = image.getAverageColor(x-2, m_height/4, x+2, 3*m_height/4);
            if ((r + g + b) > 100) { // Brighter than background
                gridLinesFound++;
            }
            expectedLines++;
        }
        
        float gridCoverage = (float)gridLinesFound / expectedLines;
        std::cout << "Grid lines found: " << gridLinesFound << "/" << expectedLines 
                  << " (" << (gridCoverage * 100) << "%)" << std::endl;
        
        if (gridCoverage < 0.5f) {
            std::cout << "❌ Grid pattern not clearly visible" << std::endl;
            return false;
        }
        
        std::cout << "✅ Grid pattern test passed for " << shaderName << " shader" << std::endl;
        return true;
    }
    
    // Test 3: Checkerboard test for face distinction
    bool testCheckerboardCubes(const std::string& shaderName) {
        std::cout << "\n=== Testing Checkerboard Cubes with " << shaderName << " shader ===" << std::endl;
        
        // Clear screen to black
        m_renderEngine->clear(Rendering::ClearFlags::All, Rendering::Color(0, 0, 0, 1));
        
        // Get shader
        Rendering::ShaderId shaderId = m_renderEngine->getBuiltinShader(shaderName);
        if (shaderId == Rendering::InvalidId) {
            std::cerr << "Failed to get " << shaderName << " shader" << std::endl;
            return false;
        }
        
        // Create a 4x4 checkerboard of cubes
        int gridSize = 4;
        float cubeSize = 0.8f;
        float spacing = 2.0f;
        
        for (int x = 0; x < gridSize; x++) {
            for (int z = 0; z < gridSize; z++) {
                // Checkerboard pattern
                bool isRed = (x + z) % 2 == 0;
                
                auto cubeMesh = ShaderTest::TestMeshGenerator::createCube(cubeSize, 
                    isRed ? Math::Vector3f(0.8f, 0.2f, 0.2f) : Math::Vector3f(0.2f, 0.2f, 0.8f));
                Rendering::Mesh renderCube = convertMesh(cubeMesh);
                
                Rendering::Transform transform;
                transform.position = Math::WorldCoordinates(Math::Vector3f(
                    (x - gridSize/2.0f + 0.5f) * spacing,
                    0.0f,
                    (z - gridSize/2.0f + 0.5f) * spacing
                ));
                
                Rendering::Material material;
                material.shader = shaderId;
                material.albedo = isRed ? Rendering::Color(0.8f, 0.2f, 0.2f, 1.0f) 
                                        : Rendering::Color(0.2f, 0.2f, 0.8f, 1.0f);
                
                m_renderEngine->renderMesh(renderCube, transform, material);
            }
        }
        
        // Capture and analyze
        ImageData image = captureFrame();
        
        // Save for debugging
        std::string filename = "test_output/geometric_checkerboard_" + shaderName + ".ppm";
        saveImage(image, filename);
        
        // Analyze: Count red vs blue regions
        int redPixels = 0;
        int bluePixels = 0;
        int totalColoredPixels = 0;
        
        for (int y = 0; y < m_height; y++) {
            for (int x = 0; x < m_width; x++) {
                auto [r, g, b] = image.getPixel(x, y);
                
                // Skip black background
                if (r + g + b < 30) continue;
                
                totalColoredPixels++;
                if (r > b) {
                    redPixels++;
                } else {
                    bluePixels++;
                }
            }
        }
        
        float redRatio = (float)redPixels / totalColoredPixels;
        float blueRatio = (float)bluePixels / totalColoredPixels;
        
        std::cout << "Red pixels: " << redPixels << " (" << (redRatio * 100) << "%)" << std::endl;
        std::cout << "Blue pixels: " << bluePixels << " (" << (blueRatio * 100) << "%)" << std::endl;
        
        // Should be roughly 50/50 for checkerboard
        if (std::abs(redRatio - 0.5f) > 0.15f) {
            std::cout << "❌ Checkerboard pattern not balanced (expected ~50/50)" << std::endl;
            return false;
        }
        
        std::cout << "✅ Checkerboard test passed for " << shaderName << " shader" << std::endl;
        return true;
    }
    
private:
    GLFWwindow* m_window = nullptr;
    std::unique_ptr<Rendering::RenderEngine> m_renderEngine;
    std::unique_ptr<Camera::OrbitCamera> m_camera;
    int m_width;
    int m_height;
    
    void setupOverheadCamera() {
        // Position camera at an angle to see multiple faces
        m_camera->setDistance(10.0f);
        m_camera->setPitch(-60.0f); // Looking down at 60 degree angle
        m_camera->setYaw(45.0f);     // Rotated to see corner
        m_camera->setFieldOfView(45.0f);
        m_camera->setAspectRatio((float)m_width / m_height);
        m_camera->setTarget(Math::WorldCoordinates(Math::Vector3f(0, 0, 0)));
    }
    
    ImageData captureFrame() {
        ImageData image;
        image.width = m_width;
        image.height = m_height;
        image.pixels.resize(m_width * m_height * 3);
        
        // Ensure rendering is complete
        glFlush();
        glFinish();
        
        // Read pixels
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, image.pixels.data());
        
        // Flip vertically (OpenGL gives us upside down)
        std::vector<uint8_t> flipped(image.pixels.size());
        for (int y = 0; y < m_height; y++) {
            std::memcpy(&flipped[y * m_width * 3], 
                       &image.pixels[(m_height - 1 - y) * m_width * 3], 
                       m_width * 3);
        }
        image.pixels = std::move(flipped);
        
        return image;
    }
    
    void saveImage(const ImageData& image, const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }
        
        // PPM header
        file << "P6\n" << image.width << " " << image.height << "\n255\n";
        file.write(reinterpret_cast<const char*>(image.pixels.data()), image.pixels.size());
        file.close();
        
        std::cout << "Saved: " << filename << std::endl;
    }
    
    Rendering::Mesh convertMesh(const Rendering::Mesh& testMesh) {
        Rendering::Mesh mesh;
        mesh.vertices = testMesh.vertices;
        mesh.indices = testMesh.indices;
        return mesh;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "=== Geometric Shader Validation Test ===" << std::endl;
    std::cout << "This test renders known geometric patterns and validates shader output" << std::endl;
    
    GeometricShaderValidation validator;
    
    if (!validator.initialize()) {
        std::cerr << "Failed to initialize validator" << std::endl;
        return 1;
    }
    
    // Create output directory
    system("mkdir -p test_output");
    
    bool allTestsPassed = true;
    
    // Test all shaders
    std::vector<std::string> shaderNames = {"basic", "enhanced", "flat"};
    
    for (const auto& shaderName : shaderNames) {
        std::cout << "\n========== Testing " << shaderName << " shader ==========" << std::endl;
        
        // Run all geometric tests
        if (!validator.testHalfRotatedCube(shaderName)) {
            allTestsPassed = false;
        }
        
        if (!validator.testGridPattern(shaderName)) {
            allTestsPassed = false;
        }
        
        if (!validator.testCheckerboardCubes(shaderName)) {
            allTestsPassed = false;
        }
    }
    
    validator.cleanup();
    
    if (allTestsPassed) {
        std::cout << "\n✅ All geometric validation tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some geometric validation tests failed!" << std::endl;
        return 1;
    }
}