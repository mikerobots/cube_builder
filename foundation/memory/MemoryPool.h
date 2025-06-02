#pragma once

#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <new>

namespace VoxelEditor {
namespace Memory {

class MemoryPool {
public:
    MemoryPool(size_t objectSize, size_t initialCapacity = 64)
        : m_objectSize(objectSize)
        , m_initialBlockSize(initialCapacity)
        , m_blockSize(initialCapacity)
        , m_totalCapacity(0)
        , m_usedCount(0) {
        assert(objectSize > 0);
        assert(initialCapacity > 0);
        allocateNewBlock();
    }
    
    ~MemoryPool() {
        clear();
    }
    
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    
    void* allocate() {
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        
        if (m_freeList.empty()) {
            allocateNewBlock();
        }
        
        void* ptr = m_freeList.top();
        m_freeList.pop();
        m_usedCount++;
        
        return ptr;
    }
    
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        
        if (!isValidPointer(ptr)) {
            return;
        }
        
        m_freeList.push(ptr);
        m_usedCount--;
    }
    
    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        static_assert(sizeof(T) <= sizeof(typename ::std::aligned_storage<sizeof(T), alignof(T)>::type));
        
        void* ptr = allocate();
        if (!ptr) return nullptr;
        
        try {
            return new(ptr) T(::std::forward<Args>(args)...);
        } catch (...) {
            deallocate(ptr);
            throw;
        }
    }
    
    template<typename T>
    void destroy(T* ptr) {
        if (!ptr) return;
        
        ptr->~T();
        deallocate(ptr);
    }
    
    size_t getObjectSize() const {
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        return m_objectSize;
    }
    
    size_t getCapacity() const {
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        return m_totalCapacity;
    }
    
    size_t getUsedCount() const {
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        return m_usedCount;
    }
    
    size_t getFreeCount() const {
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        return m_totalCapacity - m_usedCount;
    }
    
    size_t getMemoryUsage() const {
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        return m_blocks.size() * m_blockSize * m_objectSize;
    }
    
    float getUtilization() const {
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        return m_totalCapacity > 0 ? static_cast<float>(m_usedCount) / m_totalCapacity : 0.0f;
    }
    
    void clear() {
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        
        for (auto& block : m_blocks) {
            ::std::free(block.memory);
        }
        m_blocks.clear();
        
        while (!m_freeList.empty()) {
            m_freeList.pop();
        }
        
        m_totalCapacity = 0;
        m_usedCount = 0;
    }
    
    void shrink() {
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        
        // Only shrink if no objects are currently used
        if (m_usedCount == 0) {
            // Reset block size to initial value for future allocations
            m_blockSize = m_initialBlockSize;
        }
    }
    
    void reserve(size_t capacity) {
        ::std::lock_guard<::std::mutex> lock(m_mutex);
        
        while (m_totalCapacity < capacity) {
            allocateNewBlock();
        }
    }
    
private:
    struct Block {
        void* memory;
        size_t size;
        
        Block(void* mem, size_t sz) : memory(mem), size(sz) {}
    };
    
    void allocateNewBlock() {
        size_t blockMemorySize = m_blockSize * m_objectSize;
        
        // Ensure alignment is power of 2 and block size is multiple of alignment
        size_t alignment = alignof(::std::max_align_t);
        blockMemorySize = (blockMemorySize + alignment - 1) & ~(alignment - 1);
        
        void* memory = ::std::aligned_alloc(alignment, blockMemorySize);
        
        if (!memory) {
            throw ::std::bad_alloc();
        }
        
        m_blocks.emplace_back(memory, m_blockSize);
        
        char* ptr = static_cast<char*>(memory);
        for (size_t i = 0; i < m_blockSize; ++i) {
            m_freeList.push(ptr + i * m_objectSize);
        }
        
        m_totalCapacity += m_blockSize;
        
        if (m_blocks.size() > 1) {
            m_blockSize = ::std::min(m_blockSize * 2, size_t(1024));
        }
    }
    
    bool isValidPointer(void* ptr) const {
        char* charPtr = static_cast<char*>(ptr);
        
        for (const auto& block : m_blocks) {
            char* blockStart = static_cast<char*>(block.memory);
            char* blockEnd = blockStart + block.size * m_objectSize;
            
            if (charPtr >= blockStart && charPtr < blockEnd) {
                size_t offset = charPtr - blockStart;
                return (offset % m_objectSize) == 0;
            }
        }
        
        return false;
    }
    
    size_t m_objectSize;
    size_t m_initialBlockSize;
    size_t m_blockSize;
    size_t m_totalCapacity;
    size_t m_usedCount;
    
    ::std::vector<Block> m_blocks;
    ::std::stack<void*> m_freeList;
    mutable ::std::mutex m_mutex;
};

template<typename T>
class TypedMemoryPool {
public:
    TypedMemoryPool(size_t initialCapacity = 64)
        : m_pool(sizeof(T), initialCapacity) {}
    
    template<typename... Args>
    T* construct(Args&&... args) {
        return m_pool.template construct<T>(::std::forward<Args>(args)...);
    }
    
    void destroy(T* ptr) {
        m_pool.template destroy<T>(ptr);
    }
    
    size_t getCapacity() const { return m_pool.getCapacity(); }
    size_t getUsedCount() const { return m_pool.getUsedCount(); }
    size_t getFreeCount() const { return m_pool.getFreeCount(); }
    size_t getMemoryUsage() const { return m_pool.getMemoryUsage(); }
    float getUtilization() const { return m_pool.getUtilization(); }
    
    void clear() { m_pool.clear(); }
    void shrink() { m_pool.shrink(); }
    void reserve(size_t capacity) { m_pool.reserve(capacity); }
    
private:
    MemoryPool m_pool;
};

}
}