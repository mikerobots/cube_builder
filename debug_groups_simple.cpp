#include <iostream>
#include <chrono>
#include <thread>
#include "core/groups/include/groups/GroupManager.h"
#include "core/voxel_data/VoxelDataManager.h"

using namespace VoxelEditor::Groups;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

int main() {
    std::cout << "Creating VoxelDataManager..." << std::endl;
    auto realVoxelManager = std::make_unique<VoxelDataManager>();
    std::cout << "VoxelDataManager created" << std::endl;
    
    std::cout << "Creating GroupManager..." << std::endl;
    auto groupManager = std::make_unique<GroupManager>(realVoxelManager.get(), nullptr);
    std::cout << "GroupManager created" << std::endl;
    
    std::cout << "Creating test group..." << std::endl;
    GroupId testGroupId = groupManager->createGroup("Test Group");
    std::cout << "Test group created with ID: " << testGroupId << std::endl;
    
    std::cout << "Adding voxels..." << std::endl;
    for (int i = 0; i < 3; ++i) {
        std::cout << "  Adding voxel " << i << std::endl;
        VoxelResolution res = VoxelResolution::Size_32cm;
        
        // Create VoxelId using grid coordinates for the group
        GridCoordinates gridPos(Vector3i(i, 0, 0));
        VoxelId voxel(gridPos, res);
        
        // Convert grid coordinates to increment coordinates for VoxelDataManager
        Vector3f workspaceSize = realVoxelManager->getWorkspaceSize();
        IncrementCoordinates incrementPos = CoordinateConverter::gridToIncrement(
            gridPos, res, workspaceSize);
        realVoxelManager->setVoxel(incrementPos, res, true);
        
        std::cout << "    About to add voxel to group..." << std::endl;
        groupManager->addVoxelToGroup(testGroupId, voxel);
        std::cout << "    Voxel " << i << " added to group" << std::endl;
    }
    
    std::cout << "Setup complete" << std::endl;
    return 0;
}