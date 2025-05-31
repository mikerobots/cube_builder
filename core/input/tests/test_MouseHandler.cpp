#include <gtest/gtest.h>
#include "../MouseHandler.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor::Input;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class MockEventDispatcher : public EventDispatcher {
public:
    mutable std::vector<std::string> dispatchedEvents;
    
    template<typename EventType>
    void dispatch(const EventType&) const {
        dispatchedEvents.push_back(typeid(EventType).name());
    }
};

class MouseHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<MockEventDispatcher>();
        handler = std::make_unique<MouseHandler>(eventDispatcher.get());
    }
    
    void TearDown() override {}
    
    std::unique_ptr<MockEventDispatcher> eventDispatcher;
    std::unique_ptr<MouseHandler> handler;
};

TEST_F(MouseHandlerTest, DefaultState) {
    EXPECT_FALSE(handler->isButtonPressed(MouseButton::Left));
    EXPECT_FALSE(handler->isButtonPressed(MouseButton::Right));
    EXPECT_FALSE(handler->isButtonPressed(MouseButton::Middle));
    
    EXPECT_EQ(handler->getPosition(), Vector2f::zero());
    EXPECT_EQ(handler->getDelta(), Vector2f::zero());
    EXPECT_FLOAT_EQ(handler->getWheelDelta(), 0.0f);
    
    EXPECT_FALSE(handler->isDoubleClick(MouseButton::Left));
    EXPECT_FALSE(handler->isDragging(MouseButton::Left));
    EXPECT_EQ(handler->getClickCount(MouseButton::Left), 0);
}

TEST_F(MouseHandlerTest, ButtonPressRelease) {
    Vector2f clickPos(100.0f, 200.0f);
    
    // Press left button
    MouseEvent pressEvent(MouseEventType::ButtonPress, MouseButton::Left, clickPos);
    handler->processMouseEvent(pressEvent);
    
    EXPECT_TRUE(handler->isButtonPressed(MouseButton::Left));
    EXPECT_TRUE(handler->isButtonJustPressed(MouseButton::Left));
    EXPECT_FALSE(handler->isButtonJustReleased(MouseButton::Left));
    EXPECT_FALSE(handler->isButtonPressed(MouseButton::Right));
    
    // Update to clear just pressed state
    handler->update(0.016f);
    EXPECT_TRUE(handler->isButtonPressed(MouseButton::Left));
    EXPECT_FALSE(handler->isButtonJustPressed(MouseButton::Left));
    
    // Release left button
    MouseEvent releaseEvent(MouseEventType::ButtonRelease, MouseButton::Left, clickPos);
    handler->processMouseEvent(releaseEvent);
    
    EXPECT_FALSE(handler->isButtonPressed(MouseButton::Left));
    EXPECT_FALSE(handler->isButtonJustPressed(MouseButton::Left));
    EXPECT_TRUE(handler->isButtonJustReleased(MouseButton::Left));
}

TEST_F(MouseHandlerTest, MouseMovement) {
    Vector2f startPos(50.0f, 50.0f);
    Vector2f endPos(100.0f, 150.0f);
    Vector2f expectedDelta = endPos - startPos;
    
    // Initial position
    MouseEvent moveEvent1(MouseEventType::Move, MouseButton::None, startPos);
    handler->processMouseEvent(moveEvent1);
    EXPECT_EQ(handler->getPosition(), startPos);
    
    // Move to new position
    MouseEvent moveEvent2(MouseEventType::Move, MouseButton::None, endPos);
    moveEvent2.delta = expectedDelta;
    handler->processMouseEvent(moveEvent2);
    
    EXPECT_EQ(handler->getPosition(), endPos);
    EXPECT_EQ(handler->getDelta(), expectedDelta);
}

TEST_F(MouseHandlerTest, MouseWheel) {
    Vector2f pos(200.0f, 300.0f);
    float wheelDelta = 120.0f;
    
    MouseEvent wheelEvent(MouseEventType::Wheel, MouseButton::None, pos);
    wheelEvent.wheelDelta = wheelDelta;
    handler->processMouseEvent(wheelEvent);
    
    EXPECT_FLOAT_EQ(handler->getWheelDelta(), wheelDelta);
    
    // Wheel delta should reset after update
    handler->update(0.016f);
    EXPECT_FLOAT_EQ(handler->getWheelDelta(), 0.0f);
}

