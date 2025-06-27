#!/bin/bash

# Migration script to update test runners to enhanced versions

set -euo pipefail

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'
BOLD='\033[1m'

echo -e "${BOLD}Test Runner Enhancement Migration${NC}"
echo "=================================="
echo

# Function to create wrapper script
create_wrapper() {
    local original=$1
    local enhanced=$2
    local wrapper_content="#!/bin/bash
# Wrapper script that redirects to enhanced test runner
# Original script backed up as ${original}.original

exec \"./$(basename "$enhanced")\" \"\$@\"
"
    
    if [ -f "$original" ]; then
        echo -e "${CYAN}Updating $original...${NC}"
        
        # Backup original
        if [ ! -f "${original}.original" ]; then
            cp "$original" "${original}.original"
            echo "  ✓ Backed up to ${original}.original"
        fi
        
        # Create wrapper
        echo "$wrapper_content" > "$original"
        chmod +x "$original"
        echo "  ✓ Updated to use enhanced runner"
    fi
}

# Update test runners
echo -e "${BOLD}Updating test runners to use enhanced versions:${NC}"
echo

create_wrapper "run_all_unit.sh" "run_all_unit_enhanced.sh"
create_wrapper "run_integration_tests.sh" "run_integration_tests_enhanced.sh"
create_wrapper "run_e2e_tests.sh" "run_e2e_tests_enhanced.sh"
create_wrapper "run_all_tests.sh" "run_all_tests_enhanced.sh"

echo
echo -e "${GREEN}Migration complete!${NC}"
echo
echo "The original test runners now redirect to enhanced versions."
echo "Original scripts are backed up with .original extension."
echo
echo -e "${YELLOW}To revert:${NC}"
echo "  mv run_all_unit.sh.original run_all_unit.sh"
echo "  mv run_integration_tests.sh.original run_integration_tests.sh"
echo "  mv run_e2e_tests.sh.original run_e2e_tests.sh"
echo "  mv run_all_tests.sh.original run_all_tests.sh"
echo
echo -e "${GREEN}New features available:${NC}"
echo "  • Individual test case visibility (C++ tests)"
echo "  • Run specific test cases with 'test' command"
echo "  • List test cases with 'list-tests' command"
echo "  • Filter tests with --filter=PATTERN"
echo "  • Enhanced error reporting and timing"
echo
echo "Try these commands:"
echo "  ./run_all_unit.sh list"
echo "  ./run_all_unit.sh test MemoryPool.Allocation"
echo "  ./run_integration_tests.sh test \"Camera.*\""
echo "  ./run_all_tests.sh quick"