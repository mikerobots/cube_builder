#include "MouseHandler.h"
#include "../../foundation/math/MathUtils.h"
#include <algorithm>

namespace VoxelEditor {
namespace Input {

MouseHandler::MouseHandler(Events::EventDispatcher* eventDispatcher)
    : InputHandler(eventDispatcher)
    , m_clickTimeout(0.3f)
    , m_doubleClickTimeout(0.5f)
    , m_dragThreshold(5.0f)
    , m_sensitivity(1.0f)
    , m_positionFilter(false)
    , m_minimumMovement(1.0f) {
    
    // Initialize click times to zero
    for (auto& time : m_state.lastClickTime) {
        time = TimePoint{};
    }
}

void MouseHandler::processMouseEvent(const MouseEvent& event) {
    if (!isEnabled()) {
        return;
    }
    
    switch (event.type) {
        case MouseEventType::ButtonPress:
            handleButtonPress(event);
            break;
        case MouseEventType::ButtonRelease:
            handleButtonRelease(event);
            break;
        case MouseEventType::Move:
            handleMouseMove(event);
            break;
        case MouseEventType::Wheel:
            handleWheel(event);
            break;
        case MouseEventType::Enter:
            handleMouseEnter(event);
            break;
        case MouseEventType::Leave:
            handleMouseLeave(event);
            break;
    }
}

void MouseHandler::update(float deltaTime) {
    if (!isEnabled()) {
        return;
    }
    
    // Store previous state for just pressed/released detection
    m_state.previousButtonsPressed = m_state.buttonsPressed;
    m_state.previousPosition = m_state.position;
    
    // Update just pressed/released states
    updateJustPressedReleased();
    
    // Reset per-frame values
    m_state.wheelDelta = 0.0f;
    
    // Reset just pressed/released flags
    m_state.buttonsJustPressed.fill(false);
    m_state.buttonsJustReleased.fill(false);
}

bool MouseHandler::isButtonPressed(MouseButton button) const {
    size_t index = getButtonIndex(button);
    return isValidButtonIndex(index) ? m_state.buttonsPressed[index] : false;
}

bool MouseHandler::isButtonJustPressed(MouseButton button) const {
    size_t index = getButtonIndex(button);
    return isValidButtonIndex(index) ? m_state.buttonsJustPressed[index] : false;
}

bool MouseHandler::isButtonJustReleased(MouseButton button) const {
    size_t index = getButtonIndex(button);
    return isValidButtonIndex(index) ? m_state.buttonsJustReleased[index] : false;
}

bool MouseHandler::isDoubleClick(MouseButton button) const {
    size_t index = getButtonIndex(button);
    if (!isValidButtonIndex(index)) {
        return false;
    }
    
    return m_state.clickCount[index] >= 2;
}

bool MouseHandler::isDragging(MouseButton button) const {
    size_t index = getButtonIndex(button);
    return isValidButtonIndex(index) ? m_state.dragging[index] : false;
}

int MouseHandler::getClickCount(MouseButton button) const {
    size_t index = getButtonIndex(button);
    return isValidButtonIndex(index) ? m_state.clickCount[index] : 0;
}

Math::Vector2f MouseHandler::getClickPosition(MouseButton button) const {
    size_t index = getButtonIndex(button);
    return isValidButtonIndex(index) ? m_state.clickPosition[index] : Math::Vector2f::zero();
}

Math::Ray MouseHandler::createRayFromMouse(const Math::Vector2f& mousePos, 
                                          const Camera::Camera& camera, 
                                          const Math::Vector2i& viewportSize) const {
    // Convert mouse coordinates to normalized device coordinates (-1 to 1)
    float x = (2.0f * mousePos.x) / viewportSize.x - 1.0f;
    float y = 1.0f - (2.0f * mousePos.y) / viewportSize.y;
    
    // Create ray in world space
    Math::Vector3f rayStart = camera.getPosition();
    Math::Vector3f rayDirection = camera.screenToWorldDirection(Math::Vector2f(x, y));
    
    return Math::Ray(rayStart, rayDirection);
}

void MouseHandler::handleButtonPress(const MouseEvent& event) {
    size_t index = getButtonIndex(event.button);
    if (!isValidButtonIndex(index)) {
        return;
    }
    
    m_state.buttonsPressed[index] = true;
    m_state.buttonsJustPressed[index] = true;
    
    // Update click state
    updateClickState(event.button, event.position);
    
    // Dispatch click event
    dispatchMouseClickEvent(event.button, event.position, m_state.clickCount[index]);
}

void MouseHandler::handleButtonRelease(const MouseEvent& event) {
    size_t index = getButtonIndex(event.button);
    if (!isValidButtonIndex(index)) {
        return;
    }
    
    m_state.buttonsPressed[index] = false;
    m_state.buttonsJustReleased[index] = true;
    m_state.dragging[index] = false;
}

void MouseHandler::handleMouseMove(const MouseEvent& event) {
    Math::Vector2f newPosition = event.position;
    
    // Apply position filtering if enabled
    if (m_positionFilter) {
        newPosition = filterPosition(newPosition);
    }
    
    // Apply sensitivity
    Math::Vector2f delta = event.delta * m_sensitivity;
    
    m_state.position = newPosition;
    m_state.delta = delta;
    
    // Update drag state for all pressed buttons
    for (int i = 0; i < 8; ++i) {
        if (m_state.buttonsPressed[i]) {
            MouseButton button = static_cast<MouseButton>(i);
            updateDragState(button, newPosition);
        }
    }
    
    // Dispatch move event
    dispatchMouseMoveEvent(newPosition, delta);
}

void MouseHandler::handleWheel(const MouseEvent& event) {
    m_state.wheelDelta = event.wheelDelta * m_sensitivity;
    
    // Dispatch wheel event
    dispatchMouseWheelEvent(m_state.wheelDelta, event.position);
}

void MouseHandler::handleMouseEnter(const MouseEvent& event) {
    // Mouse entered the window/viewport
    m_state.position = event.position;
}

void MouseHandler::handleMouseLeave(const MouseEvent& event) {
    // Mouse left the window/viewport
    // Could reset some state here if needed
}

bool MouseHandler::isDoubleClickInternal(MouseButton button, const Math::Vector2f& position) const {
    size_t index = getButtonIndex(button);
    if (!isValidButtonIndex(index)) {
        return false;
    }
    
    auto now = std::chrono::high_resolution_clock::now();
    auto timeSinceLastClick = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_state.lastClickTime[index]).count();
    
