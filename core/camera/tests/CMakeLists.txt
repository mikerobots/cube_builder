find_package(GTest REQUIRED)

set(CAMERA_TEST_SOURCES
    test_Camera.cpp
    test_OrbitCamera.cpp
    test_OrbitCamera_transformations.cpp
    test_Viewport.cpp
    test_CameraController.cpp
)

add_executable(VoxelEditor_Camera_Tests ${CAMERA_TEST_SOURCES})

target_link_libraries(VoxelEditor_Camera_Tests
    VoxelEditor_Camera
    VoxelEditor_Math
    VoxelEditor_Events
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Camera_Tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Camera_Tests)