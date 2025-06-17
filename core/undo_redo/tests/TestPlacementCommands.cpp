#include <gtest/gtest.h>
#include "../PlacementCommands.h"
#include "../HistoryManager.h"
#include "../../foundation/math/Vector3i.h"
#include <unordered_set>
#include <memory>
#include <map>

using namespace VoxelEditor::UndoRedo;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

// Simplified mock VoxelDataManager for testing
class MockVoxelDataManager {
public:
    struct SetVoxelCall {
        Vector3i pos;
        VoxelResolution resolution;
        bool value;
    };

private:
    // Use a simpler key for the map
    using VoxelKey = std::string;
    
    std::string makeKey(const Vector3i& pos, VoxelResolution resolution) const {
        return std::to_string(pos.x) + "," + 
               std::to_string(pos.y) + "," + 
               std::to_string(pos.z) + "," + 
               std::to_string(static_cast<int>(resolution));
    }

public:
    MockVoxelDataManager() = default;
    
    bool setVoxel(const Vector3i& pos, VoxelResolution resolution, bool value) {
        if (m_shouldFailNextOperation) {
            m_shouldFailNextOperation = false;
            return false;
        }
        
        VoxelKey key = makeKey(pos, resolution);
        m_voxels[key] = value;
        m_setVoxelCalls.push_back({pos, resolution, value});
        return true;
    }
    
    bool getVoxel(const Vector3i& pos, VoxelResolution resolution) const {
        VoxelKey key = makeKey(pos, resolution);
        auto it = m_voxels.find(key);
        return it != m_voxels.end() ? it->second : false;
    }
    
    bool isValidIncrementPosition(const Vector3i& pos) const {
        // Y must be >= 0 (no voxels below ground plane)
        return pos.y >= 0;
    }
    
    bool wouldOverlap(const Vector3i& pos, VoxelResolution resolution) const {
        if (m_forceOverlap) return true;
        
        VoxelKey key = makeKey(pos, resolution);
        return m_overlappingPositions.count(key) > 0;
    }
    
    // Test control methods
    void setVoxelAt(const Vector3i& pos, VoxelResolution resolution, bool value) {
        VoxelKey key = makeKey(pos, resolution);
        m_voxels[key] = value;
    }
    
    void setForceOverlap(bool force) { m_forceOverlap = force; }
    void addOverlappingPosition(const Vector3i& pos, VoxelResolution resolution) {
        VoxelKey key = makeKey(pos, resolution);
        m_overlappingPositions.insert(key);
    }
    
    void setShouldFailNextOperation(bool fail) { m_shouldFailNextOperation = fail; }
    
    const std::vector<SetVoxelCall>& getSetVoxelCalls() const { return m_setVoxelCalls; }
    void clearSetVoxelCalls() { m_setVoxelCalls.clear(); }
    
    size_t getVoxelCount() const { return m_voxels.size(); }

private:
    std::map<VoxelKey, bool> m_voxels;
    std::unordered_set<VoxelKey> m_overlappingPositions;
    std::vector<SetVoxelCall> m_setVoxelCalls;
    bool m_forceOverlap = false;
    bool m_shouldFailNextOperation = false;
};

class PlacementCommandsTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_mockManager = std::make_unique<MockVoxelDataManager>();
    }
    
    std::unique_ptr<MockVoxelDataManager> m_mockManager;
};

// PlacementCommandFactory Tests
TEST_F(PlacementCommandsTest, CreatePlacementCommand_ValidPosition) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto command = PlacementCommandFactory::createPlacementCommand(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->getName(), "Place Voxel");
}

TEST_F(PlacementCommandsTest, CreatePlacementCommand_NullManager) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto command = PlacementCommandFactory::createPlacementCommand(nullptr, pos, resolution);
    
    EXPECT_EQ(command, nullptr);
}

TEST_F(PlacementCommandsTest, CreatePlacementCommand_InvalidPosition_BelowGroundPlane) {
    Vector3i pos(1, -1, 3); // Y < 0
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto command = PlacementCommandFactory::createPlacementCommand(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_EQ(command, nullptr);
}

TEST_F(PlacementCommandsTest, CreatePlacementCommand_OverlapDetected) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    m_mockManager->addOverlappingPosition(pos, resolution);
    
    auto command = PlacementCommandFactory::createPlacementCommand(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_EQ(command, nullptr);
}

TEST_F(PlacementCommandsTest, CreateRemovalCommand_ValidPosition) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Set up a voxel to remove
    m_mockManager->setVoxelAt(pos, resolution, true);
    
    auto command = PlacementCommandFactory::createRemovalCommand(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->getName(), "Remove Voxel");
}

