#!/bin/bash
# Check for linking errors

cd build_ninja
ninja -k 0 2>&1 | grep -A10 -B10 "undefined symbols\|duplicate symbol" | head -100