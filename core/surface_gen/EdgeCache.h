#pragma once

#include "SurfaceTypes.h"
#include "../../foundation/math/CoordinateTypes.h"
#include <unordered_map>
#include <mutex>

namespace VoxelEditor {
namespace SurfaceGen {

// Cache for edge intersections to avoid redundant calculations
class EdgeCache {
public:
    struct EdgeKey {
        Math::IncrementCoordinates v0;
        Math::IncrementCoordinates v1;
        
        bool operator==(const EdgeKey& other) const {
            return v0 == other.v0 && v1 == other.v1;
        }
        
        struct Hash {
            std::size_t operator()(const EdgeKey& key) const {
                // Combine hashes of both vertices
                std::size_t h1 = std::hash<int>{}(key.v0.x()) ^ 
                               (std::hash<int>{}(key.v0.y()) << 1) ^ 
                               (std::hash<int>{}(key.v0.z()) << 2);
                std::size_t h2 = std::hash<int>{}(key.v1.x()) ^ 
                               (std::hash<int>{}(key.v1.y()) << 1) ^ 
                               (std::hash<int>{}(key.v1.z()) << 2);
                return h1 ^ (h2 << 1);
            }
        };
    };
    
    // Check if edge intersection is cached
    bool get(const EdgeKey& key, HermiteData& data) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            data = it->second;
            m_hits++;
            return true;
        }
        m_misses++;
        return false;
    }
    
    // Store edge intersection in cache
    void put(const EdgeKey& key, const HermiteData& data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache[key] = data;
    }
    
    // Clear the cache
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache.clear();
        m_hits = 0;
        m_misses = 0;
    }
    
    // Get cache statistics
    void getStats(size_t& hits, size_t& misses) const {
        hits = m_hits;
        misses = m_misses;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.size();
    }
    
private:
    mutable std::mutex m_mutex;
    std::unordered_map<EdgeKey, HermiteData, EdgeKey::Hash> m_cache;
    mutable size_t m_hits = 0;
    mutable size_t m_misses = 0;
};

} // namespace SurfaceGen
} // namespace VoxelEditor