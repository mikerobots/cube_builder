#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <vector>

#include "VoxelTypes.h"
#include "VoxelGrid.h"
#include "WorkspaceManager.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/events/CommonEvents.h"

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
        
        VoxelGrid* grid = getGrid(resolution);
        if (!grid) return false;
        
        bool oldValue = grid->getVoxel(pos);
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
        
        VoxelGrid* grid = getGrid(resolution);
        if (!grid) return false;
        
        return grid->setVoxelAtWorldPos(worldPos, value);
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
};

}
}