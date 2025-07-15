#!/bin/bash
# Fix coordinate type issues in group tests

# Fix test_unit_core_groups_types.cpp
FILE="/Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_types.cpp"
if [ -f "$FILE" ]; then
    # Replace Vector3f with Math::WorldCoordinates where needed
    sed -i '' 's/GroupTransform transform2(translation);/GroupTransform transform2(Math::WorldCoordinates(translation));/g' "$FILE"
    sed -i '' 's/nonIdentity.translation = Vector3f(/nonIdentity.translation = Math::WorldCoordinates(Vector3f(/g' "$FILE"
    sed -i '' 's/EXPECT_EQ(metadata.pivot, Vector3f(/EXPECT_EQ(metadata.pivot.value(), Vector3f(/g' "$FILE"
    echo "Fixed $FILE"
fi

# Fix test_unit_core_groups_voxel_group.cpp
FILE="/Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_voxel_group.cpp"
if [ -f "$FILE" ]; then
    # Replace setPivot/translate calls
    sed -i '' 's/group->setPivot(pivot);/group->setPivot(Math::WorldCoordinates(pivot));/g' "$FILE"
    sed -i '' 's/group->translate(offset);/group->translate(Math::WorldCoordinates(offset));/g' "$FILE"
    sed -i '' 's/EXPECT_EQ(group->getPivot(), pivot);/EXPECT_EQ(group->getPivot().value(), pivot);/g' "$FILE"
    echo "Fixed $FILE"
fi

# Fix test_unit_core_groups_operations.cpp
FILE="/Users/michaelhalloran/cube_edit/core/groups/tests/test_unit_core_groups_operations.cpp"
if [ -f "$FILE" ]; then
    # Fix GroupTransform constructor
    sed -i '' 's/GroupTransform transform(Vector3f(/GroupTransform transform(Math::WorldCoordinates(Vector3f(/g' "$FILE"
    # Fix coordinate accessor calls - need parentheses
    sed -i '' 's/pivot\.x/pivot.x()/g' "$FILE"
    sed -i '' 's/pivot\.y/pivot.y()/g' "$FILE"
    sed -i '' 's/pivot\.z/pivot.z()/g' "$FILE"
    echo "Fixed $FILE"
fi

echo "All coordinate type fixes applied"