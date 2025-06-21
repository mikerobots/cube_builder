find_package(GTest REQUIRED)

set(MATH_TEST_SOURCES
    test_unit_foundation_math_vector3f.cpp
    test_unit_foundation_math_matrix4f.cpp
    test_unit_foundation_math_coordinate_types.cpp
    test_unit_foundation_math_coordinate_converter.cpp
)

add_executable(test_unit_foundation_math ${MATH_TEST_SOURCES})

target_link_libraries(test_unit_foundation_math
    VoxelEditor_Math
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(test_unit_foundation_math PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(test_unit_foundation_math)