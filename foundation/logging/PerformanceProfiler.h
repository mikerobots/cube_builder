#pragma once

#include <string>
#include <chrono>
#include <unordered_map>
#include <stack>
#include <vector>
#include <mutex>
#include <fstream>
#include <iomanip>
#include <algorithm>

namespace VoxelEditor {
namespace Logging {

class PerformanceProfiler {
public:
    struct ProfileData {
        std::string name;
        double totalTime;
        double averageTime;
        uint64_t callCount;
        double minTime;
        double maxTime;
        
        ProfileData() 
            : totalTime(0.0), averageTime(0.0), callCount(0), 
              minTime(std::numeric_limits<double>::max()), maxTime(0.0) {}
    };
    
    static PerformanceProfiler& getInstance() {
        static PerformanceProfiler instance;
        return instance;
    }
    
    void beginSection(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto now = std::chrono::high_resolution_clock::now();
        m_sectionStack.push({name, now});
    }
    
    void endSection(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_sectionStack.empty()) {
            return; // No matching begin
        }
        
        auto top = m_sectionStack.top();
        if (top.name != name) {
            return; // Mismatched section names
        }
        
        m_sectionStack.pop();
        
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(now - top.startTime).count();
        
        auto& data = m_sections[name];
        data.name = name;
        data.totalTime += duration;
        data.callCount++;
        data.averageTime = data.totalTime / data.callCount;
        data.minTime = std::min(data.minTime, duration);
        data.maxTime = std::max(data.maxTime, duration);
    }
    
    void recordMemoryAllocation(size_t size, const std::string& category) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_memoryStats[category].allocatedBytes += size;
        m_memoryStats[category].allocationCount++;
    }
    
    void recordMemoryDeallocation(size_t size, const std::string& category) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_memoryStats[category].deallocatedBytes += size;
        m_memoryStats[category].deallocationCount++;
    }
    
    std::vector<ProfileData> getResults() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::vector<ProfileData> results;
        results.reserve(m_sections.size());
        
        for (const auto& pair : m_sections) {
            results.push_back(pair.second);
        }
        
        // Sort by total time descending
        std::sort(results.begin(), results.end(),
            [](const ProfileData& a, const ProfileData& b) {
                return a.totalTime > b.totalTime;
            });
        
        return results;
    }
    
    struct MemoryStats {
        size_t allocatedBytes = 0;
        size_t deallocatedBytes = 0;
        uint64_t allocationCount = 0;
        uint64_t deallocationCount = 0;
        
        size_t getCurrentUsage() const {
            return allocatedBytes - deallocatedBytes;
        }
    };
    
    std::unordered_map<std::string, MemoryStats> getMemoryStats() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_memoryStats;
    }
    
    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sections.clear();
        m_memoryStats.clear();
        // Clear the stack
        while (!m_sectionStack.empty()) {
            m_sectionStack.pop();
        }
    }
    
    void saveReport(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            return;
        }
        
        file << "Performance Profile Report\n";
        file << "==========================\n\n";
        
        // Timing data
        file << "Timing Profile:\n";
        file << std::setw(30) << "Section Name" 
             << std::setw(15) << "Total (ms)"
             << std::setw(15) << "Average (ms)"
             << std::setw(10) << "Calls"
             << std::setw(15) << "Min (ms)"
             << std::setw(15) << "Max (ms)" << "\n";
        file << std::string(100, '-') << "\n";
        
        // Create results directly without calling getResults() to avoid double-locking
        std::vector<ProfileData> results;
        results.reserve(m_sections.size());
        
        for (const auto& pair : m_sections) {
            results.push_back(pair.second);
        }
        
        // Sort by total time descending
        std::sort(results.begin(), results.end(),
            [](const ProfileData& a, const ProfileData& b) {
                return a.totalTime > b.totalTime;
            });
        
        for (const auto& data : results) {
            file << std::setw(30) << data.name
                 << std::setw(15) << std::fixed << std::setprecision(3) << data.totalTime
                 << std::setw(15) << std::fixed << std::setprecision(3) << data.averageTime
                 << std::setw(10) << data.callCount
                 << std::setw(15) << std::fixed << std::setprecision(3) << data.minTime
                 << std::setw(15) << std::fixed << std::setprecision(3) << data.maxTime << "\n";
        }
        
        // Memory data
        file << "\n\nMemory Profile:\n";
        file << std::setw(20) << "Category"
             << std::setw(15) << "Allocated"
             << std::setw(15) << "Deallocated"
             << std::setw(15) << "Current"
             << std::setw(10) << "Allocs"
             << std::setw(10) << "Deallocs" << "\n";
        file << std::string(85, '-') << "\n";
        
        for (const auto& pair : m_memoryStats) {
            const auto& stats = pair.second;
            file << std::setw(20) << pair.first
                 << std::setw(15) << stats.allocatedBytes
                 << std::setw(15) << stats.deallocatedBytes
                 << std::setw(15) << stats.getCurrentUsage()
                 << std::setw(10) << stats.allocationCount
                 << std::setw(10) << stats.deallocationCount << "\n";
        }
        
        file.close();
    }
    
private:
    struct SectionInfo {
        std::string name;
        std::chrono::high_resolution_clock::time_point startTime;
    };
    
    std::unordered_map<std::string, ProfileData> m_sections;
    std::unordered_map<std::string, MemoryStats> m_memoryStats;
    std::stack<SectionInfo> m_sectionStack;
    mutable std::mutex m_mutex;
};

// RAII profiling helper
class ScopedProfiler {
public:
    explicit ScopedProfiler(const std::string& name) : m_name(name) {
        PerformanceProfiler::getInstance().beginSection(m_name);
    }
    
    ~ScopedProfiler() {
        PerformanceProfiler::getInstance().endSection(m_name);
    }
    
private:
    std::string m_name;
};

// Convenience macros
#define PROFILE_SCOPE(name) VoxelEditor::Logging::ScopedProfiler _prof(name)
#define PROFILE_FUNCTION() VoxelEditor::Logging::ScopedProfiler _prof(__FUNCTION__)

}
}