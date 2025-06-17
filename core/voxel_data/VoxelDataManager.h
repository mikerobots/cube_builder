#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <vector>
#include <cmath>

#include "VoxelTypes.h"
#include "VoxelGrid.h"
#include "WorkspaceManager.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/events/CommonEvents.h"
#include "../input/PlacementValidation.h"

namespace VoxelEditor {
namespace VoxelData {

class VoxelDataManager {
public:
    explicit VoxelDataManager(Events::EventDispatcher* eventDispatcher = nullptr)
        : m_activeResolution(VoxelResolution::Size_1cm)
        , m_eventDispatcher(eventDispatcher)
        , m_workspaceManager(std::make_unique<WorkspaceManager>(eventDispatcher)) {
        
        // Initialize octree memory pool
        SparseOctree::initializePool(1024);
        
        // Set up workspace change callback to validate grid compatibility
        m_workspaceManager->setSizeChangeCallback([this](const Math::Vector3f& oldSize, const Math::Vector3f& newSize) {
            return validateWorkspaceResize(oldSize, newSize);
        });
        
        // Initialize all grids with default workspace size
        initializeGrids();
    }
    
    ~VoxelDataManager() {
        SparseOctree::shutdownPool();
    }
    
    // Non-copyable and non-movable (due to mutex)
    VoxelDataManager(const VoxelDataManager&) = delete;
    VoxelDataManager& operator=(const VoxelDataManager&) = delete;
    VoxelDataManager(VoxelDataManager&&) = delete;
    VoxelDataManager& operator=(VoxelDataManager&&) = delete;
    
    // Voxel operations
    bool setVoxel(const Math::Vector3i& pos, VoxelResolution resolution, bool value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Validate 1cm increment position (includes Y >= 0 check)
        if (!isValidIncrementPosition(pos)) {
            return false;
        }
        
        VoxelGrid* grid = getGrid(resolution);
        if (!grid) return false;
        
        bool oldValue = grid->getVoxel(pos);
        
        // Handle redundant operations (setting same value as current)
        if (oldValue == value) {
            // This is a redundant operation - no change needed, but return true to indicate success
            return true;
        }
        
        // Check for overlaps if we're setting a voxel (not removing)
        if (value && wouldOverlapInternal(pos, resolution)) {
            return false;
        }
        
        bool success = grid->setVoxel(pos, value);
        
        if (success && oldValue != value) {
            dispatchVoxelChangedEvent(pos, resolution, oldValue, value);
        }
        
        return success;
    }
    
    bool setVoxel(const VoxelPosition& voxelPos, bool value) {
        return setVoxel(voxelPos.gridPos, voxelPos.resolution, value);
    }
    
    bool getVoxel(const Math::Vector3i& pos, VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        const VoxelGrid* grid = getGrid(resolution);
        if (!grid) return false;
        
        return grid->getVoxel(pos);
    }
    
    bool getVoxel(const VoxelPosition& voxelPos) const {
        return getVoxel(voxelPos.gridPos, voxelPos.resolution);
    }
    
    bool hasVoxel(const Math::Vector3i& pos, VoxelResolution resolution) const {
        return getVoxel(pos, resolution);
    }
    
    bool hasVoxel(const VoxelPosition& voxelPos) const {
        return getVoxel(voxelPos);
    }
    
    // World space operations
    bool setVoxelAtWorldPos(const Math::Vector3f& worldPos, VoxelResolution resolution, bool value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Removed verbose debug logging for performance
        
        // Validate 1cm increment position for world coordinates
        if (!isValidIncrementPosition(worldPos)) {
            // Position validation failed
            return false;
        }
        
        VoxelGrid* grid = getGrid(resolution);
        if (!grid) {
            // Failed to get grid
            return false;
        }
        
        // Convert world position to grid position using the grid's coordinate system
        Math::Vector3i gridPos = grid->worldToGrid(worldPos);
        // Grid position converted
        
        // Check for overlaps if we're setting a voxel (not removing)
        if (value) {
            // Check for overlaps
            if (wouldOverlapInternal(gridPos, resolution)) {
                // Overlap detected
                return false;
            }
            // No overlaps detected
        }
        
        // Get the old value before setting
        bool oldValue = grid->getVoxel(gridPos);
        
        // Set the voxel in the grid
        bool result = grid->setVoxel(gridPos, value);
        
        // Dispatch event if successful and value changed
        if (result && oldValue != value && m_eventDispatcher) {
            Events::VoxelChangedEvent event{
                gridPos,
                resolution,
                oldValue,
                value
            };
            m_eventDispatcher->dispatch(event);
            Logging::Logger::getInstance().debugfc("VoxelDataManager", 
                "Dispatched VoxelChangedEvent");
        }
        
        return result;
    }
    
