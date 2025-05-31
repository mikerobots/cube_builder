#include "InputMapping.h"
#include <fstream>
#include <sstream>

namespace VoxelEditor {
namespace Input {

InputMapping::InputMapping() 
    : mouseSensitivity(1.0f)
    , touchSensitivity(1.0f)
    , vrSensitivity(1.0f)
    , vrComfortSettings(VRComfortSettings::Default()) {
}

void InputMapping::bindKey(KeyCode key, const std::string& action, ModifierFlags modifiers) {
    KeyCombination combo(key, modifiers);
    keyBindings[combo] = action;
}

void InputMapping::bindKeyCombination(const KeyCombination& combination, const std::string& action) {
    keyBindings[combination] = action;
}

void InputMapping::bindMouseButton(MouseButton button, const std::string& action, ModifierFlags modifiers) {
    InputTrigger trigger(button, modifiers);
    mouseBindings[trigger] = action;
}

void InputMapping::bindTouchGesture(TouchGesture gesture, const std::string& action) {
    touchBindings[gesture] = action;
}

void InputMapping::bindVRGesture(VRGesture gesture, const std::string& action) {
    vrBindings[gesture] = action;
}

void InputMapping::unbindKey(KeyCode key, ModifierFlags modifiers) {
    KeyCombination combo(key, modifiers);
    keyBindings.erase(combo);
}

void InputMapping::unbindMouseButton(MouseButton button, ModifierFlags modifiers) {
    InputTrigger trigger(button, modifiers);
    mouseBindings.erase(trigger);
}

void InputMapping::unbindTouchGesture(TouchGesture gesture) {
    touchBindings.erase(gesture);
}

void InputMapping::unbindVRGesture(VRGesture gesture) {
    vrBindings.erase(gesture);
}

std::string InputMapping::getKeyAction(KeyCode key, ModifierFlags modifiers) const {
    KeyCombination combo(key, modifiers);
    auto it = keyBindings.find(combo);
    return (it != keyBindings.end()) ? it->second : "";
}

std::string InputMapping::getMouseAction(MouseButton button, ModifierFlags modifiers) const {
    InputTrigger trigger(button, modifiers);
    auto it = mouseBindings.find(trigger);
    return (it != mouseBindings.end()) ? it->second : "";
}

std::string InputMapping::getTouchAction(TouchGesture gesture) const {
    auto it = touchBindings.find(gesture);
    return (it != touchBindings.end()) ? it->second : "";
}

std::string InputMapping::getVRAction(VRGesture gesture) const {
    auto it = vrBindings.find(gesture);
    return (it != vrBindings.end()) ? it->second : "";
}

void InputMapping::clear() {
    keyBindings.clear();
    mouseBindings.clear();
    touchBindings.clear();
    vrBindings.clear();
}

bool InputMapping::isActionBound(const std::string& action) const {
    // Check key bindings
    for (const auto& pair : keyBindings) {
        if (pair.second == action) return true;
    }
    
    // Check mouse bindings
    for (const auto& pair : mouseBindings) {
        if (pair.second == action) return true;
    }
    
    // Check touch bindings
    for (const auto& pair : touchBindings) {
        if (pair.second == action) return true;
    }
    
    // Check VR bindings
    for (const auto& pair : vrBindings) {
        if (pair.second == action) return true;
    }
    
    return false;
}

std::vector<std::string> InputMapping::getAllActions() const {
    std::vector<std::string> actions;
    
    // Collect all unique actions
    for (const auto& pair : keyBindings) {
        if (std::find(actions.begin(), actions.end(), pair.second) == actions.end()) {
            actions.push_back(pair.second);
        }
    }
    
    for (const auto& pair : mouseBindings) {
        if (std::find(actions.begin(), actions.end(), pair.second) == actions.end()) {
            actions.push_back(pair.second);
        }
    }
    
    for (const auto& pair : touchBindings) {
        if (std::find(actions.begin(), actions.end(), pair.second) == actions.end()) {
            actions.push_back(pair.second);
        }
    }
    
    for (const auto& pair : vrBindings) {
        if (std::find(actions.begin(), actions.end(), pair.second) == actions.end()) {
            actions.push_back(pair.second);
        }
    }
    
    return actions;
}

bool InputMapping::validate() const {
    // Check for duplicate key bindings
    std::set<KeyCombination> usedKeys;
    for (const auto& pair : keyBindings) {
        if (usedKeys.count(pair.first)) {
            return false; // Duplicate key binding
        }
        usedKeys.insert(pair.first);
    }
    
    // Check for duplicate mouse bindings
    std::set<InputTrigger> usedMouse;
    for (const auto& pair : mouseBindings) {
        if (usedMouse.count(pair.first)) {
            return false; // Duplicate mouse binding
        }
        usedMouse.insert(pair.first);
    }
    
    // Check for duplicate touch bindings
    std::set<TouchGesture> usedTouch;
    for (const auto& pair : touchBindings) {
        if (usedTouch.count(pair.first)) {
            return false; // Duplicate touch binding
        }
        usedTouch.insert(pair.first);
    }
    
    // Check for duplicate VR bindings
    std::set<VRGesture> usedVR;
    for (const auto& pair : vrBindings) {
        if (usedVR.count(pair.first)) {
            return false; // Duplicate VR binding
        }
        usedVR.insert(pair.first);
    }
    
    return true;
}

void InputMapping::merge(const InputMapping& other) {
    // Merge key bindings
    for (const auto& pair : other.keyBindings) {
        keyBindings[pair.first] = pair.second;
    }
    
    // Merge mouse bindings
    for (const auto& pair : other.mouseBindings) {
        mouseBindings[pair.first] = pair.second;
    }
    
    // Merge touch bindings
    for (const auto& pair : other.touchBindings) {
        touchBindings[pair.first] = pair.second;
    }
    
    // Merge VR bindings
    for (const auto& pair : other.vrBindings) {
        vrBindings[pair.first] = pair.second;
    }
    
    // Update settings if they differ from defaults
    if (other.mouseSensitivity != 1.0f) {
        mouseSensitivity = other.mouseSensitivity;
    }
    if (other.touchSensitivity != 1.0f) {
        touchSensitivity = other.touchSensitivity;
    }
    if (other.vrSensitivity != 1.0f) {
        vrSensitivity = other.vrSensitivity;
    }
}

bool InputMapping::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Save sensitivity settings
    file << "[Settings]\n";
    file << "mouseSensitivity=" << mouseSensitivity << "\n";
    file << "touchSensitivity=" << touchSensitivity << "\n";
    file << "vrSensitivity=" << vrSensitivity << "\n";
    file << "\n";
    
