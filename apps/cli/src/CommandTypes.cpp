// Standard library includes first
#include <sstream>
#include <algorithm>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>

// Application headers
#include "cli/CommandTypes.h"

namespace VoxelEditor {

// CommandDefinition implementation
std::string CommandDefinition::getUsage() const {
    std::stringstream ss;
    ss << name;
    
    for (const auto& arg : arguments) {
        ss << " ";
        if (!arg.required) ss << "[";
        ss << "<" << arg.name << ":" << arg.type << ">";
        if (!arg.required) ss << "]";
    }
    
    if (!aliases.empty()) {
        ss << " (aliases: ";
        for (size_t i = 0; i < aliases.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << aliases[i];
        }
        ss << ")";
    }
    
    return ss.str();
}

std::string CommandDefinition::getHelp() const {
    std::stringstream ss;
    ss << "Usage: " << getUsage() << "\n";
    ss << "Description: " << description << "\n";
    ss << "Category: " << category << "\n";
    
    if (!arguments.empty()) {
        ss << "\nArguments:\n";
        for (const auto& arg : arguments) {
            ss << "  " << arg.name << " (" << arg.type << ")";
            if (!arg.required) {
                ss << " [optional, default: " << arg.defaultValue << "]";
            }
            ss << "\n    " << arg.description << "\n";
        }
    }
    
    return ss.str();
}

// CommandContext implementation
CommandContext::CommandContext(Application* app, const std::string& cmd, 
                             const std::vector<std::string>& args)
    : m_app(app), m_command(cmd), m_args(args) {
    parseOptions();
}

std::string CommandContext::getArg(size_t index, const std::string& defaultValue) const {
    if (index < m_args.size()) {
        return m_args[index];
    }
    return defaultValue;
}

int CommandContext::getIntArg(size_t index, int defaultValue) const {
    if (index < m_args.size()) {
        try {
            return std::stoi(m_args[index]);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

float CommandContext::getFloatArg(size_t index, float defaultValue) const {
    if (index < m_args.size()) {
        try {
            return std::stof(m_args[index]);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool CommandContext::getBoolArg(size_t index, bool defaultValue) const {
    if (index < m_args.size()) {
        const std::string& arg = m_args[index];
        if (arg == "true" || arg == "1" || arg == "yes" || arg == "on") {
            return true;
        }
        if (arg == "false" || arg == "0" || arg == "no" || arg == "off") {
            return false;
        }
    }
    return defaultValue;
}

bool CommandContext::hasOption(const std::string& name) const {
    return m_options.find(name) != m_options.end();
}

std::string CommandContext::getOption(const std::string& name, const std::string& defaultValue) const {
    auto it = m_options.find(name);
    if (it != m_options.end()) {
        return it->second;
    }
    return defaultValue;
}

void CommandContext::parseOptions() {
    for (const auto& arg : m_args) {
        if (arg.starts_with("--")) {
            size_t pos = arg.find('=');
            if (pos != std::string::npos) {
                std::string key = arg.substr(2, pos - 2);
                std::string value = arg.substr(pos + 1);
                m_options[key] = value;
            } else {
                m_options[arg.substr(2)] = "true";
            }
        }
    }
}

} // namespace VoxelEditor