#pragma once

#include <array>
#include <deque>
#include <memory>
#include <vector>

#include "InputHandler.h"
#include "InputTypes.h"
#include "../../foundation/math/Ray.h"
#include "../../foundation/events/EventBase.h"

namespace VoxelEditor {

// Forward declarations
namespace VoxelData {
    class VoxelGrid;
    struct Face;
}

namespace Input {

// Forward declaration for VR gesture recognizer
class VRGestureRecognizer;

class VRInputHandler : public InputHandler {
public:
    explicit VRInputHandler(VoxelEditor::Events::EventDispatcher* eventDispatcher = nullptr);
    ~VRInputHandler();
    
    // InputHandler interface
    void processVREvent(const VREvent& event) override;
    void update(float deltaTime) override;
    
    // Hand tracking
    HandPose getHandPose(HandType hand) const;
    bool isHandTracking(HandType hand) const;
    Math::Vector3f getHandPosition(HandType hand) const;
    Math::Quaternion getHandOrientation(HandType hand) const;
    float getHandConfidence(HandType hand) const;
    
    // Gesture recognition
    bool isGestureActive(VRGesture gesture, HandType hand = HandType::Either) const;
    float getGestureConfidence(VRGesture gesture, HandType hand = HandType::Either) const;
    Math::Vector3f getGesturePosition(VRGesture gesture, HandType hand = HandType::Either) const;
    std::vector<VRGesture> getActiveGestures(HandType hand = HandType::Either) const;
    
    // Ray casting
    Math::Ray getHandRay(HandType hand) const;
    Math::Vector3f getPointingDirection(HandType hand) const;
    bool isPointing(HandType hand) const;
    
    // Gesture configuration
    void enableGesture(VRGesture gesture, bool enabled);
    bool isGestureEnabled(VRGesture gesture) const;
    void setGestureThreshold(VRGesture gesture, float threshold);
    float getGestureThreshold(VRGesture gesture) const;
    
    // Hand tracking configuration
    void setHandTrackingEnabled(bool enabled) { m_handTrackingEnabled = enabled; }
    bool isHandTrackingEnabled() const { return m_handTrackingEnabled; }
    void setTrackingQuality(HandTrackingQuality quality) { m_trackingQuality = quality; }
    HandTrackingQuality getTrackingQuality() const { return m_trackingQuality; }
    
    // Comfort settings
    void setComfortSettings(const VRComfortSettings& settings) { m_comfortSettings = settings; }
    const VRComfortSettings& getComfortSettings() const { return m_comfortSettings; }
    
    // Filtering and smoothing
    void setPoseFiltering(bool enabled) { m_poseFiltering = enabled; }
    void setFilterStrength(float strength) { m_filterStrength = strength; }
    void setPoseHistorySize(size_t size) { m_poseHistorySize = size; }
    bool isPoseFilteringEnabled() const { return m_poseFiltering; }
    float getFilterStrength() const { return m_filterStrength; }
    
    // Sensitivity
    void setSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
    float getSensitivity() const { return m_sensitivity; }
    
    // VR utilities
    static std::string vrGestureToString(VRGesture gesture);
    static VRGesture vrGestureFromString(const std::string& str);
    static std::string handTypeToString(HandType hand);
    static HandType handTypeFromString(const std::string& str);
    static bool isValidVRGesture(VRGesture gesture);
    static bool isValidHandType(HandType hand);
    
private:
    struct VRState {
        // Hand poses
        std::array<HandPose, 2> currentPoses;
        std::array<bool, 2> handTracking = {};
        std::array<float, 2> handConfidence = {};
        
        // Gesture state
        std::vector<std::pair<VRGesture, HandType>> activeGestures;
        std::unordered_map<VRGesture, bool> enabledGestures;
        std::unordered_map<VRGesture, float> gestureThresholds;
        std::unordered_map<std::pair<VRGesture, HandType>, float> gestureConfidence;
        
        // Pose history for filtering
        std::array<std::deque<HandPose>, 2> poseHistory;
        
