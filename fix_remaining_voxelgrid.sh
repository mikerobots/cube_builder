#!/bin/bash
# Fix remaining VoxelGrid references

# Update VoxelCollision.h
sed -i '' 's/VoxelEditor::Math::VoxelGrid::/VoxelEditor::Math::VoxelGridMath::/g' /Users/michaelhalloran/cube_edit/foundation/voxel_math/include/voxel_math/VoxelCollision.h

# Update remaining source files
sed -i '' 's/VoxelGrid::/VoxelGridMath::/g' /Users/michaelhalloran/cube_edit/foundation/voxel_math/src/VoxelRaycast.cpp
sed -i '' 's/VoxelGrid::/VoxelGridMath::/g' /Users/michaelhalloran/cube_edit/foundation/voxel_math/src/FaceOperations.cpp
sed -i '' 's/VoxelGrid::/VoxelGridMath::/g' /Users/michaelhalloran/cube_edit/foundation/voxel_math/src/VoxelCollision.cpp
sed -i '' 's/VoxelGrid::/VoxelGridMath::/g' /Users/michaelhalloran/cube_edit/foundation/voxel_math/src/WorkspaceValidation.cpp

# Update README
sed -i '' 's/VoxelGrid::/VoxelGridMath::/g' /Users/michaelhalloran/cube_edit/foundation/voxel_math/README.md

echo "Fixed remaining VoxelGrid references"