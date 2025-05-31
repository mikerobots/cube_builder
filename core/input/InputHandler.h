#pragma once

#include "InputTypes.h"
#include "../../foundation/events/EventDispatcher.h"

namespace VoxelEditor {
namespace Input {

// Base class for all input handlers
class InputHandler {
public:
    explicit InputHandler(Events::EventDispatcher* eventDispatcher = nullptr)
        : m_eventDispatcher(eventDispatcher), m_enabled(true) {}
    
    virtual ~InputHandler() = default;
    
    // Pure virtual methods for specific input types
    virtual void processMouseEvent(const MouseEvent& event) {}
    virtual void processKeyboardEvent(const KeyEvent& event) {}
    virtual void processTouchEvent(const TouchEvent& event) {}
    virtual void processVREvent(const VREvent& event) {}
    
    // Frame update
    virtual void update(float deltaTime) {}
    
    // Configuration
    virtual void setEnabled(bool enabled) { m_enabled = enabled; }
    virtual bool isEnabled() const { return m_enabled; }
    
    // Event dispatcher access
    void setEventDispatcher(Events::EventDispatcher* dispatcher) { m_eventDispatcher = dispatcher; }
    Events::EventDispatcher* getEventDispatcher() const { return m_eventDispatcher; }
    
protected:
    // Helper to dispatch events if dispatcher is available
    template<typename EventType>
    void dispatchEvent(const EventType& event) {
        if (m_eventDispatcher) {
            m_eventDispatcher->dispatch(event);
        }
    }
    
    Events::EventDispatcher* m_eventDispatcher;
    bool m_enabled;
};

// Input state tracking
struct InputState {
    // Mouse state
    std::array<bool, 8> mouseButtons = {};
    Math::Vector2f mousePosition = Math::Vector2f::zero();
    Math::Vector2f mouseDelta = Math::Vector2f::zero();
    float mouseWheelDelta = 0.0f;
    
    // Keyboard state
    std::array<bool, 256> keys = {};
    ModifierFlags modifiers = ModifierFlags::None;
    std::string textInput;
    
    // Touch state
    std::vector<TouchPoint> activeTouches;
    std::vector<TouchGesture> activeGestures;
    
    // VR state
    std::array<HandPose, 2> handPoses;
    std::array<bool, 2> handTracking = {};
    std::vector<VRGesture> activeVRGestures;
    
    void reset() {
        mouseButtons.fill(false);
        mousePosition = Math::Vector2f::zero();
        mouseDelta = Math::Vector2f::zero();
        mouseWheelDelta = 0.0f;
        
        keys.fill(false);
        modifiers = ModifierFlags::None;
        textInput.clear();
        
        activeTouches.clear();
        activeGestures.clear();
        
        handTracking.fill(false);
        activeVRGestures.clear();
    }
};

}
}