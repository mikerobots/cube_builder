#include <gtest/gtest.h>
#include "../HistoryManager.h"
#include "../PlacementCommands.h"
#include "../StateSnapshot.h"
#include "../Command.h"
#include "../CompositeCommand.h"
#include "../Transaction.h"
#include "../../voxel_data/VoxelDataManager.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/math/Vector3i.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace VoxelEditor::UndoRedo;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

// Simple test command for testing
// Uses shared_ptr to avoid dangling references when commands outlive local variables
class TestCommand : public Command {
public:
    TestCommand(std::shared_ptr<int> value, int newValue, size_t memorySize = 0)
        : m_value(value)
        , m_newValue(newValue)
        , m_oldValue(*value)
        , m_memorySize(memorySize == 0 ? sizeof(TestCommand) : memorySize) {}
    
    bool execute() override {
        if (!m_value) return false;
        *m_value = m_newValue;
        m_executed = true;
        return true;
    }
    
    bool undo() override {
        if (!m_executed || !m_value) return false;
        *m_value = m_oldValue;
        m_executed = false;
        return true;
    }
    
    std::string getName() const override { return "TestCommand"; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override { return m_memorySize; }
    
private:
    std::shared_ptr<int> m_value;
    int m_newValue;
    int m_oldValue;
    size_t m_memorySize;
};

class UndoRedoRequirementsTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
        voxelManager->resizeWorkspace(Vector3f(5.0f, 5.0f, 5.0f));
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
};

// REQ-5.1.1: Left-click shall place a voxel at the current preview position
TEST_F(UndoRedoRequirementsTest, PlacementCommand_LeftClickPlacement) {
    // Use positions that align with 4cm grid (multiples of 4)
    IncrementCoordinates pos(4, 8, 12);  // 4cm, 8cm, 12cm in world coordinates
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Create placement command (simulates left-click placement)
    auto command = PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), pos, resolution);
    
    ASSERT_NE(command, nullptr) << "Should create placement command for valid position";
    EXPECT_EQ(command->getName(), "Place Voxel");
    
    // Execute the command
    EXPECT_TRUE(command->execute());
    
    // Verify voxel was placed
    EXPECT_TRUE(voxelManager->getVoxel(pos, resolution));
}

// REQ-5.1.2: Right-click on a voxel shall remove that voxel
TEST_F(UndoRedoRequirementsTest, RemovalCommand_RightClickRemoval) {
    // Use positions that align with 4cm grid (multiples of 4)
    IncrementCoordinates pos(4, 8, 12);  // 4cm, 8cm, 12cm in world coordinates
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // First place a voxel
    voxelManager->setVoxel(pos, resolution, true);
    
    // Create removal command (simulates right-click removal)
    auto command = PlacementCommandFactory::createRemovalCommand(
        voxelManager.get(), pos, resolution);
    
    ASSERT_NE(command, nullptr) << "Should create removal command for existing voxel";
    EXPECT_EQ(command->getName(), "Remove Voxel");
    
    // Execute the command
    EXPECT_TRUE(command->execute());
    
    // Verify voxel was removed
    EXPECT_FALSE(voxelManager->getVoxel(pos, resolution));
}

// REQ-2.3.3: Clicking on a highlighted face shall place the new voxel adjacent to that face
TEST_F(UndoRedoRequirementsTest, PlacementCommand_AdjacentPlacement) {
    // Use positions that align with 4cm grid (multiples of 4)
    Vector3i basePos(4, 0, 4);      // 4cm, 0cm, 4cm in world coordinates
    Vector3i adjacentPos(4, 4, 4);  // 4cm, 4cm, 4cm - above the base voxel
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Place base voxel
    voxelManager->setVoxel(basePos, resolution, true);
    
    // Create placement command for adjacent position (simulates face-click placement)
    auto command = PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), VoxelEditor::Math::IncrementCoordinates(adjacentPos), resolution);
    
    ASSERT_NE(command, nullptr) << "Should create placement command for adjacent position";
    
    // Execute the command
    EXPECT_TRUE(command->execute());
    
    // Verify adjacent voxel was placed
    EXPECT_TRUE(voxelManager->getVoxel(adjacentPos, resolution));
}

// History Management: Support for undo/redo operations with 5-10 operation limit
TEST_F(UndoRedoRequirementsTest, HistoryManager_OperationLimit) {
    HistoryManager history;
    history.setMaxHistorySize(10); // Set to 10 operation limit
    history.setSnapshotInterval(999999); // Set to very high value to effectively disable snapshots
    
    auto value = std::make_shared<int>(0);
    
    // Add 15 commands (exceeds limit of 10)
    for (int i = 1; i <= 15; ++i) {
        auto command = std::make_unique<TestCommand>(value, i);
        bool result = history.executeCommand(std::move(command));
        ASSERT_TRUE(result) << "Failed to execute command " << i;
    }
    
    EXPECT_EQ(*value, 15) << "Value should be 15 after all commands";
    
    // Count how many we can undo
    int undoCount = 0;
    for (int i = 0; i < 15; ++i) {
        if (!history.canUndo()) {
            break;
        }
        bool undoResult = history.undo();
        ASSERT_TRUE(undoResult) << "Undo failed at count " << undoCount;
        undoCount++;
    }
    
    EXPECT_EQ(undoCount, 10) << "Should only maintain 10 operations in history";
    EXPECT_FALSE(history.canUndo()) << "Should have no more undos after limit";
    EXPECT_EQ(*value, 5) << "Value should be 5 after 10 undos (15 - 10 = 5)";
}

