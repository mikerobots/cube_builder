# Groups Subsystem TODO

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