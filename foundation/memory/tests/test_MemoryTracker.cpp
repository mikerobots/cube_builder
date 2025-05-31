#include <gtest/gtest.h>
#include "../MemoryTracker.h"
#include "../MemoryOptimizer.h"
#include "../../events/EventDispatcher.h"
#include <thread>
#include <chrono>

using namespace VoxelEditor::Memory;
using namespace VoxelEditor::Events;

class MemoryTrackerTest : public ::testing::Test {
protected:
    void SetUp() override {
        MemoryTracker::getInstance().reset();
    }
    
    void TearDown() override {
        MemoryTracker::getInstance().reset();
    }
};

TEST_F(MemoryTrackerTest, BasicAllocationTracking) {
    MemoryTracker& tracker = MemoryTracker::getInstance();
    
    void* ptr = malloc(100);
    tracker.recordAllocation(ptr, 100, "Test");
    
    EXPECT_EQ(tracker.getTotalAllocated(), 100);
    EXPECT_EQ(tracker.getCurrentUsage(), 100);
    EXPECT_EQ(tracker.getPeakUsage(), 100);
    EXPECT_EQ(tracker.getActiveAllocationCount(), 1);
    
    tracker.recordDeallocation(ptr);
    
    EXPECT_EQ(tracker.getTotalAllocated(), 100);
    EXPECT_EQ(tracker.getTotalDeallocated(), 100);
    EXPECT_EQ(tracker.getCurrentUsage(), 0);
    EXPECT_EQ(tracker.getActiveAllocationCount(), 0);
    
    free(ptr);
}

TEST_F(MemoryTrackerTest, CategoryTracking) {
    MemoryTracker& tracker = MemoryTracker::getInstance();
    
    void* ptr1 = malloc(100);
    void* ptr2 = malloc(200);
    void* ptr3 = malloc(50);
    
    tracker.recordAllocation(ptr1, 100, "Voxels");
    tracker.recordAllocation(ptr2, 200, "Meshes");
    tracker.recordAllocation(ptr3, 50, "Voxels");
    
    auto categoryUsage = tracker.getUsageByCategory();
    
    EXPECT_EQ(categoryUsage["Voxels"], 150);
    EXPECT_EQ(categoryUsage["Meshes"], 200);
    EXPECT_EQ(tracker.getCurrentUsage(), 350);
    
    tracker.recordDeallocation(ptr1);
    
    categoryUsage = tracker.getUsageByCategory();
    EXPECT_EQ(categoryUsage["Voxels"], 50);
    EXPECT_EQ(categoryUsage["Meshes"], 200);
    
    tracker.recordDeallocation(ptr2);
    tracker.recordDeallocation(ptr3);
    
    free(ptr1);
    free(ptr2);
    free(ptr3);
}

TEST_F(MemoryTrackerTest, PeakUsageTracking) {
    MemoryTracker& tracker = MemoryTracker::getInstance();
    
    void* ptr1 = malloc(100);
    tracker.recordAllocation(ptr1, 100, "Test");
    EXPECT_EQ(tracker.getPeakUsage(), 100);
    
    void* ptr2 = malloc(200);
    tracker.recordAllocation(ptr2, 200, "Test");
    EXPECT_EQ(tracker.getPeakUsage(), 300);
    
    tracker.recordDeallocation(ptr1);
    EXPECT_EQ(tracker.getCurrentUsage(), 200);
    EXPECT_EQ(tracker.getPeakUsage(), 300); // Peak should remain
    
    void* ptr3 = malloc(250);
    tracker.recordAllocation(ptr3, 250, "Test");
    EXPECT_EQ(tracker.getPeakUsage(), 450); // New peak
    
    tracker.recordDeallocation(ptr2);
    tracker.recordDeallocation(ptr3);
    
    free(ptr1);
    free(ptr2);
    free(ptr3);
}

TEST_F(MemoryTrackerTest, MemoryPressureDetection) {
    MemoryTracker& tracker = MemoryTracker::getInstance();
    tracker.setMemoryLimit(1000);
    
    EXPECT_FALSE(tracker.isMemoryPressure());
    EXPECT_FLOAT_EQ(tracker.getMemoryPressureRatio(), 0.0f);
    
    void* ptr = malloc(950);
    tracker.recordAllocation(ptr, 950, "Test");
    
    EXPECT_TRUE(tracker.isMemoryPressure()); // > 90% of limit
    EXPECT_FLOAT_EQ(tracker.getMemoryPressureRatio(), 0.95f);
    
    tracker.recordDeallocation(ptr);
    free(ptr);
}

TEST_F(MemoryTrackerTest, ActiveAllocations) {
    MemoryTracker& tracker = MemoryTracker::getInstance();
    
    void* ptr1 = malloc(100);
    void* ptr2 = malloc(200);
    
    tracker.recordAllocation(ptr1, 100, "Category1");
    tracker.recordAllocation(ptr2, 200, "Category2");
    
    auto allocations = tracker.getActiveAllocations();
    EXPECT_EQ(allocations.size(), 2);
    
    bool found1 = false, found2 = false;
    for (const auto& alloc : allocations) {
        if (alloc.ptr == ptr1) {
            EXPECT_EQ(alloc.size, 100);
            EXPECT_EQ(alloc.category, "Category1");
            found1 = true;
        } else if (alloc.ptr == ptr2) {
            EXPECT_EQ(alloc.size, 200);
            EXPECT_EQ(alloc.category, "Category2");
            found2 = true;
        }
    }
    
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
    
    tracker.recordDeallocation(ptr1);
    tracker.recordDeallocation(ptr2);
    
    free(ptr1);
    free(ptr2);
}

