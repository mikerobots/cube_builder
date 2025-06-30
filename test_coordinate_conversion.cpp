#include <iostream>
#include "foundation/math/CoordinateConverter.h"
#include "foundation/math/CoordinateTypes.h"

using namespace VoxelEditor;
using namespace Math;

int main() {
    // Test world to increment conversion
    WorldCoordinates world1(1.5f, 0.0f, 1.5f);
    IncrementCoordinates inc1 = CoordinateConverter::worldToIncrement(world1);
    
    std::cout << "World (1.5, 0, 1.5) -> Increment (" 
              << inc1.x() << ", " << inc1.y() << ", " << inc1.z() << ")\n";
    
    // Convert back
    WorldCoordinates world2 = CoordinateConverter::incrementToWorld(inc1);
    std::cout << "Increment back to World: (" 
              << world2.x() << ", " << world2.y() << ", " << world2.z() << ")\n";
    
    // The issue: increment coordinates are integers, so we lose precision
    // 1.5m in world = 150cm = 150/32 = 4.6875 increments
    // But IncrementCoordinates are integers, so this becomes 4 or 5
    // When converted back: 4 * 32cm = 128cm = 1.28m (not 1.5m!)
    
    float worldPos = 1.5f;
    float cm = worldPos * 100.0f;  // 150cm
    float increments = cm / 32.0f;  // 4.6875
    int roundedInc = static_cast<int>(increments);  // 4
    float backToWorld = (roundedInc * 32.0f) / 100.0f;  // 1.28m
    
    std::cout << "\nDetailed calculation:\n";
    std::cout << "World: " << worldPos << "m\n";
    std::cout << "Centimeters: " << cm << "cm\n";
    std::cout << "Exact increments: " << increments << "\n";
    std::cout << "Rounded increments: " << roundedInc << "\n";
    std::cout << "Back to world: " << backToWorld << "m\n";
    std::cout << "Lost precision: " << (worldPos - backToWorld) << "m\n";
    
    return 0;
}