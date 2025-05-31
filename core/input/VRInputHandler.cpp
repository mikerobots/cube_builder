#include "VRInputHandler.h"
#include "../../foundation/math/MathUtils.h"
#include <algorithm>

namespace VoxelEditor {
namespace Input {

VRInputHandler::VRInputHandler(Events::EventDispatcher* eventDispatcher)
    : InputHandler(eventDispatcher)
    , m_handTrackingEnabled(true)
    , m_trackingQuality(HandTrackingQuality::None)
    , m_poseFiltering(true)
    , m_filterStrength(0.5f)
    , m_poseHistorySize(5)
    , m_sensitivity(1.0f) {
    
    // Initialize state
    m_state.reset();
    
    // Initialize comfort settings to defaults
    m_comfortSettings = VRComfortSettings::Comfort();
    
    // Initialize default gesture states
    m_state.enabledGestures[VRGesture::Point] = true;
    m_state.enabledGestures[VRGesture::Grab] = true;
    m_state.enabledGestures[VRGesture::Pinch] = true;
    m_state.enabledGestures[VRGesture::Peace] = true;
    m_state.enabledGestures[VRGesture::ThumbsUp] = true;
    m_state.enabledGestures[VRGesture::ThumbsDown] = true;
    m_state.enabledGestures[VRGesture::Fist] = true;
    m_state.enabledGestures[VRGesture::OpenPalm] = true;
    m_state.enabledGestures[VRGesture::TwoHandGrab] = true;
    m_state.enabledGestures[VRGesture::TwoHandScale] = true;
    m_state.enabledGestures[VRGesture::TwoHandRotate] = true;
    
    // Initialize default gesture thresholds
    m_state.gestureThresholds[VRGesture::Point] = 0.8f;
    m_state.gestureThresholds[VRGesture::Grab] = 0.7f;
    m_state.gestureThresholds[VRGesture::Pinch] = 0.85f;
    m_state.gestureThresholds[VRGesture::Peace] = 0.8f;
    m_state.gestureThresholds[VRGesture::ThumbsUp] = 0.75f;
    m_state.gestureThresholds[VRGesture::ThumbsDown] = 0.75f;
    m_state.gestureThresholds[VRGesture::Fist] = 0.7f;
    m_state.gestureThresholds[VRGesture::OpenPalm] = 0.8f;
    m_state.gestureThresholds[VRGesture::TwoHandGrab] = 0.7f;
    m_state.gestureThresholds[VRGesture::TwoHandScale] = 0.75f;
    m_state.gestureThresholds[VRGesture::TwoHandRotate] = 0.75f;
}

VRInputHandler::~VRInputHandler() = default;

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
        case VREventType::GestureCompleted:
            handleGestureCompleted(event);
            break;
    }
}

void VRInputHandler::update(float deltaTime) {
    (void)deltaTime; // Suppress unused parameter warning
    if (!isEnabled()) {
        return;
    }
    
    // Update gesture recognition
    recognizeGestures();
    
    // Update gesture states
    updateGestureStates();
}

bool VRInputHandler::isHandTracking(HandType hand) const {
    if (!m_handTrackingEnabled) {
        return false;
    }
    
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return false;
    }
    
    return m_state.handTracking[index] && m_state.handConfidence[index] > 0.7f;
}

HandPose VRInputHandler::getHandPose(HandType hand) const {
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return HandPose();
    }
    
    return m_state.currentPoses[index];
}

bool VRInputHandler::isGestureActive(VRGesture gesture, HandType hand) const {
    for (const auto& activeGesture : m_state.activeGestures) {
        if (activeGesture.first == gesture) {
            if (hand == HandType::Either || activeGesture.second == hand) {
                return true;
            }
        }
    }
    return false;
}

float VRInputHandler::getGestureConfidence(VRGesture gesture, HandType hand) const {
    auto key = std::make_pair(gesture, hand);
    auto it = m_state.gestureConfidence.find(key);
    if (it != m_state.gestureConfidence.end()) {
        return it->second;
    }
    
    // Check for Either hand type
    if (hand == HandType::Either) {
        float leftConf = getGestureConfidence(gesture, HandType::Left);
        float rightConf = getGestureConfidence(gesture, HandType::Right);
        return std::max(leftConf, rightConf);
    }
    
    return 0.0f;
}

