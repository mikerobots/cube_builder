# Input System Subsystem

## Purpose
Provides unified input handling across all platforms (desktop, touch, VR) with platform-specific optimizations and consistent event processing.

## Key Components

### InputManager
**Responsibility**: Main input coordination and event routing
- Unified input event processing
- Platform-specific input handler registration
- Input mapping and configuration
- Event priority and filtering

### MouseHandler
**Responsibility**: Desktop mouse input processing
- Mouse movement and click detection
- Ray casting for 3D interaction
- Scroll wheel and gesture support
- Multi-button mouse support

### KeyboardHandler
**Responsibility**: Keyboard input processing
- Key press/release events
- Modifier key combinations
- Text input handling
- Customizable key bindings

### TouchHandler
**Responsibility**: Touch and gesture input (Qt/Mobile)
- Multi-touch gesture recognition
- Touch raycast for 3D interaction
- Gesture mapping to actions
- Touch-specific optimizations

### VRInputHandler
**Responsibility**: VR hand tracking and gesture input
- Hand pose recognition
- Gesture detection and classification
- 3D spatial interaction
- Comfort and accessibility features

## Interface Design

```cpp
class InputManager {
public:
    // Handler registration
    void registerMouseHandler(MouseHandler* handler);
    void registerKeyboardHandler(KeyboardHandler* handler);
    void registerTouchHandler(TouchHandler* handler);
    void registerVRHandler(VRInputHandler* handler);
    
    // Event processing
    void processEvents();
    void injectMouseEvent(const MouseEvent& event);
    void injectKeyboardEvent(const KeyEvent& event);
    void injectTouchEvent(const TouchEvent& event);
    void injectVREvent(const VREvent& event);
    
    // Input mapping
    void setInputMapping(const InputMapping& mapping);
    InputMapping getInputMapping() const;
    void saveInputMapping(const std::string& filename);
    void loadInputMapping(const std::string& filename);
    
    // State queries
    bool isKeyPressed(KeyCode key) const;
    bool isMouseButtonPressed(MouseButton button) const;
    Vector2f getMousePosition() const;
    Vector2f getMouseDelta() const;
    
    // Action system
    void bindAction(const std::string& actionName, InputTrigger trigger);
    bool isActionPressed(const std::string& actionName) const;
    bool isActionJustPressed(const std::string& actionName) const;
    float getActionValue(const std::string& actionName) const;
    
    // Configuration
    void setEnabled(bool enabled);
    void setMouseSensitivity(float sensitivity);
    void setTouchSensitivity(float sensitivity);
    void setVRSensitivity(float sensitivity);
    
private:
    std::vector<std::unique_ptr<InputHandler>> m_handlers;
    InputMapping m_mapping;
    InputState m_currentState;
    InputState m_previousState;
    std::unordered_map<std::string, ActionBinding> m_actions;
    EventDispatcher* m_eventDispatcher;
    bool m_enabled;
};
```

## Data Structures

### Input Events
```cpp
struct MouseEvent {
    MouseEventType type;
    MouseButton button;
    Vector2f position;
    Vector2f delta;
    float wheelDelta;
    ModifierFlags modifiers;
    std::chrono::high_resolution_clock::time_point timestamp;
};

enum class MouseEventType {
    ButtonPress,
    ButtonRelease,
    Move,
    Wheel,
    Enter,
    Leave
};

enum class MouseButton {
    Left, Right, Middle,
    Button4, Button5,
    None
};

struct KeyEvent {
    KeyEventType type;
    KeyCode key;
    char character;
    ModifierFlags modifiers;
    bool repeat;
    std::chrono::high_resolution_clock::time_point timestamp;
};

enum class KeyEventType {
    Press,
    Release,
    Character
};

struct TouchEvent {
    TouchEventType type;
    std::vector<TouchPoint> points;
    std::chrono::high_resolution_clock::time_point timestamp;
};

struct TouchPoint {
    int id;
    Vector2f position;
    Vector2f delta;
    float pressure;
    TouchState state;
};

enum class TouchState {
    Pressed,
    Moved,
    Released,
    Cancelled
};

struct VREvent {
    VREventType type;
    HandType hand;
    HandPose pose;
    std::vector<Gesture> gestures;
    std::chrono::high_resolution_clock::time_point timestamp;
};

enum class VREventType {
    HandUpdate,
    GestureDetected,
    GestureCompleted
};
```

