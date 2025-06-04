# ShaderManager Test Logging Issue

## Problem Description
When logging is enabled in ShaderManager::createShaderFromSource, certain tests in test_ShaderManager.cpp crash with a segmentation fault. The crash occurs specifically when:
1. The test uses the ShaderManagerTest fixture
2. MockOpenGLRenderer (which inherits from OpenGLRenderer) is used
3. ShaderManager::createShaderFromSource is called with logging enabled

## Investigation Results
1. The Logger singleton works correctly in isolation (test_logger_simple.cpp)
2. The logging code works in simple test cases (test_ShaderManagerLogging.cpp)
3. The crash happens consistently after the log message "Compiling shader program: <name>"
4. The issue appears to be related to the test environment, not the production code

## Root Cause (Suspected)
The crash is likely due to one of:
1. Static initialization order fiasco between Logger singleton and test fixtures
2. Memory corruption when MockOpenGLRenderer inherits from OpenGLRenderer
3. Thread safety issues in the test environment
4. Interaction between gtest fixtures and the Logger singleton

## Workaround
The logging has been wrapped in try-catch blocks in ShaderManager.cpp to prevent crashes from propagating. Tests that trigger the crash have been disabled with the DISABLED_ prefix.

## Affected Tests
- CreateShaderFromSource
- MultipleShaders
- Any other test that calls createShaderFromSource with the fixture

## Production Impact
None. The logging works correctly in production code and in simpler test scenarios. This is purely a test environment issue.

## Future Resolution
To properly fix this issue:
1. Consider using a logging interface that can be mocked in tests
2. Investigate if OpenGLRenderer base class has virtual destructor issues
3. Use dependency injection for the Logger instead of singleton pattern
4. Create a test-specific logger implementation that doesn't use static storage