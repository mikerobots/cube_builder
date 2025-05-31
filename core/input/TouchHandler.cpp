#include "TouchHandler.h"
#include "../../foundation/math/MathUtils.h"

namespace VoxelEditor {
namespace Input {

TouchHandler::TouchHandler(Events::EventDispatcher* eventDispatcher)
    : InputHandler(eventDispatcher)
    , m_tapTimeout(0.3f)
    , m_doubleTapTimeout(0.5f)
    , m_tapThreshold(20.0f)
    , m_pinchThreshold(50.0f)
    , m_panThreshold(10.0f)
    , m_rotationThreshold(0.1f)
    , m_sensitivity(1.0f) {
}

void TouchHandler::processTouchEvent(const TouchEvent& event) {
    if (!isEnabled()) {
        return;
    }
    
    switch (event.type) {
        case TouchEventType::TouchBegin:
            handleTouchBegin(event);
            break;
        case TouchEventType::TouchUpdate:
            handleTouchUpdate(event);
            break;
        case TouchEventType::TouchEnd:
            handleTouchEnd(event);
            break;
        case TouchEventType::TouchCancel:
            handleTouchCancel(event);
            break;
    }
}

void TouchHandler::update(float deltaTime) {
    if (!isEnabled()) {
        return;
    }
    
    // Update gesture recognition
    updateGestureRecognition(deltaTime);
    
    // Clean up old touches
    cleanupOldTouches();
    
    // Reset per-frame gestures
    m_detectedGestures.clear();
}

std::vector<TouchPoint> TouchHandler::getActiveTouches() const {
    return m_activeTouches;
}

TouchPoint TouchHandler::getPrimaryTouch() const {
    if (m_activeTouches.empty()) {
        return TouchPoint();
    }
    return m_activeTouches[0];
}

bool TouchHandler::hasTouches() const {
    return !m_activeTouches.empty();
}

bool TouchHandler::isGestureActive(TouchGesture gesture) const {
    return m_activeGestures.count(gesture) > 0;
}

bool TouchHandler::isGestureDetected(TouchGesture gesture) const {
    return std::find(m_detectedGestures.begin(), m_detectedGestures.end(), gesture) != m_detectedGestures.end();
}

Math::Vector2f TouchHandler::getPanDelta() const {
    if (m_activeTouches.size() >= 1) {
        return m_activeTouches[0].delta;
    }
    return Math::Vector2f::zero();
}

float TouchHandler::getPinchScale() const {
    if (m_activeTouches.size() >= 2) {
        Math::Vector2f pos1 = m_activeTouches[0].position;
        Math::Vector2f pos2 = m_activeTouches[1].position;
        float currentDistance = Math::MathUtils::distance(pos1, pos2);
        
        if (m_pinchStartDistance > 0.0f) {
            return currentDistance / m_pinchStartDistance;
        }
    }
    return 1.0f;
}

float TouchHandler::getRotationAngle() const {
    return m_rotationAngle;
}

Math::Vector2f TouchHandler::getGestureCenter() const {
    if (m_activeTouches.empty()) {
        return Math::Vector2f::zero();
    }
    
    Math::Vector2f center = Math::Vector2f::zero();
    for (const auto& touch : m_activeTouches) {
        center = center + touch.position;
    }
    return center / static_cast<float>(m_activeTouches.size());
}

int TouchHandler::getTouchCount() const {
    return static_cast<int>(m_activeTouches.size());
}

void TouchHandler::handleTouchBegin(const TouchEvent& event) {
    // Add new touches
    for (const auto& point : event.points) {
        if (point.state == TouchState::Pressed) {
            m_activeTouches.push_back(point);
            
            // Record touch start time for gesture recognition
            auto now = std::chrono::high_resolution_clock::now();
            m_touchStartTimes[point.id] = now;
            m_touchStartPositions[point.id] = point.position;
        }
    }
    
    // Initialize gesture recognition for multi-touch
    if (m_activeTouches.size() == 2) {
        initializePinchGesture();
        initializeRotationGesture();
    }
    
    // Dispatch event
    if (m_eventDispatcher) {
        Events::TouchBeginEvent touchBegin(m_activeTouches);
        m_eventDispatcher->dispatch(touchBegin);
    }
}

void TouchHandler::handleTouchUpdate(const TouchEvent& event) {
    // Update existing touches
    for (const auto& newPoint : event.points) {
        for (auto& existingTouch : m_activeTouches) {
            if (existingTouch.id == newPoint.id) {
                existingTouch.position = newPoint.position;
                existingTouch.delta = newPoint.delta;
                existingTouch.pressure = newPoint.pressure;
                break;
            }
        }
    }
    
    // Detect gestures
    detectGestures();
    
    // Dispatch event
    if (m_eventDispatcher) {
        Events::TouchUpdateEvent touchUpdate(m_activeTouches);
        m_eventDispatcher->dispatch(touchUpdate);
    }
}

void TouchHandler::handleTouchEnd(const TouchEvent& event) {
    // Remove ended touches
    for (const auto& point : event.points) {
        if (point.state == TouchState::Released) {
            auto it = std::remove_if(m_activeTouches.begin(), m_activeTouches.end(),
                [point](const TouchPoint& touch) { return touch.id == point.id; });
            m_activeTouches.erase(it, m_activeTouches.end());
            
            // Check for tap gesture
            checkForTap(point);
            
            // Clean up tracking data
            m_touchStartTimes.erase(point.id);
            m_touchStartPositions.erase(point.id);
        }
    }
    
    // Reset multi-touch gestures when we have less than 2 touches
    if (m_activeTouches.size() < 2) {
        m_activeGestures.erase(TouchGesture::Pinch);
        m_activeGestures.erase(TouchGesture::Rotate);
        m_pinchStartDistance = 0.0f;
        m_rotationStartAngle = 0.0f;
    }
    
    // Dispatch event
    if (m_eventDispatcher) {
        Events::TouchEndEvent touchEnd(event.points);
        m_eventDispatcher->dispatch(touchEnd);
    }
}

void TouchHandler::handleTouchCancel(const TouchEvent& event) {
    // Cancel all active touches
    m_activeTouches.clear();
    m_activeGestures.clear();
    m_touchStartTimes.clear();
    m_touchStartPositions.clear();
    m_pinchStartDistance = 0.0f;
    m_rotationStartAngle = 0.0f;
    
    // Dispatch event
    if (m_eventDispatcher) {
        Events::TouchCancelEvent touchCancel;
        m_eventDispatcher->dispatch(touchCancel);
    }
}

void TouchHandler::detectGestures() {
    if (m_activeTouches.size() == 1) {
        detectPanGesture();
    } else if (m_activeTouches.size() == 2) {
        detectPinchGesture();
        detectRotationGesture();
    }
}

void TouchHandler::detectPanGesture() {
    if (m_activeTouches.empty()) {
        return;
    }
    
    const TouchPoint& touch = m_activeTouches[0];
    float distance = Math::MathUtils::length(touch.delta);
    
    if (distance > m_panThreshold) {
        if (m_activeGestures.find(TouchGesture::Pan) == m_activeGestures.end()) {
            m_activeGestures.insert(TouchGesture::Pan);
            m_detectedGestures.push_back(TouchGesture::Pan);
            
            // Dispatch gesture event
            if (m_eventDispatcher) {
                Events::TouchGestureEvent gestureEvent(TouchGesture::Pan, getGestureCenter());
                m_eventDispatcher->dispatch(gestureEvent);
            }
        }
    }
}

void TouchHandler::detectPinchGesture() {
    if (m_activeTouches.size() < 2) {
        return;
    }
    
    Math::Vector2f pos1 = m_activeTouches[0].position;
    Math::Vector2f pos2 = m_activeTouches[1].position;
    float currentDistance = Math::MathUtils::distance(pos1, pos2);
    
    if (m_pinchStartDistance > 0.0f) {
        float distanceChange = std::abs(currentDistance - m_pinchStartDistance);
        
        if (distanceChange > m_pinchThreshold) {
            if (m_activeGestures.find(TouchGesture::Pinch) == m_activeGestures.end()) {
                m_activeGestures.insert(TouchGesture::Pinch);
                m_detectedGestures.push_back(TouchGesture::Pinch);
                
                // Dispatch gesture event
                if (m_eventDispatcher) {
                    Events::TouchGestureEvent gestureEvent(TouchGesture::Pinch, getGestureCenter());
                    m_eventDispatcher->dispatch(gestureEvent);
                }
            }
        }
    }
}

void TouchHandler::detectRotationGesture() {
    if (m_activeTouches.size() < 2) {
        return;
    }
    
    Math::Vector2f pos1 = m_activeTouches[0].position;
    Math::Vector2f pos2 = m_activeTouches[1].position;
    Math::Vector2f center = (pos1 + pos2) * 0.5f;
    
    Math::Vector2f dir1 = pos1 - center;
    Math::Vector2f dir2 = pos2 - center;
    
    float currentAngle = std::atan2(dir2.y - dir1.y, dir2.x - dir1.x);
    
    if (m_rotationStartAngle != 0.0f) {
        float angleDiff = std::abs(currentAngle - m_rotationStartAngle);
        if (angleDiff > Math::MathUtils::PI) {
            angleDiff = 2.0f * Math::MathUtils::PI - angleDiff;
        }
        
        if (angleDiff > m_rotationThreshold) {
            if (m_activeGestures.find(TouchGesture::Rotate) == m_activeGestures.end()) {
                m_activeGestures.insert(TouchGesture::Rotate);
                m_detectedGestures.push_back(TouchGesture::Rotate);
                
                // Dispatch gesture event
                if (m_eventDispatcher) {
                    Events::TouchGestureEvent gestureEvent(TouchGesture::Rotate, getGestureCenter());
                    m_eventDispatcher->dispatch(gestureEvent);
                }
            }
            
            m_rotationAngle = currentAngle - m_rotationStartAngle;
        }
    }
}

void TouchHandler::checkForTap(const TouchPoint& touch) {
    auto it = m_touchStartTimes.find(touch.id);
    if (it == m_touchStartTimes.end()) {
        return;
    }
    
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
    
    // Check if touch was quick enough for a tap
    if (duration <= m_tapTimeout * 1000) {
        Math::Vector2f startPos = m_touchStartPositions[touch.id];
        float distance = Math::MathUtils::distance(touch.position, startPos);
        
        // Check if movement was small enough for a tap
        if (distance <= m_tapThreshold) {
            m_detectedGestures.push_back(TouchGesture::Tap);
            
            // Check for double tap
            checkForDoubleTap(touch);
            
            // Dispatch gesture event
            if (m_eventDispatcher) {
                Events::TouchGestureEvent gestureEvent(TouchGesture::Tap, touch.position);
                m_eventDispatcher->dispatch(gestureEvent);
            }
        }
    }
}

void TouchHandler::checkForDoubleTap(const TouchPoint& touch) {
    auto now = std::chrono::high_resolution_clock::now();
    
    // Check if this is close enough to the last tap
    if (m_lastTapTime != TimePoint{}) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTapTime).count();
        