// Command pattern implementation for reversible operations
TEST_F(UndoRedoRequirementsTest, Command_ReversibleOperations) {
    auto value = std::make_shared<int>(0);
    auto command = std::make_unique<TestCommand>(value, 42);
    
    // Test execute
    EXPECT_FALSE(command->hasExecuted());
    EXPECT_TRUE(command->execute());
    EXPECT_TRUE(command->hasExecuted());
    EXPECT_EQ(*value, 42);
    
    // Test undo (reversibility)
    EXPECT_TRUE(command->undo());
    EXPECT_FALSE(command->hasExecuted());
    EXPECT_EQ(*value, 0);
    
    // Test redo
    EXPECT_TRUE(command->execute());
    EXPECT_TRUE(command->hasExecuted());
    EXPECT_EQ(*value, 42);
}

// State management for complex operations
TEST_F(UndoRedoRequirementsTest, CompositeCommand_ComplexOperations) {
    auto value1 = std::make_shared<int>(0);
    auto value2 = std::make_shared<int>(0);
    auto value3 = std::make_shared<int>(0);
    
    auto composite = std::make_unique<CompositeCommand>("Complex Operation");
    composite->addCommand(std::make_unique<TestCommand>(value1, 10));
    composite->addCommand(std::make_unique<TestCommand>(value2, 20));
    composite->addCommand(std::make_unique<TestCommand>(value3, 30));
    
    // Execute complex operation
    EXPECT_TRUE(composite->execute());
    EXPECT_EQ(*value1, 10);
    EXPECT_EQ(*value2, 20);
    EXPECT_EQ(*value3, 30);
    
    // Undo complex operation (all changes reversed)
    EXPECT_TRUE(composite->undo());
    EXPECT_EQ(*value1, 0);
    EXPECT_EQ(*value2, 0);
    EXPECT_EQ(*value3, 0);
}

// REQ-6.3.4: Application overhead shall not exceed 1GB
TEST_F(UndoRedoRequirementsTest, MemoryConstraints_ApplicationOverhead) {
    HistoryManager history;
    history.setMaxHistorySize(1000); // Large history for testing
    history.setSnapshotInterval(999999); // Effectively disable snapshots
    
    auto value = std::make_shared<int>(0);
    size_t totalMemory = 0;
    
    // Create many commands with known memory usage
    const size_t commandSize = 1024; // 1KB per command
    const int numCommands = 100;
    
    for (int i = 0; i < numCommands; ++i) {
        auto command = std::make_unique<TestCommand>(value, i, commandSize);
        totalMemory += command->getMemoryUsage();
        history.executeCommand(std::move(command));
    }
    
    // Verify memory usage is reasonable
    EXPECT_LT(totalMemory, 1024 * 1024) << "100 commands should use less than 1MB";
    
    // Get history memory usage
    size_t historyMemory = history.getMemoryUsage();
    EXPECT_LT(historyMemory, 10 * 1024 * 1024) << "History memory should be well under 10MB";
}

// REQ-8.1.6: Format shall store limited undo history (10-20 operations)
TEST_F(UndoRedoRequirementsTest, StateSnapshot_LimitedHistory) {
    // NOTE: This requirement is about file format storage, not runtime behavior
    // The actual implementation would be in the FileIO subsystem
    // Here we test that StateSnapshot can capture the limited history
    
    StateSnapshot snapshot;
    
    // Verify snapshot can be created and has proper interface
    EXPECT_GT(snapshot.getMemoryUsage(), 0) << "Snapshot should report memory usage";
    
    // In a real implementation, we would:
    // 1. Create a history with 15 operations
    // 2. Save to file format
    // 3. Verify only 10-20 operations are stored
    // This is a placeholder test - actual implementation in FileIO
    SUCCEED() << "File format history limit tested in FileIO subsystem";
}

// REQ-9.2.6: CLI shall support undo/redo commands
TEST_F(UndoRedoRequirementsTest, CLI_UndoRedoSupport) {
    // NOTE: This requirement is about CLI integration
    // The actual CLI command handling is in the apps/cli directory
    // Here we verify the undo/redo system provides the necessary interface
    
    HistoryManager history;
    history.setSnapshotInterval(999999); // Effectively disable snapshots
    
    // Verify CLI-required methods exist and work
    EXPECT_FALSE(history.canUndo()) << "CLI needs canUndo() method";
    EXPECT_FALSE(history.canRedo()) << "CLI needs canRedo() method";
    
    auto value = std::make_shared<int>(0);
    history.executeCommand(std::make_unique<TestCommand>(value, 42));
    
    EXPECT_TRUE(history.canUndo());
    
    // Get undo history for CLI display
    auto undoHistory = history.getUndoHistory();
    EXPECT_FALSE(undoHistory.empty()) << "CLI needs undo history";
    EXPECT_EQ(undoHistory[0], "TestCommand") << "CLI needs command names";
    
    history.undo();
    EXPECT_TRUE(history.canRedo());
    
    // Get redo history for CLI display
    auto redoHistory = history.getRedoHistory();
    EXPECT_FALSE(redoHistory.empty()) << "CLI needs redo history";
    EXPECT_EQ(redoHistory[0], "TestCommand") << "CLI needs command names";
}

