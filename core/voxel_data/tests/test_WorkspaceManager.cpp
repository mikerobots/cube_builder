#include <gtest/gtest.h>
#include "../WorkspaceManager.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"

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
    
    // Center coordinate system: X and Z from -2.5 to 2.5, Y from 0 to 5
    EXPECT_TRUE(defaultManager.isPositionValid(Vector3f(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(defaultManager.isPositionValid(Vector3f(2.0f, 2.0f, 2.0f)));
    EXPECT_TRUE(defaultManager.isPositionValid(Vector3f(-2.0f, 2.0f, -2.0f))); // X and Z can be negative
    EXPECT_FALSE(defaultManager.isPositionValid(Vector3f(0.0f, -1.0f, 0.0f))); // Y cannot be negative
}

TEST_F(WorkspaceManagerTest, ConstructionWithEventDispatcher) {
    Vector3f size = manager->getSize();
    EXPECT_FLOAT_EQ(size.x, 5.0f);
    EXPECT_FLOAT_EQ(size.y, 5.0f);
    EXPECT_FLOAT_EQ(size.z, 5.0f);
    
    EXPECT_EQ(resizeEventCount, 0); // No events on construction
}

// REQ-6.2.2: Grid size shall scale with workspace (up to 8m x 8m)
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

// REQ-2.1.4: No voxels shall be placed below Y=0
TEST_F(WorkspaceManagerTest, PositionBoundsChecking) {
    manager->setSize(Vector3f(4.0f, 6.0f, 8.0f));
    
    // Test positions within bounds (centered coordinate system)
    // X: -2 to 2, Y: 0 to 6, Z: -4 to 4
    std::vector<Vector3f> validPositions = {
        Vector3f(0.0f, 0.0f, 0.0f),     // Origin
        Vector3f(-2.0f, 0.0f, -4.0f),   // Min corner (with Y=0)
        Vector3f(2.0f, 6.0f, 4.0f),     // Max corner
        Vector3f(1.9f, 5.9f, 3.9f),     // Near max bounds
        Vector3f(-1.9f, 0.1f, -3.9f),   // Near min bounds
        Vector3f(0.0f, 3.0f, 0.0f),     // Center
    };
    
    for (const auto& pos : validPositions) {
        EXPECT_TRUE(manager->isPositionValid(pos)) 
            << "Position (" << pos.x << ", " << pos.y << ", " << pos.z << ") should be in bounds";
    }
    
    // Test positions outside bounds
    std::vector<Vector3f> invalidPositions = {
        Vector3f(2.1f, 0.0f, 0.0f),     // X too large
        Vector3f(-2.1f, 0.0f, 0.0f),    // X too small
        Vector3f(0.0f, 6.1f, 0.0f),     // Y too large
        Vector3f(0.0f, -0.1f, 0.0f),    // Y negative (not allowed)
        Vector3f(0.0f, 0.0f, 4.1f),     // Z too large
        Vector3f(0.0f, 0.0f, -4.1f),    // Z too small
        Vector3f(3.0f, 7.0f, 5.0f),     // All dimensions too large
        Vector3f(-3.0f, -1.0f, -5.0f),  // X,Z too small, Y negative
    };
    
    for (const auto& pos : invalidPositions) {
        EXPECT_FALSE(manager->isPositionValid(pos))
            << "Position (" << pos.x << ", " << pos.y << ", " << pos.z << ") should be out of bounds";
    }
}

TEST_F(WorkspaceManagerTest, PositionClamping) {
    manager->setSize(Vector3f(4.0f, 6.0f, 8.0f));
    // X: -2 to 2, Y: 0 to 6, Z: -4 to 4
    
    // Test clamping of out-of-bounds positions
    struct TestCase {
        Vector3f input;
        Vector3f expected;
    };
    
    std::vector<TestCase> testCases = {
        { Vector3f(3.0f, 3.0f, 0.0f), Vector3f(2.0f, 3.0f, 0.0f) },      // X clamped to max
        { Vector3f(-3.0f, 3.0f, 0.0f), Vector3f(-2.0f, 3.0f, 0.0f) },   // X clamped to min
        { Vector3f(0.0f, 7.0f, 0.0f), Vector3f(0.0f, 6.0f, 0.0f) },      // Y clamped to max
        { Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.0f) },     // Y clamped to min
        { Vector3f(0.0f, 3.0f, 5.0f), Vector3f(0.0f, 3.0f, 4.0f) },      // Z clamped to max
        { Vector3f(0.0f, 3.0f, -5.0f), Vector3f(0.0f, 3.0f, -4.0f) },    // Z clamped to min
        { Vector3f(5.0f, 7.0f, 9.0f), Vector3f(2.0f, 6.0f, 4.0f) },      // All dimensions clamped
        { Vector3f(-5.0f, -1.0f, -9.0f), Vector3f(-2.0f, 0.0f, -4.0f) }, // All dimensions clamped
        { Vector3f(1.0f, 3.0f, -2.0f), Vector3f(1.0f, 3.0f, -2.0f) },    // No clamping needed
    };
    
    for (const auto& testCase : testCases) {
        Vector3f clamped = manager->clampPosition(testCase.input);
        EXPECT_FLOAT_EQ(clamped.x, testCase.expected.x);
        EXPECT_FLOAT_EQ(clamped.y, testCase.expected.y);
        EXPECT_FLOAT_EQ(clamped.z, testCase.expected.z);
        
        // Clamped position should always be in bounds
        EXPECT_TRUE(manager->isPositionValid(clamped));
    }
}

// REQ-1.1.5: The grid origin (0,0,0) shall be at the center of the workspace
TEST_F(WorkspaceManagerTest, BoundsRetrievalMethods) {
    manager->setSize(Vector3f(4.0f, 6.0f, 8.0f));
    
    // Test getMinBounds - centered coordinate system
    Vector3f minBounds = manager->getMinBounds();
    EXPECT_FLOAT_EQ(minBounds.x, -2.0f);  // -4/2
    EXPECT_FLOAT_EQ(minBounds.y, 0.0f);    // Y starts at 0
    EXPECT_FLOAT_EQ(minBounds.z, -4.0f);  // -8/2
    
    // Test getMaxBounds - centered coordinate system
    Vector3f maxBounds = manager->getMaxBounds();
    EXPECT_FLOAT_EQ(maxBounds.x, 2.0f);   // 4/2
    EXPECT_FLOAT_EQ(maxBounds.y, 6.0f);   // Full Y height
    EXPECT_FLOAT_EQ(maxBounds.z, 4.0f);   // 8/2
    
    // Test getCenter
    Vector3f center = manager->getCenter();
    EXPECT_FLOAT_EQ(center.x, 0.0f);   // Centered at origin
    EXPECT_FLOAT_EQ(center.y, 3.0f);   // 6 / 2
    EXPECT_FLOAT_EQ(center.z, 0.0f);   // Centered at origin
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
    updateEventTracking();
    EXPECT_EQ(resizeEventCount, 1);
    
    // Remove event dispatcher
    manager->setEventDispatcher(nullptr);
    
    // Change size without dispatcher - no events should be dispatched
    int previousEventCount = resizeEventCount;
    manager->setSize(Vector3f(4.0f, 4.0f, 4.0f));
    updateEventTracking();
    EXPECT_EQ(resizeEventCount, previousEventCount);
    
    // Set dispatcher back
    manager->setEventDispatcher(eventDispatcher.get());
    
    // Change size with dispatcher - events should be dispatched again
    manager->setSize(Vector3f(6.0f, 6.0f, 6.0f));
    updateEventTracking();
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
        
        // Update event tracking before checking
        updateEventTracking();
        
        // Verify event was dispatched correctly
        EXPECT_EQ(lastOldSize, previousSize);
        EXPECT_EQ(lastNewSize, targetSize);
    }
    
    updateEventTracking();
    EXPECT_EQ(resizeEventCount, sizes.size());
}

