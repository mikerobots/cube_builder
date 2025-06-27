# Surface Generation Test Optimization Notes

## Current Status
- All tests are passing after fixing coordinate system issues
- Tests are extremely slow (8-16 seconds per test)
- Main issue: Dual contouring algorithm appears to be inefficient

## Fixes Applied
1. Fixed CMakeLists.txt to include missing test file
2. Converted all Vector3i voxel positions to IncrementCoordinates
3. Fixed compilation errors in test_unit_core_surface_gen_generator_optimized.cpp

## Performance Issues
- EmptyGrid test: ~9 seconds (should be instant)
- SingleVoxel test: ~9 seconds 
- SimpleCube test: ~9.4 seconds
- AdaptiveError test: ~16.5 seconds

## Recommendations
1. Profile the DualContouring::generateMesh method
2. Consider using Preview settings for unit tests
3. Move expensive tests to integration test suite
4. Add fast smoke tests for basic functionality
5. Cache intermediate results in dual contouring algorithm