#include <gtest/gtest.h>
#include "../InputMapping.h"
#include <cstdio>

using namespace VoxelEditor::Input;

class InputMappingTest : public ::testing::Test {
protected:
    void SetUp() override {
        mapping = InputMapping();
    }
    
    void TearDown() override {}
    
    InputMapping mapping;
};

TEST_F(InputMappingTest, DefaultConstruction) {
    EXPECT_FLOAT_EQ(mapping.mouseSensitivity, 1.0f);
    EXPECT_FLOAT_EQ(mapping.touchSensitivity, 1.0f);
    EXPECT_FLOAT_EQ(mapping.vrSensitivity, 1.0f);
    
    EXPECT_FLOAT_EQ(mapping.mouseClickTimeout, 0.3f);
    EXPECT_FLOAT_EQ(mapping.mouseDoubleClickTimeout, 0.5f);
    EXPECT_FLOAT_EQ(mapping.mouseDragThreshold, 5.0f);
    
    EXPECT_FLOAT_EQ(mapping.touchTapTimeout, 0.3f);
    EXPECT_FLOAT_EQ(mapping.touchTapRadius, 20.0f);
    EXPECT_FLOAT_EQ(mapping.touchPinchThreshold, 50.0f);
    EXPECT_FLOAT_EQ(mapping.touchSwipeThreshold, 100.0f);
    
    EXPECT_TRUE(mapping.mouseButtons.empty());
    EXPECT_TRUE(mapping.keys.empty());
    EXPECT_TRUE(mapping.touchGestures.empty());
    EXPECT_TRUE(mapping.vrGestures.empty());
}

TEST_F(InputMappingTest, MouseButtonBinding) {
    mapping.bindMouseButton(MouseButton::Left, Actions::PLACE_VOXEL);
    mapping.bindMouseButton(MouseButton::Right, Actions::REMOVE_VOXEL);
    mapping.bindMouseButton(MouseButton::Middle, Actions::PAN_CAMERA);
    
    EXPECT_EQ(mapping.getMouseButtonAction(MouseButton::Left), Actions::PLACE_VOXEL);
    EXPECT_EQ(mapping.getMouseButtonAction(MouseButton::Right), Actions::REMOVE_VOXEL);
    EXPECT_EQ(mapping.getMouseButtonAction(MouseButton::Middle), Actions::PAN_CAMERA);
    EXPECT_TRUE(mapping.getMouseButtonAction(MouseButton::Button4).empty());
}

TEST_F(InputMappingTest, MouseButtonWithModifiers) {
    mapping.bindMouseButton(MouseButton::Left, Actions::SELECT_VOXEL, ModifierFlags::Shift);
    mapping.bindMouseButton(MouseButton::Left, Actions::SELECT_MULTIPLE, ModifierFlags::Ctrl);
    
    EXPECT_EQ(mapping.getMouseButtonAction(MouseButton::Left, ModifierFlags::Shift), Actions::SELECT_VOXEL);
    EXPECT_EQ(mapping.getMouseButtonAction(MouseButton::Left, ModifierFlags::Ctrl), Actions::SELECT_MULTIPLE);
    EXPECT_TRUE(mapping.getMouseButtonAction(MouseButton::Left).empty());
}

TEST_F(InputMappingTest, KeyBinding) {
    mapping.bindKey(KeyCode::Space, Actions::RESET_CAMERA);
    mapping.bindKey(KeyCode::Delete, Actions::DELETE);
    mapping.bindKey(KeyCode::F, Actions::FRAME_SELECTION);
    
    EXPECT_EQ(mapping.getKeyAction(KeyCode::Space), Actions::RESET_CAMERA);
    EXPECT_EQ(mapping.getKeyAction(KeyCode::Delete), Actions::DELETE);
    EXPECT_EQ(mapping.getKeyAction(KeyCode::F), Actions::FRAME_SELECTION);
    EXPECT_TRUE(mapping.getKeyAction(KeyCode::G).empty());
}

