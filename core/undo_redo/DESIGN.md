# Undo/Redo System Subsystem

## Purpose
Manages operation history, command pattern implementation, and state restoration for all user actions with memory-efficient state management.

## Current Implementation Status

### Implemented Components
- **Command.h**: Base command interface with validation, merging, and memory management
- **HistoryManager**: Core undo/redo coordination with transaction support
- **VoxelCommands**: Single and bulk voxel editing, fill, copy, and move operations
- **SelectionCommands**: Selection modification, region selection, and set operations
- **StateSnapshot**: Full state capture and restoration (basic implementation)
- **Transaction**: Command grouping with commit/rollback support
- **CompositeCommand**: Composite pattern for multiple commands

### Missing Components
- **CommandComposer**: Not implemented
- **MemoryOptimizer**: Not implemented
- **CommandCompressor**: Not implemented
- **IncrementalSnapshot**: Not implemented
- **Group Commands**: Not implemented
- **Camera Commands**: Not implemented
- **Workspace Commands**: Not implemented

## Architecture Issues

### 1. Circular Dependencies
- Commands directly depend on manager classes (VoxelDataManager, SelectionManager)
- StateSnapshot has hard dependencies on multiple subsystems
- No abstraction layer between commands and core systems

### 2. Tight Coupling
- Commands manipulate managers directly without interfaces
- No dependency injection or inversion of control
- Hard-coded dependencies make testing difficult

### 3. Missing Abstractions
- No "Undoable" interface for operations
- No abstract factory for command creation
- Missing strategy pattern for memory optimization
- Compression methods exist but are not implemented

### 4. Performance Bottlenecks
- StateSnapshot captures entire voxel grid (memory intensive)
- Single mutex for all HistoryManager operations
- No background processing or async command execution
- Linear operations on command history

### 5. Memory Management Issues
- Basic memory limit enforcement without optimization
- No actual compression implementation
- Missing memory pooling or command reuse
- StateSnapshot doesn't scale with large voxel grids

### 6. Thread Safety Concerns
- Coarse-grained locking with single mutex
- Potential deadlocks if commands trigger other commands
- No lock-free alternatives for high-performance scenarios

### 7. API Inconsistencies
- Mixed approaches to command creation
- Optional validation with inconsistent implementation
- Memory usage calculation often returns estimates

### 8. Missing Error Handling
- Limited exception handling in command execution
- No partial failure recovery
- Missing rollback strategies for complex commands

### 9. Scalability Limitations
- No indexing for large command histories
- StateSnapshot doesn't support incremental updates
- Missing pagination or chunking for memory management

### 10. Testing Difficulties
- Direct dependencies prevent effective mocking
- No test doubles or interfaces for managers
- Limited test coverage for edge cases

## Key Components

### HistoryManager
**Responsibility**: Main undo/redo coordination and command execution
- Execute commands and track history
- Manage undo/redo stack operations
- Memory limit enforcement
- Integration with all subsystems

### Command
**Responsibility**: Base command interface and common functionality
- Encapsulate reversible operations
- Provide execute/undo semantics
- Support command composition
- Memory-efficient state capture

### StateSnapshot
**Responsibility**: Efficient state capture and restoration
- Incremental state differences
- Compressed state storage
- Fast state comparison
- Memory pressure handling

### CommandComposer
**Responsibility**: Combine multiple commands into composite operations
- Macro command creation
- Atomic multi-step operations
- Nested command handling
- Transaction semantics

## Interface Design

```cpp
class HistoryManager {
public:
    // Command execution
    bool executeCommand(std::unique_ptr<Command> command);
    template<typename T, typename... Args>
    bool executeCommand(Args&&... args);
    
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
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;
    std::unique_ptr<Transaction> m_currentTransaction;
    
    size_t m_maxHistorySize;
    size_t m_maxMemoryUsage;
    size_t m_currentMemoryUsage;
    int m_snapshotInterval;
    bool m_compressionEnabled;
    
    UndoRedoCallback m_undoRedoCallback;
    MemoryPressureCallback m_memoryPressureCallback;
    
    std::unique_ptr<StateSnapshot> m_baseSnapshot;
    std::vector<std::unique_ptr<StateSnapshot>> m_snapshots;
    
    void pushToUndoStack(std::unique_ptr<Command> command);
    void clearRedoStack();
    void enforceMemoryLimits();
    void createSnapshot();
    void restoreFromSnapshot(const StateSnapshot& snapshot);
};
```

## Command Pattern Implementation

