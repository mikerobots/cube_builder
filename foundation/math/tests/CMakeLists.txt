find_package(GTest REQUIRED)

set(MATH_TEST_SOURCES
    test_Vector3f.cpp
    test_Matrix4f.cpp
)

add_executable(VoxelEditor_Math_Tests ${MATH_TEST_SOURCES})

target_link_libraries(VoxelEditor_Math_Tests
    VoxelEditor_Math
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Math_Tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Math_Tests)