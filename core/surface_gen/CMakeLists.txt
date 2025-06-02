# Core Surface Generation System
add_library(VoxelEditor_SurfaceGen STATIC
    SurfaceTypes.h
    SurfaceTypes.cpp
    MeshBuilder.h
    MeshBuilder.cpp
    DualContouring.h
    DualContouring.cpp
    SurfaceGenerator.h
    SurfaceGenerator.cpp
)

target_include_directories(VoxelEditor_SurfaceGen
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(VoxelEditor_SurfaceGen
    PUBLIC
        VoxelEditor_Math
        VoxelEditor_Events
        VoxelEditor_VoxelData
)

# Set compile features
target_compile_features(VoxelEditor_SurfaceGen PUBLIC cxx_std_17)

# Add tests
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()