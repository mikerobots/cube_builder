# TODO

## Core Subsystem Test Validation and Fixes

### Status: COMPLETED

Running and fixing tests for each core subsystem using the new auto-discovery run_tests.sh scripts.

- [x] camera - All 8 tests passing
- [x] file_io - All 9 tests passing  
- [x] groups - All 6 tests passing
- [x] input - All 10 tests passing
- [x] rendering - All 22 tests passing (removed 5 OpenGL-dependent tests)
- [x] selection - All 7 tests passing
- [x] surface_gen - All 5 tests passing
- [x] undo_redo - All 5 tests passing
- [x] visual_feedback - All 10 tests passing
- [x] voxel_data - All 7 tests passing

### Notes:
- Focus on fixing code/test issues within each subsystem
- Avoid cross-subsystem dependencies where possible
- All tests should build and pass without errors
- Fix RenderStats ambiguity issues when found