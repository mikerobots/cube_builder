#include "KeyboardHandler.h"

namespace VoxelEditor {
namespace Input {

KeyboardHandler::KeyboardHandler(VoxelEditor::Events::EventDispatcher* eventDispatcher)
    : InputHandler(eventDispatcher)
    , m_textInputEnabled(false)
    , m_repeatEnabled(true)
    , m_repeatDelay(0.5f)
    , m_repeatRate(30.0f) {
}

void KeyboardHandler::processKeyboardEvent(const KeyEvent& event) {
    if (!isEnabled()) {
        return;
    }
    
    switch (event.type) {
        case KeyEventType::Press:
            handleKeyPress(event);
            break;
        case KeyEventType::Release:
            handleKeyRelease(event);
            break;
        case KeyEventType::Character:
            handleCharacterInput(event);
            break;
    }
}

void KeyboardHandler::update(float deltaTime) {
    if (!isEnabled()) {
        return;
    }
    
    // Store previous state for just pressed/released detection
    m_state.previousKeysPressed = m_state.keysPressed;
    
    // Update just pressed/released states
    updateJustPressedReleased();
    
    // Handle key repeat
    if (m_repeatEnabled) {
        updateKeyRepeat(deltaTime);
    }
}

bool KeyboardHandler::isKeyPressed(KeyCode key) const {
    size_t index = getKeyIndex(key);
    return isValidKeyIndex(index) ? m_state.keysPressed[index] : false;
}

bool KeyboardHandler::isKeyJustPressed(KeyCode key) const {
    size_t index = getKeyIndex(key);
    return isValidKeyIndex(index) ? m_state.keysJustPressed[index] : false;
}

bool KeyboardHandler::isKeyJustReleased(KeyCode key) const {
    size_t index = getKeyIndex(key);
    return isValidKeyIndex(index) ? m_state.keysJustReleased[index] : false;
}

bool KeyboardHandler::isShiftPressed() const {
    return isKeyPressed(KeyCode::Shift);
}

bool KeyboardHandler::isCtrlPressed() const {
    return isKeyPressed(KeyCode::Ctrl);
}

bool KeyboardHandler::isAltPressed() const {
    return isKeyPressed(KeyCode::Alt);
}

bool KeyboardHandler::isSuperPressed() const {
    return isKeyPressed(KeyCode::Super);
}

ModifierFlags KeyboardHandler::getCurrentModifiers() const {
    ModifierFlags modifiers = ModifierFlags::None;
    
    if (isShiftPressed()) {
        modifiers = static_cast<ModifierFlags>(static_cast<uint32_t>(modifiers) | static_cast<uint32_t>(ModifierFlags::Shift));
    }
    if (isCtrlPressed()) {
        modifiers = static_cast<ModifierFlags>(static_cast<uint32_t>(modifiers) | static_cast<uint32_t>(ModifierFlags::Ctrl));
    }
    if (isAltPressed()) {
        modifiers = static_cast<ModifierFlags>(static_cast<uint32_t>(modifiers) | static_cast<uint32_t>(ModifierFlags::Alt));
    }
    if (isSuperPressed()) {
        modifiers = static_cast<ModifierFlags>(static_cast<uint32_t>(modifiers) | static_cast<uint32_t>(ModifierFlags::Super));
    }
    
    return modifiers;
}

void KeyboardHandler::bindKey(KeyCode key, const std::string& action, ModifierFlags modifiers) {
    KeyCombination combo(key, modifiers);
    m_keyBindings[combo] = action;
}

void KeyboardHandler::bindKeyCombination(const KeyCombination& combination, const std::string& action) {
    m_keyBindings[combination] = action;
}

void KeyboardHandler::unbindKey(KeyCode key, ModifierFlags modifiers) {
    KeyCombination combo(key, modifiers);
    m_keyBindings.erase(combo);
}

void KeyboardHandler::clearAllBindings() {
    m_keyBindings.clear();
}

std::string KeyboardHandler::getKeyAction(KeyCode key, ModifierFlags modifiers) const {
    KeyCombination combo(key, modifiers);
    auto it = m_keyBindings.find(combo);
    return (it != m_keyBindings.end()) ? it->second : "";
}

std::string KeyboardHandler::getKeyCombinationAction(const KeyCombination& combination) const {
    auto it = m_keyBindings.find(combination);
    return (it != m_keyBindings.end()) ? it->second : "";
}

void KeyboardHandler::setTextInputEnabled(bool enabled) {
    m_textInputEnabled = enabled;
    if (!enabled) {
        m_textInput.clear();
    }
}

void KeyboardHandler::clearTextInput() {
    m_textInput.clear();
}

void KeyboardHandler::handleKeyPress(const KeyEvent& event) {
    size_t index = getKeyIndex(event.key);
    if (!isValidKeyIndex(index)) {
        return;
    }
    
    m_state.keysPressed[index] = true;
    m_state.keysJustPressed[index] = true;
    
    // Update repeat timing
    if (m_repeatEnabled) {
        m_state.keyRepeatTime[index] = 0.0f;
        m_state.keyRepeating[index] = false;
    }
    
    // Check for bound actions
    ModifierFlags currentMods = getCurrentModifiers();
    std::string action = getKeyAction(event.key, currentMods);
    if (!action.empty()) {
        dispatchKeyActionEvent(action, event.key, currentMods);
    }
}

void KeyboardHandler::handleKeyRelease(const KeyEvent& event) {
    size_t index = getKeyIndex(event.key);
    if (!isValidKeyIndex(index)) {
        return;
    }
    
    m_state.keysPressed[index] = false;
    m_state.keysJustReleased[index] = true;
    
    // Stop repeating
    if (m_repeatEnabled) {
        m_state.keyRepeating[index] = false;
    }
}

void KeyboardHandler::handleCharacterInput(const KeyEvent& event) {
    if (m_textInputEnabled && event.character != 0) {
        // Handle special characters
        if (event.character == '\b') { // Backspace
            if (!m_textInput.empty()) {
                m_textInput.pop_back();
            }
        } else if (event.character >= 32 && event.character < 127) { // Printable ASCII
            m_textInput += static_cast<char>(event.character);
        }
    }
}

void KeyboardHandler::updateJustPressedReleased() {
    for (size_t i = 0; i < m_state.keysPressed.size(); ++i) {
        bool currentPressed = m_state.keysPressed[i];
        bool previousPressed = m_state.previousKeysPressed[i];
        
        m_state.keysJustPressed[i] = currentPressed && !previousPressed;
        m_state.keysJustReleased[i] = !currentPressed && previousPressed;
    }
}

void KeyboardHandler::updateKeyRepeat(float deltaTime) {
    for (size_t i = 0; i < m_state.keysPressed.size(); ++i) {
        if (m_state.keysPressed[i]) {
            m_state.keyRepeatTime[i] += deltaTime;
            
            // Check if we should start repeating
            if (!m_state.keyRepeating[i] && m_state.keyRepeatTime[i] >= m_repeatDelay) {
                m_state.keyRepeating[i] = true;
                m_state.keyRepeatTime[i] = 0.0f; // Reset for repeat timing
            }
            
            // Check if we should generate a repeat event
            if (m_state.keyRepeating[i] && m_state.keyRepeatTime[i] >= (1.0f / m_repeatRate)) {
                m_state.keyRepeatTime[i] = 0.0f;
                
                // Generate repeat event
                KeyCode key = static_cast<KeyCode>(i);
                if (isValidKeyCode(key)) {
                    ModifierFlags currentMods = getCurrentModifiers();
                    std::string action = getKeyAction(key, currentMods);
                    if (!action.empty()) {
                        dispatchKeyActionEvent(action, key, currentMods);
                    }
                }
            }
        } else {
            m_state.keyRepeatTime[i] = 0.0f;
            m_state.keyRepeating[i] = false;
        }
    }
}

size_t KeyboardHandler::getKeyIndex(KeyCode key) {
    return static_cast<size_t>(key);
}

bool KeyboardHandler::isValidKeyIndex(size_t index) {
    return index < 256 && index != static_cast<size_t>(KeyCode::Unknown);
}

void KeyboardHandler::dispatchKeyActionEvent(const std::string& action, KeyCode key, ModifierFlags modifiers) {
    if (m_eventDispatcher) {
        Events::KeyActionEvent actionEvent(action, key, modifiers);
        m_eventDispatcher->dispatch(actionEvent);
    }
}

std::string KeyboardHandler::keyCodeToString(KeyCode key) {
    switch (key) {
        // Letters
        case KeyCode::A: return "A";
        case KeyCode::B: return "B";
        case KeyCode::C: return "C";
        case KeyCode::D: return "D";
        case KeyCode::E: return "E";
        case KeyCode::F: return "F";
        case KeyCode::G: return "G";
        case KeyCode::H: return "H";
        case KeyCode::I: return "I";
        case KeyCode::J: return "J";
        case KeyCode::K: return "K";
        case KeyCode::L: return "L";
        case KeyCode::M: return "M";
        case KeyCode::N: return "N";
        case KeyCode::O: return "O";
        case KeyCode::P: return "P";
        case KeyCode::Q: return "Q";
        case KeyCode::R: return "R";
        case KeyCode::S: return "S";
        case KeyCode::T: return "T";
        case KeyCode::U: return "U";
        case KeyCode::V: return "V";
        case KeyCode::W: return "W";
        case KeyCode::X: return "X";
        case KeyCode::Y: return "Y";
        case KeyCode::Z: return "Z";
        
        // Numbers
        case KeyCode::Num0: return "0";
        case KeyCode::Num1: return "1";
        case KeyCode::Num2: return "2";
        case KeyCode::Num3: return "3";
        case KeyCode::Num4: return "4";
        case KeyCode::Num5: return "5";
        case KeyCode::Num6: return "6";
        case KeyCode::Num7: return "7";
        case KeyCode::Num8: return "8";
        case KeyCode::Num9: return "9";
        
        // Function keys
        case KeyCode::F1: return "F1";
        case KeyCode::F2: return "F2";
        case KeyCode::F3: return "F3";
        case KeyCode::F4: return "F4";
        case KeyCode::F5: return "F5";
        case KeyCode::F6: return "F6";
        case KeyCode::F7: return "F7";
        case KeyCode::F8: return "F8";
        case KeyCode::F9: return "F9";
        case KeyCode::F10: return "F10";
        case KeyCode::F11: return "F11";
        case KeyCode::F12: return "F12";
        
        // Special keys
        case KeyCode::Space: return "Space";
        case KeyCode::Enter: return "Enter";
        case KeyCode::Tab: return "Tab";
        case KeyCode::Escape: return "Escape";
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Delete: return "Delete";
        case KeyCode::Insert: return "Insert";
        case KeyCode::Home: return "Home";
        case KeyCode::End: return "End";
        case KeyCode::PageUp: return "PageUp";
        case KeyCode::PageDown: return "PageDown";
        
        // Arrow keys
        case KeyCode::Left: return "Left";
        case KeyCode::Right: return "Right";
        case KeyCode::Up: return "Up";
        case KeyCode::Down: return "Down";
        
        // Modifiers
        case KeyCode::Shift: return "Shift";
        case KeyCode::Ctrl: return "Ctrl";
        case KeyCode::Alt: return "Alt";
        case KeyCode::Super: return "Super";
        
        case KeyCode::Unknown:
        default:
            return "Unknown";
    }
}

KeyCode KeyboardHandler::keyCodeFromString(const std::string& str) {
    // Letters
    if (str == "A") return KeyCode::A;
    if (str == "B") return KeyCode::B;
    if (str == "C") return KeyCode::C;
    if (str == "D") return KeyCode::D;
    if (str == "E") return KeyCode::E;
    if (str == "F") return KeyCode::F;
    if (str == "G") return KeyCode::G;
    if (str == "H") return KeyCode::H;
    if (str == "I") return KeyCode::I;
    if (str == "J") return KeyCode::J;
    if (str == "K") return KeyCode::K;
    if (str == "L") return KeyCode::L;
    if (str == "M") return KeyCode::M;
    if (str == "N") return KeyCode::N;
    if (str == "O") return KeyCode::O;
    if (str == "P") return KeyCode::P;
    if (str == "Q") return KeyCode::Q;
    if (str == "R") return KeyCode::R;
    if (str == "S") return KeyCode::S;
    if (str == "T") return KeyCode::T;
    if (str == "U") return KeyCode::U;
    if (str == "V") return KeyCode::V;
    if (str == "W") return KeyCode::W;
    if (str == "X") return KeyCode::X;
    if (str == "Y") return KeyCode::Y;
    if (str == "Z") return KeyCode::Z;
    
    // Numbers
    if (str == "0") return KeyCode::Num0;
    if (str == "1") return KeyCode::Num1;
    if (str == "2") return KeyCode::Num2;
    if (str == "3") return KeyCode::Num3;
    if (str == "4") return KeyCode::Num4;
    if (str == "5") return KeyCode::Num5;
    if (str == "6") return KeyCode::Num6;
    if (str == "7") return KeyCode::Num7;
    if (str == "8") return KeyCode::Num8;
    if (str == "9") return KeyCode::Num9;
    
    // Function keys
    if (str == "F1") return KeyCode::F1;
    if (str == "F2") return KeyCode::F2;
    if (str == "F3") return KeyCode::F3;
    if (str == "F4") return KeyCode::F4;
    if (str == "F5") return KeyCode::F5;
    if (str == "F6") return KeyCode::F6;
    if (str == "F7") return KeyCode::F7;
    if (str == "F8") return KeyCode::F8;
    if (str == "F9") return KeyCode::F9;
    if (str == "F10") return KeyCode::F10;
    if (str == "F11") return KeyCode::F11;
    if (str == "F12") return KeyCode::F12;
    
    // Special keys
    if (str == "Space") return KeyCode::Space;
    if (str == "Enter") return KeyCode::Enter;
    if (str == "Tab") return KeyCode::Tab;
    if (str == "Escape") return KeyCode::Escape;
    if (str == "Backspace") return KeyCode::Backspace;
    if (str == "Delete") return KeyCode::Delete;
    if (str == "Insert") return KeyCode::Insert;
    if (str == "Home") return KeyCode::Home;
    if (str == "End") return KeyCode::End;
    if (str == "PageUp") return KeyCode::PageUp;
    if (str == "PageDown") return KeyCode::PageDown;
    
    // Arrow keys
    if (str == "Left") return KeyCode::Left;
    if (str == "Right") return KeyCode::Right;
    if (str == "Up") return KeyCode::Up;
    if (str == "Down") return KeyCode::Down;
    
    // Modifiers
    if (str == "Shift") return KeyCode::Shift;
    if (str == "Ctrl") return KeyCode::Ctrl;
    if (str == "Alt") return KeyCode::Alt;
    if (str == "Super") return KeyCode::Super;
    
    return KeyCode::Unknown;
}

std::string KeyboardHandler::modifierFlagsToString(ModifierFlags modifiers) {
    std::vector<std::string> parts;
    
    if (hasModifier(modifiers, ModifierFlags::Shift)) {
        parts.push_back("Shift");
    }
    if (hasModifier(modifiers, ModifierFlags::Ctrl)) {
        parts.push_back("Ctrl");
    }
    if (hasModifier(modifiers, ModifierFlags::Alt)) {
        parts.push_back("Alt");
    }
    if (hasModifier(modifiers, ModifierFlags::Super)) {
        parts.push_back("Super");
    }
    
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += "+";
        result += parts[i];
    }
    
    return result;
}

bool KeyboardHandler::isValidKeyCode(KeyCode key) {
    return key != KeyCode::Unknown;
}

bool KeyboardHandler::isPrintableKey(KeyCode key) {
    return (key >= KeyCode::A && key <= KeyCode::Z) ||
           (key >= KeyCode::Num0 && key <= KeyCode::Num9) ||
           key == KeyCode::Space;
}

bool KeyboardHandler::isModifierKey(KeyCode key) {
    return key == KeyCode::Shift ||
           key == KeyCode::Ctrl ||
           key == KeyCode::Alt ||
           key == KeyCode::Super;
}

}
}