#!/bin/bash

# Script to list all unit and integration gtests in the project

echo "=== UNIT TESTS ==="
echo

# Find all unit test executables
if [ -d "build_ninja/bin" ]; then
    unit_tests=$(find build_ninja/bin -name "test_unit_*" -type f -perm -111 2>/dev/null | sort)
    if [ -n "$unit_tests" ]; then
        echo "$unit_tests" | while read test; do
            echo "  $(basename "$test")"
        done
    else
        echo "  No unit tests found in build_ninja/bin"
    fi
else
    echo "  build_ninja/bin directory not found. Please build the project first."
fi

echo
echo "=== INTEGRATION TESTS ==="
echo

# Find all integration test executables
if [ -d "build_ninja/bin" ]; then
    integration_tests=$(find build_ninja/bin -name "test_integration_*" -type f -perm -111 2>/dev/null | sort)
    if [ -n "$integration_tests" ]; then
        echo "$integration_tests" | while read test; do
            echo "  $(basename "$test")"
        done
    else
        echo "  No integration tests found in build_ninja/bin"
    fi
else
    echo "  build_ninja/bin directory not found. Please build the project first."
fi

echo
echo "=== PERFORMANCE TESTS ==="
echo

# Find all performance test executables
if [ -d "build_ninja/bin" ]; then
    performance_tests=$(find build_ninja/bin -name "test_performance_*" -type f -perm -111 2>/dev/null | sort)
    if [ -n "$performance_tests" ]; then
        echo "$performance_tests" | while read test; do
            echo "  $(basename "$test")"
        done
    else
        echo "  No performance tests found in build_ninja/bin"
    fi
else
    echo "  build_ninja/bin directory not found. Please build the project first."
fi

echo
echo "=== TEST SUMMARY ==="
echo

# Count tests
if [ -d "build_ninja/bin" ]; then
    unit_count=$(find build_ninja/bin -name "test_unit_*" -type f -perm -111 2>/dev/null | wc -l | tr -d ' ')
    integration_count=$(find build_ninja/bin -name "test_integration_*" -type f -perm -111 2>/dev/null | wc -l | tr -d ' ')
    performance_count=$(find build_ninja/bin -name "test_performance_*" -type f -perm -111 2>/dev/null | wc -l | tr -d ' ')
    total_count=$((unit_count + integration_count + performance_count))
    
    echo "  Unit tests: $unit_count"
    echo "  Integration tests: $integration_count"
    echo "  Performance tests: $performance_count"
    echo "  Total tests: $total_count"
else
    echo "  Cannot count tests - build directory not found"
fi

echo
echo "=== USAGE ==="
echo "  To run a specific test: ./build_ninja/bin/<test_name>"
echo "  To run all unit tests: ./run_all_unit.sh"
echo "  To run integration tests: ./run_integration_tests.sh"