#!/bin/bash
# Fix coordinate type compilation errors

echo "Fixing STLExporter.cpp..."
# The issue is trying to add Vector3f to WorldCoordinates
# Change: vertex.position += translation;
# To: vertex.position = WorldCoordinates(vertex.position.value() + translation);
sed -i '' 's|vertex\.position += translation;|vertex.position = VoxelEditor::Math::WorldCoordinates(vertex.position.value() + translation);|g' /Users/michaelhalloran/cube_edit/core/file_io/src/STLExporter.cpp

echo "Fixing BinaryFormat.cpp..."
# Fix coordinate accessor calls - need parentheses
sed -i '' 's|w\.writeFloat(pivot\.x);|w.writeFloat(pivot.x());|g' /Users/michaelhalloran/cube_edit/core/file_io/src/BinaryFormat.cpp
sed -i '' 's|w\.writeFloat(pivot\.y);|w.writeFloat(pivot.y());|g' /Users/michaelhalloran/cube_edit/core/file_io/src/BinaryFormat.cpp
sed -i '' 's|w\.writeFloat(pivot\.z);|w.writeFloat(pivot.z());|g' /Users/michaelhalloran/cube_edit/core/file_io/src/BinaryFormat.cpp
# Fix setPivot call
sed -i '' 's|newGroup->setPivot(pivot);|newGroup->setPivot(Math::WorldCoordinates(pivot));|g' /Users/michaelhalloran/cube_edit/core/file_io/src/BinaryFormat.cpp

echo "Fixing group test files..."
# Fix namespace issues in group tests
for file in /Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_types.cpp \
            /Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_voxel_group.cpp \
            /Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_operations.cpp; do
    if [ -f "$file" ]; then
        # Add namespace alias if not already present
        if ! grep -q "namespace Math = VoxelEditor::Math;" "$file"; then
            # Find the using namespace lines and add after them
            sed -i '' '/using namespace VoxelEditor::Groups;/a\
namespace Math = VoxelEditor::Math;' "$file"
        fi
    fi
done

# Fix specific issues in test_unit_core_groups_types.cpp
FILE="/Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_types.cpp"
if [ -f "$FILE" ]; then
    # Fix missing closing parentheses
    sed -i '' 's|VoxelEditor::Math::WorldCoordinates(Vector3f(0.1f, 0, 0));|VoxelEditor::Math::WorldCoordinates(Vector3f(0.1f, 0, 0)));|g' "$FILE"
    sed -i '' 's|VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 0));|VoxelEditor::Math::WorldCoordinates(Vector3f(0, 0, 0)));|g' "$FILE"
    
    # Fix comparison between WorldCoordinates and Vector3f
    sed -i '' 's|EXPECT_EQ(transform1.translation, Vector3f(|EXPECT_TRUE(transform1.translation.isEqualTo(Vector3f(|g' "$FILE"
    sed -i '' 's|EXPECT_EQ(metadata.pivot.value(), Vector3f(|EXPECT_TRUE(metadata.pivot.isEqualTo(Vector3f(|g' "$FILE"
    # Close the extra parenthesis
    sed -i '' 's|Vector3f(0, 0, 0));|Vector3f(0, 0, 0)));|g' "$FILE"
fi

# Fix specific issues in test_unit_core_groups_operations.cpp
FILE="/Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_operations.cpp"
if [ -f "$FILE" ]; then
    # Fix missing closing parenthesis
    sed -i '' 's|VoxelEditor::Math::WorldCoordinates(Vector3f(1.0f, 0.0f, 0.0f)));|VoxelEditor::Math::WorldCoordinates(Vector3f(1.0f, 0.0f, 0.0f)));|g' "$FILE"
fi

echo "All fixes applied!"