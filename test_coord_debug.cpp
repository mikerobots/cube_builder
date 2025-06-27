#include <iostream>

int main() {
    // Test the coordinate conversion used in the boundary test
    float workspaceSize = 5.0f; // 5 meter workspace
    float halfX = workspaceSize / 2.0f; // 2.5
    
    // This is what the test places: halfX - 0.16f = 2.5 - 0.16 = 2.34
    float worldX = 2.34f;
    
    // Manual coordinate conversion (worldToIncrement logic)
    // From CoordinateConverter: increment = world * 100 (convert meters to centimeters)
    int incrementX = static_cast<int>(worldX * 100.0f); // 234
    
    std::cout << "World position X: " << worldX << std::endl;
    std::cout << "Expected increment X: " << incrementX << std::endl;
    
    // What the boundary fix is checking (240, 8, 0)
    std::cout << "Boundary fix checks: 240cm instead of 234cm" << std::endl;
    std::cout << "The issue: we're checking the wrong coordinate!" << std::endl;
    
    return 0;
}