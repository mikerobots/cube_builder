# Core Surface Generation System
add_library(core_surface_gen STATIC
    SurfaceTypes.h
    SurfaceTypes.cpp
    MeshBuilder.h
    MeshBuilder.cpp
    DualContouring.h
    DualContouring.cpp
    SurfaceGenerator.h
    SurfaceGenerator.cpp
)

target_include_directories(core_surface_gen
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(core_surface_gen
    PUBLIC
        VoxelEditor_Math
        VoxelEditor_Events
        VoxelEditor_VoxelData
)

# Set compile features
target_compile_features(core_surface_gen PUBLIC cxx_std_17)

# Add tests
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()