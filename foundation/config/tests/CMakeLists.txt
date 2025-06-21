find_package(GTest REQUIRED)

set(CONFIG_TEST_SOURCES
    test_unit_foundation_config_value.cpp
    test_unit_foundation_config_section.cpp
    test_unit_foundation_config_manager.cpp
)

add_executable(test_unit_foundation_config ${CONFIG_TEST_SOURCES})

target_link_libraries(test_unit_foundation_config
    VoxelEditor_Config
    VoxelEditor_Events
    VoxelEditor_Math
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(test_unit_foundation_config PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(test_unit_foundation_config)