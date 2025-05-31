#include "InputMapping.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace VoxelEditor {
namespace Input {

void InputMapping::bindKey(KeyCode key, const std::string& action, ModifierFlags modifiers) {
    uint32_t hash = hashKey(key, modifiers);
    keys[hash] = action;
}

void InputMapping::bindKeyCombination(const KeyCombination& combination, const std::string& action) {
    uint64_t hash = hashKeyCombination(combination);
    keyCombinations[hash] = action;
}

void InputMapping::bindMouseButton(MouseButton button, const std::string& action, ModifierFlags modifiers) {
    uint32_t hash = hashMouseButton(button, modifiers);
    mouseButtons[hash] = action;
}

void InputMapping::bindTouchGesture(TouchGesture gesture, const std::string& action) {
    touchGestures[static_cast<uint32_t>(gesture)] = action;
}

void InputMapping::bindVRGesture(VRGesture gesture, const std::string& action) {
    vrGestures[static_cast<uint32_t>(gesture)] = action;
}

std::string InputMapping::getKeyAction(KeyCode key, ModifierFlags modifiers) const {
    uint32_t hash = hashKey(key, modifiers);
    auto it = keys.find(hash);
    return (it != keys.end()) ? it->second : "";
}

std::string InputMapping::getKeyCombinationAction(const KeyCombination& combo) const {
    uint64_t hash = hashKeyCombination(combo);
    auto it = keyCombinations.find(hash);
    return (it != keyCombinations.end()) ? it->second : "";
}

std::string InputMapping::getMouseButtonAction(MouseButton button, ModifierFlags modifiers) const {
    uint32_t hash = hashMouseButton(button, modifiers);
    auto it = mouseButtons.find(hash);
    return (it != mouseButtons.end()) ? it->second : "";
}

std::string InputMapping::getTouchGestureAction(TouchGesture gesture) const {
    auto it = touchGestures.find(static_cast<uint32_t>(gesture));
    return (it != touchGestures.end()) ? it->second : "";
}

std::string InputMapping::getVRGestureAction(VRGesture gesture) const {
    auto it = vrGestures.find(static_cast<uint32_t>(gesture));
    return (it != vrGestures.end()) ? it->second : "";
}

bool InputMapping::isValid() const {
    // Check sensitivity values
    if (mouseSensitivity <= 0.0f || mouseSensitivity > 10.0f) return false;
    if (touchSensitivity <= 0.0f || touchSensitivity > 10.0f) return false;
    if (vrSensitivity <= 0.0f || vrSensitivity > 10.0f) return false;
    
    // Check timeout values
    if (mouseClickTimeout <= 0.0f || mouseDoubleClickTimeout <= 0.0f) return false;
    if (touchTapTimeout <= 0.0f) return false;
    
    // Check threshold values
    if (mouseDragThreshold <= 0.0f) return false;
    if (touchTapRadius <= 0.0f || touchPinchThreshold <= 0.0f || touchSwipeThreshold <= 0.0f) return false;
    
    return true;
}

std::vector<std::string> InputMapping::validate() const {
    std::vector<std::string> issues;
    
    if (mouseSensitivity <= 0.0f || mouseSensitivity > 10.0f) {
        issues.push_back("mouseSensitivity must be between 0 and 10");
    }
    if (touchSensitivity <= 0.0f || touchSensitivity > 10.0f) {
        issues.push_back("touchSensitivity must be between 0 and 10");
    }
    if (vrSensitivity <= 0.0f || vrSensitivity > 10.0f) {
        issues.push_back("vrSensitivity must be between 0 and 10");
    }
    
    if (mouseClickTimeout <= 0.0f) {
        issues.push_back("mouseClickTimeout must be positive");
    }
    if (mouseDoubleClickTimeout <= 0.0f) {
        issues.push_back("mouseDoubleClickTimeout must be positive");
    }
    if (mouseDragThreshold <= 0.0f) {
        issues.push_back("mouseDragThreshold must be positive");
    }
    
    if (touchTapTimeout <= 0.0f) {
        issues.push_back("touchTapTimeout must be positive");
    }
    if (touchTapRadius <= 0.0f) {
        issues.push_back("touchTapRadius must be positive");
    }
    if (touchPinchThreshold <= 0.0f) {
        issues.push_back("touchPinchThreshold must be positive");
    }
    if (touchSwipeThreshold <= 0.0f) {
        issues.push_back("touchSwipeThreshold must be positive");
    }
    
    return issues;
}

uint64_t InputMapping::hashKeyCombination(const KeyCombination& combo) const {
    return (static_cast<uint64_t>(combo.primaryKey) << 32) | static_cast<uint64_t>(combo.modifiers);
}

uint32_t InputMapping::hashMouseButton(MouseButton button, ModifierFlags modifiers) const {
    return (static_cast<uint32_t>(button) << 16) | static_cast<uint32_t>(modifiers);
}

uint32_t InputMapping::hashKey(KeyCode key, ModifierFlags modifiers) const {
    return (static_cast<uint32_t>(key) << 16) | static_cast<uint32_t>(modifiers);
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
    file << "mouseClickTimeout=" << mouseClickTimeout << "\n";
    file << "mouseDoubleClickTimeout=" << mouseDoubleClickTimeout << "\n";
    file << "mouseDragThreshold=" << mouseDragThreshold << "\n";
    file << "touchTapTimeout=" << touchTapTimeout << "\n";
    file << "touchTapRadius=" << touchTapRadius << "\n";
    file << "touchPinchThreshold=" << touchPinchThreshold << "\n";
    file << "touchSwipeThreshold=" << touchSwipeThreshold << "\n";
    file << "\n";
    
    // Save key bindings
    file << "[KeyBindings]\n";
    for (const auto& pair : keys) {
        uint32_t key = (pair.first >> 16) & 0xFFFF;
        uint32_t modifiers = pair.first & 0xFFFF;
        file << key << ":" << modifiers << "=" << pair.second << "\n";
    }
    file << "\n";
    
    // Save mouse bindings
    file << "[MouseBindings]\n";
    for (const auto& pair : mouseButtons) {
        uint32_t button = (pair.first >> 16) & 0xFFFF;
        uint32_t modifiers = pair.first & 0xFFFF;
        file << button << ":" << modifiers << "=" << pair.second << "\n";
    }
    file << "\n";
    
    // Save touch bindings
    file << "[TouchBindings]\n";
    for (const auto& pair : touchGestures) {
        file << pair.first << "=" << pair.second << "\n";
    }
    file << "\n";
    
    // Save VR bindings
    file << "[VRBindings]\n";
    for (const auto& pair : vrGestures) {
        file << pair.first << "=" << pair.second << "\n";
    }
    
    // Save VR comfort settings
    file << "\n[VRComfort]\n";
    file << "snapTurning=" << (vrComfortSettings.snapTurning ? "true" : "false") << "\n";
    file << "vignetteOnTurn=" << (vrComfortSettings.vignetteOnTurn ? "true" : "false") << "\n";
    file << "teleportMovement=" << (vrComfortSettings.teleportMovement ? "true" : "false") << "\n";
    file << "smoothMovement=" << (vrComfortSettings.smoothMovement ? "true" : "false") << "\n";
    file << "turnSpeed=" << vrComfortSettings.turnSpeed << "\n";
    file << "snapTurnAngle=" << vrComfortSettings.snapTurnAngle << "\n";
    file << "comfortZoneRadius=" << vrComfortSettings.comfortZoneRadius << "\n";
    
    return true;
}

bool InputMapping::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Clear existing mappings
    keys.clear();
    mouseButtons.clear();
    touchGestures.clear();
    vrGestures.clear();
    keyCombinations.clear();
    handPoses.clear();
    
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
            } else if (key == "mouseClickTimeout") {
                mouseClickTimeout = std::stof(value);
            } else if (key == "mouseDoubleClickTimeout") {
                mouseDoubleClickTimeout = std::stof(value);
            } else if (key == "mouseDragThreshold") {
                mouseDragThreshold = std::stof(value);
            } else if (key == "touchTapTimeout") {
                touchTapTimeout = std::stof(value);
            } else if (key == "touchTapRadius") {
                touchTapRadius = std::stof(value);
            } else if (key == "touchPinchThreshold") {
                touchPinchThreshold = std::stof(value);
            } else if (key == "touchSwipeThreshold") {
                touchSwipeThreshold = std::stof(value);
            }
        } else if (currentSection == "KeyBindings") {
            // Parse key:modifiers format
            size_t colonPos = key.find(':');
            if (colonPos != std::string::npos) {
                uint32_t keyCode = std::stoi(key.substr(0, colonPos));
                uint32_t modifiers = std::stoi(key.substr(colonPos + 1));
                uint32_t hash = (keyCode << 16) | modifiers;
                keys[hash] = value;
            }
        } else if (currentSection == "MouseBindings") {
            // Parse button:modifiers format
            size_t colonPos = key.find(':');
            if (colonPos != std::string::npos) {
                uint32_t button = std::stoi(key.substr(0, colonPos));
                uint32_t modifiers = std::stoi(key.substr(colonPos + 1));
                uint32_t hash = (button << 16) | modifiers;
                mouseButtons[hash] = value;
            }
        } else if (currentSection == "TouchBindings") {
            touchGestures[std::stoi(key)] = value;
        } else if (currentSection == "VRBindings") {
            vrGestures[std::stoi(key)] = value;
        } else if (currentSection == "VRComfort") {
            if (key == "snapTurning") {
                vrComfortSettings.snapTurning = (value == "true");
            } else if (key == "vignetteOnTurn") {
                vrComfortSettings.vignetteOnTurn = (value == "true");
            } else if (key == "teleportMovement") {
                vrComfortSettings.teleportMovement = (value == "true");
            } else if (key == "smoothMovement") {
                vrComfortSettings.smoothMovement = (value == "true");
            } else if (key == "turnSpeed") {
                vrComfortSettings.turnSpeed = std::stof(value);
            } else if (key == "snapTurnAngle") {
                vrComfortSettings.snapTurnAngle = std::stof(value);
            } else if (key == "comfortZoneRadius") {
                vrComfortSettings.comfortZoneRadius = std::stof(value);
            }
        }
    }
    
    return true;
}

