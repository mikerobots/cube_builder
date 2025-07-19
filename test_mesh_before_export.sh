#!/bin/bash

echo "Testing mesh generation before STL export..."

# Create a simple test program that generates and prints mesh
cat > test_mesh_gen.cpp << 'EOF'
#include <iostream>
#include <iomanip>

void printMeshDebug(const char* stage) {
    std::cout << "\n=== MESH DEBUG: " << stage << " ===" << std::endl;
}

int main() {
    // Simulate the mesh generation output we're seeing
    std::cout << "Generating mesh with 1 voxel at (0,0,0), size 32cm" << std::endl;
    
    // The bad vertices we're seeing
    float vertices[][3] = {
        {2.375f, -0.875f, -3.25f},
        {0.0f, 1000.0f, 0.0f},
        {-2.666667f, 2.666667f, -2.666667f},
        {2.375f, -0.875f, 3.25f},
        {-2.666667f, 2.666667f, 2.666667f}
    };
    
    std::cout << "\nVertex positions:" << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << "V" << i << ": (" 
                  << std::fixed << std::setprecision(6)
                  << vertices[i][0] << ", " 
                  << vertices[i][1] << ", " 
                  << vertices[i][2] << ")" << std::endl;
                  
        // Check for suspicious values
        if (vertices[i][1] >= 100.0f) {
            std::cout << "  WARNING: Y coordinate is " << vertices[i][1] << " - likely uninitialized!" << std::endl;
        }
    }
    
    // Expected cube vertices for a 32cm voxel at origin
    std::cout << "\nExpected vertices for 32cm voxel at (0,0,0):" << std::endl;
    std::cout << "Bottom-center coordinate system, so voxel extends:" << std::endl;
    std::cout << "  X: -16cm to +16cm" << std::endl;
    std::cout << "  Y: 0cm to 32cm" << std::endl;
    std::cout << "  Z: -16cm to +16cm" << std::endl;
    std::cout << "\nCube corners should be:" << std::endl;
    std::cout << "  (-0.16, 0.00, -0.16)" << std::endl;
    std::cout << "  ( 0.16, 0.00, -0.16)" << std::endl;
    std::cout << "  (-0.16, 0.00,  0.16)" << std::endl;
    std::cout << "  ( 0.16, 0.00,  0.16)" << std::endl;
    std::cout << "  (-0.16, 0.32, -0.16)" << std::endl;
    std::cout << "  ( 0.16, 0.32, -0.16)" << std::endl;
    std::cout << "  (-0.16, 0.32,  0.16)" << std::endl;
    std::cout << "  ( 0.16, 0.32,  0.16)" << std::endl;
    
    return 0;
}
EOF

# Compile and run
g++ -o test_mesh_gen test_mesh_gen.cpp && ./test_mesh_gen

# Clean up
rm -f test_mesh_gen test_mesh_gen.cpp