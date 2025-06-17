# TODO - Coordinate System Fixes

## 🎯 OBJECTIVE
Fix all coordinate system issues to resolve integration test failures. All subsystems must use the centered coordinate system consistently.

**Coordinate System**: Workspace extends from `(-size/2, 0, -size/2)` to `(size/2, size, size/2)` with origin at center.

## 📋 AGENT INSTRUCTIONS

### Before Starting:
1. **CLAIM YOUR SUBSYSTEM**: Update status to "🔄 IN PROGRESS - Agent: [Your Name]"
2. **Focus on Critical Files**: Start with files marked as CRITICAL
3. **Test After Each Fix**: Run unit tests for the subsystem to ensure 100% pass rate
4. **Verify Integration**: Run integration tests to verify fixes don't break other systems

### Common Fix Pattern:
Replace: `worldPos = gridPos * voxelSize`  
With: `worldPos = voxelGrid->gridToWorld(gridPos)`

## 🚨 CRITICAL PRIORITY (Integration Test Blockers)

### 1. Input System - PlacementValidation.cpp ⚠️ CRITICAL
**Status**: ⏳ PENDING  
**Agent**: None  
**File**: `core/input/PlacementValidation.cpp`

**Issues**:
- Line 45-47: `if (gridPos.y < 0) return InvalidYBelowZero;` - Prevents placement below Y=0
- Line 59: Workspace bounds check uses `worldPos.y < 0` instead of centered bounds  
- Line 72: `isValidIncrementPosition()` enforces `pos.y >= 0` constraint
- Line 302-304: Another hardcoded `worldPos.y < 0.0f` check

**Fix**: Remove all Y >= 0 constraints. Update workspace bounds checks to use centered coordinates.

**Unit Tests**: After fixing, run:
```bash
cd build_ninja && ctest -R "VoxelEditor_Input_Tests" --output-on-failure
```

### 2. Visual Feedback System - FeedbackTypes.cpp ⚠️ CRITICAL  
**Status**: ⏳ PENDING  
**Agent**: None  
**File**: `core/visual_feedback/src/FeedbackTypes.cpp`

**Issues**:
- Line 65: `Face::getWorldPosition()` uses `basePos = Vector3f(position) * voxelSize`
- Lines 88-132: `Face::getCorners()` same coordinate conversion issue

**Fix**: Use VoxelGrid's `gridToWorld()` method for proper coordinate conversion.

**Unit Tests**: After fixing, run:
```bash
cd build_ninja && ctest -R "VoxelEditor_VisualFeedback_Tests" --output-on-failure
```

### 3. Application Layer - VoxelMeshGenerator.cpp ⚠️ CRITICAL
**Status**: ⏳ PENDING  
**Agent**: None  
**File**: `apps/cli/VoxelMeshGenerator.cpp`

**Issues**:
- Lines 69-73: `worldPos = gridPos * voxelSize + voxelSize * 0.5f`
- Lines 184-188: Same issue in edge mesh generation

**Fix**: Use VoxelGrid's `gridToWorld()` method. This is the ROOT CAUSE of voxels appearing in wrong positions.

**Unit Tests**: After fixing, run:
```bash
cd build_ninja && ctest -R "VoxelEditor_VoxelMeshGenerator_Tests" --output-on-failure
```

---

## 🔥 HIGH PRIORITY

### 4. VoxelData Management - VoxelTypes.h
**Status**: ⏳ PENDING  
**Agent**: None  
**File**: `core/voxel_data/VoxelTypes.h`

**Issues**:
- Lines 59-70: `VoxelPosition::toWorldSpace()` assumes old coordinate system
- Lines 73-86: `VoxelPosition::fromWorldSpace()` same issue

**Fix**: Update methods to handle centered coordinates or remove them entirely.

**Unit Tests**: After fixing, run:
```bash
cd build_ninja && ctest -R "VoxelEditor_VoxelData_Tests" --output-on-failure
```

### 5. Visual Feedback System - Multiple Files
**Status**: ⏳ PENDING  
**Agent**: None  

**Files to Fix**:
- `core/visual_feedback/src/FeedbackRenderer.cpp` (lines 30-33): Hardcoded workspace bounds
- `core/visual_feedback/src/HighlightRenderer.cpp` (lines 416-436): `calculateVoxelTransform()`  
- `core/visual_feedback/src/OutlineRenderer.cpp` (lines 267-279): `addVoxelEdges()`

**Fix**: Replace manual coordinate conversion with VoxelGrid methods.

