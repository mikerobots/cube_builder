#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include "VoxelTypes.h"
#include "SparseOctree.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/BoundingBox.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
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
    bool setVoxel(const Math::IncrementCoordinates& pos, bool value) {
        if (!isValidIncrementPosition(pos)) {
            Logging::Logger::getInstance().debugfc("VoxelGrid", 
                "Attempt to set voxel at invalid position (%d, %d, %d)", pos.x(), pos.y(), pos.z());
            return false;
        }
        
        // Convert increment coordinates to grid coordinates for octree storage
        Math::Vector3i gridPos = incrementToGrid(pos);
        m_octree->setVoxel(gridPos, value);
        Logging::Logger::getInstance().debugfc("VoxelGrid", 
            "Set voxel at position (%d, %d, %d) to %s", pos.x(), pos.y(), pos.z(), value ? "true" : "false");
        return true;
    }
    
    bool getVoxel(const Math::IncrementCoordinates& pos) const {
        if (!isValidIncrementPosition(pos)) {
            Logging::Logger::getInstance().debugfc("VoxelGrid", 
                "Attempt to get voxel at invalid position (%d, %d, %d)", pos.x(), pos.y(), pos.z());
            return false;
        }
        
        // Convert increment coordinates to grid coordinates for octree lookup
        Math::Vector3i gridPos = incrementToGrid(pos);
        bool result = m_octree->getVoxel(gridPos);
        Logging::Logger::getInstance().debugfc("VoxelGrid", 
            "Get voxel at position (%d, %d, %d): %s", pos.x(), pos.y(), pos.z(), result ? "true" : "false");
        return result;
    }
    
    // World space operations
    bool setVoxelAtWorldPos(const Math::WorldCoordinates& worldPos, bool value) {
        Math::IncrementCoordinates incrementPos = worldToIncrement(worldPos);
        Logging::Logger::getInstance().debugfc("VoxelGrid", 
            "World position (%.3f, %.3f, %.3f) maps to increment position (%d, %d, %d)", 
            worldPos.x(), worldPos.y(), worldPos.z(), incrementPos.x(), incrementPos.y(), incrementPos.z());
        return setVoxel(incrementPos, value);
    }
    
    bool getVoxelAtWorldPos(const Math::WorldCoordinates& worldPos) const {
        Math::IncrementCoordinates incrementPos = worldToIncrement(worldPos);
        Logging::Logger::getInstance().debugfc("VoxelGrid", 
            "Query world position (%.3f, %.3f, %.3f) -> increment position (%d, %d, %d)", 
            worldPos.x(), worldPos.y(), worldPos.z(), incrementPos.x(), incrementPos.y(), incrementPos.z());
        return getVoxel(incrementPos);
    }
    
    // Position validation
    bool isValidIncrementPosition(const Math::IncrementCoordinates& pos) const {
        return Math::CoordinateConverter::isValidIncrementCoordinate(pos, m_workspaceSize);
    }
    
    bool isValidWorldPosition(const Math::WorldCoordinates& worldPos) const {
        // Use centered coordinate system to match WorkspaceManager
        float halfX = m_workspaceSize.x * 0.5f;
        float halfZ = m_workspaceSize.z * 0.5f;
        return worldPos.x() >= -halfX && worldPos.x() <= halfX &&
               worldPos.y() >= 0 && worldPos.y() <= m_workspaceSize.y &&
               worldPos.z() >= -halfZ && worldPos.z() <= halfZ;
    }
    
    // Coordinate conversion
    Math::IncrementCoordinates worldToIncrement(const Math::WorldCoordinates& worldPos) const {
        return Math::CoordinateConverter::worldToIncrement(worldPos);
    }
    
    Math::WorldCoordinates incrementToWorld(const Math::IncrementCoordinates& incrementPos) const {
        return Math::CoordinateConverter::incrementToWorld(incrementPos);
    }
    
    // Internal helper: Convert increment coordinates to grid coordinates for octree storage
    Math::Vector3i incrementToGrid(const Math::IncrementCoordinates& incrementPos) const {
        // For this grid's resolution, convert increment position to grid coordinates
        // by snapping to the voxel resolution and then dividing by voxel size
        Math::IncrementCoordinates snapped = Math::CoordinateConverter::snapToVoxelResolution(incrementPos, m_resolution);
        float voxelSizeMeters = Math::CoordinateConverter::getVoxelSizeMeters(m_resolution);
        int voxelSize_cm = static_cast<int>(voxelSizeMeters * 100.0f);
        
        // Convert to grid coordinates relative to workspace center
        Math::Vector3i gridPos(
            snapped.x() / voxelSize_cm,
            snapped.y() / voxelSize_cm,
            snapped.z() / voxelSize_cm
        );
        
        // Adjust for centered coordinate system - convert to positive grid indices
        int halfX_voxels = static_cast<int>(m_workspaceSize.x / voxelSizeMeters / 2.0f);
        int halfZ_voxels = static_cast<int>(m_workspaceSize.z / voxelSizeMeters / 2.0f);
        
        return Math::Vector3i(
            gridPos.x + halfX_voxels,
            gridPos.y,
            gridPos.z + halfZ_voxels
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
            // Convert grid position back to increment coordinates
            Math::Vector3i gridPos = pos;
            // Apply reverse transformation from incrementToGrid()
            float voxelSizeMeters = Math::CoordinateConverter::getVoxelSizeMeters(m_resolution);
            int voxelSize_cm = static_cast<int>(voxelSizeMeters * 100.0f);
            int halfX_voxels = static_cast<int>(m_workspaceSize.x / voxelSizeMeters / 2.0f);
            int halfZ_voxels = static_cast<int>(m_workspaceSize.z / voxelSizeMeters / 2.0f);
            
            Math::IncrementCoordinates incrementPos(
                (gridPos.x - halfX_voxels) * voxelSize_cm,
                gridPos.y * voxelSize_cm,
                (gridPos.z - halfZ_voxels) * voxelSize_cm
            );
            
            VoxelPosition voxelPos(incrementPos, m_resolution);
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
    
    // Backward compatibility wrapper methods (to be removed in Phase 7)
    bool setVoxel(const Math::Vector3i& pos, bool value) {
        return setVoxel(Math::IncrementCoordinates(pos), value);
    }
    
    bool getVoxel(const Math::Vector3i& pos) const {
        return getVoxel(Math::IncrementCoordinates(pos));
    }
    
    bool setVoxelAtWorldPos(const Math::Vector3f& worldPos, bool value) {
        return setVoxelAtWorldPos(Math::WorldCoordinates(worldPos), value);
    }
    
    bool getVoxelAtWorldPos(const Math::Vector3f& worldPos) const {
        return getVoxelAtWorldPos(Math::WorldCoordinates(worldPos));
    }
    
    bool isValidIncrementPosition(const Math::Vector3i& pos) const {
        return isValidIncrementPosition(Math::IncrementCoordinates(pos));
    }
    
    bool isValidWorldPosition(const Math::Vector3f& worldPos) const {
        return isValidWorldPosition(Math::WorldCoordinates(worldPos));
    }
    
    Math::Vector3i worldToIncrement(const Math::Vector3f& worldPos) const {
        return worldToIncrement(Math::WorldCoordinates(worldPos)).value();
    }
    
    Math::Vector3f incrementToWorld(const Math::Vector3i& incrementPos) const {
        return incrementToWorld(Math::IncrementCoordinates(incrementPos)).value();
    }
    
private:
    VoxelResolution m_resolution;
    Math::Vector3f m_workspaceSize;
    Math::Vector3i m_gridDimensions;
    float m_voxelSize;
    std::unique_ptr<SparseOctree> m_octree;
};

} // namespace VoxelData
} // namespace VoxelEditor