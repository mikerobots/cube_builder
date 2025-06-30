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
        
        // CRITICAL FIX: Calculate grid dimensions based on 1cm granularity, not voxel resolution
        // All voxels are now stored at 1cm increments regardless of their resolution
        // This ensures the octree can handle positions at any 1cm increment
        const float CM_SIZE = 0.01f;  // 1cm in meters
        m_gridDimensions = Math::Vector3i(
            static_cast<int>(std::ceil(workspaceSize.x / CM_SIZE)),
            static_cast<int>(std::ceil(workspaceSize.y / CM_SIZE)),
            static_cast<int>(std::ceil(workspaceSize.z / CM_SIZE))
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
            // Commented out to prevent excessive debug output during tests
            // Logging::Logger::getInstance().debugfc("VoxelGrid", 
            //     "Attempt to set voxel at invalid position (%d, %d, %d)", pos.x(), pos.y(), pos.z());
            return false;
        }
        
        // Convert increment coordinates to grid coordinates for octree storage
        Math::Vector3i gridPos = incrementToGrid(pos);
        bool success = m_octree->setVoxel(gridPos, value);
        
        
        // Commented out to prevent excessive debug output during tests
        // if (success) {
        //     Logging::Logger::getInstance().debugfc("VoxelGrid", 
        //         "Set voxel at position (%d, %d, %d) to %s", pos.x(), pos.y(), pos.z(), value ? "true" : "false");
        // } else {
        //     Logging::Logger::getInstance().debugfc("VoxelGrid", 
        //         "Failed to set voxel at position (%d, %d, %d) - octree operation failed", pos.x(), pos.y(), pos.z());
        // }
        
        return success;
    }
    
    bool getVoxel(const Math::IncrementCoordinates& pos) const {
        if (!isValidIncrementPosition(pos)) {
            // Commented out to prevent excessive debug output during tests
            // Logging::Logger::getInstance().debugfc("VoxelGrid", 
            //     "Attempt to get voxel at invalid position (%d, %d, %d)", pos.x(), pos.y(), pos.z());
            return false;
        }
        
        // Convert increment coordinates to grid coordinates for octree lookup
        Math::Vector3i gridPos = incrementToGrid(pos);
        bool result = m_octree->getVoxel(gridPos);
        // Commented out to prevent excessive debug output during tests
        // Logging::Logger::getInstance().debugfc("VoxelGrid", 
        //     "Get voxel at position (%d, %d, %d): %s", pos.x(), pos.y(), pos.z(), result ? "true" : "false");
        return result;
    }
    
    // World space operations
    bool setVoxelAtWorldPos(const Math::WorldCoordinates& worldPos, bool value) {
        Math::IncrementCoordinates incrementPos = worldToIncrement(worldPos);
        // Commented out to prevent excessive debug output during tests
        // Logging::Logger::getInstance().debugfc("VoxelGrid", 
        //     "World position (%.3f, %.3f, %.3f) maps to increment position (%d, %d, %d)", 
        //     worldPos.x(), worldPos.y(), worldPos.z(), incrementPos.x(), incrementPos.y(), incrementPos.z());
        return setVoxel(incrementPos, value);
    }
    
    bool getVoxelAtWorldPos(const Math::WorldCoordinates& worldPos) const {
        Math::IncrementCoordinates incrementPos = worldToIncrement(worldPos);
        // Commented out to prevent excessive debug output during tests
        // Logging::Logger::getInstance().debugfc("VoxelGrid", 
        //     "Query world position (%.3f, %.3f, %.3f) -> increment position (%d, %d, %d)", 
        //     worldPos.x(), worldPos.y(), worldPos.z(), incrementPos.x(), incrementPos.y(), incrementPos.z());
        return getVoxel(incrementPos);
    }
    
    // Position validation
    bool isValidIncrementPosition(const Math::IncrementCoordinates& pos) const {
        // Check if the voxel position + voxel size fits within workspace bounds
        // Get the voxel size in centimeters
        float voxelSizeMeters = VoxelData::getVoxelSize(m_resolution);
        int voxelSizeCm = static_cast<int>(voxelSizeMeters * 100.0f);
        
        // Convert workspace bounds to increment coordinates  
        int halfX_cm = static_cast<int>(m_workspaceSize.x * 100.0f * 0.5f);
        int halfZ_cm = static_cast<int>(m_workspaceSize.z * 100.0f * 0.5f);
        int height_cm = static_cast<int>(m_workspaceSize.y * 100.0f);
        
        const Math::Vector3i& increment = pos.value();
        
        // Check Y >= 0 constraint first (before general bounds check)
        if (increment.y < 0) {
            return false;
        }
        
        // Check if voxel position is within bounds (centered coordinate system)
        if (increment.x < -halfX_cm || increment.x > halfX_cm ||
            increment.z < -halfZ_cm || increment.z > halfZ_cm) {
            return false;
        }
        
        // Check if the top of the voxel would exceed workspace height
        // Voxel bottom is at increment.y, top is at increment.y + voxelSizeCm
        if (increment.y + voxelSizeCm > height_cm) {
            return false;
        }
        
        return true;
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
        // CRITICAL FIX: Store all voxels at 1cm granularity regardless of resolution
        // This ensures that voxels at different increment positions don't collide
        // The resolution is used only for rendering and collision detection, not storage granularity
        
        // Use 1cm granularity (no division) - each increment position maps to unique grid position
        Math::Vector3i gridPos(
            incrementPos.x(),  // 1cm = 1 grid unit
            incrementPos.y(),  // 1cm = 1 grid unit
            incrementPos.z()   // 1cm = 1 grid unit
        );
        
        // Adjust for centered coordinate system - convert to positive grid indices
        // Now working in 1cm units, so workspace size in cm = workspace size in meters * 100
        int halfX_cm = static_cast<int>(m_workspaceSize.x * 100.0f / 2.0f);
        int halfZ_cm = static_cast<int>(m_workspaceSize.z * 100.0f / 2.0f);
        
        return Math::Vector3i(
            gridPos.x + halfX_cm,
            gridPos.y,
            gridPos.z + halfZ_cm
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
        
        // Commented out to prevent excessive debug output during tests
        // Logging::Logger::getInstance().debugfc("VoxelGrid", 
        //     "Retrieved %zu voxel positions from octree", positions.size());
        
        for (const auto& pos : positions) {
            // Convert grid position back to increment coordinates
            Math::Vector3i gridPos = pos;
            // Apply reverse transformation from incrementToGrid()
            // Since we now store at 1cm granularity, reverse is simpler
            int halfX_cm = static_cast<int>(m_workspaceSize.x * 100.0f / 2.0f);
            int halfZ_cm = static_cast<int>(m_workspaceSize.z * 100.0f / 2.0f);
            
            Math::IncrementCoordinates incrementPos(
                gridPos.x - halfX_cm,  // Direct mapping (1cm = 1 grid unit)
                gridPos.y,             // Direct mapping (1cm = 1 grid unit)
                gridPos.z - halfZ_cm   // Direct mapping (1cm = 1 grid unit)
            );
            
            VoxelPosition voxelPos(incrementPos, m_resolution);
            voxels.push_back(voxelPos);
        }
        
        return voxels;
    }
    
    // Resize workspace
    bool resizeWorkspace(const Math::Vector3f& newSize) {
        // Calculate new grid dimensions based on 1cm granularity, not voxel resolution
        const float CM_SIZE = 0.01f;  // 1cm in meters
        Math::Vector3i newDimensions(
            static_cast<int>(std::ceil(newSize.x / CM_SIZE)),
            static_cast<int>(std::ceil(newSize.y / CM_SIZE)),
            static_cast<int>(std::ceil(newSize.z / CM_SIZE))
        );
        
        // Check if any voxels would be lost
        // Need to check if voxels would be outside the new centered workspace bounds
        bool wouldLoseVoxels = false;
        auto positions = m_octree->getAllVoxels();
        
        // Calculate new bounds in grid coordinates (accounting for centered coordinate system)
        int newHalfX_cm = static_cast<int>(newSize.x * 100.0f / 2.0f);
        int newHalfZ_cm = static_cast<int>(newSize.z * 100.0f / 2.0f);
        int newMaxY_cm = static_cast<int>(newSize.y * 100.0f);
        
        for (const auto& gridPos : positions) {
            // Convert grid position back to increment coordinates
            int halfX_cm = static_cast<int>(m_workspaceSize.x * 100.0f / 2.0f);
            int halfZ_cm = static_cast<int>(m_workspaceSize.z * 100.0f / 2.0f);
            
            Math::IncrementCoordinates incrementPos(
                gridPos.x - halfX_cm,
                gridPos.y,
                gridPos.z - halfZ_cm
            );
            
            // Check if this position would be within new bounds
            if (std::abs(incrementPos.x()) > newHalfX_cm ||
                incrementPos.y() >= newMaxY_cm ||
                std::abs(incrementPos.z()) > newHalfZ_cm) {
                wouldLoseVoxels = true;
                break;
            }
        }
        
        if (wouldLoseVoxels) {
            return false;
        }
        
        // Recalculate octree depth for new dimensions and recreate octree
        int maxDim = std::max({newDimensions.x, newDimensions.y, newDimensions.z});
        int depth = 0;
        int size = 1;
        while (size < maxDim) {
            size *= 2;
            depth++;
        }
        
        if (size == maxDim && maxDim > 1) {
            depth++;
        }
        
        // Create new octree with correct depth
        auto newOctree = std::make_unique<SparseOctree>(depth);
        
        // Transfer existing voxels to new octree
        // Need to convert from old grid coordinates to increment, then to new grid coordinates
        for (const auto& oldGridPos : positions) {
            // Convert old grid position back to increment coordinates
            int oldHalfX_cm = static_cast<int>(m_workspaceSize.x * 100.0f / 2.0f);
            int oldHalfZ_cm = static_cast<int>(m_workspaceSize.z * 100.0f / 2.0f);
            
            Math::IncrementCoordinates incrementPos(
                oldGridPos.x - oldHalfX_cm,
                oldGridPos.y,
                oldGridPos.z - oldHalfZ_cm
            );
            
            // Convert increment coordinates to new grid coordinates
            int newHalfX_cm = static_cast<int>(newSize.x * 100.0f / 2.0f);
            int newHalfZ_cm = static_cast<int>(newSize.z * 100.0f / 2.0f);
            
            Math::Vector3i newGridPos(
                incrementPos.x() + newHalfX_cm,
                incrementPos.y(),
                incrementPos.z() + newHalfZ_cm
            );
            
            newOctree->setVoxel(newGridPos, true);
        }
        
        // Replace the old octree
        m_octree = std::move(newOctree);
        
        // Update workspace size and grid dimensions after successful transfer
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