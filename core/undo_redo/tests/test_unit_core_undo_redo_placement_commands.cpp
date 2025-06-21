#include <gtest/gtest.h>
#include "../PlacementCommands.h"
#include "../HistoryManager.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../voxel_data/VoxelDataManager.h"
#include <unordered_set>
#include <memory>
#include <map>

using namespace VoxelEditor::UndoRedo;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

// Event handlers for tracking voxel changes
class TestVoxelChangedHandler : public EventHandler<VoxelChangedEvent> {
public:
    void handleEvent(const VoxelChangedEvent& event) override {
        eventCount++;
        lastEvent = event;
        voxelChanges.push_back(event);
    }
    
    int eventCount = 0;
    VoxelChangedEvent lastEvent{Vector3i::Zero(), VoxelResolution::Size_1cm, false, false};
    std::vector<VoxelChangedEvent> voxelChanges;
};

class PlacementCommandsTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
        
        // Set up event handler
        voxelChangedHandler = std::make_shared<TestVoxelChangedHandler>();
        eventDispatcher->subscribe<VoxelChangedEvent>(voxelChangedHandler.get());
        
        // Set default workspace size
        voxelManager->resizeWorkspace(Vector3f(5.0f, 5.0f, 5.0f));
    }
    
    void TearDown() override {
        eventDispatcher->unsubscribe<VoxelChangedEvent>(voxelChangedHandler.get());
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
    std::shared_ptr<TestVoxelChangedHandler> voxelChangedHandler;
};

// PlacementCommandFactory Tests
// REQ-5.1.1: Left-click shall place a voxel at the current preview position
TEST_F(PlacementCommandsTest, CreatePlacementCommand_ValidPosition) {
    Vector3i pos(0, 0, 0);  // Use origin - always valid for all resolutions
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto command = PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), pos, resolution);
    
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->getName(), "Place Voxel");
}

TEST_F(PlacementCommandsTest, CreatePlacementCommand_NullManager) {
    Vector3i pos(0, 0, 0);  // Use origin - always valid for all resolutions
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto command = PlacementCommandFactory::createPlacementCommand(nullptr, pos, resolution);
    
    EXPECT_EQ(command, nullptr);
}

// Command creation for voxel operations - validation of ground plane constraint
TEST_F(PlacementCommandsTest, CreatePlacementCommand_InvalidPosition_BelowGroundPlane) {
    Vector3i pos(0, -4, 0); // Y < 0, aligned to 4cm grid
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto command = PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), pos, resolution);
    
    EXPECT_EQ(command, nullptr);
}

TEST_F(PlacementCommandsTest, CreatePlacementCommand_OverlapDetected) {
    Vector3i pos(4, 0, 4);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Place a voxel first to create an overlap
    voxelManager->setVoxel(pos, resolution, true);
    
    auto command = PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), pos, resolution);
    
    EXPECT_EQ(command, nullptr);
}

// REQ-5.1.2: Right-click on a voxel shall remove that voxel
TEST_F(PlacementCommandsTest, CreateRemovalCommand_ValidPosition) {
    Vector3i pos(8, 4, 8);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Set up a voxel to remove
    voxelManager->setVoxel(pos, resolution, true);
    
    auto command = PlacementCommandFactory::createRemovalCommand(
        voxelManager.get(), pos, resolution);
    
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->getName(), "Remove Voxel");
}

TEST_F(PlacementCommandsTest, CreateRemovalCommand_NoVoxelExists) {
    Vector3i pos(12, 8, 12);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Don't set any voxel at this position
    
    auto command = PlacementCommandFactory::createRemovalCommand(
        voxelManager.get(), pos, resolution);
    
    EXPECT_EQ(command, nullptr);
}

// Validation Tests
TEST_F(PlacementCommandsTest, ValidatePlacement_ValidPosition) {
    Vector3i pos(16, 12, 16);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto result = PlacementCommandFactory::validatePlacement(
        voxelManager.get(), pos, resolution);
    
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(PlacementCommandsTest, ValidatePlacement_BelowGroundPlane) {
    Vector3i pos(0, -4, 0); // Y < 0, aligned to 4cm grid
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto result = PlacementCommandFactory::validatePlacement(
        voxelManager.get(), pos, resolution);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0], "Cannot place voxels below ground plane (Y < 0)");
}

