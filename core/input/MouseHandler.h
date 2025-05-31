#pragma once

#include "InputHandler.h"
#include "InputTypes.h"
#include "../camera/Camera.h"
#include "../../foundation/math/Ray.h"
#include "../../foundation/math/Vector2i.h"
#include "../../foundation/events/EventBase.h"
#include <array>
#include <chrono>

namespace VoxelEditor {

// Forward declarations
namespace VoxelData {
    class VoxelGrid;
    struct Face;
}

namespace Input {

class MouseHandler : public InputHandler {
public:
    explicit MouseHandler(::VoxelEditor::Events::EventDispatcher* eventDispatcher = nullptr);
    ~MouseHandler() = default;
    
    // InputHandler interface
    void processMouseEvent(const MouseEvent& event) override;
    void update(float deltaTime) override;
    
    // Mouse state queries
    bool isButtonPressed(MouseButton button) const;
    bool isButtonJustPressed(MouseButton button) const;
    bool isButtonJustReleased(MouseButton button) const;
    Math::Vector2f getPosition() const { return m_state.position; }
    Math::Vector2f getDelta() const { return m_state.delta; }
    float getWheelDelta() const { return m_state.wheelDelta; }
    
    // Click detection
    bool isDoubleClick(MouseButton button) const;
    bool isDragging(MouseButton button) const;
    int getClickCount(MouseButton button) const;
    Math::Vector2f getClickPosition(MouseButton button) const;
    
    // Ray casting support
    Math::Ray createRayFromMouse(const Math::Vector2f& mousePos, 
                                const Camera::Camera& camera, 
                                const Math::Vector2i& viewportSize) const;
    
    // Configuration
    void setClickTimeout(float seconds) { m_clickTimeout = seconds; }
    void setDoubleClickTimeout(float seconds) { m_doubleClickTimeout = seconds; }
    void setDragThreshold(float pixels) { m_dragThreshold = pixels; }
    void setSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
    
    float getClickTimeout() const { return m_clickTimeout; }
    float getDoubleClickTimeout() const { return m_doubleClickTimeout; }
    float getDragThreshold() const { return m_dragThreshold; }
    float getSensitivity() const { return m_sensitivity; }
    
    // Event filtering
    void setPositionFilter(bool enabled) { m_positionFilter = enabled; }
    void setMinimumMovement(float pixels) { m_minimumMovement = pixels; }
    bool isPositionFilterEnabled() const { return m_positionFilter; }
    
    // Mouse button utilities
    static std::string mouseButtonToString(MouseButton button);
    static MouseButton mouseButtonFromString(const std::string& str);
    static bool isValidMouseButton(MouseButton button);
    
private:
    struct MouseState {
        // Current state
        std::array<bool, 8> buttonsPressed = {};
        std::array<bool, 8> buttonsJustPressed = {};
        std::array<bool, 8> buttonsJustReleased = {};
        Math::Vector2f position = Math::Vector2f::zero();
        Math::Vector2f delta = Math::Vector2f::zero();
        float wheelDelta = 0.0f;
        
        // Click detection state
        std::array<TimePoint, 8> lastClickTime;
        std::array<Math::Vector2f, 8> clickPosition;
        std::array<int, 8> clickCount = {};
        std::array<bool, 8> dragging = {};
        
        // Previous frame state for just pressed/released detection
        std::array<bool, 8> previousButtonsPressed = {};
        Math::Vector2f previousPosition = Math::Vector2f::zero();
        
        void reset() {
            buttonsPressed.fill(false);
            buttonsJustPressed.fill(false);
            buttonsJustReleased.fill(false);
            position = Math::Vector2f::zero();
            delta = Math::Vector2f::zero();
            wheelDelta = 0.0f;
            clickCount.fill(0);
            dragging.fill(false);
            previousButtonsPressed.fill(false);
            previousPosition = Math::Vector2f::zero();
        }
    };
    
    MouseState m_state;
    
    // Configuration
    float m_clickTimeout;
    float m_doubleClickTimeout;
    float m_dragThreshold;
    float m_sensitivity;
    bool m_positionFilter;
    float m_minimumMovement;
    