### Input Mapping
```cpp
struct InputMapping {
    // Mouse mappings
    std::unordered_map<MouseButton, ActionId> mouseButtons;
    std::unordered_map<MouseGesture, ActionId> mouseGestures;
    
    // Keyboard mappings
    std::unordered_map<KeyCode, ActionId> keys;
    std::unordered_map<KeyCombination, ActionId> keyCombinations;
    
    // Touch mappings
    std::unordered_map<TouchGesture, ActionId> touchGestures;
    
    // VR mappings
    std::unordered_map<VRGesture, ActionId> vrGestures;
    std::unordered_map<HandPose, ActionId> handPoses;
    
    // Sensitivity settings
    float mouseSensitivity = 1.0f;
    float touchSensitivity = 1.0f;
    float vrSensitivity = 1.0f;
    
    static InputMapping Default();
    static InputMapping Gaming();
    static InputMapping Accessibility();
};

struct ActionBinding {
    std::string name;
    ActionType type;
    std::vector<InputTrigger> triggers;
    float deadzone = 0.1f;
    bool continuous = false;
};

enum class ActionType {
    Button,     // On/off state
    Axis,       // Continuous value
    Vector2,    // 2D continuous value
    Vector3     // 3D continuous value
};
```

## Mouse Input Processing

### MouseHandler
```cpp
class MouseHandler : public InputHandler {
public:
    void processEvent(const MouseEvent& event) override;
    
    // Ray casting
    Ray createRayFromMouse(const Vector2f& mousePos, const Camera& camera, const Vector2i& viewportSize);
    Face detectFaceUnderMouse(const Vector2f& mousePos, const VoxelGrid& grid, const Camera& camera);
    
    // Mouse state
    bool isButtonPressed(MouseButton button) const;
    Vector2f getPosition() const;
    Vector2f getDelta() const;
    float getWheelDelta() const;
    
    // Configuration
    void setClickTimeout(float seconds);
    void setDoubleClickTimeout(float seconds);
    void setDragThreshold(float pixels);
    
private:
    struct MouseState {
        std::array<bool, 8> buttonsPressed = {};
        Vector2f position = Vector2f::Zero();
        Vector2f delta = Vector2f::Zero();
        float wheelDelta = 0.0f;
        
        // Click detection
        std::array<std::chrono::high_resolution_clock::time_point, 8> lastClickTime;
        std::array<Vector2f, 8> clickPosition;
        std::array<int, 8> clickCount = {};
    } m_state;
    
    float m_clickTimeout;
    float m_doubleClickTimeout;
    float m_dragThreshold;
    
    void handleButtonPress(const MouseEvent& event);
    void handleButtonRelease(const MouseEvent& event);
    void handleMouseMove(const MouseEvent& event);
    void handleWheel(const MouseEvent& event);
    
    bool isDoubleClick(MouseButton button, const Vector2f& position) const;
    bool isDrag(MouseButton button, const Vector2f& position) const;
};
```

### Mouse Actions
```cpp
enum class MouseAction {
    // Voxel operations
    PlaceVoxel,
    RemoveVoxel,
    PaintVoxel,
    
    // Camera controls
    OrbitCamera,
    PanCamera,
    ZoomCamera,
    
    // Selection
    SelectVoxel,
    SelectMultiple,
    SelectBox,
    DeselectAll,
    
    // Groups
    CreateGroup,
    SelectGroup,
    MoveGroup,
    
    // View
    FrameSelection,
    ResetView
};
```

## Keyboard Input Processing

