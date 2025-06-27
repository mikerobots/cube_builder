#include <iostream>
#include "core/voxel_data/VoxelDataManager.h"
#include "foundation/events/EventDispatcher.h"
#include "foundation/logging/Logger.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

int main() {
    // Initialize logger
    auto& logger = Logging::Logger::getInstance();
    logger.setLevel(Logging::LogLevel::Debug);
    logger.clearOutputs();
    logger.addOutput(std::make_unique<Logging::ConsoleOutput>("Test"));
    
    // Create event dispatcher and voxel manager
    Events::EventDispatcher eventDispatcher;
    VoxelDataManager voxelManager(&eventDispatcher);
    
    std::cout << "\n=== Testing Different Resolution Overlap Detection ===\n";
    
    // Test 1: Place a 16cm voxel at (0,0,0)
    Vector3i pos16cm(0, 0, 0);
    bool placed16cm = voxelManager.setVoxel(pos16cm, VoxelResolution::Size_16cm, true);
    std::cout << "Placed 16cm voxel at (0,0,0): " << (placed16cm ? "SUCCESS" : "FAILED") << "\n";
    
    // Test 2: Try to place a 1cm voxel at (0,0,0) - should fail due to overlap
    Vector3i pos1cm_center(0, 0, 0);
    bool placed1cm_center = voxelManager.setVoxel(pos1cm_center, VoxelResolution::Size_1cm, true);
    std::cout << "Try to place 1cm voxel at (0,0,0): " << (placed1cm_center ? "SUCCESS (BUG!)" : "FAILED (correct)") << "\n";
    
    // Test 3: Try to place a 1cm voxel at (5,5,5) - should fail due to overlap
    Vector3i pos1cm_inside(5, 5, 5);
    bool placed1cm_inside = voxelManager.setVoxel(pos1cm_inside, VoxelResolution::Size_1cm, true);
    std::cout << "Try to place 1cm voxel at (5,5,5): " << (placed1cm_inside ? "SUCCESS (BUG!)" : "FAILED (correct)") << "\n";
    
    // Test 4: Try to place a 1cm voxel at (9,0,0) - should succeed (outside bounds)
    Vector3i pos1cm_outside(9, 0, 0);
    bool placed1cm_outside = voxelManager.setVoxel(pos1cm_outside, VoxelResolution::Size_1cm, true);
    std::cout << "Try to place 1cm voxel at (9,0,0): " << (placed1cm_outside ? "SUCCESS (correct)" : "FAILED (BUG!)") << "\n";
    
    // Test 5: Check overlap detection directly
    Vector3i testPos(3, 3, 3);
    bool wouldOverlap = voxelManager.wouldOverlap(testPos, VoxelResolution::Size_1cm);
    std::cout << "Would 1cm voxel at (3,3,3) overlap? " << (wouldOverlap ? "YES (correct)" : "NO (BUG!)") << "\n";
    
    // Print voxel counts
    std::cout << "\nVoxel counts:\n";
    std::cout << "16cm voxels: " << voxelManager.getVoxelCount(VoxelResolution::Size_16cm) << "\n";
    std::cout << "1cm voxels: " << voxelManager.getVoxelCount(VoxelResolution::Size_1cm) << "\n";
    
    return 0;
}