TEST_F(MemoryTrackerTest, MemoryStats) {
    MemoryTracker& tracker = MemoryTracker::getInstance();
    
    void* ptr1 = malloc(100);
    void* ptr2 = malloc(200);
    
    tracker.recordAllocation(ptr1, 100, "Test1");
    tracker.recordAllocation(ptr2, 200, "Test2");
    
    auto stats = tracker.getStats();
    
    EXPECT_EQ(stats.totalAllocated, 300);
    EXPECT_EQ(stats.currentUsage, 300);
    EXPECT_EQ(stats.peakUsage, 300);
    EXPECT_EQ(stats.activeAllocations, 2);
    EXPECT_EQ(stats.categoryUsage.size(), 2);
    
    tracker.recordDeallocation(ptr1);
    tracker.recordDeallocation(ptr2);
    
    free(ptr1);
    free(ptr2);
}

class MemoryOptimizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        MemoryTracker::getInstance().reset();
        dispatcher = std::make_unique<EventDispatcher>();
    }
    
    void TearDown() override {
        MemoryTracker::getInstance().reset();
        dispatcher.reset();
    }
    
    std::unique_ptr<EventDispatcher> dispatcher;
};

TEST_F(MemoryOptimizerTest, CleanupCallbacks) {
    MemoryOptimizer optimizer;
    
    int cleanupCalled = 0;
    size_t memoryFreed = 0;
    
    optimizer.registerCleanupCallback(
        [&cleanupCalled, &memoryFreed]() -> size_t {
            cleanupCalled++;
            memoryFreed = 100;
            return memoryFreed;
        },
        CleanupPriority::Medium,
        "TestCleanup"
    );
    
    EXPECT_EQ(optimizer.getCallbackCount(), 1);
    
    size_t freed = optimizer.performCleanup(CleanupPriority::Medium);
    
    EXPECT_EQ(cleanupCalled, 1);
    EXPECT_EQ(freed, 100);
}

TEST_F(MemoryOptimizerTest, CleanupPriority) {
    MemoryOptimizer optimizer;
    
    std::vector<int> callOrder;
    
    optimizer.registerCleanupCallback(
        [&callOrder]() -> size_t { callOrder.push_back(1); return 10; },
        CleanupPriority::Low, "Low"
    );
    
    optimizer.registerCleanupCallback(
        [&callOrder]() -> size_t { callOrder.push_back(2); return 20; },
        CleanupPriority::High, "High"
    );
    
    optimizer.registerCleanupCallback(
        [&callOrder]() -> size_t { callOrder.push_back(3); return 30; },
        CleanupPriority::Medium, "Medium"
    );
    
    optimizer.setAggressiveMode(true);
    size_t freed = optimizer.performCleanup(CleanupPriority::Low);
    
    EXPECT_EQ(freed, 60);
    EXPECT_EQ(callOrder.size(), 3);
    EXPECT_EQ(callOrder[0], 2); // High priority first
    EXPECT_EQ(callOrder[1], 3); // Medium priority second
    EXPECT_EQ(callOrder[2], 1); // Low priority last
}

TEST_F(MemoryOptimizerTest, MemoryPressureResponse) {
    MemoryOptimizer optimizer;
    dispatcher->subscribe<MemoryPressureEvent>(&optimizer);
    
    bool cleanupCalled = false;
    optimizer.registerCleanupCallback(
        [&cleanupCalled]() -> size_t {
            cleanupCalled = true;
            return 100;
        },
        CleanupPriority::High,
        "PressureCleanup"
    );
    
    MemoryPressureEvent event(950, 1000); // 95% usage
    dispatcher->dispatch(event);
    
    EXPECT_TRUE(cleanupCalled);
}

TEST_F(MemoryOptimizerTest, ManagedMemoryPool) {
    MemoryManager::getInstance().initialize(dispatcher.get());
    
    struct TestStruct {
        int value;
        char padding[64];
    };
    
    ManagedMemoryPool<TestStruct> pool("TestPool", 4);
    
    TestStruct* obj1 = pool.construct();
    TestStruct* obj2 = pool.construct();
    
    EXPECT_EQ(pool.getUsedCount(), 2);
    
    auto stats = MemoryManager::getInstance().getStats();
    EXPECT_GT(stats.currentUsage, 0);
    EXPECT_EQ(stats.categoryUsage["TestPool"], 2 * sizeof(TestStruct));
    
    pool.destroy(obj1);
    pool.destroy(obj2);
    
    stats = MemoryManager::getInstance().getStats();
    EXPECT_EQ(stats.categoryUsage.count("TestPool"), 0);
}

TEST_F(MemoryTrackerTest, ThreadSafety) {
    MemoryTracker& tracker = MemoryTracker::getInstance();
    
    const int numThreads = 4;
    const int allocationsPerThread = 100;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&tracker, t, allocationsPerThread]() {
            for (int i = 0; i < allocationsPerThread; ++i) {
                void* ptr = malloc(10);
                tracker.recordAllocation(ptr, 10, "ThreadTest");
                
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                
                tracker.recordDeallocation(ptr);
                free(ptr);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(tracker.getCurrentUsage(), 0);
    EXPECT_EQ(tracker.getTotalAllocated(), numThreads * allocationsPerThread * 10);
    EXPECT_EQ(tracker.getTotalDeallocated(), numThreads * allocationsPerThread * 10);
}