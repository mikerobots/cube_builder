# Selection System TODO

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