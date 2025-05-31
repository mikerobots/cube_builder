#include <gtest/gtest.h>
#include "../EventDispatcher.h"
#include "../CommonEvents.h"
#include <thread>
#include <chrono>

using namespace VoxelEditor::Events;
using namespace VoxelEditor::Math;

class TestEvent : public Event<TestEvent> {
public:
    TestEvent(int value) : value(value) {}
    int value;
};

class TestHandler : public EventHandler<TestEvent> {
public:
    TestHandler(int priority = 0) : m_priority(priority), m_callCount(0), m_lastValue(-1) {}
    
    void handleEvent(const TestEvent& event) override {
        m_callCount++;
        m_lastValue = event.value;
        m_receivedValues.push_back(event.value);
    }
    
    bool shouldHandle(const TestEvent& event) const override {
        return m_shouldHandle;
    }
    
    int getPriority() const override {
        return m_priority;
    }
    
    int getCallCount() const { return m_callCount; }
    int getLastValue() const { return m_lastValue; }
    const std::vector<int>& getReceivedValues() const { return m_receivedValues; }
    void setShouldHandle(bool should) { m_shouldHandle = should; }
    void reset() { m_callCount = 0; m_lastValue = -1; m_receivedValues.clear(); }
    
private:
    int m_priority;
    int m_callCount;
    int m_lastValue;
    std::vector<int> m_receivedValues;
    bool m_shouldHandle = true;
};

class EventDispatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        dispatcher = std::make_unique<EventDispatcher>();
    }
    
    void TearDown() override {
        dispatcher.reset();
    }
    
    std::unique_ptr<EventDispatcher> dispatcher;
};

TEST_F(EventDispatcherTest, BasicEventDispatch) {
    TestHandler handler;
    dispatcher->subscribe<TestEvent>(&handler);
    
    TestEvent event(42);
    dispatcher->dispatch(event);
    
    EXPECT_EQ(handler.getCallCount(), 1);
    EXPECT_EQ(handler.getLastValue(), 42);
}

TEST_F(EventDispatcherTest, MultipleHandlers) {
    TestHandler handler1;
    TestHandler handler2;
    
    dispatcher->subscribe<TestEvent>(&handler1);
    dispatcher->subscribe<TestEvent>(&handler2);
    
    TestEvent event(100);
    dispatcher->dispatch(event);
    
    EXPECT_EQ(handler1.getCallCount(), 1);
    EXPECT_EQ(handler1.getLastValue(), 100);
    EXPECT_EQ(handler2.getCallCount(), 1);
    EXPECT_EQ(handler2.getLastValue(), 100);
}

TEST_F(EventDispatcherTest, HandlerPriority) {
    TestHandler lowPriority(1);
    TestHandler highPriority(10);
    TestHandler mediumPriority(5);
    
    dispatcher->subscribe<TestEvent>(&lowPriority, 1);
    dispatcher->subscribe<TestEvent>(&highPriority, 10);
    dispatcher->subscribe<TestEvent>(&mediumPriority, 5);
    
    TestEvent event1(1);
    TestEvent event2(2);
    TestEvent event3(3);
    
    dispatcher->dispatch(event1);
    dispatcher->dispatch(event2);
    dispatcher->dispatch(event3);
    
    EXPECT_EQ(highPriority.getReceivedValues(), std::vector<int>({1, 2, 3}));
    EXPECT_EQ(mediumPriority.getReceivedValues(), std::vector<int>({1, 2, 3}));
    EXPECT_EQ(lowPriority.getReceivedValues(), std::vector<int>({1, 2, 3}));
}

TEST_F(EventDispatcherTest, HandlerFiltering) {
    TestHandler handler;
    handler.setShouldHandle(false);
    
    dispatcher->subscribe<TestEvent>(&handler);
    
    TestEvent event(42);
    dispatcher->dispatch(event);
    
    EXPECT_EQ(handler.getCallCount(), 0);
}

TEST_F(EventDispatcherTest, Unsubscribe) {
    TestHandler handler;
    dispatcher->subscribe<TestEvent>(&handler);
    
    TestEvent event1(1);
    dispatcher->dispatch(event1);
    EXPECT_EQ(handler.getCallCount(), 1);
    
    dispatcher->unsubscribe<TestEvent>(&handler);
    
    TestEvent event2(2);
    dispatcher->dispatch(event2);
    EXPECT_EQ(handler.getCallCount(), 1); // Should not have increased
}

