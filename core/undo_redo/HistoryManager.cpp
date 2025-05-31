#include "HistoryManager.h"
#include "Transaction.h"
#include "StateSnapshot.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>

namespace VoxelEditor {
namespace UndoRedo {

HistoryManager::HistoryManager() {
    m_undoStack.reserve(m_maxHistorySize);
    m_redoStack.reserve(m_maxHistorySize / 2);
}

HistoryManager::~HistoryManager() {
    clearHistory();
}

bool HistoryManager::executeCommand(std::unique_ptr<Command> command) {
    if (!command) {
        // // Logging::Logger::getInstance().error("HistoryManager: Cannot execute null command");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Validate command
    auto validation = command->validate();
    if (!validation.valid) {
        // // Logging::Logger::getInstance().error("HistoryManager: Command validation failed: " + command->getName());
        // for (const auto& error : validation.errors) {
        //     // Logging::Logger::getInstance().error("  - " + error);
        // }
        return false;
    }
    
    // If we're in a transaction, add to transaction instead
    if (m_currentTransaction) {
        m_currentTransaction->addCommand(std::move(command));
        return true;
    }
    
    // Execute the command
    bool success = false;
    try {
        success = command->execute();
    } catch (const std::exception& e) {
        // // Logging::Logger::getInstance().error("HistoryManager: Command execution failed: " + 
        //                                    std::string(e.what()));
        return false;
    }
    
    if (!success) {
        // // Logging::Logger::getInstance().error("HistoryManager: Command execution returned false: " + 
        //                                    command->getName());
        return false;
    }
    
    // Capture command name before moving
    std::string commandName = command->getName();
    
    // Clear redo stack when a new command is executed
    clearRedoStack();
    
    // Add to undo stack
    pushToUndoStack(std::move(command));
    
    // Enforce limits
    enforceHistoryLimits();
    enforceMemoryLimits();
    
    // Create snapshot if needed
    if (m_snapshotInterval > 0 && m_undoStack.size() % m_snapshotInterval == 0) {
        createSnapshot();
    }
    
    // Notify listeners
    notifyEvent({
        UndoRedoEventType::CommandExecuted,
        commandName,
        m_undoStack.size(),
        m_currentMemoryUsage,
        canUndoNoLock(),
        canRedoNoLock()
    });
    
    return true;
}

bool HistoryManager::canUndo() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return canUndoNoLock();
}

bool HistoryManager::canRedo() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return canRedoNoLock();
}

bool HistoryManager::canUndoNoLock() const {
    return !m_undoStack.empty() && !m_currentTransaction;
}

bool HistoryManager::canRedoNoLock() const {
    return !m_redoStack.empty() && !m_currentTransaction;
}

bool HistoryManager::undo() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!canUndoNoLock()) {
        return false;
    }
    
    // Get the command to undo
    auto command = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    
    // Undo the command
    bool success = false;
    try {
        success = command->undo();
    } catch (const std::exception& e) {
        // Logging::Logger::getInstance().error("HistoryManager: Command undo failed: " + 
        //                                    std::string(e.what()));
        // Put it back on the undo stack
        m_undoStack.push_back(std::move(command));
        return false;
    }
    
    if (!success) {
        // Logging::Logger::getInstance().error("HistoryManager: Command undo returned false: " + 
        //                                    command->getName());
        // Put it back on the undo stack
        m_undoStack.push_back(std::move(command));
        return false;
    }
    
    // Add to redo stack
    m_redoStack.push_back(std::move(command));
    
    // Update memory usage
    m_currentMemoryUsage = calculateMemoryUsage();
    
    // Notify listeners
    notifyEvent({
        UndoRedoEventType::CommandUndone,
        m_redoStack.back()->getName(),
        m_undoStack.size(),
        m_currentMemoryUsage,
        canUndoNoLock(),
        canRedoNoLock()
    });
    
    return true;
}

bool HistoryManager::redo() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!canRedoNoLock()) {
        return false;
    }
    
    // Get the command to redo
    auto command = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    
    // Re-execute the command
    bool success = false;
    try {
        success = command->execute();
    } catch (const std::exception& e) {
        // Logging::Logger::getInstance().error("HistoryManager: Command redo failed: " + 
        //                                    std::string(e.what()));
        // Put it back on the redo stack
        m_redoStack.push_back(std::move(command));
        return false;
    }
    
    if (!success) {
        // Logging::Logger::getInstance().error("HistoryManager: Command redo returned false: " + 
        //                                    command->getName());
        // Put it back on the redo stack
        m_redoStack.push_back(std::move(command));
        return false;
    }
    
    // Add back to undo stack
    m_undoStack.push_back(std::move(command));
    
    // Update memory usage
    m_currentMemoryUsage = calculateMemoryUsage();
    
    // Notify listeners
    notifyEvent({
        UndoRedoEventType::CommandRedone,
        m_undoStack.back()->getName(),
        m_undoStack.size(),
        m_currentMemoryUsage,
        canUndoNoLock(),
        canRedoNoLock()
    });
    
    return true;
}

void HistoryManager::clearHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_undoStack.clear();
    m_redoStack.clear();
    m_snapshots.clear();
    m_currentMemoryUsage = 0;
    
    notifyEvent({
        UndoRedoEventType::HistoryCleared,
        "",
        0,
        0,
        false,
        false
    });
}

void HistoryManager::setMaxHistorySize(size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxHistorySize = size;
    enforceHistoryLimits();
}

void HistoryManager::setMaxMemoryUsage(size_t bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxMemoryUsage = bytes;
    enforceMemoryLimits();
}

