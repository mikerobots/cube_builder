#include <iostream>
#include <cmath>
#include <iomanip>

// Test to show the snapping issue when converting world to increment coordinates

void testCoordinateSnapping() {
    std::cout << "Coordinate Snapping Issue Test\n";
    std::cout << "==============================\n\n";
    
    // Constants
    const float CM_PER_METER = 100.0f;
    const float METERS_PER_CM = 0.01f;
    const float voxelSize = 0.32f;  // 32cm
    
    std::cout << "When we convert from world coordinates to increment coordinates,\n";
    std::cout << "we lose precision because increments are integers (1 increment = 1cm)\n\n";
    
    // Test various world positions
    float worldPositions[] = {0.0f, 0.16f, 0.32f, 0.5f, 1.0f, 1.5f, 1.67f, 2.0f};
    
    std::cout << "World Position -> Increment -> Back to World\n";
    std::cout << "--------------------------------------------\n";
    
    for (float worldX : worldPositions) {
        // Convert to increment (integer centimeters)
        int incrementX = static_cast<int>(std::round(worldX * CM_PER_METER));
        
        // Convert back to world
        float backToWorldX = incrementX * METERS_PER_CM;
        
        // Calculate error
        float error = worldX - backToWorldX;
        
        std::cout << std::fixed << std::setprecision(3);
        std::cout << worldX << "m -> " << incrementX << "cm -> " << backToWorldX << "m";
        std::cout << " (error: " << error << "m)\n";
    }
    
    std::cout << "\nThis explains why using increment coordinates for the outline\n";
    std::cout << "causes it to snap to centimeter positions instead of following\n";
    std::cout << "the exact mouse position.\n\n";
    
    std::cout << "Example: Mouse at world position 1.67m\n";
    std::cout << "- Converts to 167 increment units\n";
    std::cout << "- Converts back to 1.67m (no error in this case)\n";
    std::cout << "- But if mouse was at 1.674m, it would still snap to 1.67m\n\n";
    
    std::cout << "The fix was to use the exact world coordinates for the bounding box,\n";
    std::cout << "bypassing the increment coordinate conversion entirely.\n";
}

void testMouseMovementWithSnapping() {
    std::cout << "\n\nMouse Movement With Snapping\n";
    std::cout << "============================\n\n";
    
    const float CM_PER_METER = 100.0f;
    const float METERS_PER_CM = 0.01f;
    
    // Simulate smooth mouse movement
    std::cout << "Simulating smooth mouse movement from 0.0m to 0.1m:\n";
    std::cout << "Mouse World Pos -> Increment -> Snapped World -> Jumps?\n";
    std::cout << "-------------------------------------------------------\n";
    
    float lastSnapped = -1;
    for (float mouseWorld = 0.0f; mouseWorld <= 0.1f; mouseWorld += 0.01f) {
        int increment = static_cast<int>(std::round(mouseWorld * CM_PER_METER));
        float snapped = increment * METERS_PER_CM;
        
        bool jumped = (lastSnapped >= 0 && snapped != lastSnapped);
        
        std::cout << std::fixed << std::setprecision(3);
        std::cout << mouseWorld << "m -> " << increment << "cm -> " << snapped << "m";
        if (jumped) {
            std::cout << " <-- JUMP!";
        }
        std::cout << "\n";
        
        lastSnapped = snapped;
    }
    
    std::cout << "\nNotice how the outline would jump in 1cm increments\n";
    std::cout << "instead of smoothly following the mouse.\n";
}

int main() {
    testCoordinateSnapping();
    testMouseMovementWithSnapping();
    return 0;
}