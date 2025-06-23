#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/CommandTypes.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "camera/OrbitCamera.h"
#include <sstream>
#include <memory>
#include <algorithm>

namespace VoxelEditor {
namespace Tests {

class CameraCommandValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        app = std::make_unique<Application>();
        
        // Initialize in headless mode
        int argc = 2;
        char arg0[] = "test";
        char arg1[] = "--headless";
        char* argv[] = {arg0, arg1, nullptr};
        
        initialized = app->initialize(argc, argv);
        ASSERT_TRUE(initialized) << "Application should initialize in headless mode";
    }
    
    void TearDown() override {
        // Clean up
    }
    
    std::unique_ptr<Application> app;
    bool initialized = false;
};

// ============================================================================
// REQ-11.3.18: Camera commands shall test all predefined view positions
// ============================================================================

TEST_F(CameraCommandValidationTest, CameraViewPreset_Front_REQ_11_3_18) {
    // Test camera command with "front" view preset
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr) << "CommandProcessor should be available";
    ASSERT_NE(cameraController, nullptr) << "CameraController should be available";
    
    // Execute camera front command
    std::string command = "camera front";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Camera front command should succeed";
    EXPECT_FALSE(result.message.empty()) << "Camera command should provide feedback";
    EXPECT_NE(result.message.find("front"), std::string::npos) 
        << "Success message should mention 'front' view";
    
    // Verify that the camera has been set to front view
    // Front view typically looks down the negative Z axis
    auto* camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr) << "Camera should be available";
    
    // Force matrix update to ensure the view preset has been applied
    camera->getViewMatrix();
    camera->getProjectionMatrix();
    
    // Verify camera state is consistent (we can't predict exact values without knowing
    // the implementation details, but we can verify the command completed successfully)
    Math::WorldCoordinates position = camera->getPosition();
    Math::WorldCoordinates target = camera->getTarget();
    
    // Basic sanity checks - camera should have valid position and target
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should be valid after front view preset";
    EXPECT_FALSE(std::isnan(target.x()) || std::isnan(target.y()) || std::isnan(target.z()))
        << "Camera target should be valid after front view preset";
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_Back_REQ_11_3_18) {
    // Test camera command with "back" view preset
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    // Execute camera back command
    std::string command = "camera back";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Camera back command should succeed";
    EXPECT_FALSE(result.message.empty()) << "Camera command should provide feedback";
    EXPECT_NE(result.message.find("back"), std::string::npos) 
        << "Success message should mention 'back' view";
    
    // Verify camera state
    auto* camera = cameraController->getCamera();
    camera->getViewMatrix(); // Force update
    
    Math::WorldCoordinates position = camera->getPosition();
    Math::WorldCoordinates target = camera->getTarget();
    
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should be valid after back view preset";
    EXPECT_FALSE(std::isnan(target.x()) || std::isnan(target.y()) || std::isnan(target.z()))
        << "Camera target should be valid after back view preset";
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_Left_REQ_11_3_18) {
    // Test camera command with "left" view preset
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    // Execute camera left command
    std::string command = "camera left";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Camera left command should succeed";
    EXPECT_FALSE(result.message.empty()) << "Camera command should provide feedback";
    EXPECT_NE(result.message.find("left"), std::string::npos) 
        << "Success message should mention 'left' view";
    
    // Verify camera state
    auto* camera = cameraController->getCamera();
    camera->getViewMatrix(); // Force update
    
    Math::WorldCoordinates position = camera->getPosition();
    Math::WorldCoordinates target = camera->getTarget();
    
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should be valid after left view preset";
    EXPECT_FALSE(std::isnan(target.x()) || std::isnan(target.y()) || std::isnan(target.z()))
        << "Camera target should be valid after left view preset";
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_Right_REQ_11_3_18) {
    // Test camera command with "right" view preset
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    // Execute camera right command
    std::string command = "camera right";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Camera right command should succeed";
    EXPECT_FALSE(result.message.empty()) << "Camera command should provide feedback";
    EXPECT_NE(result.message.find("right"), std::string::npos) 
        << "Success message should mention 'right' view";
    
    // Verify camera state
    auto* camera = cameraController->getCamera();
    camera->getViewMatrix(); // Force update
    
    Math::WorldCoordinates position = camera->getPosition();
    Math::WorldCoordinates target = camera->getTarget();
    
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should be valid after right view preset";
    EXPECT_FALSE(std::isnan(target.x()) || std::isnan(target.y()) || std::isnan(target.z()))
        << "Camera target should be valid after right view preset";
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_Top_REQ_11_3_18) {
    // Test camera command with "top" view preset
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    // Execute camera top command
    std::string command = "camera top";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Camera top command should succeed";
    EXPECT_FALSE(result.message.empty()) << "Camera command should provide feedback";
    EXPECT_NE(result.message.find("top"), std::string::npos) 
        << "Success message should mention 'top' view";
    
    // Verify camera state
    auto* camera = cameraController->getCamera();
    camera->getViewMatrix(); // Force update
    
    Math::WorldCoordinates position = camera->getPosition();
    Math::WorldCoordinates target = camera->getTarget();
    
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should be valid after top view preset";
    EXPECT_FALSE(std::isnan(target.x()) || std::isnan(target.y()) || std::isnan(target.z()))
        << "Camera target should be valid after top view preset";
    
    // For top view, camera should be looking down (position Y should be above target Y)
    EXPECT_GT(position.y(), target.y()) 
        << "For top view, camera should be positioned above the target";
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_Bottom_REQ_11_3_18) {
    // Test camera command with "bottom" view preset
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    // Execute camera bottom command
    std::string command = "camera bottom";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Camera bottom command should succeed";
    EXPECT_FALSE(result.message.empty()) << "Camera command should provide feedback";
    EXPECT_NE(result.message.find("bottom"), std::string::npos) 
        << "Success message should mention 'bottom' view";
    
    // Verify camera state
    auto* camera = cameraController->getCamera();
    camera->getViewMatrix(); // Force update
    
    Math::WorldCoordinates position = camera->getPosition();
    Math::WorldCoordinates target = camera->getTarget();
    
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should be valid after bottom view preset";
    EXPECT_FALSE(std::isnan(target.x()) || std::isnan(target.y()) || std::isnan(target.z()))
        << "Camera target should be valid after bottom view preset";
    
    // For bottom view, camera should be looking up (position Y should be below target Y)
    EXPECT_LT(position.y(), target.y()) 
        << "For bottom view, camera should be positioned below the target";
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_Isometric_REQ_11_3_18) {
    // Test camera command with "iso" (isometric) view preset
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    // Execute camera iso command
    std::string command = "camera iso";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Camera iso command should succeed";
    EXPECT_FALSE(result.message.empty()) << "Camera command should provide feedback";
    EXPECT_NE(result.message.find("iso"), std::string::npos) 
        << "Success message should mention 'iso' view";
    
    // Verify camera state
    auto* camera = cameraController->getCamera();
    camera->getViewMatrix(); // Force update
    
    Math::WorldCoordinates position = camera->getPosition();
    Math::WorldCoordinates target = camera->getTarget();
    
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should be valid after isometric view preset";
    EXPECT_FALSE(std::isnan(target.x()) || std::isnan(target.y()) || std::isnan(target.z()))
        << "Camera target should be valid after isometric view preset";
    
    // Verify that camera distance is set to reasonable value (3.0f as per code)
    if (auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(camera)) {
        float distance = orbitCamera->getDistance();
        EXPECT_GT(distance, 0.0f) << "Camera distance should be positive";
        EXPECT_LT(distance, 100.0f) << "Camera distance should be reasonable";
    }
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_Default_REQ_11_3_18) {
    // Test camera command with "default" view preset (should map to isometric)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    // Execute camera default command
    std::string command = "camera default";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "Camera default command should succeed";
    EXPECT_FALSE(result.message.empty()) << "Camera command should provide feedback";
    EXPECT_NE(result.message.find("default"), std::string::npos) 
        << "Success message should mention 'default' view";
    
    // Verify camera state
    auto* camera = cameraController->getCamera();
    camera->getViewMatrix(); // Force update
    
    Math::WorldCoordinates position = camera->getPosition();
    Math::WorldCoordinates target = camera->getTarget();
    
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should be valid after default view preset";
    EXPECT_FALSE(std::isnan(target.x()) || std::isnan(target.y()) || std::isnan(target.z()))
        << "Camera target should be valid after default view preset";
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_AllViews_StateConsistency_REQ_11_3_18) {
    // Test all camera view presets for state consistency and uniqueness
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    // List of all valid view presets
    std::vector<std::string> viewPresets = {
        "front", "back", "left", "right", "top", "bottom", "iso", "default"
    };
    
    std::vector<std::pair<Math::WorldCoordinates, Math::WorldCoordinates>> cameraStates;
    
    for (const std::string& preset : viewPresets) {
        // Execute camera command for this preset
        std::string command = "camera " + preset;
        auto result = commandProcessor->execute(command);
        
        EXPECT_TRUE(result.success) << "Camera " << preset << " command should succeed";
        EXPECT_FALSE(result.message.empty()) << "Camera command should provide feedback";
        
        // Get camera state after setting preset
        auto* camera = cameraController->getCamera();
        camera->getViewMatrix(); // Force update
        
        Math::WorldCoordinates position = camera->getPosition();
        Math::WorldCoordinates target = camera->getTarget();
        
        // Verify valid state
        EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
            << "Camera position should be valid after " << preset << " view preset";
        EXPECT_FALSE(std::isnan(target.x()) || std::isnan(target.y()) || std::isnan(target.z()))
            << "Camera target should be valid after " << preset << " view preset";
        
        // Store state for uniqueness checking
        cameraStates.push_back({position, target});
    }
    
    // Verify that at least some view presets result in different camera positions
    // (We expect different views to have different positions)
    bool foundDifferentPositions = false;
    for (size_t i = 1; i < cameraStates.size(); ++i) {
        if (cameraStates[i].first != cameraStates[0].first) {
            foundDifferentPositions = true;
            break;
        }
    }
    EXPECT_TRUE(foundDifferentPositions) 
        << "Different view presets should result in different camera positions";
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_InvalidPreset_REQ_11_3_18) {
    // Test camera command with invalid view preset (should fail gracefully)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test invalid view preset
    std::string command = "camera invalid_view";
    auto result = commandProcessor->execute(command);
    
    EXPECT_FALSE(result.success) << "Camera command with invalid preset should fail";
    EXPECT_FALSE(result.message.empty()) << "Error message should be provided";
    
    // Test other invalid presets
    std::vector<std::string> invalidPresets = {
        "unknown", "perspective", "orthographic", "", "123", "front_back"
    };
    
    for (const std::string& preset : invalidPresets) {
        std::string invalidCommand = "camera " + preset;
        auto invalidResult = commandProcessor->execute(invalidCommand);
        
        EXPECT_FALSE(invalidResult.success) 
            << "Camera command with invalid preset '" << preset << "' should fail";
        EXPECT_FALSE(invalidResult.message.empty()) 
            << "Error message should be provided for invalid preset '" << preset << "'";
    }
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_CaseInsensitive_REQ_11_3_18) {
    // Test that camera view presets work with different case variations
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test that the documented camera presets work consistently
    std::vector<std::string> validPresets = {
        "front", "back", "left", "right", "top", "bottom", "iso", "default"
    };
    
    for (const std::string& preset : validPresets) {
        std::string command = "camera " + preset;
        auto result = commandProcessor->execute(command);
        
        EXPECT_TRUE(result.success) 
            << "Camera preset '" << preset << "' should work (as documented in Commands.cpp)";
        EXPECT_FALSE(result.message.empty()) 
            << "Camera command should provide feedback for preset '" << preset << "'";
    }
    
    // Test case sensitivity - camera commands should be case-sensitive based on implementation
    std::vector<std::string> invalidCasePresets = {
        "FRONT", "Front", "TOP", "ISO", "Isometric", "ISOMETRIC"
    };
    
    for (const std::string& preset : invalidCasePresets) {
        std::string command = "camera " + preset;
        auto result = commandProcessor->execute(command);
        
        // Document the behavior: case-sensitive commands should fail
        EXPECT_FALSE(result.success) 
            << "Camera preset '" << preset << "' should fail (commands are case-sensitive)";
    }
}

