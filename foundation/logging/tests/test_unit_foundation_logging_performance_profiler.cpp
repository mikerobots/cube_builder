#include <gtest/gtest.h>
#include "../PerformanceProfiler.h"
#include <thread>
#include <chrono>
#include <filesystem>

using namespace VoxelEditor::Logging;

class PerformanceProfilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        PerformanceProfiler::getInstance().reset();
    }
    
    void TearDown() override {
        PerformanceProfiler::getInstance().reset();
    }
};

TEST_F(PerformanceProfilerTest, BasicProfiling) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    profiler.beginSection("TestSection");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    profiler.endSection("TestSection");
    
    auto results = profiler.getResults();
    ASSERT_EQ(results.size(), 1);
    
    const auto& data = results[0];
    EXPECT_EQ(data.name, "TestSection");
    EXPECT_EQ(data.callCount, 1);
    EXPECT_GE(data.totalTime, 10.0); // At least 10ms
    EXPECT_EQ(data.averageTime, data.totalTime);
    EXPECT_EQ(data.minTime, data.totalTime);
    EXPECT_EQ(data.maxTime, data.totalTime);
}

TEST_F(PerformanceProfilerTest, MultipleCalls) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    // Call the same section multiple times
    for (int i = 0; i < 3; ++i) {
        profiler.beginSection("MultipleSection");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        profiler.endSection("MultipleSection");
    }
    
    auto results = profiler.getResults();
    ASSERT_EQ(results.size(), 1);
    
    const auto& data = results[0];
    EXPECT_EQ(data.name, "MultipleSection");
    EXPECT_EQ(data.callCount, 3);
    EXPECT_GE(data.totalTime, 15.0); // At least 15ms total
    EXPECT_GT(data.minTime, 0.0);
    EXPECT_GE(data.maxTime, data.minTime);
    EXPECT_NEAR(data.averageTime, data.totalTime / 3.0, 0.1);
}

TEST_F(PerformanceProfilerTest, MultipleSections) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    profiler.beginSection("Section1");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    profiler.endSection("Section1");
    
    profiler.beginSection("Section2");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    profiler.endSection("Section2");
    
    auto results = profiler.getResults();
    ASSERT_EQ(results.size(), 2);
    
    // Results should be sorted by total time (descending)
    EXPECT_EQ(results[0].name, "Section1");
    EXPECT_EQ(results[1].name, "Section2");
    EXPECT_GE(results[0].totalTime, results[1].totalTime);
}

TEST_F(PerformanceProfilerTest, ScopedProfiler) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    {
        ScopedProfiler scoped("ScopedTest");
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
    
    auto results = profiler.getResults();
    ASSERT_EQ(results.size(), 1);
    
    const auto& data = results[0];
    EXPECT_EQ(data.name, "ScopedTest");
    EXPECT_EQ(data.callCount, 1);
    EXPECT_GE(data.totalTime, 8.0);
}

TEST_F(PerformanceProfilerTest, NestedProfiling) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    profiler.beginSection("Outer");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    
    profiler.beginSection("Inner");
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    profiler.endSection("Inner");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    profiler.endSection("Outer");
    
    auto results = profiler.getResults();
    ASSERT_EQ(results.size(), 2);
    
    // Find outer and inner sections
    auto outerIt = std::find_if(results.begin(), results.end(),
        [](const PerformanceProfiler::ProfileData& data) {
            return data.name == "Outer";
        });
    auto innerIt = std::find_if(results.begin(), results.end(),
        [](const PerformanceProfiler::ProfileData& data) {
            return data.name == "Inner";
        });
    
    ASSERT_NE(outerIt, results.end());
    ASSERT_NE(innerIt, results.end());
    
    EXPECT_GE(outerIt->totalTime, innerIt->totalTime);
    EXPECT_GE(outerIt->totalTime, 10.0); // At least 10ms
    EXPECT_GE(innerIt->totalTime, 3.0);  // At least 3ms
}

TEST_F(PerformanceProfilerTest, MismatchedSections) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    profiler.beginSection("Section1");
    profiler.endSection("Section2"); // Mismatched name
    
    auto results = profiler.getResults();
    EXPECT_EQ(results.size(), 0); // No valid sections recorded
}

TEST_F(PerformanceProfilerTest, MemoryTracking) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    profiler.recordMemoryAllocation(1000, "TestCategory");
    profiler.recordMemoryAllocation(500, "TestCategory");
    profiler.recordMemoryDeallocation(300, "TestCategory");
    
    auto memStats = profiler.getMemoryStats();
    ASSERT_EQ(memStats.size(), 1);
    
    const auto& stats = memStats["TestCategory"];
    EXPECT_EQ(stats.allocatedBytes, 1500);
    EXPECT_EQ(stats.deallocatedBytes, 300);
    EXPECT_EQ(stats.getCurrentUsage(), 1200);
    EXPECT_EQ(stats.allocationCount, 2);
    EXPECT_EQ(stats.deallocationCount, 1);
}

TEST_F(PerformanceProfilerTest, MultipleMemoryCategories) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    profiler.recordMemoryAllocation(1000, "Category1");
    profiler.recordMemoryAllocation(2000, "Category2");
    profiler.recordMemoryDeallocation(500, "Category1");
    
    auto memStats = profiler.getMemoryStats();
    ASSERT_EQ(memStats.size(), 2);
    
    EXPECT_EQ(memStats["Category1"].getCurrentUsage(), 500);
    EXPECT_EQ(memStats["Category2"].getCurrentUsage(), 2000);
}

TEST_F(PerformanceProfilerTest, Reset) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    profiler.beginSection("TestSection");
    profiler.endSection("TestSection");
    profiler.recordMemoryAllocation(1000, "TestCategory");
    
    EXPECT_EQ(profiler.getResults().size(), 1);
    EXPECT_EQ(profiler.getMemoryStats().size(), 1);
    
    profiler.reset();
    
    EXPECT_EQ(profiler.getResults().size(), 0);
    EXPECT_EQ(profiler.getMemoryStats().size(), 0);
}

TEST_F(PerformanceProfilerTest, SaveReport) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    const std::string reportFile = "test_profile_report.txt";
    
    // Clean up any existing file
    if (std::filesystem::exists(reportFile)) {
        std::filesystem::remove(reportFile);
    }
    
    // Add some profile data
    profiler.beginSection("TestSection");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    profiler.endSection("TestSection");
    
    profiler.recordMemoryAllocation(1000, "TestMemory");
    profiler.recordMemoryDeallocation(200, "TestMemory");
    
    profiler.saveReport(reportFile);
    
    // Verify file was created and contains expected content
    EXPECT_TRUE(std::filesystem::exists(reportFile));
    
    std::ifstream file(reportFile);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("Performance Profile Report") != std::string::npos);
    EXPECT_TRUE(content.find("TestSection") != std::string::npos);
    EXPECT_TRUE(content.find("TestMemory") != std::string::npos);
    EXPECT_TRUE(content.find("Timing Profile:") != std::string::npos);
    EXPECT_TRUE(content.find("Memory Profile:") != std::string::npos);
    
    // Clean up
    std::filesystem::remove(reportFile);
}

TEST_F(PerformanceProfilerTest, Macros) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    {
        PROFILE_SCOPE("MacroTest");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    auto results = profiler.getResults();
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].name, "MacroTest");
}

void testFunction() {
    PROFILE_FUNCTION();
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
}

TEST_F(PerformanceProfilerTest, FunctionMacro) {
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    
    testFunction();
    
    auto results = profiler.getResults();
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].name, "testFunction");
}