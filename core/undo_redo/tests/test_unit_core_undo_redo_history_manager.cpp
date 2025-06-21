#include <gtest/gtest.h>
#include "../HistoryManager.h"
#include "../Command.h"
#include <memory>

using namespace VoxelEditor::UndoRedo;

// Simple test command
class TestCmd : public Command {
public:
    TestCmd(int& value, int newValue)
        : m_value(value)
        , m_newValue(newValue)
        , m_oldValue(value) {}
    
    bool execute() override {
        m_value = m_newValue;
        m_executed = true;
        return true;
    }
    
    bool undo() override {
        if (!m_executed) return false;
        m_value = m_oldValue;
        m_executed = false;
        return true;
    }
    
    std::string getName() const override { return "TestCmd"; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override { return sizeof(*this); }
    
private:
    int& m_value;
    int m_newValue;
    int m_oldValue;
};

// Basic HistoryManager initialization
TEST(HistoryManagerTest, CreateAndDestroy) {
    HistoryManager history;
    EXPECT_FALSE(history.canUndo());
    EXPECT_FALSE(history.canRedo());
}

// Command pattern implementation for reversible operations
TEST(HistoryManagerTest, SingleCommand) {
    HistoryManager history;
    int value = 0;
    
    // Disable snapshots to avoid any issues
    history.setSnapshotInterval(0);
    
    auto result = history.executeCommand(std::make_unique<TestCmd>(value, 5));
    EXPECT_TRUE(result);
    EXPECT_EQ(value, 5);
    EXPECT_TRUE(history.canUndo());
    EXPECT_FALSE(history.canRedo());
}

// History Management: Support for undo/redo operations
TEST(HistoryManagerTest, UndoSingleCommand) {
    HistoryManager history;
    int value = 0;
    
    // Disable snapshots
    history.setSnapshotInterval(0);
    
    history.executeCommand(std::make_unique<TestCmd>(value, 5));
    EXPECT_EQ(value, 5);
    
    EXPECT_TRUE(history.undo());
    EXPECT_EQ(value, 0);
    EXPECT_FALSE(history.canUndo());
    EXPECT_TRUE(history.canRedo());
}

// History Management: Support for undo/redo operations - redo functionality
TEST(HistoryManagerTest, RedoSingleCommand) {
    HistoryManager history;
    int value = 0;
    
    // Disable snapshots
    history.setSnapshotInterval(0);
    
    history.executeCommand(std::make_unique<TestCmd>(value, 5));
    history.undo();
    
    EXPECT_TRUE(history.redo());
    EXPECT_EQ(value, 5);
    EXPECT_TRUE(history.canUndo());
    EXPECT_FALSE(history.canRedo());
}

// State management for complex operations - multiple command sequences
TEST(HistoryManagerTest, MultipleCommands) {
    HistoryManager history;
    int value = 0;
    
    // Disable snapshots
    history.setSnapshotInterval(0);
    
    history.executeCommand(std::make_unique<TestCmd>(value, 5));
    history.executeCommand(std::make_unique<TestCmd>(value, 10));
    history.executeCommand(std::make_unique<TestCmd>(value, 15));
    
    EXPECT_EQ(value, 15);
    
    // Undo all
    history.undo();
    EXPECT_EQ(value, 10);
    history.undo();
    EXPECT_EQ(value, 5);
    history.undo();
    EXPECT_EQ(value, 0);
    
    EXPECT_FALSE(history.canUndo());
    EXPECT_TRUE(history.canRedo());
}