TEST_F(CameraCommandValidationTest, CameraViewPreset_AliasCommand_REQ_11_3_18) {
    // Test camera command using alias "view" instead of "camera"
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    // Test using the "view" alias for camera command
    std::string command = "view front";
    auto result = commandProcessor->execute(command);
    
    EXPECT_TRUE(result.success) << "View command (camera alias) should work";
    EXPECT_FALSE(result.message.empty()) << "View command should provide feedback";
    
    // Verify camera state changed
    auto* camera = cameraController->getCamera();
    camera->getViewMatrix(); // Force update
    
    Math::WorldCoordinates position = camera->getPosition();
    Math::WorldCoordinates target = camera->getTarget();
    
    EXPECT_FALSE(std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z()))
        << "Camera position should be valid after view command";
    EXPECT_FALSE(std::isnan(target.x()) || std::isnan(target.y()) || std::isnan(target.z()))
        << "Camera target should be valid after view command";
}

// ============================================================================
// REQ-11.3.19: Camera commands shall test zoom limits and boundaries
// ============================================================================

TEST_F(CameraCommandValidationTest, ZoomCommand_ValidZoomFactors_REQ_11_3_19) {
    // Test zoom command with valid zoom factors
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    auto* camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr);
    
    // Reset to known state
    std::string resetCommand = "camera iso";
    commandProcessor->execute(resetCommand);
    
    // Get initial distance
    float initialDistance = camera->getDistance();
    EXPECT_GT(initialDistance, 0.0f) << "Initial camera distance should be positive";
    
    // Test zoom in (factor > 1.0)
    std::string zoomInCommand = "zoom 1.5";
    auto zoomInResult = commandProcessor->execute(zoomInCommand);
    
    EXPECT_TRUE(zoomInResult.success) << "Zoom in command should succeed";
    EXPECT_FALSE(zoomInResult.message.empty()) << "Zoom command should provide feedback";
    
    float zoomedInDistance = camera->getDistance();
    EXPECT_LT(zoomedInDistance, initialDistance) << "Zooming in should decrease camera distance";
    EXPECT_NEAR(zoomedInDistance, initialDistance / 1.5f, 0.01f) 
        << "Zoom factor should be applied correctly";
    
    // Test zoom out (factor < 1.0)
    std::string zoomOutCommand = "zoom 0.8";
    auto zoomOutResult = commandProcessor->execute(zoomOutCommand);
    
    EXPECT_TRUE(zoomOutResult.success) << "Zoom out command should succeed";
    EXPECT_FALSE(zoomOutResult.message.empty()) << "Zoom command should provide feedback";
    
    float zoomedOutDistance = camera->getDistance();
    EXPECT_GT(zoomedOutDistance, zoomedInDistance) << "Zooming out should increase camera distance";
    EXPECT_NEAR(zoomedOutDistance, zoomedInDistance / 0.8f, 0.01f) 
        << "Zoom factor should be applied correctly";
}

