# Voxel Rendering Debug Plan - Part 3

## Context
This is part 3 of 3 parallel TODO lists for the voxel rendering debug work.
The main rendering issue has been resolved - voxels were rendering but too dark to see.

## Remaining Tasks - Part 3 (Screenshot Analysis & Documentation)

### 5.1 PPM Output Validation
- [ ] **Add command**: `screenshot analyze` - Count colored pixels
- [ ] **Add debug overlay**: Draw coordinate axes
- [ ] **Add debug overlay**: Draw frustum bounds
- [ ] **Compare outputs**: Immediate mode vs shader rendering

### Documentation & Cleanup
- [ ] **Document**: Update ARCHITECTURE.md with rendering pipeline details
- [ ] **Document**: Create DEBUGGING.md with common rendering issues and solutions
- [ ] **Clean up**: Remove temporary debug code and logging
- [ ] **Optimize**: Profile and optimize rendering hot paths
- [ ] **Test coverage**: Ensure all new code has unit tests

### Performance Analysis
- [ ] **Profile**: Measure frame time breakdown
- [ ] **Benchmark**: Test with different voxel counts
- [ ] **Memory**: Check for GPU memory leaks
- [ ] **Optimization**: Implement frustum culling if not already done

## Notes for Part 3
- Focus on analysis tools and documentation
- These tasks can be done independently of Part 1 and Part 2
- Screenshot analysis helps validate rendering correctness
- Documentation ensures future maintainability

## Success Criteria
The debugging is complete when:
1. Yellow voxels appear in the screenshot at expected positions
2. All unit tests pass
3. Debug commands show consistent state
4. No OpenGL errors are reported

## Running the Debug Plan
When you run the voxel-cli, please make sure you send a 'quit' command at the end.

```bash
# 1. Build with debug logging
cmake -B build_debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug

# 2. Run core tests
cd build_debug && ctest -V

# 3. Run CLI with debug commands
./voxel-cli
> debug camera
> debug voxels
> place 2 2 2
> debug render
> screenshot debug.ppm

# 4. Analyze results
python3 analyze_debug.py debug.ppm
```