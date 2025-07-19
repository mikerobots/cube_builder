#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <stl_file>" << std::endl;
        return 1;
    }
    
    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 1;
    }
    
    // Skip 80-byte header
    char header[80];
    file.read(header, 80);
    
    // Read triangle count
    uint32_t triangleCount;
    file.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));
    
    std::cout << "STL File: " << argv[1] << std::endl;
    std::cout << "Triangle count: " << triangleCount << std::endl;
    std::cout << "\nFirst 10 triangles (or all if less):\n" << std::endl;
    
    // Read triangles
    for (uint32_t i = 0; i < triangleCount && i < 10; ++i) {
        float normal[3], v0[3], v1[3], v2[3];
        uint16_t attributeByteCount;
        
        // Read normal
        file.read(reinterpret_cast<char*>(normal), 3 * sizeof(float));
        
        // Read vertices
        file.read(reinterpret_cast<char*>(v0), 3 * sizeof(float));
        file.read(reinterpret_cast<char*>(v1), 3 * sizeof(float));
        file.read(reinterpret_cast<char*>(v2), 3 * sizeof(float));
        
        // Read attribute byte count
        file.read(reinterpret_cast<char*>(&attributeByteCount), sizeof(uint16_t));
        
        std::cout << "Triangle " << i << ":" << std::endl;
        std::cout << "  Normal: (" << normal[0] << ", " << normal[1] << ", " << normal[2] << ")" << std::endl;
        std::cout << "  Vertex 0: (" << v0[0] << ", " << v0[1] << ", " << v0[2] << ")" << std::endl;
        std::cout << "  Vertex 1: (" << v1[0] << ", " << v1[1] << ", " << v1[2] << ")" << std::endl;
        std::cout << "  Vertex 2: (" << v2[0] << ", " << v2[1] << ", " << v2[2] << ")" << std::endl;
        std::cout << std::endl;
        
        if (!file.good()) {
            std::cerr << "Error reading triangle " << i << std::endl;
            break;
        }
    }
    
    return 0;
}