#include <iostream>
#include <fstream>

// Simple program to analyze PPM files for yellow pixels
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ppm_file>" << std::endl;
        return 1;
    }
    
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }
    
    // Read PPM header
    std::string magic;
    int width, height, maxval;
    file >> magic >> width >> height >> maxval;
    file.get(); // consume newline
    
    if (magic != "P6") {
        std::cerr << "Not a binary PPM file" << std::endl;
        return 1;
    }
    
    // Count yellow pixels (R=255, G=255, B=0)
    int yellowCount = 0;
    int totalPixels = width * height;
    
    for (int i = 0; i < totalPixels; ++i) {
        unsigned char r = file.get();
        unsigned char g = file.get();
        unsigned char b = file.get();
        
        // Check for yellow-ish pixels (allow some tolerance)
        if (r > 200 && g > 200 && b < 50) {
            yellowCount++;
        }
    }
    
    std::cout << "Image: " << width << "x" << height << std::endl;
    std::cout << "Yellow pixels: " << yellowCount << " (" 
              << (100.0 * yellowCount / totalPixels) << "%)" << std::endl;
    
    return 0;
}