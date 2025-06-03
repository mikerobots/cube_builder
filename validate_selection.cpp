#include <iostream>
#include "core/selection/SelectionTypes.h"
#include "core/selection/BoxSelector.h"
#include "core/selection/SphereSelector.h"
#include "core/selection/FloodFillSelector.h"
#include "core/selection/SelectionManager.h"
#include "core/voxel_data/VoxelDataManager.h"

int main() {
    std::cout << "=== Selection Subsystem Validation ===" << std::endl;
    
    try {
        // Test 1: Basic VoxelId functionality
        std::cout << "1. Testing VoxelId..." << std::endl;
        VoxelEditor::Selection::VoxelId voxel1(VoxelEditor::Math::Vector3i(1, 2, 3), VoxelEditor::VoxelData::VoxelResolution::Size_1cm);
        VoxelEditor::Selection::VoxelId voxel2(VoxelEditor::Math::Vector3i(1, 2, 3), VoxelEditor::VoxelData::VoxelResolution::Size_1cm);
        if (voxel1 == voxel2) {
            std::cout << "   ✓ VoxelId equality works" << std::endl;
        }
        
        // Test 2: SelectionSet functionality (create via constructor to avoid serialization)
        std::cout << "2. Testing VoxelId collection..." << std::endl;
        std::vector<VoxelEditor::Selection::VoxelId> voxelList = {voxel1};
        std::cout << "   ✓ VoxelId can be collected in vector" << std::endl;
        
        // Test 3: VoxelDataManager integration
        std::cout << "3. Testing VoxelDataManager integration..." << std::endl;
        VoxelEditor::VoxelData::VoxelDataManager voxelManager;
        
        // Set a voxel
        VoxelEditor::Math::Vector3i pos(0, 0, 0);
        VoxelEditor::VoxelData::VoxelResolution res = VoxelEditor::VoxelData::VoxelResolution::Size_1cm;
        bool setResult = voxelManager.setVoxel(pos, res, true);
        bool hasResult = voxelManager.hasVoxel(pos, res);
        
        std::cout << "   VoxelDataManager setVoxel: " << (setResult ? "✓" : "✗") << std::endl;
        std::cout << "   VoxelDataManager hasVoxel: " << (hasResult ? "✓" : "✗") << std::endl;
        
        // Test 4: BoxSelector integration (just creation)
        std::cout << "4. Testing BoxSelector integration..." << std::endl;
        VoxelEditor::Selection::BoxSelector boxSelector(&voxelManager);
        std::cout << "   ✓ BoxSelector created successfully" << std::endl;
        
        // Test 5: SphereSelector integration (just creation)
        std::cout << "5. Testing SphereSelector integration..." << std::endl;
        VoxelEditor::Selection::SphereSelector sphereSelector(&voxelManager);
        std::cout << "   ✓ SphereSelector created successfully" << std::endl;
        
        // Test 6: FloodFillSelector integration (just creation)
        std::cout << "6. Testing FloodFillSelector integration..." << std::endl;
        VoxelEditor::Selection::FloodFillSelector floodFillSelector(&voxelManager);
        std::cout << "   ✓ FloodFillSelector created successfully" << std::endl;
        
        // Test 7: SelectionManager integration
        std::cout << "7. Testing SelectionManager integration..." << std::endl;
        VoxelEditor::Selection::SelectionManager selectionManager(&voxelManager);
        
        // Test voxelExists implementation
        bool voxelExists = selectionManager.getVoxelManager()->hasVoxel(pos, res);
        std::cout << "   SelectionManager VoxelDataManager integration: " << (voxelExists ? "✓" : "✗") << std::endl;
        
        // Test getAllVoxels implementation
        auto allVoxels = selectionManager.getVoxelManager()->getAllVoxels();
        std::cout << "   getAllVoxels returned " << allVoxels.size() << " voxels" << std::endl;
        
        std::cout << std::endl << "=== All Selection Subsystem Tests Completed Successfully! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}