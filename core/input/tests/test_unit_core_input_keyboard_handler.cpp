#include <gtest/gtest.h>
#include "../KeyboardHandler.h"
#include "../../../foundation/events/EventDispatcher.h"
#include "../../../foundation/events/EventBase.h"
#include <vector>

using namespace VoxelEditor::Input;

// The events are already defined in KeyboardHandler.h, so we just use those
using namespace VoxelEditor::Input::Events;

class MockEventDispatcher : public VoxelEditor::Events::EventDispatcher {
public:
    mutable std::vector<std::string> dispatchedEvents;
    
    template<typename EventType>
    void dispatch(const EventType& event) {
        VoxelEditor::Events::EventDispatcher::dispatch(event);
        dispatchedEvents.push_back(typeid(EventType).name());
    }
};

class KeyboardHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<MockEventDispatcher>();
        handler = std::make_unique<KeyboardHandler>(eventDispatcher.get());
    }
    
    void TearDown() override {}
    
    std::unique_ptr<MockEventDispatcher> eventDispatcher;
    std::unique_ptr<KeyboardHandler> handler;
};

TEST_F(KeyboardHandlerTest, DefaultState) {
    EXPECT_FALSE(handler->isKeyPressed(KeyCode::A));
    EXPECT_FALSE(handler->isKeyPressed(KeyCode::Space));
    EXPECT_FALSE(handler->isKeyPressed(KeyCode::Enter));
    
    EXPECT_FALSE(handler->isShiftPressed());
    EXPECT_FALSE(handler->isCtrlPressed());
    EXPECT_FALSE(handler->isAltPressed());
    EXPECT_FALSE(handler->isSuperPressed());
    
    EXPECT_EQ(handler->getCurrentModifiers(), ModifierFlags::None);
    EXPECT_TRUE(handler->getTextInput().empty());
    EXPECT_FALSE(handler->isTextInputEnabled());
}

TEST_F(KeyboardHandlerTest, KeyPressRelease) {
    // Press A key
    KeyEvent pressEvent(KeyEventType::Press, KeyCode::A);
    handler->processKeyboardEvent(pressEvent);
    
    EXPECT_TRUE(handler->isKeyPressed(KeyCode::A));
    EXPECT_TRUE(handler->isKeyJustPressed(KeyCode::A));
    EXPECT_FALSE(handler->isKeyJustReleased(KeyCode::A));
    EXPECT_FALSE(handler->isKeyPressed(KeyCode::B));
    
    // Update to clear just pressed state
    handler->update(0.016f);
    EXPECT_TRUE(handler->isKeyPressed(KeyCode::A));
    EXPECT_FALSE(handler->isKeyJustPressed(KeyCode::A));
    
    // Release A key
    KeyEvent releaseEvent(KeyEventType::Release, KeyCode::A);
    handler->processKeyboardEvent(releaseEvent);
    
    EXPECT_FALSE(handler->isKeyPressed(KeyCode::A));
    EXPECT_FALSE(handler->isKeyJustPressed(KeyCode::A));
    EXPECT_TRUE(handler->isKeyJustReleased(KeyCode::A));
}

TEST_F(KeyboardHandlerTest, ModifierKeys) {
    // Press Shift
    KeyEvent shiftPress(KeyEventType::Press, KeyCode::Shift);
    handler->processKeyboardEvent(shiftPress);
    
    EXPECT_TRUE(handler->isShiftPressed());
    EXPECT_TRUE(hasModifier(handler->getCurrentModifiers(), ModifierFlags::Shift));
    
    // Press Ctrl while Shift is held
    KeyEvent ctrlPress(KeyEventType::Press, KeyCode::Ctrl);
    handler->processKeyboardEvent(ctrlPress);
    
    EXPECT_TRUE(handler->isShiftPressed());
    EXPECT_TRUE(handler->isCtrlPressed());
    EXPECT_TRUE(hasModifier(handler->getCurrentModifiers(), ModifierFlags::Shift));
    EXPECT_TRUE(hasModifier(handler->getCurrentModifiers(), ModifierFlags::Ctrl));
    
    // Release Shift
    KeyEvent shiftRelease(KeyEventType::Release, KeyCode::Shift);
    handler->processKeyboardEvent(shiftRelease);
    
    EXPECT_FALSE(handler->isShiftPressed());
    EXPECT_TRUE(handler->isCtrlPressed());
    EXPECT_FALSE(hasModifier(handler->getCurrentModifiers(), ModifierFlags::Shift));
    EXPECT_TRUE(hasModifier(handler->getCurrentModifiers(), ModifierFlags::Ctrl));
}

