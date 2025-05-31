#include <gtest/gtest.h>
#include "../Command.h"
#include "../CompositeCommand.h"
#include "../Transaction.h"
#include <memory>
#include <string>

using namespace VoxelEditor::UndoRedo;

// Simple test command
class SimpleCommand : public Command {
public:
    SimpleCommand(int& value, int newValue)
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
    
    std::string getName() const override { return "SimpleCommand"; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override { return sizeof(*this); }
    
private:
    int& m_value;
    int m_newValue;
    int m_oldValue;
};

TEST(SimpleCommandTest, ExecuteAndUndo) {
    int value = 0;
    auto cmd = std::make_unique<SimpleCommand>(value, 42);
    
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(cmd->execute());
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(cmd->undo());
    EXPECT_EQ(value, 0);
}

TEST(CompositeCommandTest, ExecuteMultiple) {
    int value1 = 0;
    int value2 = 0;
    
    auto composite = std::make_unique<CompositeCommand>("Multi");
    composite->addCommand(std::make_unique<SimpleCommand>(value1, 10));
    composite->addCommand(std::make_unique<SimpleCommand>(value2, 20));
    
    EXPECT_TRUE(composite->execute());
    EXPECT_EQ(value1, 10);
    EXPECT_EQ(value2, 20);
    
    EXPECT_TRUE(composite->undo());
    EXPECT_EQ(value1, 0);
    EXPECT_EQ(value2, 0);
}

TEST(TransactionTest, CommitTransaction) {
    int value = 0;
    
    {
        Transaction txn("Test");
        txn.addCommand(std::make_unique<SimpleCommand>(value, 5));
        EXPECT_EQ(value, 5); // Commands execute immediately
        
        auto composite = txn.commit();
        EXPECT_NE(composite, nullptr);
        EXPECT_EQ(composite->getCommandCount(), 1);
    }
    
    EXPECT_EQ(value, 5); // Value remains after commit
}

TEST(TransactionTest, RollbackTransaction) {
    int value = 0;
    
    {
        Transaction txn("Test");
        txn.addCommand(std::make_unique<SimpleCommand>(value, 5));
        EXPECT_EQ(value, 5);
        
        txn.rollback();
    }
    
    EXPECT_EQ(value, 0); // Value reverted after rollback
}