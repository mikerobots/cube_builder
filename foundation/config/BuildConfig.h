#pragma once

// Build configuration options that can be set at compile time

// VOXEL_ASSERT_ON_MISSING_SHADERS - When enabled, the application will assert and exit
// if shader files are not found. This is useful for catching missing resources early
// in development. Defaults to enabled in Debug builds.
#ifndef VOXEL_ASSERT_ON_MISSING_SHADERS
    #ifdef NDEBUG
        // Release build - disable by default
        #define VOXEL_ASSERT_ON_MISSING_SHADERS 0
    #else
        // Debug build - enable by default
        #define VOXEL_ASSERT_ON_MISSING_SHADERS 1
    #endif
#endif

// Utility macro for shader file assertions
#if VOXEL_ASSERT_ON_MISSING_SHADERS
    #include <cassert>
    #include <iostream>
    #define VOXEL_ASSERT_SHADER_FILE(condition, message) \
        do { \
            if (!(condition)) { \
                std::cerr << "SHADER ASSERTION FAILED: " << (message) << std::endl; \
                std::cerr << "File: " << __FILE__ << ", Line: " << __LINE__ << std::endl; \
                assert(condition); \
            } \
        } while(0)
#else
    #define VOXEL_ASSERT_SHADER_FILE(condition, message) ((void)0)
#endif