TEST_F(KeyboardHandlerTest, AllModifierKeys) {
    // Test all modifier keys
    KeyEvent altPress(KeyEventType::Press, KeyCode::Alt);
    KeyEvent superPress(KeyEventType::Press, KeyCode::Super);
    
    handler->processKeyboardEvent(altPress);
    handler->processKeyboardEvent(superPress);
    
    EXPECT_TRUE(handler->isAltPressed());
    EXPECT_TRUE(handler->isSuperPressed());
    EXPECT_TRUE(hasModifier(handler->getCurrentModifiers(), ModifierFlags::Alt));
    EXPECT_TRUE(hasModifier(handler->getCurrentModifiers(), ModifierFlags::Super));
}

TEST_F(KeyboardHandlerTest, KeyBindings) {
    // Bind some keys to actions
    handler->bindKey(KeyCode::Space, "reset_camera");
    handler->bindKey(KeyCode::Delete, "delete_selection");
    handler->bindKey(KeyCode::F, "frame_selection");
    
    EXPECT_EQ(handler->getKeyAction(KeyCode::Space), "reset_camera");
    EXPECT_EQ(handler->getKeyAction(KeyCode::Delete), "delete_selection");
    EXPECT_EQ(handler->getKeyAction(KeyCode::F), "frame_selection");
    EXPECT_TRUE(handler->getKeyAction(KeyCode::G).empty());
}

TEST_F(KeyboardHandlerTest, KeyBindingsWithModifiers) {
    // Bind keys with modifiers
    handler->bindKey(KeyCode::Z, "undo", ModifierFlags::Ctrl);
    handler->bindKey(KeyCode::Y, "redo", ModifierFlags::Ctrl);
    handler->bindKey(KeyCode::S, "save", ModifierFlags::Ctrl);
    handler->bindKey(KeyCode::S, "save_as", ModifierFlags::Ctrl | ModifierFlags::Shift);
    
    EXPECT_EQ(handler->getKeyAction(KeyCode::Z, ModifierFlags::Ctrl), "undo");
    EXPECT_EQ(handler->getKeyAction(KeyCode::Y, ModifierFlags::Ctrl), "redo");
    EXPECT_EQ(handler->getKeyAction(KeyCode::S, ModifierFlags::Ctrl), "save");
    EXPECT_EQ(handler->getKeyAction(KeyCode::S, ModifierFlags::Ctrl | ModifierFlags::Shift), "save_as");
    
    // Without modifiers should not match
    EXPECT_TRUE(handler->getKeyAction(KeyCode::Z).empty());
    EXPECT_TRUE(handler->getKeyAction(KeyCode::S).empty());
}

TEST_F(KeyboardHandlerTest, KeyCombinationBindings) {
    // Test key combination bindings
    KeyCombination undoCombo(KeyCode::Z, ModifierFlags::Ctrl);
    KeyCombination redoCombo(KeyCode::Y, ModifierFlags::Ctrl);
    KeyCombination saveAsCombo(KeyCode::S, ModifierFlags::Ctrl | ModifierFlags::Shift);
    
    handler->bindKeyCombination(undoCombo, "undo");
    handler->bindKeyCombination(redoCombo, "redo");
    handler->bindKeyCombination(saveAsCombo, "save_as");
    
    EXPECT_EQ(handler->getKeyCombinationAction(undoCombo), "undo");
    EXPECT_EQ(handler->getKeyCombinationAction(redoCombo), "redo");
    EXPECT_EQ(handler->getKeyCombinationAction(saveAsCombo), "save_as");
    
    KeyCombination unknownCombo(KeyCode::X, ModifierFlags::Alt);
    EXPECT_TRUE(handler->getKeyCombinationAction(unknownCombo).empty());
}

