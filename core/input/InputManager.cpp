#include "InputManager.h"
#include "MouseHandler.h"
#include "KeyboardHandler.h"
#include "TouchHandler.h"
#include "VRInputHandler.h"

namespace VoxelEditor {
namespace Input {

InputManager::InputManager(::VoxelEditor::Events::EventDispatcher* eventDispatcher)
    : m_eventDispatcher(eventDispatcher)
    , m_enabled(true)
    , m_initialized(false)
    , m_cursorMode(CursorMode::Normal)
    , m_rawMouseInput(false) {
}

InputManager::~InputManager() {
    shutdown();
}

bool InputManager::initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Initialize default handlers if none are registered
    if (!m_mouseHandler) {
        initializeDefaultHandlers();
    }
    
    // Setup default input bindings
    setupDefaultBindings();
    
    m_initialized = true;
    return true;
}

void InputManager::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    // Clear all handlers
    m_mouseHandler.reset();
    m_keyboardHandler.reset();
    m_touchHandler.reset();
    m_vrHandler.reset();
    
    // Clear event queue
    clearEventQueue();
    
    m_initialized = false;
}

void InputManager::registerMouseHandler(std::unique_ptr<MouseHandler> handler) {
    m_mouseHandler = std::move(handler);
}

void InputManager::registerKeyboardHandler(std::unique_ptr<KeyboardHandler> handler) {
    m_keyboardHandler = std::move(handler);
}

void InputManager::registerTouchHandler(std::unique_ptr<TouchHandler> handler) {
    m_touchHandler = std::move(handler);
}

void InputManager::registerVRHandler(std::unique_ptr<VRInputHandler> handler) {
    m_vrHandler = std::move(handler);
}

void InputManager::update(float deltaTime) {
    if (!m_enabled || !m_initialized) {
        return;
    }
    
    // Process queued events first
    processQueuedEvents();
    
    // Update all handlers
    if (m_mouseHandler) {
        m_mouseHandler->update(deltaTime);
    }
    if (m_keyboardHandler) {
        m_keyboardHandler->update(deltaTime);
    }
    if (m_touchHandler) {
        m_touchHandler->update(deltaTime);
    }
    if (m_vrHandler) {
        m_vrHandler->update(deltaTime);
    }
    
    // Update input state
    updateInputState();
    
    // Update action system
    updateActionStates();
    
    // Copy current state to previous state for next frame
    m_previousState = m_currentState;
}

void InputManager::processEvents() {
    processQueuedEvents();
}

void InputManager::injectMouseEvent(const MouseEvent& event) {
    if (!m_enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_eventQueueMutex);
    m_eventQueue.emplace_back(event);
}

void InputManager::injectKeyboardEvent(const KeyEvent& event) {
    if (!m_enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_eventQueueMutex);
    m_eventQueue.emplace_back(event);
}

void InputManager::injectTouchEvent(const TouchEvent& event) {
    if (!m_enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_eventQueueMutex);
    m_eventQueue.emplace_back(event);
}

void InputManager::injectVREvent(const VREvent& event) {
    if (!m_enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_eventQueueMutex);
    m_eventQueue.emplace_back(event);
}

void InputManager::setInputMapping(const InputMapping& mapping) {
    m_mapping = mapping;
    
    // Update handler sensitivities
    if (m_mouseHandler) {
        m_mouseHandler->setSensitivity(m_mapping.mouseSensitivity);
    }
    if (m_touchHandler) {
        m_touchHandler->setSensitivity(m_mapping.touchSensitivity);
    }
    if (m_vrHandler) {
        m_vrHandler->setSensitivity(m_mapping.vrSensitivity);
    }
}

void InputManager::saveInputMapping(const std::string& filename) const {
    m_mapping.saveToFile(filename);
}

bool InputManager::loadInputMapping(const std::string& filename) {
    InputMapping newMapping;
    if (newMapping.loadFromFile(filename)) {
        setInputMapping(newMapping);
        return true;
    }
    return false;
}

void InputManager::resetToDefaultMapping() {
    setInputMapping(InputMapping::Default());
}

bool InputManager::isKeyPressed(KeyCode key) const {
    return m_keyboardHandler ? m_keyboardHandler->isKeyPressed(key) : false;
}

bool InputManager::isKeyJustPressed(KeyCode key) const {
    return m_keyboardHandler ? m_keyboardHandler->isKeyJustPressed(key) : false;
}

bool InputManager::isKeyJustReleased(KeyCode key) const {
    return m_keyboardHandler ? m_keyboardHandler->isKeyJustReleased(key) : false;
}

