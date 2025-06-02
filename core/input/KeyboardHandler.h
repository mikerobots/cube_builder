#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include "InputHandler.h"
#include "InputTypes.h"
#include "../../foundation/events/EventBase.h"

namespace VoxelEditor {
namespace Input {

class KeyboardHandler : public InputHandler {
public:
    explicit KeyboardHandler(VoxelEditor::Events::EventDispatcher* eventDispatcher = nullptr);
    ~KeyboardHandler() = default;
    
    // InputHandler interface
    void processKeyboardEvent(const KeyEvent& event) override;
    void update(float deltaTime) override;
    
    // Key state queries
    bool isKeyPressed(KeyCode key) const;
    bool isKeyJustPressed(KeyCode key) const;
    bool isKeyJustReleased(KeyCode key) const;
    
    // Modifier queries
    bool isShiftPressed() const;
    bool isCtrlPressed() const;
    bool isAltPressed() const;
    bool isSuperPressed() const;
    ModifierFlags getCurrentModifiers() const;
    
    // Text input
    const std::string& getTextInput() const { return m_textInput; }
    void clearTextInput();
    void setTextInputEnabled(bool enabled);
    bool isTextInputEnabled() const { return m_textInputEnabled; }
    
    // Key bindings
    void bindKey(KeyCode key, const std::string& action, ModifierFlags modifiers = ModifierFlags::None);
    void bindKeyCombination(const KeyCombination& combo, const std::string& action);
    void unbindKey(KeyCode key, ModifierFlags modifiers = ModifierFlags::None);
    void unbindKeyCombination(const KeyCombination& combo);
    void clearAllBindings();
    
    // Key binding queries
    std::string getKeyAction(KeyCode key, ModifierFlags modifiers = ModifierFlags::None) const;
    std::string getKeyCombinationAction(const KeyCombination& combo) const;
    std::vector<KeyCombination> getActionsForKey(KeyCode key) const;
    std::vector<std::string> getAllBoundActions() const;
    
    // Configuration
    void setRepeatEnabled(bool enabled) { m_repeatEnabled = enabled; }
    void setRepeatDelay(float seconds) { m_repeatDelay = seconds; }
    void setRepeatRate(float rate) { m_repeatRate = rate; }
    bool isRepeatEnabled() const { return m_repeatEnabled; }
    float getRepeatDelay() const { return m_repeatDelay; }
    float getRepeatRate() const { return m_repeatRate; }
    
    // Key utilities
    static std::string keyCodeToString(KeyCode key);
    static KeyCode keyCodeFromString(const std::string& str);
    static std::string modifierFlagsToString(ModifierFlags modifiers);
    static ModifierFlags modifierFlagsFromString(const std::string& str);
    static bool isValidKeyCode(KeyCode key);
    static bool isPrintableKey(KeyCode key);
    static bool isModifierKey(KeyCode key);
    
private:
    struct KeyState {
        // Current state
        std::array<bool, 256> keysPressed = {};
        std::array<bool, 256> keysJustPressed = {};
        std::array<bool, 256> keysJustReleased = {};
        std::array<float, 256> keyRepeatTime = {};
        std::array<bool, 256> keyRepeating = {};
        
        // Previous frame state for just pressed/released detection
        std::array<bool, 256> previousKeysPressed = {};
        
        void reset() {
            keysPressed.fill(false);
            keysJustPressed.fill(false);
            keysJustReleased.fill(false);
            keyRepeatTime.fill(0.0f);
            keyRepeating.fill(false);
            previousKeysPressed.fill(false);
        }
    };
    
    KeyState m_state;
    
    // Text input
    std::string m_textInput;
    bool m_textInputEnabled;
    
    // Key bindings
    std::unordered_map<KeyCombination, std::string> m_keyBindings;
    
    // Configuration
    bool m_repeatEnabled;
    float m_repeatDelay;
    float m_repeatRate;
    
    // Event processing methods
    void handleKeyPress(const KeyEvent& event);
    void handleKeyRelease(const KeyEvent& event);
    void handleCharacterInput(const KeyEvent& event);
    
    // Helper methods
    void updateJustPressedReleased();
    void updateKeyRepeat(float deltaTime);
    
    // Key index conversion
    static size_t getKeyIndex(KeyCode key);
    static bool isValidKeyIndex(size_t index);
    
    // Event dispatching
    void dispatchKeyActionEvent(const std::string& action, KeyCode key, ModifierFlags modifiers);
};

// Keyboard event types for the event system
namespace Events {
    
    struct KeyPressEvent : public VoxelEditor::Events::Event<KeyPressEvent> {
        KeyCode key;
        ModifierFlags modifiers;
        bool repeat;
        TimePoint timestamp;
        
        KeyPressEvent(KeyCode k, ModifierFlags mods = ModifierFlags::None, bool rep = false)
            : key(k), modifiers(mods), repeat(rep)
            , timestamp(std::chrono::high_resolution_clock::now()) {}
    };
    
    struct KeyReleaseEvent : public VoxelEditor::Events::Event<KeyReleaseEvent> {
        KeyCode key;
        ModifierFlags modifiers;
        TimePoint timestamp;
        
        KeyReleaseEvent(KeyCode k, ModifierFlags mods = ModifierFlags::None)
            : key(k), modifiers(mods)
            , timestamp(std::chrono::high_resolution_clock::now()) {}
    };
    
    struct KeyCombinationEvent : public VoxelEditor::Events::Event<KeyCombinationEvent> {
        KeyCombination combination;
        bool pressed;
        TimePoint timestamp;
        
        KeyCombinationEvent(const KeyCombination& combo, bool p)
            : combination(combo), pressed(p)
            , timestamp(std::chrono::high_resolution_clock::now()) {}
    };
    
    struct TextInputEvent : public VoxelEditor::Events::Event<TextInputEvent> {
        std::string text;
        TimePoint timestamp;
        
        TextInputEvent(const std::string& t)
            : text(t), timestamp(std::chrono::high_resolution_clock::now()) {}
    };
    
    struct KeyActionEvent : public VoxelEditor::Events::Event<KeyActionEvent> {
        std::string action;
        KeyCode key;
        ModifierFlags modifiers;
        TimePoint timestamp;
        
        KeyActionEvent(const std::string& a, KeyCode k, ModifierFlags mods = ModifierFlags::None)
            : action(a), key(k), modifiers(mods)
            , timestamp(std::chrono::high_resolution_clock::now()) {}
    };
    
}

}
}