# Selection Subsystem Performance Testing

## Overview
The Selection subsystem includes comprehensive performance tests to ensure operations scale well with large datasets. These tests are separated from regular unit tests to avoid slowing down the standard test suite.

## Running Performance Tests

### Quick Start
```bash
# Run all performance tests
./run_performance.sh

# Run regular tests (excludes performance tests)
./run_tests.sh
```

## Performance Test Categories

### 1. Large Selection Operations (`TestSelectionPerformance.cpp`)
- **LargeSelectionAddRemove**: Tests adding/removing 10,000 voxels
- **SelectionSetOperations**: Tests set operations (union, intersection) with 5,000 element sets
- **UndoRedoLargeSelections**: Tests undo/redo with 100 history entries of 1,000 voxels each

### 2. Selector Performance
- **BoxSelectorHighResolution**: Tests box selection with 1cm resolution (500,000+ voxels)
- **SphereSelectorVaryingRadii**: Tests sphere selection with increasing radii
- **SelectFromSphere_VeryLargeRadius**: Tests sphere selection with 10m radius

### 3. Stress Tests
- **FloodFillMaximumStress**: Tests flood fill hitting the 10,000 voxel limit

## Performance Expectations

### Time Limits
- Large selection operations: < 5 seconds
- Stats calculations: < 500ms
- Clear operations: < 100ms
- Set operations: < 1 second
- Undo/redo: < 2 seconds per batch

### Memory Usage
- Efficient sparse storage for large selections
- Memory pooling for frequently allocated objects
- Proper cleanup after operations

## Adding New Performance Tests

1. Add tests to `TestSelectionPerformance.cpp`
2. Follow naming convention: include "Performance", "Stress", or "Large" in test name
3. Set reasonable time limits based on operation complexity
4. Document expected performance characteristics

## Troubleshooting

### Tests Timing Out
- Check if voxel limits are set appropriately
- Verify test is using appropriate resolution (higher resolution = more voxels)
- Consider breaking test into smaller chunks

### Memory Issues
- Monitor memory usage during tests
- Ensure proper cleanup between test cases
- Use memory pooling for large allocations

## Performance Optimization Tips

1. **Use appropriate data structures**
   - std::unordered_set for O(1) lookups
   - Spatial indexing for region queries

2. **Batch operations**
   - Group multiple selections before updating
   - Use bulk operations instead of individual adds

3. **Resolution awareness**
   - Use lower resolution for large area selections
   - Implement LOD for visualization