std::string InputMapping::toJson() const {
    std::ostringstream json;
    json << "{\n";
    
    // Settings
    json << "  \"settings\": {\n";
    json << "    \"mouseSensitivity\": " << mouseSensitivity << ",\n";
    json << "    \"touchSensitivity\": " << touchSensitivity << ",\n";
    json << "    \"vrSensitivity\": " << vrSensitivity << ",\n";
    json << "    \"mouseClickTimeout\": " << mouseClickTimeout << ",\n";
    json << "    \"mouseDoubleClickTimeout\": " << mouseDoubleClickTimeout << ",\n";
    json << "    \"mouseDragThreshold\": " << mouseDragThreshold << ",\n";
    json << "    \"touchTapTimeout\": " << touchTapTimeout << ",\n";
    json << "    \"touchTapRadius\": " << touchTapRadius << ",\n";
    json << "    \"touchPinchThreshold\": " << touchPinchThreshold << ",\n";
    json << "    \"touchSwipeThreshold\": " << touchSwipeThreshold << "\n";
    json << "  },\n";
    
    // Key bindings
    json << "  \"keyBindings\": [\n";
    bool first = true;
    for (const auto& pair : keys) {
        if (!first) json << ",\n";
        first = false;
        uint32_t key = (pair.first >> 16) & 0xFFFF;
        uint32_t modifiers = pair.first & 0xFFFF;
        json << "    {\"key\": " << key << ", \"modifiers\": " << modifiers 
             << ", \"action\": \"" << pair.second << "\"}";
    }
    json << "\n  ],\n";
    
    // Mouse bindings
    json << "  \"mouseBindings\": [\n";
    first = true;
    for (const auto& pair : mouseButtons) {
        if (!first) json << ",\n";
        first = false;
        uint32_t button = (pair.first >> 16) & 0xFFFF;
        uint32_t modifiers = pair.first & 0xFFFF;
        json << "    {\"button\": " << button << ", \"modifiers\": " << modifiers 
             << ", \"action\": \"" << pair.second << "\"}";
    }
    json << "\n  ],\n";
    
    // Touch bindings
    json << "  \"touchBindings\": [\n";
    first = true;
    for (const auto& pair : touchGestures) {
        if (!first) json << ",\n";
        first = false;
        json << "    {\"gesture\": " << pair.first << ", \"action\": \"" << pair.second << "\"}";
    }
    json << "\n  ],\n";
    
    // VR bindings
    json << "  \"vrBindings\": [\n";
    first = true;
    for (const auto& pair : vrGestures) {
        if (!first) json << ",\n";
        first = false;
        json << "    {\"gesture\": " << pair.first << ", \"action\": \"" << pair.second << "\"}";
    }
    json << "\n  ],\n";
    
    // VR comfort settings
    json << "  \"vrComfort\": {\n";
    json << "    \"snapTurning\": " << (vrComfortSettings.snapTurning ? "true" : "false") << ",\n";
    json << "    \"vignetteOnTurn\": " << (vrComfortSettings.vignetteOnTurn ? "true" : "false") << ",\n";
    json << "    \"teleportMovement\": " << (vrComfortSettings.teleportMovement ? "true" : "false") << ",\n";
    json << "    \"smoothMovement\": " << (vrComfortSettings.smoothMovement ? "true" : "false") << ",\n";
    json << "    \"turnSpeed\": " << vrComfortSettings.turnSpeed << ",\n";
    json << "    \"snapTurnAngle\": " << vrComfortSettings.snapTurnAngle << ",\n";
    json << "    \"comfortZoneRadius\": " << vrComfortSettings.comfortZoneRadius << "\n";
    json << "  }\n";
    
    json << "}";
    return json.str();
}