bool InputManager::isMouseButtonPressed(MouseButton button) const {
    return m_mouseHandler ? m_mouseHandler->isButtonPressed(button) : false;
}

bool InputManager::isMouseButtonJustPressed(MouseButton button) const {
    return m_mouseHandler ? m_mouseHandler->isButtonJustPressed(button) : false;
}

bool InputManager::isMouseButtonJustReleased(MouseButton button) const {
    return m_mouseHandler ? m_mouseHandler->isButtonJustReleased(button) : false;
}

Math::Vector2f InputManager::getMousePosition() const {
    return m_mouseHandler ? m_mouseHandler->getPosition() : Math::Vector2f::zero();
}

Math::Vector2f InputManager::getMouseDelta() const {
    return m_mouseHandler ? m_mouseHandler->getDelta() : Math::Vector2f::zero();
}

float InputManager::getMouseWheelDelta() const {
    return m_mouseHandler ? m_mouseHandler->getWheelDelta() : 0.0f;
}

bool InputManager::isShiftPressed() const {
    return m_keyboardHandler ? m_keyboardHandler->isShiftPressed() : false;
}

bool InputManager::isCtrlPressed() const {
    return m_keyboardHandler ? m_keyboardHandler->isCtrlPressed() : false;
}

bool InputManager::isAltPressed() const {
    return m_keyboardHandler ? m_keyboardHandler->isAltPressed() : false;
}

bool InputManager::isSuperPressed() const {
    return m_keyboardHandler ? m_keyboardHandler->isSuperPressed() : false;
}

ModifierFlags InputManager::getCurrentModifiers() const {
    return m_keyboardHandler ? m_keyboardHandler->getCurrentModifiers() : ModifierFlags::None;
}

std::vector<TouchPoint> InputManager::getActiveTouches() const {
    return m_touchHandler ? m_touchHandler->getActiveTouches() : std::vector<TouchPoint>();
}

TouchPoint InputManager::getPrimaryTouch() const {
    return m_touchHandler ? m_touchHandler->getPrimaryTouch() : TouchPoint();
}

bool InputManager::hasTouches() const {
    return m_touchHandler ? m_touchHandler->hasTouches() : false;
}

bool InputManager::isGestureActive(TouchGesture gesture) const {
    return m_touchHandler ? m_touchHandler->isGestureActive(gesture) : false;
}

bool InputManager::isHandTracking(HandType hand) const {
    return m_vrHandler ? m_vrHandler->isHandTracking(hand) : false;
}

HandPose InputManager::getHandPose(HandType hand) const {
    return m_vrHandler ? m_vrHandler->getHandPose(hand) : HandPose();
}

bool InputManager::isVRGestureActive(VRGesture gesture, HandType hand) const {
    return m_vrHandler ? m_vrHandler->isGestureActive(gesture, hand) : false;
}

void InputManager::bindAction(const std::string& actionName, const InputTrigger& trigger) {
    ActionBinding binding(actionName, ActionType::Button);
    binding.triggers.push_back(trigger);
    m_actionBindings[actionName] = binding;
}

void InputManager::bindAction(const std::string& actionName, const std::vector<InputTrigger>& triggers) {
    ActionBinding binding(actionName, ActionType::Button);
    binding.triggers = triggers;
    m_actionBindings[actionName] = binding;
}

void InputManager::unbindAction(const std::string& actionName) {
    m_actionBindings.erase(actionName);
    m_actionStates.erase(actionName);
    m_actionCallbacks.erase(actionName);
}

void InputManager::registerActionCallback(const std::string& actionName, ActionCallback callback) {
    m_actionCallbacks[actionName] = callback;
}

void InputManager::unregisterActionCallback(const std::string& actionName) {
    m_actionCallbacks.erase(actionName);
}

bool InputManager::isActionPressed(const std::string& actionName) const {
    auto it = m_actionStates.find(actionName);
    return (it != m_actionStates.end()) ? it->second.active : false;
}

bool InputManager::isActionJustPressed(const std::string& actionName) const {
    auto it = m_actionStates.find(actionName);
    return (it != m_actionStates.end()) ? it->second.justPressed : false;
}

bool InputManager::isActionJustReleased(const std::string& actionName) const {
    auto it = m_actionStates.find(actionName);
    return (it != m_actionStates.end()) ? it->second.justReleased : false;
}

float InputManager::getActionValue(const std::string& actionName) const {
    auto it = m_actionStates.find(actionName);
    return (it != m_actionStates.end()) ? it->second.value : 0.0f;
}

