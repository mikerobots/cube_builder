#include <gtest/gtest.h>
#include "../Command.h"
#include "../HistoryManager.h"
#include "../CompositeCommand.h"
#include "../Transaction.h"
#include <memory>
#include <string>

using namespace VoxelEditor::UndoRedo;

// Test command implementation
class TestCommand : public Command {
public:
    TestCommand(const std::string& name, int& value, int newValue)
        : m_name(name)
        , m_value(value)
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
    
    std::string getName() const override { return m_name; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override { return sizeof(*this); }
    
private:
    std::string m_name;
    int& m_value;
    int m_newValue;
    int m_oldValue;
};

// Test failing command
class FailingCommand : public Command {
public:
    FailingCommand(bool failOnExecute, bool failOnUndo)
        : m_failOnExecute(failOnExecute)
        , m_failOnUndo(failOnUndo) {}
    
    bool execute() override {
        if (m_failOnExecute) return false;
        m_executed = true;
        return true;
    }
    
    bool undo() override {
        if (!m_executed) return false;
        if (m_failOnUndo) return false;
        m_executed = false;
        return true;
    }
    
    std::string getName() const override { return "FailingCommand"; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override { return sizeof(*this); }
    
private:
    bool m_failOnExecute;
    bool m_failOnUndo;
};

// Command pattern implementation for reversible operations
TEST(CommandTest, BasicExecuteUndo) {
    int value = 0;
    auto command = std::make_unique<TestCommand>("Set to 5", value, 5);
    
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(command->execute());
    EXPECT_EQ(value, 5);
    EXPECT_TRUE(command->undo());
    EXPECT_EQ(value, 0);
}

// History Management: Support for undo/redo operations
TEST(HistoryManagerTest, BasicUndoRedo) {
    HistoryManager history;
    history.setSnapshotInterval(0); // Disable snapshots
    int value = 0;
    
    // Execute commands
    EXPECT_TRUE(history.executeCommand(std::make_unique<TestCommand>("Set to 5", value, 5)));
    EXPECT_EQ(value, 5);
    
    EXPECT_TRUE(history.executeCommand(std::make_unique<TestCommand>("Set to 10", value, 10)));
    EXPECT_EQ(value, 10);
    
    // Test undo
    EXPECT_TRUE(history.canUndo());
    EXPECT_TRUE(history.undo());
    EXPECT_EQ(value, 5);
    
    EXPECT_TRUE(history.canUndo());
    EXPECT_TRUE(history.undo());
    EXPECT_EQ(value, 0);
    
    EXPECT_FALSE(history.canUndo());
    
    // Test redo
    EXPECT_TRUE(history.canRedo());
    EXPECT_TRUE(history.redo());
    EXPECT_EQ(value, 5);
    
    EXPECT_TRUE(history.canRedo());
    EXPECT_TRUE(history.redo());
    EXPECT_EQ(value, 10);
    
    EXPECT_FALSE(history.canRedo());
}

TEST(HistoryManagerTest, RedoStackClear) {
    HistoryManager history;
    history.setSnapshotInterval(0); // Disable snapshots
    int value = 0;
    
    history.executeCommand(std::make_unique<TestCommand>("Set to 5", value, 5));
    history.executeCommand(std::make_unique<TestCommand>("Set to 10", value, 10));
    
    history.undo();
    EXPECT_TRUE(history.canRedo());
    
    // Execute new command should clear redo stack
    history.executeCommand(std::make_unique<TestCommand>("Set to 15", value, 15));
    EXPECT_FALSE(history.canRedo());
}

// History Management: Support for undo/redo operations with 5-10 operation limit
TEST(HistoryManagerTest, HistoryLimit) {
    HistoryManager history;
    history.setMaxHistorySize(2);
    history.setSnapshotInterval(0); // Disable snapshots
    int value = 0;
    
    history.executeCommand(std::make_unique<TestCommand>("Set to 1", value, 1));
    history.executeCommand(std::make_unique<TestCommand>("Set to 2", value, 2));
    history.executeCommand(std::make_unique<TestCommand>("Set to 3", value, 3));
    
    // Should only be able to undo twice
    EXPECT_TRUE(history.undo());
    EXPECT_TRUE(history.undo());
    EXPECT_FALSE(history.canUndo());
}

// State management for complex operations
TEST(CompositeCommandTest, BasicComposite) {
    int value1 = 0;
    int value2 = 0;
    
    auto composite = std::make_unique<CompositeCommand>("Multiple Operations");
    composite->addCommand(std::make_unique<TestCommand>("Set value1 to 5", value1, 5));
    composite->addCommand(std::make_unique<TestCommand>("Set value2 to 10", value2, 10));
    
    EXPECT_TRUE(composite->execute());
    EXPECT_EQ(value1, 5);
    EXPECT_EQ(value2, 10);
    
    EXPECT_TRUE(composite->undo());
    EXPECT_EQ(value1, 0);
    EXPECT_EQ(value2, 0);
}

TEST(CompositeCommandTest, PartialFailure) {
    int value = 0;
    
    auto composite = std::make_unique<CompositeCommand>("Partial Failure");
    composite->addCommand(std::make_unique<TestCommand>("Set to 5", value, 5));
    composite->addCommand(std::make_unique<FailingCommand>(true, false)); // Fails on execute
    
    EXPECT_FALSE(composite->execute());
    EXPECT_EQ(value, 0); // Should be rolled back
}

// Transaction support for atomic operations
TEST(TransactionTest, BasicTransaction) {
    int value = 0;
    Transaction transaction("Test Transaction");
    
    transaction.addCommand(std::make_unique<TestCommand>("Set to 5", value, 5));
    EXPECT_EQ(value, 5); // Commands execute immediately
    
    transaction.addCommand(std::make_unique<TestCommand>("Set to 10", value, 10));
    EXPECT_EQ(value, 10);
    
    auto composite = transaction.commit();
    EXPECT_NE(composite, nullptr);
    EXPECT_EQ(composite->getCommandCount(), 2);
}

// Transaction support for atomic operations - auto rollback
TEST(TransactionTest, AutoRollback) {
    int value = 0;
    
    {
        Transaction transaction("Auto Rollback");
        transaction.addCommand(std::make_unique<TestCommand>("Set to 5", value, 5));
        EXPECT_EQ(value, 5);
        // Transaction goes out of scope without commit
    }
    
    EXPECT_EQ(value, 0); // Should be rolled back
}

TEST(TransactionTest, ManualRollback) {
    int value = 0;
    Transaction transaction("Manual Rollback");
    
    transaction.addCommand(std::make_unique<TestCommand>("Set to 5", value, 5));
    EXPECT_EQ(value, 5);
    
    transaction.rollback();
    EXPECT_EQ(value, 0);
}