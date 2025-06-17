#!/bin/bash
# Workspace Boundary Placement Test Script
# Tests voxel placement at workspace boundaries to verify edge placement functionality

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLI_PATH="$SCRIPT_DIR/build_ninja/bin/VoxelEditor_CLI"
OUTPUT_DIR="$SCRIPT_DIR/test_output/boundaries"
LOG_FILE="$OUTPUT_DIR/boundary_test.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Ensure output directory exists
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}=== Workspace Boundary Placement Test Suite ===${NC}"
echo "Testing voxel placement at workspace boundaries"
echo "Log file: $LOG_FILE"
echo ""

# Function to run a test and capture output
run_test() {
    local test_name="$1"
    local commands="$2"
    local expected_result="$3"  # "success" or "failure"
    
    echo -e "${YELLOW}Testing: $test_name${NC}"
    
    # Create command file
    local cmd_file="$OUTPUT_DIR/${test_name}_commands.txt"
    echo "$commands" > "$cmd_file"
    echo "quit" >> "$cmd_file"
    
    # Run CLI with commands
    local output_file="$OUTPUT_DIR/${test_name}_output.txt"
    timeout 30s "$CLI_PATH" --headless --batch < "$cmd_file" > "$output_file" 2>&1
    
    # Check output for success/failure indicators
    if [ "$expected_result" = "success" ]; then
        # Look for success indicators
        if grep -q "Voxel placed" "$output_file" || grep -q "placed successfully" "$output_file"; then
            local actual_result="success"
        else
            local actual_result="failure"
        fi
    else
        # Look for error indicators
        if grep -q "Error" "$output_file" || grep -q "Failed" "$output_file" || grep -q "outside workspace" "$output_file" || grep -q "Invalid position" "$output_file" || grep -q "Cannot place" "$output_file"; then
            local actual_result="failure"
        else
            local actual_result="success"
        fi
    fi
    
    # Check if result matches expectation
    if [ "$actual_result" = "$expected_result" ]; then
        echo -e "  ${GREEN}✓ PASS${NC} - Expected $expected_result, got $actual_result"
        echo "[PASS] $test_name" >> "$LOG_FILE"
        return 0
    else
        echo -e "  ${RED}✗ FAIL${NC} - Expected $expected_result, got $actual_result"
        echo "[FAIL] $test_name - Expected $expected_result, got $actual_result" >> "$LOG_FILE"
        echo "  Output: $(tail -n 3 "$output_file" | tr '\n' ' ')"
        return 1
    fi
}

# Function to test corner placement
test_corner_placement() {
    local workspace_size="$1"
    local resolution="$2"
    local corner_x="$3"
    local corner_y="$4" 
    local corner_z="$5"
    
    local commands="workspace $workspace_size $workspace_size $workspace_size
resolution $resolution
place ${corner_x}cm ${corner_y}cm ${corner_z}cm"
    
    run_test "corner_${workspace_size}m_${resolution}_${corner_x}_${corner_y}_${corner_z}" "$commands" "success"
}

# Function to test out-of-bounds placement
test_out_of_bounds() {
    local workspace_size="$1"
    local resolution="$2"
    local x="$3"
    local y="$4"
    local z="$5"
    
    local commands="workspace $workspace_size $workspace_size $workspace_size
resolution $resolution
place ${x}cm ${y}cm ${z}cm"
    
    run_test "oob_${workspace_size}m_${resolution}_${x}_${y}_${z}" "$commands" "failure"
}

# Initialize log
echo "=== Workspace Boundary Test Results ===" > "$LOG_FILE"
echo "Test started at: $(date)" >> "$LOG_FILE"
echo "" >> "$LOG_FILE"

# Test counter
TESTS_RUN=0
TESTS_PASSED=0

echo -e "${BLUE}1. Testing Corner Placements (should succeed)${NC}"

# 3x3x3 workspace with 8cm resolution
# Boundaries at ±1.5m = ±150cm, but voxel must fit entirely within bounds
# For 8cm voxel: max position = 150cm - 8cm = 142cm (not grid aligned)
# Nearest valid 8cm grid position = 136cm (17 * 8cm)
# Note: Y must be >= 0 due to ground plane constraint
if test_corner_placement 3 8cm -144 0 -144; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))
if test_corner_placement 3 8cm 136 288 136; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))
if test_corner_placement 3 8cm -144 288 -144; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))
if test_corner_placement 3 8cm 136 0 136; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