size_t HistoryManager::getHistorySize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_undoStack.size();
}

size_t HistoryManager::getMemoryUsage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentMemoryUsage;
}

std::vector<std::string> HistoryManager::getUndoHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::string> history;
    history.reserve(m_undoStack.size());
    
    for (auto it = m_undoStack.rbegin(); it != m_undoStack.rend(); ++it) {
        history.push_back((*it)->getName());
    }
    
    return history;
}

std::vector<std::string> HistoryManager::getRedoHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::string> history;
    history.reserve(m_redoStack.size());
    
    for (auto it = m_redoStack.rbegin(); it != m_redoStack.rend(); ++it) {
        history.push_back((*it)->getName());
    }
    
    return history;
}

std::string HistoryManager::getLastExecutedCommand() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_undoStack.empty()) {
        return "";
    }
    
    return m_undoStack.back()->getName();
}

void HistoryManager::beginTransaction(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_currentTransaction) {
        // Logging::Logger::getInstance().warning("HistoryManager: Transaction already in progress");
        return;
    }
    
    m_currentTransaction = std::make_unique<Transaction>(name);
    
    notifyEvent({
        UndoRedoEventType::TransactionStarted,
        name,
        m_undoStack.size(),
        m_currentMemoryUsage,
        canUndoNoLock(),
        canRedoNoLock()
    });
}

void HistoryManager::endTransaction() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_currentTransaction) {
        // Logging::Logger::getInstance().warning("HistoryManager: No transaction to end");
        return;
    }
    
    auto compositeCommand = m_currentTransaction->commit();
    m_currentTransaction.reset();
    
    if (compositeCommand && compositeCommand->getCommandCount() > 0) {
        // Execute the composite command
        executeCommand(std::move(compositeCommand));
    }
    
    notifyEvent({
        UndoRedoEventType::TransactionCommitted,
        compositeCommand ? compositeCommand->getName() : "",
        m_undoStack.size(),
        m_currentMemoryUsage,
        canUndoNoLock(),
        canRedoNoLock()
    });
}

bool HistoryManager::isInTransaction() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentTransaction != nullptr;
}

void HistoryManager::cancelTransaction() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_currentTransaction) {
        // Logging::Logger::getInstance().warning("HistoryManager: No transaction to cancel");
        return;
    }
    
    m_currentTransaction->rollback();
    m_currentTransaction.reset();
    
    notifyEvent({
        UndoRedoEventType::TransactionRolledBack,
        "",
        m_undoStack.size(),
        m_currentMemoryUsage,
        canUndoNoLock(),
        canRedoNoLock()
    });
}

void HistoryManager::optimizeMemory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Compress old commands
    if (m_compressionEnabled) {
        for (auto& command : m_undoStack) {
            command->compress();
        }
        for (auto& command : m_redoStack) {
            command->compress();
        }
    }
    
    // Update memory usage
    m_currentMemoryUsage = calculateMemoryUsage();
}

void HistoryManager::setCompressionEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_compressionEnabled = enabled;
    
    if (!enabled) {
        // Decompress all commands
        for (auto& command : m_undoStack) {
            command->decompress();
        }
        for (auto& command : m_redoStack) {
            command->decompress();
        }
    }
}

void HistoryManager::setSnapshotInterval(int commandCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshotInterval = std::max(1, commandCount);
}

void HistoryManager::setUndoRedoCallback(UndoRedoCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_undoRedoCallback = callback;
}

void HistoryManager::setMemoryPressureCallback(MemoryPressureCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_memoryPressureCallback = callback;
}

void HistoryManager::pushToUndoStack(std::unique_ptr<Command> command) {
    m_undoStack.push_back(std::move(command));
    m_currentMemoryUsage += m_undoStack.back()->getMemoryUsage();
}

void HistoryManager::clearRedoStack() {
    m_redoStack.clear();
}

void HistoryManager::enforceMemoryLimits() {
    while (m_currentMemoryUsage > m_maxMemoryUsage && !m_undoStack.empty()) {
        // Notify about memory pressure
        if (m_memoryPressureCallback) {
            m_memoryPressureCallback(m_currentMemoryUsage, m_maxMemoryUsage);
        }
        
        // Remove oldest command
        m_currentMemoryUsage -= m_undoStack.front()->getMemoryUsage();
        m_undoStack.erase(m_undoStack.begin());
        
        // Also remove corresponding snapshot if any
        if (!m_snapshots.empty()) {
            m_snapshots.erase(m_snapshots.begin());
        }
    }
}

void HistoryManager::enforceHistoryLimits() {
    while (m_undoStack.size() > m_maxHistorySize) {
        m_currentMemoryUsage -= m_undoStack.front()->getMemoryUsage();
        m_undoStack.erase(m_undoStack.begin());
    }
}

void HistoryManager::createSnapshot() {
    // TODO: Implement snapshot creation
    // This will capture the current state of the application
}

void HistoryManager::restoreFromSnapshot(const StateSnapshot& snapshot) {
    // TODO: Implement snapshot restoration
    // This will restore the application state from a snapshot
}

void HistoryManager::notifyEvent(const UndoRedoEvent& event) {
    if (m_undoRedoCallback) {
        m_undoRedoCallback(event);
    }
}

size_t HistoryManager::calculateMemoryUsage() const {
    size_t usage = 0;
    
    for (const auto& command : m_undoStack) {
        usage += command->getMemoryUsage();
    }
    
    for (const auto& command : m_redoStack) {
        usage += command->getMemoryUsage();
    }
    
    // TODO: Add snapshot memory usage
    
    return usage;
}

}
}