#include <iostream>
#include <memory>
#include "shader_test/ShaderTestFramework.h"
#include "shader_test/TestMeshGenerator.h"
#include "core/rendering/GroundPlaneGrid.h"
#include "core/rendering/ShaderManager.h"
#include "core/rendering/OpenGLRenderer.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Matrix4f.h"

using namespace VoxelEditor;

bool testGroundPlaneShader() {
    std::cout << "\n=== Testing Ground Plane Grid Shader ===" << std::endl;
    
    // Initialize test framework
    ShaderTestFramework framework;
    if (!framework.initialize(800, 600, true)) {
        std::cerr << "Failed to initialize test framework" << std::endl;
        return false;
    }
    
    // Get OpenGL renderer and create shader manager
    auto* glRenderer = framework.getOpenGLRenderer();
    auto shaderManager = std::make_unique<Rendering::ShaderManager>(glRenderer);
    
    // Create ground plane grid
    auto groundPlane = std::make_unique<Rendering::GroundPlaneGrid>(shaderManager.get(), glRenderer);
    
    // Initialize the grid
    if (!groundPlane->initialize()) {
        std::cerr << "Failed to initialize ground plane grid" << std::endl;
        return false;
    }
    
    // Update grid mesh for 5x5 meter workspace
    Math::Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    groundPlane->updateGridMesh(workspaceSize);
    
    // Set camera
    Math::Matrix4f viewMatrix;
    viewMatrix.lookAt(
        Math::Vector3f(2.5f, 5.0f, 7.5f),  // Eye position (looking down at grid)
        Math::Vector3f(0.0f, 0.0f, 0.0f),  // Look at origin
        Math::Vector3f(0.0f, 1.0f, 0.0f)   // Up vector
    );
    
    Math::Matrix4f projMatrix;
    projMatrix.perspective(45.0f, 800.0f/600.0f, 0.1f, 100.0f);
    
    // Test rendering
    framework.clearFramebuffer(0.3f, 0.3f, 0.3f, 1.0f); // Grey background
    
    // Render the grid
    groundPlane->render(viewMatrix, projMatrix);
    
    // Capture framebuffer
    std::vector<uint8_t> pixels;
    if (!framework.captureFramebuffer(pixels)) {
        std::cerr << "Failed to capture framebuffer" << std::endl;
        return false;
    }
    
    // Save result
    if (!framework.saveImage("test_ground_plane_shader.ppm", pixels)) {
        std::cerr << "Failed to save image" << std::endl;
        return false;
    }
    
    // Analyze results - look for grid lines
    const int width = 800;
    const int height = 600;
    int gridPixelCount = 0;
    int backgroundPixelCount = 0;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            uint8_t r = pixels[idx];
            uint8_t g = pixels[idx + 1];
            uint8_t b = pixels[idx + 2];
            
            // Check if it's background color (grey ~77)
            if (std::abs(r - 77) < 5 && std::abs(g - 77) < 5 && std::abs(b - 77) < 5) {
                backgroundPixelCount++;
            }
            // Check if it's a grid line (should be lighter grey with transparency)
            else if (r > 100 && g > 100 && b > 100 && r == g && g == b) {
                gridPixelCount++;
            }
        }
    }
    
    std::cout << "Analysis results:" << std::endl;
    std::cout << "  Background pixels: " << backgroundPixelCount << std::endl;
    std::cout << "  Grid line pixels: " << gridPixelCount << std::endl;
    std::cout << "  Other pixels: " << (width * height - backgroundPixelCount - gridPixelCount) << std::endl;
    
    // Test should have some grid pixels
    bool success = gridPixelCount > 100; // Should have at least some grid lines
    
    if (success) {
        std::cout << "Ground plane grid shader test PASSED" << std::endl;
    } else {
        std::cerr << "Ground plane grid shader test FAILED - no grid lines detected" << std::endl;
    }
    
    return success;
}

int main() {
    bool success = testGroundPlaneShader();
    return success ? 0 : 1;
}