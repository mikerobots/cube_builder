#pragma once

#include "MemoryTracker.h"
#include "MemoryPool.h"
#include "../events/EventHandler.h"
#include "../events/CommonEvents.h"
#include <functional>
#include <vector>
#include <memory>

namespace VoxelEditor {
namespace Memory {

enum class CleanupPriority {
    Low = 0,
    Medium = 1,
    High = 2,
    Critical = 3
};

class CleanupCallback {
public:
    CleanupCallback(std::function<size_t()> callback, CleanupPriority priority, const std::string& name)
        : m_callback(callback), m_priority(priority), m_name(name) {}
    
    size_t execute() const {
        return m_callback ? m_callback() : 0;
    }
    
    CleanupPriority getPriority() const { return m_priority; }
    const std::string& getName() const { return m_name; }
    
private:
    std::function<size_t()> m_callback;
    CleanupPriority m_priority;
    std::string m_name;
};

class MemoryOptimizer : public Events::EventHandler<Events::MemoryPressureEvent> {
public:
    MemoryOptimizer() : m_enabled(true), m_aggressiveMode(false) {}
    
    void handleEvent(const Events::MemoryPressureEvent& event) override {
        if (!m_enabled) return;
        
        float pressureRatio = static_cast<float>(event.currentUsage) / event.maxUsage;
        
        if (pressureRatio > 0.95f) {
            performCleanup(CleanupPriority::Critical);
        } else if (pressureRatio > 0.90f) {
            performCleanup(CleanupPriority::High);
        } else if (pressureRatio > 0.80f) {
            performCleanup(CleanupPriority::Medium);
        }
    }
    
    void registerCleanupCallback(std::function<size_t()> callback, CleanupPriority priority, const std::string& name) {
        m_cleanupCallbacks.emplace_back(callback, priority, name);
        
        std::sort(m_cleanupCallbacks.begin(), m_cleanupCallbacks.end(),
                 [](const CleanupCallback& a, const CleanupCallback& b) {
                     return static_cast<int>(a.getPriority()) > static_cast<int>(b.getPriority());
                 });
    }
    
    size_t performCleanup(CleanupPriority minPriority = CleanupPriority::Low) {
        size_t totalFreed = 0;
        
        for (const auto& callback : m_cleanupCallbacks) {
            if (static_cast<int>(callback.getPriority()) >= static_cast<int>(minPriority)) {
                size_t freed = callback.execute();
                totalFreed += freed;
                
                if (freed > 0) {
                    // Log cleanup action
                }
                
                if (!m_aggressiveMode && totalFreed > 0) {
                    break;
                }
            }
        }
        
        return totalFreed;
    }
    
    void setEnabled(bool enabled) {
        m_enabled = enabled;
    }
    
    bool isEnabled() const {
        return m_enabled;
    }
    
    void setAggressiveMode(bool aggressive) {
        m_aggressiveMode = aggressive;
    }
    
    bool isAggressiveMode() const {
        return m_aggressiveMode;
    }
    
    void forceCleanup() {
        performCleanup(CleanupPriority::Low);
    }
    
    size_t getCallbackCount() const {
        return m_cleanupCallbacks.size();
    }
    
    std::vector<std::string> getCallbackNames() const {
        std::vector<std::string> names;
        names.reserve(m_cleanupCallbacks.size());
        
        for (const auto& callback : m_cleanupCallbacks) {
            names.push_back(callback.getName());
        }
        
        return names;
    }
    
private:
    std::vector<CleanupCallback> m_cleanupCallbacks;
    bool m_enabled;
    bool m_aggressiveMode;
};

class MemoryManager {
public:
    static MemoryManager& getInstance() {
        static MemoryManager instance;
        return instance;
    }
    
    void initialize(Events::EventDispatcher* eventDispatcher) {
        m_tracker.setEventDispatcher(eventDispatcher);
        eventDispatcher->subscribe<Events::MemoryPressureEvent>(&m_optimizer);
    }
    
    MemoryTracker& getTracker() { return m_tracker; }
    MemoryOptimizer& getOptimizer() { return m_optimizer; }
    
    void setMemoryLimit(size_t limitBytes) {
        m_tracker.setMemoryLimit(limitBytes);
    }
    
    void registerCleanupCallback(std::function<size_t()> callback, CleanupPriority priority, const std::string& name) {
        m_optimizer.registerCleanupCallback(callback, priority, name);
    }
    
    MemoryTracker::MemoryStats getStats() const {
        return m_tracker.getStats();
    }
    
    bool isMemoryPressure() const {
        return m_tracker.isMemoryPressure();
    }
    
    size_t performCleanup(CleanupPriority minPriority = CleanupPriority::Low) {
        return m_optimizer.performCleanup(minPriority);
    }
    
private:
    MemoryManager() = default;
    
    MemoryTracker& m_tracker = MemoryTracker::getInstance();
    MemoryOptimizer m_optimizer;
};

template<typename T>
class ManagedMemoryPool {
public:
    ManagedMemoryPool(const std::string& name, size_t initialCapacity = 64)
        : m_pool(initialCapacity), m_name(name) {
        
        MemoryManager::getInstance().registerCleanupCallback(
            [this]() -> size_t {
                size_t beforeUsage = m_pool.getMemoryUsage();
                m_pool.shrink();
                size_t afterUsage = m_pool.getMemoryUsage();
                return beforeUsage - afterUsage;
            },
            CleanupPriority::Low,
            "Pool_" + m_name
        );
    }
    
    template<typename... Args>
    T* construct(Args&&... args) {
        T* ptr = m_pool.construct(std::forward<Args>(args)...);
        if (ptr) {
            MemoryTracker::getInstance().recordAllocation(ptr, sizeof(T), m_name.c_str());
        }
        return ptr;
    }
    
    void destroy(T* ptr) {
        if (ptr) {
            MemoryTracker::getInstance().recordDeallocation(ptr);
            m_pool.destroy(ptr);
        }
    }
    
    size_t getCapacity() const { return m_pool.getCapacity(); }
    size_t getUsedCount() const { return m_pool.getUsedCount(); }
    size_t getFreeCount() const { return m_pool.getFreeCount(); }
    size_t getMemoryUsage() const { return m_pool.getMemoryUsage(); }
    float getUtilization() const { return m_pool.getUtilization(); }
    
private:
    TypedMemoryPool<T> m_pool;
    std::string m_name;
};

}
}