### Base Command Interface
```cpp
class Command {
public:
    virtual ~Command() = default;
    
    // Core operations
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual bool canUndo() const { return true; }
    
    // Command information
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const { return getName(); }
    virtual CommandType getType() const = 0;
    
    // Memory management
    virtual size_t getMemoryUsage() const = 0;
    virtual void compress() {}
    virtual void decompress() {}
    
    // Merging capability
    virtual bool canMergeWith(const Command& other) const { return false; }
    virtual std::unique_ptr<Command> mergeWith(std::unique_ptr<Command> other) { return nullptr; }
    
    // Validation
    virtual bool isValid() const { return true; }
    virtual ValidationResult validate() const;
    
protected:
    std::chrono::high_resolution_clock::time_point m_timestamp;
    bool m_executed = false;
};

enum class CommandType {
    VoxelEdit,
    Selection,
    Group,
    Camera,
    Workspace,
    Import,
    Composite
};

struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};
```

### Voxel Commands
```cpp
class VoxelEditCommand : public Command {
public:
    VoxelEditCommand(VoxelDataManager* voxelManager, const Vector3i& position, VoxelResolution resolution, bool newValue);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Edit Voxel"; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override;
    
    bool canMergeWith(const Command& other) const override;
    std::unique_ptr<Command> mergeWith(std::unique_ptr<Command> other) override;
    
private:
    VoxelDataManager* m_voxelManager;
    Vector3i m_position;
    VoxelResolution m_resolution;
    bool m_oldValue;
    bool m_newValue;
};

class BulkVoxelEditCommand : public Command {
public:
    BulkVoxelEditCommand(VoxelDataManager* voxelManager, const std::vector<VoxelChange>& changes);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override;
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override;
    
    void compress() override;
    void decompress() override;
    
private:
    struct VoxelChange {
        Vector3i position;
        VoxelResolution resolution;
        bool oldValue;
        bool newValue;
    };
    
    VoxelDataManager* m_voxelManager;
    std::vector<VoxelChange> m_changes;
    bool m_compressed = false;
    std::vector<uint8_t> m_compressedData;
};
```

### Selection Commands
```cpp
class SelectionCommand : public Command {
public:
    SelectionCommand(SelectionManager* selectionManager, const SelectionSet& newSelection);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Change Selection"; }
    CommandType getType() const override { return CommandType::Selection; }
    size_t getMemoryUsage() const override;
    
private:
    SelectionManager* m_selectionManager;
    SelectionSet m_oldSelection;
    SelectionSet m_newSelection;
};

class MoveSelectionCommand : public Command {
public:
    MoveSelectionCommand(VoxelDataManager* voxelManager, SelectionManager* selectionManager, 
                        const SelectionSet& selection, const Vector3f& offset);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Move Selection"; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override;
    
private:
    VoxelDataManager* m_voxelManager;
    SelectionManager* m_selectionManager;
    std::vector<std::pair<VoxelId, VoxelId>> m_moves; // old -> new positions
    SelectionSet m_originalSelection;
    SelectionSet m_newSelection;
};
```

### Group Commands
```cpp
class CreateGroupCommand : public Command {
public:
    CreateGroupCommand(GroupManager* groupManager, const std::string& name, const SelectionSet& voxels);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Create Group"; }
    CommandType getType() const override { return CommandType::Group; }
    size_t getMemoryUsage() const override;
    
private:
    GroupManager* m_groupManager;
    std::string m_groupName;
    SelectionSet m_voxels;
    GroupId m_createdGroupId;
};

class MoveGroupCommand : public Command {
public:
    MoveGroupCommand(GroupManager* groupManager, VoxelDataManager* voxelManager, 
                    GroupId groupId, const Vector3f& offset);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Move Group"; }
    CommandType getType() const override { return CommandType::Group; }
    size_t getMemoryUsage() const override;
    
private:
    GroupManager* m_groupManager;
    VoxelDataManager* m_voxelManager;
    GroupId m_groupId;
    Vector3f m_offset;
    std::vector<std::pair<VoxelId, VoxelId>> m_moves;
};
```

### Camera Commands
```cpp
class CameraCommand : public Command {
public:
    CameraCommand(CameraManager* cameraManager, const CameraState& newState);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Camera Change"; }
    CommandType getType() const override { return CommandType::Camera; }
    size_t getMemoryUsage() const override;
    
    bool canMergeWith(const Command& other) const override;
    std::unique_ptr<Command> mergeWith(std::unique_ptr<Command> other) override;
    
private:
    CameraManager* m_cameraManager;
    CameraState m_oldState;
    CameraState m_newState;
    
    bool isSmallChange(const CameraState& from, const CameraState& to) const;
};
```

