#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>

#include "InputHandler.h"
#include "InputTypes.h"
#include "../../foundation/events/EventBase.h"

namespace VoxelEditor {
namespace Input {

// Forward declaration for gesture recognizer
class GestureRecognizer;

class TouchHandler : public InputHandler {
public:
    explicit TouchHandler(VoxelEditor::Events::EventDispatcher* eventDispatcher = nullptr);
    ~TouchHandler();
    
    // InputHandler interface
    void processTouchEvent(const TouchEvent& event) override;
    void update(float deltaTime) override;
    
    // Touch state queries
    std::vector<TouchPoint> getActiveTouches() const;
    TouchPoint getPrimaryTouch() const;
    bool hasTouches() const;
    size_t getTouchCount() const;
    TouchPoint getTouchById(int id) const;
    bool isGestureDetected(TouchGesture gesture) const;
    
    // Gesture data queries
    Math::Vector2f getPanDelta() const;
    float getPinchScale() const;
    float getRotationAngle() const;
    Math::Vector2f getGestureCenter() const;
    
    // Gesture recognition
    void enableGesture(TouchGesture gesture, bool enabled);
    bool isGestureEnabled(TouchGesture gesture) const;
    bool isGestureActive(TouchGesture gesture) const;
    Math::Vector2f getGestureCenter(TouchGesture gesture) const;
    float getGestureScale(TouchGesture gesture) const;
    float getGestureRotation(TouchGesture gesture) const;
    Math::Vector2f getGestureVelocity(TouchGesture gesture) const;
    
    // Gesture configuration
    void setTapTimeout(float seconds) { m_tapTimeout = seconds; }
    void setTapRadius(float pixels) { m_tapRadius = pixels; }
    void setPinchThreshold(float pixels) { m_pinchThreshold = pixels; }
    void setSwipeThreshold(float pixels) { m_swipeThreshold = pixels; }
    void setRotationThreshold(float degrees) { m_rotationThreshold = degrees; }
    void setLongPressTimeout(float seconds) { m_longPressTimeout = seconds; }
    void setSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
    
    float getTapTimeout() const { return m_tapTimeout; }
    float getTapRadius() const { return m_tapRadius; }
    float getPinchThreshold() const { return m_pinchThreshold; }
    float getSwipeThreshold() const { return m_swipeThreshold; }
    float getRotationThreshold() const { return m_rotationThreshold; }
    float getLongPressTimeout() const { return m_longPressTimeout; }
    float getSensitivity() const { return m_sensitivity; }
    
    // Touch utilities
    static std::string touchGestureToString(TouchGesture gesture);
    static TouchGesture touchGestureFromString(const std::string& str);
    static bool isValidTouchGesture(TouchGesture gesture);
    
private:
    struct TouchState {
        // Active touches
        std::unordered_map<int, TouchPoint> activeTouches;
        
        // Gesture state
        std::vector<TouchGesture> activeGestures;
        std::unordered_map<TouchGesture, bool> enabledGestures;
        
        // Gesture data
        Math::Vector2f pinchCenter = Math::Vector2f::zero();
        float pinchScale = 1.0f;
        float initialPinchDistance = 0.0f;
        
        Math::Vector2f panDelta = Math::Vector2f::zero();
        Math::Vector2f panVelocity = Math::Vector2f::zero();
        
        Math::Vector2f rotationCenter = Math::Vector2f::zero();
        float rotationAngle = 0.0f;
        float initialRotationAngle = 0.0f;
        
        // Tap detection
        std::vector<Math::Vector2f> tapPositions;
        std::vector<TimePoint> tapTimes;
        int tapCount = 0;
        
        // Long press detection
        bool longPressStarted = false;
        TimePoint longPressStartTime;
        Math::Vector2f longPressPosition = Math::Vector2f::zero();
        
        // Swipe detection
        Math::Vector2f swipeStart = Math::Vector2f::zero();
        Math::Vector2f swipeEnd = Math::Vector2f::zero();
        TimePoint swipeStartTime;
        bool swipeInProgress = false;
        
