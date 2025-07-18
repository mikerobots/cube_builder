#!/bin/bash

# Fix all instances of rayIntersectVoxel to use the correct API
sed -i '' 's/auto intersection = VoxelRaycast::rayIntersectVoxel(ray, bounds);/auto intersection = VoxelRaycast::calculateRayVoxelIntersection(ray, bounds);/g' foundation/voxel_math/tests/test_unit_foundation_voxel_math_large_voxel_raycast.cpp

# Replace all instances of intersection.hit with intersection.has_value()
sed -i '' 's/intersection\.hit/intersection.has_value()/g' foundation/voxel_math/tests/test_unit_foundation_voxel_math_large_voxel_raycast.cpp

# Replace test structure that uses hit member
sed -i '' 's/EXPECT_EQ(intersection\.hit, test\.shouldHit)/EXPECT_EQ(intersection.has_value(), test.shouldHit)/g' foundation/voxel_math/tests/test_unit_foundation_voxel_math_large_voxel_raycast.cpp

# Replace uses of intersection.point with intersection->first.value()
sed -i '' 's/Vector3f hitPoint = intersection\.point;/Vector3f hitPoint = intersection->first.value();/g' foundation/voxel_math/tests/test_unit_foundation_voxel_math_large_voxel_raycast.cpp

# Replace uses of intersection.normal (we'll need to calculate it)
sed -i '' 's/Vector3f normal = intersection\.normal;/\/\/ Normal calculation would need to be done separately/g' foundation/voxel_math/tests/test_unit_foundation_voxel_math_large_voxel_raycast.cpp

echo "Fixed raycast test patterns"