## Composite Commands

### CompositeCommand
```cpp
class CompositeCommand : public Command {
public:
    CompositeCommand(const std::string& name);
    
    void addCommand(std::unique_ptr<Command> command);
    void addCommands(std::vector<std::unique_ptr<Command>> commands);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return m_name; }
    CommandType getType() const override { return CommandType::Composite; }
    size_t getMemoryUsage() const override;
    
    // Composite-specific
    size_t getCommandCount() const { return m_commands.size(); }
    const Command& getCommand(size_t index) const;
    
    void compress() override;
    void decompress() override;
    
private:
    std::string m_name;
    std::vector<std::unique_ptr<Command>> m_commands;
    std::vector<bool> m_executionResults;
    size_t m_lastExecutedIndex = 0;
};
```

### Transaction Support
```cpp
class Transaction {
public:
    Transaction(const std::string& name);
    
    void addCommand(std::unique_ptr<Command> command);
    std::unique_ptr<CompositeCommand> commit();
    void rollback();
    
    bool isEmpty() const { return m_commands.empty(); }
    size_t getCommandCount() const { return m_commands.size(); }
    const std::string& getName() const { return m_name; }
    
private:
    std::string m_name;
    std::vector<std::unique_ptr<Command>> m_commands;
    std::vector<std::unique_ptr<Command>> m_executedCommands;
};
```

## State Snapshots

### StateSnapshot
```cpp
class StateSnapshot {
public:
    StateSnapshot();
    
    // Capture current state
    void captureVoxelData(const MultiResolutionGrid& grid);
    void captureGroupData(const GroupManager& groups);
    void captureSelectionData(const SelectionManager& selection);
    void captureCameraData(const CameraManager& camera);
    void captureWorkspaceData(const WorkspaceSettings& workspace);
    
    // Restore state
    void restoreVoxelData(MultiResolutionGrid& grid) const;
    void restoreGroupData(GroupManager& groups) const;
    void restoreSelectionData(SelectionManager& selection) const;
    void restoreCameraData(CameraManager& camera) const;
    void restoreWorkspaceData(WorkspaceSettings& workspace) const;
    
    // Memory management
    size_t getMemoryUsage() const;
    void compress();
    void decompress();
    bool isCompressed() const { return m_compressed; }
    
    // Validation
    bool isValid() const;
    bool isComplete() const;
    
    // Serialization
    void serialize(BinaryWriter& writer) const;
    void deserialize(BinaryReader& reader);
    
private:
    struct VoxelSnapshot {
        std::array<std::vector<uint8_t>, 10> grids; // One per resolution
        Vector3f workspaceSize;
        VoxelResolution activeResolution;
    };
    
    struct GroupSnapshot {
        std::vector<GroupInfo> groups;
        std::vector<std::pair<GroupId, std::vector<VoxelId>>> groupMembers;
        GroupHierarchy hierarchy;
    };
    
    struct SelectionSnapshot {
        SelectionSet currentSelection;
        std::unordered_map<std::string, SelectionSet> namedSelections;
    };
    
    std::unique_ptr<VoxelSnapshot> m_voxelData;
    std::unique_ptr<GroupSnapshot> m_groupData;
    std::unique_ptr<SelectionSnapshot> m_selectionData;
    std::unique_ptr<CameraState> m_cameraData;
    std::unique_ptr<WorkspaceSettings> m_workspaceData;
    
    std::chrono::high_resolution_clock::time_point m_timestamp;
    bool m_compressed = false;
    size_t m_uncompressedSize = 0;
};
```

### Incremental Snapshots
```cpp
class IncrementalSnapshot {
public:
    IncrementalSnapshot(const StateSnapshot& baseSnapshot);
    
    void addVoxelChange(const Vector3i& position, VoxelResolution resolution, bool oldValue, bool newValue);
    void addGroupChange(const GroupChangeRecord& change);
    void addSelectionChange(const SelectionSet& oldSelection, const SelectionSet& newSelection);
    
    void applyChanges(StateSnapshot& snapshot) const;
    void reverseChanges(StateSnapshot& snapshot) const;
    
    size_t getMemoryUsage() const;
    bool isEmpty() const;
    
private:
    struct VoxelChange {
        Vector3i position;
        VoxelResolution resolution;
        bool oldValue;
        bool newValue;
    };
    
    std::vector<VoxelChange> m_voxelChanges;
    std::vector<GroupChangeRecord> m_groupChanges;
    std::optional<std::pair<SelectionSet, SelectionSet>> m_selectionChange;
    
    const StateSnapshot& m_baseSnapshot;
};
```

