# Undo/Redo Subsystem - TODO

## Current Status (Updated: January 2025)
The undo/redo subsystem is partially implemented with basic functionality working but several issues identified:

### Working Components ✅
- **HistoryManager**: Core undo/redo operations with history management
- **Command Interface**: Base command pattern implementation  
- **VoxelCommands**: Basic voxel edit and bulk edit commands
- **SelectionCommands**: Selection modification commands
- **CompositeCommand**: Command grouping functionality
- **Transaction**: Command transaction support
- **PlacementCommands**: Voxel placement/removal with validation (added recently)

### Issues Found 🐛
1. **Test Infrastructure Problem**: PlacementCommand tests fail with mutex errors
   - Tests use `reinterpret_cast` from MockVoxelDataManager to VoxelDataManager*
   - This is undefined behavior and causes crashes when real methods are called
   - Need proper mock/interface for VoxelDataManager

2. **Missing getWorkspaceSize()**: PlacementCommands expects this method on VoxelDataManager
   - Used for bounds validation in placement operations
   - Either mock needs this method or commands need refactoring

3. **Architecture Issues** (from DESIGN.md):
   - Circular dependencies between commands and managers
   - Tight coupling without abstraction layer
   - Missing dependency injection
   - No proper interfaces for testing

### Not Implemented ❌
- Camera commands
- Group commands  
- Workspace commands
- Memory optimization/compression
- Incremental snapshots
- Command indexing

## Requirements Validation
**Total Requirements**: 3

## Requirements to Validate

### Command Creation
- [ ] REQ-2.3.3: Command creation for placements
  - Clicking on a highlighted face shall place the new voxel adjacent to that face
  - Requires: PlacementCommand for face-based placement
  
- [ ] REQ-5.1.1: Command creation for voxel placement
  - Left-click shall place a voxel at the current preview position
  - Requires: PlaceVoxelCommand or similar
  
- [ ] REQ-5.1.2: Command creation for voxel removal
  - Right-click on a voxel shall remove that voxel
  - Requires: RemoveVoxelCommand or similar

## API Review Checklist

### HistoryManager
- [ ] Check executeCommand() accepts placement commands
- [ ] Check undo() properly reverses voxel placement
- [ ] Check redo() properly re-applies voxel placement
- [ ] Check history size limits (5-10 operations as per DESIGN.md)

### Command Interface
- [ ] Check base Command interface supports execute/undo
- [ ] Check command serialization for state restoration

### VoxelCommands
- [ ] Check PlaceVoxelCommand exists and works
- [ ] Check RemoveVoxelCommand exists and works
- [ ] Check commands store necessary state (position, resolution, etc.)

### PlacementCommands (May need to create)
- [ ] Check face-based placement command
- [ ] Check grid-based placement command
- [ ] Check proper state capture for undo

## Implementation Tasks
1. Review existing VoxelCommands implementation
2. Create/update PlacementCommands if needed:
   - FacePlacementCommand - for placing on voxel faces
   - GridPlacementCommand - for placing on ground grid
3. Ensure commands capture all necessary state:
   - Position (with 1cm precision)
   - Resolution
   - Adjacent face (for face placement)
   - Timestamp
4. Test undo/redo functionality

## Test Coverage Needed
1. Place voxel command tests
2. Remove voxel command tests
3. Face-based placement command tests
4. Undo/redo sequence tests
5. History limit tests
6. Command state restoration tests