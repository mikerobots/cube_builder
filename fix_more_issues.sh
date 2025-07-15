#!/bin/bash
# Fix more compilation issues

# Fix namespace issues in group tests
echo "Fixing namespace issues in group tests..."
sed -i '' 's/Math::WorldCoordinates/VoxelEditor::Math::WorldCoordinates/g' /Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_types.cpp
sed -i '' 's/Math::WorldCoordinates/VoxelEditor::Math::WorldCoordinates/g' /Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_voxel_group.cpp
sed -i '' 's/Math::WorldCoordinates/VoxelEditor::Math::WorldCoordinates/g' /Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_operations.cpp

# Fix syntax errors in group tests (missing closing parentheses)
echo "Fixing syntax errors..."
sed -i '' 's/Vector3f(0.1f, 0, 0);/Vector3f(0.1f, 0, 0));/g' /Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_types.cpp
sed -i '' 's/Vector3f(0, 0, 0);/Vector3f(0, 0, 0));/g' /Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_types.cpp
sed -i '' 's/Vector3f(1.0f, 0.0f, 0.0f));/Vector3f(1.0f, 0.0f, 0.0f)));/g' /Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_operations.cpp

# Fix comparison issues
echo "Fixing comparisons between WorldCoordinates and Vector3f..."
sed -i '' 's/EXPECT_EQ(transform1.translation, Vector3f(/EXPECT_EQ(transform1.translation.value(), Vector3f(/g' /Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_types.cpp

# Fix STLExporter.cpp
echo "Fixing STLExporter.cpp..."
sed -i '' 's/vertex.position += translation;/vertex.position = VoxelEditor::Math::WorldCoordinates(vertex.position.value() + translation);/g' /Users/michaelhalloran/cube_edit/core/file_io/src/STLExporter.cpp

# Fix BinaryFormat.cpp - coordinate accessors need parentheses
echo "Fixing BinaryFormat.cpp..."
sed -i '' 's/pivot\.x);/pivot.x());/g' /Users/michaelhalloran/cube_edit/core/file_io/src/BinaryFormat.cpp
sed -i '' 's/pivot\.y);/pivot.y());/g' /Users/michaelhalloran/cube_edit/core/file_io/src/BinaryFormat.cpp
sed -i '' 's/pivot\.z);/pivot.z());/g' /Users/michaelhalloran/cube_edit/core/file_io/src/BinaryFormat.cpp
sed -i '' 's/newGroup->setPivot(pivot);/newGroup->setPivot(Math::WorldCoordinates(pivot));/g' /Users/michaelhalloran/cube_edit/core/file_io/src/BinaryFormat.cpp

echo "Fixes applied!"