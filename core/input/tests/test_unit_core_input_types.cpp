#include <gtest/gtest.h>
#include "../InputTypes.h"

using namespace VoxelEditor::Input;
using namespace VoxelEditor::Math;

class InputTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(InputTypesTest, MouseEventConstruction) {
    // Default construction
    MouseEvent defaultEvent;
    EXPECT_EQ(defaultEvent.type, MouseEventType::Move);
    EXPECT_EQ(defaultEvent.button, MouseButton::None);
    EXPECT_EQ(defaultEvent.position, Vector2f::zero());
    EXPECT_EQ(defaultEvent.delta, Vector2f::zero());
    EXPECT_FLOAT_EQ(defaultEvent.wheelDelta, 0.0f);
    EXPECT_EQ(defaultEvent.modifiers, 0);
    
    // Custom construction
    Vector2f pos(100.0f, 200.0f);
    MouseEvent clickEvent(MouseEventType::ButtonPress, MouseButton::Left, pos);
    EXPECT_EQ(clickEvent.type, MouseEventType::ButtonPress);
    EXPECT_EQ(clickEvent.button, MouseButton::Left);
    EXPECT_EQ(clickEvent.position, pos);
}

TEST_F(InputTypesTest, KeyEventConstruction) {
    // Default construction
    KeyEvent defaultEvent;
    EXPECT_EQ(defaultEvent.type, KeyEventType::Press);
    EXPECT_EQ(defaultEvent.key, KeyCode::Unknown);
    EXPECT_EQ(defaultEvent.character, 0);
    EXPECT_EQ(defaultEvent.modifiers, ModifierFlags::None);
    EXPECT_FALSE(defaultEvent.repeat);
    
    // Custom construction
    KeyEvent keyEvent(KeyEventType::Press, KeyCode::A, ModifierFlags::Shift);
    EXPECT_EQ(keyEvent.type, KeyEventType::Press);
    EXPECT_EQ(keyEvent.key, KeyCode::A);
    EXPECT_EQ(keyEvent.modifiers, ModifierFlags::Shift);
}

TEST_F(InputTypesTest, TouchPointConstruction) {
    // Default construction
    TouchPoint defaultPoint;
    EXPECT_EQ(defaultPoint.id, -1);
    EXPECT_EQ(defaultPoint.position, Vector2f::zero());
    EXPECT_EQ(defaultPoint.delta, Vector2f::zero());
    EXPECT_FLOAT_EQ(defaultPoint.pressure, 0.0f);
    EXPECT_EQ(defaultPoint.state, TouchState::Released);
    
    // Custom construction
    Vector2f pos(50.0f, 75.0f);
    TouchPoint touchPoint(1, pos, TouchState::Pressed);
    EXPECT_EQ(touchPoint.id, 1);
    EXPECT_EQ(touchPoint.position, pos);
    EXPECT_EQ(touchPoint.state, TouchState::Pressed);
    EXPECT_FLOAT_EQ(touchPoint.pressure, 1.0f);
}

TEST_F(InputTypesTest, TouchEventConstruction) {
    // Default construction
    TouchEvent defaultEvent;
    EXPECT_EQ(defaultEvent.type, TouchEventType::TouchBegin);
    EXPECT_TRUE(defaultEvent.points.empty());
    
    // Custom construction
    std::vector<TouchPoint> points;
    points.emplace_back(1, Vector2f(10.0f, 20.0f), TouchState::Pressed);
    points.emplace_back(2, Vector2f(30.0f, 40.0f), TouchState::Pressed);
    
    TouchEvent touchEvent(TouchEventType::TouchUpdate, points);
    EXPECT_EQ(touchEvent.type, TouchEventType::TouchUpdate);
    EXPECT_EQ(touchEvent.points.size(), 2);
    EXPECT_EQ(touchEvent.points[0].id, 1);
    EXPECT_EQ(touchEvent.points[1].id, 2);
}

TEST_F(InputTypesTest, HandPoseConstruction) {
    // Default construction
    HandPose defaultPose;
    EXPECT_EQ(defaultPose.position, Vector3f::Zero());
    EXPECT_EQ(defaultPose.orientation, VoxelEditor::Math::Quaternion::identity());
    EXPECT_FLOAT_EQ(defaultPose.confidence, 0.0f);
    EXPECT_EQ(defaultPose.hand, HandType::Left);
    
    // Check finger initialization
    for (const auto& finger : defaultPose.fingers) {
        EXPECT_FLOAT_EQ(finger.bend, 0.0f);
        EXPECT_TRUE(finger.extended);
        for (const auto& joint : finger.joints) {
            EXPECT_EQ(joint, Vector3f::Zero());
        }
    }
}