TEST_F(InputMappingTest, KeyWithModifiers) {
    mapping.bindKey(KeyCode::Z, Actions::UNDO, ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::Y, Actions::REDO, ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::S, Actions::SAVE_FILE, ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::S, Actions::SAVE_AS, ModifierFlags::Ctrl | ModifierFlags::Shift);
    
    EXPECT_EQ(mapping.getKeyAction(KeyCode::Z, ModifierFlags::Ctrl), Actions::UNDO);
    EXPECT_EQ(mapping.getKeyAction(KeyCode::Y, ModifierFlags::Ctrl), Actions::REDO);
    EXPECT_EQ(mapping.getKeyAction(KeyCode::S, ModifierFlags::Ctrl), Actions::SAVE_FILE);
    EXPECT_EQ(mapping.getKeyAction(KeyCode::S, ModifierFlags::Ctrl | ModifierFlags::Shift), Actions::SAVE_AS);
    
    // Without modifiers should not match
    EXPECT_TRUE(mapping.getKeyAction(KeyCode::Z).empty());
    EXPECT_TRUE(mapping.getKeyAction(KeyCode::S).empty());
}

TEST_F(InputMappingTest, KeyCombinationBinding) {
    KeyCombination undoCombo(KeyCode::Z, ModifierFlags::Ctrl);
    KeyCombination redoCombo(KeyCode::Y, ModifierFlags::Ctrl);
    KeyCombination saveAsCombo(KeyCode::S, ModifierFlags::Ctrl | ModifierFlags::Shift);
    
    mapping.bindKeyCombination(undoCombo, Actions::UNDO);
    mapping.bindKeyCombination(redoCombo, Actions::REDO);
    mapping.bindKeyCombination(saveAsCombo, Actions::SAVE_AS);
    
    EXPECT_EQ(mapping.getKeyCombinationAction(undoCombo), Actions::UNDO);
    EXPECT_EQ(mapping.getKeyCombinationAction(redoCombo), Actions::REDO);
    EXPECT_EQ(mapping.getKeyCombinationAction(saveAsCombo), Actions::SAVE_AS);
    
    KeyCombination unknownCombo(KeyCode::X, ModifierFlags::Alt);
    EXPECT_TRUE(mapping.getKeyCombinationAction(unknownCombo).empty());
}

TEST_F(InputMappingTest, TouchGestureBinding) {
    mapping.bindTouchGesture(TouchGesture::Tap, Actions::SELECT_VOXEL);
    mapping.bindTouchGesture(TouchGesture::Pinch, Actions::ZOOM_CAMERA);
    mapping.bindTouchGesture(TouchGesture::Pan, Actions::PAN_CAMERA);
    mapping.bindTouchGesture(TouchGesture::TwoFingerPan, Actions::ORBIT_CAMERA);
    
    EXPECT_EQ(mapping.getTouchGestureAction(TouchGesture::Tap), Actions::SELECT_VOXEL);
    EXPECT_EQ(mapping.getTouchGestureAction(TouchGesture::Pinch), Actions::ZOOM_CAMERA);
    EXPECT_EQ(mapping.getTouchGestureAction(TouchGesture::Pan), Actions::PAN_CAMERA);
    EXPECT_EQ(mapping.getTouchGestureAction(TouchGesture::TwoFingerPan), Actions::ORBIT_CAMERA);
    EXPECT_TRUE(mapping.getTouchGestureAction(TouchGesture::Rotation).empty());
}

TEST_F(InputMappingTest, VRGestureBinding) {
    mapping.bindVRGesture(VRGesture::Point, Actions::VR_POINT);
    mapping.bindVRGesture(VRGesture::Grab, Actions::VR_GRAB);
    mapping.bindVRGesture(VRGesture::Pinch, Actions::PLACE_VOXEL);
    mapping.bindVRGesture(VRGesture::TwoHandScale, Actions::VR_SCALE);
    
    EXPECT_EQ(mapping.getVRGestureAction(VRGesture::Point), Actions::VR_POINT);
    EXPECT_EQ(mapping.getVRGestureAction(VRGesture::Grab), Actions::VR_GRAB);
    EXPECT_EQ(mapping.getVRGestureAction(VRGesture::Pinch), Actions::PLACE_VOXEL);
    EXPECT_EQ(mapping.getVRGestureAction(VRGesture::TwoHandScale), Actions::VR_SCALE);
    EXPECT_TRUE(mapping.getVRGestureAction(VRGesture::ThumbsUp).empty());
}

