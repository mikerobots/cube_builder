#pragma once

#include <queue>
#include <unordered_set>

#include "SelectionTypes.h"
#include "SelectionSet.h"
#include "../voxel_data/VoxelDataManager.h"

namespace VoxelEditor {
namespace Selection {

class FloodFillSelector {
public:
    explicit FloodFillSelector(VoxelData::VoxelDataManager* voxelManager = nullptr);
    ~FloodFillSelector() = default;
    
    // Set the voxel manager
    void setVoxelManager(VoxelData::VoxelDataManager* manager) { m_voxelManager = manager; }
    
    // Basic flood fill from seed voxel
    SelectionSet selectFloodFill(const VoxelId& seed, 
                               FloodFillCriteria criteria = FloodFillCriteria::Connected6);
    
    // Flood fill with custom predicate
    SelectionSet selectFloodFillCustom(const VoxelId& seed,
                                     const SelectionPredicate& predicate);
    
    // Flood fill with maximum distance/steps
    SelectionSet selectFloodFillLimited(const VoxelId& seed,
                                      FloodFillCriteria criteria,
                                      int maxSteps);
    
    // Flood fill within bounds
    SelectionSet selectFloodFillBounded(const VoxelId& seed,
                                      FloodFillCriteria criteria,
                                      const Math::BoundingBox& bounds);
    
    // Planar flood fill (2D flood fill on a plane)
    SelectionSet selectPlanarFloodFill(const VoxelId& seed,
                                     const Math::Vector3f& planeNormal,
                                     float planeTolerance = 0.01f);
    
    // Configuration
    void setMaxVoxels(size_t max) { m_maxVoxels = max; }
    size_t getMaxVoxels() const { return m_maxVoxels; }
    
    void setDiagonalConnectivity(bool enabled) { m_diagonalConnectivity = enabled; }
    bool getDiagonalConnectivity() const { return m_diagonalConnectivity; }
    
    // Connectivity mode
    enum class ConnectivityMode {
        Face6,      // 6-connectivity (faces only)
        Edge18,     // 18-connectivity (faces + edges)
        Vertex26    // 26-connectivity (faces + edges + vertices)
    };
    
    void setConnectivityMode(ConnectivityMode mode) { m_connectivityMode = mode; }
    ConnectivityMode getConnectivityMode() const { return m_connectivityMode; }
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    size_t m_maxVoxels;
    bool m_diagonalConnectivity;
    ConnectivityMode m_connectivityMode;
    
    // Helper methods
    std::vector<VoxelId> getNeighbors(const VoxelId& voxel) const;
    bool meetsFloodFillCriteria(const VoxelId& current, const VoxelId& neighbor, 
                               FloodFillCriteria criteria) const;
    bool voxelExists(const VoxelId& voxel) const;
    bool areVoxelsConnected(const VoxelId& voxel1, const VoxelId& voxel2) const;
    
    // Internal flood fill implementation
    SelectionSet floodFillInternal(const VoxelId& seed,
                                 const std::function<bool(const VoxelId&, const VoxelId&)>& canVisit,
                                 const std::function<bool(const VoxelId&)>& shouldStop);
};

}
}