Math::Vector3f VRInputHandler::getGesturePosition(VRGesture gesture, HandType hand) const {
    if (isGestureActive(gesture, hand)) {
        if (hand == HandType::Either) {
            // Return position of the hand that has the gesture active
            for (const auto& activeGesture : m_state.activeGestures) {
                if (activeGesture.first == gesture) {
                    return getHandPosition(activeGesture.second);
                }
            }
        } else {
            return getHandPosition(hand);
        }
    }
    return Math::Vector3f::Zero();
}

std::vector<VRGesture> VRInputHandler::getActiveGestures(HandType hand) const {
    std::vector<VRGesture> gestures;
    for (const auto& activeGesture : m_state.activeGestures) {
        if (hand == HandType::Either || activeGesture.second == hand) {
            gestures.push_back(activeGesture.first);
        }
    }
    return gestures;
}

Math::Vector3f VRInputHandler::getHandPosition(HandType hand) const {
    return getHandPose(hand).position;
}

Math::Quaternion VRInputHandler::getHandOrientation(HandType hand) const {
    return getHandPose(hand).orientation;
}

Math::Ray VRInputHandler::getHandRay(HandType hand) const {
    HandPose pose = getHandPose(hand);
    return createHandRay(pose);
}

Math::Vector3f VRInputHandler::getPointingDirection(HandType hand) const {
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return Math::Vector3f::Zero();
    }
    return m_state.pointingDirection[index];
}

bool VRInputHandler::isPointing(HandType hand) const {
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return false;
    }
    return m_state.pointing[index];
}

float VRInputHandler::getHandConfidence(HandType hand) const {
    return getHandPose(hand).confidence;
}

void VRInputHandler::enableGesture(VRGesture gesture, bool enabled) {
    m_state.enabledGestures[gesture] = enabled;
}

bool VRInputHandler::isGestureEnabled(VRGesture gesture) const {
    auto it = m_state.enabledGestures.find(gesture);
    return (it != m_state.enabledGestures.end()) ? it->second : false;
}

void VRInputHandler::setGestureThreshold(VRGesture gesture, float threshold) {
    m_state.gestureThresholds[gesture] = threshold;
}

float VRInputHandler::getGestureThreshold(VRGesture gesture) const {
    auto it = m_state.gestureThresholds.find(gesture);
    return (it != m_state.gestureThresholds.end()) ? it->second : 0.8f;
}

bool VRInputHandler::isValidVRGesture(VRGesture gesture) {
    switch (gesture) {
        case VRGesture::Point:
        case VRGesture::Grab:
        case VRGesture::Pinch:
        case VRGesture::Peace:
        case VRGesture::ThumbsUp:
        case VRGesture::ThumbsDown:
        case VRGesture::Fist:
        case VRGesture::OpenPalm:
        case VRGesture::TwoHandGrab:
        case VRGesture::TwoHandScale:
        case VRGesture::TwoHandRotate:
            return true;
        default:
            return false;
    }
}

bool VRInputHandler::isValidHandType(HandType hand) {
    switch (hand) {
        case HandType::Left:
        case HandType::Right:
        case HandType::Either:
            return true;
        default:
            return false;
    }
}

void VRInputHandler::handleHandUpdate(const VREvent& event) {
    size_t index = getHandIndex(event.hand);
    if (index >= 2) {
        return;
    }
    
    // Update hand pose
    updateHandPose(event.hand, event.pose);
    
    // Mark hand as tracking
    m_state.handTracking[index] = true;
    m_state.handConfidence[index] = event.pose.confidence;
    
    // Update pointing direction if pointing
    if (isFingerExtended(event.pose, 1) && !isFingerExtended(event.pose, 2)) {
        m_state.pointing[index] = true;
        m_state.pointingDirection[index] = calculatePointingDirection(event.pose);
    } else {
        m_state.pointing[index] = false;
    }
    
    // Dispatch hand tracking event
    if (m_eventDispatcher) {
        VoxelEditor::Events::HandTrackingEvent handEvent(event.hand, m_state.currentPoses[index]);
        m_eventDispatcher->dispatch(handEvent);
    }
}

