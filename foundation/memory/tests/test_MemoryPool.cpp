#include <gtest/gtest.h>
#include "../MemoryPool.h"
#include <thread>
#include <vector>
#include <set>

using namespace VoxelEditor::Memory;

class MemoryPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MemoryPoolTest, BasicAllocation) {
    MemoryPool pool(sizeof(int), 4);
    
    EXPECT_EQ(pool.getObjectSize(), sizeof(int));
    EXPECT_EQ(pool.getCapacity(), 4);
    EXPECT_EQ(pool.getUsedCount(), 0);
    EXPECT_EQ(pool.getFreeCount(), 4);
}

TEST_F(MemoryPoolTest, AllocateAndDeallocate) {
    MemoryPool pool(sizeof(int), 2);
    
    void* ptr1 = pool.allocate();
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(pool.getUsedCount(), 1);
    EXPECT_EQ(pool.getFreeCount(), 1);
    
    void* ptr2 = pool.allocate();
    ASSERT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2);
    EXPECT_EQ(pool.getUsedCount(), 2);
    EXPECT_EQ(pool.getFreeCount(), 0);
    
    pool.deallocate(ptr1);
    EXPECT_EQ(pool.getUsedCount(), 1);
    EXPECT_EQ(pool.getFreeCount(), 1);
    
    pool.deallocate(ptr2);
    EXPECT_EQ(pool.getUsedCount(), 0);
    EXPECT_EQ(pool.getFreeCount(), 2);
}

TEST_F(MemoryPoolTest, AutoExpansion) {
    MemoryPool pool(sizeof(int), 2);
    
    void* ptr1 = pool.allocate();
    void* ptr2 = pool.allocate();
    
    EXPECT_EQ(pool.getCapacity(), 2);
    EXPECT_EQ(pool.getUsedCount(), 2);
    
    void* ptr3 = pool.allocate();
    ASSERT_NE(ptr3, nullptr);
    
    EXPECT_GT(pool.getCapacity(), 2);
    EXPECT_EQ(pool.getUsedCount(), 3);
    
    pool.deallocate(ptr1);
    pool.deallocate(ptr2);
    pool.deallocate(ptr3);
}

TEST_F(MemoryPoolTest, MemoryAlignment) {
    MemoryPool pool(sizeof(double), 4);
    
    void* ptr1 = pool.allocate();
    void* ptr2 = pool.allocate();
    
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % alignof(double), 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % alignof(double), 0);
    
    pool.deallocate(ptr1);
    pool.deallocate(ptr2);
}

TEST_F(MemoryPoolTest, InvalidPointerHandling) {
    MemoryPool pool(sizeof(int), 2);
    
    pool.deallocate(nullptr);
    
    int localVar = 42;
    pool.deallocate(&localVar);
    
    EXPECT_EQ(pool.getUsedCount(), 0);
}

TEST_F(MemoryPoolTest, Utilization) {
    MemoryPool pool(sizeof(int), 4);
    
    EXPECT_FLOAT_EQ(pool.getUtilization(), 0.0f);
    
    void* ptr1 = pool.allocate();
    EXPECT_FLOAT_EQ(pool.getUtilization(), 0.25f);
    
    void* ptr2 = pool.allocate();
    EXPECT_FLOAT_EQ(pool.getUtilization(), 0.5f);
    
    pool.deallocate(ptr1);
    EXPECT_FLOAT_EQ(pool.getUtilization(), 0.25f);
    
    pool.deallocate(ptr2);
    EXPECT_FLOAT_EQ(pool.getUtilization(), 0.0f);
}

TEST_F(MemoryPoolTest, Clear) {
    MemoryPool pool(sizeof(int), 2);
    
    void* ptr1 = pool.allocate();
    void* ptr2 = pool.allocate();
    
    EXPECT_EQ(pool.getUsedCount(), 2);
    EXPECT_GT(pool.getMemoryUsage(), 0);
    
    pool.clear();
    
    EXPECT_EQ(pool.getUsedCount(), 0);
    EXPECT_EQ(pool.getCapacity(), 0);
}

TEST_F(MemoryPoolTest, Shrink) {
    MemoryPool pool(sizeof(int), 2);
    
    // Force expansion by allocating beyond initial capacity
    void* ptr1 = pool.allocate();
    void* ptr2 = pool.allocate();
    void* ptr3 = pool.allocate(); // This should trigger expansion
    
    size_t expandedCapacity = pool.getCapacity();
    EXPECT_GT(expandedCapacity, 2); // Should have expanded
    
    // Deallocate all
    pool.deallocate(ptr1);
    pool.deallocate(ptr2);
    pool.deallocate(ptr3);
    
    // Shrink should reset block size for future allocations
    pool.shrink();
    
    EXPECT_EQ(pool.getUsedCount(), 0);
    // Capacity stays the same since we don't deallocate existing blocks
    EXPECT_EQ(pool.getCapacity(), expandedCapacity);
}

