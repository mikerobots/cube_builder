set(VOXEL_DATA_SOURCES
    SparseOctree.cpp
    VoxelGrid.cpp
)

set(VOXEL_DATA_HEADERS
    VoxelTypes.h
    SparseOctree.h
    VoxelGrid.h
    WorkspaceManager.h
    VoxelDataManager.h
)

add_library(VoxelEditor_VoxelData ${VOXEL_DATA_SOURCES})

target_include_directories(VoxelEditor_VoxelData PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VoxelEditor_VoxelData PUBLIC
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Memory
)

target_compile_features(VoxelEditor_VoxelData PUBLIC cxx_std_20)

# Add tests subdirectory
add_subdirectory(tests)