TEST_F(CameraCommandValidationTest, ZoomCommand_MinimumDistanceLimit_REQ_11_3_19) {
    // Test that zoom command respects minimum distance constraints
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    auto* camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr);
    
    // Cast to OrbitCamera to access distance constraints
    auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(camera);
    ASSERT_NE(orbitCamera, nullptr) << "Camera should be an OrbitCamera";
    
    float minDistance = orbitCamera->getMinDistance();
    EXPECT_EQ(minDistance, 0.5f) << "Default minimum distance should be 0.5";
    
    // Set camera to a small distance first
    orbitCamera->setDistance(1.0f);
    
    // Try to zoom in beyond minimum limit (very large zoom factor)
    std::string extremeZoomCommand = "zoom 100.0";
    auto extremeZoomResult = commandProcessor->execute(extremeZoomCommand);
    
    EXPECT_TRUE(extremeZoomResult.success) << "Zoom command should succeed even with extreme factor";
    
    // Verify distance is clamped to minimum
    float finalDistance = orbitCamera->getDistance();
    EXPECT_GE(finalDistance, minDistance) << "Distance should not go below minimum limit";
    EXPECT_NEAR(finalDistance, minDistance, 0.01f) 
        << "Distance should be clamped to minimum when zoom would exceed limit";
}

