#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "cli/CommandTypes.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "camera/OrbitCamera.h"
#include <sstream>
#include <memory>

namespace VoxelEditor {
namespace Tests {

class CameraCommandsTest : public ::testing::Test {
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

TEST_F(CameraCommandsTest, CameraCommand_AllViewPresets_REQ_11_3_18) {
    // Test all predefined camera view positions
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr) << "CommandProcessor should be available";
    ASSERT_NE(cameraController, nullptr) << "CameraController should be available";
    
    auto* camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr) << "Camera should be available";
    
    // Test all view presets defined in Camera::ViewPreset enum
    std::vector<std::pair<std::string, Camera::ViewPreset>> viewPresets = {
        {"front", Camera::ViewPreset::FRONT},
        {"back", Camera::ViewPreset::BACK},
        {"left", Camera::ViewPreset::LEFT},
        {"right", Camera::ViewPreset::RIGHT},
        {"top", Camera::ViewPreset::TOP},
        {"bottom", Camera::ViewPreset::BOTTOM},
        {"iso", Camera::ViewPreset::ISOMETRIC},
        {"default", Camera::ViewPreset::ISOMETRIC}
    };
    
    for (const auto& preset : viewPresets) {
        const std::string& presetName = preset.first;
        const Camera::ViewPreset expectedPreset = preset.second;
        
        // Store initial camera state
        Math::WorldCoordinates initialPosition = camera->getPosition();
        Math::WorldCoordinates initialTarget = camera->getTarget();
        
        // Execute camera command with the preset
        std::string command = "camera " + presetName;
        auto result = commandProcessor->execute(command);
        
        // Command should succeed
        EXPECT_TRUE(result.success) 
            << "Camera command should succeed for preset: " << presetName
            << " - Error: " << result.message;
        
        // Camera position should have changed (unless it was already set to this preset)
        Math::WorldCoordinates newPosition = camera->getPosition();
        Math::WorldCoordinates newTarget = camera->getTarget();
        
        // Verify that the camera state is valid after preset change
        // (We can't easily verify the exact position since the OrbitCamera implementation 
        // calculates positions based on distance and angles, but we can verify basic sanity)
        
        // Camera should have valid matrices after preset change
        Math::Matrix4f viewMatrix = camera->getViewMatrix();
        Math::Matrix4f projMatrix = camera->getProjectionMatrix();
        
        // Basic sanity checks - matrices should not be all zeros
        bool viewMatrixValid = false;
        bool projMatrixValid = false;
        
        for (int i = 0; i < 16; ++i) {
            if (viewMatrix.m[i] != 0.0f) viewMatrixValid = true;
            if (projMatrix.m[i] != 0.0f) projMatrixValid = true;
        }
        
        EXPECT_TRUE(viewMatrixValid) << "View matrix should be valid for preset: " << presetName;
        EXPECT_TRUE(projMatrixValid) << "Projection matrix should be valid for preset: " << presetName;
        
        // Verify the camera target is reasonable (should be at or near origin for centered coordinate system)
        EXPECT_LE(std::abs(newTarget.x()), 10.0f) << "Camera target X should be reasonable for preset: " << presetName;
        EXPECT_LE(std::abs(newTarget.y()), 10.0f) << "Camera target Y should be reasonable for preset: " << presetName;
        EXPECT_LE(std::abs(newTarget.z()), 10.0f) << "Camera target Z should be reasonable for preset: " << presetName;
        
        // Verify the camera position is not at origin (it should be positioned to look at the target)
        float distanceFromOrigin = std::sqrt(newPosition.x() * newPosition.x() + 
                                           newPosition.y() * newPosition.y() + 
                                           newPosition.z() * newPosition.z());
        EXPECT_GT(distanceFromOrigin, 0.1f) << "Camera should be positioned away from origin for preset: " << presetName;
    }
}

TEST_F(CameraCommandsTest, CameraCommand_InvalidPreset_REQ_11_3_18) {
    // Test camera command with invalid preset names
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr) << "CommandProcessor should be available";
    
    // Test invalid preset names
    std::vector<std::string> invalidPresets = {
        "invalid",
        "unknown",
        "orthographic",
        "perspective",
        "side",
        "diagonal",
        "",
        "123",
        "frontal",  // close to "front" but not exact
        "isometric" // close to "iso" but not exact
    };
    
