#include <iostream>
#include <iomanip>
#include "core/voxel_data/VoxelDataManager.h"
#include "foundation/events/EventDispatcher.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

void printVoxelBounds(const IncrementCoordinates& pos, VoxelResolution res) {
    VoxelPosition voxelPos(pos, res);
    Vector3f min, max;
    voxelPos.getWorldBounds(min, max);
    
    std::cout << "Voxel at increment pos (" << pos.x() << "," << pos.y() << "," << pos.z() << ") "
              << "with resolution " << getVoxelSizeName(res) << ":\n";
    std::cout << "  World bounds: min=(" << std::fixed << std::setprecision(3) 
              << min.x << "," << min.y << "," << min.z << ") max=(" 
              << max.x << "," << max.y << "," << max.z << ")\n";
}

int main() {
    std::cout << "\n=== Debugging Collision Detection ===\n\n";
    
    // Show how voxel bounds work
    std::cout << "Understanding voxel bounds:\n";
    printVoxelBounds(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_16cm);
    printVoxelBounds(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm);
    printVoxelBounds(IncrementCoordinates(8, 8, 8), VoxelResolution::Size_1cm);
    
    // Create voxel manager
    Events::EventDispatcher dispatcher;
    VoxelDataManager manager(&dispatcher);
    
    std::cout << "\nTest scenarios:\n";
    
    // Scenario 1: 16cm voxel at origin
    std::cout << "\n1. Placing 16cm voxel at (0,0,0):\n";
    bool result1 = manager.setVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_16cm, true);
    std::cout << "   Result: " << (result1 ? "SUCCESS" : "FAILED") << "\n";
    
    // Scenario 2: Try 1cm voxel at various positions
    std::vector<IncrementCoordinates> testPositions = {
        IncrementCoordinates(0, 0, 0),    // Center - should overlap
        IncrementCoordinates(7, 7, 7),    // Inside bounds - should overlap
        IncrementCoordinates(8, 8, 8),    // Edge - should overlap
        IncrementCoordinates(9, 0, 0),    // Outside X - should succeed
        IncrementCoordinates(0, 17, 0),   // Outside Y - should succeed
        IncrementCoordinates(-9, 0, 0)    // Outside -X - should succeed
    };
    
    for (const auto& pos : testPositions) {
        std::cout << "\n2. Trying 1cm voxel at (" << pos.x() << "," << pos.y() << "," << pos.z() << "):\n";
        
        // Check overlap first
        bool wouldOverlap = manager.wouldOverlap(pos, VoxelResolution::Size_1cm);
        std::cout << "   Would overlap: " << (wouldOverlap ? "YES" : "NO") << "\n";
        
        // Try to place
        bool placed = manager.setVoxel(pos, VoxelResolution::Size_1cm, true);
        std::cout << "   Placement result: " << (placed ? "SUCCESS" : "FAILED") << "\n";
        
        // Show expected behavior
        printVoxelBounds(pos, VoxelResolution::Size_1cm);
    }
    
    // Print final voxel counts
    std::cout << "\nFinal voxel counts:\n";
    std::cout << "  16cm voxels: " << manager.getVoxelCount(VoxelResolution::Size_16cm) << "\n";
    std::cout << "  1cm voxels: " << manager.getVoxelCount(VoxelResolution::Size_1cm) << "\n";
    
    return 0;
}