    // Event processing methods
    void handleButtonPress(const MouseEvent& event);
    void handleButtonRelease(const MouseEvent& event);
    void handleMouseMove(const MouseEvent& event);
    void handleWheel(const MouseEvent& event);
    void handleMouseEnter(const MouseEvent& event);
    void handleMouseLeave(const MouseEvent& event);
    
    // Helper methods
    bool isDoubleClickInternal(MouseButton button, const Math::Vector2f& position) const;
    bool isDragInternal(MouseButton button, const Math::Vector2f& position) const;
    void updateClickState(MouseButton button, const Math::Vector2f& position);
    void updateDragState(MouseButton button, const Math::Vector2f& position);
    void updateJustPressedReleased();
    
    // Position filtering
    Math::Vector2f filterPosition(const Math::Vector2f& newPosition) const;
    bool shouldFilterMovement(const Math::Vector2f& delta) const;
    
    // Button index conversion
    static size_t getButtonIndex(MouseButton button);
    static bool isValidButtonIndex(size_t index);
    
    // Event dispatching
    void dispatchMouseClickEvent(MouseButton button, const Math::Vector2f& position, int clickCount);
    void dispatchMouseDragEvent(MouseButton button, const Math::Vector2f& startPos, const Math::Vector2f& currentPos);
    void dispatchMouseMoveEvent(const Math::Vector2f& position, const Math::Vector2f& delta);
    void dispatchMouseWheelEvent(float delta, const Math::Vector2f& position);
};

// Mouse event types for the event system
namespace Events {
    
    struct MouseClickEvent : public ::VoxelEditor::Events::EventBase {
        MouseButton button;
        Math::Vector2f position;
        int clickCount;
        ModifierFlags modifiers;
        TimePoint timestamp;
        
        MouseClickEvent(MouseButton btn, const Math::Vector2f& pos, int count, ModifierFlags mods = ModifierFlags::None)
            : button(btn), position(pos), clickCount(count), modifiers(mods)
            , timestamp(std::chrono::high_resolution_clock::now()) {}
            
        const char* getEventType() const override { return "MouseClickEvent"; }
    };
    
    struct MouseDragEvent : public ::VoxelEditor::Events::EventBase {
        MouseButton button;
        Math::Vector2f startPosition;
        Math::Vector2f currentPosition;
        Math::Vector2f delta;
        ModifierFlags modifiers;
        TimePoint timestamp;
        
        MouseDragEvent(MouseButton btn, const Math::Vector2f& start, const Math::Vector2f& current, ModifierFlags mods = ModifierFlags::None)
            : button(btn), startPosition(start), currentPosition(current)
            , delta(current - start), modifiers(mods)
            , timestamp(std::chrono::high_resolution_clock::now()) {}
            
        const char* getEventType() const override { return "MouseDragEvent"; }
    };
    
    struct MouseMoveEvent : public ::VoxelEditor::Events::EventBase {
        Math::Vector2f position;
        Math::Vector2f delta;
        ModifierFlags modifiers;
        TimePoint timestamp;
        
        MouseMoveEvent(const Math::Vector2f& pos, const Math::Vector2f& d, ModifierFlags mods = ModifierFlags::None)
            : position(pos), delta(d), modifiers(mods)
            , timestamp(std::chrono::high_resolution_clock::now()) {}
            
        const char* getEventType() const override { return "MouseMoveEvent"; }
    };
    
    struct MouseWheelEvent : public ::VoxelEditor::Events::EventBase {
        float delta;
        Math::Vector2f position;
        ModifierFlags modifiers;
        TimePoint timestamp;
        
        MouseWheelEvent(float d, const Math::Vector2f& pos, ModifierFlags mods = ModifierFlags::None)
            : delta(d), position(pos), modifiers(mods)
            , timestamp(std::chrono::high_resolution_clock::now()) {}
            
        const char* getEventType() const override { return "MouseWheelEvent"; }
    };
    
}

}
}