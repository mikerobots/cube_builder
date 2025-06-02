// Standard library includes first
#include <iostream>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include <algorithm>

// Application headers
#include "cli/Application.h"

int main(int argc, char* argv[]) {
    std::cout << "Voxel Editor CLI v1.0" << std::endl;
    std::cout << "====================\n" << std::endl;
    
    VoxelEditor::Application app;
    
    if (!app.initialize(argc, argv)) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    return app.run();
}