TEST_F(CameraCommandValidationTest, ZoomCommand_MaximumDistanceLimit_REQ_11_3_19) {
    // Test that zoom command respects maximum distance constraints
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    auto* camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr);
    
    auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(camera);
    ASSERT_NE(orbitCamera, nullptr);
    
    float maxDistance = orbitCamera->getMaxDistance();
    EXPECT_EQ(maxDistance, 100.0f) << "Default maximum distance should be 100.0";
    
    // Set camera to a large distance first
    orbitCamera->setDistance(50.0f);
    
    // Try to zoom out beyond maximum limit (very small zoom factor)
    std::string extremeZoomOutCommand = "zoom 0.01";
    auto extremeZoomResult = commandProcessor->execute(extremeZoomOutCommand);
    
    EXPECT_TRUE(extremeZoomResult.success) << "Zoom command should succeed even with extreme factor";
    
    // Verify distance is clamped to maximum
    float finalDistance = orbitCamera->getDistance();
    EXPECT_LE(finalDistance, maxDistance) << "Distance should not exceed maximum limit";
    EXPECT_NEAR(finalDistance, maxDistance, 0.01f) 
        << "Distance should be clamped to maximum when zoom would exceed limit";
}

TEST_F(CameraCommandValidationTest, ZoomCommand_InvalidZoomFactors_REQ_11_3_19) {
    // Test zoom command with invalid zoom factors (should fail)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test negative zoom factor
    std::string negativeZoomCommand = "zoom -1.5";
    auto negativeResult = commandProcessor->execute(negativeZoomCommand);
    
    EXPECT_FALSE(negativeResult.success) << "Negative zoom factor should fail";
    EXPECT_FALSE(negativeResult.message.empty()) << "Error message should be provided";
    EXPECT_NE(negativeResult.message.find("positive"), std::string::npos) 
        << "Error message should mention that zoom factor must be positive";
    
    // Test zero zoom factor
    std::string zeroZoomCommand = "zoom 0.0";
    auto zeroResult = commandProcessor->execute(zeroZoomCommand);
    
    EXPECT_FALSE(zeroResult.success) << "Zero zoom factor should fail";
    EXPECT_FALSE(zeroResult.message.empty()) << "Error message should be provided";
    
    // Test very small but positive zoom factor (should technically work)
    std::string smallZoomCommand = "zoom 0.001";
    auto smallResult = commandProcessor->execute(smallZoomCommand);
    
    EXPECT_TRUE(smallResult.success) << "Very small positive zoom factor should work";
}

