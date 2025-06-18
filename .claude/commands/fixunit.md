You are in change of fixing all the broken unit tests one at a
time. You run all the tests with ./run_all_unit.sh. Once there is a
failure, please kill the script. Fix the failure. You may assume the
test is poorly written possibly for our old coordinate system. Now 0,0
is center of the coordinate system.

SO you want to run the tests, stop when you find a failure, fix the
failure, run the tests again, fix the failure, repeat until all
failures are fixed and the tests pass 100%.