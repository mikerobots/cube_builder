# Event System Foundation

## Purpose
Provides decoupled communication between components through a type-safe event dispatching system with priority handling and filtering.

## Key Components

### EventDispatcher
- Type-safe event registration and dispatch
- Priority-based event handling
- Event filtering and queuing
- Thread-safe event processing

### Event Types
- Strongly-typed event definitions
- Event inheritance hierarchy
- Event metadata and timestamps
- Serializable events

### EventHandler
- Abstract base for event handlers
- Automatic registration/deregistration
- Handler priority management
- Conditional event processing

## Interface Design

```cpp
class EventDispatcher {
public:
    template<typename EventType>
    void subscribe(EventHandler<EventType>* handler, int priority = 0);
    
    template<typename EventType>
    void unsubscribe(EventHandler<EventType>* handler);
    
    template<typename EventType>
    void dispatch(const EventType& event);
    
    template<typename EventType>
    void dispatchAsync(const EventType& event);
    
    void processQueuedEvents();
    void clearQueue();
    
    size_t getQueueSize() const;
    void setMaxQueueSize(size_t maxSize);
    
private:
    template<typename EventType>
    struct HandlerEntry {
        EventHandler<EventType>* handler;
        int priority;
        bool operator<(const HandlerEntry& other) const {
            return priority > other.priority; // Higher priority first
        }
    };
    
    std::unordered_map<std::type_index, std::vector<std::unique_ptr<HandlerEntryBase>>> m_handlers;
    std::queue<std::unique_ptr<EventBase>> m_eventQueue;
    std::mutex m_mutex;
    size_t m_maxQueueSize = 1000;
};

template<typename EventType>
class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual void handleEvent(const EventType& event) = 0;
    virtual bool shouldHandle(const EventType& event) const { return true; }
};

class EventBase {
public:
    virtual ~EventBase() = default;
    
    std::chrono::high_resolution_clock::time_point getTimestamp() const { return m_timestamp; }
    uint64_t getEventId() const { return m_eventId; }
    
protected:
    EventBase() : m_timestamp(std::chrono::high_resolution_clock::now()), m_eventId(generateEventId()) {}
    
private:
    std::chrono::high_resolution_clock::time_point m_timestamp;
    uint64_t m_eventId;
    
    static uint64_t generateEventId();
};
```

## Dependencies
- Standard C++ threading and type_traits
- No external dependencies