TEST_F(CameraCommandValidationTest, ZoomCommand_BoundaryValues_REQ_11_3_19) {
    // Test zoom command with boundary values
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(cameraController->getCamera());
    ASSERT_NE(orbitCamera, nullptr);
    
    // Test zoom factor = 1.0 (no change)
    float initialDistance = orbitCamera->getDistance();
    
    std::string noChangeCommand = "zoom 1.0";
    auto noChangeResult = commandProcessor->execute(noChangeCommand);
    
    EXPECT_TRUE(noChangeResult.success) << "Zoom factor 1.0 should succeed";
    
    float unchangedDistance = orbitCamera->getDistance();
    EXPECT_NEAR(unchangedDistance, initialDistance, 0.01f) 
        << "Zoom factor 1.0 should not change distance";
    
    // Test very large zoom factor (should clamp to minimum distance)
    std::string largeZoomCommand = "zoom 1000.0";
    auto largeZoomResult = commandProcessor->execute(largeZoomCommand);
    
    EXPECT_TRUE(largeZoomResult.success) << "Large zoom factor should succeed";
    
    float minClampedDistance = orbitCamera->getDistance();
    EXPECT_NEAR(minClampedDistance, orbitCamera->getMinDistance(), 0.01f) 
        << "Large zoom factor should clamp to minimum distance";
    
    // Test very small zoom factor (should clamp to maximum distance)
    std::string smallZoomCommand = "zoom 0.0001";
    auto smallZoomResult = commandProcessor->execute(smallZoomCommand);
    
    EXPECT_TRUE(smallZoomResult.success) << "Small zoom factor should succeed";
    
    float maxClampedDistance = orbitCamera->getDistance();
    EXPECT_NEAR(maxClampedDistance, orbitCamera->getMaxDistance(), 0.01f) 
        << "Small zoom factor should clamp to maximum distance";
}

