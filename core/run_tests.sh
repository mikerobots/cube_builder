#!/bin/bash

# Core Subsystems Master Test Runner
# Usage: ./run_tests.sh [pattern]
#
# This script runs tests for all 10 core subsystems:
#   1. Camera System
#   2. File I/O System  
#   3. Groups System
#   4. Input System
#   5. Rendering System
#   6. Selection System
#   7. Surface Generation
#   8. Undo/Redo System
#   9. Visual Feedback
#   10. VoxelData System

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Test pattern (default to all)
PATTERN="${1:-*}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}Core Subsystems Master Test Runner${NC}"
echo -e "${CYAN}=========================================${NC}"
echo "Project root: $PROJECT_ROOT"
echo "Test pattern: $PATTERN"
echo ""

# Change to project root
cd "$PROJECT_ROOT"

# Define all core subsystems
declare -a SUBSYSTEMS=(
    "camera"
    "file_io"
    "groups"
    "input"
    "rendering"
    "selection"
    "surface_gen"
    "undo_redo"
    "visual_feedback"
    "voxel_data"
)

# Function to get display name for subsystem
get_display_name() {
    case "$1" in
        "camera") echo "Camera System" ;;
        "file_io") echo "File I/O System" ;;
        "groups") echo "Groups System" ;;
        "input") echo "Input System" ;;
        "rendering") echo "Rendering System" ;;
        "selection") echo "Selection System" ;;
        "surface_gen") echo "Surface Generation" ;;
        "undo_redo") echo "Undo/Redo System" ;;
        "visual_feedback") echo "Visual Feedback" ;;
        "voxel_data") echo "VoxelData System" ;;
        *) echo "$1" ;;
    esac
}

# Tracking variables
TOTAL_SUBSYSTEMS=${#SUBSYSTEMS[@]}
PASSED_SUBSYSTEMS=0
FAILED_SUBSYSTEMS=0
SKIPPED_SUBSYSTEMS=0
TOTAL_TESTS_RUN=0
declare -a FAILED_LIST=()
declare -a SKIPPED_LIST=()

# Start timing
MASTER_START_TIME=$(date +%s)

echo -e "${BLUE}Running tests for $TOTAL_SUBSYSTEMS core subsystems...${NC}"
echo ""

# Check if pattern excludes performance tests
if [[ "$PATTERN" == "*" ]] || [[ -z "$PATTERN" ]]; then
    echo -e "${YELLOW}Note: Running all unit tests. Performance tests (test_uperf_*) are excluded by default.${NC}"
    echo -e "${YELLOW}To run performance tests, use: ./run_perf_tests.sh in each subsystem directory.${NC}"
    echo ""
fi

# Loop through each subsystem
for i in "${!SUBSYSTEMS[@]}"; do
    subsystem="${SUBSYSTEMS[$i]}"
    display_name=$(get_display_name "$subsystem")
    subsystem_num=$((i + 1))
    
    echo -e "${MAGENTA}[$subsystem_num/$TOTAL_SUBSYSTEMS] Testing: $display_name${NC}"
    echo -e "${CYAN}==========================================${NC}"
    
    # Check if subsystem directory exists
    SUBSYSTEM_DIR="$PROJECT_ROOT/core/$subsystem"
    if [ ! -d "$SUBSYSTEM_DIR" ]; then
        echo -e "${YELLOW}⚠️  Skipping $display_name: Directory not found${NC}"
        SKIPPED_SUBSYSTEMS=$((SKIPPED_SUBSYSTEMS + 1))
        SKIPPED_LIST+=("$display_name")
        echo ""
        continue
    fi
    
    # Check if run_tests.sh exists
    TEST_SCRIPT="$SUBSYSTEM_DIR/run_tests.sh"
    if [ ! -f "$TEST_SCRIPT" ]; then
        echo -e "${YELLOW}⚠️  Skipping $display_name: run_tests.sh not found${NC}"
        SKIPPED_SUBSYSTEMS=$((SKIPPED_SUBSYSTEMS + 1))
        SKIPPED_LIST+=("$display_name")
        echo ""
        continue
    fi
    
    # Make sure script is executable
    chmod +x "$TEST_SCRIPT"
    
    # Run the subsystem tests
    echo "Executing: $TEST_SCRIPT $PATTERN"
    echo ""
    
    # Capture test count from output
    if cd "$SUBSYSTEM_DIR" && OUTPUT=$(./run_tests.sh "$PATTERN" 2>&1); then
        echo "$OUTPUT"
        
        # Extract test count if available
        TEST_COUNT=$(echo "$OUTPUT" | grep -oE "Total Tests: [0-9]+" | grep -oE "[0-9]+$" || echo "0")
        TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + TEST_COUNT))
        
        echo ""
        echo -e "${GREEN}✅ $display_name: PASSED${NC}"
        PASSED_SUBSYSTEMS=$((PASSED_SUBSYSTEMS + 1))
    else
        echo "$OUTPUT"
        echo ""
        echo -e "${RED}❌ $display_name: FAILED${NC}"
        FAILED_SUBSYSTEMS=$((FAILED_SUBSYSTEMS + 1))
        FAILED_LIST+=("$display_name")
    fi
    
    # Return to project root
    cd "$PROJECT_ROOT"
    
    echo ""
    echo -e "${CYAN}------------------------------------------${NC}"
    echo ""
