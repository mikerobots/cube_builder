# Core Surface Generation System
add_library(VoxelEditor_SurfaceGen STATIC
    SurfaceTypes.h
    SurfaceTypes.cpp
    MeshBuilder.h
    MeshBuilder.cpp
    MeshSmoother.h
    MeshSmoother.cpp
    TopologyPreserver.h
    TopologyPreserver.cpp
    MeshValidator.h
    MeshValidator.cpp
    DualContouring.h
    DualContouring.cpp
    DualContouringFast.h
    DualContouringFast.cpp
    DualContouringSparse.h
    DualContouringSparse.cpp
    DualContouringNEON.h
    DualContouringNEON.cpp
    DualContouringTables.h
    EdgeCache.h
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
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()