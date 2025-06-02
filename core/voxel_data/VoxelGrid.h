#pragma once

#include <memory>

#include "VoxelTypes.h"
#include "SparseOctree.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector3i.h"

namespace VoxelEditor {
namespace VoxelData {

class VoxelGrid {
public:
    VoxelGrid(::VoxelEditor::VoxelData::VoxelResolution resolution, const Math::Vector3f& workspaceSize)
        : m_resolution(resolution)
        , m_workspaceSize(workspaceSize)
        , m_voxelCount(0) {
        
        // Calculate maximum grid dimensions for this resolution
        m_maxDimensions = ::VoxelEditor::VoxelData::calculateMaxGridDimensions(resolution, workspaceSize);
        
        // Calculate octree depth needed to contain the maximum dimensions
        int maxDim = std::max({m_maxDimensions.x, m_maxDimensions.y, m_maxDimensions.z});
        int depth = 0;
        int size = 1;
        while (size < maxDim) {
            size <<= 1;
            depth++;
        }
        
        m_octree = std::make_unique<SparseOctree>(depth + 1);
    }
    
    ~VoxelGrid() = default;
    
    // Non-copyable but movable
    VoxelGrid(const VoxelGrid&) = delete;
    VoxelGrid& operator=(const VoxelGrid&) = delete;
    VoxelGrid(VoxelGrid&&) = default;
    VoxelGrid& operator=(VoxelGrid&&) = default;
    
    // Voxel operations
    bool setVoxel(const Math::Vector3i& gridPos, bool value) {
        if (!isValidGridPosition(gridPos)) {
            return false;
        }
        
        bool currentValue = m_octree->getVoxel(gridPos);
        bool success = m_octree->setVoxel(gridPos, value);
        
        if (success && currentValue != value) {
            if (value) {
                m_voxelCount++;
            } else {
                m_voxelCount--;
            }
        }
        
        return success;
    }
    
    bool getVoxel(const Math::Vector3i& gridPos) const {
        if (!isValidGridPosition(gridPos)) {
            return false;
        }
        
        return m_octree->getVoxel(gridPos);
    }
    
    bool hasVoxel(const Math::Vector3i& gridPos) const {
        return getVoxel(gridPos);
    }
    
    // Convenience methods using VoxelPosition
    bool setVoxel(const ::VoxelEditor::VoxelData::VoxelPosition& pos, bool value) {
        if (pos.resolution != m_resolution) {
            return false;
        }
        return setVoxel(pos.gridPos, value);
    }
    
    bool getVoxel(const ::VoxelEditor::VoxelData::VoxelPosition& pos) const {
        if (pos.resolution != m_resolution) {
            return false;
        }
        return getVoxel(pos.gridPos);
    }
    
    bool hasVoxel(const ::VoxelEditor::VoxelData::VoxelPosition& pos) const {
        return getVoxel(pos);
    }
    
    // World space operations
    bool setVoxelAtWorldPos(const Math::Vector3f& worldPos, bool value) {
        ::VoxelEditor::VoxelData::VoxelPosition pos = ::VoxelEditor::VoxelData::VoxelPosition::fromWorldSpace(worldPos, m_resolution, m_workspaceSize);
        return setVoxel(pos, value);
    }
    
    bool getVoxelAtWorldPos(const Math::Vector3f& worldPos) const {
        ::VoxelEditor::VoxelData::VoxelPosition pos = ::VoxelEditor::VoxelData::VoxelPosition::fromWorldSpace(worldPos, m_resolution, m_workspaceSize);
        return getVoxel(pos);
    }
    
    bool hasVoxelAtWorldPos(const Math::Vector3f& worldPos) const {
        return getVoxelAtWorldPos(worldPos);
    }
    
    // Bulk operations
    void clear() {
        m_octree->clear();
        m_voxelCount = 0;
    }
    
    std::vector<::VoxelEditor::VoxelData::VoxelPosition> getAllVoxels() const {
        std::vector<Math::Vector3i> gridPositions = m_octree->getAllVoxels();
        std::vector<::VoxelEditor::VoxelData::VoxelPosition> voxelPositions;
        voxelPositions.reserve(gridPositions.size());
        
        for (const auto& gridPos : gridPositions) {
            voxelPositions.emplace_back(gridPos, m_resolution);
        }
        
        return voxelPositions;
    }
    
