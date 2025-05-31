#include <gtest/gtest.h>
#include "../WorkspaceManager.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

// Event handler for testing
class TestWorkspaceResizedHandler : public EventHandler<WorkspaceResizedEvent> {
public:
    void handleEvent(const WorkspaceResizedEvent& event) override {
        eventCount++;
        lastOldSize = event.oldSize;
        lastNewSize = event.newSize;
    }
    
    int eventCount = 0;
    Vector3f lastOldSize;
    Vector3f lastNewSize;
};

class WorkspaceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        manager = std::make_unique<WorkspaceManager>(eventDispatcher.get());
        
        // Track events
        resizeHandler = std::make_unique<TestWorkspaceResizedHandler>();
        eventDispatcher->subscribe<WorkspaceResizedEvent>(resizeHandler.get());
        
        resizeEventCount = 0;
        lastOldSize = Vector3f(0, 0, 0);
        lastNewSize = Vector3f(0, 0, 0);
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<WorkspaceManager> manager;
    std::unique_ptr<TestWorkspaceResizedHandler> resizeHandler;
    
    // Event tracking properties (for compatibility with existing test code)
    int resizeEventCount;
    Vector3f lastOldSize;
    Vector3f lastNewSize;
    
    // Helper to update tracking from handler
    void updateEventTracking() {
        resizeEventCount = resizeHandler->eventCount;
        lastOldSize = resizeHandler->lastOldSize;
        lastNewSize = resizeHandler->lastNewSize;
    }
};

