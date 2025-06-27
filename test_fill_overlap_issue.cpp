#include <iostream>
#include <memory>
#include "core/voxel_data/VoxelDataManager.h"
#include "core/undo_redo/VoxelCommands.h"
#include "foundation/events/EventDispatcher.h"
#include "foundation/math/CoordinateTypes.h"
#include "foundation/logging/Logger.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;
using namespace VoxelEditor::UndoRedo;

int main() {
    // Initialize logger
    auto& logger = Logging::Logger::getInstance();
    logger.setLevel(Logging::LogLevel::Debug);
    logger.clearOutputs();
    logger.addOutput(std::make_unique<Logging::ConsoleOutput>("Test"));
    
    // Create managers
    auto eventDispatcher = std::make_unique<EventDispatcher>();
    auto voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
    
    // Set workspace
    voxelManager->resizeWorkspace(Vector3f(5.0f, 5.0f, 5.0f));
    
    std::cout << "\n=== Testing Fill Command Overlap Issue ===\n";
    
    // Step 1: Place a 1cm voxel at (0,0,0)
    std::cout << "\n1. Placing 1cm voxel at (0,0,0)...\n";
    bool result = voxelManager->setVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_1cm, true);
    std::cout << "   Result: " << (result ? "SUCCESS" : "FAILED") << "\n";
    std::cout << "   1cm voxel count: " << voxelManager->getVoxelCount(VoxelResolution::Size_1cm) << "\n";
    
    // Step 2: Switch to 16cm resolution
    std::cout << "\n2. Switching to 16cm resolution...\n";
    voxelManager->setActiveResolution(VoxelResolution::Size_16cm);
    
    // Step 3: Try to fill a region that includes the 1cm voxel
    std::cout << "\n3. Attempting to fill region from (-16cm,-16cm,-16cm) to (16cm,16cm,16cm) with 16cm voxels...\n";
    
    // Create fill region - this should include the 1cm voxel at origin
    BoundingBox region(
        Vector3f(-0.16f, 0.0f, -0.16f),  // -16cm, 0cm, -16cm (Y=0 to respect ground plane)
        Vector3f(0.16f, 0.16f, 0.16f)    // 16cm, 16cm, 16cm
    );
    
    // Create and execute fill command
    auto fillCommand = std::make_unique<VoxelFillCommand>(
        voxelManager.get(),
        region,
        VoxelResolution::Size_16cm,
        true // fill with voxels
    );
    
    bool fillResult = fillCommand->execute();
    std::cout << "   Fill command result: " << (fillResult ? "SUCCESS" : "FAILED") << "\n";
    
    // Check the state
    std::cout << "\n4. Final state:\n";
    std::cout << "   1cm voxel count: " << voxelManager->getVoxelCount(VoxelResolution::Size_1cm) << "\n";
    std::cout << "   16cm voxel count: " << voxelManager->getVoxelCount(VoxelResolution::Size_16cm) << "\n";
    
    // Check if 16cm voxel was placed at origin
    bool has16cmAtOrigin = voxelManager->hasVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_16cm);
    std::cout << "   16cm voxel at (0,0,0): " << (has16cmAtOrigin ? "YES" : "NO") << "\n";
    
    // Check specific positions that should have failed
    std::cout << "\n5. Checking overlap detection:\n";
    
    // The 16cm voxel at (0,0,0) should overlap with 1cm voxel at (0,0,0)
    bool wouldOverlap = voxelManager->wouldOverlap(Vector3i(0, 0, 0), VoxelResolution::Size_16cm);
    std::cout << "   Would 16cm voxel at (0,0,0) overlap? " << (wouldOverlap ? "YES" : "NO") << "\n";
    
    // Try to manually place a 16cm voxel at origin to see if collision is detected
    std::cout << "\n6. Manually trying to place 16cm voxel at (0,0,0)...\n";
    bool manualResult = voxelManager->setVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_16cm, true);
    std::cout << "   Result: " << (manualResult ? "SUCCESS" : "FAILED") << "\n";
    
    std::cout << "\n=== Analysis ===\n";
    if (fillResult && has16cmAtOrigin) {
        std::cout << "BUG CONFIRMED: Fill command succeeded in placing 16cm voxel despite overlap with 1cm voxel!\n";
        std::cout << "The fill command should have failed or skipped the overlapping position.\n";
    } else if (!fillResult) {
        std::cout << "Fill command correctly failed due to overlaps.\n";
    } else if (fillResult && !has16cmAtOrigin) {
        std::cout << "Fill command succeeded but correctly skipped the overlapping position.\n";
    }
    
    return 0;
}