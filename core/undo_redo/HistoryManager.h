#pragma once

#include "Command.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <mutex>

namespace VoxelEditor {
namespace UndoRedo {

// Forward declarations
class Transaction;
class StateSnapshot;

// Event types for undo/redo notifications
enum class UndoRedoEventType {
    CommandExecuted,
    CommandUndone,
    CommandRedone,
    HistoryCleared,
    TransactionStarted,
    TransactionCommitted,
    TransactionRolledBack,
    MemoryPressure
};

struct UndoRedoEvent {
    UndoRedoEventType type;
    std::string commandName;
    size_t historySize;
    size_t memoryUsage;
    bool canUndo;
    bool canRedo;
};

// Callbacks
using UndoRedoCallback = std::function<void(const UndoRedoEvent&)>;
using MemoryPressureCallback = std::function<void(size_t currentUsage, size_t maxUsage)>;

class HistoryManager {
public:
    HistoryManager();
    ~HistoryManager();
    
    // Command execution
    bool executeCommand(std::unique_ptr<Command> command);
    
    template<typename T, typename... Args>
    bool executeCommand(Args&&... args) {
        auto command = std::make_unique<T>(std::forward<Args>(args)...);
        return executeCommand(std::move(command));
    }
    
    // Undo/Redo operations
    bool canUndo() const;
    bool canRedo() const;
    bool undo();
    bool redo();
    void clearHistory();
    
    // History management
    void setMaxHistorySize(size_t size);
    void setMaxMemoryUsage(size_t bytes);
    size_t getHistorySize() const;
    size_t getMemoryUsage() const;
    
    // History queries
    std::vector<std::string> getUndoHistory() const;
    std::vector<std::string> getRedoHistory() const;
    std::string getLastExecutedCommand() const;
    
    // Grouping and transactions
    void beginTransaction(const std::string& name);
    void endTransaction();
    bool isInTransaction() const;
    void cancelTransaction();
    
    // Memory management
    void optimizeMemory();
    void setCompressionEnabled(bool enabled);
    void setSnapshotInterval(int commandCount);
    
    // Events
    void setUndoRedoCallback(UndoRedoCallback callback);
    void setMemoryPressureCallback(MemoryPressureCallback callback);
    
private:
    // Core data structures
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;
    std::unique_ptr<Transaction> m_currentTransaction;
    
    // Limits and settings
    size_t m_maxHistorySize = 100;
    size_t m_maxMemoryUsage = 256 * 1024 * 1024; // 256 MB default
    size_t m_currentMemoryUsage = 0;
    int m_snapshotInterval = 10;
    bool m_compressionEnabled = true;
    
    // Callbacks
    UndoRedoCallback m_undoRedoCallback;
    MemoryPressureCallback m_memoryPressureCallback;
    
    // Snapshots
    std::unique_ptr<StateSnapshot> m_baseSnapshot;
    std::vector<std::unique_ptr<StateSnapshot>> m_snapshots;
    
    // Thread safety
    mutable std::mutex m_mutex;
    
    // Private methods
    void pushToUndoStack(std::unique_ptr<Command> command);
    void clearRedoStack();
    void enforceMemoryLimits();
    void enforceHistoryLimits();
    void createSnapshot();
    void restoreFromSnapshot(const StateSnapshot& snapshot);
    void notifyEvent(const UndoRedoEvent& event);
    size_t calculateMemoryUsage() const;
    bool canUndoNoLock() const;
    bool canRedoNoLock() const;
};

}
}