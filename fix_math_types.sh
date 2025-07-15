#!/bin/bash
# Fix unqualified Math types in a file

FILE=$1

# Replace IncrementCoordinates with Math::IncrementCoordinates
sed -i '' 's/\([^:]\)IncrementCoordinates(/\1Math::IncrementCoordinates(/g' "$FILE"

# Replace Vector3i with Math::Vector3i  
sed -i '' 's/\([^:]\)Vector3i(/\1Math::Vector3i(/g' "$FILE"

# Replace Vector3f with Math::Vector3f
sed -i '' 's/\([^:]\)Vector3f(/\1Math::Vector3f(/g' "$FILE"

# Replace WorldCoordinates with Math::WorldCoordinates
sed -i '' 's/\([^:]\)WorldCoordinates(/\1Math::WorldCoordinates(/g' "$FILE"

# Replace BoundingBox with Math::BoundingBox (when not prefixed)
sed -i '' 's/\([^:]\)BoundingBox /\1Math::BoundingBox /g' "$FILE"

echo "Fixed Math types in $FILE"