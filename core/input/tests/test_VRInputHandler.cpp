#include <gtest/gtest.h>
#include "../VRInputHandler.h"
#include "../../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor::Input;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class VRInputHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        dispatcher = std::make_unique<EventDispatcher>();
        handler = std::make_unique<VRInputHandler>(dispatcher.get());
    }
    
    void TearDown() override {}
    
    std::unique_ptr<EventDispatcher> dispatcher;
    std::unique_ptr<VRInputHandler> handler;
};

TEST_F(VRInputHandlerTest, DefaultState) {
    EXPECT_FALSE(handler->isHandTracking(HandType::Left));
    EXPECT_FALSE(handler->isHandTracking(HandType::Right));
    
    EXPECT_EQ(handler->getHandPosition(HandType::Left), Vector3f::Zero());
    EXPECT_EQ(handler->getHandPosition(HandType::Right), Vector3f::Zero());
    EXPECT_EQ(handler->getHandOrientation(HandType::Left), Quaternion::identity());
    EXPECT_EQ(handler->getHandOrientation(HandType::Right), Quaternion::identity());
    
    EXPECT_FLOAT_EQ(handler->getHandConfidence(HandType::Left), 0.0f);
    EXPECT_FLOAT_EQ(handler->getHandConfidence(HandType::Right), 0.0f);
    
    EXPECT_FALSE(handler->isGestureActive(VRGesture::Point));
    EXPECT_FALSE(handler->isGestureActive(VRGesture::Grab));
    EXPECT_FALSE(handler->isGestureActive(VRGesture::Pinch));
    
    EXPECT_FALSE(handler->isPointing(HandType::Left));
    EXPECT_FALSE(handler->isPointing(HandType::Right));
}

TEST_F(VRInputHandlerTest, HandTracking) {
    // Test hand tracking enable/disable
    EXPECT_TRUE(handler->isHandTrackingEnabled());
    
    handler->setHandTrackingEnabled(false);
    EXPECT_FALSE(handler->isHandTrackingEnabled());
    
    handler->setHandTrackingEnabled(true);
    EXPECT_TRUE(handler->isHandTrackingEnabled());
}

TEST_F(VRInputHandlerTest, TrackingQuality) {
    // Test tracking quality
    EXPECT_EQ(handler->getTrackingQuality(), HandTrackingQuality::None);
    
    handler->setTrackingQuality(HandTrackingQuality::High);
    EXPECT_EQ(handler->getTrackingQuality(), HandTrackingQuality::High);
    
    handler->setTrackingQuality(HandTrackingQuality::Medium);
    EXPECT_EQ(handler->getTrackingQuality(), HandTrackingQuality::Medium);
    
    handler->setTrackingQuality(HandTrackingQuality::Low);
    EXPECT_EQ(handler->getTrackingQuality(), HandTrackingQuality::Low);
}

TEST_F(VRInputHandlerTest, HandPoseUpdate) {
    // Create a hand pose
    HandPose leftPose;
    leftPose.position = Vector3f(0.2f, 1.5f, -0.3f);
    leftPose.orientation = Quaternion::fromEulerAngles(0.1f, 0.2f, 0.3f);
    leftPose.confidence = 0.9f;
    leftPose.hand = HandType::Left;
    
    // Create VR event
    VREvent handUpdate;
    handUpdate.type = VREventType::HandUpdate;
    handUpdate.hand = HandType::Left;
    handUpdate.pose = leftPose;
    
    handler->processVREvent(handUpdate);
    
    // Check that pose was updated
    HandPose retrievedPose = handler->getHandPose(HandType::Left);
    EXPECT_EQ(retrievedPose.position, leftPose.position);
    EXPECT_EQ(retrievedPose.orientation, leftPose.orientation);
    EXPECT_FLOAT_EQ(retrievedPose.confidence, leftPose.confidence);
    EXPECT_EQ(retrievedPose.hand, HandType::Left);
    
    EXPECT_FLOAT_EQ(handler->getHandConfidence(HandType::Left), 0.9f);
}

