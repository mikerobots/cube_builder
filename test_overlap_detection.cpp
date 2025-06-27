#include <iostream>
#include <cmath>

// Simulating the overlap detection logic
bool checkOverlap(float pos1X, float pos1Y, float pos1Z, float size1,
                  float pos2X, float pos2Y, float pos2Z, float size2) {
    // Calculate bounds for voxel 1 (bottom-center placement)
    float halfSize1 = size1 * 0.5f;
    float min1X = pos1X - halfSize1;
    float max1X = pos1X + halfSize1;
    float min1Y = pos1Y;
    float max1Y = pos1Y + size1;
    float min1Z = pos1Z - halfSize1;
    float max1Z = pos1Z + halfSize1;
    
    // Calculate bounds for voxel 2 (bottom-center placement)
    float halfSize2 = size2 * 0.5f;
    float min2X = pos2X - halfSize2;
    float max2X = pos2X + halfSize2;
    float min2Y = pos2Y;
    float max2Y = pos2Y + size2;
    float min2Z = pos2Z - halfSize2;
    float max2Z = pos2Z + halfSize2;
    
    // Check for overlap (AABB intersection)
    bool overlapsX = min1X < max2X && max1X > min2X;
    bool overlapsY = min1Y < max2Y && max1Y > min2Y;
    bool overlapsZ = min1Z < max2Z && max1Z > min2Z;
    
    return overlapsX && overlapsY && overlapsZ;
}

int main() {
    // Test case 1: 16cm voxel at (0.01, 0, 0.01) and (0.07, 0, 0.13)
    float voxel1X = 0.01f, voxel1Y = 0.0f, voxel1Z = 0.01f;
    float voxel2X = 0.07f, voxel2Y = 0.0f, voxel2Z = 0.13f;
    float size = 0.16f; // 16cm
    
    bool overlaps = checkOverlap(voxel1X, voxel1Y, voxel1Z, size,
                                voxel2X, voxel2Y, voxel2Z, size);
    
    std::cout << "16cm voxel at (0.01, 0, 0.01) and (0.07, 0, 0.13): " 
              << (overlaps ? "OVERLAP" : "NO OVERLAP") << std::endl;
    
    // Calculate actual bounds
    std::cout << "Voxel 1 bounds: X[" << (voxel1X - size/2) << ", " << (voxel1X + size/2) << "]" << std::endl;
    std::cout << "Voxel 2 bounds: X[" << (voxel2X - size/2) << ", " << (voxel2X + size/2) << "]" << std::endl;
    
    // Test case 2: Check if they would touch
    float distance = std::abs(voxel2X - voxel1X);
    std::cout << "Distance between centers: " << distance << "m" << std::endl;
    std::cout << "Minimum non-overlapping distance: " << size << "m" << std::endl;
    
    return 0;
}