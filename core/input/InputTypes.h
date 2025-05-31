#pragma once

#include "../../foundation/math/Vector2f.h"
#include "../../foundation/math/Vector2i.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Quaternion.h"
#include <chrono>
#include <vector>
#include <array>
#include <functional>
#include <deque>

namespace VoxelEditor {
namespace Input {

// Forward declarations
class InputManager;
class InputHandler;
class MouseHandler;
class KeyboardHandler;
class TouchHandler;
class VRInputHandler;

// Common type aliases
using ActionId = uint32_t;
using TimePoint = std::chrono::high_resolution_clock::time_point;

// Input device types
enum class InputDevice {
    Unknown,
    Mouse,
    Keyboard,
    Touch,
    VRController,
    VRHands
};

// Mouse input types
enum class MouseEventType {
    ButtonPress,
    ButtonRelease,
    Move,
    Wheel,
    Enter,
    Leave
};

enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Button6 = 5,
    Button7 = 6,
    Button8 = 7,
    None = 255
};

struct MouseEvent {
    MouseEventType type;
    MouseButton button;
    Math::Vector2f position;
    Math::Vector2f delta;
    float wheelDelta;
    uint32_t modifiers;
    TimePoint timestamp;
    
    MouseEvent() 
        : type(MouseEventType::Move)
        , button(MouseButton::None)
        , position(Math::Vector2f::zero())
        , delta(Math::Vector2f::zero())
        , wheelDelta(0.0f)
        , modifiers(0)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
    
    MouseEvent(MouseEventType t, MouseButton b, const Math::Vector2f& pos)
        : type(t), button(b), position(pos), delta(Math::Vector2f::zero())
        , wheelDelta(0.0f), modifiers(0)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
};

// Keyboard input types
enum class KeyEventType {
    Press,
    Release,
    Character
};

enum class KeyCode {
    // Letters
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Numbers
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    
    // Function keys
    F1 = 112, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    
    // Special keys
    Space = 32,
    Enter = 13,
    Escape = 27,
    Tab = 9,
    Backspace = 8,
    Delete = 127,
    Insert = 155,
    Home = 156,
    End = 157,
    PageUp = 158,
    PageDown = 159,
    
    // Arrow keys
    Up = 200,
    Down = 201,
    Left = 202,
    Right = 203,
    
    // Modifier keys
    Shift = 160,
    Ctrl = 162,
    Alt = 164,
    Super = 166,
    
    // Symbols
    Minus = 45,
    Plus = 43,
    Equals = 61,
    LeftBracket = 91,
    RightBracket = 93,
    Backslash = 92,
    Semicolon = 59,
    Quote = 39,
    Comma = 44,
    Period = 46,
    Slash = 47,
    Tilde = 96,
    