        void reset() {
            activeTouches.clear();
            activeGestures.clear();
            pinchCenter = Math::Vector2f::zero();
            pinchScale = 1.0f;
            initialPinchDistance = 0.0f;
            panDelta = Math::Vector2f::zero();
            panVelocity = Math::Vector2f::zero();
            rotationCenter = Math::Vector2f::zero();
            rotationAngle = 0.0f;
            initialRotationAngle = 0.0f;
            tapPositions.clear();
            tapTimes.clear();
            tapCount = 0;
            longPressStarted = false;
            swipeInProgress = false;
        }
    };
    
    TouchState m_state;
    std::unique_ptr<GestureRecognizer> m_gestureRecognizer;
    
    // Configuration
    float m_tapTimeout;
    float m_tapRadius;
    float m_pinchThreshold;
    float m_swipeThreshold;
    float m_rotationThreshold;
    float m_longPressTimeout;
    float m_sensitivity;
    float m_doubleTapTimeout;
    float m_panThreshold;
    
    // Touch tracking
    std::vector<TouchPoint> m_activeTouches;
    std::unordered_map<int, TimePoint> m_touchStartTimes;
    std::unordered_map<int, Math::Vector2f> m_touchStartPositions;
    
    // Gesture state
    std::unordered_set<TouchGesture> m_activeGestures;
    std::vector<TouchGesture> m_detectedGestures;
    std::unordered_map<TouchGesture, bool> m_enabledGestures;
    
    // Pinch gesture data
    float m_pinchStartDistance;
    
    // Rotation gesture data
    float m_rotationStartAngle;
    float m_rotationAngle;
    
    // Tap detection
    TimePoint m_lastTapTime;
    Math::Vector2f m_lastTapPosition;
    
    // Event processing methods
    void handleTouchBegin(const TouchEvent& event);
    void handleTouchUpdate(const TouchEvent& event);
    void handleTouchEnd(const TouchEvent& event);
    void handleTouchCancel(const TouchEvent& event);
    
    // Touch point management
    void updateTouchPoints(const TouchEvent& event);
    void addTouchPoint(const TouchPoint& point);
    void updateTouchPoint(const TouchPoint& point);
    void removeTouchPoint(int id);
    
    // Gesture recognition
    void recognizeGestures();
    void updateGestureStates();
    void checkTapGesture();
    void checkLongPressGesture();
    void checkSwipeGesture();
    void checkPanGesture();
    void checkPinchGesture();
    void checkRotationGesture();
    
    // Gesture state management
    void startGesture(TouchGesture gesture);
    void updateGesture(TouchGesture gesture);
    void endGesture(TouchGesture gesture);
    bool isGestureInProgress(TouchGesture gesture) const;
    
    // Helper methods
    Math::Vector2f calculateCentroid() const;
    float calculateDistance(const Math::Vector2f& p1, const Math::Vector2f& p2) const;
    float calculateAngle(const Math::Vector2f& p1, const Math::Vector2f& p2) const;
    float calculateAverageDistance() const;
    bool isWithinTapRadius(const Math::Vector2f& pos1, const Math::Vector2f& pos2) const;
    bool isWithinTimeLimit(const TimePoint& start, float timeout) const;
    
    // Event dispatching
    void dispatchTouchEvent(const TouchEvent& event);
    void dispatchGestureEvent(TouchGesture gesture, bool started, bool ended = false);
    void dispatchTapEvent(const Math::Vector2f& position, int tapCount);
    void dispatchSwipeEvent(const Math::Vector2f& start, const Math::Vector2f& end, const Math::Vector2f& velocity);
    void dispatchPinchEvent(const Math::Vector2f& center, float scale, float velocity);
    void dispatchPanEvent(const Math::Vector2f& delta, const Math::Vector2f& velocity);
    