Math::Vector2f InputManager::getActionVector2(const std::string& actionName) const {
    auto it = m_actionStates.find(actionName);
    return (it != m_actionStates.end()) ? it->second.vector2 : Math::Vector2f::zero();
}

Math::Vector3f InputManager::getActionVector3(const std::string& actionName) const {
    auto it = m_actionStates.find(actionName);
    return (it != m_actionStates.end()) ? it->second.vector3 : Math::Vector3f::Zero();
}

void InputManager::setEnabled(bool enabled) {
    m_enabled = enabled;
    
    // Propagate to handlers
    if (m_mouseHandler) {
        m_mouseHandler->setEnabled(enabled);
    }
    if (m_keyboardHandler) {
        m_keyboardHandler->setEnabled(enabled);
    }
    if (m_touchHandler) {
        m_touchHandler->setEnabled(enabled);
    }
    if (m_vrHandler) {
        m_vrHandler->setEnabled(enabled);
    }
}

void InputManager::setMouseSensitivity(float sensitivity) {
    m_mapping.mouseSensitivity = sensitivity;
    if (m_mouseHandler) {
        m_mouseHandler->setSensitivity(sensitivity);
    }
}

void InputManager::setTouchSensitivity(float sensitivity) {
    m_mapping.touchSensitivity = sensitivity;
    if (m_touchHandler) {
        m_touchHandler->setSensitivity(sensitivity);
    }
}

void InputManager::setVRSensitivity(float sensitivity) {
    m_mapping.vrSensitivity = sensitivity;
    if (m_vrHandler) {
        m_vrHandler->setSensitivity(sensitivity);
    }
}

void InputManager::setCursorMode(CursorMode mode) {
    m_cursorMode = mode;
}

void InputManager::setRawMouseInput(bool enabled) {
    m_rawMouseInput = enabled;
}

void InputManager::setVRComfortSettings(const VRComfortSettings& settings) {
    m_mapping.vrComfortSettings = settings;
}

size_t InputManager::getEventQueueSize() const {
    std::lock_guard<std::mutex> lock(m_eventQueueMutex);
    return m_eventQueue.size();
}

void InputManager::clearEventQueue() {
    std::lock_guard<std::mutex> lock(m_eventQueueMutex);
    m_eventQueue.clear();
}

void InputManager::processQueuedEvents() {
    std::vector<QueuedEvent> events;
    
    // Copy events to local vector to minimize lock time
    {
        std::lock_guard<std::mutex> lock(m_eventQueueMutex);
        events = m_eventQueue;
        m_eventQueue.clear();
    }
    
    // Process events
    for (const auto& event : events) {
        switch (event.type) {
            case QueuedEvent::Mouse:
                processMouseEventInternal(std::get<MouseEvent>(event.data));
                break;
            case QueuedEvent::Keyboard:
                processKeyboardEventInternal(std::get<KeyEvent>(event.data));
                break;
            case QueuedEvent::Touch:
                processTouchEventInternal(std::get<TouchEvent>(event.data));
                break;
            case QueuedEvent::VR:
                processVREventInternal(std::get<VREvent>(event.data));
                break;
        }
    }
}

void InputManager::processMouseEventInternal(const MouseEvent& event) {
    if (m_mouseHandler) {
        m_mouseHandler->processMouseEvent(event);
    }
    
    // Check for action triggers
    checkActionTriggers();
}

void InputManager::processKeyboardEventInternal(const KeyEvent& event) {
    if (m_keyboardHandler) {
        m_keyboardHandler->processKeyboardEvent(event);
    }
    
    // Check for action triggers
    checkActionTriggers();
}

void InputManager::processTouchEventInternal(const TouchEvent& event) {
    if (m_touchHandler) {
        m_touchHandler->processTouchEvent(event);
    }
    
    // Check for action triggers
    checkActionTriggers();
}

void InputManager::processVREventInternal(const VREvent& event) {
    if (m_vrHandler) {
        m_vrHandler->processVREvent(event);
    }
    
    // Check for action triggers
    checkActionTriggers();
}

void InputManager::updateInputState() {
    // Update current state based on handlers
    if (m_mouseHandler) {
        m_currentState.mousePosition = m_mouseHandler->getPosition();
        m_currentState.mouseDelta = m_mouseHandler->getDelta();
        m_currentState.mouseWheelDelta = m_mouseHandler->getWheelDelta();
    }
    
    if (m_keyboardHandler) {
        m_currentState.modifiers = m_keyboardHandler->getCurrentModifiers();
    }
    
    if (m_touchHandler) {
        m_currentState.activeTouches = m_touchHandler->getActiveTouches();
    }
    
    if (m_vrHandler) {
        m_currentState.handPoses[0] = m_vrHandler->getHandPose(HandType::Left);
        m_currentState.handPoses[1] = m_vrHandler->getHandPose(HandType::Right);
    }
}