echo ""
echo -e "${BLUE}2. Testing Face Center Placements (should succeed)${NC}"

# Face centers at boundaries
if run_test "face_center_x_neg" "workspace 3 3 3
resolution 8cm
place -144cm 0cm 0cm" "success"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

if run_test "face_center_x_pos" "workspace 3 3 3
resolution 8cm
place 136cm 0cm 0cm" "success"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

if run_test "face_center_y_bottom" "workspace 3 3 3
resolution 8cm
place 0cm 0cm 0cm" "success"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

# Y: 0 to 3m, max pos = 300cm - 8cm = 292cm
# Nearest valid 8cm grid position = 288cm (36 * 8cm)
if run_test "face_center_y_pos" "workspace 3 3 3
resolution 8cm
place 0cm 288cm 0cm" "success"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

echo ""
echo -e "${BLUE}3. Testing Out-of-Bounds Placements (should fail)${NC}"

# Positions outside 3x3x3 workspace (beyond ±1.5m)
if test_out_of_bounds 3 8cm -160 0 0; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))
if test_out_of_bounds 3 8cm 160 0 0; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))
if run_test "oob_y_neg" "workspace 3 3 3
resolution 8cm
place 0cm -8cm 0cm" "failure"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))
# Y max = 300cm - 8cm = 292cm, so 296cm should fail
if test_out_of_bounds 3 8cm 0 296 0; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

echo ""
echo -e "${BLUE}4. Testing Different Resolutions${NC}"

# 4cm resolution - max position = 150cm - 4cm = 146cm
# Nearest valid 4cm grid position = 144cm (36 * 4cm)
if run_test "res_4cm_boundary" "workspace 3 3 3
resolution 4cm
place 144cm 0cm 0cm" "success"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

# 16cm resolution - max position = 150cm - 16cm = 134cm
# Nearest valid 16cm grid position = 128cm (8 * 16cm)
if run_test "res_16cm_on_grid" "workspace 3 3 3
resolution 16cm
place 128cm 0cm 0cm" "success"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

if run_test "res_16cm_off_grid" "workspace 3 3 3
resolution 16cm
place 152cm 0cm 0cm" "failure"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

echo ""
echo -e "${BLUE}5. Testing Workspace Resizing${NC}"

# Place in large workspace, then shrink and verify placement fails
if run_test "resize_large_to_small" "workspace 4 4 4
resolution 8cm
place 152cm 0cm 0cm
workspace 2 2 2
place 152cm 0cm 0cm" "failure"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

echo ""
echo -e "${BLUE}6. Testing Asymmetric Workspaces${NC}"

# 4x2x6 workspace (different boundaries per axis)
# X: ±2m boundary, max pos = 200cm - 8cm = 192cm
if run_test "asymmetric_x_boundary" "workspace 4 2 6
resolution 8cm
place 192cm 0cm 0cm" "success"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

# Y: 0 to 2m, max pos = 200cm - 8cm = 192cm
if run_test "asymmetric_y_boundary" "workspace 4 2 6
resolution 8cm
place 0cm 192cm 0cm" "success"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

# Z: ±3m boundary, max pos = 300cm - 8cm = 292cm
# Nearest valid 8cm grid position = 288cm (36 * 8cm)
if run_test "asymmetric_z_boundary" "workspace 4 2 6
resolution 8cm
place 0cm 0cm 288cm" "success"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))

echo ""
echo -e "${BLUE}7. Testing Center Placement (should always work)${NC}"

# Center should work with all resolutions
for resolution in 1cm 2cm 4cm 8cm 16cm 32cm; do
    if run_test "center_${resolution}" "workspace 3 3 3
resolution $resolution
place 0cm 0cm 0cm" "success"; then ((TESTS_PASSED++)); fi; ((TESTS_RUN++))
done

echo ""
echo -e "${BLUE}=== Test Summary ===${NC}"
echo "Tests run: $TESTS_RUN"
echo "Tests passed: $TESTS_PASSED"
echo "Tests failed: $((TESTS_RUN - TESTS_PASSED))"

# Final log entry
echo "" >> "$LOG_FILE"
echo "Test completed at: $(date)" >> "$LOG_FILE"
echo "Results: $TESTS_PASSED/$TESTS_RUN tests passed" >> "$LOG_FILE"

if [ $TESTS_PASSED -eq $TESTS_RUN ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Check log: $LOG_FILE${NC}"
    echo -e "${RED}Failed tests output in: $OUTPUT_DIR${NC}"
    exit 1
fi