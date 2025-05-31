#include "VRInputHandler.h"
#include "../../foundation/math/MathUtils.h"

namespace VoxelEditor {
namespace Input {

VRInputHandler::VRInputHandler(Events::EventDispatcher* eventDispatcher)
    : InputHandler(eventDispatcher)
    , m_gestureThreshold(0.1f)
    , m_confidence_threshold(0.7f)
    , m_handTrackingEnabled(true)
    , m_gestureRecognitionEnabled(true)
    , m_spatialSensitivity(1.0f) {
    
    // Initialize hand poses
    for (auto& pose : m_handPoses) {
        pose = HandPose();
    }
}

void VRInputHandler::processVREvent(const VREvent& event) {
    if (!isEnabled()) {
        return;
    }
    
    switch (event.type) {
        case VREventType::HandUpdate:
            handleHandUpdate(event);
            break;
        case VREventType::GestureDetected:
            handleGestureDetected(event);
            break;
        case VREventType::HandLost:
            handleHandLost(event);
            break;
        case VREventType::SystemGesture:
            handleSystemGesture(event);
            break;
    }
}

void VRInputHandler::update(float deltaTime) {
    if (!isEnabled()) {
        return;
    }
    
    // Update gesture recognition
    if (m_gestureRecognitionEnabled) {
        recognizeGestures(deltaTime);
    }
    
    // Update hand smoothing
    smoothHandMovement(deltaTime);
    
    // Clean up old gesture data
    cleanupGestureHistory();
}

bool VRInputHandler::isHandTracking(HandType hand) const {
    if (!m_handTrackingEnabled) {
        return false;
    }
    
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return false;
    }
    
    return m_handPoses[index].confidence > m_confidence_threshold;
}

HandPose VRInputHandler::getHandPose(HandType hand) const {
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return HandPose();
    }
    
    return m_handPoses[index];
}

bool VRInputHandler::isVRGestureActive(VRGesture gesture, HandType hand) const {
    if (hand == HandType::Either) {
        return m_activeGestures[0].count(gesture) > 0 || m_activeGestures[1].count(gesture) > 0;
    }
    
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return false;
    }
    
    return m_activeGestures[index].count(gesture) > 0;
}

bool VRInputHandler::isVRGestureDetected(VRGesture gesture, HandType hand) const {
    if (hand == HandType::Either) {
        for (const auto& detectedGesture : m_detectedGestures) {
            if (detectedGesture.gesture == gesture) {
                return true;
            }
        }
        return false;
    }
    
    for (const auto& detectedGesture : m_detectedGestures) {
        if (detectedGesture.gesture == gesture && detectedGesture.hand == hand) {
            return true;
        }
    }
    return false;
}

Math::Vector3f VRInputHandler::getHandPosition(HandType hand) const {
    return getHandPose(hand).position;
}

Math::Quaternion VRInputHandler::getHandOrientation(HandType hand) const {
    return getHandPose(hand).orientation;
}

Math::Vector3f VRInputHandler::getHandVelocity(HandType hand) const {
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return Math::Vector3f::Zero();
    }
    
    return m_handVelocities[index];
}

float VRInputHandler::getHandConfidence(HandType hand) const {
    return getHandPose(hand).confidence;
}

std::vector<Math::Vector3f> VRInputHandler::getFingerPositions(HandType hand, int fingerIndex) const {
    HandPose pose = getHandPose(hand);
    if (fingerIndex < 0 || fingerIndex >= 5) {
        return {};
    }
    
    return pose.fingers[fingerIndex].joints;
}

bool VRInputHandler::isFingerExtended(HandType hand, int fingerIndex) const {
    HandPose pose = getHandPose(hand);
    if (fingerIndex < 0 || fingerIndex >= 5) {
        return false;
    }
    
    return pose.fingers[fingerIndex].extended;
}

float VRInputHandler::getFingerBend(HandType hand, int fingerIndex) const {
    HandPose pose = getHandPose(hand);
    if (fingerIndex < 0 || fingerIndex >= 5) {
        return 0.0f;
    }
    
    return pose.fingers[fingerIndex].bend;
}