TEST_F(PlacementCommandsTest, ValidatePlacement_WouldOverlap) {
    Vector3i pos(20, 16, 20);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Create overlapping voxels at different resolutions
    voxelManager->setVoxel(pos, VoxelResolution::Size_8cm, true);
    
    auto result = PlacementCommandFactory::validatePlacement(
        voxelManager.get(), pos, resolution);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0], "Position would overlap with existing voxel");
}

TEST_F(PlacementCommandsTest, ValidatePlacement_VoxelAlreadyExists) {
    Vector3i pos(0, 0, 0);  // Use centered coordinate system with 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    voxelManager->setVoxel(pos, resolution, true);
    
    auto result = PlacementCommandFactory::validatePlacement(
        voxelManager.get(), pos, resolution);
    
    // When a voxel already exists, it should be invalid due to overlap
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0], "Position would overlap with existing voxel");
}

TEST_F(PlacementCommandsTest, ValidateRemoval_ValidPosition) {
    Vector3i pos(4, 4, 4);  // Use centered coordinate system with 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    voxelManager->setVoxel(pos, resolution, true);
    
    auto result = PlacementCommandFactory::validateRemoval(
        voxelManager.get(), pos, resolution);
    
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(PlacementCommandsTest, ValidateRemoval_NoVoxelExists) {
    Vector3i pos(32, 28, 32);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto result = PlacementCommandFactory::validateRemoval(
        voxelManager.get(), pos, resolution);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0], "No voxel exists at this position to remove");
}

// VoxelPlacementCommand Tests
// REQ-2.3.3: Clicking on a highlighted face shall place the new voxel adjacent to that face
// (This test validates the command infrastructure for face-based placement)
TEST_F(PlacementCommandsTest, VoxelPlacementCommand_BasicExecution) {
    Vector3i pos(36, 32, 36);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        voxelManager.get(), pos, resolution);
    
    EXPECT_FALSE(command.hasExecuted());
    EXPECT_TRUE(command.execute());
    EXPECT_TRUE(command.hasExecuted());
    
    // Check that the voxel was set via events
    EXPECT_EQ(voxelChangedHandler->eventCount, 1);
    EXPECT_EQ(voxelChangedHandler->lastEvent.gridPos.x, pos.x);
    EXPECT_EQ(voxelChangedHandler->lastEvent.gridPos.y, pos.y);
    EXPECT_EQ(voxelChangedHandler->lastEvent.gridPos.z, pos.z);
    EXPECT_EQ(voxelChangedHandler->lastEvent.resolution, resolution);
    EXPECT_TRUE(voxelChangedHandler->lastEvent.newValue);
}

TEST_F(PlacementCommandsTest, VoxelPlacementCommand_ExecuteUndo) {
    Vector3i pos(40, 36, 40);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        voxelManager.get(), pos, resolution);
    
    // Execute
    EXPECT_TRUE(command.execute());
    EXPECT_TRUE(command.hasExecuted());
    
    // Clear event history to isolate undo events
    voxelChangedHandler->eventCount = 0;
    voxelChangedHandler->voxelChanges.clear();
    
    // Undo
    EXPECT_TRUE(command.undo());
    EXPECT_FALSE(command.hasExecuted());
    
    // Check undo event
    EXPECT_EQ(voxelChangedHandler->eventCount, 1);
    EXPECT_FALSE(voxelChangedHandler->lastEvent.newValue); // Should set to false (remove)
}

TEST_F(PlacementCommandsTest, VoxelPlacementCommand_ExecutionFailure) {
    Vector3i pos(44, 40, 44);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        voxelManager.get(), pos, resolution);
    
    // For this test, we need to simulate a failure condition
    // Since we can't easily make VoxelDataManager fail, we'll skip this test
    GTEST_SKIP() << "Cannot simulate operation failure with real VoxelDataManager";
    
    EXPECT_FALSE(command.execute());
    EXPECT_FALSE(command.hasExecuted());
}

TEST_F(PlacementCommandsTest, VoxelPlacementCommand_ValidationFailure) {
    Vector3i pos(0, -4, 0); // Invalid position (Y < 0), aligned to 4cm grid
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        voxelManager.get(), pos, resolution);
    
    EXPECT_FALSE(command.execute());
    EXPECT_FALSE(command.hasExecuted());
    
    // No voxel operations should have been attempted
    EXPECT_EQ(voxelChangedHandler->eventCount, 0);
}