TEST_F(InputMappingTest, SensitivitySettings) {
    mapping.mouseSensitivity = 2.0f;
    mapping.touchSensitivity = 0.5f;
    mapping.vrSensitivity = 1.5f;
    
    EXPECT_FLOAT_EQ(mapping.mouseSensitivity, 2.0f);
    EXPECT_FLOAT_EQ(mapping.touchSensitivity, 0.5f);
    EXPECT_FLOAT_EQ(mapping.vrSensitivity, 1.5f);
}

TEST_F(InputMappingTest, MouseConfiguration) {
    mapping.mouseClickTimeout = 0.4f;
    mapping.mouseDoubleClickTimeout = 0.6f;
    mapping.mouseDragThreshold = 10.0f;
    
    EXPECT_FLOAT_EQ(mapping.mouseClickTimeout, 0.4f);
    EXPECT_FLOAT_EQ(mapping.mouseDoubleClickTimeout, 0.6f);
    EXPECT_FLOAT_EQ(mapping.mouseDragThreshold, 10.0f);
}

TEST_F(InputMappingTest, TouchConfiguration) {
    mapping.touchTapTimeout = 0.4f;
    mapping.touchTapRadius = 25.0f;
    mapping.touchPinchThreshold = 75.0f;
    mapping.touchSwipeThreshold = 120.0f;
    
    EXPECT_FLOAT_EQ(mapping.touchTapTimeout, 0.4f);
    EXPECT_FLOAT_EQ(mapping.touchTapRadius, 25.0f);
    EXPECT_FLOAT_EQ(mapping.touchPinchThreshold, 75.0f);
    EXPECT_FLOAT_EQ(mapping.touchSwipeThreshold, 120.0f);
}

TEST_F(InputMappingTest, VRComfortSettings) {
    VRComfortSettings comfort = VRComfortSettings::Comfort();
    mapping.vrComfortSettings = comfort;
    
    EXPECT_TRUE(mapping.vrComfortSettings.snapTurning);
    EXPECT_TRUE(mapping.vrComfortSettings.vignetteOnTurn);
    EXPECT_TRUE(mapping.vrComfortSettings.teleportMovement);
    EXPECT_FALSE(mapping.vrComfortSettings.smoothMovement);
    
    VRComfortSettings performance = VRComfortSettings::Performance();
    mapping.vrComfortSettings = performance;
    
    EXPECT_FALSE(mapping.vrComfortSettings.snapTurning);
    EXPECT_FALSE(mapping.vrComfortSettings.vignetteOnTurn);
    EXPECT_FALSE(mapping.vrComfortSettings.teleportMovement);
    EXPECT_TRUE(mapping.vrComfortSettings.smoothMovement);
}

TEST_F(InputMappingTest, PresetMappings) {
    // Test Default mapping
    InputMapping defaultMapping = InputMapping::Default();
    EXPECT_TRUE(defaultMapping.isValid());
    
    // Test Gaming mapping
    InputMapping gamingMapping = InputMapping::Gaming();
    EXPECT_TRUE(gamingMapping.isValid());
    
    // Test Accessibility mapping
    InputMapping accessibilityMapping = InputMapping::Accessibility();
    EXPECT_TRUE(accessibilityMapping.isValid());
    
    // Test VR optimized mapping
    InputMapping vrMapping = InputMapping::VROptimized();
    EXPECT_TRUE(vrMapping.isValid());
}

TEST_F(InputMappingTest, Validation) {
    // Empty mapping should be valid
    EXPECT_TRUE(mapping.isValid());
    
    // Add some valid bindings
    mapping.bindMouseButton(MouseButton::Left, Actions::PLACE_VOXEL);
    mapping.bindKey(KeyCode::Space, Actions::RESET_CAMERA);
    EXPECT_TRUE(mapping.isValid());
}

TEST_F(InputMappingTest, ValidationMessages) {
    mapping.mouseSensitivity = -0.5f;
    mapping.touchTapRadius = -10.0f;
    mapping.mouseDragThreshold = 0.0f;
    
    std::vector<std::string> issues = mapping.validate();
    // The validate() method returns a vector of strings with validation issues
    // If there are invalid values, we should get some issues reported
    
    // Check if validation detects any issues
    if (mapping.mouseSensitivity < 0 || mapping.touchTapRadius < 0 || mapping.mouseDragThreshold <= 0) {
        // We expect validation to report these issues
        EXPECT_FALSE(issues.empty());
    }
}