void VRInputHandler::handleGestureDetected(const VREvent& event) {
    if (event.gestures.empty()) {
        return;
    }
    
    for (const auto& gesture : event.gestures) {
        // Add to active gestures
        m_state.activeGestures.push_back(std::make_pair(gesture, event.hand));
        
        // Update gesture confidence
        auto key = std::make_pair(gesture, event.hand);
        m_state.gestureConfidence[key] = event.pose.confidence;
        
        // Dispatch gesture event
        if (m_eventDispatcher) {
            VoxelEditor::Events::VRGestureEvent gestureEvent(gesture, event.hand, event.pose.position, 
                                              event.pose.confidence, true, false);
            m_eventDispatcher->dispatch(gestureEvent);
        }
    }
}

void VRInputHandler::handleGestureCompleted(const VREvent& event) {
    // Remove from active gestures
    m_state.activeGestures.erase(
        std::remove_if(m_state.activeGestures.begin(), m_state.activeGestures.end(),
            [&event](const std::pair<VRGesture, HandType>& gesture) {
                return std::find(event.gestures.begin(), event.gestures.end(), gesture.first) != event.gestures.end() 
                       && gesture.second == event.hand;
            }),
        m_state.activeGestures.end()
    );
    
    // Clear gesture confidence
    for (const auto& gesture : event.gestures) {
        auto key = std::make_pair(gesture, event.hand);
        m_state.gestureConfidence.erase(key);
    }
    
    // Dispatch gesture ended events
    if (m_eventDispatcher) {
        for (const auto& gesture : event.gestures) {
            VoxelEditor::Events::VRGestureEvent gestureEvent(gesture, event.hand, event.pose.position, 
                                              0.0f, false, true);
            m_eventDispatcher->dispatch(gestureEvent);
        }
    }
}

void VRInputHandler::handleHandLost(HandType hand) {
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return;
    }
    
    // Reset hand state
    m_state.currentPoses[index] = HandPose();
    m_state.handTracking[index] = false;
    m_state.handConfidence[index] = 0.0f;
    m_state.pointing[index] = false;
    m_state.pinching[index] = false;
    m_state.grabbing[index] = false;
    m_state.pointingDirection[index] = Math::Vector3f::Zero();
    
    // Clear active gestures for this hand
    m_state.activeGestures.erase(
        std::remove_if(m_state.activeGestures.begin(), m_state.activeGestures.end(),
            [hand](const std::pair<VRGesture, HandType>& gesture) {
                return gesture.second == hand;
            }),
        m_state.activeGestures.end()
    );
    
    // Clear gesture confidence for this hand
    for (auto it = m_state.gestureConfidence.begin(); it != m_state.gestureConfidence.end(); ) {
        if (it->first.second == hand) {
            it = m_state.gestureConfidence.erase(it);
        } else {
            ++it;
        }
    }
    
    // Clear pose history
    m_state.poseHistory[index].clear();
    
    // Dispatch hand lost event
    if (m_eventDispatcher) {
        VoxelEditor::Events::HandLostEvent handLostEvent(hand);
        m_eventDispatcher->dispatch(handLostEvent);
    }
}

void VRInputHandler::updateHandPose(HandType hand, const HandPose& pose) {
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return;
    }
    
    // Apply sensitivity to position
    HandPose adjustedPose = pose;
    adjustedPose.position = adjustedPose.position * m_sensitivity;
    
    if (m_poseFiltering) {
        // Add to history and get filtered pose
        addPoseToHistory(hand, adjustedPose);
        m_state.currentPoses[index] = getFilteredPose(hand);
    } else {
        m_state.currentPoses[index] = adjustedPose;
    }
}

HandPose VRInputHandler::filterHandPose(HandType hand, const HandPose& pose) const {
    // Simple filtering implementation - can be enhanced
    (void)hand; // Suppress unused parameter warning
    return pose;
}

void VRInputHandler::addPoseToHistory(HandType hand, const HandPose& pose) {
    size_t index = getHandIndex(hand);
    if (index >= 2) {
        return;
    }
    
    m_state.poseHistory[index].push_back(pose);
    
    // Keep history size limited
    while (m_state.poseHistory[index].size() > m_poseHistorySize) {
        m_state.poseHistory[index].pop_front();
    }
}