// REQ-2.1.4: No voxels shall be placed below Y=0
TEST_F(WorkspaceManagerTest, EdgeCaseBounds) {
    // Test workspace at minimum size
    manager->setSize(Vector3f(2.0f, 2.0f, 2.0f));
    // X: -1 to 1, Y: 0 to 2, Z: -1 to 1
    
    EXPECT_TRUE(manager->isPositionValid(Vector3f(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(manager->isPositionValid(Vector3f(1.0f, 2.0f, 1.0f))); // Exactly at max
    EXPECT_TRUE(manager->isPositionValid(Vector3f(-1.0f, 0.0f, -1.0f))); // Exactly at min
    EXPECT_FALSE(manager->isPositionValid(Vector3f(1.1f, 1.0f, 0.0f))); // X too large
    EXPECT_FALSE(manager->isPositionValid(Vector3f(-1.1f, 1.0f, 0.0f))); // X too small
    
    // Test workspace at maximum size
    manager->setSize(Vector3f(8.0f, 8.0f, 8.0f));
    // X: -4 to 4, Y: 0 to 8, Z: -4 to 4
    
    EXPECT_TRUE(manager->isPositionValid(Vector3f(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(manager->isPositionValid(Vector3f(4.0f, 8.0f, 4.0f))); // Exactly at max
    EXPECT_TRUE(manager->isPositionValid(Vector3f(-4.0f, 0.0f, -4.0f))); // Exactly at min
    EXPECT_FALSE(manager->isPositionValid(Vector3f(4.1f, 4.0f, 0.0f))); // X too large
    EXPECT_FALSE(manager->isPositionValid(Vector3f(-4.1f, 4.0f, 0.0f))); // X too small
}

TEST_F(WorkspaceManagerTest, NonCubicWorkspaces) {
    // Test asymmetric workspace
    manager->setSize(Vector3f(2.0f, 4.0f, 8.0f));
    // X: -1 to 1, Y: 0 to 4, Z: -4 to 4
    
    // Test bounds for each dimension independently
    EXPECT_TRUE(manager->isPositionValid(Vector3f(0.5f, 0.0f, 0.0f)));
    EXPECT_TRUE(manager->isPositionValid(Vector3f(1.0f, 0.0f, 0.0f))); // At max X
    EXPECT_FALSE(manager->isPositionValid(Vector3f(1.1f, 0.0f, 0.0f))); // Beyond max X
    
    EXPECT_TRUE(manager->isPositionValid(Vector3f(0.0f, 2.0f, 0.0f)));
    EXPECT_TRUE(manager->isPositionValid(Vector3f(0.0f, 4.0f, 0.0f))); // At max Y
    EXPECT_FALSE(manager->isPositionValid(Vector3f(0.0f, 4.1f, 0.0f))); // Beyond max Y
    
    EXPECT_TRUE(manager->isPositionValid(Vector3f(0.0f, 0.0f, 2.0f)));
    EXPECT_TRUE(manager->isPositionValid(Vector3f(0.0f, 0.0f, 4.0f))); // At max Z
    EXPECT_FALSE(manager->isPositionValid(Vector3f(0.0f, 0.0f, 4.1f))); // Beyond max Z
    
    // Test corner cases
    EXPECT_TRUE(manager->isPositionValid(Vector3f(0.5f, 2.0f, 2.0f)));
    EXPECT_TRUE(manager->isPositionValid(Vector3f(1.0f, 4.0f, 4.0f))); // At max bounds
    EXPECT_TRUE(manager->isPositionValid(Vector3f(-1.0f, 0.0f, -4.0f))); // At min bounds (X,Z can be negative)
    EXPECT_FALSE(manager->isPositionValid(Vector3f(0.0f, -0.1f, 0.0f))); // Y negative not allowed
    EXPECT_FALSE(manager->isPositionValid(Vector3f(1.1f, 4.1f, 4.1f))); // Beyond max bounds
}

TEST_F(WorkspaceManagerTest, ConstMethodsWithConstManager) {
    const WorkspaceManager& constManager = *manager;
    
    // Test that const methods work correctly
    Vector3f size = constManager.getSize();
    EXPECT_GT(size.x, 0.0f);
    EXPECT_GT(size.y, 0.0f);
    EXPECT_GT(size.z, 0.0f);
    
    EXPECT_TRUE(constManager.isPositionValid(Vector3f(0.0f, 0.0f, 0.0f)));
    
    Vector3f minBounds = constManager.getMinBounds();
    Vector3f maxBounds = constManager.getMaxBounds();
    EXPECT_LT(minBounds.x, maxBounds.x);
    EXPECT_LT(minBounds.y, maxBounds.y);
    EXPECT_LT(minBounds.z, maxBounds.z);
    
    Vector3f clamped = constManager.clampPosition(Vector3f(100.0f, 100.0f, 100.0f));
    EXPECT_TRUE(constManager.isPositionValid(clamped));
}

// New tests for coordinate type integration
TEST_F(WorkspaceManagerTest, WorldCoordinatesIntegration) {
    manager->setSize(Vector3f(4.0f, 6.0f, 8.0f));
    
    // Test WorldCoordinates position validation
    WorldCoordinates validPos(0.0f, 2.0f, 0.0f);
    EXPECT_TRUE(manager->isPositionValid(validPos));
    
    WorldCoordinates invalidPos(5.0f, 7.0f, 10.0f);
    EXPECT_FALSE(manager->isPositionValid(invalidPos));
    
    // Test WorldCoordinates clamping
    WorldCoordinates outOfBounds(5.0f, 7.0f, 10.0f);
    WorldCoordinates clamped = manager->clampPosition(outOfBounds);
    EXPECT_TRUE(manager->isPositionValid(clamped));
    
    // Verify clamped values
    EXPECT_FLOAT_EQ(clamped.x(), 2.0f);   // 4/2 = 2
    EXPECT_FLOAT_EQ(clamped.y(), 6.0f);   // max Y
    EXPECT_FLOAT_EQ(clamped.z(), 4.0f);   // 8/2 = 4
}

TEST_F(WorkspaceManagerTest, WorldCoordinatesBounds) {
    manager->setSize(Vector3f(5.0f, 8.0f, 5.0f));
    
    // Test new WorldCoordinates bounds methods
    WorldCoordinates minBounds = manager->getMinBoundsWorld();
    WorldCoordinates maxBounds = manager->getMaxBoundsWorld();
    WorldCoordinates center = manager->getCenterWorld();
    
    // Check min bounds (-2.5, 0, -2.5) - Y starts at 0
    EXPECT_FLOAT_EQ(minBounds.x(), -2.5f);
    EXPECT_FLOAT_EQ(minBounds.y(), 0.0f);
    EXPECT_FLOAT_EQ(minBounds.z(), -2.5f);
    
    // Check max bounds (2.5, 8, 2.5)
    EXPECT_FLOAT_EQ(maxBounds.x(), 2.5f);
    EXPECT_FLOAT_EQ(maxBounds.y(), 8.0f);
    EXPECT_FLOAT_EQ(maxBounds.z(), 2.5f);
    
    // Check center (0, 4, 0) - Y center is at size/2
    EXPECT_FLOAT_EQ(center.x(), 0.0f);
    EXPECT_FLOAT_EQ(center.y(), 4.0f);
    EXPECT_FLOAT_EQ(center.z(), 0.0f);
    
    // Verify legacy methods still work
    Vector3f legacyMin = manager->getMinBounds();
    Vector3f legacyMax = manager->getMaxBounds();
    Vector3f legacyCenter = manager->getCenter();
    
    EXPECT_EQ(legacyMin, minBounds.value());
    EXPECT_EQ(legacyMax, maxBounds.value());
    EXPECT_EQ(legacyCenter, center.value());
}

TEST_F(WorkspaceManagerTest, IncrementCoordinatesValidation) {
    manager->setSize(Vector3f(4.0f, 4.0f, 4.0f));
    
    // Test grid position validation with new coordinate types
    // For 32cm voxels in 4m workspace: 4.0 / 0.32 = 12.5 -> 13 voxels per axis
    IncrementCoordinates validIncrementPos(5, 5, 5);
    EXPECT_TRUE(manager->isIncrementPositionValid(validIncrementPos));
    
    IncrementCoordinates invalidIncrementPos(1000, 1000, 1000);  // Outside 4m workspace
    EXPECT_FALSE(manager->isIncrementPositionValid(invalidIncrementPos));
    
    // Test with more increment positions
    // All voxel storage uses 1cm increments now, regardless of visual resolution
    IncrementCoordinates validSmallIncrement(100, 100, 100);  // 1m from origin
    EXPECT_TRUE(manager->isIncrementPositionValid(validSmallIncrement));
    
    IncrementCoordinates invalidSmallIncrement(500, 500, 500);  // 5m from origin, outside 4m workspace
    EXPECT_FALSE(manager->isIncrementPositionValid(invalidSmallIncrement));
}

TEST_F(WorkspaceManagerTest, CoordinateTypeBackwardCompatibility) {
    manager->setSize(Vector3f(4.0f, 6.0f, 8.0f));
    
    // Test that legacy Vector3f methods still work
    Vector3f legacyPos(1.0f, 2.0f, 3.0f);
    bool legacyValid = manager->isPositionValid(legacyPos);
    
    // Test equivalent WorldCoordinates method
    WorldCoordinates newPos(1.0f, 2.0f, 3.0f);
    bool newValid = manager->isPositionValid(newPos);
    
    EXPECT_EQ(legacyValid, newValid);
    
    // Test legacy clamping vs new clamping
    Vector3f outOfBounds(10.0f, 10.0f, 10.0f);
    Vector3f legacyClamped = manager->clampPosition(outOfBounds);
    WorldCoordinates newClamped = manager->clampPosition(WorldCoordinates(outOfBounds));
    
    EXPECT_EQ(legacyClamped, newClamped.value());
    
    // Test that both methods validate the same
    EXPECT_TRUE(manager->isPositionValid(legacyClamped));
    EXPECT_TRUE(manager->isPositionValid(newClamped));
}

TEST_F(WorkspaceManagerTest, CoordinateConverterIntegration) {
    manager->setSize(Vector3f(5.0f, 5.0f, 5.0f));
    
    // Test integration with CoordinateConverter
    WorldCoordinates worldPos(1.0f, 2.0f, 1.5f);
    
    // Verify position is valid in workspace
    EXPECT_TRUE(manager->isPositionValid(worldPos));
    
    // Convert to increment coordinates and back
    IncrementCoordinates incCoord = CoordinateConverter::worldToIncrement(worldPos);
    WorldCoordinates convertedBack = CoordinateConverter::incrementToWorld(incCoord);
    
    // Should be very close (within floating point precision)
    EXPECT_NEAR(worldPos.x(), convertedBack.x(), 0.01f);
    EXPECT_NEAR(worldPos.y(), convertedBack.y(), 0.01f);
    EXPECT_NEAR(worldPos.z(), convertedBack.z(), 0.01f);
    
    // Both should be valid in workspace
    EXPECT_TRUE(manager->isPositionValid(worldPos));
    EXPECT_TRUE(manager->isPositionValid(convertedBack));
}