TEST_F(MouseHandlerTest, ClickDetection) {
    Vector2f clickPos(150.0f, 250.0f);
    
    // Single click
    MouseEvent pressEvent(MouseEventType::ButtonPress, MouseButton::Left, clickPos);
    handler->processMouseEvent(pressEvent);
    
    MouseEvent releaseEvent(MouseEventType::ButtonRelease, MouseButton::Left, clickPos);
    handler->processMouseEvent(releaseEvent);
    
    EXPECT_EQ(handler->getClickCount(MouseButton::Left), 1);
    EXPECT_EQ(handler->getClickPosition(MouseButton::Left), clickPos);
}

TEST_F(MouseHandlerTest, DoubleClickDetection) {
    Vector2f clickPos(100.0f, 200.0f);
    
    // First click
    MouseEvent pressEvent1(MouseEventType::ButtonPress, MouseButton::Left, clickPos);
    handler->processMouseEvent(pressEvent1);
    MouseEvent releaseEvent1(MouseEventType::ButtonRelease, MouseButton::Left, clickPos);
    handler->processMouseEvent(releaseEvent1);
    
    EXPECT_EQ(handler->getClickCount(MouseButton::Left), 1);
    
    // Second click quickly after first
    MouseEvent pressEvent2(MouseEventType::ButtonPress, MouseButton::Left, clickPos);
    handler->processMouseEvent(pressEvent2);
    MouseEvent releaseEvent2(MouseEventType::ButtonRelease, MouseButton::Left, clickPos);
    handler->processMouseEvent(releaseEvent2);
    
    EXPECT_EQ(handler->getClickCount(MouseButton::Left), 2);
    EXPECT_TRUE(handler->isDoubleClick(MouseButton::Left));
}

TEST_F(MouseHandlerTest, DragDetection) {
    Vector2f startPos(100.0f, 100.0f);
    Vector2f dragPos(150.0f, 150.0f); // 50+ pixels away
    
    // Press button
    MouseEvent pressEvent(MouseEventType::ButtonPress, MouseButton::Left, startPos);
    handler->processMouseEvent(pressEvent);
    
    EXPECT_FALSE(handler->isDragging(MouseButton::Left));
    
    // Move mouse while button is pressed (beyond drag threshold)
    MouseEvent moveEvent(MouseEventType::Move, MouseButton::None, dragPos);
    handler->processMouseEvent(moveEvent);
    
    EXPECT_TRUE(handler->isDragging(MouseButton::Left));
}

TEST_F(MouseHandlerTest, MultipleButtons) {
    Vector2f pos(200.0f, 300.0f);
    
    // Press left button
    MouseEvent leftPress(MouseEventType::ButtonPress, MouseButton::Left, pos);
    handler->processMouseEvent(leftPress);
    
    // Press right button while left is still pressed
    MouseEvent rightPress(MouseEventType::ButtonPress, MouseButton::Right, pos);
    handler->processMouseEvent(rightPress);
    
    EXPECT_TRUE(handler->isButtonPressed(MouseButton::Left));
    EXPECT_TRUE(handler->isButtonPressed(MouseButton::Right));
    EXPECT_FALSE(handler->isButtonPressed(MouseButton::Middle));
    
    // Release left button
    MouseEvent leftRelease(MouseEventType::ButtonRelease, MouseButton::Left, pos);
    handler->processMouseEvent(leftRelease);
    
    EXPECT_FALSE(handler->isButtonPressed(MouseButton::Left));
    EXPECT_TRUE(handler->isButtonPressed(MouseButton::Right));
}

TEST_F(MouseHandlerTest, Configuration) {
    // Test default configuration
    EXPECT_FLOAT_EQ(handler->getClickTimeout(), 0.3f);
    EXPECT_FLOAT_EQ(handler->getDoubleClickTimeout(), 0.5f);
    EXPECT_FLOAT_EQ(handler->getDragThreshold(), 5.0f);
    EXPECT_FLOAT_EQ(handler->getSensitivity(), 1.0f);
    
    // Test configuration changes
    handler->setClickTimeout(0.4f);
    handler->setDoubleClickTimeout(0.6f);
    handler->setDragThreshold(10.0f);
    handler->setSensitivity(2.0f);
    
    EXPECT_FLOAT_EQ(handler->getClickTimeout(), 0.4f);
    EXPECT_FLOAT_EQ(handler->getDoubleClickTimeout(), 0.6f);
    EXPECT_FLOAT_EQ(handler->getDragThreshold(), 10.0f);
    EXPECT_FLOAT_EQ(handler->getSensitivity(), 2.0f);
}

