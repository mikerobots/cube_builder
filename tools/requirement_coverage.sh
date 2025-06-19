#!/bin/bash

# Simple Requirement Coverage Tracker
# Parses GoogleTest output to extract requirement coverage from test names

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${1:-build_ninja}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Requirement Coverage Analysis${NC}"
echo -e "${BLUE}========================================${NC}"
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo ""

# Create output directory
mkdir -p "$PROJECT_ROOT/coverage_reports"
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
REPORT_DIR="$PROJECT_ROOT/coverage_reports/$TIMESTAMP"
mkdir -p "$REPORT_DIR"

echo -e "${YELLOW}Running all tests and capturing requirement coverage...${NC}"

# Run unit tests and capture output
echo "Running unit tests..."
cd "$PROJECT_ROOT"
./run_all_unit.sh "$BUILD_DIR" > "$REPORT_DIR/unit_tests.log" 2>&1 || true

# Run integration tests and capture output  
echo "Running integration tests..."
./run_integration_tests.sh all > "$REPORT_DIR/integration_tests.log" 2>&1 || true

echo -e "${YELLOW}Analyzing requirement coverage...${NC}"

# Extract requirements from test names and their status
parse_requirements() {
    local log_file="$1"
    local test_type="$2"
    
    # Extract test results with requirement IDs from GoogleTest output
    grep -E '\[ +(RUN|OK|FAILED) +\].*REQ_[0-9_]+' "$log_file" | while read line; do
        if echo "$line" | grep -q '\[ *RUN *\]'; then
            # Extract requirement ID from test name (convert REQ_X_X_X to REQ-X.X.X)
            req_id=$(echo "$line" | grep -o 'REQ_[0-9_]*' | sed 's/_/\./g' | sed 's/REQ\./REQ-/')
            test_name=$(echo "$line" | sed 's/.*\] *//')
            echo "$req_id|$test_name|RUNNING|$test_type"
        elif echo "$line" | grep -q '\[ *OK *\]'; then
            req_id=$(echo "$line" | grep -o 'REQ_[0-9_]*' | sed 's/_/\./g' | sed 's/REQ\./REQ-/')
            test_name=$(echo "$line" | sed 's/.*\] *//' | sed 's/ *(.*ms).*//')
            echo "$req_id|$test_name|PASSED|$test_type"
        elif echo "$line" | grep -q '\[ *FAILED *\]'; then
            req_id=$(echo "$line" | grep -o 'REQ_[0-9_]*' | sed 's/_/\./g' | sed 's/REQ\./REQ-/')
            test_name=$(echo "$line" | sed 's/.*\] *//' | sed 's/ *(.*ms).*//')
            echo "$req_id|$test_name|FAILED|$test_type"
        fi
    done
}

# Parse both test types
parse_requirements "$REPORT_DIR/unit_tests.log" "UNIT" > "$REPORT_DIR/unit_requirements.csv"
parse_requirements "$REPORT_DIR/integration_tests.log" "INTEGRATION" > "$REPORT_DIR/integration_requirements.csv"

# Combine results
cat "$REPORT_DIR/unit_requirements.csv" "$REPORT_DIR/integration_requirements.csv" > "$REPORT_DIR/all_requirements.csv"

# Generate summary report
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Requirement Coverage Summary${NC}"
echo -e "${BLUE}========================================${NC}"

total_requirements=$(cat "$REPORT_DIR/all_requirements.csv" | cut -d'|' -f1 | sort | uniq | wc -l)
passed_requirements=$(grep "|PASSED|" "$REPORT_DIR/all_requirements.csv" | cut -d'|' -f1 | sort | uniq | wc -l)
failed_requirements=$(grep "|FAILED|" "$REPORT_DIR/all_requirements.csv" | cut -d'|' -f1 | sort | uniq | wc -l)

echo "Total requirements with tests: $total_requirements"
echo -e "Passing requirements: ${GREEN}$passed_requirements${NC}"
echo -e "Failing requirements: ${RED}$failed_requirements${NC}"

if [ $total_requirements -gt 0 ]; then
    pass_rate=$(( (passed_requirements * 100) / total_requirements ))
    echo -e "Pass rate: ${GREEN}$pass_rate%${NC}"
fi

echo ""
echo -e "${YELLOW}Detailed breakdown:${NC}"

# Unit test breakdown
unit_reqs=$(grep "|UNIT|" "$REPORT_DIR/all_requirements.csv" | cut -d'|' -f1 | sort | uniq | wc -l)
unit_passed=$(grep "|PASSED|UNIT" "$REPORT_DIR/all_requirements.csv" | cut -d'|' -f1 | sort | uniq | wc -l)
echo "Unit tests: $unit_passed/$unit_reqs requirements passing"

# Integration test breakdown
int_reqs=$(grep "|INTEGRATION|" "$REPORT_DIR/all_requirements.csv" | cut -d'|' -f1 | sort | uniq | wc -l)
int_passed=$(grep "|PASSED|INTEGRATION" "$REPORT_DIR/all_requirements.csv" | cut -d'|' -f1 | sort | uniq | wc -l)
echo "Integration tests: $int_passed/$int_reqs requirements passing"

# Show failing requirements if any
if [ $failed_requirements -gt 0 ]; then
    echo ""
    echo -e "${RED}Failing requirements:${NC}"
    grep "|FAILED|" "$REPORT_DIR/all_requirements.csv" | cut -d'|' -f1 | sort | uniq | while read req; do
        echo -e "  ${RED}✗${NC} $req"
    done
fi

echo ""
echo "Detailed reports saved to: $REPORT_DIR"
echo "  - all_requirements.csv: Complete requirement test results"
echo "  - unit_tests.log: Full unit test output"
echo "  - integration_tests.log: Full integration test output"

# Create a simple HTML report
cat > "$REPORT_DIR/coverage_report.html" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>Requirement Coverage Report - $TIMESTAMP</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .pass { color: green; }
        .fail { color: red; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <h1>Requirement Coverage Report</h1>
    <p>Generated: $TIMESTAMP</p>
    
    <h2>Summary</h2>
    <ul>
        <li>Total requirements with tests: $total_requirements</li>
        <li class="pass">Passing requirements: $passed_requirements</li>
        <li class="fail">Failing requirements: $failed_requirements</li>
        <li>Pass rate: $pass_rate%</li>
    </ul>
    
    <h2>Test Breakdown</h2>
    <table>
        <tr><th>Test Type</th><th>Requirements Covered</th><th>Passing</th><th>Pass Rate</th></tr>
        <tr><td>Unit Tests</td><td>$unit_reqs</td><td class="pass">$unit_passed</td><td>$(( unit_reqs > 0 ? (unit_passed * 100) / unit_reqs : 0 ))%</td></tr>
        <tr><td>Integration Tests</td><td>$int_reqs</td><td class="pass">$int_passed</td><td>$(( int_reqs > 0 ? (int_passed * 100) / int_reqs : 0 ))%</td></tr>
    </table>
</body>
</html>
EOF

echo ""
echo -e "${GREEN}HTML report generated: $REPORT_DIR/coverage_report.html${NC}"