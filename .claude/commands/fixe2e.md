You are in change of fixing all the broken e2e tests one at a
time. You will find all of the .sh files with the patter test_e2e_*.

Run each independantly. Make sure you create a 30 second timeout where
the script is killed. Keep fixing the app code, or scripts until it
works. Look at the logs after running each scripts to make sure there
are no errors.

You may assume the test is poorly written possibly for our old
coordinate system. Now 0,0 is center of the coordinate system.

SO you want to run the tests, stop when you find a failure, fix the
failure, run the tests again, fix the failure, repeat until all
failures are fixed and the tests pass 100%.

Updat the TODO.md to have a detailed plan with a checklist. Delete all
old TODO.md items.