#!/bin/bash

# Simple test of requirement coverage - run a few requirement tests to validate approach

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Simple Requirement Coverage Test${NC}"
echo -e "${BLUE}========================================${NC}"

# Run a few requirement tests to demonstrate the approach
cd "$PROJECT_ROOT/build_ninja"

echo -e "${YELLOW}Running sample requirement tests...${NC}"

# Run a few VoxelData requirement tests with verbose output
timeout 15s ctest -R "REQ_[0-9_]*" --verbose > /tmp/requirement_test_sample.log 2>&1 || true

echo "Parsing requirement coverage from test output..."

# Extract requirements and their results
parse_requirements() {
    local log_file="$1"
    
    grep -E '\[ +(RUN|OK|FAILED) +\].*REQ_[0-9_]+' "$log_file" | while read line; do
        if echo "$line" | grep -q '\[ *OK *\]'; then
            req_id=$(echo "$line" | grep -o 'REQ_[0-9_]*' | sed 's/_/\./g' | sed 's/REQ\./REQ-/')
            test_name=$(echo "$line" | sed 's/.*\] *//' | sed 's/ *(.*ms).*//')
            echo "✅ $req_id - $test_name"
        elif echo "$line" | grep -q '\[ *FAILED *\]'; then
            req_id=$(echo "$line" | grep -o 'REQ_[0-9_]*' | sed 's/_/\./g' | sed 's/REQ\./REQ-/')
            test_name=$(echo "$line" | sed 's/.*\] *//' | sed 's/ *(.*ms).*//')
            echo "❌ $req_id - $test_name"
        fi
    done
}

echo ""
echo -e "${GREEN}Requirements tested and their results:${NC}"
parse_requirements /tmp/requirement_test_sample.log

echo ""
echo "Summary of requirement test coverage found:"
total_reqs=$(grep -o 'REQ_[0-9_]*' /tmp/requirement_test_sample.log | sort | uniq | wc -l)
passed_reqs=$(grep '\[ *OK *\].*REQ_[0-9_]*' /tmp/requirement_test_sample.log | wc -l)
failed_reqs=$(grep '\[ *FAILED *\].*REQ_[0-9_]*' /tmp/requirement_test_sample.log | wc -l)

echo "- Total requirement tests found: $total_reqs"
echo "- Passed: $passed_reqs"
echo "- Failed: $failed_reqs"

if [ $total_reqs -gt 0 ]; then
    echo ""
    echo -e "${GREEN}✅ Requirement traceability working!${NC}"
    echo "This demonstrates that we can:"
    echo "1. Extract requirement IDs from test names automatically"
    echo "2. Track test pass/fail status for each requirement"
    echo "3. Generate coverage reports without code changes"
    echo ""
    echo "Next steps:"
    echo "- Run full test suite with this approach"
    echo "- Generate comprehensive coverage reports"
    echo "- Compare against requirements.md files"
else
    echo ""
    echo -e "${YELLOW}⚠️ No requirement tests found in sample${NC}"
    echo "This might mean:"
    echo "- Tests need to be built first"
    echo "- Different test naming pattern"
    echo "- Need to run specific test suites"
fi

echo ""
echo "Full test output saved to: /tmp/requirement_test_sample.log"