void VRInputHandler::bindVRGesture(VRGesture gesture, const std::string& action) {
    m_gestureBindings[gesture] = action;
}

void VRInputHandler::unbindVRGesture(VRGesture gesture) {
    m_gestureBindings.erase(gesture);
}

std::string VRInputHandler::getVRGestureAction(VRGesture gesture) const {
    auto it = m_gestureBindings.find(gesture);
    return (it != m_gestureBindings.end()) ? it->second : "";
}

void VRInputHandler::clearVRGestureBindings() {
    m_gestureBindings.clear();
}

void VRInputHandler::handleHandUpdate(const VREvent& event) {
    size_t index = getHandIndex(event.hand);
    if (index >= 2) {
        return;
    }
    
    HandPose previousPose = m_handPoses[index];
    m_handPoses[index] = event.pose;
    
    // Calculate velocity
    if (previousPose.confidence > 0.0f) {
        Math::Vector3f deltaPos = event.pose.position - previousPose.position;
        // Assuming fixed time step for simplicity - in real implementation would use actual delta time
        m_handVelocities[index] = deltaPos * 60.0f; // Approximate 60 FPS
    }
    
    // Apply spatial sensitivity
    m_handPoses[index].position = m_handPoses[index].position * m_spatialSensitivity;
    
    // Dispatch hand tracking event
    if (m_eventDispatcher) {
        Events::HandTrackingEvent handEvent(event.hand, m_handPoses[index]);
        m_eventDispatcher->dispatch(handEvent);
    }
}

void VRInputHandler::handleGestureDetected(const VREvent& event) {
    // Add gesture to detected list
    DetectedGesture detected;
    detected.gesture = event.gestures.empty() ? VRGesture::Point : event.gestures[0];
    detected.hand = event.hand;
    detected.timestamp = std::chrono::high_resolution_clock::now();
    detected.confidence = event.pose.confidence;
    
    m_detectedGestures.push_back(detected);
    
    // Check for bound actions
    std::string action = getVRGestureAction(detected.gesture);
    if (!action.empty()) {
        dispatchVRGestureActionEvent(action, detected.gesture, event.hand);
    }
    
    // Dispatch gesture event
    if (m_eventDispatcher) {
        Events::VRGestureEvent gestureEvent(detected.gesture, event.hand, event.pose);
        m_eventDispatcher->dispatch(gestureEvent);
    }
}

void VRInputHandler::handleHandLost(const VREvent& event) {
    size_t index = getHandIndex(event.hand);
    if (index >= 2) {
        return;
    }
    
    // Reset hand pose
    m_handPoses[index] = HandPose();
    m_handVelocities[index] = Math::Vector3f::Zero();
    
    // Clear active gestures for this hand
    m_activeGestures[index].clear();
    
    // Dispatch hand lost event
    if (m_eventDispatcher) {
        Events::HandLostEvent handLostEvent(event.hand);
        m_eventDispatcher->dispatch(handLostEvent);
    }
}

void VRInputHandler::handleSystemGesture(const VREvent& event) {
    // Handle system-level VR gestures (like menu activation)
    if (m_eventDispatcher) {
        Events::VRSystemGestureEvent systemEvent(event.gestures, event.hand);
        m_eventDispatcher->dispatch(systemEvent);
    }
}

void VRInputHandler::recognizeGestures(float deltaTime) {
    for (int handIndex = 0; handIndex < 2; ++handIndex) {
        HandType hand = (handIndex == 0) ? HandType::Left : HandType::Right;
        
        if (!isHandTracking(hand)) {
            continue;
        }
        
        recognizeHandGestures(hand);
    }
}

