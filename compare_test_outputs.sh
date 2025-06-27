#!/bin/bash

# Script to demonstrate the difference between original and enhanced test runner output

echo "=================================="
echo "EXAMPLE OUTPUT COMPARISON"
echo "=================================="
echo

echo "1. ORIGINAL TEST RUNNER OUTPUT (file-level only):"
echo "================================================"
cat << 'EOF'
==========================================
Foundation Unit Tests
==========================================

Building foundation unit tests...
✓ test_unit_foundation_memory_pool (2s)
✓ test_unit_foundation_event_dispatcher (1s)
✗ test_unit_foundation_math_utils (3s)
  Error output:
    [  FAILED  ] MathUtilsTest.VectorNormalization
    Expected: (magnitude) == (1.0f), actual: 0.999999 vs 1
    [  FAILED  ] 1 test, listed below:
    [  FAILED  ] MathUtilsTest.VectorNormalization

==========================================
Unit Test Summary
==========================================
Total Tests: 3
  Passed:  2
  Failed:  1
  Skipped: 0

Failed Tests:
  - test_unit_foundation_math_utils

Some unit tests failed. Please check the logs.
EOF

echo
echo
echo "2. ENHANCED TEST RUNNER OUTPUT (individual test cases):"
echo "======================================================"
cat << 'EOF'
==========================================
Foundation Unit Tests
==========================================

Building foundation unit tests...

Running test_unit_foundation_memory_pool...
  Running 8 tests from MemoryPoolTest
    ✓ MemoryPoolTest.AllocateAndDeallocate (5ms)
    ✓ MemoryPoolTest.MultipleAllocations (3ms)
    ✓ MemoryPoolTest.PoolReset (2ms)
    ✓ MemoryPoolTest.AllocationAlignment (1ms)
    ✓ MemoryPoolTest.OutOfMemoryHandling (4ms)
    ✓ MemoryPoolTest.DeallocateNull (0ms)
    ✓ MemoryPoolTest.MemoryReuse (2ms)
    ✓ MemoryPoolTest.LargeAllocations (8ms)
  Summary: 8 test cases passed (2s)

Running test_unit_foundation_event_dispatcher...
  Running 5 tests from EventDispatcherTest
    ✓ EventDispatcherTest.SubscribeAndDispatch (2ms)
    ✓ EventDispatcherTest.MultipleSubscribers (3ms)
    ✓ EventDispatcherTest.UnsubscribeHandler (1ms)
    ✓ EventDispatcherTest.EventPriority (2ms)
    ✓ EventDispatcherTest.RecursiveDispatch (4ms)
  Summary: 5 test cases passed (1s)

Running test_unit_foundation_math_utils...
  Running 6 tests from MathUtilsTest
    ✓ MathUtilsTest.VectorAddition (1ms)
    ✓ MathUtilsTest.VectorSubtraction (1ms)
    ✗ MathUtilsTest.VectorNormalization (2ms)
      /path/to/test_unit_foundation_math_utils.cpp:78: Failure
      Expected: (magnitude) == (1.0f), actual: 0.999999 vs 1
      Floating point precision error in normalization
    ✓ MathUtilsTest.CrossProduct (1ms)
    ✓ MathUtilsTest.DotProduct (0ms)
    ✓ MathUtilsTest.MatrixMultiplication (3ms)
  Summary: 1/6 test cases failed (3s)

==========================================
Unit Test Summary
==========================================
Test Files: 3
  Passed:  2
  Failed:  1
  Skipped: 0

Test Cases: 19
  Passed:  18
  Failed:  1
  Skipped: 0

Failed Test Cases:
  - test_unit_foundation_math_utils::MathUtilsTest.VectorNormalization

Some unit tests failed. Please check the logs.
EOF

echo
echo
echo "3. KEY IMPROVEMENTS:"
echo "==================="
echo "• Shows individual test case names within each test file"
echo "• Reports execution time for each test case (in milliseconds)"
echo "• Displays inline error messages for failed test cases"
echo "• Provides both file-level and case-level statistics"
echo "• Maintains the same color coding and visual hierarchy"
echo "• Lists specific failed test cases with full path (file::suite.test)"
echo "• Compatible with existing workflow (stops at first failure)"
echo
echo "4. ADDITIONAL OPTIONS:"
echo "====================="
echo "• --brief flag to get original output format"
echo "• --filter=PATTERN to run specific test cases"
echo "• XML/JSON output available for CI integration"
EOF