    // Grid information
    ::VoxelEditor::VoxelData::VoxelResolution getResolution() const { return m_resolution; }
    float getVoxelSize() const { return ::VoxelEditor::VoxelData::getVoxelSize(m_resolution); }
    const Math::Vector3f& getWorkspaceSize() const { return m_workspaceSize; }
    const Math::Vector3i& getMaxDimensions() const { return m_maxDimensions; }
    const Math::Vector3i& getMaxGridDimensions() const { return m_maxDimensions; } // Alias for compatibility
    size_t getVoxelCount() const { return m_voxelCount; }
    
    // Coordinate conversion methods
    Math::Vector3f gridToWorldPos(const Math::Vector3i& gridPos) const {
        ::VoxelEditor::VoxelData::VoxelPosition voxelPos(gridPos, m_resolution);
        return voxelPos.toWorldSpace(m_workspaceSize);
    }
    
    Math::Vector3i worldToGridPos(const Math::Vector3f& worldPos) const {
        ::VoxelEditor::VoxelData::VoxelPosition voxelPos = ::VoxelEditor::VoxelData::VoxelPosition::fromWorldSpace(worldPos, m_resolution, m_workspaceSize);
        return voxelPos.gridPos;
    }
    
    // Memory management
    size_t getMemoryUsage() const {
        return m_octree->getMemoryUsage() + sizeof(*this);
    }
    
    void optimizeMemory() {
        m_octree->optimize();
    }
    
    // Workspace resize support
    bool resizeWorkspace(const Math::Vector3f& newWorkspaceSize) {
        if (!::VoxelEditor::VoxelData::WorkspaceConstraints::isValidSize(newWorkspaceSize)) {
            return false;
        }
        
        // Calculate new maximum dimensions
        Math::Vector3i newMaxDimensions = ::VoxelEditor::VoxelData::calculateMaxGridDimensions(m_resolution, newWorkspaceSize);
        
        // If shrinking, check if any voxels would be out of bounds
        if (newMaxDimensions.x < m_maxDimensions.x ||
            newMaxDimensions.y < m_maxDimensions.y ||
            newMaxDimensions.z < m_maxDimensions.z) {
            
            // Get all current voxels and check if any are out of bounds
            auto voxels = getAllVoxels();
            for (const auto& voxel : voxels) {
                if (!isPositionInBounds(voxel.gridPos, m_resolution, newWorkspaceSize)) {
                    return false; // Cannot resize - would lose voxels
                }
            }
        }
        
        m_workspaceSize = newWorkspaceSize;
        m_maxDimensions = newMaxDimensions;
        return true;
    }
    
    // Validation
    bool isValidGridPosition(const Math::Vector3i& gridPos) const {
        return isPositionInBounds(gridPos, m_resolution, m_workspaceSize);
    }
    
    bool isValidWorldPosition(const Math::Vector3f& worldPos) const {
        // Check if world position is within workspace bounds
        Math::Vector3f halfWorkspace = m_workspaceSize * 0.5f;
        return worldPos.x >= -halfWorkspace.x && worldPos.x <= halfWorkspace.x &&
               worldPos.y >= -halfWorkspace.y && worldPos.y <= halfWorkspace.y &&
               worldPos.z >= -halfWorkspace.z && worldPos.z <= halfWorkspace.z;
    }
    
    // Statistics
    float getMemoryEfficiency() const {
        if (m_voxelCount == 0) return 1.0f;
        
        size_t octreeNodes = m_octree->getNodeCount();
        size_t theoreticalNodes = m_voxelCount; // Best case: one node per voxel
        
        return static_cast<float>(theoreticalNodes) / static_cast<float>(octreeNodes);
    }
    
    float getSpaceFillRatio() const {
        if (m_maxDimensions.x == 0 || m_maxDimensions.y == 0 || m_maxDimensions.z == 0) {
            return 0.0f;
        }
        
        size_t totalPossibleVoxels = static_cast<size_t>(m_maxDimensions.x) * 
                                   static_cast<size_t>(m_maxDimensions.y) * 
                                   static_cast<size_t>(m_maxDimensions.z);
        
        if (totalPossibleVoxels == 0) return 0.0f;
        
        return static_cast<float>(m_voxelCount) / static_cast<float>(totalPossibleVoxels);
    }
    
private:
    ::VoxelEditor::VoxelData::VoxelResolution m_resolution;
    Math::Vector3f m_workspaceSize;
    Math::Vector3i m_maxDimensions;
    std::unique_ptr<SparseOctree> m_octree;
    size_t m_voxelCount;
};

}
}