void VRInputHandler::recognizeHandGestures(HandType hand) {
    HandPose pose = getHandPose(hand);
    size_t index = getHandIndex(hand);
    
    if (index >= 2) {
        return;
    }
    
    // Clear previous frame's active gestures
    std::set<VRGesture> previousGestures = m_activeGestures[index];
    m_activeGestures[index].clear();
    
    // Recognize pointing gesture
    if (recognizePointGesture(pose)) {
        m_activeGestures[index].insert(VRGesture::Point);
        if (previousGestures.find(VRGesture::Point) == previousGestures.end()) {
            triggerGestureDetection(VRGesture::Point, hand, pose);
        }
    }
    
    // Recognize grab gesture
    if (recognizeGrabGesture(pose)) {
        m_activeGestures[index].insert(VRGesture::Grab);
        if (previousGestures.find(VRGesture::Grab) == previousGestures.end()) {
            triggerGestureDetection(VRGesture::Grab, hand, pose);
        }
    }
    
    // Recognize pinch gesture
    if (recognizePinchGesture(pose)) {
        m_activeGestures[index].insert(VRGesture::Pinch);
        if (previousGestures.find(VRGesture::Pinch) == previousGestures.end()) {
            triggerGestureDetection(VRGesture::Pinch, hand, pose);
        }
    }
    
    // Recognize peace sign
    if (recognizePeaceGesture(pose)) {
        m_activeGestures[index].insert(VRGesture::Peace);
        if (previousGestures.find(VRGesture::Peace) == previousGestures.end()) {
            triggerGestureDetection(VRGesture::Peace, hand, pose);
        }
    }
    
    // Recognize thumbs up
    if (recognizeThumbsUpGesture(pose)) {
        m_activeGestures[index].insert(VRGesture::Thumbs_Up);
        if (previousGestures.find(VRGesture::Thumbs_Up) == previousGestures.end()) {
            triggerGestureDetection(VRGesture::Thumbs_Up, hand, pose);
        }
    }
    
    // Recognize wave gesture
    if (recognizeWaveGesture(pose)) {
        m_activeGestures[index].insert(VRGesture::Wave);
        if (previousGestures.find(VRGesture::Wave) == previousGestures.end()) {
            triggerGestureDetection(VRGesture::Wave, hand, pose);
        }
    }
}

bool VRInputHandler::recognizePointGesture(const HandPose& pose) const {
    // Index finger extended, others curled
    return pose.fingers[1].extended && 
           !pose.fingers[0].extended && // thumb
           !pose.fingers[2].extended && // middle
           !pose.fingers[3].extended && // ring
           !pose.fingers[4].extended;   // pinky
}

bool VRInputHandler::recognizeGrabGesture(const HandPose& pose) const {
    // All fingers curled (high bend values)
    for (int i = 0; i < 5; ++i) {
        if (pose.fingers[i].bend < 0.7f) {
            return false;
        }
    }
    return true;
}

bool VRInputHandler::recognizePinchGesture(const HandPose& pose) const {
    // Thumb and index finger close together
    if (pose.fingers[0].joints.empty() || pose.fingers[1].joints.empty()) {
        return false;
    }
    
    Math::Vector3f thumbTip = pose.fingers[0].joints.back();
    Math::Vector3f indexTip = pose.fingers[1].joints.back();
    float distance = Math::MathUtils::distance(thumbTip, indexTip);
    
    return distance < 0.02f; // 2cm threshold
}

bool VRInputHandler::recognizePeaceGesture(const HandPose& pose) const {
    // Index and middle fingers extended, others curled
    return pose.fingers[1].extended && // index
           pose.fingers[2].extended && // middle
           !pose.fingers[0].extended && // thumb
           !pose.fingers[3].extended && // ring
           !pose.fingers[4].extended;   // pinky
}

bool VRInputHandler::recognizeThumbsUpGesture(const HandPose& pose) const {
    // Only thumb extended
    return pose.fingers[0].extended && // thumb
           !pose.fingers[1].extended && // index
           !pose.fingers[2].extended && // middle
           !pose.fingers[3].extended && // ring
           !pose.fingers[4].extended;   // pinky
}

