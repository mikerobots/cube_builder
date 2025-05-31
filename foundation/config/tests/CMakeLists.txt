find_package(GTest REQUIRED)

set(CONFIG_TEST_SOURCES
    test_ConfigValue.cpp
    test_ConfigSection.cpp
    test_ConfigManager.cpp
)

add_executable(VoxelEditor_Config_Tests ${CONFIG_TEST_SOURCES})

target_link_libraries(VoxelEditor_Config_Tests
    VoxelEditor_Config
    VoxelEditor_Events
    VoxelEditor_Math
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Config_Tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Config_Tests)