    bool setVoxelAtWorldPos(const Math::Vector3f& worldPos, bool value) {
        return setVoxelAtWorldPos(worldPos, m_activeResolution, value);
    }
    
    bool getVoxelAtWorldPos(const Math::Vector3f& worldPos, VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        const VoxelGrid* grid = getGrid(resolution);
        if (!grid) return false;
        
        return grid->getVoxelAtWorldPos(worldPos);
    }
    
    bool getVoxelAtWorldPos(const Math::Vector3f& worldPos) const {
        return getVoxelAtWorldPos(worldPos, m_activeResolution);
    }
    
    bool hasVoxelAtWorldPos(const Math::Vector3f& worldPos, VoxelResolution resolution) const {
        return getVoxelAtWorldPos(worldPos, resolution);
    }
    
    bool hasVoxelAtWorldPos(const Math::Vector3f& worldPos) const {
        return getVoxelAtWorldPos(worldPos, m_activeResolution);
    }
    
    // Resolution management
    void setActiveResolution(VoxelResolution resolution) {
        if (!isValidResolution(static_cast<int>(resolution))) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        VoxelResolution oldResolution = m_activeResolution;
        m_activeResolution = resolution;
        
        if (m_eventDispatcher && oldResolution != resolution) {
            Events::ResolutionChangedEvent event(oldResolution, resolution);
            m_eventDispatcher->dispatch(event);
        }
    }
    
    VoxelResolution getActiveResolution() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_activeResolution;
    }
    
    float getActiveVoxelSize() const {
        return getVoxelSize(getActiveResolution());
    }
    
    // Workspace management
    bool resizeWorkspace(const Math::Vector3f& newSize) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_workspaceManager->setSize(newSize);
    }
    
    bool resizeWorkspace(float size) {
        return resizeWorkspace(Math::Vector3f(size, size, size));
    }
    