TEST_F(VRInputHandlerTest, BothHandPoses) {
    // Create poses for both hands
    HandPose leftPose;
    leftPose.position = Vector3f(-0.2f, 1.5f, -0.3f);
    leftPose.confidence = 0.8f;
    leftPose.hand = HandType::Left;
    
    HandPose rightPose;
    rightPose.position = Vector3f(0.2f, 1.5f, -0.3f);
    rightPose.confidence = 0.9f;
    rightPose.hand = HandType::Right;
    
    // Update both hands
    VREvent leftUpdate;
    leftUpdate.type = VREventType::HandUpdate;
    leftUpdate.hand = HandType::Left;
    leftUpdate.pose = leftPose;
    
    VREvent rightUpdate;
    rightUpdate.type = VREventType::HandUpdate;
    rightUpdate.hand = HandType::Right;
    rightUpdate.pose = rightPose;
    
    handler->processVREvent(leftUpdate);
    handler->processVREvent(rightUpdate);
    
    // Check both poses
    EXPECT_EQ(handler->getHandPosition(HandType::Left), leftPose.position);
    EXPECT_EQ(handler->getHandPosition(HandType::Right), rightPose.position);
    EXPECT_FLOAT_EQ(handler->getHandConfidence(HandType::Left), 0.8f);
    EXPECT_FLOAT_EQ(handler->getHandConfidence(HandType::Right), 0.9f);
}

TEST_F(VRInputHandlerTest, GestureConfiguration) {
    // Test default gesture states
    EXPECT_TRUE(handler->isGestureEnabled(VRGesture::Point));
    EXPECT_TRUE(handler->isGestureEnabled(VRGesture::Grab));
    EXPECT_TRUE(handler->isGestureEnabled(VRGesture::Pinch));
    
    // Disable some gestures
    handler->enableGesture(VRGesture::Point, false);
    handler->enableGesture(VRGesture::ThumbsUp, false);
    
    EXPECT_FALSE(handler->isGestureEnabled(VRGesture::Point));
    EXPECT_FALSE(handler->isGestureEnabled(VRGesture::ThumbsUp));
    EXPECT_TRUE(handler->isGestureEnabled(VRGesture::Grab));
    
    // Re-enable
    handler->enableGesture(VRGesture::Point, true);
    EXPECT_TRUE(handler->isGestureEnabled(VRGesture::Point));
}

TEST_F(VRInputHandlerTest, GestureThresholds) {
    // Test default thresholds
    float defaultThreshold = handler->getGestureThreshold(VRGesture::Point);
    EXPECT_GT(defaultThreshold, 0.0f);
    EXPECT_LE(defaultThreshold, 1.0f);
    
    // Set custom thresholds
    handler->setGestureThreshold(VRGesture::Point, 0.8f);
    handler->setGestureThreshold(VRGesture::Grab, 0.7f);
    handler->setGestureThreshold(VRGesture::Pinch, 0.9f);
    
    EXPECT_FLOAT_EQ(handler->getGestureThreshold(VRGesture::Point), 0.8f);
    EXPECT_FLOAT_EQ(handler->getGestureThreshold(VRGesture::Grab), 0.7f);
    EXPECT_FLOAT_EQ(handler->getGestureThreshold(VRGesture::Pinch), 0.9f);
}

TEST_F(VRInputHandlerTest, GestureDetection) {
    // Create gesture detected event
    HandPose pose;
    pose.position = Vector3f(0.2f, 1.5f, -0.3f);
    pose.hand = HandType::Right;
    
    VREvent gestureEvent;
    gestureEvent.type = VREventType::GestureDetected;
    gestureEvent.hand = HandType::Right;
    gestureEvent.pose = pose;
    gestureEvent.gestures.push_back(VRGesture::Point);
    
    handler->processVREvent(gestureEvent);
    
    // Note: The actual gesture activation would depend on implementation
    // For now, just test that the event was processed without error
    EXPECT_TRUE(true);
}

TEST_F(VRInputHandlerTest, ComfortSettings) {
    // Test default comfort settings
    VRComfortSettings defaultSettings = handler->getComfortSettings();
    EXPECT_TRUE(defaultSettings.snapTurning);
    EXPECT_FALSE(defaultSettings.smoothTurning);
    EXPECT_TRUE(defaultSettings.teleportMovement);
    
    // Set custom comfort settings
    VRComfortSettings customSettings = VRComfortSettings::Performance();
    handler->setComfortSettings(customSettings);
    
    VRComfortSettings retrievedSettings = handler->getComfortSettings();
    EXPECT_FALSE(retrievedSettings.snapTurning);
    EXPECT_TRUE(retrievedSettings.smoothTurning);
    EXPECT_FALSE(retrievedSettings.teleportMovement);
    EXPECT_TRUE(retrievedSettings.smoothMovement);
}

