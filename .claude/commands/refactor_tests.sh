We want to refactor our tests for each of the subsystems in the core folder. Each folder has a run_tests.sh. We want them to have the same options. These scripts should run ALL tests in the subsystem!! The options will be:

--quick: Runs all tests under 1 sec.
--full: Runs all tests under 5 sec.
--slow: Runs all tests over 5 sec. Typically includes performance tests.

Please clear the main TODO.md file and make a detailed plan with this information. Add the following work instructions up front:
## 📋 WORK INSTRUCTIONS

**IMPORTANT**: This TODO.md file is shared across multiple agents/developers. To avoid conflicts:

1. **BEFORE STARTING ANY WORK**: Mark the item as "🔄 IN PROGRESS - [Your Name]"
2. **UPDATE STATUS**: Change back to normal when complete or if you stop working on it
3. **ATOMIC UPDATES**: Make small, frequent updates to avoid conflicts
4. **COMMUNICATE**: If you see something marked "IN PROGRESS", work on a different item