bool VRInputHandler::recognizeWaveGesture(const HandPose& pose) const {
    // All fingers extended (open hand)
    for (int i = 0; i < 5; ++i) {
        if (!pose.fingers[i].extended) {
            return false;
        }
    }
    return true;
}

void VRInputHandler::triggerGestureDetection(VRGesture gesture, HandType hand, const HandPose& pose) {
    DetectedGesture detected;
    detected.gesture = gesture;
    detected.hand = hand;
    detected.timestamp = std::chrono::high_resolution_clock::now();
    detected.confidence = pose.confidence;
    
    m_detectedGestures.push_back(detected);
    
    // Check for bound actions
    std::string action = getVRGestureAction(gesture);
    if (!action.empty()) {
        dispatchVRGestureActionEvent(action, gesture, hand);
    }
}

void VRInputHandler::smoothHandMovement(float deltaTime) {
    // Apply smoothing to hand movements to reduce jitter
    for (int i = 0; i < 2; ++i) {
        if (m_handPoses[i].confidence > 0.0f) {
            // Simple exponential smoothing
            float smoothingFactor = 0.8f;
            if (m_previousHandPoses[i].confidence > 0.0f) {
                m_handPoses[i].position = m_previousHandPoses[i].position * (1.0f - smoothingFactor) + 
                                         m_handPoses[i].position * smoothingFactor;
            }
            m_previousHandPoses[i] = m_handPoses[i];
        }
    }
}

void VRInputHandler::cleanupGestureHistory() {
    auto now = std::chrono::high_resolution_clock::now();
    
    // Remove old detected gestures (older than 1 second)
    m_detectedGestures.erase(
        std::remove_if(m_detectedGestures.begin(), m_detectedGestures.end(),
            [now](const DetectedGesture& gesture) {
                auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - gesture.timestamp).count();
                return age > 1000;
            }),
        m_detectedGestures.end()
    );
}

size_t VRInputHandler::getHandIndex(HandType hand) const {
    switch (hand) {
        case HandType::Left: return 0;
        case HandType::Right: return 1;
        default: return 2; // Invalid
    }
}

void VRInputHandler::dispatchVRGestureActionEvent(const std::string& action, VRGesture gesture, HandType hand) {
    if (m_eventDispatcher) {
        Events::VRGestureActionEvent actionEvent(action, gesture, hand);
        m_eventDispatcher->dispatch(actionEvent);
    }
}

std::string VRInputHandler::vrGestureToString(VRGesture gesture) {
    switch (gesture) {
        case VRGesture::Point: return "Point";
        case VRGesture::Grab: return "Grab";
        case VRGesture::Pinch: return "Pinch";
        case VRGesture::Peace: return "Peace";
        case VRGesture::Thumbs_Up: return "Thumbs_Up";
        case VRGesture::Wave: return "Wave";
        case VRGesture::Fist: return "Fist";
        case VRGesture::Open_Palm: return "Open_Palm";
        default: return "Unknown";
    }
}

VRGesture VRInputHandler::vrGestureFromString(const std::string& str) {
    if (str == "Point") return VRGesture::Point;
    if (str == "Grab") return VRGesture::Grab;
    if (str == "Pinch") return VRGesture::Pinch;
    if (str == "Peace") return VRGesture::Peace;
    if (str == "Thumbs_Up") return VRGesture::Thumbs_Up;
    if (str == "Wave") return VRGesture::Wave;
    if (str == "Fist") return VRGesture::Fist;
    if (str == "Open_Palm") return VRGesture::Open_Palm;
    return VRGesture::Point; // Default fallback
}

std::string VRInputHandler::handTypeToString(HandType hand) {
    switch (hand) {
        case HandType::Left: return "Left";
        case HandType::Right: return "Right";
        case HandType::Either: return "Either";
        default: return "Unknown";
    }
}

HandType VRInputHandler::handTypeFromString(const std::string& str) {
    if (str == "Left") return HandType::Left;
    if (str == "Right") return HandType::Right;
    if (str == "Either") return HandType::Either;
    return HandType::Left; // Default fallback
}

}
}