TEST_F(PlacementCommandsTest, CreateRemovalCommand_NoVoxelExists) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Don't set any voxel at this position
    
    auto command = PlacementCommandFactory::createRemovalCommand(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_EQ(command, nullptr);
}

// Validation Tests
TEST_F(PlacementCommandsTest, ValidatePlacement_ValidPosition) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto result = PlacementCommandFactory::validatePlacement(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(PlacementCommandsTest, ValidatePlacement_BelowGroundPlane) {
    Vector3i pos(1, -1, 3); // Y < 0
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto result = PlacementCommandFactory::validatePlacement(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0], "Cannot place voxels below ground plane (Y < 0)");
}

TEST_F(PlacementCommandsTest, ValidatePlacement_WouldOverlap) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    m_mockManager->addOverlappingPosition(pos, resolution);
    
    auto result = PlacementCommandFactory::validatePlacement(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0], "Position would overlap with existing voxel");
}

TEST_F(PlacementCommandsTest, ValidatePlacement_VoxelAlreadyExists) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    m_mockManager->setVoxelAt(pos, resolution, true);
    
    auto result = PlacementCommandFactory::validatePlacement(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_TRUE(result.valid); // Still valid, just a warning
    EXPECT_TRUE(result.errors.empty());
    EXPECT_FALSE(result.warnings.empty());
    EXPECT_EQ(result.warnings[0], "Voxel already exists at this position");
}

TEST_F(PlacementCommandsTest, ValidateRemoval_ValidPosition) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    m_mockManager->setVoxelAt(pos, resolution, true);
    
    auto result = PlacementCommandFactory::validateRemoval(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(PlacementCommandsTest, ValidateRemoval_NoVoxelExists) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto result = PlacementCommandFactory::validateRemoval(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0], "No voxel exists at this position to remove");
}

// VoxelPlacementCommand Tests
TEST_F(PlacementCommandsTest, VoxelPlacementCommand_BasicExecution) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_FALSE(command.hasExecuted());
    EXPECT_TRUE(command.execute());
    EXPECT_TRUE(command.hasExecuted());
    
    // Check that the voxel was set
    auto calls = m_mockManager->getSetVoxelCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls[0].pos.x, pos.x);
    EXPECT_EQ(calls[0].pos.y, pos.y);
    EXPECT_EQ(calls[0].pos.z, pos.z);
    EXPECT_EQ(calls[0].resolution, resolution);
    EXPECT_TRUE(calls[0].value);
}

TEST_F(PlacementCommandsTest, VoxelPlacementCommand_ExecuteUndo) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    // Execute
    EXPECT_TRUE(command.execute());
    EXPECT_TRUE(command.hasExecuted());
    
    // Clear call history to isolate undo calls
    m_mockManager->clearSetVoxelCalls();
    
    // Undo
    EXPECT_TRUE(command.undo());
    EXPECT_FALSE(command.hasExecuted());
    
    // Check undo call
    auto calls = m_mockManager->getSetVoxelCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_FALSE(calls[0].value); // Should set to false (remove)
}

TEST_F(PlacementCommandsTest, VoxelPlacementCommand_ExecutionFailure) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    m_mockManager->setShouldFailNextOperation(true);
    
    EXPECT_FALSE(command.execute());
    EXPECT_FALSE(command.hasExecuted());
}

TEST_F(PlacementCommandsTest, VoxelPlacementCommand_ValidationFailure) {
    Vector3i pos(1, -1, 3); // Invalid position (Y < 0)
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_FALSE(command.execute());
    EXPECT_FALSE(command.hasExecuted());
    
    // No voxel operations should have been attempted
    auto calls = m_mockManager->getSetVoxelCalls();
    EXPECT_TRUE(calls.empty());
}

TEST_F(PlacementCommandsTest, VoxelPlacementCommand_GetDescription) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    std::string description = command.getDescription();
    EXPECT_EQ(description, "Place 4cm voxel at (1, 2, 3)");
}

TEST_F(PlacementCommandsTest, VoxelPlacementCommand_MemoryUsage) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    VoxelPlacementCommand command(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    size_t memoryUsage = command.getMemoryUsage();
    EXPECT_GT(memoryUsage, 0);
    EXPECT_GE(memoryUsage, sizeof(VoxelPlacementCommand));
}