TEST_F(KeyboardHandlerTest, UnbindingKeys) {
    // Bind and then unbind
    handler->bindKey(KeyCode::Space, "test_action");
    EXPECT_EQ(handler->getKeyAction(KeyCode::Space), "test_action");
    
    handler->unbindKey(KeyCode::Space);
    EXPECT_TRUE(handler->getKeyAction(KeyCode::Space).empty());
    
    // Test unbinding with modifiers
    handler->bindKey(KeyCode::A, "test_action", ModifierFlags::Ctrl);
    EXPECT_EQ(handler->getKeyAction(KeyCode::A, ModifierFlags::Ctrl), "test_action");
    
    handler->unbindKey(KeyCode::A, ModifierFlags::Ctrl);
    EXPECT_TRUE(handler->getKeyAction(KeyCode::A, ModifierFlags::Ctrl).empty());
}

TEST_F(KeyboardHandlerTest, ClearAllBindings) {
    // Add several bindings
    handler->bindKey(KeyCode::A, "action_a");
    handler->bindKey(KeyCode::B, "action_b");
    handler->bindKey(KeyCode::C, "action_c", ModifierFlags::Ctrl);
    
    EXPECT_FALSE(handler->getKeyAction(KeyCode::A).empty());
    EXPECT_FALSE(handler->getKeyAction(KeyCode::B).empty());
    EXPECT_FALSE(handler->getKeyAction(KeyCode::C, ModifierFlags::Ctrl).empty());
    
    // Clear all bindings
    handler->clearAllBindings();
    
    EXPECT_TRUE(handler->getKeyAction(KeyCode::A).empty());
    EXPECT_TRUE(handler->getKeyAction(KeyCode::B).empty());
    EXPECT_TRUE(handler->getKeyAction(KeyCode::C, ModifierFlags::Ctrl).empty());
}

TEST_F(KeyboardHandlerTest, TextInput) {
    // REQ-9.1.1: CLI shall provide interactive command prompt with auto-completion
    // REQ-5.3.2: Resolution changed via `resolution <size>` command
    // Enable text input
    handler->setTextInputEnabled(true);
    EXPECT_TRUE(handler->isTextInputEnabled());
    
    // Simulate character input
    KeyEvent charEvent(KeyEventType::Character, KeyCode::Unknown);
    charEvent.character = 'H';
    handler->processKeyboardEvent(charEvent);
    
    charEvent.character = 'i';
    handler->processKeyboardEvent(charEvent);
    
    EXPECT_EQ(handler->getTextInput(), "Hi");
    
    // Clear text input
    handler->clearTextInput();
    EXPECT_TRUE(handler->getTextInput().empty());
    
    // Disable text input
    handler->setTextInputEnabled(false);
    EXPECT_FALSE(handler->isTextInputEnabled());
    
    // New characters should not be added when disabled
    charEvent.character = 'X';
    handler->processKeyboardEvent(charEvent);
    EXPECT_TRUE(handler->getTextInput().empty());
}

TEST_F(KeyboardHandlerTest, KeyRepeat) {
    // Test key repeat configuration
    EXPECT_TRUE(handler->isRepeatEnabled());
    EXPECT_FLOAT_EQ(handler->getRepeatDelay(), 0.5f);
    EXPECT_FLOAT_EQ(handler->getRepeatRate(), 30.0f);
    
    handler->setRepeatEnabled(false);
    handler->setRepeatDelay(0.3f);
    handler->setRepeatRate(20.0f);
    
    EXPECT_FALSE(handler->isRepeatEnabled());
    EXPECT_FLOAT_EQ(handler->getRepeatDelay(), 0.3f);
    EXPECT_FLOAT_EQ(handler->getRepeatRate(), 20.0f);
}