    if (timeSinceLastClick > m_doubleClickTimeout * 1000) {
        return false;
    }
    
    // Check if position is close enough to previous click
    Math::Vector2f lastPos = m_state.clickPosition[index];
    float distance = Math::MathUtils::distance(position, lastPos);
    
    return distance <= m_dragThreshold;
}

bool MouseHandler::isDragInternal(MouseButton button, const Math::Vector2f& position) const {
    size_t index = getButtonIndex(button);
    if (!isValidButtonIndex(index) || !m_state.buttonsPressed[index]) {
        return false;
    }
    
    Math::Vector2f startPos = m_state.clickPosition[index];
    float distance = Math::MathUtils::distance(position, startPos);
    
    return distance > m_dragThreshold;
}

void MouseHandler::updateClickState(MouseButton button, const Math::Vector2f& position) {
    size_t index = getButtonIndex(button);
    if (!isValidButtonIndex(index)) {
        return;
    }
    
    auto now = std::chrono::high_resolution_clock::now();
    
    if (isDoubleClickInternal(button, position)) {
        m_state.clickCount[index]++;
    } else {
        m_state.clickCount[index] = 1;
    }
    
    m_state.lastClickTime[index] = now;
    m_state.clickPosition[index] = position;
}

void MouseHandler::updateDragState(MouseButton button, const Math::Vector2f& position) {
    size_t index = getButtonIndex(button);
    if (!isValidButtonIndex(index)) {
        return;
    }
    
    if (!m_state.dragging[index] && isDragInternal(button, position)) {
        m_state.dragging[index] = true;
        
        // Dispatch drag start event
        dispatchMouseDragEvent(button, m_state.clickPosition[index], position);
    } else if (m_state.dragging[index]) {
        // Continue dragging - dispatch drag update event
        dispatchMouseDragEvent(button, m_state.clickPosition[index], position);
    }
}

void MouseHandler::updateJustPressedReleased() {
    for (size_t i = 0; i < m_state.buttonsPressed.size(); ++i) {
        bool currentPressed = m_state.buttonsPressed[i];
        bool previousPressed = m_state.previousButtonsPressed[i];
        
        m_state.buttonsJustPressed[i] = currentPressed && !previousPressed;
        m_state.buttonsJustReleased[i] = !currentPressed && previousPressed;
    }
}

Math::Vector2f MouseHandler::filterPosition(const Math::Vector2f& newPosition) const {
    Math::Vector2f delta = newPosition - m_state.position;
    
    if (shouldFilterMovement(delta)) {
        // Apply simple smoothing filter
        return m_state.position + delta * 0.5f;
    }
    
    return newPosition;
}

bool MouseHandler::shouldFilterMovement(const Math::Vector2f& delta) const {
    float distance = Math::MathUtils::length(delta);
    return distance < m_minimumMovement;
}

size_t MouseHandler::getButtonIndex(MouseButton button) {
    return static_cast<size_t>(button);
}

bool MouseHandler::isValidButtonIndex(size_t index) {
    return index < 8 && index != static_cast<size_t>(MouseButton::None);
}

void MouseHandler::dispatchMouseClickEvent(MouseButton button, const Math::Vector2f& position, int clickCount) {
    if (m_eventDispatcher) {
        Events::MouseClickEvent clickEvent(button, position, clickCount);
        m_eventDispatcher->dispatch(clickEvent);
    }
}

void MouseHandler::dispatchMouseDragEvent(MouseButton button, const Math::Vector2f& startPos, const Math::Vector2f& currentPos) {
    if (m_eventDispatcher) {
        Events::MouseDragEvent dragEvent(button, startPos, currentPos);
        m_eventDispatcher->dispatch(dragEvent);
    }
}

void MouseHandler::dispatchMouseMoveEvent(const Math::Vector2f& position, const Math::Vector2f& delta) {
    if (m_eventDispatcher) {
        Events::MouseMoveEvent moveEvent(position, delta);
        m_eventDispatcher->dispatch(moveEvent);
    }
}

void MouseHandler::dispatchMouseWheelEvent(float delta, const Math::Vector2f& position) {
    if (m_eventDispatcher) {
        Events::MouseWheelEvent wheelEvent(delta, position);
        m_eventDispatcher->dispatch(wheelEvent);
    }
}

std::string MouseHandler::mouseButtonToString(MouseButton button) {
    switch (button) {
        case MouseButton::Left: return "Left";
        case MouseButton::Right: return "Right";
        case MouseButton::Middle: return "Middle";
        case MouseButton::Button4: return "Button4";
        case MouseButton::Button5: return "Button5";
        case MouseButton::Button6: return "Button6";
        case MouseButton::Button7: return "Button7";
        case MouseButton::Button8: return "Button8";
        case MouseButton::None: return "None";
        default: return "Unknown";
    }
}

MouseButton MouseHandler::mouseButtonFromString(const std::string& str) {
    if (str == "Left") return MouseButton::Left;
    if (str == "Right") return MouseButton::Right;
    if (str == "Middle") return MouseButton::Middle;
    if (str == "Button4") return MouseButton::Button4;
    if (str == "Button5") return MouseButton::Button5;
    if (str == "Button6") return MouseButton::Button6;
    if (str == "Button7") return MouseButton::Button7;
    if (str == "Button8") return MouseButton::Button8;
    return MouseButton::None;
}

bool MouseHandler::isValidMouseButton(MouseButton button) {
    return button != MouseButton::None && 
           static_cast<int>(button) >= 0 && 
           static_cast<int>(button) <= 7;
}

}
}