#!/bin/bash

# Demo script showing how to run specific tests with the enhanced runner

echo "========================================"
echo "DEMO: Running Specific Tests"
echo "========================================"
echo

echo "The enhanced test runner now supports running specific test cases!"
echo

echo "1. LIST TEST CASES IN A FILE:"
echo "-----------------------------"
echo "Command: ./run_all_unit_enhanced.sh list-tests test_unit_foundation_memory_pool"
echo
echo "Example output:"
cat << 'EOF'
==========================================
Test cases in test_unit_foundation_memory_pool
==========================================

MemoryPoolTest.
  AllocateAndDeallocate
  MultipleAllocations
  PoolReset
  AllocationAlignment
  OutOfMemoryHandling
  DeallocateNull
  MemoryReuse
  LargeAllocations
EOF

echo
echo
echo "2. RUN A SPECIFIC TEST FILE:"
echo "----------------------------"
echo "Command: ./run_all_unit_enhanced.sh test test_unit_foundation_memory_pool"
echo
echo "This builds and runs just that one test file with all its test cases."
echo

echo
echo "3. RUN A SPECIFIC TEST CASE:"
echo "----------------------------"
echo "Command: ./run_all_unit_enhanced.sh test MemoryPool.AllocateAndDeallocate"
echo
echo "Example output:"
cat << 'EOF'
Searching for tests matching pattern: MemoryPool.AllocateAndDeallocate

Running test_unit_foundation_memory_pool with filter: *MemoryPool.AllocateAndDeallocate*
  Running 1 test from MemoryPoolTest
    ✓ MemoryPoolTest.AllocateAndDeallocate (5ms)
  Summary: 1 test case passed (0s)

==========================================
Unit Test Summary
==========================================
Test Files: 1
  Passed:  1
  Failed:  0
  Skipped: 0

Test Cases: 1
  Passed:  1
  Failed:  0
  Skipped: 0

All unit tests passed! 🎉
EOF

echo
echo
echo "4. RUN ALL TESTS MATCHING A PATTERN:"
echo "------------------------------------"
echo "Command: ./run_all_unit_enhanced.sh test \"MemoryPool.*\""
echo
echo "This runs all test cases that start with 'MemoryPool.' across all test files."
echo

echo
echo "5. USE FILTER WITH SUBSYSTEM:"
echo "-----------------------------"
echo "Command: ./run_all_unit_enhanced.sh foundation --filter=\"*Vector*\""
echo
echo "This runs all Vector-related tests within the foundation subsystem."
echo

echo
echo "6. GOOGLE TEST FILTER PATTERNS:"
echo "-------------------------------"
echo "The filter supports Google Test patterns:"
echo "  - Use * for wildcards"
echo "  - Use ? for single character"
echo "  - Use - to exclude tests"
echo "  - Combine with :"
echo
echo "Examples:"
echo "  *Vector*           - All tests with 'Vector' in the name"
echo "  Math*:Vector*      - All Math OR Vector tests"
echo "  *-*Slow*           - All tests EXCEPT those with 'Slow'"
echo "  TestSuite.Test*    - All tests starting with 'Test' in TestSuite"
echo

echo
echo "7. BENEFITS:"
echo "------------"
echo "• Debug failing tests faster by running just that test"
echo "• Iterate quickly on specific functionality"
echo "• Run related tests together with patterns"
echo "• Skip long-running tests during development"
echo "• Focus on the code you're working on"
echo

echo
echo "Try it out:"
echo "  ./run_all_unit_enhanced.sh list                    # See all subsystems"
echo "  ./run_all_unit_enhanced.sh list-tests <test_file>  # See test cases"
echo "  ./run_all_unit_enhanced.sh test <test_case>        # Run specific test"
echo