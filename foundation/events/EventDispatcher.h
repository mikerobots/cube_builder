#pragma once

#include "EventBase.h"
#include "EventHandler.h"
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <queue>
#include <mutex>
#include <algorithm>

namespace VoxelEditor {
namespace Events {

class EventDispatcher {
public:
    EventDispatcher() : m_maxQueueSize(1000) {}
    
    template<typename EventType>
    void subscribe(EventHandler<EventType>* handler, int priority = 0) {
        if (!handler) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::type_index typeIndex(typeid(EventType));
        auto& handlers = m_handlers[typeIndex];
        
        auto entry = std::make_unique<HandlerEntry<EventType>>(handler, priority);
        handlers.push_back(std::move(entry));
        
        std::sort(handlers.begin(), handlers.end(), 
                 [](const std::unique_ptr<HandlerEntryBase>& a, 
                    const std::unique_ptr<HandlerEntryBase>& b) {
                     return a->getPriority() > b->getPriority();
                 });
    }
    
    template<typename EventType>
    void unsubscribe(EventHandler<EventType>* handler) {
        if (!handler) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::type_index typeIndex(typeid(EventType));
        auto it = m_handlers.find(typeIndex);
        if (it == m_handlers.end()) return;
        
        auto& handlers = it->second;
        handlers.erase(
            std::remove_if(handlers.begin(), handlers.end(),
                          [handler](const std::unique_ptr<HandlerEntryBase>& entry) {
                              auto* typedEntry = dynamic_cast<HandlerEntry<EventType>*>(entry.get());
                              return typedEntry && typedEntry->getHandler() == handler;
                          }),
            handlers.end());
        
        if (handlers.empty()) {
            m_handlers.erase(it);
        }
    }
    
    template<typename EventType>
    void dispatch(const EventType& event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::type_index typeIndex(typeid(EventType));
        auto it = m_handlers.find(typeIndex);
        if (it == m_handlers.end()) return;
        
        for (const auto& entry : it->second) {
            if (entry->shouldHandle(event)) {
                entry->handleEvent(event);
            }
        }
    }
    
    template<typename EventType>
    void dispatchAsync(const EventType& event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_eventQueue.size() >= m_maxQueueSize) {
            return;
        }
        
        m_eventQueue.push(std::make_unique<EventType>(event));
    }
    
    void processQueuedEvents() {
        std::queue<std::unique_ptr<EventBase>> tempQueue;
        
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            tempQueue.swap(m_eventQueue);
        }
        
        while (!tempQueue.empty()) {
            auto event = std::move(tempQueue.front());
            tempQueue.pop();
            
            dispatchEvent(*event);
        }
    }
    
    void clearQueue() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_eventQueue.empty()) {
            m_eventQueue.pop();
        }
    }
    
    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_eventQueue.size();
    }
    
    void setMaxQueueSize(size_t maxSize) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxQueueSize = maxSize;
    }
    
    size_t getMaxQueueSize() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_maxQueueSize;
    }
    
    template<typename EventType>
    size_t getHandlerCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::type_index typeIndex(typeid(EventType));
        auto it = m_handlers.find(typeIndex);
        return it != m_handlers.end() ? it->second.size() : 0;
    }
    
    size_t getTotalHandlerCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        size_t total = 0;
        for (const auto& pair : m_handlers) {
            total += pair.second.size();
        }
        return total;
    }
    
private:
    void dispatchEvent(const EventBase& event) {
        std::type_index typeIndex(typeid(event));
        auto it = m_handlers.find(typeIndex);
        if (it == m_handlers.end()) return;
        
        for (const auto& entry : it->second) {
            if (entry->shouldHandle(event)) {
                entry->handleEvent(event);
            }
        }
    }
    
    std::unordered_map<std::type_index, std::vector<std::unique_ptr<HandlerEntryBase>>> m_handlers;
    std::queue<std::unique_ptr<EventBase>> m_eventQueue;
    mutable std::mutex m_mutex;
    size_t m_maxQueueSize;
};

}
}