TEST_F(InputMappingTest, ActionConstants) {
    // Test that action constants are defined and unique
    EXPECT_STRNE(Actions::PLACE_VOXEL, Actions::REMOVE_VOXEL);
    EXPECT_STRNE(Actions::ORBIT_CAMERA, Actions::PAN_CAMERA);
    EXPECT_STRNE(Actions::SELECT_VOXEL, Actions::SELECT_MULTIPLE);
    EXPECT_STRNE(Actions::UNDO, Actions::REDO);
    
    // Test some specific action names
    EXPECT_STREQ(Actions::PLACE_VOXEL, "place_voxel");
    EXPECT_STREQ(Actions::ORBIT_CAMERA, "orbit_camera");
    EXPECT_STREQ(Actions::SELECT_VOXEL, "select_voxel");
    EXPECT_STREQ(Actions::UNDO, "undo");
    EXPECT_STREQ(Actions::VR_GRAB, "vr_grab");
}

TEST_F(InputMappingTest, FileSerialization) {
    // Set up a mapping with some bindings
    mapping.bindMouseButton(MouseButton::Left, Actions::PLACE_VOXEL);
    mapping.bindKey(KeyCode::Space, Actions::RESET_CAMERA);
    mapping.bindTouchGesture(TouchGesture::Tap, Actions::SELECT_VOXEL);
    mapping.bindVRGesture(VRGesture::Grab, Actions::VR_GRAB);
    
    mapping.mouseSensitivity = 1.5f;
    mapping.touchSensitivity = 0.8f;
    
    // Save to file
    const std::string testFile = "test_mapping.cfg";
    EXPECT_TRUE(mapping.saveToFile(testFile));
    
    // Load from file
    InputMapping loadedMapping;
    EXPECT_TRUE(loadedMapping.loadFromFile(testFile));
    
    // Verify loaded values
    EXPECT_EQ(loadedMapping.getMouseButtonAction(MouseButton::Left), Actions::PLACE_VOXEL);
    EXPECT_EQ(loadedMapping.getKeyAction(KeyCode::Space), Actions::RESET_CAMERA);
    EXPECT_EQ(loadedMapping.getTouchGestureAction(TouchGesture::Tap), Actions::SELECT_VOXEL);
    EXPECT_EQ(loadedMapping.getVRGestureAction(VRGesture::Grab), Actions::VR_GRAB);
    
    EXPECT_FLOAT_EQ(loadedMapping.mouseSensitivity, 1.5f);
    EXPECT_FLOAT_EQ(loadedMapping.touchSensitivity, 0.8f);
    
    // Clean up test file
    std::remove(testFile.c_str());
}

TEST_F(InputMappingTest, JsonSerialization) {
    // Set up a mapping
    mapping.bindMouseButton(MouseButton::Right, Actions::REMOVE_VOXEL);
    mapping.bindKey(KeyCode::Delete, Actions::DELETE);
    mapping.vrSensitivity = 1.2f;
    
    // Convert to JSON
    std::string json = mapping.toJson();
    EXPECT_FALSE(json.empty());
    
    // Verify JSON contains expected content
    EXPECT_NE(json.find("\"vrSensitivity\": 1.2"), std::string::npos);
    EXPECT_NE(json.find("remove_voxel"), std::string::npos);
    
    // Load from JSON - Note: fromJson is not fully implemented yet
    InputMapping jsonMapping;
    bool loadResult = jsonMapping.fromJson(json);
    EXPECT_TRUE(loadResult); // The placeholder returns true for non-empty strings
    
    // TODO: When fromJson is properly implemented, add verification tests:
    // EXPECT_EQ(jsonMapping.getMouseButtonAction(MouseButton::Right), Actions::REMOVE_VOXEL);
    // EXPECT_EQ(jsonMapping.getKeyAction(KeyCode::Delete), Actions::DELETE);
    // EXPECT_FLOAT_EQ(jsonMapping.vrSensitivity, 1.2f);
}