TEST_F(InputTypesTest, VREventConstruction) {
    // Default construction
    VREvent defaultEvent;
    EXPECT_EQ(defaultEvent.type, VREventType::HandUpdate);
    EXPECT_EQ(defaultEvent.hand, HandType::Left);
    EXPECT_TRUE(defaultEvent.gestures.empty());
    
    // Custom construction
    HandPose pose;
    pose.position = Vector3f(1.0f, 2.0f, 3.0f);
    pose.confidence = 0.8f;
    pose.hand = HandType::Right;
    
    VREvent vrEvent(VREventType::GestureDetected, HandType::Right, pose);
    EXPECT_EQ(vrEvent.type, VREventType::GestureDetected);
    EXPECT_EQ(vrEvent.hand, HandType::Right);
    EXPECT_EQ(vrEvent.pose.position, Vector3f(1.0f, 2.0f, 3.0f));
    EXPECT_FLOAT_EQ(vrEvent.pose.confidence, 0.8f);
}

TEST_F(InputTypesTest, ModifierFlags) {
    // Test bitwise operations
    ModifierFlags combined = ModifierFlags::Shift | ModifierFlags::Ctrl;
    EXPECT_TRUE(hasModifier(combined, ModifierFlags::Shift));
    EXPECT_TRUE(hasModifier(combined, ModifierFlags::Ctrl));
    EXPECT_FALSE(hasModifier(combined, ModifierFlags::Alt));
    
    // Test individual flags
    EXPECT_TRUE(hasModifier(ModifierFlags::Alt, ModifierFlags::Alt));
    EXPECT_FALSE(hasModifier(ModifierFlags::None, ModifierFlags::Shift));
    
    // Test combination with all flags
    ModifierFlags allFlags = ModifierFlags::Shift | ModifierFlags::Ctrl | 
                            ModifierFlags::Alt | ModifierFlags::Super;
    EXPECT_TRUE(hasModifier(allFlags, ModifierFlags::Shift));
    EXPECT_TRUE(hasModifier(allFlags, ModifierFlags::Ctrl));
    EXPECT_TRUE(hasModifier(allFlags, ModifierFlags::Alt));
    EXPECT_TRUE(hasModifier(allFlags, ModifierFlags::Super));
}

TEST_F(InputTypesTest, InputTriggerMatching) {
    // Mouse trigger
    InputTrigger mouseTrigger(MouseButton::Left, ModifierFlags::Ctrl);
    
    MouseEvent mouseEvent(MouseEventType::ButtonPress, MouseButton::Left, Vector2f::zero());
    mouseEvent.modifiers = static_cast<uint32_t>(ModifierFlags::Ctrl);
    EXPECT_TRUE(mouseTrigger.matches(mouseEvent));
    
    mouseEvent.button = MouseButton::Right;
    EXPECT_FALSE(mouseTrigger.matches(mouseEvent));
    
    // Key trigger
    InputTrigger keyTrigger(KeyCode::A, ModifierFlags::Shift);
    
    KeyEvent keyEvent(KeyEventType::Press, KeyCode::A, ModifierFlags::Shift);
    EXPECT_TRUE(keyTrigger.matches(keyEvent));
    
    keyEvent.key = KeyCode::B;
    EXPECT_FALSE(keyTrigger.matches(keyEvent));
    
    // Touch trigger
    InputTrigger touchTrigger(TouchGesture::Tap);
    EXPECT_TRUE(touchTrigger.matches(TouchGesture::Tap));
    EXPECT_FALSE(touchTrigger.matches(TouchGesture::Pinch));
    
    // VR trigger
    InputTrigger vrTrigger(VRGesture::Point);
    EXPECT_TRUE(vrTrigger.matches(VRGesture::Point));
    EXPECT_FALSE(vrTrigger.matches(VRGesture::Grab));
}

TEST_F(InputTypesTest, KeyCombinationToString) {
    // Simple key
    KeyCombination simple(KeyCode::A);
    EXPECT_EQ(simple.toString(), "A");
    
    // Key with modifier
    KeyCombination withCtrl(KeyCode::C, ModifierFlags::Ctrl);
    EXPECT_EQ(withCtrl.toString(), "Ctrl+C");
    
    // Key with multiple modifiers
    KeyCombination withMultiple(KeyCode::V, ModifierFlags::Ctrl | ModifierFlags::Shift);
    std::string result = withMultiple.toString();
    EXPECT_TRUE(result.find("Ctrl") != std::string::npos);
    EXPECT_TRUE(result.find("Shift") != std::string::npos);
    EXPECT_TRUE(result.find("V") != std::string::npos);
    
    // Function key
    KeyCombination functionKey(KeyCode::F1);
    EXPECT_EQ(functionKey.toString(), "F1");
    
    // Special keys
    KeyCombination spaceKey(KeyCode::Space);
    EXPECT_EQ(spaceKey.toString(), "Space");
    
    KeyCombination enterKey(KeyCode::Enter);
    EXPECT_EQ(enterKey.toString(), "Enter");
}

TEST_F(InputTypesTest, KeyCombinationFromString) {
    // Simple key
    KeyCombination simple = KeyCombination::fromString("A");
    EXPECT_EQ(simple.primaryKey, KeyCode::A);
    EXPECT_EQ(simple.modifiers, ModifierFlags::None);
    
    // Key with modifier
    KeyCombination withCtrl = KeyCombination::fromString("Ctrl+C");
    EXPECT_EQ(withCtrl.primaryKey, KeyCode::C);
    EXPECT_TRUE(hasModifier(withCtrl.modifiers, ModifierFlags::Ctrl));
    
    // Function key
    KeyCombination functionKey = KeyCombination::fromString("F1");
    EXPECT_EQ(functionKey.primaryKey, KeyCode::F1);
    
    // Special keys
    KeyCombination spaceKey = KeyCombination::fromString("Space");
    EXPECT_EQ(spaceKey.primaryKey, KeyCode::Space);
}

