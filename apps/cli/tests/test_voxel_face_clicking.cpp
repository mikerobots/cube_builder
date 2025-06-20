#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <sstream>

#include "cli/Application.h"
#include "cli/MouseInteraction.h"
#include "cli/RenderWindow.h"
#include "voxel_data/VoxelDataManager.h"
#include "foundation/logging/Logger.h"
#include "camera/CameraController.h"
#include "camera/OrbitCamera.h"
#include "foundation/math/CoordinateConverter.h"

namespace VoxelEditor {
namespace Tests {

class VoxelFaceClickingTest : public ::testing::Test {
protected:
    std::unique_ptr<Application> app;
    VoxelData::VoxelDataManager* voxelManager = nullptr;
    std::unique_ptr<MouseInteraction> mouseInteraction;
    Camera::CameraController* cameraController = nullptr;
    
    void SetUp() override {
        // Initialize logging - suppress output for tests
        Logging::Logger::getInstance().setLevel(Logging::Logger::Level::Warning);
        
        // Skip test in headless CI environment
        if (std::getenv("CI") != nullptr || std::getenv("GITHUB_ACTIONS") != nullptr) {
            GTEST_SKIP() << "Skipping GUI tests in CI environment";
        }
        
        // Create application in headless mode for testing
        app = std::make_unique<Application>();
        
        // Initialize in headless mode
        int argc = 2;
        char arg0[] = "test";
        char arg1[] = "--headless";
        char* argv[] = {arg0, arg1, nullptr};
        
        // Redirect stdout to suppress initialization messages
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        
        bool initialized = app->initialize(argc, argv);
        
        // Restore stdout
        std::cout.rdbuf(old);
        
        ASSERT_TRUE(initialized) << "Application should initialize in headless mode";
        
        // Get managers
        voxelManager = app->getVoxelManager();
        cameraController = app->getCameraController();
        
        // Create mouse interaction
        mouseInteraction = std::make_unique<MouseInteraction>(app.get());
        mouseInteraction->initialize();
        
        // Set default resolution to 64cm for easier testing
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_64cm);
    }
    
    void TearDown() override {
        mouseInteraction.reset();
        app.reset();
    }
    
    // Simulate a mouse click at normalized device coordinates
    void simulateClick(float ndcX, float ndcY) {
        auto renderWindow = app->getRenderWindow();
        
        // In headless mode, use default window size
        int width = 800;
        int height = 600;
        
        if (renderWindow) {
            width = renderWindow->getWidth();
            height = renderWindow->getHeight();
        }
        
        double screenX = (ndcX + 1.0) * 0.5 * width;
        double screenY = (1.0 - ndcY) * 0.5 * height;  // Flip Y for screen coordinates
        
        // Update mouse position
        mouseInteraction->onMouseMove(screenX, screenY);
        
        // Simulate left click
        mouseInteraction->onMouseClick(GLFW_MOUSE_BUTTON_LEFT, true, screenX, screenY);
        mouseInteraction->onMouseClick(GLFW_MOUSE_BUTTON_LEFT, false, screenX, screenY);
    }
    
