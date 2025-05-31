#pragma once

#include "../events/EventDispatcher.h"
#include "../events/CommonEvents.h"
#include <unordered_map>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <vector>

namespace VoxelEditor {
namespace Memory {

struct AllocationInfo {
    void* ptr;
    size_t size;
    std::string category;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    AllocationInfo(void* p, size_t s, const std::string& cat)
        : ptr(p), size(s), category(cat)
        , timestamp(std::chrono::high_resolution_clock::now()) {}
};

class MemoryTracker {
public:
    static MemoryTracker& getInstance() {
        static MemoryTracker instance;
        return instance;
    }
    
    void recordAllocation(void* ptr, size_t size, const char* category = "Unknown") {
        if (!ptr || size == 0) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_allocations.emplace(ptr, AllocationInfo(ptr, size, category));
        
        m_totalAllocated.fetch_add(size, std::memory_order_relaxed);
        size_t currentUsage = m_currentUsage.fetch_add(size, std::memory_order_relaxed) + size;
        
        if (currentUsage > m_peakUsage.load(std::memory_order_relaxed)) {
            m_peakUsage.store(currentUsage, std::memory_order_relaxed);
        }
        
        m_categoryUsage[category] += size;
        
        checkMemoryPressure(currentUsage);
    }
    
    void recordDeallocation(void* ptr) {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_allocations.find(ptr);
        if (it != m_allocations.end()) {
            size_t size = it->second.size;
            const std::string& category = it->second.category;
            
            m_totalDeallocated.fetch_add(size, std::memory_order_relaxed);
            m_currentUsage.fetch_sub(size, std::memory_order_relaxed);
            
            m_categoryUsage[category] -= size;
            if (m_categoryUsage[category] == 0) {
                m_categoryUsage.erase(category);
            }
            
            m_allocations.erase(it);
        }
    }
    
    size_t getTotalAllocated() const {
        return m_totalAllocated.load(std::memory_order_relaxed);
    }
    
    size_t getTotalDeallocated() const {
        return m_totalDeallocated.load(std::memory_order_relaxed);
    }
    
    size_t getCurrentUsage() const {
        return m_currentUsage.load(std::memory_order_relaxed);
    }
    
    size_t getPeakUsage() const {
        return m_peakUsage.load(std::memory_order_relaxed);
    }
    
    std::unordered_map<std::string, size_t> getUsageByCategory() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_categoryUsage;
    }
    
    std::vector<AllocationInfo> getActiveAllocations() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::vector<AllocationInfo> result;
        result.reserve(m_allocations.size());
        
        for (const auto& pair : m_allocations) {
            result.push_back(pair.second);
        }
        
        return result;
    }
    
    size_t getActiveAllocationCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_allocations.size();
    }
    
    void setMemoryLimit(size_t limit) {
        m_memoryLimit.store(limit, std::memory_order_relaxed);
    }
    
    size_t getMemoryLimit() const {
        return m_memoryLimit.load(std::memory_order_relaxed);
    }
    
    bool isMemoryPressure() const {
        size_t current = getCurrentUsage();
        size_t limit = getMemoryLimit();
        return limit > 0 && current > (limit * 90 / 100); // 90% threshold
    }
    
    float getMemoryPressureRatio() const {
        size_t current = getCurrentUsage();
        size_t limit = getMemoryLimit();
        return limit > 0 ? static_cast<float>(current) / limit : 0.0f;
    }
    
    void setEventDispatcher(Events::EventDispatcher* dispatcher) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_eventDispatcher = dispatcher;
    }
    
    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_allocations.clear();
        m_categoryUsage.clear();
        
        m_totalAllocated.store(0, std::memory_order_relaxed);
        m_totalDeallocated.store(0, std::memory_order_relaxed);
        m_currentUsage.store(0, std::memory_order_relaxed);
        m_peakUsage.store(0, std::memory_order_relaxed);
    }
    
    struct MemoryStats {
        size_t totalAllocated;
        size_t totalDeallocated;
        size_t currentUsage;
        size_t peakUsage;
        size_t activeAllocations;
        float pressureRatio;
        std::unordered_map<std::string, size_t> categoryUsage;
    };
    
    MemoryStats getStats() const {
        MemoryStats stats;
        stats.totalAllocated = getTotalAllocated();
        stats.totalDeallocated = getTotalDeallocated();
        stats.currentUsage = getCurrentUsage();
        stats.peakUsage = getPeakUsage();
        stats.activeAllocations = getActiveAllocationCount();
        stats.pressureRatio = getMemoryPressureRatio();
        stats.categoryUsage = getUsageByCategory();
        return stats;
    }
    
private:
    MemoryTracker() : m_memoryLimit(0), m_eventDispatcher(nullptr) {}
    
    void checkMemoryPressure(size_t currentUsage) {
        size_t limit = m_memoryLimit.load(std::memory_order_relaxed);
        if (limit > 0 && m_eventDispatcher && currentUsage > (limit * 90 / 100)) {
            Events::MemoryPressureEvent event(currentUsage, limit);
            m_eventDispatcher->dispatch(event);
        }
    }
    
    std::unordered_map<void*, AllocationInfo> m_allocations;
    std::unordered_map<std::string, size_t> m_categoryUsage;
    
    std::atomic<size_t> m_totalAllocated{0};
    std::atomic<size_t> m_totalDeallocated{0};
    std::atomic<size_t> m_currentUsage{0};
    std::atomic<size_t> m_peakUsage{0};
    std::atomic<size_t> m_memoryLimit{0};
    
    Events::EventDispatcher* m_eventDispatcher;
    mutable std::mutex m_mutex;
};

class ScopedAllocationTracker {
public:
    ScopedAllocationTracker(const char* category) 
        : m_category(category), m_startUsage(MemoryTracker::getInstance().getCurrentUsage()) {}
    
    ~ScopedAllocationTracker() {
        size_t endUsage = MemoryTracker::getInstance().getCurrentUsage();
        size_t allocated = endUsage > m_startUsage ? endUsage - m_startUsage : 0;
        // Could log or track scope-specific allocations here
    }
    
private:
    const char* m_category;
    size_t m_startUsage;
};

#define TRACK_MEMORY_SCOPE(category) ScopedAllocationTracker _tracker(category)

}
}