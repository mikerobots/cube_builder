#include "Transaction.h"
#include "../../foundation/logging/Logger.h"

namespace VoxelEditor {
namespace UndoRedo {

Transaction::Transaction(const std::string& name)
    : m_name(name) {
}

Transaction::~Transaction() {
    if (!m_committed && !m_rolledBack) {
        // Auto-rollback if transaction wasn't committed
        rollback();
    }
}

void Transaction::addCommand(std::unique_ptr<Command> command) {
    if (!command) {
        Logging::Logger::getInstance().warning("Transaction: Attempted to add null command");
        return;
    }
    
    if (m_committed || m_rolledBack) {
        Logging::Logger::getInstance().error("Transaction: Cannot add commands to a completed transaction");
        return;
    }
    
    // Execute the command immediately within the transaction
    bool success = false;
    try {
        success = command->execute();
    } catch (const std::exception& e) {
        Logging::Logger::getInstance().error("Transaction: Command execution failed: " + 
                                           std::string(e.what()));
    }
    
    if (success) {
        m_executedCommands.push_back(std::move(command));
    } else {
        Logging::Logger::getInstance().error("Transaction: Command " + command->getName() + 
                                           " failed to execute");
        // The command is not added to the transaction
    }
}

std::unique_ptr<CompositeCommand> Transaction::commit() {
    if (m_committed || m_rolledBack) {
        Logging::Logger::getInstance().error("Transaction: Cannot commit a completed transaction");
        return nullptr;
    }
    
    m_committed = true;
    
    // Create composite command from executed commands
    auto composite = std::make_unique<CompositeCommand>(m_name);
    
    // Move all executed commands to the composite
    for (auto& command : m_executedCommands) {
        m_commands.push_back(std::move(command));
    }
    m_executedCommands.clear();
    
    // Add all commands to the composite
    composite->addCommands(std::move(m_commands));
    
    return composite;
}

void Transaction::rollback() {
    if (m_committed || m_rolledBack) {
        Logging::Logger::getInstance().warning("Transaction: Cannot rollback a completed transaction");
        return;
    }
    
    m_rolledBack = true;
    
    // Undo all executed commands in reverse order
    for (auto it = m_executedCommands.rbegin(); it != m_executedCommands.rend(); ++it) {
        try {
            (*it)->undo();
        } catch (const std::exception& e) {
            Logging::Logger::getInstance().error("Transaction: Failed to rollback command: " + 
                                               std::string(e.what()));
            // Continue rolling back other commands
        }
    }
    
    m_executedCommands.clear();
    m_commands.clear();
}

size_t Transaction::getMemoryUsage() const {
    size_t usage = sizeof(*this);
    usage += m_name.capacity();
    
    for (const auto& command : m_commands) {
        usage += command->getMemoryUsage();
    }
    
    for (const auto& command : m_executedCommands) {
        usage += command->getMemoryUsage();
    }
    
    return usage;
}

}
}