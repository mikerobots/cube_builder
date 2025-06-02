#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <variant>

#include "InputTypes.h"
#include "InputHandler.h"
#include "InputMapping.h"
#include "../../foundation/events/EventDispatcher.h"

namespace VoxelEditor {
namespace Input {

// Forward declarations
class MouseHandler;
class KeyboardHandler;
class TouchHandler;
class VRInputHandler;

class InputManager {
public:
    explicit InputManager(VoxelEditor::Events::EventDispatcher* eventDispatcher = nullptr);
    ~InputManager();
    
    // Disable copy/move for singleton-like behavior
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    InputManager(InputManager&&) = delete;
    InputManager& operator=(InputManager&&) = delete;
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Handler registration
    void registerMouseHandler(std::unique_ptr<MouseHandler> handler);
    void registerKeyboardHandler(std::unique_ptr<KeyboardHandler> handler);
    void registerTouchHandler(std::unique_ptr<TouchHandler> handler);
    void registerVRHandler(std::unique_ptr<VRInputHandler> handler);
    
    // Event processing
    void update(float deltaTime);
    void processEvents();
    void injectMouseEvent(const MouseEvent& event);
    void injectKeyboardEvent(const KeyEvent& event);
    void injectTouchEvent(const TouchEvent& event);
    void injectVREvent(const VREvent& event);
    
    // Input mapping
    void setInputMapping(const InputMapping& mapping);
    const InputMapping& getInputMapping() const { return m_mapping; }
    void saveInputMapping(const std::string& filename) const;
    bool loadInputMapping(const std::string& filename);
    void resetToDefaultMapping();
    
    // State queries
    bool isKeyPressed(KeyCode key) const;
    bool isKeyJustPressed(KeyCode key) const;
    bool isKeyJustReleased(KeyCode key) const;
    bool isMouseButtonPressed(MouseButton button) const;
    bool isMouseButtonJustPressed(MouseButton button) const;
    bool isMouseButtonJustReleased(MouseButton button) const;
    Math::Vector2f getMousePosition() const;
    Math::Vector2f getMouseDelta() const;
    float getMouseWheelDelta() const;
    
    // Modifier key queries
    bool isShiftPressed() const;
    bool isCtrlPressed() const;
    bool isAltPressed() const;
    bool isSuperPressed() const;
    ModifierFlags getCurrentModifiers() const;
    
    // Touch state queries
    std::vector<TouchPoint> getActiveTouches() const;
    TouchPoint getPrimaryTouch() const;
    bool hasTouches() const;
    bool isGestureActive(TouchGesture gesture) const;
    
    // VR state queries
    bool isHandTracking(HandType hand) const;
    HandPose getHandPose(HandType hand) const;
    bool isVRGestureActive(VRGesture gesture, HandType hand = HandType::Either) const;
    
    // Action system
    void bindAction(const std::string& actionName, const InputTrigger& trigger);
    void bindAction(const std::string& actionName, const std::vector<InputTrigger>& triggers);
    void unbindAction(const std::string& actionName);
    void registerActionCallback(const std::string& actionName, ActionCallback callback);
    void unregisterActionCallback(const std::string& actionName);
    
    bool isActionPressed(const std::string& actionName) const;
    bool isActionJustPressed(const std::string& actionName) const;
    bool isActionJustReleased(const std::string& actionName) const;
    float getActionValue(const std::string& actionName) const;
    Math::Vector2f getActionVector2(const std::string& actionName) const;
    Math::Vector3f getActionVector3(const std::string& actionName) const;
    
    // Configuration
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    void setMouseSensitivity(float sensitivity);
    void setTouchSensitivity(float sensitivity);
    void setVRSensitivity(float sensitivity);
    float getMouseSensitivity() const { return m_mapping.mouseSensitivity; }
    float getTouchSensitivity() const { return m_mapping.touchSensitivity; }
    float getVRSensitivity() const { return m_mapping.vrSensitivity; }
    