    Unknown = 0
};

enum class ModifierFlags : uint32_t {
    None = 0,
    Shift = 1 << 0,
    Ctrl = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3
};

inline ModifierFlags operator|(ModifierFlags a, ModifierFlags b) {
    return static_cast<ModifierFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ModifierFlags operator&(ModifierFlags a, ModifierFlags b) {
    return static_cast<ModifierFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool hasModifier(ModifierFlags flags, ModifierFlags modifier) {
    return (flags & modifier) != ModifierFlags::None;
}

struct KeyEvent {
    KeyEventType type;
    KeyCode key;
    char character;
    ModifierFlags modifiers;
    bool repeat;
    TimePoint timestamp;
    
    KeyEvent()
        : type(KeyEventType::Press), key(KeyCode::Unknown), character(0)
        , modifiers(ModifierFlags::None), repeat(false)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
    
    KeyEvent(KeyEventType t, KeyCode k, ModifierFlags mods = ModifierFlags::None)
        : type(t), key(k), character(0), modifiers(mods), repeat(false)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
};

// Touch input types
enum class TouchState {
    Pressed,
    Moved,
    Released,
    Cancelled
};

struct TouchPoint {
    int id;
    Math::Vector2f position;
    Math::Vector2f delta;
    float pressure;
    TouchState state;
    
    TouchPoint()
        : id(-1), position(Math::Vector2f::zero()), delta(Math::Vector2f::zero())
        , pressure(0.0f), state(TouchState::Released) {}
    
    TouchPoint(int touchId, const Math::Vector2f& pos, TouchState st)
        : id(touchId), position(pos), delta(Math::Vector2f::zero())
        , pressure(1.0f), state(st) {}
};

enum class TouchEventType {
    TouchBegin,
    TouchUpdate,
    TouchEnd,
    TouchCancel
};

struct TouchEvent {
    TouchEventType type;
    std::vector<TouchPoint> points;
    TimePoint timestamp;
    
    TouchEvent()
        : type(TouchEventType::TouchBegin)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
    
    TouchEvent(TouchEventType t, const std::vector<TouchPoint>& pts)
        : type(t), points(pts)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
};

enum class TouchGesture {
    Tap,
    DoubleTap,
    LongPress,
    Pan,
    Pinch,
    Rotation,
    Swipe,
    TwoFingerPan,
    ThreeFingerPan
};

// VR input types
enum class HandType {
    Left = 0,
    Right = 1,
    Either = 2
};

struct FingerPose {
    std::array<Math::Vector3f, 4> joints; // MCP, PIP, DIP, TIP
    float bend; // 0.0 = straight, 1.0 = fully bent
    bool extended;
    
    FingerPose() : bend(0.0f), extended(true) {
        for (auto& joint : joints) {
            joint = Math::Vector3f::Zero();
        }
    }
};

struct HandPose {
    Math::Vector3f position;
    Math::Quaternion orientation;
    std::array<FingerPose, 5> fingers;
    float confidence;
    HandType hand;
    
    HandPose()
        : position(Math::Vector3f::Zero())
        , orientation(Math::Quaternion::identity())
        , confidence(0.0f)
        , hand(HandType::Left) {}
};

enum class VREventType {
    HandUpdate,
    GestureDetected,
    GestureCompleted
};

enum class VRGesture {
    Point,
    Pinch,
    Grab,
    ThumbsUp,
    ThumbsDown,
    Fist,
    OpenPalm,
    Peace,
    TwoHandGrab,
    TwoHandScale,
    TwoHandRotate
};

struct VREvent {
    VREventType type;
    HandType hand;
    HandPose pose;
    std::vector<VRGesture> gestures;
    TimePoint timestamp;
    
    VREvent()
        : type(VREventType::HandUpdate), hand(HandType::Left)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
    
    VREvent(VREventType t, HandType h, const HandPose& p)
        : type(t), hand(h), pose(p)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
};

// Action system types
enum class ActionType {
    Button,     // On/off state
    Axis,       // Continuous value (-1 to 1)
    Vector2,    // 2D continuous value
    Vector3     // 3D continuous value
};

struct ActionContext {
    ActionType type;
    bool pressed = false;
    float value = 0.0f;
    Math::Vector2f vector2 = Math::Vector2f::zero();
    Math::Vector3f vector3 = Math::Vector3f::Zero();
    ModifierFlags modifiers = ModifierFlags::None;
    InputDevice device = InputDevice::Unknown;
    TimePoint timestamp;
    
    ActionContext() : timestamp(std::chrono::high_resolution_clock::now()) {}
    
    ActionContext(ActionType t) : type(t), timestamp(std::chrono::high_resolution_clock::now()) {}
};

using ActionCallback = std::function<void(const ActionContext&)>;

// Input trigger for action binding
struct InputTrigger {
    InputDevice device;
    union {
        MouseButton mouseButton;
        KeyCode keyCode;
        TouchGesture touchGesture;
        VRGesture vrGesture;
        uint32_t raw;
    } input;
    ModifierFlags requiredModifiers;
    
    InputTrigger() : device(InputDevice::Unknown), requiredModifiers(ModifierFlags::None) {
        input.raw = 0;
    }
    
    InputTrigger(MouseButton button, ModifierFlags mods = ModifierFlags::None)
        : device(InputDevice::Mouse), requiredModifiers(mods) {
        input.mouseButton = button;
    }
    
    InputTrigger(KeyCode key, ModifierFlags mods = ModifierFlags::None)
        : device(InputDevice::Keyboard), requiredModifiers(mods) {
        input.keyCode = key;
    }
    
    InputTrigger(TouchGesture gesture)
        : device(InputDevice::Touch), requiredModifiers(ModifierFlags::None) {
        input.touchGesture = gesture;
    }
    
    InputTrigger(VRGesture gesture)
        : device(InputDevice::VRHands), requiredModifiers(ModifierFlags::None) {
        input.vrGesture = gesture;
    }
    
    bool matches(const MouseEvent& event) const;
    bool matches(const KeyEvent& event) const;
    bool matches(TouchGesture gesture) const;
    bool matches(VRGesture gesture) const;
};

// Key combination structure
struct KeyCombination {
    KeyCode primaryKey;
    ModifierFlags modifiers;
    
    KeyCombination() : primaryKey(KeyCode::Unknown), modifiers(ModifierFlags::None) {}
    
    KeyCombination(KeyCode key, ModifierFlags mods = ModifierFlags::None)
        : primaryKey(key), modifiers(mods) {}
    
    bool matches(KeyCode key, ModifierFlags mods) const {
        return primaryKey == key && modifiers == mods;
    }
    
    std::string toString() const;
    static KeyCombination fromString(const std::string& str);
    
    // Comparison operators for use in containers
    bool operator==(const KeyCombination& other) const {
        return primaryKey == other.primaryKey && modifiers == other.modifiers;
    }
    
    bool operator!=(const KeyCombination& other) const {
        return !(*this == other);
    }
    
    bool operator<(const KeyCombination& other) const {
        if (primaryKey != other.primaryKey) {
            return primaryKey < other.primaryKey;
        }
        return modifiers < other.modifiers;
    }
};

// Action binding for mapping inputs to actions
struct ActionBinding {
    std::string name;
    ActionType type;
    std::vector<InputTrigger> triggers;
    float deadzone = 0.1f;
    bool continuous = false;
    
    ActionBinding() : type(ActionType::Button) {}
    
    ActionBinding(const std::string& actionName, ActionType actionType)
        : name(actionName), type(actionType) {}
};

// Cursor modes for desktop platforms
enum class CursorMode {
    Normal,
    Hidden,
    Captured
};

// VR comfort settings
struct VRComfortSettings {
    bool snapTurning = true;
    float snapTurnAngle = 30.0f;
    bool smoothTurning = false;
    float turnSpeed = 90.0f;
    bool vignetteOnTurn = true;
    float comfortZoneRadius = 2.0f;
    bool teleportMovement = true;
    bool smoothMovement = false;
    
    static VRComfortSettings Default() {
        return VRComfortSettings{};
    }
    
    static VRComfortSettings Comfort() {
        VRComfortSettings settings;
        settings.snapTurning = true;
        settings.vignetteOnTurn = true;
        settings.teleportMovement = true;
        settings.smoothMovement = false;
        return settings;
    }
    
    static VRComfortSettings Performance() {
        VRComfortSettings settings;
        settings.snapTurning = false;
        settings.smoothTurning = true;
        settings.vignetteOnTurn = false;
        settings.teleportMovement = false;
        settings.smoothMovement = true;
        return settings;
    }
};

enum class HandTrackingQuality {
    None,
    Low,
    Medium,
    High
};

}
}

// Hash function for KeyCombination to use with std::unordered_map
namespace std {
    template<>
    struct hash<VoxelEditor::Input::KeyCombination> {
        size_t operator()(const VoxelEditor::Input::KeyCombination& combo) const {
            size_t keyHash = std::hash<int>{}(static_cast<int>(combo.primaryKey));
            size_t modHash = std::hash<uint32_t>{}(static_cast<uint32_t>(combo.modifiers));
            return keyHash ^ (modHash << 1);
        }
    };
    
    // Hash specialization for gesture-hand pairs
    template<>
    struct hash<std::pair<VoxelEditor::Input::VRGesture, VoxelEditor::Input::HandType>> {
        size_t operator()(const std::pair<VoxelEditor::Input::VRGesture, VoxelEditor::Input::HandType>& p) const {
            return hash<int>()(static_cast<int>(p.first)) ^ (hash<int>()(static_cast<int>(p.second)) << 1);
        }
    };
}