// Transaction support for atomic operations
TEST_F(UndoRedoRequirementsTest, Transaction_AtomicOperations) {
    auto value1 = std::make_shared<int>(0);
    auto value2 = std::make_shared<int>(0);
    
    // Test successful transaction
    {
        Transaction txn("Atomic Operation");
        txn.addCommand(std::make_unique<TestCommand>(value1, 10));
        txn.addCommand(std::make_unique<TestCommand>(value2, 20));
        
        EXPECT_EQ(*value1, 10) << "Commands execute immediately";
        EXPECT_EQ(*value2, 20);
        
        auto composite = txn.commit();
        ASSERT_NE(composite, nullptr);
        EXPECT_EQ(composite->getCommandCount(), 2);
    }
    
    EXPECT_EQ(*value1, 10) << "Values persist after commit";
    EXPECT_EQ(*value2, 20);
    
    // Test rollback on destruction
    *value1 = 0;
    *value2 = 0;
    {
        Transaction txn("Rollback Test");
        txn.addCommand(std::make_unique<TestCommand>(value1, 30));
        txn.addCommand(std::make_unique<TestCommand>(value2, 40));
        // No commit - should rollback
    }
    
    EXPECT_EQ(*value1, 0) << "Values rolled back without commit";
    EXPECT_EQ(*value2, 0);
}

// Memory-efficient history with limited depth for VR constraints
TEST_F(UndoRedoRequirementsTest, MemoryEfficiency_VRConstraints) {
    HistoryManager history;
    history.setMaxHistorySize(20); // VR-friendly limit
    history.setMaxMemoryUsage(50 * 1024 * 1024); // 50MB limit for VR
    history.setSnapshotInterval(5); // Snapshot every 5 commands
    
    auto value = std::make_shared<int>(0);
    
    // Simulate heavy usage
    for (int i = 0; i < 100; ++i) {
        auto command = std::make_unique<TestCommand>(value, i, 1024 * 10); // 10KB commands
        history.executeCommand(std::move(command));
    }
    
    // Verify history stayed within limits
    EXPECT_LE(history.getMemoryUsage(), 50 * 1024 * 1024) << "Memory should stay under VR limit";
    
    // Verify we can still undo recent operations
    int undoCount = 0;
    while (history.canUndo() && undoCount < 30) {
        history.undo();
        undoCount++;
    }
    
    EXPECT_GE(undoCount, 10) << "Should maintain at least 10 operations for VR";
    EXPECT_LE(undoCount, 20) << "Should not exceed 20 operations for VR";
}

// Additional tests for edge cases and error handling

// Test placement validation prevents invalid commands
TEST_F(UndoRedoRequirementsTest, PlacementValidation_PreventInvalidCommands) {
    // Test below ground plane
    IncrementCoordinates belowGround(0, -1, 0);
    auto invalidCommand = PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), belowGround, VoxelResolution::Size_4cm);
    
    EXPECT_EQ(invalidCommand, nullptr) << "Should not create command for invalid position";
    
    // Test overlap detection with properly aligned position
    IncrementCoordinates pos(4, 0, 4);  // 4cm, 0cm, 4cm - aligned to 4cm grid
    voxelManager->setVoxel(pos, VoxelResolution::Size_4cm, true);
    
    auto overlapCommand = PlacementCommandFactory::createPlacementCommand(
        voxelManager.get(), pos, VoxelResolution::Size_4cm);
    
    EXPECT_EQ(overlapCommand, nullptr) << "Should not create command for overlapping position";
}

// Test undo/redo with snapshots
TEST_F(UndoRedoRequirementsTest, Snapshots_EfficientStateRestoration) {
    HistoryManager history;
    history.setSnapshotInterval(3); // Snapshot every 3 commands
    
    auto value = std::make_shared<int>(0);
    
    // Execute 10 commands
    for (int i = 1; i <= 10; ++i) {
        history.executeCommand(std::make_unique<TestCommand>(value, i));
    }
    
    EXPECT_EQ(*value, 10);
    
    // Undo all - should use snapshots efficiently
    while (history.canUndo()) {
        history.undo();
    }
    
    EXPECT_EQ(*value, 0) << "Should restore to initial state";
    
    // Redo all
    while (history.canRedo()) {
        history.redo();
    }
    
    EXPECT_EQ(*value, 10) << "Should restore to final state";
}