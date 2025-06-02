#pragma once

#include <memory>
#include <vector>
#include <string>

#include "Command.h"

namespace VoxelEditor {
namespace UndoRedo {

class CompositeCommand : public Command {
public:
    explicit CompositeCommand(const std::string& name);
    ~CompositeCommand() override = default;
    
    // Add commands
    void addCommand(std::unique_ptr<Command> command);
    void addCommands(std::vector<std::unique_ptr<Command>> commands);
    
    // Command interface
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return m_name; }
    CommandType getType() const override { return CommandType::Composite; }
    size_t getMemoryUsage() const override;
    
    // Composite-specific methods
    size_t getCommandCount() const { return m_commands.size(); }
    const Command& getCommand(size_t index) const;
    
    // Memory optimization
    void compress() override;
    void decompress() override;
    
private:
    std::string m_name;
    std::vector<std::unique_ptr<Command>> m_commands;
    std::vector<bool> m_executionResults;
    size_t m_lastExecutedIndex = 0;
};

}
}