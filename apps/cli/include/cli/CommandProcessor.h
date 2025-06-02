#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <deque>

#include "cli/CommandTypes.h"

namespace VoxelEditor {
namespace CLI {

class Application;

class CommandProcessor {
public:
    CommandProcessor(Application* app);
    ~CommandProcessor() = default;
    
    // Command registration
    void registerCommand(const CommandDefinition& command);
    void registerAlias(const std::string& alias, const std::string& command);
    
    // Command execution
    CommandResult execute(const std::string& input);
    CommandResult executeCommand(const std::string& command, 
                               const std::vector<std::string>& args);
    
    // Command discovery
    std::vector<std::string> getCommands() const;
    std::vector<std::string> getCommandsInCategory(const std::string& category) const;
    const CommandDefinition* getCommand(const std::string& name) const;
    
    // Auto-completion
    std::vector<std::string> getCompletions(const std::string& partial) const;
    std::vector<std::string> getCommandCompletions(const std::string& partial) const;
    std::vector<std::string> getArgumentCompletions(const std::string& command, 
                                                   size_t argIndex,
                                                   const std::string& partial) const;
    
    // History
    void addToHistory(const std::string& command);
    const std::deque<std::string>& getHistory() const { return m_history; }
    std::string getPreviousCommand(int offset = 1) const;
    std::string getNextCommand(int offset = 1) const;
    
    // Help
    std::string getHelp() const;
    std::string getHelp(const std::string& command) const;
    std::string getCategoryHelp(const std::string& category) const;
    
    // Input parsing (public for AutoComplete)
    std::vector<std::string> parseInput(const std::string& input) const;
    
private:
    Application* m_app;
    std::unordered_map<std::string, CommandDefinition> m_commands;
    std::unordered_map<std::string, std::string> m_aliases;
    std::deque<std::string> m_history;
    size_t m_maxHistorySize = 100;
    int m_historyIndex = -1;
    
    // Helper methods
    std::string resolveAlias(const std::string& name) const;
    void registerBuiltinCommands();
};

} // namespace CLI
} // namespace VoxelEditor