void InputManager::updateActionStates() {
    for (auto& pair : m_actionStates) {
        pair.second.reset();
    }
}

void InputManager::checkActionTriggers() {
    for (const auto& bindingPair : m_actionBindings) {
        const std::string& actionName = bindingPair.first;
        const ActionBinding& binding = bindingPair.second;
        
        bool triggered = false;
        ActionContext context(binding.type);
        
        // Check each trigger for this action
        for (const auto& trigger : binding.triggers) {
            if (checkInputTrigger(trigger, MouseEvent()) || 
                checkInputTrigger(trigger, KeyEvent()) ||
                checkInputTrigger(trigger, TouchGesture::Tap) ||
                checkInputTrigger(trigger, VRGesture::Point)) {
                triggered = true;
                break;
            }
        }
        
        if (triggered) {
            triggerAction(actionName, context);
        }
    }
}

void InputManager::triggerAction(const std::string& actionName, const ActionContext& context) {
    // Update action state
    auto& state = m_actionStates[actionName];
    if (!state.active) {
        state.justPressed = true;
    }
    state.active = true;
    state.value = context.value;
    state.vector2 = context.vector2;
    state.vector3 = context.vector3;
    state.lastTriggered = std::chrono::high_resolution_clock::now();
    
    // Call callback if registered
    auto callbackIt = m_actionCallbacks.find(actionName);
    if (callbackIt != m_actionCallbacks.end()) {
        callbackIt->second(context);
    }
}

bool InputManager::checkInputTrigger(const InputTrigger& trigger, const MouseEvent& event) const {
    if (trigger.device != InputDevice::Mouse) {
        return false;
    }
    
    if (!m_mouseHandler) {
        return false;
    }
    
    bool buttonPressed = m_mouseHandler->isButtonPressed(trigger.input.mouseButton);
    bool modifiersMatch = (m_currentState.modifiers == trigger.requiredModifiers);
    
    return buttonPressed && modifiersMatch;
}

bool InputManager::checkInputTrigger(const InputTrigger& trigger, const KeyEvent& event) const {
    if (trigger.device != InputDevice::Keyboard) {
        return false;
    }
    
    if (!m_keyboardHandler) {
        return false;
    }
    
    bool keyPressed = m_keyboardHandler->isKeyPressed(trigger.input.keyCode);
    bool modifiersMatch = (m_currentState.modifiers == trigger.requiredModifiers);
    
    return keyPressed && modifiersMatch;
}

bool InputManager::checkInputTrigger(const InputTrigger& trigger, TouchGesture gesture) const {
    if (trigger.device != InputDevice::Touch) {
        return false;
    }
    
    if (!m_touchHandler) {
        return false;
    }
    
    return m_touchHandler->isGestureActive(trigger.input.touchGesture);
}

bool InputManager::checkInputTrigger(const InputTrigger& trigger, VRGesture gesture) const {
    if (trigger.device != InputDevice::VRHands) {
        return false;
    }
    
    if (!m_vrHandler) {
        return false;
    }
    
    return m_vrHandler->isGestureActive(trigger.input.vrGesture, HandType::Either);
}

ActionContext InputManager::createActionContext(const ActionBinding& binding, bool pressed, float value) const {
    ActionContext context(binding.type);
    context.pressed = pressed;
    context.value = value;
    context.modifiers = m_currentState.modifiers;
    context.device = InputDevice::Unknown; // Could be determined from trigger type
    
    return context;
}

void InputManager::initializeDefaultHandlers() {
    if (!m_mouseHandler) {
        m_mouseHandler = std::make_unique<MouseHandler>(m_eventDispatcher);
    }
    if (!m_keyboardHandler) {
        m_keyboardHandler = std::make_unique<KeyboardHandler>(m_eventDispatcher);
    }
    if (!m_touchHandler) {
        m_touchHandler = std::make_unique<TouchHandler>(m_eventDispatcher);
    }
    if (!m_vrHandler) {
        m_vrHandler = std::make_unique<VRInputHandler>(m_eventDispatcher);
    }
}

void InputManager::setupDefaultBindings() {
    // Set default input mapping
    if (m_mapping.keys.empty() && m_mapping.mouseButtons.empty()) {
        m_mapping = InputMapping::Default();
    }
}

}
}