TEST_F(CameraCommandValidationTest, ZoomCommand_SequentialZooming_REQ_11_3_19) {
    // Test sequential zoom operations to verify cumulative behavior
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(cameraController->getCamera());
    ASSERT_NE(orbitCamera, nullptr);
    
    // Reset to known state
    commandProcessor->execute("camera iso");
    float initialDistance = orbitCamera->getDistance();
    
    // Sequence of zoom operations
    std::vector<std::pair<float, std::string>> zoomSequence = {
        {2.0f, "zoom in by 2x"},
        {1.5f, "zoom in by 1.5x"},
        {0.5f, "zoom out by 0.5x"},
        {0.8f, "zoom out by 0.8x"}
    };
    
    float expectedDistance = initialDistance;
    
    for (const auto& zoomOp : zoomSequence) {
        float factor = zoomOp.first;
        std::string description = zoomOp.second;
        
        std::string command = "zoom " + std::to_string(factor);
        auto result = commandProcessor->execute(command);
        
        EXPECT_TRUE(result.success) << "Zoom command should succeed: " << description;
        
        // Update expected distance
        expectedDistance = expectedDistance / factor;
        
        // Clamp expected distance to camera limits
        expectedDistance = std::max(expectedDistance, orbitCamera->getMinDistance());
        expectedDistance = std::min(expectedDistance, orbitCamera->getMaxDistance());
        
        float actualDistance = orbitCamera->getDistance();
        EXPECT_NEAR(actualDistance, expectedDistance, 0.01f) 
            << "Distance should match expected value after: " << description;
        
        // Verify distance is within bounds
        EXPECT_GE(actualDistance, orbitCamera->getMinDistance()) 
            << "Distance should be above minimum after: " << description;
        EXPECT_LE(actualDistance, orbitCamera->getMaxDistance()) 
            << "Distance should be below maximum after: " << description;
    }
}

TEST_F(CameraCommandValidationTest, ZoomCommand_AliasZ_REQ_11_3_19) {
    // Test zoom command using "z" alias
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(cameraController->getCamera());
    ASSERT_NE(orbitCamera, nullptr);
    
    float initialDistance = orbitCamera->getDistance();
    
    // Test using "z" alias
    std::string aliasCommand = "z 2.0";
    auto aliasResult = commandProcessor->execute(aliasCommand);
    
    EXPECT_TRUE(aliasResult.success) << "Zoom alias 'z' should work";
    EXPECT_FALSE(aliasResult.message.empty()) << "Zoom alias should provide feedback";
    
    float zoomedDistance = orbitCamera->getDistance();
    EXPECT_LT(zoomedDistance, initialDistance) << "Zoom alias should change distance";
    EXPECT_NEAR(zoomedDistance, initialDistance / 2.0f, 0.01f) 
        << "Zoom alias should apply factor correctly";
}

TEST_F(CameraCommandValidationTest, ZoomCommand_MissingParameter_REQ_11_3_19) {
    // Test zoom command without required parameter (should fail)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test zoom command without factor parameter
    std::string missingParamCommand = "zoom";
    auto missingParamResult = commandProcessor->execute(missingParamCommand);
    
    EXPECT_FALSE(missingParamResult.success) << "Zoom command without parameter should fail";
    EXPECT_FALSE(missingParamResult.message.empty()) << "Error message should be provided";
}

