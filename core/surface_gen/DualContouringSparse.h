#pragma once

#include "DualContouring.h"
#include <unordered_set>
#include <thread>
#include <mutex>

namespace VoxelEditor {
namespace SurfaceGen {

/**
 * Sparse dual contouring implementation that only processes regions containing voxels.
 * This dramatically improves performance for typical sparse voxel grids.
 */
class DualContouringSparse : public DualContouring {
public:
    DualContouringSparse() = default;
    
protected:
    // Override the edge extraction to use sparse traversal
    void extractEdgeIntersections(const VoxelData::VoxelGrid& grid) override;
    
private:
    // Build set of cells that need processing
    std::unordered_set<uint64_t> buildActiveCellSet(const VoxelData::VoxelGrid& grid);
    
    // Process a batch of cells in parallel
    void processActiveCellsParallel(const VoxelData::VoxelGrid& grid,
                                   const std::unordered_set<uint64_t>& activeCells);
    
    // Process a single cell
    void processCell(const Math::IncrementCoordinates& cellPos,
                    const VoxelData::VoxelGrid& grid);
    
    // Thread-safe cell data insertion
    std::mutex m_cellDataMutex;
};

}
}