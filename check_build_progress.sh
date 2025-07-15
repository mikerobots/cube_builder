#!/bin/bash
# Check build progress

cd build_ninja
ninja -k 50 2>&1 | grep -E "(FAILED:|error:|Building CXX|Linking CXX|succeeded)" | tail -100