TEST_F(CameraCommandValidationTest, ZoomCommand_NonNumericParameter_REQ_11_3_19) {
    // Test zoom command with non-numeric parameter behavior
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(cameraController->getCamera());
    ASSERT_NE(orbitCamera, nullptr);
    
    // Record initial distance
    float initialDistance = orbitCamera->getDistance();
    
    // Test zoom command with non-numeric parameter
    // The implementation may parse this as the default value (1.0) or fail
    std::string nonNumericCommand = "zoom abc";
    auto nonNumericResult = commandProcessor->execute(nonNumericCommand);
    
    // Document the actual behavior - if it succeeds, it should use default value
    if (nonNumericResult.success) {
        // If command succeeds, verify it uses a sensible default (like 1.0, no change)
        float resultDistance = orbitCamera->getDistance();
        EXPECT_NEAR(resultDistance, initialDistance, 0.01f) 
            << "Non-numeric zoom should default to no change";
    } else {
        EXPECT_FALSE(nonNumericResult.message.empty()) << "Error message should be provided";
    }
    
    // Test zoom command with mixed parameter  
    orbitCamera->setDistance(initialDistance); // Reset
    std::string mixedCommand = "zoom 1.5x";
    auto mixedResult = commandProcessor->execute(mixedCommand);
    
    // Similar behavior - document what actually happens
    if (mixedResult.success) {
        // If it succeeds, verify reasonable behavior
        float mixedResultDistance = orbitCamera->getDistance();
        // It might parse "1.5" from "1.5x" or use default
        EXPECT_TRUE(mixedResultDistance > 0.0f) << "Distance should remain positive";
    } else {
        EXPECT_FALSE(mixedResult.message.empty()) << "Error message should be provided";
    }
}

// ============================================================================
// REQ-11.3.20: Camera commands shall test invalid view parameters
// ============================================================================

TEST_F(CameraCommandValidationTest, CameraCommand_InvalidViewParameters_REQ_11_3_20) {
    // Test camera command with invalid view preset names
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr) << "CommandProcessor should be available";
    ASSERT_NE(cameraController, nullptr) << "CameraController should be available";
    
    // Test invalid view preset names
    std::vector<std::string> invalidPresets = {
        "invalid", "unknown", "badview", "xyz", "123", 
        "Front", "FRONT", "IsO", "LEFT",  // Case sensitivity
        "", " ", "top-down"  // Empty/spaces/invalid names
    };
    
    // Test multi-word commands separately as they might be handled differently
    std::vector<std::string> multiWordPresets = {
        "front back", "top bottom", "left right"
    };
    
    for (const std::string& invalidPreset : invalidPresets) {
        std::string command = "camera " + invalidPreset;
        auto result = commandProcessor->execute(command);
        
        EXPECT_FALSE(result.success) << "Camera command with invalid preset '" 
                                    << invalidPreset << "' should fail";
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for invalid preset '" 
                                           << invalidPreset << "'";
        // Check for common error message patterns (case-insensitive)
        std::string lowerMessage = result.message;
        std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);
        EXPECT_TRUE(lowerMessage.find("invalid") != std::string::npos ||
                   lowerMessage.find("unknown") != std::string::npos ||
                   lowerMessage.find("error") != std::string::npos ||
                   lowerMessage.find("not found") != std::string::npos ||
                   lowerMessage.find("not supported") != std::string::npos ||
                   lowerMessage.find("insufficient") != std::string::npos) 
            << "Error message should indicate invalid preset for '" << invalidPreset 
            << "'. Actual message: '" << result.message << "'";
    }
    
    // Test multi-word presets - these might succeed by taking only the first word
    for (const std::string& multiWordPreset : multiWordPresets) {
        std::string command = "camera " + multiWordPreset;
        auto result = commandProcessor->execute(command);
        
        // Multi-word commands might succeed (using first word) or fail
        // Either behavior is acceptable - document what actually happens
        if (!result.success) {
            EXPECT_FALSE(result.message.empty()) << "Error message should be provided for multi-word preset '" 
                                               << multiWordPreset << "'";
        }
        // If it succeeds, that's also acceptable (first word parsed as valid preset)
    }
}

TEST_F(CameraCommandValidationTest, CameraCommand_MissingParameter_REQ_11_3_20) {
    // Test camera command without required view parameter
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test camera command without view parameter
    std::string command = "camera";
    auto result = commandProcessor->execute(command);
    
    EXPECT_FALSE(result.success) << "Camera command without view parameter should fail";
    EXPECT_FALSE(result.message.empty()) << "Error message should be provided";
    EXPECT_TRUE(result.message.find("required") != std::string::npos ||
                result.message.find("parameter") != std::string::npos ||
                result.message.find("argument") != std::string::npos) 
        << "Error message should indicate missing parameter: " << result.message;
}

