# Workspace Boundary Test Issue Summary

## The Hang Issue - FIXED ✓

### Root Cause
The test was hanging due to a **deadlock** in `VoxelDataManager::setVoxelAtWorldPos()`.

### The Deadlock
1. `setVoxelAtWorldPos()` acquires the mutex with `std::lock_guard<std::mutex> lock(m_mutex)`
2. Inside the method, it calls `getWorkspaceSize()`
3. `getWorkspaceSize()` also tries to acquire the same mutex
4. Since `std::mutex` is not recursive, this causes a deadlock

### The Fix
Replace `getWorkspaceSize()` with direct access to `m_workspaceManager->getSize()` inside methods that already hold the lock.

```cpp
// Before (deadlock):
Math::Vector3f workspaceSize = getWorkspaceSize();

// After (fixed):
Math::Vector3f workspaceSize = m_workspaceManager->getSize();
```

## Remaining Issues - Coordinate System Mismatch

The workspace boundary tests are still failing because of a fundamental coordinate system mismatch:

### Current State
1. **WorkspaceManager** uses a centered coordinate system:
   - X: -size.x/2 to +size.x/2
   - Y: 0 to size.y
   - Z: -size.z/2 to +size.z/2

2. **VoxelGrid** uses a non-centered coordinate system:
   - X: 0 to size.x
   - Y: 0 to size.y
   - Z: 0 to size.z

3. **Tests** expect the centered coordinate system (matching WorkspaceManager)

### Impact
- Negative world coordinates are rejected by VoxelGrid
- Tests expecting to place voxels at negative coordinates fail
- The workspace appears shifted by half its size in X and Z

### Recommended Fix
Update VoxelGrid to use the centered coordinate system to match WorkspaceManager:

1. Update `VoxelGrid::isValidWorldPosition()` to check centered bounds
2. Update `VoxelGrid::worldToGrid()` to handle the coordinate offset
3. Update `VoxelGrid::gridToWorld()` to apply the offset in reverse

See `COORDINATE_SYSTEM_FIX.md` for detailed implementation.

## Lessons Learned

1. **Mutex Usage**: Be very careful about calling methods that acquire locks from within methods that already hold locks. Consider:
   - Using recursive mutexes (`std::recursive_mutex`) if needed
   - Creating internal versions of methods that don't lock
   - Documenting which methods acquire locks

2. **Coordinate Systems**: Ensure all components use the same coordinate system. Document clearly whether the system is centered or corner-based.

3. **Debug Output**: When debugging hangs, use direct output (cout) rather than logging systems that might also use locks.