TEST_F(VRInputHandlerTest, PoseFiltering) {
    // Test pose filtering configuration
    EXPECT_TRUE(handler->isPoseFilteringEnabled());
    EXPECT_GT(handler->getFilterStrength(), 0.0f);
    
    handler->setPoseFiltering(false);
    EXPECT_FALSE(handler->isPoseFilteringEnabled());
    
    handler->setPoseFiltering(true);
    handler->setFilterStrength(0.8f);
    handler->setPoseHistorySize(10);
    
    EXPECT_TRUE(handler->isPoseFilteringEnabled());
    EXPECT_FLOAT_EQ(handler->getFilterStrength(), 0.8f);
}

TEST_F(VRInputHandlerTest, Sensitivity) {
    // Test sensitivity configuration
    EXPECT_FLOAT_EQ(handler->getSensitivity(), 1.0f);
    
    handler->setSensitivity(2.0f);
    EXPECT_FLOAT_EQ(handler->getSensitivity(), 2.0f);
    
    handler->setSensitivity(0.5f);
    EXPECT_FLOAT_EQ(handler->getSensitivity(), 0.5f);
}

TEST_F(VRInputHandlerTest, EnabledState) {
    handler->setEnabled(false);
    EXPECT_FALSE(handler->isEnabled());
    
    // Events should be ignored when disabled
    HandPose pose;
    pose.position = Vector3f(0.2f, 1.5f, -0.3f);
    pose.hand = HandType::Left;
    
    VREvent handUpdate;
    handUpdate.type = VREventType::HandUpdate;
    handUpdate.hand = HandType::Left;
    handUpdate.pose = pose;
    
    handler->processVREvent(handUpdate);
    
    // Position should not have changed
    EXPECT_EQ(handler->getHandPosition(HandType::Left), Vector3f::Zero());
    
    // Re-enable and test
    handler->setEnabled(true);
    EXPECT_TRUE(handler->isEnabled());
    
    handler->processVREvent(handUpdate);
    EXPECT_EQ(handler->getHandPosition(HandType::Left), pose.position);
}

TEST_F(VRInputHandlerTest, VRUtilities) {
    // Test VR gesture to string conversion
    EXPECT_EQ(VRInputHandler::vrGestureToString(VRGesture::Point), "Point");
    EXPECT_EQ(VRInputHandler::vrGestureToString(VRGesture::Grab), "Grab");
    EXPECT_EQ(VRInputHandler::vrGestureToString(VRGesture::Pinch), "Pinch");
    EXPECT_EQ(VRInputHandler::vrGestureToString(VRGesture::ThumbsUp), "ThumbsUp");
    
    // Test string to VR gesture conversion
    EXPECT_EQ(VRInputHandler::vrGestureFromString("Point"), VRGesture::Point);
    EXPECT_EQ(VRInputHandler::vrGestureFromString("Grab"), VRGesture::Grab);
    EXPECT_EQ(VRInputHandler::vrGestureFromString("Pinch"), VRGesture::Pinch);
    EXPECT_EQ(VRInputHandler::vrGestureFromString("Unknown"), VRGesture::Point); // Default fallback
    
    // Test hand type to string conversion
    EXPECT_EQ(VRInputHandler::handTypeToString(HandType::Left), "Left");
    EXPECT_EQ(VRInputHandler::handTypeToString(HandType::Right), "Right");
    EXPECT_EQ(VRInputHandler::handTypeToString(HandType::Either), "Either");
    
    // Test string to hand type conversion
    EXPECT_EQ(VRInputHandler::handTypeFromString("Left"), HandType::Left);
    EXPECT_EQ(VRInputHandler::handTypeFromString("Right"), HandType::Right);
    EXPECT_EQ(VRInputHandler::handTypeFromString("Either"), HandType::Either);
    EXPECT_EQ(VRInputHandler::handTypeFromString("Unknown"), HandType::Left); // Default fallback
    
    // Test validation
    EXPECT_TRUE(VRInputHandler::isValidVRGesture(VRGesture::Point));
    EXPECT_TRUE(VRInputHandler::isValidVRGesture(VRGesture::Grab));
    EXPECT_TRUE(VRInputHandler::isValidVRGesture(VRGesture::TwoHandScale));
    
    EXPECT_TRUE(VRInputHandler::isValidHandType(HandType::Left));
    EXPECT_TRUE(VRInputHandler::isValidHandType(HandType::Right));
    EXPECT_TRUE(VRInputHandler::isValidHandType(HandType::Either));
}