done

# Calculate execution time
MASTER_END_TIME=$(date +%s)
MASTER_DURATION=$((MASTER_END_TIME - MASTER_START_TIME))

# Print final summary
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}MASTER TEST SUMMARY${NC}"
echo -e "${CYAN}=========================================${NC}"
echo "Test pattern: $PATTERN"
echo "Total execution time: ${MASTER_DURATION} seconds"
echo "Total tests run: $TOTAL_TESTS_RUN"
echo ""

# Results breakdown
echo -e "${GREEN}✅ Passed: $PASSED_SUBSYSTEMS/$TOTAL_SUBSYSTEMS subsystems${NC}"
if [ $FAILED_SUBSYSTEMS -gt 0 ]; then
    echo -e "${RED}❌ Failed: $FAILED_SUBSYSTEMS/$TOTAL_SUBSYSTEMS subsystems${NC}"
fi
if [ $SKIPPED_SUBSYSTEMS -gt 0 ]; then
    echo -e "${YELLOW}⚠️  Skipped: $SKIPPED_SUBSYSTEMS/$TOTAL_SUBSYSTEMS subsystems${NC}"
fi

# Detailed failure and skip information
if [ ${#FAILED_LIST[@]} -gt 0 ]; then
    echo ""
    echo -e "${RED}Failed subsystems:${NC}"
    for failed in "${FAILED_LIST[@]}"; do
        echo -e "${RED}  - $failed${NC}"
    done
fi

if [ ${#SKIPPED_LIST[@]} -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}Skipped subsystems:${NC}"
    for skipped in "${SKIPPED_LIST[@]}"; do
        echo -e "${YELLOW}  - $skipped${NC}"
    done
fi

# Performance analysis
echo ""
echo -e "${BLUE}Performance Analysis:${NC}"
if [ $MASTER_DURATION -lt 60 ]; then
    echo -e "${GREEN}✅ Excellent performance: ${MASTER_DURATION}s total${NC}"
elif [ $MASTER_DURATION -lt 300 ]; then
    echo -e "${YELLOW}⚠️  Moderate performance: ${MASTER_DURATION}s total${NC}"
else
    echo -e "${RED}⚠️  Slow performance: ${MASTER_DURATION}s total${NC}"
fi

# Average time per test
if [ $TOTAL_TESTS_RUN -gt 0 ]; then
    AVG_TIME_PER_TEST=$((MASTER_DURATION * 1000 / TOTAL_TESTS_RUN))
    echo -e "${BLUE}Average time per test: ${AVG_TIME_PER_TEST}ms${NC}"
fi

echo ""
echo -e "${BLUE}Individual subsystem results available above.${NC}"
echo -e "${BLUE}To run tests for a specific subsystem:${NC}"
echo -e "${BLUE}  cd core/[subsystem] && ./run_tests.sh [pattern]${NC}"
echo -e "${BLUE}To run performance tests for a subsystem:${NC}"
echo -e "${BLUE}  cd core/[subsystem] && ./run_perf_tests.sh${NC}"

# Exit with appropriate code
if [ $FAILED_SUBSYSTEMS -gt 0 ]; then
    echo ""
    echo -e "${RED}❌ OVERALL RESULT: FAILED ($FAILED_SUBSYSTEMS failures)${NC}"
    exit 1
else
    echo ""
    echo -e "${GREEN}✅ OVERALL RESULT: PASSED (All subsystems passed)${NC}"
    exit 0
fi