    Math::Vector3f getWorkspaceSize() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_workspaceManager->getSize();
    }
    
    WorkspaceManager* getWorkspaceManager() const {
        return m_workspaceManager.get();
    }
    
    // Position validation
    bool isValidPosition(const Math::Vector3i& pos, VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        const VoxelGrid* grid = getGrid(resolution);
        if (!grid) return false;
        
        return grid->isValidGridPosition(pos);
    }
    
    bool isValidWorldPosition(const Math::Vector3f& worldPos) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_workspaceManager->isPositionValid(worldPos);
    }
    
    // Bulk operations
    void clearAll() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        for (auto& grid : m_grids) {
            if (grid) {
                grid->clear();
            }
        }
    }
    
    void clearResolution(VoxelResolution resolution) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        VoxelGrid* grid = getGrid(resolution);
        if (grid) {
            grid->clear();
        }
    }
    
    void clearActiveResolution() {
        clearResolution(getActiveResolution());
    }
    
    // Statistics
    size_t getVoxelCount(VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        const VoxelGrid* grid = getGrid(resolution);
        if (!grid) return 0;
        
        return grid->getVoxelCount();
    }
    
    size_t getVoxelCount() const {
        return getVoxelCount(getActiveResolution());
    }
    
    size_t getTotalVoxelCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        size_t total = 0;
        for (const auto& grid : m_grids) {
            if (grid) {
                total += grid->getVoxelCount();
            }
        }
        return total;
    }
    
    // Memory management
    size_t getMemoryUsage() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        size_t total = sizeof(*this);
        
        for (const auto& grid : m_grids) {
            if (grid) {
                total += grid->getMemoryUsage();
            }
        }
        
        return total;
    }
    
    size_t getMemoryUsage(VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        const VoxelGrid* grid = getGrid(resolution);
        if (!grid) return 0;
        
        return grid->getMemoryUsage();
    }
    
    void optimizeMemory() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        for (auto& grid : m_grids) {
            if (grid) {
                grid->optimizeMemory();
            }
        }
    }
    
    void optimizeMemory(VoxelResolution resolution) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        VoxelGrid* grid = getGrid(resolution);
        if (grid) {
            grid->optimizeMemory();
        }
    }
    
    // Grid access
    const VoxelGrid* getGrid(VoxelResolution resolution) const {
        int index = static_cast<int>(resolution);
        if (!isValidResolution(index)) return nullptr;
        return m_grids[index].get();
    }
    
    VoxelGrid* getGrid(VoxelResolution resolution) {
        int index = static_cast<int>(resolution);
        if (!isValidResolution(index)) return nullptr;
        return m_grids[index].get();
    }
    
    // Data export
    std::vector<VoxelPosition> getAllVoxels(VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        const VoxelGrid* grid = getGrid(resolution);
        if (!grid) return {};
        
        return grid->getAllVoxels();
    }
    
    std::vector<VoxelPosition> getAllVoxels() const {
        return getAllVoxels(getActiveResolution());
    }
    
    // Enhancement: 1cm increment validation
    bool isValidIncrementPosition(const Math::Vector3i& pos) const {
        // All integer positions are valid 1cm increments since our base unit is 1cm
        // The constraint is simply that positions must be integers
        // Additionally, Y must be >= 0 (no voxels below ground plane)
        return pos.y >= 0;
    }
    
    bool isValidIncrementPosition(const Math::Vector3f& worldPos) const {
        // Check if world position aligns to 1cm grid
        // Since 1cm = 0.01m, positions should be multiples of 0.01
        const float epsilon = 0.0001f; // Small tolerance for float precision
        
        Logging::Logger::getInstance().debugfc("VoxelDataManager", 
            "isValidIncrementPosition: checking worldPos=(%.6f, %.6f, %.6f)",
            worldPos.x, worldPos.y, worldPos.z);
        
        // Check Y >= 0 constraint
        if (worldPos.y < -epsilon) {
            Logging::Logger::getInstance().debugfc("VoxelDataManager", 
                "Position failed Y >= 0 check");
            return false;
        }
        
        // Check if each component is close to a 1cm increment
        float xRemainder = std::fmod(std::abs(worldPos.x), 0.01f);
        float yRemainder = std::fmod(std::abs(worldPos.y), 0.01f);
        float zRemainder = std::fmod(std::abs(worldPos.z), 0.01f);
        
        Logging::Logger::getInstance().debugfc("VoxelDataManager", 
            "Remainders: x=%.6f, y=%.6f, z=%.6f",
            xRemainder, yRemainder, zRemainder);
        
        bool result = (xRemainder < epsilon || xRemainder > (0.01f - epsilon)) &&
                     (yRemainder < epsilon || yRemainder > (0.01f - epsilon)) &&
                     (zRemainder < epsilon || zRemainder > (0.01f - epsilon));
        
        Logging::Logger::getInstance().debugfc("VoxelDataManager", 
            "isValidIncrementPosition result: %s", result ? "true" : "false");
        
        return result;
    }
    
    // Enhancement: Collision detection
    bool wouldOverlap(const Math::Vector3i& pos, VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return wouldOverlapInternal(pos, resolution);
    }
    
    bool wouldOverlap(const VoxelPosition& voxelPos) const {
        return wouldOverlap(voxelPos.gridPos, voxelPos.resolution);
    }
    
    // Enhancement: Adjacent position calculation
    Math::Vector3i getAdjacentPosition(const Math::Vector3i& pos, FaceDirection face, 
                                     VoxelResolution sourceRes, VoxelResolution targetRes) const {
        // Calculate the position for placing a voxel of targetRes adjacent to a voxel at pos with sourceRes
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // For same-size voxels, just offset by 1 in the appropriate direction
        if (sourceRes == targetRes) {
            Math::Vector3i offset(0, 0, 0);
            switch (face) {
                case FaceDirection::PosX: offset.x = 1; break;
                case FaceDirection::NegX: offset.x = -1; break;
                case FaceDirection::PosY: offset.y = 1; break;
                case FaceDirection::NegY: offset.y = -1; break;
                case FaceDirection::PosZ: offset.z = 1; break;
                case FaceDirection::NegZ: offset.z = -1; break;
            }
            return pos + offset;
        }
        
        // For different sizes, convert to world space using grid coordinate system
        const VoxelGrid* sourceGrid = getGrid(sourceRes);
        const VoxelGrid* targetGrid = getGrid(targetRes);
        if (!sourceGrid || !targetGrid) {
            return pos; // Fallback
        }
        
        float sourceSize = getVoxelSize(sourceRes);
        float targetSize = getVoxelSize(targetRes);
        
        // Convert source position to world space
        Math::Vector3f sourceWorldPos = sourceGrid->gridToWorld(pos);
        
        // Calculate offset based on face direction
        Math::Vector3f offset(0, 0, 0);
        switch (face) {
            case FaceDirection::PosX:
                offset.x = sourceSize;
                break;
            case FaceDirection::NegX:
                offset.x = -targetSize;
                break;
            case FaceDirection::PosY:
                offset.y = sourceSize;
                break;
            case FaceDirection::NegY:
                offset.y = -targetSize;
                break;
            case FaceDirection::PosZ:
                offset.z = sourceSize;
                break;
            case FaceDirection::NegZ:
                offset.z = -targetSize;
                break;
        }
        
        // Calculate target world position
        Math::Vector3f targetWorldPos = sourceWorldPos + offset;
        
        // Convert to grid coordinates for target resolution
        Math::Vector3i targetGridPos = targetGrid->worldToGrid(targetWorldPos);
        
        return targetGridPos;
    }
    
    // Event dispatcher
    void setEventDispatcher(Events::EventDispatcher* eventDispatcher) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_eventDispatcher = eventDispatcher;
        m_workspaceManager->setEventDispatcher(eventDispatcher);
    }
    
    // Performance metrics
    struct PerformanceMetrics {
        size_t totalVoxels;
        size_t totalMemoryUsage;
        float memoryEfficiency;
        std::array<size_t, static_cast<int>(VoxelResolution::COUNT)> voxelsByResolution;
        std::array<size_t, static_cast<int>(VoxelResolution::COUNT)> memoryByResolution;
    };
    
    PerformanceMetrics getPerformanceMetrics() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        PerformanceMetrics metrics = {};
        
        for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
            if (m_grids[i]) {
                metrics.voxelsByResolution[i] = m_grids[i]->getVoxelCount();
                metrics.memoryByResolution[i] = m_grids[i]->getMemoryUsage();
                metrics.totalVoxels += metrics.voxelsByResolution[i];
                metrics.totalMemoryUsage += metrics.memoryByResolution[i];
            }
        }
        
        // Calculate overall memory efficiency
        if (metrics.totalMemoryUsage > 0) {
            size_t theoreticalMinMemory = metrics.totalVoxels * sizeof(bool);
            metrics.memoryEfficiency = static_cast<float>(theoreticalMinMemory) / 
                                     static_cast<float>(metrics.totalMemoryUsage);
        } else {
            metrics.memoryEfficiency = 1.0f;
        }
        
        return metrics;
    }
    