TEST_F(InputTypesTest, KeyCombinationMatching) {
    KeyCombination combo(KeyCode::S, ModifierFlags::Ctrl | ModifierFlags::Shift);
    
    // Exact match
    EXPECT_TRUE(combo.matches(KeyCode::S, ModifierFlags::Ctrl | ModifierFlags::Shift));
    
    // Wrong key
    EXPECT_FALSE(combo.matches(KeyCode::A, ModifierFlags::Ctrl | ModifierFlags::Shift));
    
    // Wrong modifiers
    EXPECT_FALSE(combo.matches(KeyCode::S, ModifierFlags::Ctrl));
    EXPECT_FALSE(combo.matches(KeyCode::S, ModifierFlags::None));
    
    // No modifiers combo
    KeyCombination noMods(KeyCode::Escape);
    EXPECT_TRUE(noMods.matches(KeyCode::Escape, ModifierFlags::None));
    EXPECT_FALSE(noMods.matches(KeyCode::Escape, ModifierFlags::Shift));
}

TEST_F(InputTypesTest, ActionContext) {
    // Default construction
    ActionContext defaultContext;
    EXPECT_FALSE(defaultContext.pressed);
    EXPECT_FLOAT_EQ(defaultContext.value, 0.0f);
    EXPECT_EQ(defaultContext.vector2, Vector2f::zero());
    EXPECT_EQ(defaultContext.vector3, Vector3f::Zero());
    EXPECT_EQ(defaultContext.modifiers, ModifierFlags::None);
    EXPECT_EQ(defaultContext.device, InputDevice::Unknown);
    
    // Typed construction
    ActionContext buttonContext(ActionType::Button);
    EXPECT_EQ(buttonContext.type, ActionType::Button);
    
    ActionContext axisContext(ActionType::Axis);
    EXPECT_EQ(axisContext.type, ActionType::Axis);
}

TEST_F(InputTypesTest, ActionBinding) {
    // Default construction
    ActionBinding defaultBinding;
    EXPECT_EQ(defaultBinding.type, ActionType::Button);
    EXPECT_TRUE(defaultBinding.triggers.empty());
    EXPECT_FLOAT_EQ(defaultBinding.deadzone, 0.1f);
    EXPECT_FALSE(defaultBinding.continuous);
    
    // Custom construction
    ActionBinding customBinding("test_action", ActionType::Axis);
    EXPECT_EQ(customBinding.name, "test_action");
    EXPECT_EQ(customBinding.type, ActionType::Axis);
}

TEST_F(InputTypesTest, VRComfortSettings) {
    // Default settings
    VRComfortSettings defaultSettings = VRComfortSettings::Default();
    EXPECT_TRUE(defaultSettings.snapTurning);
    EXPECT_FLOAT_EQ(defaultSettings.snapTurnAngle, 30.0f);
    EXPECT_FALSE(defaultSettings.smoothTurning);
    EXPECT_TRUE(defaultSettings.teleportMovement);
    
    // Comfort settings
    VRComfortSettings comfortSettings = VRComfortSettings::Comfort();
    EXPECT_TRUE(comfortSettings.snapTurning);
    EXPECT_TRUE(comfortSettings.vignetteOnTurn);
    EXPECT_TRUE(comfortSettings.teleportMovement);
    EXPECT_FALSE(comfortSettings.smoothMovement);
    
    // Performance settings
    VRComfortSettings perfSettings = VRComfortSettings::Performance();
    EXPECT_FALSE(perfSettings.snapTurning);
    EXPECT_TRUE(perfSettings.smoothTurning);
    EXPECT_FALSE(perfSettings.vignetteOnTurn);
    EXPECT_FALSE(perfSettings.teleportMovement);
    EXPECT_TRUE(perfSettings.smoothMovement);
}

TEST_F(InputTypesTest, EnumValues) {
    // Test that enums have expected values
    EXPECT_EQ(static_cast<int>(MouseButton::Left), 0);
    EXPECT_EQ(static_cast<int>(MouseButton::Right), 1);
    EXPECT_EQ(static_cast<int>(MouseButton::Middle), 2);
    
    EXPECT_EQ(static_cast<int>(HandType::Left), 0);
    EXPECT_EQ(static_cast<int>(HandType::Right), 1);
    EXPECT_EQ(static_cast<int>(HandType::Either), 2);
    
    EXPECT_EQ(static_cast<int>(ActionType::Button), 0);
    EXPECT_EQ(static_cast<int>(ActionType::Axis), 1);
    EXPECT_EQ(static_cast<int>(ActionType::Vector2), 2);
    EXPECT_EQ(static_cast<int>(ActionType::Vector3), 3);
}