        if (duration <= m_doubleTapTimeout * 1000) {
            float distance = Math::MathUtils::distance(touch.position, m_lastTapPosition);
            
            if (distance <= m_tapThreshold) {
                m_detectedGestures.push_back(TouchGesture::DoubleTap);
                
                // Dispatch gesture event
                if (m_eventDispatcher) {
                    Events::TouchGestureEvent gestureEvent(TouchGesture::DoubleTap, touch.position);
                    m_eventDispatcher->dispatch(gestureEvent);
                }
                
                // Reset to prevent triple taps
                m_lastTapTime = TimePoint{};
                return;
            }
        }
    }
    
    // Record this tap for potential double tap
    m_lastTapTime = now;
    m_lastTapPosition = touch.position;
}

void TouchHandler::initializePinchGesture() {
    if (m_activeTouches.size() >= 2) {
        Math::Vector2f pos1 = m_activeTouches[0].position;
        Math::Vector2f pos2 = m_activeTouches[1].position;
        m_pinchStartDistance = Math::MathUtils::distance(pos1, pos2);
    }
}

void TouchHandler::initializeRotationGesture() {
    if (m_activeTouches.size() >= 2) {
        Math::Vector2f pos1 = m_activeTouches[0].position;
        Math::Vector2f pos2 = m_activeTouches[1].position;
        Math::Vector2f center = (pos1 + pos2) * 0.5f;
        
        Math::Vector2f dir1 = pos1 - center;
        Math::Vector2f dir2 = pos2 - center;
        
        m_rotationStartAngle = std::atan2(dir2.y - dir1.y, dir2.x - dir1.x);
    }
}

