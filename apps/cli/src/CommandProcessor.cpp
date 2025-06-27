// Standard library includes first
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>

// Application headers
#include "cli/CommandProcessor.h"
#include "cli/Application.h"

namespace VoxelEditor {

CommandProcessor::CommandProcessor(Application* app)
    : m_app(app) {
    registerBuiltinCommands();
}

void CommandProcessor::registerCommand(const CommandDefinition& command) {
    m_commands[command.name] = command;
    for (const auto& alias : command.aliases) {
        m_aliases[alias] = command.name;
    }
}

void CommandProcessor::registerAlias(const std::string& alias, const std::string& command) {
    m_aliases[alias] = command;
}

CommandResult CommandProcessor::execute(const std::string& input) {
    // Check if input is empty or only whitespace
    if (input.empty() || input.find_first_not_of(" \t\n\r") == std::string::npos) {
        return CommandResult::Error("Invalid command: empty or whitespace only");
    }
    
    addToHistory(input);
    
    auto tokens = parseInput(input);
    if (tokens.empty()) {
        return CommandResult::Error("Invalid command: no valid tokens");
    }
    
    return executeCommand(tokens[0], std::vector<std::string>(tokens.begin() + 1, tokens.end()));
}

CommandResult CommandProcessor::executeCommand(const std::string& command, 
                                             const std::vector<std::string>& args) {
    std::string cmdName = resolveAlias(command);
    
    auto it = m_commands.find(cmdName);
    if (it == m_commands.end()) {
        return CommandResult::Error("Unknown command: " + command + ". Type 'help' for available commands.");
    }
    
    const CommandDefinition& def = it->second;
    
    // Validate required arguments
    size_t requiredCount = 0;
    for (const auto& arg : def.arguments) {
        if (arg.required) requiredCount++;
    }
    
    if (args.size() < requiredCount) {
        return CommandResult::Error("Insufficient arguments. " + def.getUsage());
    }
    
    // Check for too many arguments
    if (args.size() > def.arguments.size()) {
        return CommandResult::Error("Too many arguments. " + def.getUsage());
    }
    
    CommandContext context(m_app, cmdName, args);
    
    try {
        return def.handler(context);
    } catch (const std::exception& e) {
        return CommandResult::Error("Command failed: " + std::string(e.what()));
    }
}

std::vector<std::string> CommandProcessor::getCommands() const {
    std::vector<std::string> commands;
    commands.reserve(m_commands.size());
    
    for (const auto& [name, def] : m_commands) {
        commands.push_back(name);
    }
    
    std::sort(commands.begin(), commands.end());
    return commands;
}

std::vector<std::string> CommandProcessor::getCommandsInCategory(const std::string& category) const {
    std::vector<std::string> commands;
    
    for (const auto& [name, def] : m_commands) {
        if (def.category == category) {
            commands.push_back(name);
        }
    }
    
    std::sort(commands.begin(), commands.end());
    return commands;
}

const CommandDefinition* CommandProcessor::getCommand(const std::string& name) const {
    std::string cmdName = resolveAlias(name);
    auto it = m_commands.find(cmdName);
    return (it != m_commands.end()) ? &it->second : nullptr;
}

std::vector<std::string> CommandProcessor::getCompletions(const std::string& partial) const {
    auto tokens = parseInput(partial);
    
    if (tokens.empty()) {
        return getCommandCompletions("");
    }
    
    if (tokens.size() == 1 && !partial.ends_with(' ')) {
        return getCommandCompletions(tokens[0]);
    }
    
    const std::string& cmdName = tokens[0];
    auto cmd = getCommand(cmdName);
    if (!cmd) {
        return {};
    }
    
    size_t argIndex = tokens.size() - 2;
    if (partial.ends_with(' ')) {
        argIndex++;
    }
    
    std::string argPartial = (tokens.size() > 1 && !partial.ends_with(' ')) 
                           ? tokens.back() : "";
    
    return getArgumentCompletions(cmdName, argIndex, argPartial);
}

std::vector<std::string> CommandProcessor::getCommandCompletions(const std::string& partial) const {
    std::vector<std::string> completions;
    
    // Commands
    for (const auto& [name, def] : m_commands) {
        if (name.starts_with(partial)) {
            completions.push_back(name);
        }
    }
    
    // Aliases
    for (const auto& [alias, cmd] : m_aliases) {
        if (alias.starts_with(partial)) {
            completions.push_back(alias);
        }
    }
    
    std::sort(completions.begin(), completions.end());
    completions.erase(std::unique(completions.begin(), completions.end()), completions.end());
    
    return completions;
}

std::vector<std::string> CommandProcessor::getArgumentCompletions(const std::string& command, 
                                                                size_t argIndex,
                                                                const std::string& partial) const {
    auto cmd = getCommand(command);
    if (!cmd || argIndex >= cmd->arguments.size()) {
        return {};
    }
    
    const CommandArgument& arg = cmd->arguments[argIndex];
    
    // Type-specific completions
    if (arg.type == "bool") {
        std::vector<std::string> bools = {"true", "false", "yes", "no", "on", "off"};
        std::vector<std::string> completions;
        for (const auto& b : bools) {
            if (b.starts_with(partial)) {
                completions.push_back(b);
            }
        }
        return completions;
    }
    
    // Command-specific completions would go here
    // (e.g., filenames, group names, view names, etc.)
    
    return {};
}

void CommandProcessor::addToHistory(const std::string& command) {
    if (!command.empty()) {
        m_history.push_back(command);
        if (m_history.size() > m_maxHistorySize) {
            m_history.pop_front();
        }
    }
    m_historyIndex = -1;
}

std::string CommandProcessor::getPreviousCommand(int offset) const {
    if (m_history.empty()) return "";
    
    size_t index = m_history.size() - offset;
    if (index < m_history.size()) {
        return m_history[index];
    }
    return "";
}

std::string CommandProcessor::getNextCommand(int offset) const {
    if (m_history.empty() || offset <= 0) return "";
    
    size_t index = m_history.size() - offset;
    if (index < m_history.size()) {
        return m_history[index];
    }
    return "";
}

std::string CommandProcessor::getHelp() const {
    std::stringstream ss;
    ss << "Voxel Editor CLI - Available Commands\n";
    ss << "=====================================\n\n";
    
    // Group commands by category
    std::map<std::string, std::vector<std::string>> byCategory;
    for (const auto& [name, def] : m_commands) {
        byCategory[def.category].push_back(name);
    }
    
    for (const auto& [category, commands] : byCategory) {
        ss << category << ":\n";
        
        for (const auto& cmd : commands) {
            const auto& def = m_commands.at(cmd);
            ss << "  " << std::left << std::setw(20) << cmd 
               << " - " << def.description << "\n";
        }
        ss << "\n";
    }
    
    ss << "Type 'help <command>' for detailed command help.\n";
    return ss.str();
}

std::string CommandProcessor::getHelp(const std::string& command) const {
    auto cmd = getCommand(command);
    if (!cmd) {
        return "Unknown command: " + command;
    }
    return cmd->getHelp();
}

std::string CommandProcessor::getCategoryHelp(const std::string& category) const {
    std::stringstream ss;
    ss << category << " Commands:\n";
    ss << std::string(category.length() + 10, '=') << "\n\n";
    
    auto commands = getCommandsInCategory(category);
    for (const auto& cmd : commands) {
        const auto& def = m_commands.at(cmd);
        ss << def.getUsage() << "\n";
        ss << "  " << def.description << "\n\n";
    }
    
    return ss.str();
}

std::vector<std::string> CommandProcessor::parseInput(const std::string& input) const {
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;
    
    while (ss >> std::quoted(token)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string CommandProcessor::resolveAlias(const std::string& name) const {
    auto it = m_aliases.find(name);
    return (it != m_aliases.end()) ? it->second : name;
}

void CommandProcessor::registerBuiltinCommands() {
    // Help commands
    registerCommand({
        Commands::HELP,
        "Show available commands or command help",
        CommandCategory::HELP,
        {"h", "?"},
        {{"command", "Command to get help for", "string", false, ""}},
        [this](const CommandContext& ctx) {
            if (ctx.getArgCount() > 0) {
                return CommandResult::Success(getHelp(ctx.getArg(0)));
            }
            return CommandResult::Success(getHelp());
        }
    });
    
    // System commands
    registerCommand({
        Commands::QUIT,
        "Exit the application",
        CommandCategory::SYSTEM,
        {"exit", "q"},
        {},
        [](const CommandContext& ctx) {
            return CommandResult::Exit();
        }
    });
    
    registerCommand({
        Commands::CLEAR,
        "Clear the screen",
        CommandCategory::SYSTEM,
        {"cls"},
        {},
        [](const CommandContext& ctx) {
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
            return CommandResult::Success();
        }
    });
}

} // namespace VoxelEditor