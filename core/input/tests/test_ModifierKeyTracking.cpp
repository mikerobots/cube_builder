#include <gtest/gtest.h>
#include "../InputManager.h"
#include "../KeyboardHandler.h"
#include "../MouseHandler.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Input;
using namespace VoxelEditor::Events;

class ModifierKeyTrackingTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_eventDispatcher = std::make_unique<EventDispatcher>();
        m_inputManager = std::make_unique<InputManager>(m_eventDispatcher.get());
        m_inputManager->initialize();
    }
    
    void TearDown() override {
        m_inputManager->shutdown();
    }
    
    std::unique_ptr<EventDispatcher> m_eventDispatcher;
    std::unique_ptr<InputManager> m_inputManager;
};

// Test basic modifier key state tracking
TEST_F(ModifierKeyTrackingTest, BasicModifierKeyStates) {
    // Initially no modifiers should be pressed
    EXPECT_FALSE(m_inputManager->isShiftPressed());
    EXPECT_FALSE(m_inputManager->isCtrlPressed());
    EXPECT_FALSE(m_inputManager->isAltPressed());
    EXPECT_FALSE(m_inputManager->isSuperPressed());
    EXPECT_EQ(m_inputManager->getCurrentModifiers(), ModifierFlags::None);
    
    // Inject Shift key press
    KeyEvent shiftPress(KeyEventType::Press, KeyCode::Shift);
    m_inputManager->injectKeyboardEvent(shiftPress);
    m_inputManager->processEvents();
    
    EXPECT_TRUE(m_inputManager->isShiftPressed());
    EXPECT_FALSE(m_inputManager->isCtrlPressed());
    EXPECT_FALSE(m_inputManager->isAltPressed());
    EXPECT_FALSE(m_inputManager->isSuperPressed());
    EXPECT_EQ(m_inputManager->getCurrentModifiers(), ModifierFlags::Shift);
    
    // Inject Shift key release
    KeyEvent shiftRelease(KeyEventType::Release, KeyCode::Shift);
    m_inputManager->injectKeyboardEvent(shiftRelease);
    m_inputManager->processEvents();
    
    EXPECT_FALSE(m_inputManager->isShiftPressed());
    EXPECT_EQ(m_inputManager->getCurrentModifiers(), ModifierFlags::None);
}

// Test multiple modifier keys pressed simultaneously
TEST_F(ModifierKeyTrackingTest, MultipleModifierKeys) {
    // Press Ctrl and Shift
    KeyEvent ctrlPress(KeyEventType::Press, KeyCode::Ctrl);
    KeyEvent shiftPress(KeyEventType::Press, KeyCode::Shift);
    
    m_inputManager->injectKeyboardEvent(ctrlPress);
    m_inputManager->injectKeyboardEvent(shiftPress);
    m_inputManager->processEvents();
    
    EXPECT_TRUE(m_inputManager->isShiftPressed());
    EXPECT_TRUE(m_inputManager->isCtrlPressed());
    EXPECT_FALSE(m_inputManager->isAltPressed());
    EXPECT_FALSE(m_inputManager->isSuperPressed());
    
    ModifierFlags expected = ModifierFlags::Ctrl | ModifierFlags::Shift;
    EXPECT_EQ(m_inputManager->getCurrentModifiers(), expected);
    
    // Release Ctrl, keep Shift pressed
    KeyEvent ctrlRelease(KeyEventType::Release, KeyCode::Ctrl);
    m_inputManager->injectKeyboardEvent(ctrlRelease);
    m_inputManager->processEvents();
    
    EXPECT_TRUE(m_inputManager->isShiftPressed());
    EXPECT_FALSE(m_inputManager->isCtrlPressed());
    EXPECT_EQ(m_inputManager->getCurrentModifiers(), ModifierFlags::Shift);
}

// Test modifier state transitions during frame updates
TEST_F(ModifierKeyTrackingTest, ModifierStateTransitions) {
    // Press and release within same frame
    KeyEvent shiftPress(KeyEventType::Press, KeyCode::Shift);
    KeyEvent shiftRelease(KeyEventType::Release, KeyCode::Shift);
    
    m_inputManager->injectKeyboardEvent(shiftPress);
    m_inputManager->injectKeyboardEvent(shiftRelease);
    m_inputManager->processEvents();
    
    // Should reflect final state (released)
    EXPECT_FALSE(m_inputManager->isShiftPressed());
    
    // Update frame to clear previous just pressed/released states
    m_inputManager->update(0.016f);
    
    // Test just pressed/released detection
    m_inputManager->injectKeyboardEvent(shiftPress);
    m_inputManager->processEvents();
    
    EXPECT_TRUE(m_inputManager->isKeyJustPressed(KeyCode::Shift));
    EXPECT_FALSE(m_inputManager->isKeyJustReleased(KeyCode::Shift));
    
    // Update frame
    m_inputManager->update(0.016f);
    
    // Just pressed should clear after frame
    EXPECT_FALSE(m_inputManager->isKeyJustPressed(KeyCode::Shift));
    EXPECT_TRUE(m_inputManager->isKeyPressed(KeyCode::Shift));
    
    // Release key
    m_inputManager->injectKeyboardEvent(shiftRelease);
    m_inputManager->processEvents();
    
    EXPECT_FALSE(m_inputManager->isKeyPressed(KeyCode::Shift));
    EXPECT_TRUE(m_inputManager->isKeyJustReleased(KeyCode::Shift));
    
    // Update frame to clear just released
    m_inputManager->update(0.016f);
    EXPECT_FALSE(m_inputManager->isKeyJustReleased(KeyCode::Shift));
}