HandPose VRInputHandler::getFilteredPose(HandType hand) const {
    size_t index = getHandIndex(hand);
    if (index >= 2 || m_state.poseHistory[index].empty()) {
        return HandPose();
    }
    
    // Simple weighted average filter
    HandPose filtered;
    float totalWeight = 0.0f;
    
    size_t historySize = m_state.poseHistory[index].size();
    for (size_t i = 0; i < historySize; ++i) {
        float weight = (i + 1) * m_filterStrength / historySize;
        const HandPose& historicalPose = m_state.poseHistory[index][i];
        
        filtered.position = filtered.position + historicalPose.position * weight;
        // Note: Quaternion interpolation would need proper slerp implementation
        filtered.confidence += historicalPose.confidence * weight;
        
        totalWeight += weight;
    }
    
    if (totalWeight > 0.0f) {
        filtered.position = filtered.position / totalWeight;
        filtered.confidence /= totalWeight;
    }
    
    // Copy latest orientation and other data
    filtered.orientation = m_state.poseHistory[index].back().orientation;
    filtered.hand = hand;
    filtered.fingers = m_state.poseHistory[index].back().fingers;
    
    return filtered;
}

bool VRInputHandler::isFingerExtended(const HandPose& pose, int fingerIndex) const {
    if (fingerIndex < 0 || fingerIndex >= 5) {
        return false;
    }
    return pose.fingers[fingerIndex].extended;
}

bool VRInputHandler::isFingerBent(const HandPose& pose, int fingerIndex) const {
    if (fingerIndex < 0 || fingerIndex >= 5) {
        return false;
    }
    return pose.fingers[fingerIndex].bend > 0.5f;
}

float VRInputHandler::calculateFingerBend(const FingerPose& finger) const {
    return finger.bend;
}

Math::Vector3f VRInputHandler::calculatePointingDirection(const HandPose& pose) const {
    // Calculate pointing direction based on hand orientation and index finger
    Math::Vector3f forward(0, 0, -1);
    return pose.orientation.rotate(forward);
}

float VRInputHandler::calculatePinchDistance(const HandPose& pose) const {
    if (pose.fingers[0].joints.empty() || pose.fingers[1].joints.empty()) {
        return 1.0f; // Max distance
    }
    
    Math::Vector3f thumbTip = pose.fingers[0].joints.back();
    Math::Vector3f indexTip = pose.fingers[1].joints.back();
    return (thumbTip - indexTip).length();
}

bool VRInputHandler::isHandClosed(const HandPose& pose) const {
    // Check if all fingers are bent
    for (int i = 0; i < 5; ++i) {
        if (!isFingerBent(pose, i)) {
            return false;
        }
    }
    return true;
}

bool VRInputHandler::isHandOpen(const HandPose& pose) const {
    // Check if all fingers are extended
    for (int i = 0; i < 5; ++i) {
        if (!isFingerExtended(pose, i)) {
            return false;
        }
    }
    return true;
}

size_t VRInputHandler::getHandIndex(HandType hand) const {
    switch (hand) {
        case HandType::Left: return 0;
        case HandType::Right: return 1;
        default: return 2; // Invalid
    }
}

bool VRInputHandler::isValidHandIndex(size_t index) const {
    return index < 2;
}

float VRInputHandler::calculateGestureConfidence(VRGesture gesture, HandType hand) const {
    auto key = std::make_pair(gesture, hand);
    auto it = m_state.gestureConfidence.find(key);
    return (it != m_state.gestureConfidence.end()) ? it->second : 0.0f;
}

bool VRInputHandler::meetsGestureThreshold(VRGesture gesture, float confidence) const {
    return confidence >= getGestureThreshold(gesture);
}

Math::Ray VRInputHandler::createHandRay(const HandPose& pose) const {
    Math::Vector3f direction = calculatePointingDirection(pose);
    return Math::Ray(pose.position, direction);
}

Math::Vector3f VRInputHandler::getFingerTipPosition(const HandPose& pose, int fingerIndex) const {
    if (fingerIndex < 0 || fingerIndex >= 5 || pose.fingers[fingerIndex].joints.empty()) {
        return pose.position;
    }
    return pose.fingers[fingerIndex].joints.back();
}


