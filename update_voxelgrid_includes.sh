#!/bin/bash
# Update all includes from VoxelGrid.h to VoxelGridMath.h

echo "Updating all #include references..."
find /Users/michaelhalloran/cube_edit -name "*.cpp" -o -name "*.h" | while read file; do
    if grep -q "voxel_math/VoxelGrid\.h" "$file" 2>/dev/null; then
        echo "Updating include in $file"
        sed -i '' 's|voxel_math/VoxelGrid\.h|voxel_math/VoxelGridMath.h|g' "$file"
    fi
done

# Also update the include in VoxelGridMath.cpp itself
sed -i '' 's|#include "../include/voxel_math/VoxelGrid.h"|#include "../include/voxel_math/VoxelGridMath.h"|g' /Users/michaelhalloran/cube_edit/foundation/voxel_math/src/VoxelGridMath.cpp

# Update CMakeLists.txt
sed -i '' 's|src/VoxelGrid\.cpp|src/VoxelGridMath.cpp|g' /Users/michaelhalloran/cube_edit/foundation/voxel_math/CMakeLists.txt

echo "Include updates complete!"