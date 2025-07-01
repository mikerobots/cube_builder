#include "../include/voxel_math/VoxelCollision.h"
#include "../include/voxel_math/VoxelGrid.h"
#include "../include/voxel_math/FaceOperations.h"
#include "../../../core/voxel_data/VoxelGrid.h"
#include "../../math/CoordinateConverter.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace VoxelEditor {
namespace Math {

bool VoxelCollision::checkCollision(const IncrementCoordinates& pos1, VoxelData::VoxelResolution res1,
                                   const IncrementCoordinates& pos2, VoxelData::VoxelResolution res2) {
    // Create bounds for both voxels
    VoxelBounds bounds1(pos1, VoxelGrid::getVoxelSizeMeters(res1));
    VoxelBounds bounds2(pos2, VoxelGrid::getVoxelSizeMeters(res2));
    
    // Check if bounds overlap
    return boundsOverlap(bounds1, bounds2);
}

bool VoxelCollision::boundsOverlap(const VoxelBounds& bounds1, const VoxelBounds& bounds2) {
    const Vector3f& min1 = bounds1.min().value();
    const Vector3f& max1 = bounds1.max().value();
    const Vector3f& min2 = bounds2.min().value();
    const Vector3f& max2 = bounds2.max().value();
    
    // Check for separation along each axis
    // If separated along any axis, they don't overlap
    // Use <= and >= to consider touching voxels as NOT overlapping
    return !(max1.x <= min2.x || min1.x >= max2.x ||
             max1.y <= min2.y || min1.y >= max2.y ||
             max1.z <= min2.z || min1.z >= max2.z);
}

bool VoxelCollision::checkCollisionWithGrid(const IncrementCoordinates& pos,
                                           VoxelData::VoxelResolution resolution,
                                           const VoxelData::VoxelGrid& grid) {
    // Create bounds for the voxel we want to place
    VoxelBounds placementBounds(pos, VoxelGrid::getVoxelSizeMeters(resolution));
    
    // Get all voxels from the grid
    auto allVoxels = grid.getAllVoxels();
    
    // Check collision with each existing voxel
    for (const auto& voxelData : allVoxels) {
        if (checkCollision(pos, resolution, voxelData.incrementPos, voxelData.resolution)) {
            return true;  // Collision found
        }
    }
    
    return false;  // No collision
}

std::vector<VoxelCollision::VoxelInfo> VoxelCollision::getCollidingVoxels(
    const IncrementCoordinates& pos,
    VoxelData::VoxelResolution resolution,
    const VoxelData::VoxelGrid& grid) {
    
    std::vector<VoxelInfo> collidingVoxels;
    
    // Create bounds for the voxel we want to place
    VoxelBounds placementBounds(pos, VoxelGrid::getVoxelSizeMeters(resolution));
    
    // Get all voxels from the grid
    auto allVoxels = grid.getAllVoxels();
    
    // Check collision with each existing voxel
    for (const auto& voxelData : allVoxels) {
        if (checkCollision(pos, resolution, voxelData.incrementPos, voxelData.resolution)) {
            collidingVoxels.emplace_back(voxelData.incrementPos, voxelData.resolution);
        }
    }
    
    return collidingVoxels;
}

std::vector<VoxelCollision::VoxelInfo> VoxelCollision::getVoxelsInRegion(
    const VoxelBounds& region,
    const VoxelData::VoxelGrid& grid) {
    
    std::vector<VoxelInfo> voxelsInRegion;
    
    // Get all voxels from the grid
    auto allVoxels = grid.getAllVoxels();
    
    // Check each voxel to see if it's in the region
    for (const auto& voxelData : allVoxels) {
        VoxelBounds voxelBounds(voxelData.incrementPos, 
                               VoxelGrid::getVoxelSizeMeters(voxelData.resolution));
        
        // Check if voxel bounds overlap with region
        if (boundsOverlap(voxelBounds, region)) {
            voxelsInRegion.emplace_back(voxelData.incrementPos, voxelData.resolution);
        }
    }
    
    return voxelsInRegion;
}

std::optional<IncrementCoordinates> VoxelCollision::findNearestFreePosition(
    const IncrementCoordinates& desiredPos,
    VoxelData::VoxelResolution resolution,
    const VoxelData::VoxelGrid& grid,
    int maxSearchDistance) {
    
    // If desired position is free, return it
    if (!checkCollisionWithGrid(desiredPos, resolution, grid)) {
        return desiredPos;
    }
    
    // Use a priority queue to search positions by distance
    struct SearchNode {
        IncrementCoordinates pos;
        int distance;
        
        bool operator>(const SearchNode& other) const {
            return distance > other.distance;
        }
    };
    
    std::priority_queue<SearchNode, std::vector<SearchNode>, std::greater<SearchNode>> searchQueue;
    std::unordered_set<std::string> visited;
    
    // Helper to create unique key for position
    auto makeKey = [](const IncrementCoordinates& p) {
        return std::to_string(p.x()) + "," + std::to_string(p.y()) + "," + std::to_string(p.z());
    };
    
    // Start search from desired position
    searchQueue.push({desiredPos, 0});
    visited.insert(makeKey(desiredPos));
    
    // Get voxel size for search steps
    int voxelSizeCm = VoxelGrid::getVoxelSizeCm(resolution);
    
    while (!searchQueue.empty()) {
        SearchNode current = searchQueue.top();
        searchQueue.pop();
        
        // Check if we've exceeded search distance
        if (current.distance > maxSearchDistance) {
            break;
        }
        
        // Try positions in all 6 directions
        for (int dir = 0; dir < 6; ++dir) {
            auto faceDir = FaceOperations::indexToFaceDirection(dir);
            Vector3i offset = FaceOperations::getFaceOffset(faceDir, voxelSizeCm);
            
            IncrementCoordinates newPos(
                current.pos.x() + offset.x,
                current.pos.y() + offset.y,
                current.pos.z() + offset.z
            );
            
            // Skip if already visited
            std::string key = makeKey(newPos);
            if (visited.find(key) != visited.end()) {
                continue;
            }
            visited.insert(key);
            
            // Check if position is free
            if (!checkCollisionWithGrid(newPos, resolution, grid)) {
                return newPos;  // Found free position
            }
            
            // Add to search queue
            int newDistance = std::abs(newPos.x() - desiredPos.x()) +
                             std::abs(newPos.y() - desiredPos.y()) +
                             std::abs(newPos.z() - desiredPos.z());
            searchQueue.push({newPos, newDistance});
        }
    }
    
    return std::nullopt;  // No free position found
}

bool VoxelCollision::isCompletelySurrounded(const IncrementCoordinates& pos,
                                           VoxelData::VoxelResolution resolution,
                                           const VoxelData::VoxelGrid& grid) {
    // Check all 6 faces
    for (int dir = 0; dir < 6; ++dir) {
        auto faceDir = FaceOperations::indexToFaceDirection(dir);
        IncrementCoordinates adjacentPos = VoxelGrid::getAdjacentPosition(pos, faceDir, resolution);
        
        // If any adjacent position is empty, not surrounded
        if (!grid.getVoxel(adjacentPos)) {
            return false;
        }
    }
    
    return true;  // All faces blocked
}

std::vector<VoxelCollision::VoxelInfo> VoxelCollision::getConnectedVoxels(
    const IncrementCoordinates& pos,
    VoxelData::VoxelResolution resolution,
    const VoxelData::VoxelGrid& grid) {
    
    std::vector<VoxelInfo> connectedVoxels;
    
    // Check all 6 adjacent positions
    for (int dir = 0; dir < 6; ++dir) {
        auto faceDir = FaceOperations::indexToFaceDirection(dir);
        IncrementCoordinates adjacentPos = VoxelGrid::getAdjacentPosition(pos, faceDir, resolution);
        
        // Get all voxels at this position (might be different resolutions)
        auto voxelsAtPos = grid.getAllVoxels();
        for (const auto& voxelData : voxelsAtPos) {
            // Check if this voxel shares a face with our voxel
            VoxelBounds ourBounds(pos, VoxelGrid::getVoxelSizeMeters(resolution));
            VoxelBounds theirBounds(voxelData.incrementPos, 
                                   VoxelGrid::getVoxelSizeMeters(voxelData.resolution));
            
            if (boundsOverlap(ourBounds, theirBounds)) {
                // Additional check: they should share a face, not just overlap
                // This is a simplified check - in practice you'd want more precise face sharing logic
                connectedVoxels.emplace_back(voxelData.incrementPos, voxelData.resolution);
            }
        }
    }
    
    return connectedVoxels;
}

float VoxelCollision::calculateIntersectionVolume(const IncrementCoordinates& pos1, 
                                                 VoxelData::VoxelResolution res1,
                                                 const IncrementCoordinates& pos2, 
                                                 VoxelData::VoxelResolution res2) {
    // Create bounds for both voxels
    VoxelBounds bounds1(pos1, VoxelGrid::getVoxelSizeMeters(res1));
    VoxelBounds bounds2(pos2, VoxelGrid::getVoxelSizeMeters(res2));
    
    // Get min/max for both bounds
    const Vector3f& min1 = bounds1.min().value();
    const Vector3f& max1 = bounds1.max().value();
    const Vector3f& min2 = bounds2.min().value();
    const Vector3f& max2 = bounds2.max().value();
    
    // Calculate intersection box
    float intersectMinX = std::max(min1.x, min2.x);
    float intersectMaxX = std::min(max1.x, max2.x);
    float intersectMinY = std::max(min1.y, min2.y);
    float intersectMaxY = std::min(max1.y, max2.y);
    float intersectMinZ = std::max(min1.z, min2.z);
    float intersectMaxZ = std::min(max1.z, max2.z);
    
    // Check if there's actual intersection
    if (intersectMaxX <= intersectMinX || 
        intersectMaxY <= intersectMinY || 
        intersectMaxZ <= intersectMinZ) {
        return 0.0f;  // No intersection
    }
    
    // Calculate volume
    float width = intersectMaxX - intersectMinX;
    float height = intersectMaxY - intersectMinY;
    float depth = intersectMaxZ - intersectMinZ;
    
    return width * height * depth;
}

bool VoxelCollision::checkStability(const IncrementCoordinates& pos,
                                   VoxelData::VoxelResolution resolution,
                                   const VoxelData::VoxelGrid& grid) {
    // Check if voxel is on ground plane
    if (pos.y() == 0) {
        return true;  // On ground, stable
    }
    
    // Check if there's support underneath
    IncrementCoordinates belowPos = VoxelGrid::getAdjacentPosition(
        pos, VoxelData::FaceDirection::NegY, resolution);
    
    // Check if there's any voxel below that could provide support
    auto allVoxels = grid.getAllVoxels();
    VoxelBounds ourBounds(pos, VoxelGrid::getVoxelSizeMeters(resolution));
    
    for (const auto& voxelData : allVoxels) {
        // Check if this voxel is below us
        if (voxelData.incrementPos.y() < pos.y()) {
            VoxelBounds theirBounds(voxelData.incrementPos, 
                                   VoxelGrid::getVoxelSizeMeters(voxelData.resolution));
            
            // Check if their top face overlaps with our bottom face
            // Simplified check - in practice you'd want more precise support calculation
            if (std::abs(theirBounds.max().value().y - ourBounds.min().value().y) < 0.001f) {
                // Check X-Z overlap
                const Vector3f& theirMin = theirBounds.min().value();
                const Vector3f& theirMax = theirBounds.max().value();
                const Vector3f& ourMin = ourBounds.min().value();
                const Vector3f& ourMax = ourBounds.max().value();
                
                bool xOverlap = !(theirMax.x < ourMin.x || theirMin.x > ourMax.x);
                bool zOverlap = !(theirMax.z < ourMin.z || theirMin.z > ourMax.z);
                
                if (xOverlap && zOverlap) {
                    return true;  // Has support
                }
            }
        }
    }
    
    return false;  // No support found
}

bool VoxelCollision::pointInBounds(const WorldCoordinates& point, const VoxelBounds& bounds) {
    return bounds.contains(point);
}

std::vector<IncrementCoordinates> VoxelCollision::getPotentialCollisionCandidates(
    const VoxelBounds& searchBounds,
    const VoxelData::VoxelGrid& grid) {
    
    std::vector<IncrementCoordinates> candidates;
    
    // This is a simplified implementation
    // In practice, you'd want to use the grid's spatial indexing
    // to efficiently find candidates in the search region
    
    // For now, just get all voxels and filter by bounds
    auto allVoxels = grid.getAllVoxels();
    
    for (const auto& voxelData : allVoxels) {
        VoxelBounds voxelBounds(voxelData.incrementPos, 
                               VoxelGrid::getVoxelSizeMeters(voxelData.resolution));
        
        if (boundsOverlap(voxelBounds, searchBounds)) {
            candidates.push_back(voxelData.incrementPos);
        }
    }
    
    return candidates;
}

} // namespace Math
} // namespace VoxelEditor