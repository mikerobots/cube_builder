#include <gtest/gtest.h>
#include "../TouchHandler.h"

using namespace VoxelEditor::Input;
using namespace VoxelEditor::Math;

class TouchHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<TouchHandler>();
    }
    
    void TearDown() override {}
    
    std::unique_ptr<TouchHandler> handler;
};

TEST_F(TouchHandlerTest, DefaultState) {
    EXPECT_FALSE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 0);
    EXPECT_TRUE(handler->getActiveTouches().empty());
    
    // Default gesture states
    EXPECT_FALSE(handler->isGestureActive(TouchGesture::Tap));
    EXPECT_FALSE(handler->isGestureActive(TouchGesture::Pinch));
    EXPECT_FALSE(handler->isGestureActive(TouchGesture::Pan));
    
    // Default configuration
    EXPECT_FLOAT_EQ(handler->getTapTimeout(), 0.3f);
    EXPECT_FLOAT_EQ(handler->getTapRadius(), 20.0f);
    EXPECT_FLOAT_EQ(handler->getPinchThreshold(), 50.0f);
    EXPECT_FLOAT_EQ(handler->getSwipeThreshold(), 100.0f);
}

TEST_F(TouchHandlerTest, SingleTouchBeginEnd) {
    // REQ-7.3.1: System shall use Qt6 for desktop GUI application
    Vector2f touchPos(100.0f, 200.0f);
    
    // Touch begin
    std::vector<TouchPoint> points;
    points.emplace_back(1, touchPos, TouchState::Pressed);
    TouchEvent beginEvent(TouchEventType::TouchBegin, points);
    
    handler->processTouchEvent(beginEvent);
    
    EXPECT_TRUE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 1);
    
    auto activeTouches = handler->getActiveTouches();
    EXPECT_EQ(activeTouches.size(), 1);
    EXPECT_EQ(activeTouches[0].id, 1);
    EXPECT_EQ(activeTouches[0].position, touchPos);
    EXPECT_EQ(activeTouches[0].state, TouchState::Pressed);
    
    // Touch end
    points[0].state = TouchState::Released;
    TouchEvent endEvent(TouchEventType::TouchEnd, points);
    
    handler->processTouchEvent(endEvent);
    
    EXPECT_FALSE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 0);
}

TEST_F(TouchHandlerTest, TouchUpdate) {
    Vector2f startPos(100.0f, 200.0f);
    Vector2f newPos(150.0f, 250.0f);
    Vector2f expectedDelta = newPos - startPos;
    
    // Touch begin
    std::vector<TouchPoint> points;
    points.emplace_back(1, startPos, TouchState::Pressed);
    TouchEvent beginEvent(TouchEventType::TouchBegin, points);
    handler->processTouchEvent(beginEvent);
    
    // Touch update
    points[0].position = newPos;
    points[0].delta = expectedDelta;
    points[0].state = TouchState::Moved;
    TouchEvent updateEvent(TouchEventType::TouchUpdate, points);
    
    handler->processTouchEvent(updateEvent);
    
    auto activeTouches = handler->getActiveTouches();
    EXPECT_EQ(activeTouches.size(), 1);
    EXPECT_EQ(activeTouches[0].position, newPos);
    EXPECT_EQ(activeTouches[0].delta, expectedDelta);
    EXPECT_EQ(activeTouches[0].state, TouchState::Moved);
}

TEST_F(TouchHandlerTest, MultipleTouches) {
    Vector2f pos1(100.0f, 200.0f);
    Vector2f pos2(300.0f, 400.0f);
    
    // Two touch begin
    std::vector<TouchPoint> points;
    points.emplace_back(1, pos1, TouchState::Pressed);
    points.emplace_back(2, pos2, TouchState::Pressed);
    TouchEvent beginEvent(TouchEventType::TouchBegin, points);
    
    handler->processTouchEvent(beginEvent);
    
    EXPECT_TRUE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 2);
    
    auto activeTouches = handler->getActiveTouches();
    EXPECT_EQ(activeTouches.size(), 2);
    
    // Check both touches
    TouchPoint touch1 = handler->getTouchById(1);
    TouchPoint touch2 = handler->getTouchById(2);
    
    EXPECT_EQ(touch1.id, 1);
    EXPECT_EQ(touch1.position, pos1);
    EXPECT_EQ(touch2.id, 2);
    EXPECT_EQ(touch2.position, pos2);
    
    // End one touch
    points.clear();
    points.emplace_back(1, pos1, TouchState::Released);
    TouchEvent endEvent(TouchEventType::TouchEnd, points);
    
    handler->processTouchEvent(endEvent);
    
    EXPECT_TRUE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 1);
    
    // Only touch 2 should remain
    activeTouches = handler->getActiveTouches();
    EXPECT_EQ(activeTouches.size(), 1);
    EXPECT_EQ(activeTouches[0].id, 2);
}