void TouchHandler::updateGestureRecognition(float deltaTime) {
    // Update gesture timing and states
    // This could include gesture smoothing, filtering, etc.
}

void TouchHandler::cleanupOldTouches() {
    // Clean up any tracking data for touches that are no longer active
    // This helps prevent memory leaks from long-running sessions
}

std::string TouchHandler::touchGestureToString(TouchGesture gesture) {
    switch (gesture) {
        case TouchGesture::Tap: return "Tap";
        case TouchGesture::DoubleTap: return "DoubleTap";
        case TouchGesture::LongPress: return "LongPress";
        case TouchGesture::Pan: return "Pan";
        case TouchGesture::Pinch: return "Pinch";
        case TouchGesture::Rotate: return "Rotate";
        case TouchGesture::Swipe: return "Swipe";
        case TouchGesture::ThreeFingerTap: return "ThreeFingerTap";
        case TouchGesture::FourFingerTap: return "FourFingerTap";
        case TouchGesture::TwoFingerPan: return "TwoFingerPan";
        default: return "Unknown";
    }
}

TouchGesture TouchHandler::touchGestureFromString(const std::string& str) {
    if (str == "Tap") return TouchGesture::Tap;
    if (str == "DoubleTap") return TouchGesture::DoubleTap;
    if (str == "LongPress") return TouchGesture::LongPress;
    if (str == "Pan") return TouchGesture::Pan;
    if (str == "Pinch") return TouchGesture::Pinch;
    if (str == "Rotate") return TouchGesture::Rotate;
    if (str == "Swipe") return TouchGesture::Swipe;
    if (str == "ThreeFingerTap") return TouchGesture::ThreeFingerTap;
    if (str == "FourFingerTap") return TouchGesture::FourFingerTap;
    if (str == "TwoFingerPan") return TouchGesture::TwoFingerPan;
    return TouchGesture::Tap; // Default fallback
}

}
}