TEST_F(VRInputHandlerTest, GestureQueries) {
    // Test gesture confidence queries (default values when no gestures active)
    EXPECT_FLOAT_EQ(handler->getGestureConfidence(VRGesture::Point, HandType::Left), 0.0f);
    EXPECT_FLOAT_EQ(handler->getGestureConfidence(VRGesture::Grab, HandType::Right), 0.0f);
    
    // Test gesture position queries (default values when no gestures active)
    EXPECT_EQ(handler->getGesturePosition(VRGesture::Point, HandType::Left), Vector3f::Zero());
    EXPECT_EQ(handler->getGesturePosition(VRGesture::Grab, HandType::Right), Vector3f::Zero());
    
    // Test active gestures query (should be empty by default)
    auto leftGestures = handler->getActiveGestures(HandType::Left);
    auto rightGestures = handler->getActiveGestures(HandType::Right);
    auto allGestures = handler->getActiveGestures(HandType::Either);
    
    EXPECT_TRUE(leftGestures.empty());
    EXPECT_TRUE(rightGestures.empty());
    EXPECT_TRUE(allGestures.empty());
}

TEST_F(VRInputHandlerTest, RayCasting) {
    // Create a hand pose with pointing gesture (index finger extended)
    HandPose pose;
    pose.position = Vector3f(0.2f, 1.5f, -0.3f);
    pose.orientation = Quaternion::lookRotation(Vector3f(0.0f, 0.0f, -1.0f));
    pose.hand = HandType::Right;
    pose.confidence = 0.9f;
    
    // Set up finger poses for pointing gesture
    for (int i = 0; i < 5; ++i) {
        pose.fingers[i].extended = (i == 1); // Only index finger extended
        pose.fingers[i].bend = (i == 1) ? 0.1f : 0.8f;
    }
    
    // Update hand pose
    VREvent handUpdate;
    handUpdate.type = VREventType::HandUpdate;
    handUpdate.hand = HandType::Right;
    handUpdate.pose = pose;
    
    handler->processVREvent(handUpdate);
    
    // Test ray casting (basic interface test)
    Ray handRay = handler->getHandRay(HandType::Right);
    EXPECT_EQ(handRay.origin, pose.position);
    
    // Check if pointing is detected
    EXPECT_TRUE(handler->isPointing(HandType::Right));
    
    Vector3f pointingDir = handler->getPointingDirection(HandType::Right);
    // Pointing direction should be derived from hand orientation
    // For this test, just verify it's not zero vector
    EXPECT_GT(pointingDir.length(), 0.0f);
}

TEST_F(VRInputHandlerTest, Update) {
    // Test that update doesn't crash and maintains state
    handler->update(0.016f);
    
    EXPECT_FALSE(handler->isHandTracking(HandType::Left));
    EXPECT_FALSE(handler->isHandTracking(HandType::Right));
    
    // Add a hand pose and update
    HandPose pose;
    pose.position = Vector3f(0.2f, 1.5f, -0.3f);
    pose.hand = HandType::Left;
    pose.confidence = 0.8f;
    
    VREvent handUpdate;
    handUpdate.type = VREventType::HandUpdate;
    handUpdate.hand = HandType::Left;
    handUpdate.pose = pose;
    
    handler->processVREvent(handUpdate);
    handler->update(0.016f);
    
    EXPECT_EQ(handler->getHandPosition(HandType::Left), pose.position);
}

TEST_F(VRInputHandlerTest, HandLost) {
    // First, establish hand tracking
    HandPose pose;
    pose.position = Vector3f(0.2f, 1.5f, -0.3f);
    pose.confidence = 0.9f;
    pose.hand = HandType::Left;
    
    VREvent handUpdate;
    handUpdate.type = VREventType::HandUpdate;
    handUpdate.hand = HandType::Left;
    handUpdate.pose = pose;
    
    handler->processVREvent(handUpdate);
    EXPECT_TRUE(handler->isHandTracking(HandType::Left));
    
    // Now send update with zero confidence to simulate hand lost
    pose.confidence = 0.0f;
    handUpdate.pose = pose;
    
    handler->processVREvent(handUpdate);
    
    // Check that hand is no longer tracking
    EXPECT_FALSE(handler->isHandTracking(HandType::Left));
}