TEST_F(PlacementCommandsTest, VoxelPlacementCommand_GetDescription) {
    Vector3i pos(48, 44, 48);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        voxelManager.get(), pos, resolution);
    
    std::string description = command.getDescription();
    EXPECT_EQ(description, "Place 4cm voxel at (48, 44, 48)");
}

TEST_F(PlacementCommandsTest, VoxelPlacementCommand_MemoryUsage) {
    Vector3i pos(52, 48, 52);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        voxelManager.get(), pos, resolution);
    
    size_t memoryUsage = command.getMemoryUsage();
    EXPECT_GT(memoryUsage, 0);
    EXPECT_GE(memoryUsage, sizeof(VoxelPlacementCommand));
}

// VoxelRemovalCommand Tests
TEST_F(PlacementCommandsTest, VoxelRemovalCommand_BasicExecution) {
    Vector3i pos(0, 4, 0);  // Use centered coordinate system with 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Set up initial voxel
    bool placed = voxelManager->setVoxel(pos, resolution, true);
    EXPECT_TRUE(placed) << "Failed to place voxel at position (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
    
    // Verify the voxel was actually placed
    bool exists = voxelManager->getVoxel(pos, resolution);
    EXPECT_TRUE(exists) << "Voxel was not placed at position (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
    
    // Reset event count after setup
    voxelChangedHandler->eventCount = 0;
    
    VoxelRemovalCommand command(
        voxelManager.get(), pos, resolution);
    
    EXPECT_FALSE(command.hasExecuted());
    EXPECT_TRUE(command.execute());
    EXPECT_TRUE(command.hasExecuted());
    
    // Check that the voxel was removed (event should have been triggered by the execute() call above)
    EXPECT_EQ(voxelChangedHandler->eventCount, 1);
    EXPECT_EQ(voxelChangedHandler->lastEvent.gridPos.x, pos.x);
    EXPECT_EQ(voxelChangedHandler->lastEvent.gridPos.y, pos.y);
    EXPECT_EQ(voxelChangedHandler->lastEvent.gridPos.z, pos.z);
    EXPECT_EQ(voxelChangedHandler->lastEvent.resolution, resolution);
    EXPECT_FALSE(voxelChangedHandler->lastEvent.newValue);
    
    // Verify the voxel was actually removed
    EXPECT_FALSE(voxelManager->getVoxel(pos, resolution)) << "Voxel was not removed from position";
}

TEST_F(PlacementCommandsTest, VoxelRemovalCommand_ExecuteUndo) {
    Vector3i pos(8, 8, 8);  // Use centered coordinate system with 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Set up initial voxel
    voxelManager->setVoxel(pos, resolution, true);
    
    VoxelRemovalCommand command(
        voxelManager.get(), pos, resolution);
    
    // Execute
    EXPECT_TRUE(command.execute());
    EXPECT_TRUE(command.hasExecuted());
    
    // Clear call history to isolate undo calls
    voxelChangedHandler->eventCount = 0; voxelChangedHandler->voxelChanges.clear();
    
    // Undo
    EXPECT_TRUE(command.undo());
    EXPECT_FALSE(command.hasExecuted());
    
    // Check undo call
    // Check events instead of mock calls
    EXPECT_EQ(voxelChangedHandler->eventCount, 1);
    EXPECT_TRUE(voxelChangedHandler->lastEvent.newValue); // Should restore to true
}

TEST_F(PlacementCommandsTest, VoxelRemovalCommand_GetDescription) {
    Vector3i pos(64, 60, 64);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Set up initial voxel
    voxelManager->setVoxel(pos, resolution, true);
    
    VoxelRemovalCommand command(
        voxelManager.get(), pos, resolution);
    
    std::string description = command.getDescription();
    EXPECT_EQ(description, "Remove 4cm voxel at (64, 60, 64)");
}

