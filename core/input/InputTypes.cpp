#include "InputTypes.h"
#include <sstream>
#include <unordered_map>

namespace VoxelEditor {
namespace Input {

// InputTrigger implementation
bool InputTrigger::matches(const MouseEvent& event) const {
    if (device != InputDevice::Mouse) return false;
    if (event.type != MouseEventType::ButtonPress && event.type != MouseEventType::ButtonRelease) return false;
    return input.mouseButton == event.button && 
           (requiredModifiers == ModifierFlags::None || 
            static_cast<ModifierFlags>(event.modifiers) == requiredModifiers);
}

bool InputTrigger::matches(const KeyEvent& event) const {
    if (device != InputDevice::Keyboard) return false;
    if (event.type != KeyEventType::Press && event.type != KeyEventType::Release) return false;
    return input.keyCode == event.key && 
           (requiredModifiers == ModifierFlags::None || event.modifiers == requiredModifiers);
}

bool InputTrigger::matches(TouchGesture gesture) const {
    if (device != InputDevice::Touch) return false;
    return input.touchGesture == gesture;
}

bool InputTrigger::matches(VRGesture gesture) const {
    if (device != InputDevice::VRHands) return false;
    return input.vrGesture == gesture;
}

// KeyCombination implementation
std::string KeyCombination::toString() const {
    std::ostringstream oss;
    
    if (hasModifier(modifiers, ModifierFlags::Ctrl)) {
        oss << "Ctrl+";
    }
    if (hasModifier(modifiers, ModifierFlags::Alt)) {
        oss << "Alt+";
    }
    if (hasModifier(modifiers, ModifierFlags::Shift)) {
        oss << "Shift+";
    }
    if (hasModifier(modifiers, ModifierFlags::Super)) {
        oss << "Super+";
    }
    
    // Key name mapping
    static const std::unordered_map<KeyCode, std::string> keyNames = {
        {KeyCode::A, "A"}, {KeyCode::B, "B"}, {KeyCode::C, "C"}, {KeyCode::D, "D"},
        {KeyCode::E, "E"}, {KeyCode::F, "F"}, {KeyCode::G, "G"}, {KeyCode::H, "H"},
        {KeyCode::I, "I"}, {KeyCode::J, "J"}, {KeyCode::K, "K"}, {KeyCode::L, "L"},
        {KeyCode::M, "M"}, {KeyCode::N, "N"}, {KeyCode::O, "O"}, {KeyCode::P, "P"},
        {KeyCode::Q, "Q"}, {KeyCode::R, "R"}, {KeyCode::S, "S"}, {KeyCode::T, "T"},
        {KeyCode::U, "U"}, {KeyCode::V, "V"}, {KeyCode::W, "W"}, {KeyCode::X, "X"},
        {KeyCode::Y, "Y"}, {KeyCode::Z, "Z"},
        
        {KeyCode::Num0, "0"}, {KeyCode::Num1, "1"}, {KeyCode::Num2, "2"},
        {KeyCode::Num3, "3"}, {KeyCode::Num4, "4"}, {KeyCode::Num5, "5"},
        {KeyCode::Num6, "6"}, {KeyCode::Num7, "7"}, {KeyCode::Num8, "8"},
        {KeyCode::Num9, "9"},
        
        {KeyCode::F1, "F1"}, {KeyCode::F2, "F2"}, {KeyCode::F3, "F3"},
        {KeyCode::F4, "F4"}, {KeyCode::F5, "F5"}, {KeyCode::F6, "F6"},
        {KeyCode::F7, "F7"}, {KeyCode::F8, "F8"}, {KeyCode::F9, "F9"},
        {KeyCode::F10, "F10"}, {KeyCode::F11, "F11"}, {KeyCode::F12, "F12"},
        
        {KeyCode::Space, "Space"}, {KeyCode::Enter, "Enter"},
        {KeyCode::Escape, "Escape"}, {KeyCode::Tab, "Tab"},
        {KeyCode::Backspace, "Backspace"}, {KeyCode::Delete, "Delete"},
        {KeyCode::Insert, "Insert"}, {KeyCode::Home, "Home"},
        {KeyCode::End, "End"}, {KeyCode::PageUp, "PageUp"},
        {KeyCode::PageDown, "PageDown"},
        
        {KeyCode::Up, "Up"}, {KeyCode::Down, "Down"},
        {KeyCode::Left, "Left"}, {KeyCode::Right, "Right"},
        
        {KeyCode::Shift, "Shift"}, {KeyCode::Ctrl, "Ctrl"},
        {KeyCode::Alt, "Alt"}, {KeyCode::Super, "Super"}
    };
    
    auto it = keyNames.find(primaryKey);
    if (it != keyNames.end()) {
        oss << it->second;
    } else {
        oss << "Unknown";
    }
    
    return oss.str();
}

KeyCombination KeyCombination::fromString(const std::string& str) {
    KeyCombination combo;
    
    // Parse modifier flags
    size_t pos = 0;
    while ((pos = str.find('+', pos)) != std::string::npos) {
        std::string modifier = str.substr(0, pos);
        if (modifier == "Ctrl") {
            combo.modifiers = combo.modifiers | ModifierFlags::Ctrl;
        } else if (modifier == "Alt") {
            combo.modifiers = combo.modifiers | ModifierFlags::Alt;
        } else if (modifier == "Shift") {
            combo.modifiers = combo.modifiers | ModifierFlags::Shift;
        } else if (modifier == "Super") {
            combo.modifiers = combo.modifiers | ModifierFlags::Super;
        }
        pos++;
    }
    
    // Parse key name
    std::string keyName = str.substr(str.rfind('+') + 1);
    
    // Key name to KeyCode mapping (reverse of above)
    static const std::unordered_map<std::string, KeyCode> stringToKey = {
        {"A", KeyCode::A}, {"B", KeyCode::B}, {"C", KeyCode::C}, {"D", KeyCode::D},
        {"E", KeyCode::E}, {"F", KeyCode::F}, {"G", KeyCode::G}, {"H", KeyCode::H},
        {"I", KeyCode::I}, {"J", KeyCode::J}, {"K", KeyCode::K}, {"L", KeyCode::L},
        {"M", KeyCode::M}, {"N", KeyCode::N}, {"O", KeyCode::O}, {"P", KeyCode::P},
        {"Q", KeyCode::Q}, {"R", KeyCode::R}, {"S", KeyCode::S}, {"T", KeyCode::T},
        {"U", KeyCode::U}, {"V", KeyCode::V}, {"W", KeyCode::W}, {"X", KeyCode::X},
        {"Y", KeyCode::Y}, {"Z", KeyCode::Z},
        
        {"0", KeyCode::Num0}, {"1", KeyCode::Num1}, {"2", KeyCode::Num2},
        {"3", KeyCode::Num3}, {"4", KeyCode::Num4}, {"5", KeyCode::Num5},
        {"6", KeyCode::Num6}, {"7", KeyCode::Num7}, {"8", KeyCode::Num8},
        {"9", KeyCode::Num9},
        
        {"F1", KeyCode::F1}, {"F2", KeyCode::F2}, {"F3", KeyCode::F3},
        {"F4", KeyCode::F4}, {"F5", KeyCode::F5}, {"F6", KeyCode::F6},
        {"F7", KeyCode::F7}, {"F8", KeyCode::F8}, {"F9", KeyCode::F9},
        {"F10", KeyCode::F10}, {"F11", KeyCode::F11}, {"F12", KeyCode::F12},
        
        {"Space", KeyCode::Space}, {"Enter", KeyCode::Enter},
        {"Escape", KeyCode::Escape}, {"Tab", KeyCode::Tab},
        {"Backspace", KeyCode::Backspace}, {"Delete", KeyCode::Delete},
        {"Insert", KeyCode::Insert}, {"Home", KeyCode::Home},
        {"End", KeyCode::End}, {"PageUp", KeyCode::PageUp},
        {"PageDown", KeyCode::PageDown},
        
        {"Up", KeyCode::Up}, {"Down", KeyCode::Down},
        {"Left", KeyCode::Left}, {"Right", KeyCode::Right},
        
        {"Shift", KeyCode::Shift}, {"Ctrl", KeyCode::Ctrl},
        {"Alt", KeyCode::Alt}, {"Super", KeyCode::Super}
    };
    
    auto it = stringToKey.find(keyName);
    if (it != stringToKey.end()) {
        combo.primaryKey = it->second;
    } else {
        combo.primaryKey = KeyCode::Unknown;
    }
    
    return combo;
}

}
}