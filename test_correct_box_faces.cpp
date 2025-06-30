#include <iostream>

// Visualize correct box corner ordering

int main() {
    std::cout << "Correct Box Face Analysis\n";
    std::cout << "========================\n\n";
    
    std::cout << "Corner numbering scheme:\n";
    std::cout << "  Y\n";
    std::cout << "  |\n";
    std::cout << "  |  7------6\n";
    std::cout << "  | /|     /|\n";
    std::cout << "  |/ |    / |\n";
    std::cout << "  3------2  |\n";
    std::cout << "  |  4---|--5\n";
    std::cout << "  | /    | /\n";
    std::cout << "  |/     |/\n";
    std::cout << "  0------1----X\n";
    std::cout << " /\n";
    std::cout << "Z\n\n";
    
    std::cout << "Based on the corner generation:\n";
    std::cout << "Corner 0: (min.x, min.y, min.z) - Left, Bottom, Back\n";
    std::cout << "Corner 1: (max.x, min.y, min.z) - Right, Bottom, Back\n";
    std::cout << "Corner 2: (max.x, max.y, min.z) - Right, Top, Back\n";
    std::cout << "Corner 3: (min.x, max.y, min.z) - Left, Top, Back\n";
    std::cout << "Corner 4: (min.x, min.y, max.z) - Left, Bottom, Front\n";
    std::cout << "Corner 5: (max.x, min.y, max.z) - Right, Bottom, Front\n";
    std::cout << "Corner 6: (max.x, max.y, max.z) - Right, Top, Front\n";
    std::cout << "Corner 7: (min.x, max.y, max.z) - Left, Top, Front\n\n";
    
    std::cout << "Correct face definitions:\n";
    std::cout << "Bottom face (Y=min): 0-1-5-4 (counterclockwise from below)\n";
    std::cout << "Top face (Y=max): 3-7-6-2 (counterclockwise from above)\n";
    std::cout << "Back face (Z=min): 0-3-2-1 (counterclockwise from back)\n";
    std::cout << "Front face (Z=max): 4-5-6-7 (counterclockwise from front)\n";
    std::cout << "Left face (X=min): 0-4-7-3 (counterclockwise from left)\n";
    std::cout << "Right face (X=max): 1-2-6-5 (counterclockwise from right)\n\n";
    
    std::cout << "The original code had:\n";
    std::cout << "Bottom: 0-1-2-3 (WRONG - mixes Y values)\n";
    std::cout << "Top: 4-5-6-7 (WRONG - mixes Y values)\n";
    
    return 0;
}