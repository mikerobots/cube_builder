#!/bin/bash
# Rename VoxelGrid to VoxelGridMath in voxel_math library

# Update the source file
sed -i '' 's/VoxelGrid::/VoxelGridMath::/g' /Users/michaelhalloran/cube_edit/foundation/voxel_math/src/VoxelGrid.cpp

# Find and update all references to Math::VoxelGrid
echo "Finding all files that reference Math::VoxelGrid..."
find /Users/michaelhalloran/cube_edit -name "*.cpp" -o -name "*.h" | while read file; do
    if grep -q "Math::VoxelGrid\b" "$file" 2>/dev/null; then
        echo "Updating $file"
        sed -i '' 's/Math::VoxelGrid\b/Math::VoxelGridMath/g' "$file"
    fi
done

# Also check for voxel_math/VoxelGrid.h includes
find /Users/michaelhalloran/cube_edit -name "*.cpp" -o -name "*.h" | while read file; do
    if grep -q "voxel_math/VoxelGrid\.h" "$file" 2>/dev/null; then
        echo "Found include in $file - leaving as is (file name unchanged)"
    fi
done

echo "Rename complete!"