private:
    std::array<std::unique_ptr<VoxelGrid>, static_cast<int>(VoxelResolution::COUNT)> m_grids;
    VoxelResolution m_activeResolution;
    std::unique_ptr<WorkspaceManager> m_workspaceManager;
    Events::EventDispatcher* m_eventDispatcher;
    mutable std::mutex m_mutex;
    
    void initializeGrids() {
        Math::Vector3f workspaceSize = m_workspaceManager->getSize();
        
        for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
            VoxelResolution resolution = static_cast<VoxelResolution>(i);
            m_grids[i] = std::make_unique<VoxelGrid>(resolution, workspaceSize);
        }
    }
    
    bool validateWorkspaceResize(const Math::Vector3f& oldSize, const Math::Vector3f& newSize) {
        // Check if any grids would lose voxels during resize
        for (const auto& grid : m_grids) {
            if (grid && !grid->resizeWorkspace(newSize)) {
                return false; // Cannot resize - would lose voxels
            }
        }
        return true;
    }
    
    void dispatchVoxelChangedEvent(const Math::Vector3i& position, VoxelResolution resolution, 
                                 bool oldValue, bool newValue) {
        if (m_eventDispatcher) {
            Events::VoxelChangedEvent event(position, resolution, oldValue, newValue);
            m_eventDispatcher->dispatch(event);
        }
    }
    
    // Internal collision detection without lock (must be called with lock already held)
    bool wouldOverlapInternal(const Math::Vector3i& pos, VoxelResolution resolution) const {
        // Check if placing a voxel at this position would overlap with existing voxels
        // Optimized version that uses spatial queries to reduce checks
        
        Logging::Logger::getInstance().debugfc("VoxelDataManager", 
            "wouldOverlapInternal: checking position (%d, %d, %d) with resolution %d",
            pos.x, pos.y, pos.z, static_cast<int>(resolution));
        
        // Get the grid for this resolution to convert to world space correctly
        const VoxelGrid* targetGrid = getGrid(resolution);
        if (!targetGrid) return false;
        
        // Convert grid position to world space using the grid's coordinate system
        Math::Vector3f worldMin = targetGrid->gridToWorld(pos);
        float voxelSize = getVoxelSize(resolution);
        Math::Vector3f worldMax = worldMin + Math::Vector3f(voxelSize, voxelSize, voxelSize);
        
        Logging::Logger::getInstance().debugfc("VoxelDataManager", 
            "World bounds: min=(%.3f, %.3f, %.3f) max=(%.3f, %.3f, %.3f)",
            worldMin.x, worldMin.y, worldMin.z, worldMax.x, worldMax.y, worldMax.z);
        
        // Check each resolution level for overlaps
        for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
            VoxelResolution checkRes = static_cast<VoxelResolution>(i);
            const VoxelGrid* grid = getGrid(checkRes);
            if (!grid || grid->getVoxelCount() == 0) continue;
            
            Logging::Logger::getInstance().debugfc("VoxelDataManager", 
                "Checking against resolution %d with %zu voxels",
                i, grid->getVoxelCount());
            
            float checkVoxelSize = getVoxelSize(checkRes);
            
            // Convert world bounds to grid coordinates for this resolution
            // to limit our search space
            Math::Vector3i minGridCheck = grid->worldToGrid(worldMin);
            Math::Vector3i maxGridCheck = grid->worldToGrid(worldMax);
            
            // Adjust max bounds to be inclusive (ceil behavior)
            maxGridCheck.x = std::min(maxGridCheck.x + 1, grid->getGridDimensions().x);
            maxGridCheck.y = std::min(maxGridCheck.y + 1, grid->getGridDimensions().y);
            maxGridCheck.z = std::min(maxGridCheck.z + 1, grid->getGridDimensions().z);
            
            Logging::Logger::getInstance().debugfc("VoxelDataManager", 
                "Grid check bounds: min=(%d, %d, %d) max=(%d, %d, %d)",
                minGridCheck.x, minGridCheck.y, minGridCheck.z,
                maxGridCheck.x, maxGridCheck.y, maxGridCheck.z);
            
            // Clamp bounds to valid grid range
            minGridCheck.x = std::max(0, minGridCheck.x);
            minGridCheck.y = std::max(0, minGridCheck.y);
            minGridCheck.z = std::max(0, minGridCheck.z);
            
            // Quick check: if the search range is too large, use getAllVoxels instead
            int searchVolume = (maxGridCheck.x - minGridCheck.x) * 
                              (maxGridCheck.y - minGridCheck.y) * 
                              (maxGridCheck.z - minGridCheck.z);
            
            if (searchVolume > 1000 || grid->getVoxelCount() < searchVolume / 2) {
                // More efficient to check all voxels directly
                auto voxels = grid->getAllVoxels();
                for (const auto& voxelPos : voxels) {
                    Math::Vector3f voxelMin = grid->gridToWorld(voxelPos.gridPos);
                    Math::Vector3f voxelMax = voxelMin + Math::Vector3f(checkVoxelSize, checkVoxelSize, checkVoxelSize);
                    
                    // Check for overlap (AABB intersection)
                    if (worldMin.x < voxelMax.x && worldMax.x > voxelMin.x &&
                        worldMin.y < voxelMax.y && worldMax.y > voxelMin.y &&
                        worldMin.z < voxelMax.z && worldMax.z > voxelMin.z) {
                        return true; // Would overlap
                    }
                }
                continue; // Next resolution
            }
            
            // Check only the voxels that could potentially overlap
            for (int x = minGridCheck.x; x < maxGridCheck.x; ++x) {
                for (int y = minGridCheck.y; y < maxGridCheck.y; ++y) {
                    for (int z = minGridCheck.z; z < maxGridCheck.z; ++z) {
                        if (grid->getVoxel(Math::Vector3i(x, y, z))) {
                            // Found a voxel that might overlap
                            Math::Vector3f voxelMin = grid->gridToWorld(Math::Vector3i(x, y, z));
                            Math::Vector3f voxelMax = voxelMin + Math::Vector3f(checkVoxelSize, checkVoxelSize, checkVoxelSize);
                            
                            // Check for overlap (AABB intersection)
                            if (worldMin.x < voxelMax.x && worldMax.x > voxelMin.x &&
                                worldMin.y < voxelMax.y && worldMax.y > voxelMin.y &&
                                worldMin.z < voxelMax.z && worldMax.z > voxelMin.z) {
                                return true; // Would overlap
                            }
                        }
                    }
                }
            }
        }
        
        return false; // No overlap
    }
};

}
}