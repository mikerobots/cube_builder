set(CAMERA_HEADERS
    Camera.h
    OrbitCamera.h
    Viewport.h
    CameraController.h
)

add_library(VoxelEditor_Camera INTERFACE)

target_include_directories(VoxelEditor_Camera INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VoxelEditor_Camera INTERFACE
    VoxelEditor_Math
    VoxelEditor_Events
)

target_compile_features(VoxelEditor_Camera INTERFACE cxx_std_20)

# Add tests subdirectory
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()