std::string VRInputHandler::vrGestureToString(VRGesture gesture) {
    switch (gesture) {
        case VRGesture::Point: return "Point";
        case VRGesture::Grab: return "Grab";
        case VRGesture::Pinch: return "Pinch";
        case VRGesture::Peace: return "Peace";
        case VRGesture::ThumbsUp: return "ThumbsUp";
        case VRGesture::ThumbsDown: return "ThumbsDown";
        case VRGesture::Fist: return "Fist";
        case VRGesture::OpenPalm: return "OpenPalm";
        default: return "Unknown";
    }
}

VRGesture VRInputHandler::vrGestureFromString(const std::string& str) {
    if (str == "Point") return VRGesture::Point;
    if (str == "Grab") return VRGesture::Grab;
    if (str == "Pinch") return VRGesture::Pinch;
    if (str == "Peace") return VRGesture::Peace;
    if (str == "ThumbsUp") return VRGesture::ThumbsUp;
    if (str == "Fist") return VRGesture::Fist;
    if (str == "OpenPalm") return VRGesture::OpenPalm;
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

void VRInputHandler::recognizeGestures() {
    // Implementation for gesture recognition
    for (int i = 0; i < 2; ++i) {
        HandType hand = (i == 0) ? HandType::Left : HandType::Right;
        if (isHandTracking(hand)) {
            checkPointingGesture(hand);
            checkPinchGesture(hand);
            checkGrabGesture(hand);
            checkThumbsGestures(hand);
            checkFistGesture(hand);
            checkOpenPalmGesture(hand);
        }
    }
    checkTwoHandGestures();
}

void VRInputHandler::updateGestureStates() {
    // Clean up old gesture states
    // This is a simplified implementation
}

void VRInputHandler::checkPointingGesture(HandType hand) {
    HandPose pose = getHandPose(hand);
    if (isFingerExtended(pose, 1) && !isFingerExtended(pose, 2) && 
        !isFingerExtended(pose, 3) && !isFingerExtended(pose, 4)) {
        if (!isGestureActive(VRGesture::Point, hand)) {
            startGesture(VRGesture::Point, hand);
        }
    } else if (isGestureActive(VRGesture::Point, hand)) {
        endGesture(VRGesture::Point, hand);
    }
}

void VRInputHandler::checkPinchGesture(HandType hand) {
    HandPose pose = getHandPose(hand);
    float distance = calculatePinchDistance(pose);
    if (distance < 0.02f) { // 2cm threshold
        if (!isGestureActive(VRGesture::Pinch, hand)) {
            startGesture(VRGesture::Pinch, hand);
        }
    } else if (isGestureActive(VRGesture::Pinch, hand)) {
        endGesture(VRGesture::Pinch, hand);
    }
}

void VRInputHandler::checkGrabGesture(HandType hand) {
    HandPose pose = getHandPose(hand);
    if (isHandClosed(pose)) {
        if (!isGestureActive(VRGesture::Grab, hand)) {
            startGesture(VRGesture::Grab, hand);
        }
    } else if (isGestureActive(VRGesture::Grab, hand)) {
        endGesture(VRGesture::Grab, hand);
    }
}

void VRInputHandler::checkThumbsGestures(HandType hand) {
    HandPose pose = getHandPose(hand);
    // Thumbs up: only thumb extended
    if (isFingerExtended(pose, 0) && !isFingerExtended(pose, 1) && 
        !isFingerExtended(pose, 2) && !isFingerExtended(pose, 3) && 
        !isFingerExtended(pose, 4)) {
        if (!isGestureActive(VRGesture::ThumbsUp, hand)) {
            startGesture(VRGesture::ThumbsUp, hand);
        }
    } else if (isGestureActive(VRGesture::ThumbsUp, hand)) {
        endGesture(VRGesture::ThumbsUp, hand);
    }
}

void VRInputHandler::checkFistGesture(HandType hand) {
    HandPose pose = getHandPose(hand);
    if (isHandClosed(pose)) {
        if (!isGestureActive(VRGesture::Fist, hand)) {
            startGesture(VRGesture::Fist, hand);
        }
    } else if (isGestureActive(VRGesture::Fist, hand)) {
        endGesture(VRGesture::Fist, hand);
    }
}

void VRInputHandler::checkOpenPalmGesture(HandType hand) {
    HandPose pose = getHandPose(hand);
    if (isHandOpen(pose)) {
        if (!isGestureActive(VRGesture::OpenPalm, hand)) {
            startGesture(VRGesture::OpenPalm, hand);
        }
    } else if (isGestureActive(VRGesture::OpenPalm, hand)) {
        endGesture(VRGesture::OpenPalm, hand);
    }
}

void VRInputHandler::checkTwoHandGestures() {
    if (isHandTracking(HandType::Left) && isHandTracking(HandType::Right)) {
        // Check for two-hand gestures
        // This is a simplified implementation
    }
}

void VRInputHandler::startGesture(VRGesture gesture, HandType hand) {
    if (!isGestureEnabled(gesture)) {
        return;
    }
    
    m_state.activeGestures.push_back(std::make_pair(gesture, hand));
    auto key = std::make_pair(gesture, hand);
    m_state.gestureConfidence[key] = 1.0f;
    
    // Dispatch gesture started event
    if (m_eventDispatcher) {
        HandPose pose = getHandPose(hand);
        VoxelEditor::Events::VRGestureEvent gestureEvent(gesture, hand, pose.position, 1.0f, true, false);
        m_eventDispatcher->dispatch(gestureEvent);
    }
}

void VRInputHandler::updateGesture(VRGesture gesture, HandType hand) {
    // Update gesture confidence and state
    auto key = std::make_pair(gesture, hand);
    auto it = m_state.gestureConfidence.find(key);
    if (it != m_state.gestureConfidence.end()) {
        // Update confidence based on current pose
        HandPose pose = getHandPose(hand);
        it->second = pose.confidence;
    }
}

void VRInputHandler::endGesture(VRGesture gesture, HandType hand) {
    // Remove from active gestures
    m_state.activeGestures.erase(
        std::remove_if(m_state.activeGestures.begin(), m_state.activeGestures.end(),
            [gesture, hand](const std::pair<VRGesture, HandType>& g) {
                return g.first == gesture && g.second == hand;
            }),
        m_state.activeGestures.end()
    );
    
    // Remove confidence
    auto key = std::make_pair(gesture, hand);
    m_state.gestureConfidence.erase(key);
    
    // Dispatch gesture ended event
    if (m_eventDispatcher) {
        HandPose pose = getHandPose(hand);
        VoxelEditor::Events::VRGestureEvent gestureEvent(gesture, hand, pose.position, 0.0f, false, true);
        m_eventDispatcher->dispatch(gestureEvent);
    }
}

bool VRInputHandler::isGestureInProgress(VRGesture gesture, HandType hand) const {
    return isGestureActive(gesture, hand);
}

void VRInputHandler::dispatchVREvent(const VREvent& event) {
    processVREvent(event);
}

void VRInputHandler::dispatchHandPoseEvent(HandType hand, const HandPose& pose) {
    if (m_eventDispatcher) {
        VoxelEditor::Events::VRHandPoseEvent poseEvent(hand, pose, true);
        m_eventDispatcher->dispatch(poseEvent);
    }
}

void VRInputHandler::dispatchGestureEvent(VRGesture gesture, HandType hand, bool started, bool ended) {
    if (m_eventDispatcher) {
        HandPose pose = getHandPose(hand);
        float confidence = calculateGestureConfidence(gesture, hand);
        VoxelEditor::Events::VRGestureEvent gestureEvent(gesture, hand, pose.position, confidence, started, ended);
        m_eventDispatcher->dispatch(gestureEvent);
    }
}

void VRInputHandler::dispatchPointingEvent(HandType hand, const Math::Vector3f& direction, bool started) {
    if (m_eventDispatcher) {
        HandPose pose = getHandPose(hand);
        VoxelEditor::Events::VRPointingEvent pointingEvent(hand, direction, pose.position, started);
        m_eventDispatcher->dispatch(pointingEvent);
    }
}

void VRInputHandler::dispatchPinchEvent(HandType hand, float distance, bool started) {
    if (m_eventDispatcher) {
        HandPose pose = getHandPose(hand);
        VoxelEditor::Events::VRPinchEvent pinchEvent(hand, distance, pose.position, started);
        m_eventDispatcher->dispatch(pinchEvent);
    }
}

void VRInputHandler::dispatchGrabEvent(HandType hand, bool started) {
    if (m_eventDispatcher) {
        HandPose pose = getHandPose(hand);
        VoxelEditor::Events::VRGrabEvent grabEvent(hand, pose.position, started);
        m_eventDispatcher->dispatch(grabEvent);
    }
}

}
}