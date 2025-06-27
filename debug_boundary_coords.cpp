#include "math/CoordinateConverter.h"
#include "math/CoordinateTypes.h"
#include <iostream>

using namespace VoxelEditor;

int main() {
    // Test the coordinate conversion used in the boundary test
    float workspaceSize = 5.0f; // 5 meter workspace
    float halfX = workspaceSize / 2.0f; // 2.5
    
    // This is what the test places: halfX - 0.16f = 2.5 - 0.16 = 2.34
    Math::WorldCoordinates worldPos(2.34f, 0.0f, 0.0f);
    Math::IncrementCoordinates increment = Math::CoordinateConverter::worldToIncrement(worldPos);
    
    std::cout << "World position: (" << worldPos.x() << ", " << worldPos.y() << ", " << worldPos.z() << ")" << std::endl;
    std::cout << "Increment position: (" << increment.x() << ", " << increment.y() << ", " << increment.z() << ")" << std::endl;
    
    // Convert back to world to verify
    Math::WorldCoordinates backToWorld = Math::CoordinateConverter::incrementToWorld(increment);
    std::cout << "Back to world: (" << backToWorld.x() << ", " << backToWorld.y() << ", " << backToWorld.z() << ")" << std::endl;
    
    return 0;
}