    // Platform integration
    void setCursorMode(CursorMode mode);
    CursorMode getCursorMode() const { return m_cursorMode; }
    void setRawMouseInput(bool enabled);
    bool isRawMouseInputEnabled() const { return m_rawMouseInput; }
    
    // VR configuration
    void setVRComfortSettings(const VRComfortSettings& settings);
    const VRComfortSettings& getVRComfortSettings() const { return m_mapping.vrComfortSettings; }
    
    // Debug and statistics
    const InputState& getCurrentState() const { return m_currentState; }
    const InputState& getPreviousState() const { return m_previousState; }
    size_t getEventQueueSize() const;
    void clearEventQueue();
    
    // Handler access
    MouseHandler* getMouseHandler() const { return m_mouseHandler.get(); }
    KeyboardHandler* getKeyboardHandler() const { return m_keyboardHandler.get(); }
    TouchHandler* getTouchHandler() const { return m_touchHandler.get(); }
    VRInputHandler* getVRHandler() const { return m_vrHandler.get(); }
    
private:
    // Core components
    VoxelEditor::Events::EventDispatcher* m_eventDispatcher;
    
    // Input handlers
    std::unique_ptr<MouseHandler> m_mouseHandler;
    std::unique_ptr<KeyboardHandler> m_keyboardHandler;
    std::unique_ptr<TouchHandler> m_touchHandler;
    std::unique_ptr<VRInputHandler> m_vrHandler;
    
    // State management
    InputState m_currentState;
    InputState m_previousState;
    InputMapping m_mapping;
    
    // Action system
    struct ActionState {
        bool active = false;
        bool justPressed = false;
        bool justReleased = false;
        float value = 0.0f;
        Math::Vector2f vector2 = Math::Vector2f::zero();
        Math::Vector3f vector3 = Math::Vector3f::Zero();
        TimePoint lastTriggered;
        
        void reset() {
            justPressed = false;
            justReleased = false;
            if (!active) {
                value = 0.0f;
                vector2 = Math::Vector2f::zero();
                vector3 = Math::Vector3f::Zero();
            }
        }
    };
    
    std::unordered_map<std::string, ActionBinding> m_actionBindings;
    std::unordered_map<std::string, ActionState> m_actionStates;
    std::unordered_map<std::string, ActionCallback> m_actionCallbacks;
    
    // Configuration
    bool m_enabled;
    bool m_initialized;
    CursorMode m_cursorMode;
    bool m_rawMouseInput;
    
    // Event queue for thread-safe event injection
    struct QueuedEvent {
        enum Type { Mouse, Keyboard, Touch, VR } type;
        std::variant<MouseEvent, KeyEvent, TouchEvent, VREvent> data;
        
        QueuedEvent(const MouseEvent& event) : type(Mouse), data(event) {}
        QueuedEvent(const KeyEvent& event) : type(Keyboard), data(event) {}
        QueuedEvent(const TouchEvent& event) : type(Touch), data(event) {}
        QueuedEvent(const VREvent& event) : type(VR), data(event) {}
    };
    
    std::vector<QueuedEvent> m_eventQueue;
    mutable std::mutex m_eventQueueMutex;
    
    // Internal processing methods
    void processQueuedEvents();
    void processMouseEventInternal(const MouseEvent& event);
    void processKeyboardEventInternal(const KeyEvent& event);
    void processTouchEventInternal(const TouchEvent& event);
    void processVREventInternal(const VREvent& event);
    
    void updateInputState();
    void updateActionStates();
    void checkActionTriggers();
    void triggerAction(const std::string& actionName, const ActionContext& context);
    
    // Helper methods
    bool checkInputTrigger(const InputTrigger& trigger, const MouseEvent& event) const;
    bool checkInputTrigger(const InputTrigger& trigger, const KeyEvent& event) const;
    bool checkInputTrigger(const InputTrigger& trigger, TouchGesture gesture) const;
    bool checkInputTrigger(const InputTrigger& trigger, VRGesture gesture) const;
    
    ActionContext createActionContext(const ActionBinding& binding, bool pressed, float value = 0.0f) const;
    
    void initializeDefaultHandlers();
    void setupDefaultBindings();
};

}
}