TEST_F(WorkspaceManagerTest, DefaultConstruction) {
    WorkspaceManager defaultManager;
    
    Vector3f defaultSize = defaultManager.getSize();
    EXPECT_FLOAT_EQ(defaultSize.x, 5.0f);
    EXPECT_FLOAT_EQ(defaultSize.y, 5.0f);
    EXPECT_FLOAT_EQ(defaultSize.z, 5.0f);
    
    EXPECT_TRUE(defaultManager.isPositionInBounds(Vector3f(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(defaultManager.isPositionInBounds(Vector3f(2.0f, 2.0f, 2.0f)));
    EXPECT_TRUE(defaultManager.isPositionInBounds(Vector3f(-2.0f, -2.0f, -2.0f)));
}

TEST_F(WorkspaceManagerTest, ConstructionWithEventDispatcher) {
    Vector3f size = manager->getSize();
    EXPECT_FLOAT_EQ(size.x, 5.0f);
    EXPECT_FLOAT_EQ(size.y, 5.0f);
    EXPECT_FLOAT_EQ(size.z, 5.0f);
    
    EXPECT_EQ(resizeEventCount, 0); // No events on construction
}

TEST_F(WorkspaceManagerTest, ValidSizeChanges) {
    Vector3f originalSize = manager->getSize();
    
    // Test valid size changes
    std::vector<Vector3f> validSizes = {
        Vector3f(2.0f, 2.0f, 2.0f),    // Minimum size
        Vector3f(3.5f, 4.0f, 2.5f),    // Mixed dimensions within range
        Vector3f(8.0f, 8.0f, 8.0f),    // Maximum size
        Vector3f(6.0f, 3.0f, 7.5f)     // Different valid dimensions
    };
    
    for (const auto& newSize : validSizes) {
        Vector3f oldSize = manager->getSize();
        EXPECT_TRUE(manager->setSize(newSize));
        EXPECT_EQ(manager->getSize(), newSize);
        
        // Check event was dispatched
        updateEventTracking();
        EXPECT_EQ(lastOldSize, oldSize);
        EXPECT_EQ(lastNewSize, newSize);
    }
    
    updateEventTracking();
    EXPECT_EQ(resizeEventCount, validSizes.size());
}

TEST_F(WorkspaceManagerTest, InvalidSizeChanges) {
    Vector3f originalSize = manager->getSize();
    int originalEventCount = resizeEventCount;
    
    // Test invalid size changes
    std::vector<Vector3f> invalidSizes = {
        Vector3f(1.0f, 5.0f, 5.0f),    // X too small
        Vector3f(5.0f, 1.0f, 5.0f),    // Y too small
        Vector3f(5.0f, 5.0f, 1.0f),    // Z too small
        Vector3f(9.0f, 5.0f, 5.0f),    // X too large
        Vector3f(5.0f, 9.0f, 5.0f),    // Y too large
        Vector3f(5.0f, 5.0f, 9.0f),    // Z too large
        Vector3f(0.0f, 5.0f, 5.0f),    // Zero dimension
        Vector3f(-1.0f, 5.0f, 5.0f)    // Negative dimension
    };
    
    for (const auto& invalidSize : invalidSizes) {
        EXPECT_FALSE(manager->setSize(invalidSize));
        EXPECT_EQ(manager->getSize(), originalSize); // Should remain unchanged
    }
    
    // No events should be dispatched for invalid changes
    updateEventTracking();
    EXPECT_EQ(resizeEventCount, originalEventCount);
}

TEST_F(WorkspaceManagerTest, CubicSizeShorthand) {
    // Test cubic size setting
    EXPECT_TRUE(manager->setSize(3.0f));
    Vector3f size = manager->getSize();
    EXPECT_FLOAT_EQ(size.x, 3.0f);
    EXPECT_FLOAT_EQ(size.y, 3.0f);
    EXPECT_FLOAT_EQ(size.z, 3.0f);
    
    // Test invalid cubic sizes
    EXPECT_FALSE(manager->setSize(1.0f));  // Too small
    EXPECT_FALSE(manager->setSize(10.0f)); // Too large
    EXPECT_FALSE(manager->setSize(0.0f));  // Zero
    EXPECT_FALSE(manager->setSize(-1.0f)); // Negative
}

TEST_F(WorkspaceManagerTest, PositionBoundsChecking) {
    manager->setSize(Vector3f(4.0f, 6.0f, 8.0f));
    
    // Test positions within bounds
    std::vector<Vector3f> validPositions = {
        Vector3f(0.0f, 0.0f, 0.0f),     // Center
        Vector3f(1.9f, 2.9f, 3.9f),     // Near positive bounds
        Vector3f(-1.9f, -2.9f, -3.9f),  // Near negative bounds
        Vector3f(2.0f, 3.0f, 4.0f),     // Exactly at positive bounds
        Vector3f(-2.0f, -3.0f, -4.0f),  // Exactly at negative bounds
    };
    
    for (const auto& pos : validPositions) {
        EXPECT_TRUE(manager->isPositionInBounds(pos)) 
            << "Position (" << pos.x << ", " << pos.y << ", " << pos.z << ") should be in bounds";
    }
    
    // Test positions outside bounds
    std::vector<Vector3f> invalidPositions = {
        Vector3f(2.1f, 0.0f, 0.0f),     // X too large
        Vector3f(-2.1f, 0.0f, 0.0f),    // X too small
        Vector3f(0.0f, 3.1f, 0.0f),     // Y too large
        Vector3f(0.0f, -3.1f, 0.0f),    // Y too small
        Vector3f(0.0f, 0.0f, 4.1f),     // Z too large
        Vector3f(0.0f, 0.0f, -4.1f),    // Z too small
        Vector3f(3.0f, 4.0f, 5.0f),     // All dimensions too large
    };
    
    for (const auto& pos : invalidPositions) {
        EXPECT_FALSE(manager->isPositionInBounds(pos))
            << "Position (" << pos.x << ", " << pos.y << ", " << pos.z << ") should be out of bounds";
    }
}

TEST_F(WorkspaceManagerTest, PositionClamping) {
    manager->setSize(Vector3f(4.0f, 6.0f, 8.0f));
    
    // Test clamping of out-of-bounds positions
    struct TestCase {
        Vector3f input;
        Vector3f expected;
    };
    
    std::vector<TestCase> testCases = {
        { Vector3f(3.0f, 0.0f, 0.0f), Vector3f(2.0f, 0.0f, 0.0f) },     // X clamped to max
        { Vector3f(-3.0f, 0.0f, 0.0f), Vector3f(-2.0f, 0.0f, 0.0f) },   // X clamped to min
        { Vector3f(0.0f, 4.0f, 0.0f), Vector3f(0.0f, 3.0f, 0.0f) },     // Y clamped to max
        { Vector3f(0.0f, -4.0f, 0.0f), Vector3f(0.0f, -3.0f, 0.0f) },   // Y clamped to min
        { Vector3f(0.0f, 0.0f, 5.0f), Vector3f(0.0f, 0.0f, 4.0f) },     // Z clamped to max
        { Vector3f(0.0f, 0.0f, -5.0f), Vector3f(0.0f, 0.0f, -4.0f) },   // Z clamped to min
        { Vector3f(5.0f, 7.0f, 9.0f), Vector3f(2.0f, 3.0f, 4.0f) },     // All dimensions clamped
        { Vector3f(1.0f, 1.0f, 1.0f), Vector3f(1.0f, 1.0f, 1.0f) },     // No clamping needed
    };
    
    for (const auto& testCase : testCases) {
        Vector3f clamped = manager->clampPosition(testCase.input);
        EXPECT_FLOAT_EQ(clamped.x, testCase.expected.x);
        EXPECT_FLOAT_EQ(clamped.y, testCase.expected.y);
        EXPECT_FLOAT_EQ(clamped.z, testCase.expected.z);
        
        // Clamped position should always be in bounds
        EXPECT_TRUE(manager->isPositionInBounds(clamped));
    }
}

TEST_F(WorkspaceManagerTest, BoundsRetrievalMethods) {
    manager->setSize(Vector3f(4.0f, 6.0f, 8.0f));
    
    // Test getMinBounds
    Vector3f minBounds = manager->getMinBounds();
    EXPECT_FLOAT_EQ(minBounds.x, -2.0f);
    EXPECT_FLOAT_EQ(minBounds.y, -3.0f);
    EXPECT_FLOAT_EQ(minBounds.z, -4.0f);
    
    // Test getMaxBounds
    Vector3f maxBounds = manager->getMaxBounds();
    EXPECT_FLOAT_EQ(maxBounds.x, 2.0f);
    EXPECT_FLOAT_EQ(maxBounds.y, 3.0f);
    EXPECT_FLOAT_EQ(maxBounds.z, 4.0f);
    
    // Test getBounds
    Vector3f minOut, maxOut;
    manager->getBounds(minOut, maxOut);
    EXPECT_EQ(minOut, minBounds);
    EXPECT_EQ(maxOut, maxBounds);
}

TEST_F(WorkspaceManagerTest, SizeChangeCallbacks) {
    bool callbackInvoked = false;
    Vector3f callbackOldSize, callbackNewSize;
    
    // Set up callback
    manager->setSizeChangeCallback([&](const Vector3f& oldSize, const Vector3f& newSize) {
        callbackInvoked = true;
        callbackOldSize = oldSize;
        callbackNewSize = newSize;
        return true; // Allow the change
    });
    
    Vector3f originalSize = manager->getSize();
    Vector3f newSize(3.0f, 3.0f, 3.0f);
    
    // Change size - callback should be invoked
    EXPECT_TRUE(manager->setSize(newSize));
    EXPECT_TRUE(callbackInvoked);
    EXPECT_EQ(callbackOldSize, originalSize);
    EXPECT_EQ(callbackNewSize, newSize);
    EXPECT_EQ(manager->getSize(), newSize);
}

TEST_F(WorkspaceManagerTest, SizeChangeCallbackVeto) {
    Vector3f originalSize = manager->getSize();
    
    // Set up callback that vetoes changes
    manager->setSizeChangeCallback([](const Vector3f& oldSize, const Vector3f& newSize) {
        return false; // Veto the change
    });
    
    Vector3f attemptedSize(3.0f, 3.0f, 3.0f);
    
    // Attempt to change size - should be vetoed
    EXPECT_FALSE(manager->setSize(attemptedSize));
    EXPECT_EQ(manager->getSize(), originalSize); // Should remain unchanged
    
    // Event should not be dispatched for vetoed changes
    EXPECT_EQ(resizeEventCount, 0);
}

TEST_F(WorkspaceManagerTest, SizeChangeCallbackConditional) {
    Vector3f originalSize = manager->getSize();
    
    // Set up callback that only allows increases
    manager->setSizeChangeCallback([](const Vector3f& oldSize, const Vector3f& newSize) {
        return newSize.x >= oldSize.x && newSize.y >= oldSize.y && newSize.z >= oldSize.z;
    });
    
    // Try to increase size - should succeed
    Vector3f largerSize(6.0f, 6.0f, 6.0f);
    EXPECT_TRUE(manager->setSize(largerSize));
    EXPECT_EQ(manager->getSize(), largerSize);
    
    // Try to decrease size - should fail
    Vector3f smallerSize(3.0f, 3.0f, 3.0f);
    EXPECT_FALSE(manager->setSize(smallerSize));
    EXPECT_EQ(manager->getSize(), largerSize); // Should remain unchanged
}

TEST_F(WorkspaceManagerTest, EventDispatcherChanges) {
    // Change size with dispatcher
    manager->setSize(Vector3f(3.0f, 3.0f, 3.0f));
    EXPECT_EQ(resizeEventCount, 1);
    
    // Remove event dispatcher
    manager->setEventDispatcher(nullptr);
    
    // Change size without dispatcher - no events should be dispatched
    int previousEventCount = resizeEventCount;
    manager->setSize(Vector3f(4.0f, 4.0f, 4.0f));
    EXPECT_EQ(resizeEventCount, previousEventCount);
    
    // Set dispatcher back
    manager->setEventDispatcher(eventDispatcher.get());
    
    // Change size with dispatcher - events should be dispatched again
    manager->setSize(Vector3f(6.0f, 6.0f, 6.0f));
    EXPECT_EQ(resizeEventCount, previousEventCount + 1);
}

TEST_F(WorkspaceManagerTest, MultipleSizeChanges) {
    std::vector<Vector3f> sizes = {
        Vector3f(2.0f, 2.0f, 2.0f),
        Vector3f(4.0f, 3.0f, 5.0f),
        Vector3f(8.0f, 8.0f, 8.0f),
        Vector3f(3.0f, 7.0f, 2.5f),
        Vector3f(5.0f, 5.0f, 5.0f)
    };
    
    Vector3f currentSize = manager->getSize();
    
    for (const auto& targetSize : sizes) {
        Vector3f previousSize = currentSize;
        EXPECT_TRUE(manager->setSize(targetSize));
        
        currentSize = manager->getSize();
        EXPECT_EQ(currentSize, targetSize);
        
        // Verify event was dispatched correctly
        EXPECT_EQ(lastOldSize, previousSize);
        EXPECT_EQ(lastNewSize, targetSize);
    }
    
    EXPECT_EQ(resizeEventCount, sizes.size());
}

TEST_F(WorkspaceManagerTest, EdgeCaseBounds) {
    // Test workspace at minimum size
    manager->setSize(Vector3f(2.0f, 2.0f, 2.0f));
    
    EXPECT_TRUE(manager->isPositionInBounds(Vector3f(1.0f, 1.0f, 1.0f)));
    EXPECT_TRUE(manager->isPositionInBounds(Vector3f(-1.0f, -1.0f, -1.0f)));
    EXPECT_FALSE(manager->isPositionInBounds(Vector3f(1.1f, 0.0f, 0.0f)));
    EXPECT_FALSE(manager->isPositionInBounds(Vector3f(-1.1f, 0.0f, 0.0f)));
    
    // Test workspace at maximum size
    manager->setSize(Vector3f(8.0f, 8.0f, 8.0f));
    
    EXPECT_TRUE(manager->isPositionInBounds(Vector3f(4.0f, 4.0f, 4.0f)));
    EXPECT_TRUE(manager->isPositionInBounds(Vector3f(-4.0f, -4.0f, -4.0f)));
    EXPECT_FALSE(manager->isPositionInBounds(Vector3f(4.1f, 0.0f, 0.0f)));
    EXPECT_FALSE(manager->isPositionInBounds(Vector3f(-4.1f, 0.0f, 0.0f)));
}

TEST_F(WorkspaceManagerTest, NonCubicWorkspaces) {
    // Test asymmetric workspace
    manager->setSize(Vector3f(2.0f, 4.0f, 8.0f));
    
    // Test bounds for each dimension independently
    EXPECT_TRUE(manager->isPositionInBounds(Vector3f(1.0f, 0.0f, 0.0f)));
    EXPECT_FALSE(manager->isPositionInBounds(Vector3f(1.1f, 0.0f, 0.0f)));
    
    EXPECT_TRUE(manager->isPositionInBounds(Vector3f(0.0f, 2.0f, 0.0f)));
    EXPECT_FALSE(manager->isPositionInBounds(Vector3f(0.0f, 2.1f, 0.0f)));
    
    EXPECT_TRUE(manager->isPositionInBounds(Vector3f(0.0f, 0.0f, 4.0f)));
    EXPECT_FALSE(manager->isPositionInBounds(Vector3f(0.0f, 0.0f, 4.1f)));
    
    // Test corner cases
    EXPECT_TRUE(manager->isPositionInBounds(Vector3f(1.0f, 2.0f, 4.0f)));
    EXPECT_TRUE(manager->isPositionInBounds(Vector3f(-1.0f, -2.0f, -4.0f)));
    EXPECT_FALSE(manager->isPositionInBounds(Vector3f(1.1f, 2.1f, 4.1f)));
}

TEST_F(WorkspaceManagerTest, ConstMethodsWithConstManager) {
    const WorkspaceManager& constManager = *manager;
    
    // Test that const methods work correctly
    Vector3f size = constManager.getSize();
    EXPECT_GT(size.x, 0.0f);
    EXPECT_GT(size.y, 0.0f);
    EXPECT_GT(size.z, 0.0f);
    
    EXPECT_TRUE(constManager.isPositionInBounds(Vector3f(0.0f, 0.0f, 0.0f)));
    
    Vector3f minBounds = constManager.getMinBounds();
    Vector3f maxBounds = constManager.getMaxBounds();
    EXPECT_LT(minBounds.x, maxBounds.x);
    EXPECT_LT(minBounds.y, maxBounds.y);
    EXPECT_LT(minBounds.z, maxBounds.z);
    
    Vector3f clamped = constManager.clampPosition(Vector3f(100.0f, 100.0f, 100.0f));
    EXPECT_TRUE(constManager.isPositionInBounds(clamped));
}