In the root directory, we have:

# Show available test groups
./run_integration_tests.sh

# Run specific groups
./run_integration_tests.sh core cli rendering
./run_integration_tests.sh quick      # Quick smoke tests
./run_integration_tests.sh all        # All tests

# Run visual validation tests
./run_integration_tests.sh visual


You are in change of fixing all the broken integration tests one at a
time. You may encounter issues because we changed our coordinate
system. Some tests may be invalid.  Now 0,0 is center of the
coordinate system.

SO you want to run the tests, stop when you find a failure, fix the
failure, run the tests again, fix the failure, repeat until all
failures are fixed and the tests pass 100%.

We want to make sure that all the integration tests are working. 