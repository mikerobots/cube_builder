#pragma once

#include <memory>
#include <vector>
#include <string>

#include "Command.h"
#include "CompositeCommand.h"

namespace VoxelEditor {
namespace UndoRedo {

class Transaction {
public:
    explicit Transaction(const std::string& name);
    ~Transaction();
    
    // Add a command to the transaction
    void addCommand(std::unique_ptr<Command> command);
    
    // Commit the transaction and return a composite command
    std::unique_ptr<CompositeCommand> commit();
    
    // Cancel the transaction and undo any executed commands
    void rollback();
    
    // Query methods
    bool isEmpty() const { return m_commands.empty(); }
    size_t getCommandCount() const { return m_commands.size(); }
    const std::string& getName() const { return m_name; }
    size_t getMemoryUsage() const;
    
private:
    std::string m_name;
    std::vector<std::unique_ptr<Command>> m_commands;
    std::vector<std::unique_ptr<Command>> m_executedCommands;
    bool m_committed = false;
    bool m_rolledBack = false;
};

}
}