TEST_F(KeyboardHandlerTest, MultipleKeys) {
    // Press multiple keys simultaneously
    KeyEvent aPress(KeyEventType::Press, KeyCode::A);
    KeyEvent bPress(KeyEventType::Press, KeyCode::B);
    KeyEvent cPress(KeyEventType::Press, KeyCode::C);
    
    handler->processKeyboardEvent(aPress);
    handler->processKeyboardEvent(bPress);
    handler->processKeyboardEvent(cPress);
    
    EXPECT_TRUE(handler->isKeyPressed(KeyCode::A));
    EXPECT_TRUE(handler->isKeyPressed(KeyCode::B));
    EXPECT_TRUE(handler->isKeyPressed(KeyCode::C));
    EXPECT_FALSE(handler->isKeyPressed(KeyCode::D));
    
    // Release one key
    KeyEvent bRelease(KeyEventType::Release, KeyCode::B);
    handler->processKeyboardEvent(bRelease);
    
    EXPECT_TRUE(handler->isKeyPressed(KeyCode::A));
    EXPECT_FALSE(handler->isKeyPressed(KeyCode::B));
    EXPECT_TRUE(handler->isKeyPressed(KeyCode::C));
}

TEST_F(KeyboardHandlerTest, EnabledState) {
    handler->setEnabled(false);
    EXPECT_FALSE(handler->isEnabled());
    
    // Events should be ignored when disabled
    KeyEvent pressEvent(KeyEventType::Press, KeyCode::A);
    handler->processKeyboardEvent(pressEvent);
    
    EXPECT_FALSE(handler->isKeyPressed(KeyCode::A));
    
    // Re-enable and test
    handler->setEnabled(true);
    EXPECT_TRUE(handler->isEnabled());
    
    handler->processKeyboardEvent(pressEvent);
    EXPECT_TRUE(handler->isKeyPressed(KeyCode::A));
}

TEST_F(KeyboardHandlerTest, KeyUtilities) {
    // Test key code to string conversion
    EXPECT_EQ(KeyboardHandler::keyCodeToString(KeyCode::A), "A");
    EXPECT_EQ(KeyboardHandler::keyCodeToString(KeyCode::Space), "Space");
    EXPECT_EQ(KeyboardHandler::keyCodeToString(KeyCode::F1), "F1");
    EXPECT_EQ(KeyboardHandler::keyCodeToString(KeyCode::Enter), "Enter");
    
    // Test string to key code conversion
    EXPECT_EQ(KeyboardHandler::keyCodeFromString("A"), KeyCode::A);
    EXPECT_EQ(KeyboardHandler::keyCodeFromString("Space"), KeyCode::Space);
    EXPECT_EQ(KeyboardHandler::keyCodeFromString("F1"), KeyCode::F1);
    EXPECT_EQ(KeyboardHandler::keyCodeFromString("Unknown"), KeyCode::Unknown);
    
    // Test modifier flags to string
    ModifierFlags combined = ModifierFlags::Ctrl | ModifierFlags::Shift;
    std::string modStr = KeyboardHandler::modifierFlagsToString(combined);
    EXPECT_TRUE(modStr.find("Ctrl") != std::string::npos);
    EXPECT_TRUE(modStr.find("Shift") != std::string::npos);
    
    // Test key validation
    EXPECT_TRUE(KeyboardHandler::isValidKeyCode(KeyCode::A));
    EXPECT_TRUE(KeyboardHandler::isValidKeyCode(KeyCode::Space));
    EXPECT_FALSE(KeyboardHandler::isValidKeyCode(KeyCode::Unknown));
    
    // Test printable key detection
    EXPECT_TRUE(KeyboardHandler::isPrintableKey(KeyCode::A));
    EXPECT_TRUE(KeyboardHandler::isPrintableKey(KeyCode::Num1));
    EXPECT_FALSE(KeyboardHandler::isPrintableKey(KeyCode::F1));
    EXPECT_FALSE(KeyboardHandler::isPrintableKey(KeyCode::Ctrl));
    
    // Test modifier key detection
    EXPECT_TRUE(KeyboardHandler::isModifierKey(KeyCode::Shift));
    EXPECT_TRUE(KeyboardHandler::isModifierKey(KeyCode::Ctrl));
    EXPECT_TRUE(KeyboardHandler::isModifierKey(KeyCode::Alt));
    EXPECT_FALSE(KeyboardHandler::isModifierKey(KeyCode::A));
}