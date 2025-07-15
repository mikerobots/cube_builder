#!/bin/bash
# Run unit tests and show summary

echo "Building and running unit tests..."
./run_all_unit.sh all 2>&1 | tee unit_test_output.log | grep -E "(Building [0-9]+ unit|Failed to build|Build failed|tests passed|All tests passed|test_unit_)" | tail -100