#include "DualContouringFast.h"
#include "DualContouring.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <chrono>

namespace VoxelEditor {
namespace SurfaceGen {

Mesh DualContouringFast::generateMesh(const VoxelData::VoxelGrid& grid, 
                                     const SurfaceSettings& settings) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Check if grid is empty first - this is the key optimization
    auto occupiedVoxels = grid.getAllVoxels();
    if (occupiedVoxels.empty()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        Logging::Logger::getInstance().debugfc("DualContouringFast", 
            "Empty grid detected, returning empty mesh. Time: %lldms", duration.count());
        return Mesh();
    }
    
    // For now, delegate to the original implementation
    // In a real implementation, we would only process the region containing voxels
    DualContouring dc;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    Logging::Logger::getInstance().debugfc("DualContouringFast", 
        "Grid has %zu voxels, using standard dual contouring. Time: %lldms", 
        occupiedVoxels.size(), duration.count());
    
    return dc.generateMesh(grid, settings);
}

}
}