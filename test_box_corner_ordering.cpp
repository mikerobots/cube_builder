#include <iostream>
#include <iomanip>

struct Vec3 {
    float x, y, z;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

void printCorner(const char* label, const Vec3& v) {
    std::cout << label << ": (" << std::fixed << std::setprecision(2) 
              << v.x << ", " << v.y << ", " << v.z << ")\n";
}

void analyzeBoxCorners() {
    std::cout << "Box Corner Analysis\n";
    std::cout << "==================\n\n";
    
    // Test box at origin with size 0.32
    Vec3 min(0, 0, 0);
    Vec3 max(0.32f, 0.32f, 0.32f);
    
    std::cout << "Box: min(" << min.x << ", " << min.y << ", " << min.z << ") ";
    std::cout << "max(" << max.x << ", " << max.y << ", " << max.z << ")\n\n";
    
    // Generate corners as in OutlineRenderer
    Vec3 corners[8] = {
        Vec3(min.x, min.y, min.z), // 0
        Vec3(max.x, min.y, min.z), // 1
        Vec3(max.x, max.y, min.z), // 2
        Vec3(min.x, max.y, min.z), // 3
        Vec3(min.x, min.y, max.z), // 4
        Vec3(max.x, min.y, max.z), // 5
        Vec3(max.x, max.y, max.z), // 6
        Vec3(min.x, max.y, max.z)  // 7
    };
    
    std::cout << "Corner positions:\n";
    for (int i = 0; i < 8; i++) {
        std::cout << "Corner " << i << ": (" 
                  << corners[i].x << ", " << corners[i].y << ", " << corners[i].z << ")";
        
        // Label the position
        std::cout << " - ";
        if (corners[i].y == min.y) std::cout << "Bottom";
        else std::cout << "Top";
        std::cout << ", ";
        if (corners[i].x == min.x) std::cout << "Left";
        else std::cout << "Right";
        std::cout << ", ";
        if (corners[i].z == min.z) std::cout << "Back";
        else std::cout << "Front";
        std::cout << "\n";
    }
    
    std::cout << "\nBottom face edges (Y=0):\n";
    std::cout << "0->1: (" << corners[0].x << "," << corners[0].z << ") to (" 
              << corners[1].x << "," << corners[1].z << ")\n";
    std::cout << "1->2: (" << corners[1].x << "," << corners[1].z << ") to (" 
              << corners[2].x << "," << corners[2].z << ")\n";
    std::cout << "2->3: (" << corners[2].x << "," << corners[2].z << ") to (" 
              << corners[3].x << "," << corners[3].z << ")\n";
    std::cout << "3->0: (" << corners[3].x << "," << corners[3].z << ") to (" 
              << corners[0].x << "," << corners[0].z << ")\n";
    
    std::cout << "\nWait! Corner 2 has Y=" << corners[2].y << " (should be bottom)\n";
    std::cout << "Corner 1 has Y=" << corners[1].y << " (correct - bottom)\n";
    std::cout << "\nThe issue: The 'bottom face' is connecting corners with different Y values!\n";
    std::cout << "Corners 1 and 3 are at Y=0 (bottom), but corners 2 and 0 are at Y=0.32 (top)\n";
}

int main() {
    analyzeBoxCorners();
    return 0;
}