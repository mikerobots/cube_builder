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

// Configuration constants
constexpr size_t DEFAULT_OCTREE_POOL_SIZE = 1024;
constexpr size_t COLLISION_SEARCH_VOLUME_THRESHOLD = 1000;

class VoxelDataManager {
public:
    explicit VoxelDataManager(Events::EventDispatcher* eventDispatcher = nullptr)
        : m_activeResolution(VoxelResolution::Size_1cm)
        , m_workspaceManager(std::make_unique<WorkspaceManager>(eventDispatcher))
        , m_eventDispatcher(eventDispatcher) {
        
        // Initialize octree memory pool
        SparseOctree::initializePool(DEFAULT_OCTREE_POOL_SIZE);
        
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
    bool setVoxel(const Math::IncrementCoordinates& pos, VoxelResolution resolution, bool value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Validate 1cm increment position (includes Y >= 0 check)
        if (!isValidIncrementPosition(pos)) {
            return false;
        }
        
        VoxelGrid* grid = getGrid(resolution);
        if (!grid) return false;
        
        // Use the VoxelGrid to handle coordinate validation and conversion
        bool oldValue = grid->getVoxel(pos);
        
        // Handle redundant operations (setting same value as current)
        if (oldValue == value) {
            // Redundant operation - fail to indicate no change was made
            return false;
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
    
    // Overload for backward compatibility with Vector3i
    // NOTE: This interprets pos as INCREMENT coordinates for backward compatibility
    bool setVoxel(const Math::Vector3i& pos, VoxelResolution resolution, bool value) {
        Math::IncrementCoordinates incCoord(pos);
        return setVoxel(incCoord, resolution, value);
    }
    
    bool setVoxel(const VoxelPosition& voxelPos, bool value) {
        // VoxelPosition now stores increment coordinates directly
        return setVoxel(voxelPos.incrementPos, voxelPos.resolution, value);
    }
    
    bool getVoxel(const Math::IncrementCoordinates& pos, VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        const VoxelGrid* grid = getGrid(resolution);
        if (!grid) return false;
        
        return grid->getVoxel(pos);
    }
    
    // Overload for backward compatibility with Vector3i
    // NOTE: This interprets pos as INCREMENT coordinates for backward compatibility
    bool getVoxel(const Math::Vector3i& pos, VoxelResolution resolution) const {
        Math::IncrementCoordinates incCoord(pos);
        return getVoxel(incCoord, resolution);
    }
    
    bool getVoxel(const VoxelPosition& voxelPos) const {
        // VoxelPosition now stores increment coordinates directly
        return getVoxel(voxelPos.incrementPos, voxelPos.resolution);
    }
    
    bool hasVoxel(const Math::IncrementCoordinates& pos, VoxelResolution resolution) const {
        return getVoxel(pos, resolution);
    }
    
    // Overload for backward compatibility with Vector3i  
    // NOTE: This interprets pos as INCREMENT coordinates for backward compatibility
    bool hasVoxel(const Math::Vector3i& pos, VoxelResolution resolution) const {
        Math::IncrementCoordinates incCoord(pos);
        return hasVoxel(incCoord, resolution);
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
        
        // Convert world position to increment coordinates
        Math::IncrementCoordinates incrementPos = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(worldPos));
        
        // Check for overlaps if we're setting a voxel (not removing)
        if (value) {
            // Check for overlaps
            if (wouldOverlapInternal(incrementPos, resolution)) {
                // Overlap detected
                return false;
            }
            // No overlaps detected
        }
        
        // Get the old value before setting
        bool oldValue = grid->getVoxel(incrementPos);
        
        // Set the voxel in the grid
        bool result = grid->setVoxel(incrementPos, value);
        
        // Dispatch event if successful and value changed
        if (result && oldValue != value && m_eventDispatcher) {
            Events::VoxelChangedEvent event{
                incrementPos.value(),
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
        
        return grid->getVoxelAtWorldPos(Math::WorldCoordinates(worldPos));
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
    bool isValidPosition(const Math::IncrementCoordinates& pos, VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        const VoxelGrid* grid = getGrid(resolution);
        if (!grid) return false;
        
        return grid->isValidIncrementPosition(pos);
    }
    
    // Overload for backward compatibility with Vector3i
    // NOTE: This interprets pos as INCREMENT coordinates for backward compatibility  
    bool isValidPosition(const Math::Vector3i& pos, VoxelResolution resolution) const {
        Math::IncrementCoordinates incCoord(pos);
        return isValidPosition(incCoord, resolution);
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
    
    // Convenience alias for clearAll()
    void clear() {
        clearAll();
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
    bool isValidIncrementPosition(const Math::IncrementCoordinates& pos) const {
        // All integer positions are valid 1cm increments since our base unit is 1cm
        // The constraint is simply that positions must be integers
        // Additionally, Y must be >= 0 (no voxels below ground plane)
        return pos.y() >= 0;
    }
    
    // Overload for backward compatibility with Vector3i
    // NOTE: This interprets pos as INCREMENT coordinates (not grid coordinates)
    // because it's specifically checking increment positions
    bool isValidIncrementPosition(const Math::Vector3i& pos) const {
        return isValidIncrementPosition(Math::IncrementCoordinates(pos));
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
    bool wouldOverlap(const Math::IncrementCoordinates& pos, VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return wouldOverlapInternal(pos, resolution);
    }
    
    // Overload for backward compatibility with Vector3i
    // NOTE: This interprets pos as INCREMENT coordinates for backward compatibility
    bool wouldOverlap(const Math::Vector3i& pos, VoxelResolution resolution) const {
        Math::IncrementCoordinates incCoord(pos);
        return wouldOverlap(incCoord, resolution);
    }
    
    bool wouldOverlap(const VoxelPosition& voxelPos) const {
        return wouldOverlap(voxelPos.incrementPos, voxelPos.resolution);
    }
    
    // Enhancement: Adjacent position calculation
    Math::IncrementCoordinates getAdjacentPosition(const Math::IncrementCoordinates& pos, FaceDirection face, 
                                     VoxelResolution sourceRes, VoxelResolution targetRes) const {
        (void)sourceRes; // Currently unused - all voxels stored at 1cm granularity
        (void)targetRes; // Currently unused - all voxels stored at 1cm granularity
        // In the new coordinate system, all voxels are stored at 1cm increment granularity
        // Adjacent voxels are always 1 increment apart regardless of resolution
        // The resolution only affects rendering and collision detection
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        Math::IncrementCoordinates offset(0, 0, 0);
        switch (face) {
            case FaceDirection::PosX: offset = Math::IncrementCoordinates(1, 0, 0); break;
            case FaceDirection::NegX: offset = Math::IncrementCoordinates(-1, 0, 0); break;
            case FaceDirection::PosY: offset = Math::IncrementCoordinates(0, 1, 0); break;
            case FaceDirection::NegY: offset = Math::IncrementCoordinates(0, -1, 0); break;
            case FaceDirection::PosZ: offset = Math::IncrementCoordinates(0, 0, 1); break;
            case FaceDirection::NegZ: offset = Math::IncrementCoordinates(0, 0, -1); break;
        }
        
        return pos + offset;
    }
    
    // Overload for backward compatibility with Vector3i
    // NOTE: This interprets pos as INCREMENT coordinates for backward compatibility
    Math::Vector3i getAdjacentPosition(const Math::Vector3i& pos, FaceDirection face,
                                     VoxelResolution sourceRes, VoxelResolution targetRes) const {
        Math::IncrementCoordinates sourceIncCoord(pos);
        Math::IncrementCoordinates targetIncCoord = getAdjacentPosition(
            sourceIncCoord, face, sourceRes, targetRes);
        return targetIncCoord.value();
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
    
    // Internal helper methods that don't lock (must be called with mutex already held)
    Math::Vector3f getWorkspaceSizeInternal() const {
        return m_workspaceManager->getSize();
    }
    
    void initializeGrids() {
        Math::Vector3f workspaceSize = m_workspaceManager->getSize();
        
        for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
            VoxelResolution resolution = static_cast<VoxelResolution>(i);
            m_grids[i] = std::make_unique<VoxelGrid>(resolution, workspaceSize);
        }
    }
    
    bool validateWorkspaceResize(const Math::Vector3f& oldSize, const Math::Vector3f& newSize) {
        (void)oldSize; // Currently unused - validation only checks if voxels would be lost
        // Check if any grids would lose voxels during resize
        for (const auto& grid : m_grids) {
            if (grid && !grid->resizeWorkspace(newSize)) {
                return false; // Cannot resize - would lose voxels
            }
        }
        return true;
    }
    
    
    void dispatchVoxelChangedEvent(const Math::IncrementCoordinates& position, VoxelResolution resolution, 
                                 bool oldValue, bool newValue) {
        if (m_eventDispatcher) {
            Events::VoxelChangedEvent event(position.value(), resolution, oldValue, newValue);
            m_eventDispatcher->dispatch(event);
        }
    }
    
    // Internal collision detection without lock (must be called with lock already held)
    // Note: pos parameter is expected to be in IncrementCoordinates
    bool wouldOverlapInternal(const Math::IncrementCoordinates& pos, VoxelResolution resolution) const {
        // Check if placing a voxel at this position would overlap with existing voxels
        // Optimized version that uses spatial queries to reduce checks
        
        Logging::Logger::getInstance().debugfc("VoxelDataManager", 
            "wouldOverlapInternal: checking increment position (%d, %d, %d) with resolution %d",
            pos.x(), pos.y(), pos.z(), static_cast<int>(resolution));
        
        // Convert increment position to world space (bottom-center coordinate)
        Math::WorldCoordinates worldBottomCenter = Math::CoordinateConverter::incrementToWorld(pos);
        float voxelSize = getVoxelSize(resolution);
        float halfSize = voxelSize * 0.5f;
        
        // Calculate bounds where worldBottomCenter is at the bottom face center
        Math::Vector3f worldMin = Math::Vector3f(
            worldBottomCenter.value().x - halfSize, 
            worldBottomCenter.value().y,              // Bottom at placement Y
            worldBottomCenter.value().z - halfSize
        );
        Math::Vector3f worldMax = Math::Vector3f(
            worldBottomCenter.value().x + halfSize, 
            worldBottomCenter.value().y + voxelSize,  // Top at placement Y + voxelSize
            worldBottomCenter.value().z + halfSize
        );
        
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
            
            // Convert world bounds to increment coordinates for this resolution
            Math::IncrementCoordinates minIncCheck = Math::CoordinateConverter::worldToIncrement(
                Math::WorldCoordinates(worldMin));
            Math::IncrementCoordinates maxIncCheck = Math::CoordinateConverter::worldToIncrement(
                Math::WorldCoordinates(worldMax));
            
            Logging::Logger::getInstance().debugfc("VoxelDataManager", 
                "Increment check bounds: min=(%d, %d, %d) max=(%d, %d, %d)",
                minIncCheck.x(), minIncCheck.y(), minIncCheck.z(),
                maxIncCheck.x(), maxIncCheck.y(), maxIncCheck.z());
            
            // Always use getAllVoxels approach for consistency and correctness
            auto voxels = grid->getAllVoxels();
            for (const auto& voxelPos : voxels) {
                // Use the VoxelPosition's getWorldBounds method for correct bottom-center bounds
                Math::Vector3f voxelMin, voxelMax;
                voxelPos.getWorldBounds(voxelMin, voxelMax);
                
                // Check for overlap (AABB intersection) with epsilon tolerance for floating-point precision
                const float epsilon = 1e-6f; // Small tolerance for floating-point comparisons
                bool overlapsX = (worldMin.x + epsilon) < voxelMax.x && (worldMax.x - epsilon) > voxelMin.x;
                bool overlapsY = (worldMin.y + epsilon) < voxelMax.y && (worldMax.y - epsilon) > voxelMin.y;
                bool overlapsZ = (worldMin.z + epsilon) < voxelMax.z && (worldMax.z - epsilon) > voxelMin.z;
                bool overlaps = overlapsX && overlapsY && overlapsZ;
                
                if (overlaps) {
                    Logging::Logger::getInstance().debugfc("VoxelDataManager", 
                        "Overlap detected: new voxel at (%d,%d,%d) overlaps with existing voxel at (%d,%d,%d)",
                        pos.x(), pos.y(), pos.z(), 
                        voxelPos.incrementPos.x(), voxelPos.incrementPos.y(), voxelPos.incrementPos.z());
                    return true; // Would overlap
                }
            }
        }
        
        return false; // No overlap detected
    }
};

}
}