TEST_F(EventDispatcherTest, AsyncEventDispatch) {
    TestHandler handler;
    dispatcher->subscribe<TestEvent>(&handler);
    
    TestEvent event(42);
    dispatcher->dispatchAsync(event);
    
    EXPECT_EQ(handler.getCallCount(), 0);
    EXPECT_EQ(dispatcher->getQueueSize(), 1);
    
    dispatcher->processQueuedEvents();
    
    EXPECT_EQ(handler.getCallCount(), 1);
    EXPECT_EQ(handler.getLastValue(), 42);
    EXPECT_EQ(dispatcher->getQueueSize(), 0);
}

TEST_F(EventDispatcherTest, QueueSizeLimit) {
    dispatcher->setMaxQueueSize(2);
    
    TestEvent event1(1);
    TestEvent event2(2);
    TestEvent event3(3); // This should be dropped
    
    dispatcher->dispatchAsync(event1);
    dispatcher->dispatchAsync(event2);
    dispatcher->dispatchAsync(event3);
    
    EXPECT_EQ(dispatcher->getQueueSize(), 2);
}

TEST_F(EventDispatcherTest, ClearQueue) {
    TestEvent event1(1);
    TestEvent event2(2);
    
    dispatcher->dispatchAsync(event1);
    dispatcher->dispatchAsync(event2);
    
    EXPECT_EQ(dispatcher->getQueueSize(), 2);
    
    dispatcher->clearQueue();
    
    EXPECT_EQ(dispatcher->getQueueSize(), 0);
}

TEST_F(EventDispatcherTest, HandlerCount) {
    TestHandler handler1;
    TestHandler handler2;
    
    EXPECT_EQ(dispatcher->getHandlerCount<TestEvent>(), 0);
    EXPECT_EQ(dispatcher->getTotalHandlerCount(), 0);
    
    dispatcher->subscribe<TestEvent>(&handler1);
    EXPECT_EQ(dispatcher->getHandlerCount<TestEvent>(), 1);
    EXPECT_EQ(dispatcher->getTotalHandlerCount(), 1);
    
    dispatcher->subscribe<TestEvent>(&handler2);
    EXPECT_EQ(dispatcher->getHandlerCount<TestEvent>(), 2);
    EXPECT_EQ(dispatcher->getTotalHandlerCount(), 2);
    
    dispatcher->unsubscribe<TestEvent>(&handler1);
    EXPECT_EQ(dispatcher->getHandlerCount<TestEvent>(), 1);
    EXPECT_EQ(dispatcher->getTotalHandlerCount(), 1);
}

TEST_F(EventDispatcherTest, CommonVoxelChangedEvent) {
    class VoxelHandler : public EventHandler<VoxelChangedEvent> {
    public:
        void handleEvent(const VoxelChangedEvent& event) override {
            position = event.position;
            resolution = event.resolution;
            oldValue = event.oldValue;
            newValue = event.newValue;
            callCount++;
        }
        
        Vector3i position = Vector3i::Zero();
        VoxelResolution resolution = VoxelResolution::Size_1cm;
        bool oldValue = false;
        bool newValue = false;
        int callCount = 0;
    };
    
    VoxelHandler handler;
    dispatcher->subscribe<VoxelChangedEvent>(&handler);
    
    VoxelChangedEvent event(Vector3i(1, 2, 3), VoxelResolution::Size_4cm, false, true);
    dispatcher->dispatch(event);
    
    EXPECT_EQ(handler.callCount, 1);
    EXPECT_EQ(handler.position, Vector3i(1, 2, 3));
    EXPECT_EQ(handler.resolution, VoxelResolution::Size_4cm);
    EXPECT_EQ(handler.oldValue, false);
    EXPECT_EQ(handler.newValue, true);
}

TEST_F(EventDispatcherTest, EventTimestamp) {
    auto beforeTime = std::chrono::high_resolution_clock::now();
    TestEvent event(42);
    auto afterTime = std::chrono::high_resolution_clock::now();
    
    auto timestamp = event.getTimestamp();
    EXPECT_GE(timestamp, beforeTime);
    EXPECT_LE(timestamp, afterTime);
}

TEST_F(EventDispatcherTest, EventId) {
    TestEvent event1(1);
    TestEvent event2(2);
    
    EXPECT_NE(event1.getEventId(), event2.getEventId());
    EXPECT_GT(event2.getEventId(), event1.getEventId());
}

TEST_F(EventDispatcherTest, ThreadSafety) {
    TestHandler handler;
    dispatcher->subscribe<TestEvent>(&handler);
    
    const int numThreads = 4;
    const int eventsPerThread = 100;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t, eventsPerThread]() {
            for (int i = 0; i < eventsPerThread; ++i) {
                TestEvent event(t * eventsPerThread + i);
                dispatcher->dispatch(event);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(handler.getCallCount(), numThreads * eventsPerThread);
}