TEST_F(TouchHandlerTest, PrimaryTouch) {
    Vector2f pos1(100.0f, 200.0f);
    Vector2f pos2(300.0f, 400.0f);
    
    // Add first touch
    std::vector<TouchPoint> points;
    points.emplace_back(1, pos1, TouchState::Pressed);
    TouchEvent beginEvent1(TouchEventType::TouchBegin, points);
    handler->processTouchEvent(beginEvent1);
    
    TouchPoint primary1 = handler->getPrimaryTouch();
    EXPECT_EQ(primary1.id, 1);
    EXPECT_EQ(primary1.position, pos1);
    
    // Add second touch
    points.clear();
    points.emplace_back(2, pos2, TouchState::Pressed);
    TouchEvent beginEvent2(TouchEventType::TouchBegin, points);
    handler->processTouchEvent(beginEvent2);
    
    // Primary should still be the first touch
    TouchPoint primary2 = handler->getPrimaryTouch();
    EXPECT_EQ(primary2.id, 1);
    EXPECT_EQ(primary2.position, pos1);
}

TEST_F(TouchHandlerTest, GestureConfiguration) {
    // Test default gesture states
    EXPECT_TRUE(handler->isGestureEnabled(TouchGesture::Tap));
    EXPECT_TRUE(handler->isGestureEnabled(TouchGesture::Pan));
    EXPECT_TRUE(handler->isGestureEnabled(TouchGesture::Pinch));
    
    // Disable some gestures
    handler->enableGesture(TouchGesture::Tap, false);
    handler->enableGesture(TouchGesture::Rotation, false);
    
    EXPECT_FALSE(handler->isGestureEnabled(TouchGesture::Tap));
    EXPECT_FALSE(handler->isGestureEnabled(TouchGesture::Rotation));
    EXPECT_TRUE(handler->isGestureEnabled(TouchGesture::Pan));
    
    // Re-enable
    handler->enableGesture(TouchGesture::Tap, true);
    EXPECT_TRUE(handler->isGestureEnabled(TouchGesture::Tap));
}

TEST_F(TouchHandlerTest, TouchConfiguration) {
    // Test configuration changes
    handler->setTapTimeout(0.4f);
    handler->setTapRadius(25.0f);
    handler->setPinchThreshold(75.0f);
    handler->setSwipeThreshold(120.0f);
    handler->setRotationThreshold(15.0f);
    handler->setLongPressTimeout(1.5f);
    handler->setSensitivity(2.0f);
    
    EXPECT_FLOAT_EQ(handler->getTapTimeout(), 0.4f);
    EXPECT_FLOAT_EQ(handler->getTapRadius(), 25.0f);
    EXPECT_FLOAT_EQ(handler->getPinchThreshold(), 75.0f);
    EXPECT_FLOAT_EQ(handler->getSwipeThreshold(), 120.0f);
    EXPECT_FLOAT_EQ(handler->getRotationThreshold(), 15.0f);
    EXPECT_FLOAT_EQ(handler->getLongPressTimeout(), 1.5f);
    EXPECT_FLOAT_EQ(handler->getSensitivity(), 2.0f);
}

TEST_F(TouchHandlerTest, TouchCancel) {
    Vector2f touchPos(100.0f, 200.0f);
    
    // Touch begin
    std::vector<TouchPoint> points;
    points.emplace_back(1, touchPos, TouchState::Pressed);
    TouchEvent beginEvent(TouchEventType::TouchBegin, points);
    handler->processTouchEvent(beginEvent);
    
    EXPECT_TRUE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 1);
    
    // Touch cancel
    points[0].state = TouchState::Cancelled;
    TouchEvent cancelEvent(TouchEventType::TouchCancel, points);
    handler->processTouchEvent(cancelEvent);
    
    EXPECT_FALSE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 0);
}

