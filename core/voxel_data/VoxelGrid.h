#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include "VoxelTypes.h"
#include "SparseOctree.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/BoundingBox.h"
#include "../../foundation/logging/Logger.h"

namespace VoxelEditor {
namespace VoxelData {

class VoxelGrid {
public:
    VoxelGrid(VoxelResolution resolution, const Math::Vector3f& workspaceSize)
        : m_resolution(resolution)
        , m_workspaceSize(workspaceSize)
        , m_voxelSize(VoxelData::getVoxelSize(resolution)) {
        
        // Calculate grid dimensions
        m_gridDimensions = Math::Vector3i(
            static_cast<int>(std::ceil(workspaceSize.x / m_voxelSize)),
            static_cast<int>(std::ceil(workspaceSize.y / m_voxelSize)),
            static_cast<int>(std::ceil(workspaceSize.z / m_voxelSize))
        );
        
        // Calculate octree depth based on largest dimension
        // We need enough depth so each leaf represents exactly one voxel
        int maxDim = std::max({m_gridDimensions.x, m_gridDimensions.y, m_gridDimensions.z});
        int depth = 0;
        int size = 1;
        while (size < maxDim) {
            size *= 2;
            depth++;
        }
        
        // IMPORTANT: If size equals maxDim, we still need one more level
        // to ensure each voxel has its own leaf node
        // For a 4x4x4 grid: depth=2 gives 4x4x4 nodes at the last internal level,
        // but we need depth=3 to have unique leaf nodes for each voxel
        if (size == maxDim && maxDim > 1) {
            depth++;
        }
        
        // Create sparse octree
        m_octree = std::make_unique<SparseOctree>(depth);
    }
    
    // Voxel operations
    bool setVoxel(const Math::Vector3i& pos, bool value) {
        if (!isValidGridPosition(pos)) {
            Logging::Logger::getInstance().debugfc("VoxelGrid", 
                "Attempt to set voxel at invalid position (%d, %d, %d)", pos.x, pos.y, pos.z);
            return false;
        }
        
        m_octree->setVoxel(pos, value);
        Logging::Logger::getInstance().debugfc("VoxelGrid", 
            "Set voxel at position (%d, %d, %d) to %s", pos.x, pos.y, pos.z, value ? "true" : "false");
        return true;
    }
    
    bool getVoxel(const Math::Vector3i& pos) const {
        if (!isValidGridPosition(pos)) {
            Logging::Logger::getInstance().debugfc("VoxelGrid", 
                "Attempt to get voxel at invalid position (%d, %d, %d)", pos.x, pos.y, pos.z);
            return false;
        }
        
        bool result = m_octree->getVoxel(pos);
        Logging::Logger::getInstance().debugfc("VoxelGrid", 
            "Get voxel at position (%d, %d, %d): %s", pos.x, pos.y, pos.z, result ? "true" : "false");
        return result;
    }
    
    // World space operations
    bool setVoxelAtWorldPos(const Math::Vector3f& worldPos, bool value) {
        Math::Vector3i gridPos = worldToGrid(worldPos);
        Logging::Logger::getInstance().debugfc("VoxelGrid", 
            "World position (%.3f, %.3f, %.3f) maps to grid position (%d, %d, %d)", 
            worldPos.x, worldPos.y, worldPos.z, gridPos.x, gridPos.y, gridPos.z);
        return setVoxel(gridPos, value);
    }
    
    bool getVoxelAtWorldPos(const Math::Vector3f& worldPos) const {
        Math::Vector3i gridPos = worldToGrid(worldPos);
        Logging::Logger::getInstance().debugfc("VoxelGrid", 
            "Query world position (%.3f, %.3f, %.3f) -> grid position (%d, %d, %d)", 
            worldPos.x, worldPos.y, worldPos.z, gridPos.x, gridPos.y, gridPos.z);
        return getVoxel(gridPos);
    }
    
