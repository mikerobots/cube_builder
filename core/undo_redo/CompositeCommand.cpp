#include "CompositeCommand.h"
#include "../../foundation/logging/Logger.h"
#include <stdexcept>

namespace VoxelEditor {
namespace UndoRedo {

CompositeCommand::CompositeCommand(const std::string& name)
    : m_name(name) {
}

void CompositeCommand::addCommand(std::unique_ptr<Command> command) {
    if (!command) {
        Logging::Logger::getInstance().warning("CompositeCommand: Attempted to add null command");
        return;
    }
    
    if (m_executed) {
        Logging::Logger::getInstance().error("CompositeCommand: Cannot add commands after execution");
        throw std::runtime_error("Cannot add commands to an executed composite command");
    }
    
    m_commands.push_back(std::move(command));
}

void CompositeCommand::addCommands(std::vector<std::unique_ptr<Command>> commands) {
    for (auto& command : commands) {
        addCommand(std::move(command));
    }
}

bool CompositeCommand::execute() {
    if (m_executed) {
        // Re-execute from the beginning
        m_lastExecutedIndex = 0;
        m_executionResults.clear();
    }
    
    m_executionResults.reserve(m_commands.size());
    bool allSuccessful = true;
    
    for (size_t i = m_lastExecutedIndex; i < m_commands.size(); ++i) {
        bool success = false;
        
        try {
            success = m_commands[i]->execute();
        } catch (const std::exception& e) {
            Logging::Logger::getInstance().error("CompositeCommand: Command execution failed: " + 
                                               std::string(e.what()));
            success = false;
        }
        
        m_executionResults.push_back(success);
        
        if (!success) {
            allSuccessful = false;
            m_lastExecutedIndex = i;
            
            Logging::Logger::getInstance().error("CompositeCommand: Command " + 
                                               m_commands[i]->getName() + " failed");
            
            // Undo any successfully executed commands
            for (int j = static_cast<int>(i) - 1; j >= 0; --j) {
                if (m_executionResults[j]) {
                    try {
                        m_commands[j]->undo();
                    } catch (const std::exception& e) {
                        Logging::Logger::getInstance().error("CompositeCommand: Failed to undo command during rollback: " +
                                                           std::string(e.what()));
                    }
                }
            }
            
            m_executionResults.clear();
            m_lastExecutedIndex = 0;
            return false;
        }
        
        m_lastExecutedIndex = i + 1;
    }
    
    m_executed = true;
    return allSuccessful;
}

bool CompositeCommand::undo() {
    if (!m_executed) {
        Logging::Logger::getInstance().warning("CompositeCommand: Cannot undo a command that hasn't been executed");
        return false;
    }
    
    bool allSuccessful = true;
    
    // Undo commands in reverse order
    for (int i = static_cast<int>(m_commands.size()) - 1; i >= 0; --i) {
        if (i < m_executionResults.size() && m_executionResults[i]) {
            bool success = false;
            
            try {
                success = m_commands[i]->undo();
            } catch (const std::exception& e) {
                Logging::Logger::getInstance().error("CompositeCommand: Command undo failed: " + 
                                                   std::string(e.what()));
                success = false;
            }
            
            if (!success) {
                allSuccessful = false;
                Logging::Logger::getInstance().error("CompositeCommand: Failed to undo command " + 
                                                   m_commands[i]->getName());
                // Continue trying to undo other commands
            }
        }
    }
    
    m_executed = false;
    m_lastExecutedIndex = 0;
    m_executionResults.clear();
    
    return allSuccessful;
}

size_t CompositeCommand::getMemoryUsage() const {
    size_t totalUsage = sizeof(*this);
    totalUsage += m_name.capacity();
    totalUsage += m_executionResults.capacity() * sizeof(bool);
    
    for (const auto& command : m_commands) {
        if (command) {
            totalUsage += command->getMemoryUsage();
        }
    }
    
    return totalUsage;
}

const Command& CompositeCommand::getCommand(size_t index) const {
    if (index >= m_commands.size()) {
        throw std::out_of_range("CompositeCommand: Command index out of range");
    }
    
    return *m_commands[index];
}

void CompositeCommand::compress() {
    for (auto& command : m_commands) {
        if (command) {
            command->compress();
        }
    }
}

void CompositeCommand::decompress() {
    for (auto& command : m_commands) {
        if (command) {
            command->decompress();
        }
    }
}

}
}