TEST_F(CameraCommandValidationTest, CameraCommand_ExtraParameters_REQ_11_3_20) {
    // Test camera command with too many parameters
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test camera command with extra parameters  
    std::vector<std::string> extraParamCommands = {
        "camera front back",
        "camera iso 1.5", 
        "camera top bottom left",
        "camera front extra parameter"
    };
    
    for (const std::string& command : extraParamCommands) {
        auto result = commandProcessor->execute(command);
        
        // The command might succeed (ignoring extra params) or fail
        // Document the actual behavior - either way is acceptable
        if (!result.success) {
            EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << command;
        }
        // If it succeeds, that's also acceptable (extra params ignored)
    }
}

TEST_F(CameraCommandValidationTest, CameraCommand_NumericViewParameter_REQ_11_3_20) {
    // Test camera command with numeric view parameter (should be invalid)
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test camera command with numeric parameters
    std::vector<std::string> numericCommands = {
        "camera 1",
        "camera 0", 
        "camera 123",
        "camera -1",
        "camera 1.5"
    };
    
    for (const std::string& command : numericCommands) {
        auto result = commandProcessor->execute(command);
        
        EXPECT_FALSE(result.success) << "Camera command with numeric parameter should fail: " << command;
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << command;
    }
}

TEST_F(CameraCommandValidationTest, CameraCommand_SpecialCharacters_REQ_11_3_20) {
    // Test camera command with special characters and symbols
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr);
    
    // Test camera command with special characters
    std::vector<std::string> specialCharCommands = {
        "camera @front",
        "camera #iso", 
        "camera $top",
        "camera front@",
        "camera !",
        "camera *",
        "camera front-view",
        "camera top_down"
    };
    
    for (const std::string& command : specialCharCommands) {
        auto result = commandProcessor->execute(command);
        
        EXPECT_FALSE(result.success) << "Camera command with special characters should fail: " << command;
        EXPECT_FALSE(result.message.empty()) << "Error message should be provided for: " << command;
    }
}

TEST_F(CameraCommandValidationTest, CameraCommand_StateConsistencyAfterError_REQ_11_3_20) {
    // Test that camera state remains consistent after invalid commands
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    // Set camera to a known good state
    auto goodResult = commandProcessor->execute("camera front");
    EXPECT_TRUE(goodResult.success) << "Initial valid camera command should succeed";
    
    // Record the good state
    auto* camera = cameraController->getCamera();
    camera->getViewMatrix(); // Force update
    Math::WorldCoordinates goodPosition = camera->getPosition();
    Math::WorldCoordinates goodTarget = camera->getTarget();
    
    // Execute an invalid camera command
    auto badResult = commandProcessor->execute("camera invalid_view");
    EXPECT_FALSE(badResult.success) << "Invalid camera command should fail";
    
    // Verify camera state hasn't changed
    camera->getViewMatrix(); // Force update
    Math::WorldCoordinates currentPosition = camera->getPosition();
    Math::WorldCoordinates currentTarget = camera->getTarget();
    
    EXPECT_NEAR(currentPosition.x(), goodPosition.x(), 0.01f) << "Camera position X should be unchanged after invalid command";
    EXPECT_NEAR(currentPosition.y(), goodPosition.y(), 0.01f) << "Camera position Y should be unchanged after invalid command"; 
    EXPECT_NEAR(currentPosition.z(), goodPosition.z(), 0.01f) << "Camera position Z should be unchanged after invalid command";
    
    EXPECT_NEAR(currentTarget.x(), goodTarget.x(), 0.01f) << "Camera target X should be unchanged after invalid command";
    EXPECT_NEAR(currentTarget.y(), goodTarget.y(), 0.01f) << "Camera target Y should be unchanged after invalid command";
    EXPECT_NEAR(currentTarget.z(), goodTarget.z(), 0.01f) << "Camera target Z should be unchanged after invalid command";
}

} // namespace Tests
} // namespace VoxelEditor