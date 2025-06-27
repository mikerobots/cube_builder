#include <iostream>
#include <vector>
#include <algorithm>

// Debug program to analyze the OutlineRenderer issue

struct OutlineVertex {
    float position[3];
    float color[4];
    float patternCoord;
};

void analyzeBoxIndices() {
    // Simulating what happens in addBox -> addLineSegment
    std::vector<OutlineVertex> vertices;
    std::vector<uint32_t> indices;
    
    // Box has 12 edges, each edge adds 2 vertices and 2 indices
    for (int edge = 0; edge < 12; edge++) {
        uint32_t startIndex = static_cast<uint32_t>(vertices.size());
        
        // Add two vertices for the line segment
        OutlineVertex v1, v2;
        vertices.push_back(v1);
        vertices.push_back(v2);
        
        // Add indices for the line
        indices.push_back(startIndex);
        indices.push_back(startIndex + 1);
    }
    
    std::cout << "After adding a box:\n";
    std::cout << "  Vertices: " << vertices.size() << "\n";
    std::cout << "  Indices: " << indices.size() << "\n";
    std::cout << "  Max index value: " << *std::max_element(indices.begin(), indices.end()) << "\n";
    std::cout << "  Expected max index: " << (vertices.size() - 1) << "\n";
    
    // Check if any index is out of bounds
    bool hasInvalidIndex = false;
    for (size_t i = 0; i < indices.size(); i++) {
        if (indices[i] >= vertices.size()) {
            std::cout << "ERROR: Index " << i << " has value " << indices[i] 
                      << " which is >= vertex count " << vertices.size() << "\n";
            hasInvalidIndex = true;
        }
    }
    
    if (!hasInvalidIndex) {
        std::cout << "All indices are valid.\n";
    }
    
    // Simulate what happens if we render multiple boxes without clearing
    std::cout << "\nSimulating multiple boxes without clearing:\n";
    for (int box = 1; box <= 3; box++) {
        for (int edge = 0; edge < 12; edge++) {
            uint32_t startIndex = static_cast<uint32_t>(vertices.size());
            OutlineVertex v1, v2;
            vertices.push_back(v1);
            vertices.push_back(v2);
            indices.push_back(startIndex);
            indices.push_back(startIndex + 1);
        }
        std::cout << "  After box " << box << ": vertices=" << vertices.size() 
                  << ", indices=" << indices.size() 
                  << ", max_index=" << *std::max_element(indices.begin(), indices.end()) << "\n";
    }
}

int main() {
    analyzeBoxIndices();
    return 0;
}