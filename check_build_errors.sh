#!/bin/bash
# Check for compilation errors in detail

cd build_ninja
ninja -k 0 2>&1 | grep -E "(error:|undefined|cannot|no matching|undeclared)" | head -50