    // Helper to count voxels in the grid
    int countVoxels() {
        // Use VoxelDataManager's built-in count which is more efficient and accurate
        return static_cast<int>(voxelManager->getVoxelCount());
    }
};

TEST_F(VoxelFaceClickingTest, ClickOnVoxelFaceAddsAdjacentVoxel) {
    // Place initial voxel at world center using increment coordinates
    // For a centered workspace, the origin in world space (0,0,0) corresponds to increment coordinates (0,0,0)
    voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    EXPECT_EQ(countVoxels(), 1) << "Should have 1 voxel after initial placement";
    
    // Update mesh and mouse interaction
    app->updateVoxelMeshes();
    
    // Click on the center of the screen (should hit the voxel's front face)
    simulateClick(0.0f, 0.0f);
    
    // Update again to process the click result
    app->updateVoxelMeshes();
    
    // Should now have 2 voxels
    EXPECT_EQ(countVoxels(), 2) << "Should have 2 voxels after clicking on face";
    
    // Click again to add a third voxel
    simulateClick(0.1f, 0.0f);  // Slightly offset to ensure we hit a face
    app->updateVoxelMeshes();
    
    EXPECT_EQ(countVoxels(), 3) << "Should have 3 voxels after second click";
}

TEST_F(VoxelFaceClickingTest, ClickOnDifferentFacesAddsVoxelsCorrectly) {
    // Place initial voxel at centered coordinate system origin
    voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    EXPECT_EQ(countVoxels(), 1);
    
    // Get camera
    auto camera = dynamic_cast<Camera::OrbitCamera*>(cameraController->getCamera());
    ASSERT_NE(camera, nullptr);
    
    // View from front - click to add voxel on front face
    // Convert increment coordinates to world coordinates for camera target
    Math::WorldCoordinates voxelWorldPos = Math::CoordinateConverter::incrementToWorld(Math::IncrementCoordinates(0, 0, 0));
    camera->setTarget(voxelWorldPos);
    camera->setDistance(5.0f);
    camera->setOrbitAngles(0.0f, 0.0f);  // Front view
    
    app->updateVoxelMeshes();
    simulateClick(0.0f, 0.0f);
    app->updateVoxelMeshes();
    EXPECT_EQ(countVoxels(), 2) << "Should add voxel on front face";
    
    // View from right - click to add voxel on right face
    camera->setOrbitAngles(90.0f, 0.0f);  // Right view
    app->updateVoxelMeshes();
    simulateClick(0.0f, 0.0f);
    app->updateVoxelMeshes();
    EXPECT_EQ(countVoxels(), 3) << "Should add voxel on right face";
}

TEST_F(VoxelFaceClickingTest, MultipleVoxelPlacementBug) {
    // Test the specific bug: place at (0,5,0) then click
    voxelManager->setVoxel(Math::Vector3i(0, 5, 0), VoxelData::VoxelResolution::Size_64cm, true);
    EXPECT_EQ(countVoxels(), 1);
    
    // Position camera to see the voxel
    auto camera = dynamic_cast<Camera::OrbitCamera*>(cameraController->getCamera());
    ASSERT_NE(camera, nullptr);
    // Calculate world position for voxel at increment coordinates (0,5,0) 
    Math::WorldCoordinates voxelWorldPos = Math::CoordinateConverter::incrementToWorld(Math::IncrementCoordinates(0, 5, 0));
    float voxelSize = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_64cm);
    camera->setTarget(Math::WorldCoordinates(voxelWorldPos.value() + Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f)));
    camera->setDistance(5.0f);
    
    app->updateVoxelMeshes();
    
    // Try to click on the voxel
    simulateClick(0.0f, 0.0f);
    app->updateVoxelMeshes();
    
    // Should be able to add adjacent voxel
    int voxelCount = countVoxels();
    EXPECT_GT(voxelCount, 1) << "Should be able to add voxel by clicking on (0,5,0)";
    
    // Test the working case: place at (0,0,0) then click
    // Clear all voxels first - use centered coordinate range
    for (int x = -5; x <= 5; x++) {
        for (int y = 0; y <= 10; y++) {  // Keep Y positive since Y=0 is ground
            for (int z = -5; z <= 5; z++) {
                voxelManager->setVoxel(Math::Vector3i(x, y, z), VoxelData::VoxelResolution::Size_64cm, false);
            }
        }
    }
    
    voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    EXPECT_EQ(countVoxels(), 1);
    
    // Calculate world position for voxel at increment coordinates (0,0,0)
    Math::WorldCoordinates voxelWorldPos2 = Math::CoordinateConverter::incrementToWorld(Math::IncrementCoordinates(0, 0, 0));
    camera->setTarget(Math::WorldCoordinates(voxelWorldPos2.value() + Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f)));
    app->updateVoxelMeshes();
    
    simulateClick(0.0f, 0.0f);
    app->updateVoxelMeshes();
    
    EXPECT_EQ(countVoxels(), 2) << "Should add voxel by clicking on (0,0,0)";
}

TEST_F(VoxelFaceClickingTest, ClosestVoxelIsSelected) {
    // Place two voxels along the same ray path
    voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    voxelManager->setVoxel(Math::Vector3i(1, 0, 0), VoxelData::VoxelResolution::Size_64cm, true);
    EXPECT_EQ(countVoxels(), 2);
    
    // Position camera so that (1,0,0) is closer than (0,0,0)
    auto camera = dynamic_cast<Camera::OrbitCamera*>(cameraController->getCamera());
    ASSERT_NE(camera, nullptr);
    // Use coordinate converter to get proper world position for voxel (1,0,0)
    Math::WorldCoordinates voxelWorldPos = Math::CoordinateConverter::incrementToWorld(Math::IncrementCoordinates(1, 0, 0));
    camera->setTarget(voxelWorldPos);
    camera->setDistance(3.0f);
    camera->setOrbitAngles(90.0f, 0.0f);  // Look from positive X direction
    
    app->updateVoxelMeshes();
    
    // Click - should hit the closer voxel (1,0,0) and add voxel at (2,0,0)
    simulateClick(0.0f, 0.0f);
    app->updateVoxelMeshes();
    
    EXPECT_EQ(countVoxels(), 3) << "Should add voxel adjacent to the closer one";
    
    // Verify the new voxel is at (2,0,0)
    EXPECT_TRUE(voxelManager->hasVoxel(Math::Vector3i(2, 0, 0), VoxelData::VoxelResolution::Size_64cm)) 
        << "New voxel should be placed at (2,0,0) adjacent to closer voxel";
}

} // namespace Tests
} // namespace VoxelEditor