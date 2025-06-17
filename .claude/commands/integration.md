In the root directory, we have:

# Show available test groups
./run_integration_tests.sh

# Run specific groups
./run_integration_tests.sh core cli rendering
./run_integration_tests.sh quick      # Quick smoke tests
./run_integration_tests.sh all        # All tests

# Run visual validation tests
./run_integration_tests.sh visual


Please start running through the integration tests and highlight what is failing. Don't fix anything yet.