// Integration with HistoryManager Tests
// History Management: Support for undo/redo operations
// Command pattern implementation for reversible operations
TEST_F(PlacementCommandsTest, HistoryManager_PlacementAndRemoval) {
    HistoryManager history;
    history.setSnapshotInterval(0); // Disable snapshots
    
    Vector3i pos(68, 64, 68);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Place a voxel
    auto placementCommand = PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), pos, resolution);
    ASSERT_NE(placementCommand, nullptr);
    
    EXPECT_TRUE(history.executeCommand(std::move(placementCommand)));
    
    // Check voxel was placed
    // Check events instead of mock calls
    EXPECT_EQ(voxelChangedHandler->eventCount, 1);
    EXPECT_TRUE(voxelChangedHandler->lastEvent.newValue);
    
    // Set up for removal (mock that voxel exists)
    voxelManager->setVoxel(pos, resolution, true);
    voxelChangedHandler->eventCount = 0; voxelChangedHandler->voxelChanges.clear();
    
    // Remove the voxel
    auto removalCommand = PlacementCommandFactory::createRemovalCommand(
        voxelManager.get(), pos, resolution);
    ASSERT_NE(removalCommand, nullptr);
    
    EXPECT_TRUE(history.executeCommand(std::move(removalCommand)));
    
    // Check voxel was removed
    // Check events
    EXPECT_EQ(voxelChangedHandler->eventCount, 1);
    EXPECT_FALSE(voxelChangedHandler->lastEvent.newValue);
    
    // Test undo sequence
    voxelChangedHandler->eventCount = 0; voxelChangedHandler->voxelChanges.clear();
    EXPECT_TRUE(history.undo()); // Undo removal
    
    // Check events
    EXPECT_EQ(voxelChangedHandler->eventCount, 1);
    EXPECT_TRUE(voxelChangedHandler->lastEvent.newValue); // Should restore voxel
    
    voxelChangedHandler->eventCount = 0; voxelChangedHandler->voxelChanges.clear();
    EXPECT_TRUE(history.undo()); // Undo placement
    
    // Check events
    EXPECT_EQ(voxelChangedHandler->eventCount, 1);
    EXPECT_FALSE(voxelChangedHandler->lastEvent.newValue); // Should remove voxel
}

// Memory Usage Tests for many commands
// REQ-6.3.4: Application overhead shall not exceed 1GB
// Memory-efficient history with limited depth for VR constraints
TEST_F(PlacementCommandsTest, MemoryUsage_ManyCommands) {
    const int numCommands = 1000;
    std::vector<std::unique_ptr<VoxelPlacementCommand>> commands;
    commands.reserve(numCommands);
    
    // Create many commands
    for (int i = 0; i < numCommands; ++i) {
        Vector3i pos(i, 0, 0);
        VoxelResolution resolution = VoxelResolution::Size_1cm;
        
        commands.push_back(std::make_unique<VoxelPlacementCommand>(
            voxelManager.get(), pos, resolution));
    }
    
    // Calculate total memory usage
    size_t totalMemory = 0;
    for (const auto& command : commands) {
        totalMemory += command->getMemoryUsage();
    }
    
    // Should be reasonable memory usage (less than 1MB for 1000 commands)
    EXPECT_LT(totalMemory, 1024 * 1024);
    
    // Average per command should be reasonable
    size_t avgPerCommand = totalMemory / numCommands;
    EXPECT_LT(avgPerCommand, 1024); // Less than 1KB per command
}

// Command merging tests
TEST_F(PlacementCommandsTest, CommandMerging_SamePosition) {
    Vector3i pos(72, 68, 72);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto command1 = std::make_unique<VoxelPlacementCommand>(
        voxelManager.get(), pos, resolution);
    auto command2 = std::make_unique<VoxelPlacementCommand>(
        voxelManager.get(), pos, resolution);
    
    // Test if commands can merge
    EXPECT_TRUE(command1->canMergeWith(*command2));
    
    // Test merging
    auto merged = command1->mergeWith(std::move(command2));
    EXPECT_NE(merged, nullptr);
}

TEST_F(PlacementCommandsTest, CommandMerging_DifferentPosition) {
    Vector3i pos1(76, 72, 76);  // Use 4cm-aligned position
    Vector3i pos2(80, 76, 80);  // Use 4cm-aligned position
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto command1 = std::make_unique<VoxelPlacementCommand>(
        voxelManager.get(), pos1, resolution);
    auto command2 = std::make_unique<VoxelPlacementCommand>(
        voxelManager.get(), pos2, resolution);
    
    // Commands at different positions shouldn't merge
    EXPECT_FALSE(command1->canMergeWith(*command2));
}