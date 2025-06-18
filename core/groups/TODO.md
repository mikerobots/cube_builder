# Groups Subsystem TODO

## 🚨 CRITICAL: COORDINATE SYSTEM MIGRATION REQUIRED

**IMPORTANT**: The foundation coordinate system has been simplified, but this subsystem still uses the old GridCoordinates system and needs immediate updating.

### 📖 REQUIRED READING
**BEFORE STARTING**: Read `/coordinate.md` in the root directory to understand the new simplified coordinate system.

### 🎯 Migration Overview
Update the Groups subsystem from the old GridCoordinates system to the new simplified coordinate system:
- **OLD**: GridCoordinates with complex grid-to-world conversions
- **NEW**: IncrementCoordinates (1cm granularity) for all voxel operations, centered at origin (0,0,0)

### 📋 Migration Tasks (HIGH PRIORITY)

#### Phase 1: Remove GridCoordinates Dependencies ✅ COMPLETED
- [x] **Update GroupTypes.h** - ✅ DONE: Replaced GridCoordinates with IncrementCoordinates in VoxelId struct
- [x] **Update VoxelGroup.h** - ✅ DONE: Already compatible with new coordinate system
- [x] **Update GroupOperations.h** - ✅ DONE: Already compatible with new coordinate system
- [x] **Update GroupManager.h** - ✅ DONE: Already compatible with new coordinate system

#### Phase 2: Update Implementation Files ✅ COMPLETED
- [x] **Update VoxelGroup.cpp** - ✅ DONE: Updated coordinate conversion calls for centered coordinates
- [x] **Update GroupOperations.cpp** - ✅ DONE: Fixed all coordinate conversion methods and parameter counts
- [x] **Update GroupManager.cpp** - ✅ DONE: Compiles without GridCoordinates errors
- [x] **Update GroupHierarchy.cpp** - ✅ DONE: Compiles without GridCoordinates errors

#### Phase 3: Update Tests
- [ ] **TestVoxelGroup.cpp** - Update group tests for IncrementCoordinates
- [ ] **TestGroupOperations.cpp** - Update operation tests for centered coordinates
- [ ] **TestGroupManager.cpp** - Update manager tests for new coordinate system
- [ ] **TestGroupHierarchy.cpp** - Update hierarchy tests for IncrementCoordinates
- [ ] **TestGroupTypes.cpp** - Update type tests for new coordinate system

#### Phase 4: Validation
- [ ] **Compile Check** - Ensure all files compile without GridCoordinates errors
- [ ] **Unit Tests** - Run `cd build_ninja && ctest -R "VoxelEditor_Groups_Tests"`
- [ ] **Fix Issues** - Address any failing tests or compilation errors

### 🔧 Key Code Changes Required

```cpp
// OLD - Remove all instances of:
GridCoordinates gridPos;
convertWorldToGrid();
convertGridToWorld();
#include "GridCoordinates.h"

// NEW - Replace with:
IncrementCoordinates voxelPos;
CoordinateConverter::worldToIncrement();
CoordinateConverter::incrementToWorld();
#include "foundation/math/CoordinateConverter.h"
```

### 🎯 Groups-Specific Changes

#### VoxelGroup Updates
- Update `VoxelGroup` to store voxel positions as IncrementCoordinates
- Ensure group bounds calculations work with centered coordinate system
- Update group operations (move, rotate, scale) for IncrementCoordinates

#### Group Operations Updates
- Update `GroupOperations::moveGroup()` for centered coordinate system
- Ensure transformation operations work with IncrementCoordinates
- Update group collision detection for centered coordinates

#### Group Manager Updates
- Update `GroupManager` to handle IncrementCoordinates in group operations
- Ensure group creation and management work with centered system
- Update group hierarchy for IncrementCoordinates

### 🎯 Success Criteria
- ✅ All GridCoordinates references removed
- ✅ All group operations use IncrementCoordinates
- ✅ Group bounds and operations work with centered coordinate system
- ✅ All files compile without coordinate system errors
- ✅ All Groups unit tests pass

**PRIORITY**: HIGH - Groups system is important for user workflow

---

## Overview
The groups subsystem manages voxel grouping, operations, and metadata. It provides functionality for organizing voxels into logical groups with hierarchical relationships.

## Current Implementation Issues

### Missing Features
1. **Rotation Operations Not Implemented**
   - Location: `VoxelGroup::rotate()` and `GroupOperations::transformVoxel()`
   - Status: Stub methods with TODO comments
   - Impact: Cannot rotate voxel groups
   - Required: Implement 3D rotation with proper voxel grid snapping

2. **Scaling Operations Not Implemented**  
   - Location: `VoxelGroup::scale()` and `GroupOperations::scaleGroup()`
   - Status: Basic integer scaling only, TODO for sophisticated resampling
   - Impact: Limited scaling functionality
   - Required: Implement proper voxel resampling for non-integer scales

### Code Quality Issues
1. **Incomplete Transformation Pipeline**
   - The `transformVoxel()` method only applies translation
   - Rotation and scale transforms are ignored
   - Need complete 3x3 or 4x4 transformation matrix support

2. **Thread Safety Concerns**
   - Using raw mutex locks instead of RAII patterns consistently
   - Potential for deadlocks in hierarchical operations
   - Consider using shared_mutex for read operations

3. **Memory Management**
   - Groups store full voxel lists which could be memory intensive
   - No lazy loading or streaming for large groups
   - Consider storing only voxel IDs and fetching data on demand

4. **Bug: MoveGroupOperation Double Translation** (FIXED)
   - Was manually moving voxels AND calling group->translate()
   - This caused voxels to be duplicated instead of moved
   - Fixed by only updating VoxelDataManager and letting translate() handle group updates

### Design Issues
1. **Coupling with VoxelDataManager**
   - Direct dependency makes testing harder
   - Consider dependency injection or interface abstraction

2. **Event System Integration**
   - Events are fired for every operation, no batching
   - Could cause performance issues with large operations

3. **No Serialization Support**
   - Groups cannot be saved/loaded
   - Need integration with file I/O system

## Testing Gaps
1. No performance tests for large groups
2. Limited concurrency testing
3. No tests for error recovery
4. Missing integration tests with rendering

## Priority Fixes
1. **High**: Implement basic rotation (at least 90-degree increments)
2. **High**: Add serialization support for persistence  
3. **Medium**: Improve scaling with proper resampling
4. **Medium**: Add performance optimizations for large groups
5. **Low**: Implement full transformation matrix support

## API Improvements Needed
1. Add bulk operations API to reduce event spam
2. Add group querying methods (by name, by metadata, etc.)
3. Add group statistics (voxel count, bounds, etc.)
4. Add undo/redo integration