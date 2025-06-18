#include <iostream>
#include "core/groups/include/groups/GroupOperations.h"
#include "core/groups/include/groups/GroupManager.h"
#include "core/voxel_data/VoxelDataManager.h"

using namespace VoxelEditor::Groups;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

int main() {
    // Create a simple test
    auto voxelManager = std::make_unique<VoxelDataManager>();
    auto groupManager = std::make_unique<GroupManager>(voxelManager.get(), nullptr);
    
    // Create a group with one voxel
    GroupId groupId = groupManager->createGroup("Test Group");
    std::cout << "Created group with ID: " << groupId << std::endl;
    
    // Create and add a voxel
    GridCoordinates gridPos(Vector3i(0, 0, 0));
    VoxelId voxel(gridPos, VoxelResolution::Size_32cm);
    
    // Set voxel in VoxelDataManager
    bool setResult = voxelManager->setVoxel(voxel.position.value(), voxel.resolution, true);
    std::cout << "Set voxel result: " << setResult << std::endl;
    
    // Add to group
    groupManager->addVoxelToGroup(groupId, voxel);
    
    auto group = groupManager->getGroup(groupId);
    std::cout << "Group voxel count: " << group->getVoxelCount() << std::endl;
    
    // Try to move the group
    WorldCoordinates offset(Vector3f(1.0f, 0.0f, 0.0f));
    MoveGroupOperation moveOp(groupManager.get(), voxelManager.get(), groupId, offset);
    
    std::cout << "Executing move operation..." << std::endl;
    bool result = moveOp.execute();
    std::cout << "Move operation result: " << result << std::endl;
    
    if (!result) {
        std::cout << "Move operation failed" << std::endl;
    } else {
        std::cout << "Move operation succeeded" << std::endl;
        std::cout << "Group voxel count after move: " << group->getVoxelCount() << std::endl;
    }
    
    return 0;
}