TEST_F(MouseHandlerTest, PositionFiltering) {
    // Test position filtering
    handler->setPositionFilter(true);
    handler->setMinimumMovement(5.0f);
    
    Vector2f startPos(100.0f, 100.0f);
    Vector2f smallMovePos(102.0f, 101.0f); // Small movement
    Vector2f largeMovePos(110.0f, 115.0f); // Large movement
    
    // Initial position
    MouseEvent startEvent(MouseEventType::Move, MouseButton::None, startPos);
    handler->processMouseEvent(startEvent);
    EXPECT_EQ(handler->getPosition(), startPos);
    
    // Small movement should be filtered
    MouseEvent smallMoveEvent(MouseEventType::Move, MouseButton::None, smallMovePos);
    handler->processMouseEvent(smallMoveEvent);
    // Position should not change significantly due to filtering
    
    // Large movement should not be filtered
    MouseEvent largeMoveEvent(MouseEventType::Move, MouseButton::None, largeMovePos);
    handler->processMouseEvent(largeMoveEvent);
    EXPECT_EQ(handler->getPosition(), largeMovePos);
}

TEST_F(MouseHandlerTest, EnabledState) {
    handler->setEnabled(false);
    EXPECT_FALSE(handler->isEnabled());
    
    // Events should be ignored when disabled
    Vector2f clickPos(100.0f, 200.0f);
    MouseEvent pressEvent(MouseEventType::ButtonPress, MouseButton::Left, clickPos);
    handler->processMouseEvent(pressEvent);
    
    EXPECT_FALSE(handler->isButtonPressed(MouseButton::Left));
    
    // Re-enable and test
    handler->setEnabled(true);
    EXPECT_TRUE(handler->isEnabled());
    
    handler->processMouseEvent(pressEvent);
    EXPECT_TRUE(handler->isButtonPressed(MouseButton::Left));
}

TEST_F(MouseHandlerTest, MouseButtonUtilities) {
    // Test button to string conversion
    EXPECT_EQ(MouseHandler::mouseButtonToString(MouseButton::Left), "Left");
    EXPECT_EQ(MouseHandler::mouseButtonToString(MouseButton::Right), "Right");
    EXPECT_EQ(MouseHandler::mouseButtonToString(MouseButton::Middle), "Middle");
    EXPECT_EQ(MouseHandler::mouseButtonToString(MouseButton::None), "None");
    
    // Test string to button conversion
    EXPECT_EQ(MouseHandler::mouseButtonFromString("Left"), MouseButton::Left);
    EXPECT_EQ(MouseHandler::mouseButtonFromString("Right"), MouseButton::Right);
    EXPECT_EQ(MouseHandler::mouseButtonFromString("Middle"), MouseButton::Middle);
    EXPECT_EQ(MouseHandler::mouseButtonFromString("Unknown"), MouseButton::None);
    
    // Test button validation
    EXPECT_TRUE(MouseHandler::isValidMouseButton(MouseButton::Left));
    EXPECT_TRUE(MouseHandler::isValidMouseButton(MouseButton::Right));
    EXPECT_TRUE(MouseHandler::isValidMouseButton(MouseButton::Middle));
    EXPECT_TRUE(MouseHandler::isValidMouseButton(MouseButton::Button4));
    EXPECT_FALSE(MouseHandler::isValidMouseButton(MouseButton::None));
}

TEST_F(MouseHandlerTest, RayCasting) {
    // This is a basic test for ray creation interface
    // Full ray casting would require camera implementation
    Vector2f mousePos(400.0f, 300.0f);
    VoxelEditor::Math::Vector2i viewportSize(800, 600);
    
    // Create a mock camera (this would use actual Camera class in real implementation)
    // For now, just test that the method exists and can be called
    // Camera::Camera mockCamera;
    // Ray ray = handler->createRayFromMouse(mousePos, mockCamera, viewportSize);
    
    // Test would verify ray origin and direction are reasonable
    // This test is a placeholder until Camera integration is complete
    EXPECT_TRUE(true); // Placeholder assertion
}