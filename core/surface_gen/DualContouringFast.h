#pragma once

#include "DualContouring.h"
#include "MeshBuilder.h"
#include <thread>
#include <future>
#include <unordered_map>

namespace VoxelEditor {
namespace SurfaceGen {

/**
 * Fast dual contouring implementation with practical optimizations:
 * 1. Sparse voxel traversal - only process occupied regions
 * 2. Early exit for empty regions
 * 
 * This is a simplified version that demonstrates the key optimization:
 * only processing regions that contain voxels instead of the entire grid.
 */
class DualContouringFast {
public:
    DualContouringFast() = default;
    
    Mesh generateMesh(const VoxelData::VoxelGrid& grid, 
                     const SurfaceSettings& settings = SurfaceSettings::Default());
};

}
}