// Test modifier flags with mouse events
TEST_F(ModifierKeyTrackingTest, ModifiersWithMouseEvents) {
    // Press Shift first
    KeyEvent shiftPress(KeyEventType::Press, KeyCode::Shift);
    m_inputManager->injectKeyboardEvent(shiftPress);
    m_inputManager->processEvents();
    
    // Create mouse event with modifiers
    MouseEvent mouseClick;
    mouseClick.type = MouseEventType::ButtonPress;
    mouseClick.button = MouseButton::Left;
    mouseClick.position = Math::Vector2f(100.0f, 100.0f);
    mouseClick.modifiers = static_cast<uint32_t>(ModifierFlags::Shift);
    
    m_inputManager->injectMouseEvent(mouseClick);
    m_inputManager->processEvents();
    
    // Both keyboard and mouse should report Shift pressed
    EXPECT_TRUE(m_inputManager->isShiftPressed());
    EXPECT_TRUE(m_inputManager->isMouseButtonPressed(MouseButton::Left));
}

// Test all modifier combinations
TEST_F(ModifierKeyTrackingTest, AllModifierCombinations) {
    struct ModifierTest {
        KeyCode key;
        ModifierFlags flag;
        bool (InputManager::*checkFunc)() const;
    };
    
    ModifierTest tests[] = {
        { KeyCode::Shift, ModifierFlags::Shift, &InputManager::isShiftPressed },
        { KeyCode::Ctrl, ModifierFlags::Ctrl, &InputManager::isCtrlPressed },
        { KeyCode::Alt, ModifierFlags::Alt, &InputManager::isAltPressed },
        { KeyCode::Super, ModifierFlags::Super, &InputManager::isSuperPressed }
    };
    
    // Test each modifier individually
    for (const auto& test : tests) {
        // Press
        KeyEvent press(KeyEventType::Press, test.key);
        m_inputManager->injectKeyboardEvent(press);
        m_inputManager->processEvents();
        
        EXPECT_TRUE((m_inputManager.get()->*test.checkFunc)());
        EXPECT_EQ(m_inputManager->getCurrentModifiers(), test.flag);
        
        // Release
        KeyEvent release(KeyEventType::Release, test.key);
        m_inputManager->injectKeyboardEvent(release);
        m_inputManager->processEvents();
        
        EXPECT_FALSE((m_inputManager.get()->*test.checkFunc)());
        EXPECT_EQ(m_inputManager->getCurrentModifiers(), ModifierFlags::None);
    }
    
    // Test all modifiers together
    for (const auto& test : tests) {
        KeyEvent press(KeyEventType::Press, test.key);
        m_inputManager->injectKeyboardEvent(press);
    }
    m_inputManager->processEvents();
    
    ModifierFlags allMods = ModifierFlags::Shift | ModifierFlags::Ctrl | 
                           ModifierFlags::Alt | ModifierFlags::Super;
    EXPECT_EQ(m_inputManager->getCurrentModifiers(), allMods);
}

// Test keyboard handler directly
TEST_F(ModifierKeyTrackingTest, KeyboardHandlerDirectTest) {
    EventDispatcher dispatcher;
    KeyboardHandler handler(&dispatcher);
    
    // Test getCurrentModifiers with no keys pressed
    EXPECT_EQ(handler.getCurrentModifiers(), ModifierFlags::None);
    
    // Simulate Shift+Ctrl press
    KeyEvent shiftPress(KeyEventType::Press, KeyCode::Shift);
    KeyEvent ctrlPress(KeyEventType::Press, KeyCode::Ctrl);
    
    handler.processKeyboardEvent(shiftPress);
    handler.processKeyboardEvent(ctrlPress);
    handler.update(0.016f);
    
    ModifierFlags expected = ModifierFlags::Shift | ModifierFlags::Ctrl;
    EXPECT_EQ(handler.getCurrentModifiers(), expected);
    
    // Test modifierFlagsToString
    std::string modString = KeyboardHandler::modifierFlagsToString(expected);
    EXPECT_EQ(modString, "Shift+Ctrl");
}

// Test modifier flags helper functions
TEST_F(ModifierKeyTrackingTest, ModifierFlagsHelpers) {
    ModifierFlags none = ModifierFlags::None;
    ModifierFlags shift = ModifierFlags::Shift;
    ModifierFlags ctrl = ModifierFlags::Ctrl;
    ModifierFlags shiftCtrl = shift | ctrl;
    
    // Test hasModifier function
    EXPECT_FALSE(hasModifier(none, ModifierFlags::Shift));
    EXPECT_TRUE(hasModifier(shift, ModifierFlags::Shift));
    EXPECT_FALSE(hasModifier(shift, ModifierFlags::Ctrl));
    EXPECT_TRUE(hasModifier(shiftCtrl, ModifierFlags::Shift));
    EXPECT_TRUE(hasModifier(shiftCtrl, ModifierFlags::Ctrl));
    EXPECT_FALSE(hasModifier(shiftCtrl, ModifierFlags::Alt));
    
    // Test OR operator
    ModifierFlags combined = shift | ctrl;
    EXPECT_TRUE(hasModifier(combined, ModifierFlags::Shift));
    EXPECT_TRUE(hasModifier(combined, ModifierFlags::Ctrl));
    
    // Test AND operator
    ModifierFlags masked = shiftCtrl & shift;
    EXPECT_EQ(masked, shift);
}