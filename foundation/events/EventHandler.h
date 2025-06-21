#pragma once

#include "EventBase.h"

namespace VoxelEditor {
namespace Events {

template<typename EventType>
class EventHandler {
public:
    virtual ~EventHandler() = default;
    
    virtual void handleEvent(const EventType& event) = 0;
    
    virtual bool shouldHandle(const EventType& event) const { 
        (void)event; // Suppress unused parameter warning
        return true; 
    }
    
    virtual int getPriority() const { 
        return 0; 
    }
};

class HandlerEntryBase {
public:
    virtual ~HandlerEntryBase() = default;
    virtual void handleEvent(const EventBase& event) = 0;
    virtual bool shouldHandle(const EventBase& event) const = 0;
    virtual int getPriority() const = 0;
};

template<typename EventType>
class HandlerEntry : public HandlerEntryBase {
public:
    HandlerEntry(EventHandler<EventType>* handler, int priority)
        : m_handler(handler), m_priority(priority) {}
    
    void handleEvent(const EventBase& event) override {
        const EventType* typedEvent = dynamic_cast<const EventType*>(&event);
        if (typedEvent && m_handler) {
            m_handler->handleEvent(*typedEvent);
        }
    }
    
    bool shouldHandle(const EventBase& event) const override {
        const EventType* typedEvent = dynamic_cast<const EventType*>(&event);
        if (typedEvent && m_handler) {
            return m_handler->shouldHandle(*typedEvent);
        }
        return false;
    }
    
    int getPriority() const override {
        return m_priority;
    }
    
    EventHandler<EventType>* getHandler() const {
        return m_handler;
    }
    
private:
    EventHandler<EventType>* m_handler;
    int m_priority;
};

}
}