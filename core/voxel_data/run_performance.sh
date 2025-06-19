#!/bin/bash

# This script has been integrated into run_tests.sh
# Please use: ./run_tests.sh [build_dir] performance

echo "This script has been deprecated and integrated into run_tests.sh"
echo ""
echo "Please use one of the following commands:"
echo "  ./run_tests.sh build_ninja performance    # Run performance tests"
echo "  ./run_tests.sh build_ninja quick          # Run quick tests (excludes performance)"
echo "  ./run_tests.sh build_ninja full           # Run all tests except extreme performance"
echo "  ./run_tests.sh build_ninja requirements   # Run requirements tests only"
echo ""
echo "Redirecting to: ./run_tests.sh $1 performance"
echo ""

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run the new script with performance mode
exec "$SCRIPT_DIR/run_tests.sh" "$1" performance