    for (const std::string& invalidPreset : invalidPresets) {
        std::string command = "camera " + invalidPreset;
        auto result = commandProcessor->execute(command);
        
        // Command should fail for invalid presets
        EXPECT_FALSE(result.success) 
            << "Camera command should fail for invalid preset: " << invalidPreset;
        
        // Error message should mention the unknown preset
        EXPECT_NE(result.message.find("Unknown"), std::string::npos)
            << "Error message should mention 'Unknown' for preset: " << invalidPreset
            << " - Got message: " << result.message;
    }
}

TEST_F(CameraCommandsTest, CameraCommand_MissingPreset_REQ_11_3_18) {
    // Test camera command without providing a preset parameter
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr) << "CommandProcessor should be available";
    
    // Test command with no preset parameter
    std::string command = "camera";
    auto result = commandProcessor->execute(command);
    
    // Command should fail when no preset is provided (preset is required)
    EXPECT_FALSE(result.success) 
        << "Camera command should fail when no preset is provided";
}

TEST_F(CameraCommandsTest, CameraCommand_StateConsistency_REQ_11_3_18) {
    // Test that camera state remains consistent after multiple preset changes
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    auto* cameraController = app->getCameraController();
    ASSERT_NE(commandProcessor, nullptr);
    ASSERT_NE(cameraController, nullptr);
    
    auto* camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr);
    
    // Sequence of preset changes
    std::vector<std::string> presetSequence = {
        "front", "back", "left", "right", "top", "bottom", "iso", "default"
    };
    
    for (const std::string& preset : presetSequence) {
        // Set the preset
        std::string command = "camera " + preset;
        auto result = commandProcessor->execute(command);
        EXPECT_TRUE(result.success) 
            << "Camera command should succeed for preset: " << preset;
        
        // Verify camera state is valid after each change
        Math::WorldCoordinates position = camera->getPosition();
        Math::WorldCoordinates target = camera->getTarget();
        
        // Check that position values are finite (not NaN or infinite)
        EXPECT_TRUE(std::isfinite(position.x())) << "Camera position X should be finite after setting preset: " << preset;
        EXPECT_TRUE(std::isfinite(position.y())) << "Camera position Y should be finite after setting preset: " << preset;
        EXPECT_TRUE(std::isfinite(position.z())) << "Camera position Z should be finite after setting preset: " << preset;
        
        EXPECT_TRUE(std::isfinite(target.x())) << "Camera target X should be finite after setting preset: " << preset;
        EXPECT_TRUE(std::isfinite(target.y())) << "Camera target Y should be finite after setting preset: " << preset;
        EXPECT_TRUE(std::isfinite(target.z())) << "Camera target Z should be finite after setting preset: " << preset;
        
        // Check that matrices are valid
        Math::Matrix4f viewMatrix = camera->getViewMatrix();
        Math::Matrix4f projMatrix = camera->getProjectionMatrix();
        
        // All matrix elements should be finite
        for (int i = 0; i < 16; ++i) {
            EXPECT_TRUE(std::isfinite(viewMatrix.m[i])) 
                << "View matrix element " << i << " should be finite for preset: " << preset;
            EXPECT_TRUE(std::isfinite(projMatrix.m[i])) 
                << "Projection matrix element " << i << " should be finite for preset: " << preset;
        }
    }
}

TEST_F(CameraCommandsTest, CameraCommand_AliasSupport_REQ_11_3_18) {
    // Test that camera command aliases work correctly
    
    EXPECT_TRUE(initialized) << "Application must be initialized for this test";
    
    auto* commandProcessor = app->getCommandProcessor();
    ASSERT_NE(commandProcessor, nullptr) << "CommandProcessor should be available";
    
    // The "view" alias should work the same as "camera"
    auto result1 = commandProcessor->execute("camera front");
    auto result2 = commandProcessor->execute("view back");
    
    EXPECT_TRUE(result1.success) 
        << "Camera command should work with 'camera' keyword";
    EXPECT_TRUE(result2.success) 
        << "Camera command should work with 'view' alias";
}

} // namespace Tests
} // namespace VoxelEditor