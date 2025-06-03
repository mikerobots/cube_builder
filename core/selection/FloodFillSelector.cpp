#include "FloodFillSelector.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>

namespace VoxelEditor {
namespace Selection {

FloodFillSelector::FloodFillSelector(VoxelData::VoxelDataManager* voxelManager)
    : m_voxelManager(voxelManager)
    , m_maxVoxels(1000000)
    , m_diagonalConnectivity(false)
    , m_connectivityMode(ConnectivityMode::Face6) {
}

SelectionSet FloodFillSelector::selectFloodFill(const VoxelId& seed, FloodFillCriteria criteria) {
    if (!voxelExists(seed)) {
        return SelectionSet();
    }
    
    auto canVisit = [this, criteria](const VoxelId& current, const VoxelId& neighbor) {
        return meetsFloodFillCriteria(current, neighbor, criteria);
    };
    
    auto shouldStop = [this](const VoxelId& voxel) {
        return false; // No early stopping
    };
    
    return floodFillInternal(seed, canVisit, shouldStop);
}

SelectionSet FloodFillSelector::selectFloodFillCustom(const VoxelId& seed,
                                                    const SelectionPredicate& predicate) {
    if (!voxelExists(seed) || !predicate(seed)) {
        return SelectionSet();
    }
    
    auto canVisit = [this, &predicate](const VoxelId& current, const VoxelId& neighbor) {
        return voxelExists(neighbor) && predicate(neighbor);
    };
    
    auto shouldStop = [](const VoxelId& voxel) {
        return false;
    };
    
    return floodFillInternal(seed, canVisit, shouldStop);
}

SelectionSet FloodFillSelector::selectFloodFillLimited(const VoxelId& seed,
                                                     FloodFillCriteria criteria,
                                                     int maxSteps) {
    if (!voxelExists(seed)) {
        return SelectionSet();
    }
    
    SelectionSet result;
    std::queue<std::pair<VoxelId, int>> toVisit;
    std::unordered_set<VoxelId> visited;
    
    toVisit.push({seed, 0});
    visited.insert(seed);
    
    while (!toVisit.empty() && result.size() < m_maxVoxels) {
        auto [current, steps] = toVisit.front();
        toVisit.pop();
        
        result.add(current);
        
        if (steps >= maxSteps) {
            continue;
        }
        
        std::vector<VoxelId> neighbors = getNeighbors(current);
        for (const auto& neighbor : neighbors) {
            if (visited.find(neighbor) == visited.end() && 
                voxelExists(neighbor) &&
                meetsFloodFillCriteria(current, neighbor, criteria)) {
                visited.insert(neighbor);
                toVisit.push({neighbor, steps + 1});
            }
        }
    }
    
    return result;
}

SelectionSet FloodFillSelector::selectFloodFillBounded(const VoxelId& seed,
                                                     FloodFillCriteria criteria,
                                                     const Math::BoundingBox& bounds) {
    if (!voxelExists(seed) || !bounds.contains(seed.getWorldPosition())) {
        return SelectionSet();
    }
    
    auto canVisit = [this, criteria, &bounds](const VoxelId& current, const VoxelId& neighbor) {
        return bounds.contains(neighbor.getWorldPosition()) &&
               meetsFloodFillCriteria(current, neighbor, criteria);
    };
    
    auto shouldStop = [](const VoxelId& voxel) {
        return false;
    };
    
    return floodFillInternal(seed, canVisit, shouldStop);
}

SelectionSet FloodFillSelector::selectPlanarFloodFill(const VoxelId& seed,
                                                    const Math::Vector3f& planeNormal,
                                                    float planeTolerance) {
    if (!voxelExists(seed)) {
        return SelectionSet();
    }
    
    Math::Vector3f normalizedNormal = planeNormal.normalized();
    Math::Vector3f seedPos = seed.getWorldPosition();
    float planeD = -normalizedNormal.dot(seedPos);
    
    auto canVisit = [this, &normalizedNormal, planeD, planeTolerance](const VoxelId& current, const VoxelId& neighbor) {
        if (!voxelExists(neighbor)) return false;
        
        // Check if neighbor is on the same plane
        Math::Vector3f neighborPos = neighbor.getWorldPosition();
        float distance = std::abs(normalizedNormal.dot(neighborPos) + planeD);
        return distance <= planeTolerance;
    };
    
    auto shouldStop = [](const VoxelId& voxel) {
        return false;
    };
    
    return floodFillInternal(seed, canVisit, shouldStop);
}

std::vector<VoxelId> FloodFillSelector::getNeighbors(const VoxelId& voxel) const {
    std::vector<VoxelId> neighbors;
    
    // Face neighbors (6-connectivity)
    neighbors.emplace_back(Math::Vector3i(voxel.position.x + 1, voxel.position.y, voxel.position.z), voxel.resolution);
    neighbors.emplace_back(Math::Vector3i(voxel.position.x - 1, voxel.position.y, voxel.position.z), voxel.resolution);
    neighbors.emplace_back(Math::Vector3i(voxel.position.x, voxel.position.y + 1, voxel.position.z), voxel.resolution);
    neighbors.emplace_back(Math::Vector3i(voxel.position.x, voxel.position.y - 1, voxel.position.z), voxel.resolution);
    neighbors.emplace_back(Math::Vector3i(voxel.position.x, voxel.position.y, voxel.position.z + 1), voxel.resolution);
    neighbors.emplace_back(Math::Vector3i(voxel.position.x, voxel.position.y, voxel.position.z - 1), voxel.resolution);
    
    if (m_connectivityMode == ConnectivityMode::Edge18 || m_connectivityMode == ConnectivityMode::Vertex26) {
        // Edge neighbors (12 additional for 18-connectivity)
        for (int dx = -1; dx <= 1; dx += 2) {
            for (int dy = -1; dy <= 1; dy += 2) {
                neighbors.emplace_back(Math::Vector3i(voxel.position.x + dx, voxel.position.y + dy, voxel.position.z), voxel.resolution);
                neighbors.emplace_back(Math::Vector3i(voxel.position.x + dx, voxel.position.y, voxel.position.z + dy), voxel.resolution);
                neighbors.emplace_back(Math::Vector3i(voxel.position.x, voxel.position.y + dy, voxel.position.z + dx), voxel.resolution);
            }
        }
    }
    
    if (m_connectivityMode == ConnectivityMode::Vertex26) {
        // Vertex neighbors (8 additional for 26-connectivity)
        for (int dx = -1; dx <= 1; dx += 2) {
            for (int dy = -1; dy <= 1; dy += 2) {
                for (int dz = -1; dz <= 1; dz += 2) {
                    neighbors.emplace_back(Math::Vector3i(voxel.position.x + dx, voxel.position.y + dy, voxel.position.z + dz), voxel.resolution);
                }
            }
        }
    }
    
    return neighbors;
}

bool FloodFillSelector::meetsFloodFillCriteria(const VoxelId& current, const VoxelId& neighbor, 
                                              FloodFillCriteria criteria) const {
    switch (criteria) {
        case FloodFillCriteria::Connected6:
        case FloodFillCriteria::Connected18:
        case FloodFillCriteria::Connected26:
            return areVoxelsConnected(current, neighbor);
            
        case FloodFillCriteria::SameResolution:
            return current.resolution == neighbor.resolution;
            
        case FloodFillCriteria::ConnectedSameRes:
            return areVoxelsConnected(current, neighbor) && 
                   current.resolution == neighbor.resolution;
            
        default:
            return false;
    }
}

bool FloodFillSelector::voxelExists(const VoxelId& voxel) const {
    if (!m_voxelManager) return true; // For testing: assume all voxels exist when no manager
    
    return m_voxelManager->hasVoxel(voxel.position, voxel.resolution);
}

bool FloodFillSelector::areVoxelsConnected(const VoxelId& voxel1, const VoxelId& voxel2) const {
    // Check if voxels are adjacent
    Math::Vector3i diff = voxel2.position - voxel1.position;
    int manhattan = std::abs(diff.x) + std::abs(diff.y) + std::abs(diff.z);
    
    switch (m_connectivityMode) {
        case ConnectivityMode::Face6:
            return manhattan == 1;
            
        case ConnectivityMode::Edge18:
            return manhattan <= 2 && manhattan > 0;
            
        case ConnectivityMode::Vertex26:
            return manhattan <= 3 && manhattan > 0;
            
        default:
            return false;
    }
}

SelectionSet FloodFillSelector::floodFillInternal(const VoxelId& seed,
                                                 const std::function<bool(const VoxelId&, const VoxelId&)>& canVisit,
                                                 const std::function<bool(const VoxelId&)>& shouldStop) {
    SelectionSet result;
    std::queue<VoxelId> toVisit;
    std::unordered_set<VoxelId> visited;
    
    toVisit.push(seed);
    visited.insert(seed);
    
    while (!toVisit.empty() && result.size() < m_maxVoxels) {
        VoxelId current = toVisit.front();
        toVisit.pop();
        
        result.add(current);
        
        if (shouldStop(current)) {
            break;
        }
        
        std::vector<VoxelId> neighbors = getNeighbors(current);
        for (const auto& neighbor : neighbors) {
            if (visited.find(neighbor) == visited.end() && 
                voxelExists(neighbor) &&
                canVisit(current, neighbor)) {
                visited.insert(neighbor);
                toVisit.push(neighbor);
            }
        }
    }
    
    if (result.size() >= m_maxVoxels) {
        Logging::Logger::getInstance().warning("FloodFillSelector: Reached maximum voxel limit");
    }
    
    return result;
}

}
}