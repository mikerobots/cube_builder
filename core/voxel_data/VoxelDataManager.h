#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <vector>
#include <cmath>
#include <sstream>

#include "VoxelTypes.h"
#include "VoxelGrid.h"
#include "WorkspaceManager.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/events/CommonEvents.h"
#include "../../foundation/math/BoundingBox.h"
#include "../input/PlacementValidation.h"

namespace VoxelEditor {
namespace VoxelData {

// Configuration constants
constexpr size_t DEFAULT_OCTREE_POOL_SIZE = 1024;
constexpr size_t COLLISION_SEARCH_VOLUME_THRESHOLD = 1000;

// Position validation result structure
struct PositionValidation {
    bool valid = false;
    bool withinBounds = false;
    bool aboveGroundPlane = false;
    bool alignedToGrid = false;
    bool noOverlap = true;
    std::string errorMessage;
    
    // Constructor for convenience
    PositionValidation() = default;
    PositionValidation(bool v, const std::string& msg = "") 
        : valid(v), errorMessage(msg) {}
};

// Region fill result structure
struct FillResult {
    bool success = false;
    size_t voxelsFilled = 0;
    size_t voxelsSkipped = 0;
    size_t totalPositions = 0;
    std::string errorMessage;
    
    // Per-failure type counts
    size_t failedBelowGround = 0;
    size_t failedOutOfBounds = 0;
    size_t failedOverlap = 0;
    size_t failedNotAligned = 0;
};

// Region query result structure
struct RegionQuery {
    size_t voxelCount = 0;
    bool isEmpty = true;
    Math::BoundingBox actualBounds;  // Actual bounds of voxels in region
    std::vector<VoxelPosition> voxels;  // Optional: filled if requested
};

// Batch operation structures
struct VoxelChange {
    Math::IncrementCoordinates position;
    VoxelResolution resolution;
    bool oldValue;
    bool newValue;
    
    VoxelChange(const Math::IncrementCoordinates& pos, VoxelResolution res, bool old, bool newVal)
        : position(pos), resolution(res), oldValue(old), newValue(newVal) {}
};

struct BatchResult {
    bool success = false;
    size_t totalOperations = 0;
    size_t successfulOperations = 0;
    size_t failedOperations = 0;
    std::string errorMessage;
    
    // Detailed failure tracking
    std::vector<size_t> failedIndices;  // Indices of failed operations
    std::vector<std::string> failureReasons;  // Reasons for failures
};

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
                                     VoxelResolution sourceRes, VoxelResolution /*targetRes*/) const {
        // Calculate the offset based on the source voxel resolution
        // When placing adjacent to a voxel, we need to offset by the source voxel's size
        // to ensure the new voxel is placed properly adjacent without overlap
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Get the size of the source voxel in increments (1cm units)
        float sourceVoxelSizeMeters = getVoxelSize(sourceRes);
        int offsetIncrements = static_cast<int>(sourceVoxelSizeMeters * Math::CoordinateConverter::METERS_TO_CM); // Convert meters to cm
        
        Math::IncrementCoordinates offset(0, 0, 0);
        switch (face) {
            case FaceDirection::PosX: offset = Math::IncrementCoordinates(offsetIncrements, 0, 0); break;
            case FaceDirection::NegX: offset = Math::IncrementCoordinates(-offsetIncrements, 0, 0); break;
            case FaceDirection::PosY: offset = Math::IncrementCoordinates(0, offsetIncrements, 0); break;
            case FaceDirection::NegY: offset = Math::IncrementCoordinates(0, -offsetIncrements, 0); break;
            case FaceDirection::PosZ: offset = Math::IncrementCoordinates(0, 0, offsetIncrements); break;
            case FaceDirection::NegZ: offset = Math::IncrementCoordinates(0, 0, -offsetIncrements); break;
        }
        
        return pos + offset;
    }
    
