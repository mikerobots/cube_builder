# Core/Selection CMakeLists.txt

# Selection library
add_library(VoxelEditor_Selection
    SelectionTypes.cpp
    SelectionSet.cpp
    SelectionManager.cpp
    SelectionRenderer.cpp
    BoxSelector.cpp
    SphereSelector.cpp
    FloodFillSelector.cpp
)

target_include_directories(VoxelEditor_Selection PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(VoxelEditor_Selection PUBLIC
    VoxelEditor_VoxelData
    VoxelEditor_Rendering
    VoxelEditor_FileIO
)

# Add tests subdirectory
add_subdirectory(tests)