#pragma once

#include <optional>
#include <vector>

#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include "../voxel_data/VoxelTypes.h"

namespace VoxelEditor {

// Forward declarations
namespace VoxelData {
    class VoxelDataManager;
}

namespace Input {

// Represents a placement plane at a specific height
struct PlacementPlane {
    float height;                           // World Y coordinate of the plane
    Math::IncrementCoordinates referenceVoxel; // Position of voxel that defines this plane
    VoxelData::VoxelResolution resolution; // Resolution of the reference voxel
    bool isGroundPlane;                    // True if this is the ground plane (Y=0)
    
    PlacementPlane() 
        : height(0.0f)
        , referenceVoxel(0, 0, 0)
        , resolution(VoxelData::VoxelResolution::Size_1cm)
        , isGroundPlane(true) {}
        
    PlacementPlane(float h, const Math::IncrementCoordinates& refVoxel, VoxelData::VoxelResolution res)
        : height(h)
        , referenceVoxel(refVoxel)
        , resolution(res)
        , isGroundPlane(h == 0.0f) {}
        
    // Create ground plane
    static PlacementPlane GroundPlane() {
        return PlacementPlane(0.0f, Math::IncrementCoordinates(0, 0, 0), VoxelData::VoxelResolution::Size_1cm);
    }
    
    bool operator==(const PlacementPlane& other) const {
        return std::abs(height - other.height) < 0.001f &&
               referenceVoxel == other.referenceVoxel &&
               resolution == other.resolution &&
               isGroundPlane == other.isGroundPlane;
    }
    
    bool operator!=(const PlacementPlane& other) const {
        return !(*this == other);
    }
};

// Context information for plane detection
struct PlaneDetectionContext {
    Math::Vector3f worldPosition;          // World position from ray cast
    Math::Vector3f rayOrigin;              // Ray origin
    Math::Vector3f rayDirection;           // Ray direction (normalized)
    VoxelData::VoxelResolution currentResolution; // Current voxel resolution being placed
    
    PlaneDetectionContext()
        : worldPosition(0.0f, 0.0f, 0.0f)
        , rayOrigin(0.0f, 0.0f, 0.0f)
        , rayDirection(0.0f, -1.0f, 0.0f)
        , currentResolution(VoxelData::VoxelResolution::Size_1cm) {}
};

// Result of plane detection
struct PlaneDetectionResult {
    bool found;                            // True if a valid plane was found
    PlacementPlane plane;                  // The detected plane
    std::vector<Math::IncrementCoordinates> voxelsOnPlane; // Voxels found on this plane
    float distanceFromRay;                 // Distance from ray origin to plane
    
    PlaneDetectionResult() : found(false), distanceFromRay(0.0f) {}
    
    static PlaneDetectionResult NotFound() {
        return PlaneDetectionResult();
    }
    
    static PlaneDetectionResult Found(const PlacementPlane& p, float distance = 0.0f) {
        PlaneDetectionResult result;
        result.found = true;
        result.plane = p;
        result.distanceFromRay = distance;
        return result;
    }
};

/**
 * PlaneDetector handles detection and persistence of placement planes for voxel placement.
 * 
 * Key responsibilities:
 * 1. Find the highest voxel under cursor position
 * 2. Track current placement plane state  
 * 3. Detect when larger voxel would overlap smaller voxels
 * 4. Maintain plane persistence until preview completely clears
 * 5. Handle plane transitions smoothly
 */
class PlaneDetector {
public:
    // Structure to track voxel with its resolution
    struct VoxelInfo {
        Math::IncrementCoordinates position;
        VoxelData::VoxelResolution resolution;
        
        VoxelInfo(const Math::IncrementCoordinates& pos, VoxelData::VoxelResolution res)
            : position(pos), resolution(res) {}
    };
    explicit PlaneDetector(VoxelData::VoxelDataManager* voxelManager);
    ~PlaneDetector() = default;
    
    // Non-copyable
    PlaneDetector(const PlaneDetector&) = delete;
    PlaneDetector& operator=(const PlaneDetector&) = delete;
    
    // Core plane detection
    PlaneDetectionResult detectPlane(const PlaneDetectionContext& context);
    
    // Find highest voxel under cursor (vertical ray cast)
    // Returns the voxel position and its resolution
    std::optional<VoxelInfo> findHighestVoxelUnderCursor(const Math::Vector3f& worldPos, 
                                                         float searchRadius = 0.16f); // 32cm / 2
    
    // Get current active placement plane
    std::optional<PlacementPlane> getCurrentPlane() const { return m_currentPlane; }
    
    // Update plane persistence based on preview position
    void updatePlanePersistence(const Math::IncrementCoordinates& previewPosition, 
                               VoxelData::VoxelResolution previewResolution,
                               float deltaTime);
    
    // Check if preview overlaps any voxels on current plane
    bool previewOverlapsCurrentPlane(const Math::IncrementCoordinates& previewPosition,
                                   VoxelData::VoxelResolution previewResolution) const;
    
    // Force set the current plane (for external control)
    void setCurrentPlane(const PlacementPlane& plane) { m_currentPlane = plane; }
    
    // Clear current plane (resets to ground plane)
    void clearCurrentPlane() { m_currentPlane = std::nullopt; }
    
    // Check if we should transition to a new plane
    bool shouldTransitionToNewPlane(const PlaneDetectionResult& newPlaneResult) const;
    
    // Get all voxels at a specific height
    std::vector<Math::IncrementCoordinates> getVoxelsAtHeight(float height, float tolerance = 0.001f) const;
    
    // Calculate the top face height of a voxel
    float calculateVoxelTopHeight(const Math::IncrementCoordinates& voxelPos, VoxelData::VoxelResolution resolution) const;
    
    // Reset to initial state
    void reset();
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    std::optional<PlacementPlane> m_currentPlane;
    
    // Plane persistence tracking
    bool m_planePersistenceActive;
    float m_persistenceTimeout;
    
    // Constants
    static constexpr float PERSISTENCE_TIMEOUT_SECONDS = 0.5f;
    static constexpr float MAX_VOXEL_SEARCH_HEIGHT = 20.0f;  // Maximum height to search for voxels
    static constexpr float DEFAULT_SEARCH_RADIUS = 5.0f;     // Default radius for area searches
    
    // Search for voxels in a cylindrical area under the cursor
    std::vector<Math::IncrementCoordinates> searchVoxelsInCylinder(const Math::Vector3f& centerPos, 
                                                      float radius, 
                                                      float minHeight, 
                                                      float maxHeight) const;
    
    // Find the highest voxel from a collection
    std::optional<Math::IncrementCoordinates> findHighestVoxel(const std::vector<Math::IncrementCoordinates>& voxels) const;
    
    // Check if a larger voxel would overlap smaller voxels at a position
    bool wouldLargerVoxelOverlapSmaller(const Math::IncrementCoordinates& position,
                                       VoxelData::VoxelResolution largerResolution) const;
    
    // Get all resolutions to check for conflicts
    std::vector<VoxelData::VoxelResolution> getAllResolutions() const;
    
    // Note: These conversion methods are now handled by CoordinateConverter
    // Use Math::CoordinateConverter::worldToIncrement() and incrementToWorld() instead
};

}
}