    // Overload for backward compatibility with Vector3i
    // NOTE: This interprets pos as INCREMENT coordinates for backward compatibility
    Math::Vector3i getAdjacentPosition(const Math::Vector3i& pos, FaceDirection face,
                                     VoxelResolution sourceRes, VoxelResolution /*targetRes*/) const {
        Math::IncrementCoordinates sourceIncCoord(pos);
        Math::IncrementCoordinates targetIncCoord = getAdjacentPosition(
            sourceIncCoord, face, sourceRes, sourceRes);
        return targetIncCoord.value();
    }
    
    // Event dispatcher
    void setEventDispatcher(Events::EventDispatcher* eventDispatcher) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_eventDispatcher = eventDispatcher;
        m_workspaceManager->setEventDispatcher(eventDispatcher);
    }
    
    // Comprehensive position validation API
    PositionValidation validatePosition(const Math::IncrementCoordinates& pos, 
                                       VoxelResolution resolution,
                                       bool checkOverlap = true) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return validatePositionInternal(pos, resolution, checkOverlap);
    }
    
    PositionValidation validatePosition(const Math::WorldCoordinates& worldPos,
                                       VoxelResolution resolution,
                                       bool checkOverlap = true) const {
        auto incPos = Math::CoordinateConverter::worldToIncrement(worldPos);
        return validatePosition(incPos, resolution, checkOverlap);
    }
    
    // Individual validation helper methods
    bool isWithinWorkspaceBounds(const Math::IncrementCoordinates& pos) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return isWithinWorkspaceBoundsInternal(pos);
    }
    
    bool isAboveGroundPlane(const Math::IncrementCoordinates& pos) const {
        return pos.y() >= 0;
    }
    
    
    Math::IncrementCoordinates clampToWorkspace(const Math::IncrementCoordinates& pos) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return clampToWorkspaceInternal(pos);
    }
    
    // Region operations API
    FillResult fillRegion(const Math::BoundingBox& region,
                         VoxelResolution resolution,
                         bool fillValue = true) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return fillRegionInternal(region, resolution, fillValue);
    }
    
    bool canFillRegion(const Math::BoundingBox& region,
                      VoxelResolution resolution) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return canFillRegionInternal(region, resolution);
    }
    
    bool isRegionEmpty(const Math::BoundingBox& region) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return isRegionEmptyInternal(region);
    }
    
    RegionQuery queryRegion(const Math::BoundingBox& region,
                           bool includeVoxelList = false) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return queryRegionInternal(region, includeVoxelList);
    }
    
    std::vector<VoxelPosition> getVoxelsInRegion(const Math::BoundingBox& region) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return getVoxelsInRegionInternal(region);
    }
    
    // Batch operations API
    BatchResult batchSetVoxels(const std::vector<VoxelChange>& changes) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return batchSetVoxelsInternal(changes);
    }
    
    bool batchValidate(const std::vector<VoxelChange>& changes,
                      std::vector<PositionValidation>& validationResults) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return batchValidateInternal(changes, validationResults);
    }
    
    // Convenience method for creating batch changes
    std::vector<VoxelChange> createBatchChanges(
        const std::vector<Math::IncrementCoordinates>& positions,
        VoxelResolution resolution,
        bool newValue) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return createBatchChangesInternal(positions, resolution, newValue);
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
    
    // Internal validation methods (must be called with lock already held)
    PositionValidation validatePositionInternal(const Math::IncrementCoordinates& pos,
                                               VoxelResolution resolution,
                                               bool checkOverlap) const {
        PositionValidation result;
        
        // Check if above ground plane
        result.aboveGroundPlane = isAboveGroundPlane(pos);
        if (!result.aboveGroundPlane) {
            result.errorMessage = "Position is below ground plane (Y must be >= 0)";
            return result;
        }
        
        // Check if placement position is within workspace bounds
        result.withinBounds = isWithinWorkspaceBoundsInternal(pos);
        if (!result.withinBounds) {
            result.errorMessage = "Position is outside workspace bounds";
            return result;
        }
        
        // Check if entire voxel extent would fit within workspace bounds
        if (!isVoxelExtentWithinBoundsInternal(pos, resolution)) {
            result.withinBounds = false;
            result.errorMessage = "Voxel would extend outside workspace bounds";
            return result;
        }
        
        // All 1cm increment positions are valid (no resolution-based grid alignment)
        result.alignedToGrid = true;
        
        // Check for overlaps if requested
        if (checkOverlap) {
            result.noOverlap = !wouldOverlapInternal(pos, resolution);
            if (!result.noOverlap) {
                result.errorMessage = "Position would overlap with existing voxel";
                return result;
            }
        }
        
        // All checks passed
        result.valid = true;
        result.errorMessage.clear();
        return result;
    }
    
    bool isWithinWorkspaceBoundsInternal(const Math::IncrementCoordinates& pos) const {
        // Convert to world coordinates for bounds checking
        Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(pos);
        return m_workspaceManager->isPositionValid(worldPos.value());
    }
    
    bool isVoxelExtentWithinBoundsInternal(const Math::IncrementCoordinates& pos, VoxelResolution resolution) const {
        // Check if entire voxel extent would fit within workspace bounds
        Math::WorldCoordinates worldBottomCenter = Math::CoordinateConverter::incrementToWorld(pos);
        float voxelSize = getVoxelSize(resolution);
        float halfSize = voxelSize * 0.5f;
        
        // Calculate voxel bounds (bottom-center coordinate system)
        Math::Vector3f voxelMin(
            worldBottomCenter.value().x - halfSize,
            worldBottomCenter.value().y,  // Bottom at placement Y
            worldBottomCenter.value().z - halfSize
        );
        Math::Vector3f voxelMax(
            worldBottomCenter.value().x + halfSize,
            worldBottomCenter.value().y + voxelSize,  // Top at Y + voxelSize
            worldBottomCenter.value().z + halfSize
        );
        
        // Get workspace bounds
        Math::Vector3f workspaceSize = m_workspaceManager->getSize();
        Math::Vector3f workspaceMin(-workspaceSize.x * 0.5f, 0.0f, -workspaceSize.z * 0.5f);
        Math::Vector3f workspaceMax(workspaceSize.x * 0.5f, workspaceSize.y, workspaceSize.z * 0.5f);
        
        // Check if voxel is completely within workspace bounds
        return voxelMin.x >= workspaceMin.x && voxelMax.x <= workspaceMax.x &&
               voxelMin.y >= workspaceMin.y && voxelMax.y <= workspaceMax.y &&
               voxelMin.z >= workspaceMin.z && voxelMax.z <= workspaceMax.z;
    }
    
    
    Math::IncrementCoordinates clampToWorkspaceInternal(const Math::IncrementCoordinates& pos) const {
        // Get workspace bounds in world coordinates
        Math::Vector3f workspaceSize = m_workspaceManager->getSize();
        Math::Vector3f halfSize = workspaceSize * 0.5f;
        
        // Convert to world coordinates
        Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(pos);
        
        // Clamp to workspace bounds
        float clampedX = std::max(-halfSize.x, std::min(halfSize.x, worldPos.value().x));
        float clampedY = std::max(0.0f, std::min(workspaceSize.y, worldPos.value().y));
        float clampedZ = std::max(-halfSize.z, std::min(halfSize.z, worldPos.value().z));
        
        // Convert back to increment coordinates
        return Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(clampedX, clampedY, clampedZ)
        );
    }
    
    // Internal collision detection without lock (must be called with lock already held)
    // Note: pos parameter is expected to be in IncrementCoordinates
    bool wouldOverlapInternal(const Math::IncrementCoordinates& pos, VoxelResolution resolution) const {
        // Check if placing a voxel at this position would overlap with existing voxels
        // REQ-5.2.5: Smaller voxels may be placed on the faces of larger voxels for detailed work
        // REQ-4.3.4: Collision detection shall apply between voxels of different resolutions (smaller voxels allowed on larger ones)
        
        Logging::Logger::getInstance().debugfc("VoxelDataManager", 
            "wouldOverlapInternal: checking increment position (%d, %d, %d) with resolution %d",
            pos.x(), pos.y(), pos.z(), static_cast<int>(resolution));
        
        // Get the size of the voxel we're trying to place
        float voxelSize = getVoxelSize(resolution);
        float halfSize = voxelSize * 0.5f;
        
        // Convert to world coordinates for bounds calculation
        Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(pos);
        
        // Calculate bounds for the voxel we're trying to place (bottom-center coordinate system)
        Math::Vector3f newVoxelMin(
            worldPos.value().x - halfSize,
            worldPos.value().y,  // Bottom at Y
            worldPos.value().z - halfSize
        );
        Math::Vector3f newVoxelMax(
            worldPos.value().x + halfSize,
            worldPos.value().y + voxelSize,  // Top at Y + size
            worldPos.value().z + halfSize
        );
        
        // Check each resolution level for overlaps
        for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
            VoxelResolution checkRes = static_cast<VoxelResolution>(i);
            const VoxelGrid* grid = getGrid(checkRes);
            if (!grid || grid->getVoxelCount() == 0) continue;
            
            Logging::Logger::getInstance().debugfc("VoxelDataManager", 
                "Checking against resolution %d with %zu voxels",
                i, grid->getVoxelCount());
            
            // Get all voxels in this resolution level
            auto voxels = grid->getAllVoxels();
            for (const auto& voxelPos : voxels) {
                // Calculate bounds for the existing voxel
                Math::Vector3f existingMin, existingMax;
                voxelPos.getWorldBounds(existingMin, existingMax);
                
                // Check for AABB overlap (same algorithm as VoxelCollision::boundsOverlap)
                // Add small epsilon to handle floating point precision for adjacent voxels
                const float epsilon = 0.0001f;
                bool overlaps = (newVoxelMin.x < existingMax.x - epsilon && newVoxelMax.x > existingMin.x + epsilon) &&
                               (newVoxelMin.y < existingMax.y - epsilon && newVoxelMax.y > existingMin.y + epsilon) &&
                               (newVoxelMin.z < existingMax.z - epsilon && newVoxelMax.z > existingMin.z + epsilon);
                
                if (overlaps) {
                    // REQ-5.2.5: Allow smaller voxels to be placed on or inside larger voxels for detailed work
                    float existingSize = getVoxelSize(voxelPos.resolution);
                    
                    // Check if voxels are at the exact same position - this should always be considered an overlap
                    if (pos == voxelPos.incrementPos) {
                        Logging::Logger::getInstance().debugfc("VoxelDataManager", 
                            "Overlap detected: voxels at exact same position (%d,%d,%d)",
                            pos.x(), pos.y(), pos.z());
                        return true; // Same position always overlaps
                    }
                    
                    // If the new voxel is smaller than the existing one, allow the placement
                    // This enables detailed work on larger voxels
                    if (voxelSize < existingSize) {
                        Logging::Logger::getInstance().debugfc("VoxelDataManager", 
                            "Allowing placement: smaller voxel (%dcm) on/inside larger voxel (%dcm)",
                            static_cast<int>(voxelSize * Math::CoordinateConverter::METERS_TO_CM),
                            static_cast<int>(existingSize * Math::CoordinateConverter::METERS_TO_CM));
                        continue; // Allow placement, check next voxel
                    }
                    
                    // Otherwise, overlap is not allowed
                    Logging::Logger::getInstance().debugfc("VoxelDataManager", 
                        "Overlap detected: new voxel at (%d,%d,%d) size %dcm would overlap with existing voxel at (%d,%d,%d) size %dcm",
                        pos.x(), pos.y(), pos.z(), static_cast<int>(voxelSize * Math::CoordinateConverter::METERS_TO_CM),
                        voxelPos.incrementPos.x(), voxelPos.incrementPos.y(), voxelPos.incrementPos.z(),
                        static_cast<int>(existingSize * Math::CoordinateConverter::METERS_TO_CM));
                    return true; // Would overlap
                }
            }
        }
        
        return false; // No overlap detected
    }
    
    // Internal region operation methods (must be called with lock already held)
    FillResult fillRegionInternal(const Math::BoundingBox& region,
                                 VoxelResolution resolution,
                                 bool fillValue) {
        FillResult result;
        
        // Convert world bounds to increment coordinates
        Math::IncrementCoordinates minInc = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(region.min)
        );
        Math::IncrementCoordinates maxInc = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(region.max)
        );
        
        // Get voxel size for alignment
        float voxelSizeMeters = getVoxelSize(resolution);
        int voxelSizeIncrements = static_cast<int>(voxelSizeMeters * Math::CoordinateConverter::METERS_TO_CM);
        
        // Align bounds to voxel grid
        int alignedMinX = (minInc.x() / voxelSizeIncrements) * voxelSizeIncrements;
        int alignedMinY = (minInc.y() / voxelSizeIncrements) * voxelSizeIncrements;
        int alignedMinZ = (minInc.z() / voxelSizeIncrements) * voxelSizeIncrements;
        
        // If min is not aligned, round up to next valid position
        if (alignedMinX < minInc.x()) alignedMinX += voxelSizeIncrements;
        if (alignedMinY < minInc.y()) alignedMinY += voxelSizeIncrements;
        if (alignedMinZ < minInc.z()) alignedMinZ += voxelSizeIncrements;
        
        // Quick empty region check for fill operations
        bool hasAnyVoxels = false;
        if (fillValue && !hasAnyVoxels) {
            hasAnyVoxels = !isRegionEmptyInternal(region);
        }
        
        // Iterate through all positions in the aligned region
        for (int x = alignedMinX; x <= maxInc.x(); x += voxelSizeIncrements) {
            for (int y = alignedMinY; y <= maxInc.y(); y += voxelSizeIncrements) {
                for (int z = alignedMinZ; z <= maxInc.z(); z += voxelSizeIncrements) {
                    result.totalPositions++;
                    
                    Math::IncrementCoordinates pos(x, y, z);
                    
                    // Validate position (skip overlap checks for performance)
                    auto validation = validatePositionInternal(pos, resolution, false);
                    
                    if (!validation.valid) {
                        result.voxelsSkipped++;
                        
                        // Track failure reasons
                        if (!validation.aboveGroundPlane) result.failedBelowGround++;
                        else if (!validation.withinBounds) result.failedOutOfBounds++;
                        else if (!validation.alignedToGrid) result.failedNotAligned++;
                        else if (!validation.noOverlap) result.failedOverlap++;
                        
                        continue;
                    }
                    
                    // Check if we need to change the voxel using direct grid access
                    VoxelGrid* grid = getGrid(resolution);
                    if (!grid) {
                        result.voxelsSkipped++;
                        continue;
                    }
                    
                    bool currentValue = grid->getVoxel(pos);
                    if (currentValue == fillValue) {
                        result.voxelsSkipped++;
                        continue;
                    }
                    
                    // Set the voxel using direct grid access
                    if (grid->setVoxel(pos, fillValue)) {
                        result.voxelsFilled++;
                        // Dispatch event manually since we're bypassing the public setVoxel
                        dispatchVoxelChangedEvent(pos, resolution, currentValue, fillValue);
                    } else {
                        result.voxelsSkipped++;
                    }
                }
            }
        }
        
        // Set result status
        result.success = (result.voxelsSkipped == 0) || 
                        (result.voxelsFilled > 0 && result.totalPositions == result.voxelsFilled + result.voxelsSkipped);
        
        // Build error message if there were failures
        if (!result.success && result.voxelsSkipped > 0) {
            std::stringstream ss;
            ss << "Fill operation partially failed: ";
            if (result.failedBelowGround > 0) ss << result.failedBelowGround << " below ground, ";
            if (result.failedOutOfBounds > 0) ss << result.failedOutOfBounds << " out of bounds, ";
            if (result.failedOverlap > 0) ss << result.failedOverlap << " would overlap, ";
            if (result.failedNotAligned > 0) ss << result.failedNotAligned << " not aligned";
            result.errorMessage = ss.str();
        }
        
        return result;
    }
    
    bool canFillRegionInternal(const Math::BoundingBox& region, VoxelResolution resolution) const {
        // Convert bounds and check if any position would fail
        Math::IncrementCoordinates minInc = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(region.min)
        );
        Math::IncrementCoordinates maxInc = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(region.max)
        );
        
        // Get voxel size for alignment
        float voxelSizeMeters = getVoxelSize(resolution);
        int voxelSizeIncrements = static_cast<int>(voxelSizeMeters * Math::CoordinateConverter::METERS_TO_CM);
        
        // Quick boundary checks
        if (minInc.y() < 0) return false;  // Below ground
        
        // Check a few sample positions for quick rejection
        std::vector<Math::IncrementCoordinates> samplePoints = {
            minInc,
            maxInc,
            Math::IncrementCoordinates((minInc.x() + maxInc.x()) / 2,
                                     (minInc.y() + maxInc.y()) / 2,
                                     (minInc.z() + maxInc.z()) / 2)
        };
        
        for (const auto& pos : samplePoints) {
            auto validation = validatePositionInternal(pos, resolution, true);
            if (!validation.valid) {
                return false;
            }
        }
        
        return true;
    }
    
    bool isRegionEmptyInternal(const Math::BoundingBox& region) const {
        // Check all resolutions for any voxels in the region
        for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
            VoxelResolution res = static_cast<VoxelResolution>(i);
            const VoxelGrid* grid = getGrid(res);
            if (!grid || grid->getVoxelCount() == 0) continue;
            
            // Get all voxels and check if any are in the region
            auto voxels = grid->getAllVoxels();
            for (const auto& voxel : voxels) {
                Math::Vector3f voxelMin, voxelMax;
                voxel.getWorldBounds(voxelMin, voxelMax);
                
                // Check AABB intersection
                if (region.intersects(Math::BoundingBox(voxelMin, voxelMax))) {
                    return false;  // Found a voxel in the region
                }
            }
        }
        
        return true;  // No voxels found
    }
    
    RegionQuery queryRegionInternal(const Math::BoundingBox& region, bool includeVoxelList) const {
        RegionQuery query;
        query.actualBounds = Math::BoundingBox();  // Invalid bounds initially
        
        bool firstVoxel = true;
        
        // Check all resolutions
        for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
            VoxelResolution res = static_cast<VoxelResolution>(i);
            const VoxelGrid* grid = getGrid(res);
            if (!grid || grid->getVoxelCount() == 0) continue;
            
            // Get all voxels and check if they're in the region
            auto voxels = grid->getAllVoxels();
            for (const auto& voxel : voxels) {
                Math::Vector3f voxelMin, voxelMax;
                voxel.getWorldBounds(voxelMin, voxelMax);
                Math::BoundingBox voxelBounds(voxelMin, voxelMax);
                
                // Check if voxel intersects the query region
                if (region.intersects(voxelBounds)) {
                    query.voxelCount++;
                    query.isEmpty = false;
                    
                    // Update actual bounds
                    if (firstVoxel) {
                        query.actualBounds = voxelBounds;
                        firstVoxel = false;
                    } else {
                        query.actualBounds.expandToInclude(voxelBounds);
                    }
                    
                    // Add to voxel list if requested
                    if (includeVoxelList) {
                        query.voxels.push_back(voxel);
                    }
                }
            }
        }
        
        return query;
    }
    
    std::vector<VoxelPosition> getVoxelsInRegionInternal(const Math::BoundingBox& region) const {
        auto query = queryRegionInternal(region, true);
        return query.voxels;
    }
    
    // Internal batch operation methods (must be called with lock already held)
    BatchResult batchSetVoxelsInternal(const std::vector<VoxelChange>& changes) {
        BatchResult result;
        result.totalOperations = changes.size();
        
        // Pre-validate all changes first for atomicity
        std::vector<PositionValidation> validations;
        validations.reserve(changes.size());
        
        for (size_t i = 0; i < changes.size(); ++i) {
            const auto& change = changes[i];
            auto validation = validatePositionInternal(change.position, change.resolution, change.newValue);
            validations.push_back(validation);
            
            if (!validation.valid) {
                result.failedOperations++;
                result.failedIndices.push_back(i);
                result.failureReasons.push_back(validation.errorMessage);
            }
        }
        
        // If any validation failed, abort the entire operation (atomicity)
        if (result.failedOperations > 0) {
            result.errorMessage = "Batch validation failed for " + 
                                std::to_string(result.failedOperations) + " operations";
            return result;
        }
        
        // All validations passed, now perform the actual changes
        std::vector<VoxelChange> appliedChanges;  // For rollback if needed
        appliedChanges.reserve(changes.size());
        
        for (size_t i = 0; i < changes.size(); ++i) {
            const auto& change = changes[i];
            
            // Get current value for rollback using internal non-locking method
            VoxelGrid* grid = getGrid(change.resolution);
            if (!grid) {
                result.failedOperations++;
                result.failedIndices.push_back(i);
                result.failureReasons.push_back("Invalid resolution");
                
                // Rollback all applied changes
                for (const auto& appliedChange : appliedChanges) {
                    VoxelGrid* rollbackGrid = getGrid(appliedChange.resolution);
                    if (rollbackGrid) {
                        rollbackGrid->setVoxel(appliedChange.position, appliedChange.oldValue);
                    }
                }
                
                result.errorMessage = "Batch operation failed at operation " + 
                                    std::to_string(i) + ", all changes rolled back";
                result.successfulOperations = 0;
                return result;
            }
            
            bool currentValue = grid->getVoxel(change.position);
            
            // Only apply if there's actually a change
            if (currentValue != change.newValue) {
                if (grid->setVoxel(change.position, change.newValue)) {
                    result.successfulOperations++;
                    appliedChanges.emplace_back(change.position, change.resolution, 
                                              currentValue, change.newValue);
                    
                    // Dispatch event manually since we're bypassing the public setVoxel
                    dispatchVoxelChangedEvent(change.position, change.resolution, currentValue, change.newValue);
                } else {
                    // Unexpected failure - rollback all changes
                    result.failedOperations++;
                    result.failedIndices.push_back(i);
                    result.failureReasons.push_back("Unexpected setVoxel failure");
                    
                    // Rollback all applied changes
                    for (const auto& appliedChange : appliedChanges) {
                        VoxelGrid* rollbackGrid = getGrid(appliedChange.resolution);
                        if (rollbackGrid) {
                            rollbackGrid->setVoxel(appliedChange.position, appliedChange.oldValue);
                            // Dispatch rollback event
                            dispatchVoxelChangedEvent(appliedChange.position, appliedChange.resolution, appliedChange.newValue, appliedChange.oldValue);
                        }
                    }
                    
                    result.errorMessage = "Batch operation failed at operation " + 
                                        std::to_string(i) + ", all changes rolled back";
                    result.successfulOperations = 0;
                    return result;
                }
            } else {
                // No change needed, but count as successful
                result.successfulOperations++;
            }
        }
        
        result.success = (result.failedOperations == 0);
        return result;
    }
    
    bool batchValidateInternal(const std::vector<VoxelChange>& changes,
                              std::vector<PositionValidation>& validationResults) const {
        validationResults.clear();
        validationResults.reserve(changes.size());
        
        bool allValid = true;
        
        for (const auto& change : changes) {
            auto validation = validatePositionInternal(change.position, change.resolution, change.newValue);
            validationResults.push_back(validation);
            
            if (!validation.valid) {
                allValid = false;
            }
        }
        
        return allValid;
    }
    
    std::vector<VoxelChange> createBatchChangesInternal(
        const std::vector<Math::IncrementCoordinates>& positions,
        VoxelResolution resolution,
        bool newValue) const {
        
        std::vector<VoxelChange> changes;
        changes.reserve(positions.size());
        
        for (const auto& pos : positions) {
            // Use internal non-locking version to avoid deadlock
            const VoxelGrid* grid = getGrid(resolution);
            bool currentValue = (grid != nullptr) ? grid->getVoxel(pos) : false;
            changes.emplace_back(pos, resolution, currentValue, newValue);
        }
        
        return changes;
    }
};

}
}