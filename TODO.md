# TODO - Core Subsystem Review and Cleanup

## ⚠️ IMPORTANT: MULTIPLE AGENTS WORKING CONCURRENTLY ⚠️

**This TODO list is being worked on by MULTIPLE AGENTS simultaneously.**

### Before Starting Work:
1. **CHECK STATUS FIRST**: Only work on subsystems marked as `[ ]` (not started)
2. **SKIP IN-PROGRESS ITEMS**: If a subsystem is marked as `[IN PROGRESS]`, DO NOT work on it
3. **MARK YOUR WORK**: Immediately mark a subsystem as `[IN PROGRESS]` when you start
4. **ONE AT A TIME**: Only work on ONE subsystem at a time

### Process for Each Subsystem

For each subsystem in the core folder:

1. **Review Changes**: Look at git diffs to identify any hacks or temporary fixes
2. **Document Issues**: Record identified hacks in the subsystem's TODO.md file
3. **Update Documentation**: Update the subsystem's DESIGN.md to reflect current implementation
4. **Run Tests**: Execute the subsystem's test suite
5. **Fix Issues**: Address all test failures and documented hacks
6. **Commit Changes**: Create a clean commit for the subsystem
7. **Update Progress**: Mark subsystem as complete in this file

## Subsystems to Review

### Camera System (`core/camera/`)
- [x] Review diffs for hacks
- [x] Update camera/TODO.md with issues
- [x] Update camera/DESIGN.md
- [x] Run camera tests
- [x] Fix all issues
- [x] Commit changes

### Groups System (`core/groups/`)
- [x] Review diffs for hacks
- [x] Update groups/TODO.md with issues
- [x] Update groups/DESIGN.md
- [x] Run groups tests
- [x] Fix all issues
- [x] Commit changes

### Input System (`core/input/`) [IN PROGRESS]
- [ ] Review diffs for hacks
- [ ] Update input/TODO.md with issues
- [ ] Update input/DESIGN.md
- [ ] Run input tests
- [ ] Fix all issues
- [ ] Commit changes

### Rendering System (`core/rendering/`) [IN PROGRESS]
- [ ] Review diffs for hacks
- [ ] Update rendering/TODO.md with issues
- [ ] Update rendering/DESIGN.md
- [ ] Run rendering tests
- [ ] Fix all issues
- [ ] Commit changes

### Selection System (`core/selection/`) [IN PROGRESS]
- [ ] Review diffs for hacks
- [ ] Update selection/TODO.md with issues
- [ ] Update selection/DESIGN.md
- [ ] Run selection tests
- [ ] Fix all issues
- [ ] Commit changes

### Surface Generation (`core/surface_gen/`) [IN PROGRESS]
- [ ] Review diffs for hacks
- [ ] Update surface_gen/TODO.md with issues
- [ ] Update surface_gen/DESIGN.md
- [ ] Run surface_gen tests
- [ ] Fix all issues
- [ ] Commit changes

### Undo/Redo System (`core/undo_redo/`)
- [ ] Review diffs for hacks
- [ ] Update undo_redo/TODO.md with issues
- [ ] Update undo_redo/DESIGN.md
- [ ] Run undo_redo tests
- [ ] Fix all issues
- [ ] Commit changes

### Visual Feedback System (`core/visual_feedback/`)
- [ ] Review diffs for hacks
- [ ] Update visual_feedback/TODO.md with issues
- [ ] Update visual_feedback/DESIGN.md
- [ ] Run visual_feedback tests
- [ ] Fix all issues
- [ ] Commit changes

### Voxel Data System (`core/voxel_data/`)
- [ ] Review diffs for hacks
- [ ] Update voxel_data/TODO.md with issues
- [ ] Update voxel_data/DESIGN.md
- [ ] Run voxel_data tests
- [ ] Fix all issues
- [ ] Commit changes


### File I/O System (`core/file_io/`)
- [ ] Review diffs for hacks
- [ ] Update file_io/TODO.md with issues
- [ ] Update file_io/DESIGN.md
- [ ] Run file_io tests
- [ ] Fix all issues
- [ ] Commit changes


## Completed Subsystems

### Camera System (`core/camera/`) - 2025-06-16
- Fixed perspective divide issues in ray generation
- Eliminated magic numbers with named constants
- Fixed frame-rate dependent smoothing
- All 108 tests passing
- Committed with message: "Fix camera subsystem code quality issues"

### Groups System (`core/groups/`) - 2025-06-16
- Fixed MoveGroupOperation bug duplicating voxels on undo
- Created TODO.md documenting missing features (rotation/scaling)
- Updated DESIGN.md with current implementation status
- All 75 tests passing
- Committed with message: "Fix groups subsystem issues and update documentation"