### KeyboardHandler
```cpp
class KeyboardHandler : public InputHandler {
public:
    void processEvent(const KeyEvent& event) override;
    
    // Key state queries
    bool isKeyPressed(KeyCode key) const;
    bool isKeyJustPressed(KeyCode key) const;
    bool isKeyJustReleased(KeyCode key) const;
    
    // Modifier queries
    bool isShiftPressed() const;
    bool isCtrlPressed() const;
    bool isAltPressed() const;
    
    // Text input
    std::string getTextInput() const;
    void clearTextInput();
    void setTextInputEnabled(bool enabled);
    
    // Key bindings
    void bindKey(KeyCode key, const std::string& action);
    void bindKeyCombination(const KeyCombination& combo, const std::string& action);
    
private:
    struct KeyState {
        std::array<bool, 256> keysPressed = {};
        std::array<bool, 256> keysJustPressed = {};
        std::array<bool, 256> keysJustReleased = {};
        std::string textInput;
        bool textInputEnabled = false;
        ModifierFlags modifiers = ModifierFlags::None;
    } m_state;
    
    std::unordered_map<KeyCode, std::string> m_keyBindings;
    std::unordered_map<KeyCombination, std::string> m_comboBindings;
    
    void updateKeyState(const KeyEvent& event);
    void handleTextInput(const KeyEvent& event);
    void checkKeyBindings(const KeyEvent& event);
};
```

### Key Combinations
```cpp
struct KeyCombination {
    KeyCode primaryKey;
    ModifierFlags modifiers;
    
    bool matches(KeyCode key, ModifierFlags mods) const;
    std::string toString() const;
    static KeyCombination fromString(const std::string& str);
};

enum class ModifierFlags : uint32_t {
    None = 0,
    Shift = 1 << 0,
    Ctrl = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3
};
```

## Touch Input Processing

### TouchHandler
```cpp
class TouchHandler : public InputHandler {
public:
    void processEvent(const TouchEvent& event) override;
    
    // Gesture recognition
    void enableGesture(TouchGesture gesture, bool enabled);
    bool isGestureActive(TouchGesture gesture) const;
    Vector2f getGestureCenter(TouchGesture gesture) const;
    float getGestureScale(TouchGesture gesture) const;
    
    // Touch state
    std::vector<TouchPoint> getActiveTouches() const;
    TouchPoint getPrimaryTouch() const;
    bool hasTouches() const;
    
    // Configuration
    void setTapTimeout(float seconds);
    void setTapRadius(float pixels);
    void setPinchThreshold(float pixels);
    void setSwipeThreshold(float pixels);
    
private:
    struct TouchState {
        std::unordered_map<int, TouchPoint> activeTouches;
        std::vector<TouchGesture> activeGestures;
        
        // Gesture state
        Vector2f pinchCenter;
        float pinchScale = 1.0f;
        Vector2f panDelta;
        Vector2f rotationCenter;
        float rotationAngle = 0.0f;
    } m_state;
    
    std::unique_ptr<GestureRecognizer> m_gestureRecognizer;
    
    void updateTouchPoints(const TouchEvent& event);
    void recognizeGestures();
    void handleGestureStart(TouchGesture gesture);
    void handleGestureUpdate(TouchGesture gesture);
    void handleGestureEnd(TouchGesture gesture);
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
```

## VR Input Processing