bool InputMapping::fromJson(const std::string& jsonStr) {
    // This is a simplified JSON parser - in production, use a proper JSON library
    // For now, we'll just check if the string is not empty
    if (jsonStr.empty()) {
        return false;
    }
    
    // Clear existing mappings
    keys.clear();
    mouseButtons.clear();
    touchGestures.clear();
    vrGestures.clear();
    keyCombinations.clear();
    handPoses.clear();
    
    // TODO: Implement proper JSON parsing
    // This is a placeholder that accepts any non-empty JSON
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
    mapping.bindKey(KeyCode::Home, Actions::RESET_CAMERA);
    mapping.bindKey(KeyCode::F, Actions::FRAME_SELECTION);
    
    // Edit operations
    mapping.bindKey(KeyCode::Delete, Actions::DELETE);
    mapping.bindKey(KeyCode::X, Actions::CUT, ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::C, Actions::COPY, ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::V, Actions::PASTE, ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::Z, Actions::UNDO, ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::Y, Actions::REDO, ModifierFlags::Ctrl);
    
    // File operations
    mapping.bindKey(KeyCode::S, Actions::SAVE_FILE, ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::O, Actions::OPEN_FILE, ModifierFlags::Ctrl);
    mapping.bindKey(KeyCode::N, Actions::NEW_FILE, ModifierFlags::Ctrl);
    
    // Mouse bindings
    mapping.bindMouseButton(MouseButton::Left, Actions::SELECT_VOXEL);
    mapping.bindMouseButton(MouseButton::Middle, Actions::PAN_CAMERA);
    mapping.bindMouseButton(MouseButton::Right, Actions::ORBIT_CAMERA);
    
    // Touch gestures
    mapping.bindTouchGesture(TouchGesture::Tap, Actions::SELECT_VOXEL);
    mapping.bindTouchGesture(TouchGesture::Pan, Actions::PAN_CAMERA);
    mapping.bindTouchGesture(TouchGesture::Pinch, Actions::ZOOM_CAMERA);
    mapping.bindTouchGesture(TouchGesture::Rotation, Actions::ORBIT_CAMERA);
    
    // VR gestures
    mapping.bindVRGesture(VRGesture::Point, Actions::VR_POINT);
    mapping.bindVRGesture(VRGesture::Grab, Actions::VR_GRAB);
    mapping.bindVRGesture(VRGesture::Pinch, Actions::PLACE_VOXEL);
    
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

InputMapping InputMapping::Accessibility() {
    InputMapping mapping = Default();
    
    // Accessibility features
    mapping.bindKey(KeyCode::Enter, Actions::SELECT_VOXEL);
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

InputMapping InputMapping::VROptimized() {
    InputMapping mapping;
    
    // VR-specific bindings
    mapping.bindVRGesture(VRGesture::Point, "teleport");
    mapping.bindVRGesture(VRGesture::Grab, Actions::VR_GRAB);
    mapping.bindVRGesture(VRGesture::Pinch, "precise_select");
    mapping.bindVRGesture(VRGesture::OpenPalm, Actions::VR_MENU);
    mapping.bindVRGesture(VRGesture::ThumbsUp, "confirm");
    mapping.bindVRGesture(VRGesture::Peace, Actions::UNDO);
    
    // VR comfort settings
    mapping.vrComfortSettings = VRComfortSettings::Comfort();
    mapping.vrSensitivity = 1.2f;
    
    return mapping;
}

}
}