**Unit Tests**: After fixing, run:
```bash
cd build_ninja && ctest -R "VoxelEditor_VisualFeedback_Tests" --output-on-failure
```

### 6. Surface Generation - DualContouring.cpp
**Status**: ⏳ PENDING  
**Agent**: None  
**File**: `core/surface_gen/DualContouring.cpp`

**Issues**:
- Line 231: `Math::Vector3f worldVertex = vertex * voxelSize;`

**Fix**: Use VoxelGrid's `gridToWorld()` method for proper vertex positioning.

**Unit Tests**: After fixing, run:
```bash
cd build_ninja && ctest -R "VoxelEditor_SurfaceGen_Tests" --output-on-failure
```

---

## 🔧 MEDIUM PRIORITY

### 7. Selection System - Multiple Files
**Status**: ⏳ PENDING  
**Agent**: None  

**Files to Fix**:
- `core/selection/SelectionTypes.cpp` (lines 8-12, 21-25): VoxelId coordinate methods
- `core/selection/BoxSelector.cpp` (lines 74-78): Hardcoded workspace bounds  
- `core/selection/SphereSelector.cpp` (lines 68-71): Same workspace bounds issue

**Fix**: Use WorkspaceManager's centered bounds instead of hardcoded `(0,0,0)` to `workspaceSize`.

**Unit Tests**: After fixing, run:
```bash
cd build_ninja && ctest -R "VoxelEditor_Selection_Tests" --output-on-failure
```

### 8. Groups System - VoxelGroup.cpp  
**Status**: ⏳ PENDING  
**Agent**: None  
**File**: `core/groups/src/VoxelGroup.cpp`

**Issues**:
- Line 112: `translate()` method uses `voxel.position * voxelSize`
- Lines 176-177: `updateBounds()` method same issue

**Fix**: Use VoxelGrid's coordinate conversion or account for workspace center.

**Unit Tests**: After fixing, run:
```bash
cd build_ninja && ctest -R "VoxelEditor_Groups_Tests" --output-on-failure
```

### 9. Undo/Redo System - Multiple Files
**Status**: ⏳ PENDING  
**Agent**: None  

**Files to Fix**:
- `core/undo_redo/VoxelCommands.cpp` (lines 233-245): VoxelFillCommand coordinate calculations
- `core/undo_redo/StateSnapshot.cpp` (lines 50-72, 188-196): Raw coordinate storage
- `core/undo_redo/PlacementCommands.cpp` (lines 80, 103): Uses broken PlacementUtils

**Fix**: Update coordinate calculations for negative space and add coordinate system versioning.

**Unit Tests**: After fixing, run:
```bash
cd build_ninja && ctest -R "VoxelEditor_UndoRedo_Tests" --output-on-failure
```

---

## 🎯 LOW PRIORITY (CLI Application - Do Last)

### 10. Application Layer - CLI Commands  
**Status**: ⏳ PENDING  
**Agent**: None  

**Files to Fix**:
- `apps/cli/Commands.cpp` (multiple lines): Camera centering and debug commands
- `apps/cli/MouseInteraction.cpp` (lines 442-443, 658-662): Placement position calculation

**Fix**: Replace manual coordinate conversions with VoxelGrid methods.

**Unit Tests**: After fixing, run:
```bash
cd build_ninja && ctest -R "VoxelEditor_CLI_Tests" --output-on-failure
```

---

## 📊 COMPLETION TRACKING

**Total Fixes Needed**: 10 subsystems  
**Critical (Blocking Tests)**: 3  
**High Priority**: 3  
**Medium Priority**: 3  
**Low Priority**: 1  

**Status**: 0 Complete, 10 Pending

---

## 🧪 TESTING VALIDATION

**CRITICAL**: After fixing each subsystem:

1. **Run Unit Tests** (must pass 100%):
```bash
cd build_ninja && ctest -R "VoxelEditor_[Subsystem]_Tests" --output-on-failure
```

2. **Run Integration Tests** (verify no regressions):
```bash
./run_integration_tests.sh core
```

3. **Run Failing Tests** (verify fixes work):
```bash
cd build_ninja && ctest -R "test_click_voxel_placement" --output-on-failure
cd build_ninja && ctest -R "test_mouse_ground_plane_clicking" --output-on-failure
```

**DO NOT PROCEED** to next subsystem until current subsystem's unit tests pass 100%.

## 🎯 SUCCESS CRITERIA

All coordinate system fixes are complete when:
1. All integration tests pass
2. Click placement tests work correctly  
3. Voxels render in correct positions
4. Mouse interaction works at all workspace locations
5. Visual feedback appears at correct positions