TEST_F(MemoryPoolTest, Reserve) {
    MemoryPool pool(sizeof(int), 2);
    
    pool.reserve(10);
    
    EXPECT_GE(pool.getCapacity(), 10);
    EXPECT_EQ(pool.getUsedCount(), 0);
}

class TestObject {
public:
    TestObject(int value = 0) : m_value(value), m_constructed(true) {
        s_constructCount++;
    }
    
    ~TestObject() {
        m_constructed = false;
        s_destructCount++;
    }
    
    int getValue() const { return m_value; }
    bool isConstructed() const { return m_constructed; }
    
    static void resetCounts() {
        s_constructCount = 0;
        s_destructCount = 0;
    }
    
    static int getConstructCount() { return s_constructCount; }
    static int getDestructCount() { return s_destructCount; }
    
private:
    int m_value;
    bool m_constructed;
    
    static int s_constructCount;
    static int s_destructCount;
};

int TestObject::s_constructCount = 0;
int TestObject::s_destructCount = 0;

TEST_F(MemoryPoolTest, ObjectConstruction) {
    TestObject::resetCounts();
    MemoryPool pool(sizeof(TestObject), 2);
    
    TestObject* obj1 = pool.construct<TestObject>(42);
    ASSERT_NE(obj1, nullptr);
    EXPECT_TRUE(obj1->isConstructed());
    EXPECT_EQ(obj1->getValue(), 42);
    EXPECT_EQ(TestObject::getConstructCount(), 1);
    
    TestObject* obj2 = pool.construct<TestObject>(100);
    ASSERT_NE(obj2, nullptr);
    EXPECT_EQ(obj2->getValue(), 100);
    EXPECT_EQ(TestObject::getConstructCount(), 2);
    
    pool.destroy(obj1);
    EXPECT_EQ(TestObject::getDestructCount(), 1);
    
    pool.destroy(obj2);
    EXPECT_EQ(TestObject::getDestructCount(), 2);
}

TEST_F(MemoryPoolTest, TypedMemoryPool) {
    TestObject::resetCounts();
    TypedMemoryPool<TestObject> pool(2);
    
    TestObject* obj1 = pool.construct(42);
    ASSERT_NE(obj1, nullptr);
    EXPECT_EQ(obj1->getValue(), 42);
    EXPECT_EQ(pool.getUsedCount(), 1);
    
    TestObject* obj2 = pool.construct(100);
    ASSERT_NE(obj2, nullptr);
    EXPECT_EQ(obj2->getValue(), 100);
    EXPECT_EQ(pool.getUsedCount(), 2);
    
    pool.destroy(obj1);
    EXPECT_EQ(pool.getUsedCount(), 1);
    
    pool.destroy(obj2);
    EXPECT_EQ(pool.getUsedCount(), 0);
    
    EXPECT_EQ(TestObject::getConstructCount(), 2);
    EXPECT_EQ(TestObject::getDestructCount(), 2);
}

TEST_F(MemoryPoolTest, ThreadSafety) {
    MemoryPool pool(sizeof(int), 16);
    const int numThreads = 4;
    const int allocationsPerThread = 100;
    
    std::vector<std::thread> threads;
    std::vector<std::vector<void*>> threadAllocations(numThreads);
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&pool, &threadAllocations, t, allocationsPerThread]() {
            for (int i = 0; i < allocationsPerThread; ++i) {
                void* ptr = pool.allocate();
                if (ptr) {
                    threadAllocations[t].push_back(ptr);
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    size_t totalAllocated = 0;
    std::set<void*> uniquePointers;
    
    for (const auto& allocations : threadAllocations) {
        totalAllocated += allocations.size();
        for (void* ptr : allocations) {
            EXPECT_TRUE(uniquePointers.insert(ptr).second) << "Duplicate pointer detected";
        }
    }
    
    EXPECT_EQ(pool.getUsedCount(), totalAllocated);
    
    for (const auto& allocations : threadAllocations) {
        for (void* ptr : allocations) {
            pool.deallocate(ptr);
        }
    }
    
    EXPECT_EQ(pool.getUsedCount(), 0);
}