## Memory Management

### Memory Optimization
```cpp
class MemoryOptimizer {
public:
    void optimizeCommandHistory(std::vector<std::unique_ptr<Command>>& commands, size_t maxMemory);
    void compressOldCommands(std::vector<std::unique_ptr<Command>>& commands, int threshold);
    void mergeCompatibleCommands(std::vector<std::unique_ptr<Command>>& commands);
    void createSnapshots(std::vector<std::unique_ptr<Command>>& commands, int interval);
    
    size_t calculateMemoryUsage(const std::vector<std::unique_ptr<Command>>& commands) const;
    std::vector<size_t> getMemoryUsageByType(const std::vector<std::unique_ptr<Command>>& commands) const;
    
private:
    bool canMergeCommands(const Command& first, const Command& second) const;
    std::unique_ptr<Command> mergeCommands(std::unique_ptr<Command> first, std::unique_ptr<Command> second);
    void removeRedundantCommands(std::vector<std::unique_ptr<Command>>& commands);
};
```

### Command Compression
```cpp
class CommandCompressor {
public:
    bool compressCommand(Command& command);
    bool decompressCommand(Command& command);
    
    bool compressBulkVoxelEdit(BulkVoxelEditCommand& command);
    bool compressComposite(CompositeCommand& command);
    
    float getCompressionRatio() const { return m_lastCompressionRatio; }
    
private:
    float m_lastCompressionRatio = 1.0f;
    
    std::vector<uint8_t> compressVoxelChanges(const std::vector<VoxelChange>& changes);
    std::vector<VoxelChange> decompressVoxelChanges(const std::vector<uint8_t>& data);
    
    // Delta compression for similar commands
    std::vector<uint8_t> deltaCompress(const std::vector<uint8_t>& reference, const std::vector<uint8_t>& data);
    std::vector<uint8_t> deltaDecompress(const std::vector<uint8_t>& reference, const std::vector<uint8_t>& delta);
};
```

## Performance Considerations

### Fast Execution
- Lazy state capture until needed
- Incremental state differences
- Command merging for small operations
- Background compression

### Memory Efficiency
- Sparse representation for voxel changes
- Reference counting for shared data
- Automatic cleanup of old history
- Progressive compression

### Responsiveness
- Non-blocking undo/redo operations
- Progressive state restoration
- Priority-based memory cleanup
- Background optimization

## Events and Callbacks

### Event Types
```cpp
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

using UndoRedoCallback = std::function<void(const UndoRedoEvent&)>;
using MemoryPressureCallback = std::function<void(size_t currentUsage, size_t maxUsage)>;
```

## Testing Strategy

### Unit Tests
- Command execution and undo correctness
- Memory usage validation
- State snapshot accuracy
- Command merging logic
- Compression effectiveness

### Integration Tests
- Cross-subsystem command integration
- Transaction semantics
- Memory pressure handling
- Performance under load

### Stress Tests
- Large history management
- Memory limit enforcement
- Rapid undo/redo operations
- Complex composite commands

### Performance Tests
- Undo/redo speed benchmarks
- Memory usage optimization
- Compression performance
- State restoration speed

## Dependencies
- **Core/VoxelData**: Voxel state management
- **Core/Groups**: Group state management (not yet integrated)
- **Core/Selection**: Selection state management
- **Core/Camera**: Camera state management (not yet integrated)
- **Foundation/Memory**: Memory pool management (not utilized)
- **Foundation/Events**: Event system integration (partial)

## Refactoring Recommendations

### 1. Introduce Abstraction Layer
- Create interfaces for all manager dependencies
- Implement dependency injection for commands
- Use factory pattern for command creation

### 2. Implement Missing Components
- Add compression support for memory efficiency
- Create incremental snapshot system
- Implement memory optimizer with configurable strategies

### 3. Improve Thread Safety
- Use fine-grained locking or lock-free structures
- Implement async command execution
- Add command queuing for concurrent operations

### 4. Enhance Error Handling
- Add comprehensive exception handling
- Implement rollback strategies
- Create recovery mechanisms for corrupted state

### 5. Optimize Performance
- Add command indexing for fast lookup
- Implement lazy state capture
- Use memory pools for command allocation

### 6. Improve Testability
- Extract interfaces for all dependencies
- Add mock implementations for testing
- Increase test coverage for edge cases

## Platform Considerations

### Memory Constraints
- VR memory limits (Quest 3)
- Mobile memory optimization
- Desktop large history support
- Automatic adaptation

### Performance Optimization
- Platform-specific compression
- Memory-mapped snapshots
- Background processing
- Priority-based cleanup