        // Gesture detection state
        std::array<bool, 2> pointing = {};
        std::array<bool, 2> pinching = {};
        std::array<bool, 2> grabbing = {};
        std::array<Math::Vector3f, 2> pointingDirection;
        
        void reset() {
            handTracking.fill(false);
            handConfidence.fill(0.0f);
            activeGestures.clear();
            for (auto& history : poseHistory) {
                history.clear();
            }
            pointing.fill(false);
            pinching.fill(false);
            grabbing.fill(false);
            for (auto& dir : pointingDirection) {
                dir = Math::Vector3f::Zero();
            }
        }
    };
    
    VRState m_state;
    std::unique_ptr<VRGestureRecognizer> m_gestureRecognizer;
    
    // Configuration
    bool m_handTrackingEnabled;
    HandTrackingQuality m_trackingQuality;
    VRComfortSettings m_comfortSettings;
    bool m_poseFiltering;
    float m_filterStrength;
    size_t m_poseHistorySize;
    float m_sensitivity;
    
    // Event processing methods
    void handleHandUpdate(const VREvent& event);
    void handleGestureDetected(const VREvent& event);
    void handleGestureCompleted(const VREvent& event);
    void handleHandLost(HandType hand);
    
    // Hand pose processing
    void updateHandPose(HandType hand, const HandPose& pose);
    HandPose filterHandPose(HandType hand, const HandPose& pose) const;
    void addPoseToHistory(HandType hand, const HandPose& pose);
    HandPose getFilteredPose(HandType hand) const;
    
    // Gesture recognition
    void recognizeGestures();
    void updateGestureStates();
    void checkPointingGesture(HandType hand);
    void checkPinchGesture(HandType hand);
    void checkGrabGesture(HandType hand);
    void checkThumbsGestures(HandType hand);
    void checkFistGesture(HandType hand);
    void checkOpenPalmGesture(HandType hand);
    void checkTwoHandGestures();
    
    // Gesture state management
    void startGesture(VRGesture gesture, HandType hand);
    void updateGesture(VRGesture gesture, HandType hand);
    void endGesture(VRGesture gesture, HandType hand);
    bool isGestureInProgress(VRGesture gesture, HandType hand) const;
    
    // Hand analysis
    bool isFingerExtended(const HandPose& pose, int fingerIndex) const;
    bool isFingerBent(const HandPose& pose, int fingerIndex) const;
    float calculateFingerBend(const FingerPose& finger) const;
    Math::Vector3f calculatePointingDirection(const HandPose& pose) const;
    float calculatePinchDistance(const HandPose& pose) const;
    bool isHandClosed(const HandPose& pose) const;
    bool isHandOpen(const HandPose& pose) const;
    
    // Helper methods
    size_t getHandIndex(HandType hand) const;
    bool isValidHandIndex(size_t index) const;
    float calculateGestureConfidence(VRGesture gesture, HandType hand) const;
    bool meetsGestureThreshold(VRGesture gesture, float confidence) const;
    
    // Ray casting helpers
    Math::Ray createHandRay(const HandPose& pose) const;
    Math::Vector3f getFingerTipPosition(const HandPose& pose, int fingerIndex) const;
    
    // Event dispatching
    void dispatchVREvent(const VREvent& event);
    void dispatchHandPoseEvent(HandType hand, const HandPose& pose);
    void dispatchGestureEvent(VRGesture gesture, HandType hand, bool started, bool ended = false);
    void dispatchPointingEvent(HandType hand, const Math::Vector3f& direction, bool started);
    void dispatchPinchEvent(HandType hand, float distance, bool started);
    void dispatchGrabEvent(HandType hand, bool started);
};

// VR gesture recognizer
class VRGestureRecognizer {
public:
    VRGestureRecognizer();
    ~VRGestureRecognizer() = default;
    
    void updateHandPoses(const std::array<HandPose, 2>& poses);
    std::vector<std::pair<VRGesture, HandType>> recognizeGestures();
    