// VoxelRemovalCommand Tests
TEST_F(PlacementCommandsTest, VoxelRemovalCommand_BasicExecution) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Set up initial voxel
    m_mockManager->setVoxelAt(pos, resolution, true);
    
    VoxelRemovalCommand command(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    EXPECT_FALSE(command.hasExecuted());
    EXPECT_TRUE(command.execute());
    EXPECT_TRUE(command.hasExecuted());
    
    // Check that the voxel was removed
    auto calls = m_mockManager->getSetVoxelCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls[0].pos.x, pos.x);
    EXPECT_EQ(calls[0].pos.y, pos.y);
    EXPECT_EQ(calls[0].pos.z, pos.z);
    EXPECT_EQ(calls[0].resolution, resolution);
    EXPECT_FALSE(calls[0].value);
}

TEST_F(PlacementCommandsTest, VoxelRemovalCommand_ExecuteUndo) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Set up initial voxel
    m_mockManager->setVoxelAt(pos, resolution, true);
    
    VoxelRemovalCommand command(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    // Execute
    EXPECT_TRUE(command.execute());
    EXPECT_TRUE(command.hasExecuted());
    
    // Clear call history to isolate undo calls
    m_mockManager->clearSetVoxelCalls();
    
    // Undo
    EXPECT_TRUE(command.undo());
    EXPECT_FALSE(command.hasExecuted());
    
    // Check undo call
    auto calls = m_mockManager->getSetVoxelCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_TRUE(calls[0].value); // Should restore to true
}

TEST_F(PlacementCommandsTest, VoxelRemovalCommand_GetDescription) {
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Set up initial voxel
    m_mockManager->setVoxelAt(pos, resolution, true);
    
    VoxelRemovalCommand command(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    std::string description = command.getDescription();
    EXPECT_EQ(description, "Remove 4cm voxel at (1, 2, 3)");
}

// Integration with HistoryManager Tests
TEST_F(PlacementCommandsTest, HistoryManager_PlacementAndRemoval) {
    HistoryManager history;
    history.setSnapshotInterval(0); // Disable snapshots
    
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Place a voxel
    auto placementCommand = PlacementCommandFactory::createPlacementCommand(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    ASSERT_NE(placementCommand, nullptr);
    
    EXPECT_TRUE(history.executeCommand(std::move(placementCommand)));
    
    // Check voxel was placed
    auto calls = m_mockManager->getSetVoxelCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_TRUE(calls[0].value);
    
    // Set up for removal (mock that voxel exists)
    m_mockManager->setVoxelAt(pos, resolution, true);
    m_mockManager->clearSetVoxelCalls();
    
    // Remove the voxel
    auto removalCommand = PlacementCommandFactory::createRemovalCommand(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    ASSERT_NE(removalCommand, nullptr);
    
    EXPECT_TRUE(history.executeCommand(std::move(removalCommand)));
    
    // Check voxel was removed
    calls = m_mockManager->getSetVoxelCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_FALSE(calls[0].value);
    
    // Test undo sequence
    m_mockManager->clearSetVoxelCalls();
    EXPECT_TRUE(history.undo()); // Undo removal
    
    calls = m_mockManager->getSetVoxelCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_TRUE(calls[0].value); // Should restore voxel
    
    m_mockManager->clearSetVoxelCalls();
    EXPECT_TRUE(history.undo()); // Undo placement
    
    calls = m_mockManager->getSetVoxelCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_FALSE(calls[0].value); // Should remove voxel
}

// Memory Usage Tests for many commands
TEST_F(PlacementCommandsTest, MemoryUsage_ManyCommands) {
    const int numCommands = 1000;
    std::vector<std::unique_ptr<VoxelPlacementCommand>> commands;
    commands.reserve(numCommands);
    
    // Create many commands
    for (int i = 0; i < numCommands; ++i) {
        Vector3i pos(i, 0, 0);
        VoxelResolution resolution = VoxelResolution::Size_1cm;
        
        commands.push_back(std::make_unique<VoxelPlacementCommand>(
            reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution));
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
    Vector3i pos(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto command1 = std::make_unique<VoxelPlacementCommand>(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    auto command2 = std::make_unique<VoxelPlacementCommand>(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos, resolution);
    
    // Test if commands can merge
    EXPECT_TRUE(command1->canMergeWith(*command2));
    
    // Test merging
    auto merged = command1->mergeWith(std::move(command2));
    EXPECT_NE(merged, nullptr);
}

TEST_F(PlacementCommandsTest, CommandMerging_DifferentPosition) {
    Vector3i pos1(1, 2, 3);
    Vector3i pos2(4, 5, 6);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    auto command1 = std::make_unique<VoxelPlacementCommand>(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos1, resolution);
    auto command2 = std::make_unique<VoxelPlacementCommand>(
        reinterpret_cast<VoxelDataManager*>(m_mockManager.get()), pos2, resolution);
    
    // Commands at different positions shouldn't merge
    EXPECT_FALSE(command1->canMergeWith(*command2));
}