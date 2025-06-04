#include <iostream>
#include "core/voxel_data/VoxelDataManager.h"
#include "foundation/events/EventDispatcher.h"

void testWorldSpaceOperations() {
    std::cout << "=== World Space Operations Test ===" << std::endl;
    
    // Create the same setup as the test
    VoxelEditor::Events::EventDispatcher eventDispatcher;
    VoxelEditor::VoxelData::VoxelDataManager manager(&eventDispatcher);
    
    VoxelEditor::Math::Vector3f worldPos(1.0f, 0.5f, 2.0f);
    VoxelEditor::VoxelData::VoxelResolution resolution = VoxelEditor::VoxelData::VoxelResolution::Size_4cm;
    
    std::cout << "World position: (" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")" << std::endl;
    std::cout << "Resolution: Size_4cm" << std::endl;
    
    // Check basic setup
    std::cout << "Workspace size: " << manager.getWorkspaceSize().x << "x" << manager.getWorkspaceSize().y << "x" << manager.getWorkspaceSize().z << std::endl;
    std::cout << "Is valid world position: " << (manager.isValidWorldPosition(worldPos) ? "YES" : "NO") << std::endl;
    
    // Try step by step
    std::cout << "\n--- Step 1: setVoxelAtWorldPos ---" << std::endl;
    bool setResult1 = manager.setVoxelAtWorldPos(worldPos, resolution, true);
    std::cout << "Result: " << (setResult1 ? "SUCCESS" : "FAILED") << std::endl;
    
    if (!setResult1) {
        std::cout << "Investigating failure..." << std::endl;
        const auto* grid = manager.getGrid(resolution);
        if (!grid) {
            std::cout << "ERROR: Grid is null!" << std::endl;
            return;
        }
        std::cout << "Grid exists: YES" << std::endl;
        std::cout << "Grid workspace size: " << grid->getWorkspaceSize().x << "x" << grid->getWorkspaceSize().y << "x" << grid->getWorkspaceSize().z << std::endl;
        std::cout << "Grid valid world pos: " << (grid->isValidWorldPosition(worldPos) ? "YES" : "NO") << std::endl;
        return;
    }
    
    std::cout << "\n--- Step 2: getVoxelAtWorldPos ---" << std::endl;
    bool getResult1 = manager.getVoxelAtWorldPos(worldPos, resolution);
    std::cout << "Result: " << (getResult1 ? "SUCCESS" : "FAILED") << std::endl;
    
    std::cout << "\n--- Step 3: hasVoxelAtWorldPos ---" << std::endl;
    bool hasResult1 = manager.hasVoxelAtWorldPos(worldPos, resolution);
    std::cout << "Result: " << (hasResult1 ? "SUCCESS" : "FAILED") << std::endl;
    
    // Test with active resolution
    std::cout << "\n--- Step 4: Set active resolution and test ---" << std::endl;
    manager.setActiveResolution(resolution);
    std::cout << "Active resolution set to Size_4cm" << std::endl;
    
    bool setResult2 = manager.setVoxelAtWorldPos(worldPos, true);
    std::cout << "setVoxelAtWorldPos (active res): " << (setResult2 ? "SUCCESS" : "FAILED") << std::endl;
    
    bool getResult2 = manager.getVoxelAtWorldPos(worldPos);
    std::cout << "getVoxelAtWorldPos (active res): " << (getResult2 ? "SUCCESS" : "FAILED") << std::endl;
    
    bool hasResult2 = manager.hasVoxelAtWorldPos(worldPos);
    std::cout << "hasVoxelAtWorldPos (active res): " << (hasResult2 ? "SUCCESS" : "FAILED") << std::endl;
    
    // Summary
    std::cout << "\n=== SUMMARY ===" << std::endl;
    bool allPassed = setResult1 && getResult1 && hasResult1 && setResult2 && getResult2 && hasResult2;
    std::cout << "All tests: " << (allPassed ? "PASSED" : "FAILED") << std::endl;
}

int main() {
    testWorldSpaceOperations();
    return 0;
}