    // Configuration
    void setGestureThreshold(VRGesture gesture, float threshold);
    float getGestureThreshold(VRGesture gesture) const;
    
private:
    std::array<HandPose, 2> m_currentPoses;
    std::array<HandPose, 2> m_previousPoses;
    std::unordered_map<VRGesture, float> m_gestureThresholds;
    
    float recognizePointing(HandType hand);
    float recognizePinch(HandType hand);
    float recognizeGrab(HandType hand);
    float recognizeThumbsUp(HandType hand);
    float recognizeThumbsDown(HandType hand);
    float recognizeFist(HandType hand);
    float recognizeOpenPalm(HandType hand);
    float recognizePeace(HandType hand);
    float recognizeTwoHandGrab();
    float recognizeTwoHandScale();
    float recognizeTwoHandRotate();
    
    bool isFingerExtended(const HandPose& pose, int fingerIndex) const;
    float calculateFingerBend(const FingerPose& finger) const;
    Math::Vector3f getFingerTip(const HandPose& pose, int fingerIndex) const;
    float calculateDistance(const Math::Vector3f& p1, const Math::Vector3f& p2) const;
    float calculateHandDistance() const;
};

}
}

// VR event types for the event system
namespace VoxelEditor {
namespace Events {
    
    struct VRHandPoseEvent : public Event<VRHandPoseEvent> {
        Input::HandType hand;
        Input::HandPose pose;
        bool tracking;
        
        VRHandPoseEvent(Input::HandType h, const Input::HandPose& p, bool t)
            : hand(h), pose(p), tracking(t) {}
    };
    
    struct VRGestureEvent : public Event<VRGestureEvent> {
        Input::VRGesture gesture;
        Input::HandType hand;
        Math::Vector3f position;
        float confidence;
        bool started;
        bool ended;
        
        VRGestureEvent(Input::VRGesture g, Input::HandType h, const Math::Vector3f& pos, float conf, bool s, bool e)
            : gesture(g), hand(h), position(pos), confidence(conf), started(s), ended(e) {}
    };
    
    struct VRPointingEvent : public Event<VRPointingEvent> {
        Input::HandType hand;
        Math::Vector3f direction;
        Math::Vector3f position;
        bool started;
        
        VRPointingEvent(Input::HandType h, const Math::Vector3f& dir, const Math::Vector3f& pos, bool s)
            : hand(h), direction(dir), position(pos), started(s) {}
    };
    
    struct VRPinchEvent : public Event<VRPinchEvent> {
        Input::HandType hand;
        float distance;
        Math::Vector3f position;
        bool started;
        
        VRPinchEvent(Input::HandType h, float d, const Math::Vector3f& pos, bool s)
            : hand(h), distance(d), position(pos), started(s) {}
    };
    
    struct VRGrabEvent : public Event<VRGrabEvent> {
        Input::HandType hand;
        Math::Vector3f position;
        bool started;
        
        VRGrabEvent(Input::HandType h, const Math::Vector3f& pos, bool s)
            : hand(h), position(pos), started(s) {}
    };
    
    struct HandTrackingEvent : public Event<HandTrackingEvent> {
        Input::HandType hand;
        Input::HandPose pose;
        
        HandTrackingEvent(Input::HandType h, const Input::HandPose& p)
            : hand(h), pose(p) {}
    };
    
    struct HandLostEvent : public Event<HandLostEvent> {
        Input::HandType hand;
        
        HandLostEvent(Input::HandType h)
            : hand(h) {}
    };
    
    struct VRSystemGestureEvent : public Event<VRSystemGestureEvent> {
        std::vector<Input::VRGesture> gestures;
        Input::HandType hand;
        
        VRSystemGestureEvent(const std::vector<Input::VRGesture>& g, Input::HandType h)
            : gestures(g), hand(h) {}
    };
    
    struct VRGestureActionEvent : public Event<VRGestureActionEvent> {
        std::string action;
        Input::VRGesture gesture;
        Input::HandType hand;
        
        VRGestureActionEvent(const std::string& a, Input::VRGesture g, Input::HandType h)
            : action(a), gesture(g), hand(h) {}
    };
    
}
}