    // Save key bindings
    file << "[KeyBindings]\n";
    for (const auto& pair : keyBindings) {
        file << pair.first.toString() << "=" << pair.second << "\n";
    }
    file << "\n";
    
    // Save mouse bindings
    file << "[MouseBindings]\n";
    for (const auto& pair : mouseBindings) {
        file << static_cast<int>(pair.first.mouseButton) << ":" << static_cast<int>(pair.first.modifiers) 
             << "=" << pair.second << "\n";
    }
    file << "\n";
    
    // Save touch bindings
    file << "[TouchBindings]\n";
    for (const auto& pair : touchBindings) {
        file << static_cast<int>(pair.first) << "=" << pair.second << "\n";
    }
    file << "\n";
    
    // Save VR bindings
    file << "[VRBindings]\n";
    for (const auto& pair : vrBindings) {
        file << static_cast<int>(pair.first) << "=" << pair.second << "\n";
    }
    
    return true;
}

bool InputMapping::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    clear();
    
    std::string line;
    std::string currentSection;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Check for section headers
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Parse key=value pairs
        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);
        
        if (currentSection == "Settings") {
            if (key == "mouseSensitivity") {
                mouseSensitivity = std::stof(value);
            } else if (key == "touchSensitivity") {
                touchSensitivity = std::stof(value);
            } else if (key == "vrSensitivity") {
                vrSensitivity = std::stof(value);
            }
        } else if (currentSection == "KeyBindings") {
            KeyCombination combo = KeyCombination::fromString(key);
            keyBindings[combo] = value;
        }
        // Additional parsing for mouse, touch, and VR bindings would go here
    }
    
    return true;
}