### VRInputHandler
```cpp
class VRInputHandler : public InputHandler {
public:
    void processEvent(const VREvent& event) override;
    
    // Hand tracking
    HandPose getHandPose(HandType hand) const;
    bool isHandTracking(HandType hand) const;
    Vector3f getHandPosition(HandType hand) const;
    Quaternion getHandOrientation(HandType hand) const;
    
    // Gesture recognition
    bool isGestureActive(VRGesture gesture, HandType hand = HandType::Either) const;
    float getGestureConfidence(VRGesture gesture, HandType hand) const;
    Vector3f getGesturePosition(VRGesture gesture, HandType hand) const;
    
    // Ray casting
    Ray getHandRay(HandType hand) const;
    Face detectFaceFromHand(HandType hand, const VoxelGrid& grid) const;
    
    // Configuration
    void setGestureThreshold(VRGesture gesture, float threshold);
    void setHandTrackingEnabled(bool enabled);
    void setComfortSettings(const VRComfortSettings& settings);
    
private:
    struct VRState {
        std::array<HandPose, 2> handPoses;
        std::array<bool, 2> handTracking = {};
        std::vector<std::pair<VRGesture, HandType>> activeGestures;
        
        // Gesture history for filtering
        std::array<std::deque<HandPose>, 2> poseHistory;
    } m_state;
    
    std::unique_ptr<VRGestureRecognizer> m_gestureRecognizer;
    VRComfortSettings m_comfortSettings;
    
    void updateHandPose(HandType hand, const HandPose& pose);
    void recognizeGestures();
    void filterHandPose(HandPose& pose) const;
};

struct HandPose {
    Vector3f position;
    Quaternion orientation;
    std::array<FingerPose, 5> fingers;
    float confidence;
    HandType hand;
};

struct FingerPose {
    std::array<Vector3f, 4> joints; // MCP, PIP, DIP, TIP
    float bend; // 0.0 = straight, 1.0 = fully bent
    bool extended;
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

enum class HandType {
    Left,
    Right,
    Either
};
```

## Action System

### Action Processing
```cpp
class ActionProcessor {
public:
    void registerAction(const std::string& name, ActionCallback callback);
    void triggerAction(const std::string& name, const ActionContext& context);
    
    bool isActionActive(const std::string& name) const;
    float getActionValue(const std::string& name) const;
    Vector2f getActionVector2(const std::string& name) const;
    Vector3f getActionVector3(const std::string& name) const;
    
private:
    struct ActionState {
        bool active = false;
        float value = 0.0f;
        Vector2f vector2 = Vector2f::Zero();
        Vector3f vector3 = Vector3f::Zero();
        std::chrono::high_resolution_clock::time_point lastTriggered;
    };
    
    std::unordered_map<std::string, ActionCallback> m_callbacks;
    std::unordered_map<std::string, ActionState> m_states;
    
    void updateActionState(const std::string& name, const ActionContext& context);
};

struct ActionContext {
    ActionType type;
    bool pressed = false;
    float value = 0.0f;
    Vector2f vector2 = Vector2f::Zero();
    Vector3f vector3 = Vector3f::Zero();
    ModifierFlags modifiers = ModifierFlags::None;
    InputDevice device = InputDevice::Unknown;
};

using ActionCallback = std::function<void(const ActionContext&)>;
```

## Platform Integration

### Desktop Integration
```cpp
class DesktopInputIntegration {
public:
    void initializeForPlatform();
    void handleNativeEvent(void* nativeEvent);
    
    // Window integration
    void setWindow(void* windowHandle);
    void setCursorMode(CursorMode mode);
    void setRawMouseInput(bool enabled);
    
private:
    void* m_windowHandle;
    CursorMode m_cursorMode;
    bool m_rawMouseInput;
};

enum class CursorMode {
    Normal,
    Hidden,
    Captured
};
```

### VR Integration
```cpp
class VRInputIntegration {
public:
    bool initializeHandTracking();
    void updateHandTracking();
    void shutdown();
    
    bool isHandTrackingSupported() const;
    bool isHandTrackingEnabled() const;
    HandTrackingQuality getTrackingQuality() const;
    
private:
    void* m_handTrackingHandle;
    HandTrackingQuality m_trackingQuality;
};

enum class HandTrackingQuality {
    None,
    Low,
    Medium,
    High
};
```

## Testing Strategy

### Unit Tests
- Input event parsing
- Gesture recognition accuracy
- Key combination detection
- Action mapping validation

### Integration Tests
- Cross-platform input handling
- VR hand tracking integration
- Touch gesture recognition
- Camera control responsiveness

### Performance Tests
- Input latency measurement
- Event processing throughput
- Memory usage optimization
- VR tracking performance

## Dependencies
- **Core/Camera**: Camera control integration
- **Core/VoxelData**: Voxel interaction
- **Core/Selection**: Selection operations
- **Foundation/Events**: Input event dispatching
- **Foundation/Math**: 3D math for ray casting

## Configuration
- Customizable input mappings
- Platform-specific optimizations
- Accessibility options
- Sensitivity adjustments