    // Gesture detection helpers
    void detectGestures();
    void detectPanGesture();
    void detectPinchGesture();
    void detectRotationGesture();
    void checkForTap(const TouchPoint& touch);
    void checkForDoubleTap(const TouchPoint& touch);
    void initializePinchGesture();
    void initializeRotationGesture();
    void updateGestureRecognition(float deltaTime);
    void cleanupOldTouches();
};

// Simple gesture recognizer
class GestureRecognizer {
public:
    GestureRecognizer();
    ~GestureRecognizer() = default;
    
    void updateTouches(const std::vector<TouchPoint>& touches);
    std::vector<TouchGesture> recognizeGestures();
    
    // Configuration
    void setTapRadius(float radius) { m_tapRadius = radius; }
    void setSwipeThreshold(float threshold) { m_swipeThreshold = threshold; }
    void setPinchThreshold(float threshold) { m_pinchThreshold = threshold; }
    
private:
    std::vector<TouchPoint> m_touches;
    std::vector<TouchPoint> m_previousTouches;
    
    float m_tapRadius;
    float m_swipeThreshold;
    float m_pinchThreshold;
    
    bool recognizeTap();
    bool recognizeSwipe();
    bool recognizePinch();
    bool recognizePan();
    bool recognizeRotation();
};

// Touch event types for the event system
namespace Events {
    
    struct TouchBeginEvent : public VoxelEditor::Events::Event<TouchBeginEvent> {
        std::vector<TouchPoint> touches;
        
        TouchBeginEvent(const std::vector<TouchPoint>& t)
            : touches(t) {}
    };
    
    struct TouchEndEvent : public VoxelEditor::Events::Event<TouchEndEvent> {
        std::vector<TouchPoint> touches;
        
        TouchEndEvent(const std::vector<TouchPoint>& t)
            : touches(t) {}
    };
    
    struct TouchGestureEvent : public VoxelEditor::Events::Event<TouchGestureEvent> {
        TouchGesture gesture;
        Math::Vector2f position;
        Math::Vector2f data; // Scale/rotation/velocity depending on gesture
        bool started;
        bool ended;
        
        TouchGestureEvent(TouchGesture g, const Math::Vector2f& pos, bool s, bool e)
            : gesture(g), position(pos), data(Math::Vector2f::zero()), started(s), ended(e) {}
    };
    
    struct TouchTapEvent : public VoxelEditor::Events::Event<TouchTapEvent> {
        Math::Vector2f position;
        int tapCount;
        
        TouchTapEvent(const Math::Vector2f& pos, int count)
            : position(pos), tapCount(count) {}
    };
    
    struct TouchSwipeEvent : public VoxelEditor::Events::Event<TouchSwipeEvent> {
        Math::Vector2f startPosition;
        Math::Vector2f endPosition;
        Math::Vector2f velocity;
        
        TouchSwipeEvent(const Math::Vector2f& start, const Math::Vector2f& end, const Math::Vector2f& vel)
            : startPosition(start), endPosition(end), velocity(vel) {}
    };
    
    struct TouchPinchEvent : public VoxelEditor::Events::Event<TouchPinchEvent> {
        Math::Vector2f center;
        float scale;
        float velocity;
        
        TouchPinchEvent(const Math::Vector2f& c, float s, float v)
            : center(c), scale(s), velocity(v) {}
    };
    
    struct TouchPanEvent : public VoxelEditor::Events::Event<TouchPanEvent> {
        Math::Vector2f delta;
        Math::Vector2f velocity;
        
        TouchPanEvent(const Math::Vector2f& d, const Math::Vector2f& v)
            : delta(d), velocity(v) {}
    };
    
    struct TouchUpdateEvent : public VoxelEditor::Events::Event<TouchUpdateEvent> {
        std::vector<TouchPoint> touches;
        
        TouchUpdateEvent(const std::vector<TouchPoint>& t)
            : touches(t) {}
    };
    
    struct TouchCancelEvent : public VoxelEditor::Events::Event<TouchCancelEvent> {
        TouchCancelEvent() {}
    };
    
    
}

}
}