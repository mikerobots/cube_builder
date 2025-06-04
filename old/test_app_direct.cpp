// Direct test of the application rendering
#include "apps/cli/include/cli/Application.h"
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    std::cout << "=== Direct Application Test ===" << std::endl;
    
    // Create application
    VoxelEditor::Application app;
    
    // Initialize
    if (!app.initialize(argc, argv)) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    std::cout << "Application initialized" << std::endl;
    
    // Get managers
    auto* voxelManager = app.getVoxelDataManager();
    auto* cameraController = app.getCameraController();
    
    // Set up workspace
    voxelManager->resizeWorkspace(4.0f);
    voxelManager->setActiveResolution(VoxelEditor::VoxelData::VoxelResolution::Size_32cm);
    
    // Place a voxel
    voxelManager->setVoxel(VoxelEditor::Math::Vector3i(5, 5, 5), 
                          VoxelEditor::VoxelData::VoxelResolution::Size_32cm, true);
    
    std::cout << "Placed voxel at (5,5,5)" << std::endl;
    std::cout << "Voxel count: " << voxelManager->getVoxelCount() << std::endl;
    
    // Update mesh
    app.requestMeshUpdate();
    
    // Set camera
    cameraController->setViewPreset(VoxelEditor::Camera::ViewPreset::FRONT);
    cameraController->getCamera()->setDistance(8.0f);
    
    std::cout << "Camera set to front view" << std::endl;
    
    // Render a few frames
    std::cout << "Rendering frames..." << std::endl;
    for (int i = 0; i < 10; i++) {
        app.render();
        app.getRenderWindow()->swapBuffers();
        app.getRenderWindow()->pollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Take screenshot
    app.getRenderWindow()->captureScreenshot("test_app_direct.ppm");
    std::cout << "Screenshot saved to test_app_direct.ppm" << std::endl;
    
    // Analyze the screenshot
    std::ifstream file("test_app_direct.ppm.ppm", std::ios::binary);
    if (file) {
        std::string line;
        std::getline(file, line); // P6
        std::getline(file, line); // dimensions
        std::getline(file, line); // max value
        
        // Read some pixels
        std::vector<unsigned char> pixels(100 * 3);
        file.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
        
        // Check if all pixels are the same
        bool allSame = true;
        for (size_t i = 3; i < pixels.size(); i += 3) {
            if (pixels[i] != pixels[0] || pixels[i+1] != pixels[1] || pixels[i+2] != pixels[2]) {
                allSame = false;
                break;
            }
        }
        
        if (allSame) {
            std::cout << "WARNING: All sampled pixels are the same color: RGB(" 
                      << (int)pixels[0] << "," << (int)pixels[1] << "," << (int)pixels[2] << ")" << std::endl;
        } else {
            std::cout << "SUCCESS: Multiple colors found in screenshot" << std::endl;
        }
    }
    
    return 0;
}