InputMapping InputMapping::Default() {
    InputMapping mapping;
    
    // Basic navigation
    mapping.bindKey(KeyCode::W, "move_forward");
    mapping.bindKey(KeyCode::S, "move_backward");
    mapping.bindKey(KeyCode::A, "move_left");
    mapping.bindKey(KeyCode::D, "move_right");
    mapping.bindKey(KeyCode::Q, "move_down");
    mapping.bindKey(KeyCode::E, "move_up");
    
    // Camera controls
    mapping.bindKey(KeyCode::Home, "reset_camera");
    mapping.bindKey(KeyCode::F, "frame_selection");
    
    // Edit operations
    mapping.bindKey(KeyCode::Delete, "delete_selection");
    mapping.bindKey(KeyCode::X, "cut", ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::C, "copy", ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::V, "paste", ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::Z, "undo", ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::Y, "redo", ModifierFlags::Ctrl);
    
    // File operations
    mapping.bindKey(KeyCode::S, "save", ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::O, "open", ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::N, "new", ModifierFlags::Ctrl);
    
    // Mouse bindings
    mapping.bindMouseButton(MouseButton::Left, "select");
    mapping.bindMouseButton(MouseButton::Middle, "pan_camera");
    mapping.bindMouseButton(MouseButton::Right, "context_menu");
    
    // Touch gestures
    mapping.bindTouchGesture(TouchGesture::Tap, "select");
    mapping.bindTouchGesture(TouchGesture::Pan, "pan_camera");
    mapping.bindTouchGesture(TouchGesture::Pinch, "zoom_camera");
    mapping.bindTouchGesture(TouchGesture::Rotate, "rotate_camera");
    
    // VR gestures
    mapping.bindVRGesture(VRGesture::Point, "select");
    mapping.bindVRGesture(VRGesture::Grab, "move_object");
    mapping.bindVRGesture(VRGesture::Pinch, "scale_object");
    
    return mapping;
}

InputMapping InputMapping::Gaming() {
    InputMapping mapping = Default();
    
    // Override with gaming-focused bindings
    mapping.bindKey(KeyCode::Space, "jump");
    mapping.bindKey(KeyCode::Shift, "run", ModifierFlags::None);
    mapping.bindKey(KeyCode::Ctrl, "crouch", ModifierFlags::None);
    mapping.bindKey(KeyCode::R, "reload");
    mapping.bindKey(KeyCode::Tab, "inventory");
    
    // Mouse sensitivity for gaming
    mapping.mouseSensitivity = 1.5f;
    
    return mapping;
}

InputMapping InputMapping::Productivity() {
    InputMapping mapping = Default();
    
    // Productivity shortcuts
    mapping.bindKey(KeyCode::S, "save", ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::S, "save_as", ModifierFlags::Ctrl | ModifierFlags::Shift);
    mapping.bindKey(KeyCode::D, "duplicate", ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::G, "group", ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::U, "ungroup", ModifierFlags::Ctrl);
    
    // Lower sensitivity for precision work
    mapping.mouseSensitivity = 0.8f;
    mapping.touchSensitivity = 0.8f;
    
    return mapping;
}

InputMapping InputMapping::VROptimized() {
    InputMapping mapping;
    
    // VR-specific bindings
    mapping.bindVRGesture(VRGesture::Point, "teleport");
    mapping.bindVRGesture(VRGesture::Grab, "grab_object");
    mapping.bindVRGesture(VRGesture::Pinch, "precise_select");
    mapping.bindVRGesture(VRGesture::Wave, "menu");
    mapping.bindVRGesture(VRGesture::Thumbs_Up, "confirm");
    mapping.bindVRGesture(VRGesture::Peace, "undo");
    
    // VR comfort settings
    mapping.vrComfortSettings = VRComfortSettings::Comfort();
    mapping.vrSensitivity = 1.2f;
    
    return mapping;
}

InputMapping InputMapping::Accessibility() {
    InputMapping mapping = Default();
    
    // Accessibility features
    mapping.bindKey(KeyCode::Enter, "select");
    mapping.bindKey(KeyCode::Space, "activate");
    mapping.bindKey(KeyCode::Escape, "cancel");
    mapping.bindKey(KeyCode::Tab, "next_item");
    mapping.bindKey(KeyCode::Tab, "previous_item", ModifierFlags::Shift);
    
    // Voice control alternatives
    mapping.bindKey(KeyCode::F1, "help");
    mapping.bindKey(KeyCode::F2, "speak_item");
    mapping.bindKey(KeyCode::F3, "toggle_voice_control");
    
    // Reduced sensitivity for motor impairments
    mapping.mouseSensitivity = 0.5f;
    mapping.touchSensitivity = 0.6f;
    
    return mapping;
}

}
}