TEST_F(TouchHandlerTest, EnabledState) {
    handler->setEnabled(false);
    EXPECT_FALSE(handler->isEnabled());
    
    // Events should be ignored when disabled
    Vector2f touchPos(100.0f, 200.0f);
    std::vector<TouchPoint> points;
    points.emplace_back(1, touchPos, TouchState::Pressed);
    TouchEvent beginEvent(TouchEventType::TouchBegin, points);
    
    handler->processTouchEvent(beginEvent);
    
    EXPECT_FALSE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 0);
    
    // Re-enable and test
    handler->setEnabled(true);
    EXPECT_TRUE(handler->isEnabled());
    
    handler->processTouchEvent(beginEvent);
    EXPECT_TRUE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 1);
}

TEST_F(TouchHandlerTest, TouchGestureUtilities) {
    // Test gesture to string conversion
    EXPECT_EQ(TouchHandler::touchGestureToString(TouchGesture::Tap), "Tap");
    EXPECT_EQ(TouchHandler::touchGestureToString(TouchGesture::Pinch), "Pinch");
    EXPECT_EQ(TouchHandler::touchGestureToString(TouchGesture::Pan), "Pan");
    EXPECT_EQ(TouchHandler::touchGestureToString(TouchGesture::Swipe), "Swipe");
    EXPECT_EQ(TouchHandler::touchGestureToString(TouchGesture::Rotation), "Rotation");
    
    // Test string to gesture conversion
    EXPECT_EQ(TouchHandler::touchGestureFromString("Tap"), TouchGesture::Tap);
    EXPECT_EQ(TouchHandler::touchGestureFromString("Pinch"), TouchGesture::Pinch);
    EXPECT_EQ(TouchHandler::touchGestureFromString("Pan"), TouchGesture::Pan);
    EXPECT_EQ(TouchHandler::touchGestureFromString("Unknown"), TouchGesture::Tap); // Default fallback
    
    // Test gesture validation
    EXPECT_TRUE(TouchHandler::isValidTouchGesture(TouchGesture::Tap));
    EXPECT_TRUE(TouchHandler::isValidTouchGesture(TouchGesture::Pinch));
    EXPECT_TRUE(TouchHandler::isValidTouchGesture(TouchGesture::Pan));
    EXPECT_TRUE(TouchHandler::isValidTouchGesture(TouchGesture::Swipe));
    EXPECT_TRUE(TouchHandler::isValidTouchGesture(TouchGesture::Rotation));
}

TEST_F(TouchHandlerTest, GestureData) {
    // Test gesture data retrieval (when gestures are active)
    // Note: These would require actual gesture recognition to be active
    
    Vector2f center = handler->getGestureCenter(TouchGesture::Pinch);
    EXPECT_EQ(center, Vector2f::zero()); // Default when no gesture
    
    float scale = handler->getGestureScale(TouchGesture::Pinch);
    EXPECT_FLOAT_EQ(scale, 1.0f); // Default scale
    
    float rotation = handler->getGestureRotation(TouchGesture::Rotation);
    EXPECT_FLOAT_EQ(rotation, 0.0f); // Default rotation
    
    Vector2f velocity = handler->getGestureVelocity(TouchGesture::Pan);
    EXPECT_EQ(velocity, Vector2f::zero()); // Default velocity
}

TEST_F(TouchHandlerTest, Update) {
    // Test that update doesn't crash and maintains state
    handler->update(0.016f);
    
    EXPECT_FALSE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 0);
    
    // Add a touch and update
    Vector2f touchPos(100.0f, 200.0f);
    std::vector<TouchPoint> points;
    points.emplace_back(1, touchPos, TouchState::Pressed);
    TouchEvent beginEvent(TouchEventType::TouchBegin, points);
    
    handler->processTouchEvent(beginEvent);
    handler->update(0.016f);
    
    EXPECT_TRUE(handler->hasTouches());
    EXPECT_EQ(handler->getTouchCount(), 1);
}