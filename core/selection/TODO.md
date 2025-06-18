# Selection System TODO

## 🚨 CRITICAL: COORDINATE SYSTEM MIGRATION REQUIRED

**IMPORTANT**: The foundation coordinate system has been simplified, but this subsystem still uses the old GridCoordinates system and needs immediate updating.

### 📖 REQUIRED READING
**BEFORE STARTING**: Read `/coordinate.md` in the root directory to understand the new simplified coordinate system.

### 🎯 Migration Overview
Update the Selection subsystem from the old GridCoordinates system to the new simplified coordinate system:
- **OLD**: GridCoordinates with complex grid-to-world conversions
- **NEW**: IncrementCoordinates (1cm granularity) for all voxel operations, centered at origin (0,0,0)

### 📋 Migration Tasks (HIGH PRIORITY)

#### Phase 1: Remove GridCoordinates Dependencies ✅ COMPLETED
- [x] **Update SelectionTypes.h** - ✅ DONE: Replaced GridCoordinates with IncrementCoordinates in VoxelId struct
- [ ] **Update SelectionSet.h** - Use IncrementCoordinates for voxel positions in selections
- [ ] **Update BoxSelector.h** - Use IncrementCoordinates for box selection bounds
- [ ] **Update SphereSelector.h** - Use IncrementCoordinates for sphere selection center
- [ ] **Update FloodFillSelector.h** - Use IncrementCoordinates for flood fill operations

#### Phase 2: Update Implementation Files ✅ MOSTLY COMPLETED
- [x] **Update SelectionTypes.cpp** - ✅ DONE: Updated coordinate conversion methods
- [ ] **Update SelectionSet.cpp** - Update voxel position handling with IncrementCoordinates
- [x] **Update SelectionManager.cpp** - ✅ DONE: Fixed voxelPos.gridPos to voxelPos.incrementPos
- [x] **Update BoxSelector.cpp** - ✅ DONE: Updated coordinate conversion for centered coordinates
- [x] **Update SphereSelector.cpp** - ✅ DONE: Removed invalid coordinate conversion calls
- [x] **Update FloodFillSelector.cpp** - ✅ DONE: Removed invalid coordinate conversion calls
- [ ] **Update SelectionRenderer.cpp** - Use IncrementCoordinates for selection rendering

#### Phase 3: Update Tests
- [ ] **TestSelectionTypes.cpp** - Update tests for IncrementCoordinates
- [ ] **TestSelectionSet.cpp** - Update selection set tests for centered coordinates
- [ ] **TestSelectionManager.cpp** - Update manager tests for new coordinate system
- [ ] **TestBoxSelector.cpp** - Update box selection tests for centered coordinates
- [ ] **TestSphereSelector.cpp** - Update sphere selection tests for centered coordinates
- [ ] **TestFloodFillSelector.cpp** - Update flood fill tests for IncrementCoordinates

#### Phase 4: Validation
- [ ] **Compile Check** - Ensure all files compile without GridCoordinates errors
- [ ] **Unit Tests** - Run `cd build_ninja && ctest -R "VoxelEditor_Selection_Tests"`
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

### 🎯 Selection-Specific Changes

#### Box Selection Updates
- Update `BoxSelector` to use IncrementCoordinates for selection bounds
- Ensure box selection works correctly with negative coordinates (centered system)
- Update bounds checking for centered workspace

#### Sphere Selection Updates
- Update `SphereSelector` to use IncrementCoordinates for center position
- Ensure sphere selection works with centered coordinate system
- Update distance calculations for IncrementCoordinates

#### Selection Set Updates
- Update `SelectionSet` to store voxel positions as IncrementCoordinates
- Ensure selection serialization/deserialization works with new coordinates
- Update selection operations for centered coordinate system

### 🎯 Success Criteria
- ✅ All GridCoordinates references removed
- ✅ All selections use IncrementCoordinates
- ✅ Selection algorithms work with centered coordinate system
- ✅ All files compile without coordinate system errors
- ✅ All Selection unit tests pass

**PRIORITY**: HIGH - Selection system is core to user interaction

---

## Issues Found During Code Review

### Magic Numbers and Hardcoded Values
- [ ] Replace hardcoded history size limit (100) in SelectionManager with named constant
- [ ] Document or extract magic numbers in SelectionRenderer:
  - [ ] Cylinder segments (16)
  - [ ] Marker size multiplier (0.05f)
  - [ ] Indicator size constants (0.1f, 0.01f)
  - [ ] Animation timing constants (2.0f for logging, 2.0f for pulse)
  - [ ] Golden ratio calculation magic number
- [ ] Extract default workspace bounds in BoxSelector to constants
- [ ] Make max voxels limit in FloodFillSelector configurable

### Code Quality Issues
- [ ] Remove unused inline shader code in SelectionRenderer (lines 634-684)
- [ ] Fix shader loading to use inline shaders or ensure shader files exist
- [ ] Optimize trimHistory() in SelectionManager to avoid temporary stack
- [ ] Remove testing assumptions in selectors (BoxSelector, SphereSelector, FloodFillSelector)
  - "For testing: assume all voxels exist when no manager"

### Missing Features
- [ ] Implement proper shader resource management in SelectionRenderer
- [ ] Add error handling for rendering operations
- [ ] Add version migration path for SelectionSet serialization
- [ ] Implement text rendering for selection info display (TODO at line 588)

### Performance Optimizations
- [ ] Cache rendering resources instead of creating/destroying on each call
- [ ] Use vertex buffer objects for selection rendering
- [ ] Batch rendering operations where possible

### Consistency Issues
- [ ] Standardize error handling across all selector classes
- [ ] Ensure consistent nullptr checking patterns
- [ ] Unify immediate/retained mode rendering approach

## Future Enhancements
- [ ] Add selection groups/layers
- [ ] Implement selection masks
- [ ] Add more selection shapes (lasso, polygon)
- [ ] Support for selection feathering/soft edges
- [ ] Selection history visualization