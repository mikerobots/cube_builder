#pragma once

#include <chrono>
#include <atomic>

namespace VoxelEditor {
namespace Events {

class EventBase {
public:
    virtual ~EventBase() = default;
    
    std::chrono::high_resolution_clock::time_point getTimestamp() const { 
        return m_timestamp; 
    }
    
    uint64_t getEventId() const { 
        return m_eventId; 
    }
    
    virtual const char* getEventType() const = 0;

protected:
    EventBase() 
        : m_timestamp(std::chrono::high_resolution_clock::now())
        , m_eventId(generateEventId()) {}
    
private:
    std::chrono::high_resolution_clock::time_point m_timestamp;
    uint64_t m_eventId;
    
    static uint64_t generateEventId() {
        static std::atomic<uint64_t> counter{1};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
};

template<typename Derived>
class Event : public EventBase {
public:
    const char* getEventType() const override {
        return typeid(Derived).name();
    }
    
    static const char* getStaticEventType() {
        return typeid(Derived).name();
    }
};

}
}