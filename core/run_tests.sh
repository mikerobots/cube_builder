#!/bin/bash

# Core Subsystems Master Test Runner
# Usage: ./run_tests.sh [build_dir] [--quick|--full|--slow]
#
# Test execution modes:
#   --quick  : Run all tests under 1 second across all subsystems (default)
#   --full   : Run all tests under 5 seconds across all subsystems
#   --slow   : Run all tests over 5 seconds across all subsystems (performance tests)
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

# Default build directory
BUILD_DIR="${1:-build_ninja}"

# Test mode (default to quick)
TEST_MODE="quick"
if [[ "$2" == "--full" ]]; then
    TEST_MODE="full"
elif [[ "$2" == "--slow" ]]; then
    TEST_MODE="slow"
elif [[ "$2" == "--quick" ]]; then
    TEST_MODE="quick"
elif [[ "$2" == "--help" ]]; then
    echo "Core Subsystems Master Test Runner"
    echo ""
    echo "Usage: $0 [build_dir] [--quick|--full|--slow|--help]"
    echo ""
    echo "Modes:"
    echo "  --quick  : Fast essential tests across all subsystems (<1s each)"
    echo "  --full   : All stable tests across all subsystems (<5s each)"
    echo "  --slow   : Performance and stress tests across all subsystems (>5s)"
    echo ""
    echo "Subsystems tested:"
    echo "  1. Camera System"
    echo "  2. File I/O System"
    echo "  3. Groups System"
    echo "  4. Input System"
    echo "  5. Rendering System"
    echo "  6. Selection System"
    echo "  7. Surface Generation"
    echo "  8. Undo/Redo System"
    echo "  9. Visual Feedback"
    echo "  10. VoxelData System"
    echo ""
    echo "Examples:"
    echo "  $0                    # Quick tests with default build"
    echo "  $0 --quick           # Quick tests with default build"
    echo "  $0 build_debug       # Quick tests with debug build"
    echo "  $0 build_debug --full # Full tests with debug build"
    exit 0
fi

# Check if first argument is actually a mode flag
if [[ "$1" == "--quick" || "$1" == "--full" || "$1" == "--slow" || "$1" == "--help" ]]; then
    BUILD_DIR="build_ninja"
    TEST_MODE="${1#--}"
fi

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}Core Subsystems Master Test Runner${NC}"
echo -e "${CYAN}=========================================${NC}"
echo "Build directory: $BUILD_DIR"
echo "Project root: $PROJECT_ROOT"
echo "Test mode: $TEST_MODE"
echo ""

# Change to project root
cd "$PROJECT_ROOT"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory '$BUILD_DIR' does not exist${NC}"
    echo "Please run: cmake -B $BUILD_DIR -G Ninja"
    exit 1
fi

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
declare -a FAILED_LIST=()
declare -a SKIPPED_LIST=()

# Start timing
MASTER_START_TIME=$(date +%s)

echo -e "${BLUE}Running tests for $TOTAL_SUBSYSTEMS core subsystems in $TEST_MODE mode...${NC}"
echo ""

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
    echo "Executing: $TEST_SCRIPT $BUILD_DIR --$TEST_MODE"
    echo ""
    
    if cd "$SUBSYSTEM_DIR" && ./run_tests.sh "$BUILD_DIR" "--$TEST_MODE"; then
        echo ""
        echo -e "${GREEN}✅ $display_name: PASSED${NC}"
        PASSED_SUBSYSTEMS=$((PASSED_SUBSYSTEMS + 1))
    else
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
echo "Test mode: $TEST_MODE"
echo "Total execution time: ${MASTER_DURATION} seconds"
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

# Mode-specific expectations
case "$TEST_MODE" in
    "quick")
        if [ $MASTER_DURATION -gt 30 ]; then
            echo -e "${YELLOW}⚠️  Quick mode took longer than expected (${MASTER_DURATION}s > 30s)${NC}"
        fi
        ;;
    "full")
        if [ $MASTER_DURATION -gt 120 ]; then
            echo -e "${YELLOW}⚠️  Full mode took longer than expected (${MASTER_DURATION}s > 120s)${NC}"
        fi
        ;;
    "slow")
        echo -e "${BLUE}ℹ️  Slow mode timing is expected to be variable${NC}"
        ;;
esac

echo ""
echo -e "${BLUE}Individual subsystem results available above.${NC}"
echo -e "${BLUE}To run tests for a specific subsystem:${NC}"
echo -e "${BLUE}  cd core/[subsystem] && ./run_tests.sh $BUILD_DIR --$TEST_MODE${NC}"

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