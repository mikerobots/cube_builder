#include <iostream>
#include "core/selection/BoxSelector.h"
#include "foundation/math/BoundingBox.h"
#include "foundation/math/CoordinateConverter.h"

using namespace VoxelEditor;

int main() {
    // Test the same box as in the failing test
    Math::BoundingBox testBox(
        Math::Vector3f(-0.04f, 0.0f, -0.04f),
        Math::Vector3f(0.04f, 0.04f, 0.04f)
    );
    
    std::cout << "World box: " << testBox.min.x << "," << testBox.min.y << "," << testBox.min.z 
              << " to " << testBox.max.x << "," << testBox.max.y << "," << testBox.max.z << std::endl;
    
    // Convert to increment coordinates
    Math::WorldCoordinates minWorld(testBox.min);
    Math::WorldCoordinates maxWorld(testBox.max);
    
    Math::IncrementCoordinates minIncrementCoord = Math::CoordinateConverter::worldToIncrement(minWorld);
    Math::IncrementCoordinates maxIncrementCoord = Math::CoordinateConverter::worldToIncrement(maxWorld);
    
    std::cout << "Increment coords: " << minIncrementCoord.x() << "," << minIncrementCoord.y() << "," << minIncrementCoord.z()
              << " to " << maxIncrementCoord.x() << "," << maxIncrementCoord.y() << "," << maxIncrementCoord.z() << std::endl;
    
    // Test with 4cm and 8cm resolutions
    for (auto resolution : {VoxelData::VoxelResolution::Size_4cm, VoxelData::VoxelResolution::Size_8cm}) {
        float voxelSizeMeters = VoxelData::getVoxelSize(resolution);
        int voxelSizeCm = static_cast<int>(voxelSizeMeters * 100.0f);
        
        std::cout << "\nResolution: " << voxelSizeCm << "cm" << std::endl;
        
        // Snap to voxel boundaries
        Math::IncrementCoordinates snappedMin = Math::CoordinateConverter::snapToVoxelResolution(minIncrementCoord, resolution);
        Math::IncrementCoordinates snappedMax = Math::CoordinateConverter::snapToVoxelResolution(maxIncrementCoord, resolution);
        
        std::cout << "Snapped: " << snappedMin.x() << "," << snappedMin.y() << "," << snappedMin.z()
                  << " to " << snappedMax.x() << "," << snappedMax.y() << "," << snappedMax.z() << std::endl;
        
        // Count iterations
        int count = 0;
        for (int x = snappedMin.x(); x <= snappedMax.x(); x += voxelSizeCm) {
            for (int y = snappedMin.y(); y <= snappedMax.y(); y += voxelSizeCm) {
                for (int z = snappedMin.z(); z <= snappedMax.z(); z += voxelSizeCm) {
                    count++;
                    if (count < 10) {
                        std::cout << "  Voxel at: " << x << "," << y << "," << z << std::endl;
                    }
                }
            }
        }
        std::cout << "Total iterations: " << count << std::endl;
    }
    
    return 0;
}
EOF < /dev/null