    // Position validation
    bool isValidGridPosition(const Math::Vector3i& pos) const {
        return pos.x >= 0 && pos.x < m_gridDimensions.x &&
               pos.y >= 0 && pos.y < m_gridDimensions.y &&
               pos.z >= 0 && pos.z < m_gridDimensions.z;
    }
    
    bool isValidWorldPosition(const Math::Vector3f& worldPos) const {
        return worldPos.x >= 0 && worldPos.x <= m_workspaceSize.x &&
               worldPos.y >= 0 && worldPos.y <= m_workspaceSize.y &&
               worldPos.z >= 0 && worldPos.z <= m_workspaceSize.z;
    }
    
    // Coordinate conversion
    Math::Vector3i worldToGrid(const Math::Vector3f& worldPos) const {
        return Math::Vector3i(
            static_cast<int>(std::floor(worldPos.x / m_voxelSize)),
            static_cast<int>(std::floor(worldPos.y / m_voxelSize)),
            static_cast<int>(std::floor(worldPos.z / m_voxelSize))
        );
    }
    
    Math::Vector3f gridToWorld(const Math::Vector3i& gridPos) const {
        return Math::Vector3f(
            gridPos.x * m_voxelSize,
            gridPos.y * m_voxelSize,
            gridPos.z * m_voxelSize
        );
    }
    
    // Bulk operations
    void clear() {
        m_octree->clear();
    }
    
    // Statistics
    size_t getVoxelCount() const {
        return m_octree->getVoxelCount();
    }
    
    bool isEmpty() const {
        return getVoxelCount() == 0;
    }
    
    size_t getMemoryUsage() const {
        return sizeof(*this) + m_octree->getMemoryUsage();
    }
    
    // Memory management
    void optimizeMemory() {
        // Octree already optimizes memory automatically
    }
    
    // Data export - Get all voxels as VoxelPosition objects
    std::vector<VoxelPosition> getAllVoxels() const {
        std::vector<VoxelPosition> voxels;
        
        // Get all voxel positions from octree
        auto positions = m_octree->getAllVoxels();
        
        Logging::Logger::getInstance().debugfc("VoxelGrid", 
            "Retrieved %zu voxel positions from octree", positions.size());
        
        for (const auto& pos : positions) {
            VoxelPosition voxelPos;
            voxelPos.gridPos = pos;
            voxelPos.resolution = m_resolution;
            voxels.push_back(voxelPos);
        }
        
        return voxels;
    }
    
    // Resize workspace
    bool resizeWorkspace(const Math::Vector3f& newSize) {
        // Calculate new grid dimensions
        Math::Vector3i newDimensions(
            static_cast<int>(std::ceil(newSize.x / m_voxelSize)),
            static_cast<int>(std::ceil(newSize.y / m_voxelSize)),
            static_cast<int>(std::ceil(newSize.z / m_voxelSize))
        );
        
        // Check if any voxels would be lost
        bool wouldLoseVoxels = false;
        auto positions = m_octree->getAllVoxels();
        for (const auto& pos : positions) {
            if (pos.x >= newDimensions.x || pos.y >= newDimensions.y || pos.z >= newDimensions.z) {
                wouldLoseVoxels = true;
                break;
            }
        }
        
        if (wouldLoseVoxels) {
            return false;
        }
        
        m_workspaceSize = newSize;
        m_gridDimensions = newDimensions;
        return true;
    }
    
    // Getters
    VoxelResolution getResolution() const { return m_resolution; }
    float getVoxelSize() const { return m_voxelSize; }
    const Math::Vector3f& getWorkspaceSize() const { return m_workspaceSize; }
    const Math::Vector3i& getGridDimensions() const { return m_gridDimensions; }
    
private:
    VoxelResolution m_resolution;
    Math::Vector3f m_workspaceSize;
    Math::Vector3i m_gridDimensions;
    float m_voxelSize;
    std::unique_ptr<SparseOctree> m_octree;
};

} // namespace VoxelData
} // namespace VoxelEditor