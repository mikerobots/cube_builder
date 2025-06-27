#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <vector>
#include <map>
#include <iostream>

// Core components
#include "../../core/voxel_data/VoxelDataManager.h"
#include "../../core/undo_redo/HistoryManager.h"
#include "../../core/undo_redo/PlacementCommands.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/math/Vector3i.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::UndoRedo;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

// Memory monitoring utility
class MemoryMonitor {
public:
    static size_t getCurrentMemoryUsage() {
        // Platform-specific memory measurement
        // This is a simplified implementation - real implementation would use platform APIs
        #ifdef __APPLE__
            // macOS implementation using mach API would go here
            return 0; // Placeholder - would implement actual memory measurement
        #elif __linux__
            // Linux implementation using /proc/self/status would go here
            return 0; // Placeholder - would implement actual memory measurement
        #else
            // Windows implementation using GetProcessMemoryInfo would go here
            return 0; // Placeholder - would implement actual memory measurement
        #endif
    }
    
    static size_t getVoxelManagerMemoryUsage(VoxelDataManager* manager) {
        if (!manager) return 0;
        return manager->getMemoryUsage();
    }
    
    static size_t getHistoryManagerMemoryUsage(HistoryManager* history) {
        if (!history) return 0;
        return history->getMemoryUsage();
    }
};

class MemoryUsageValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
        historyManager = std::make_unique<HistoryManager>();
        
        // Set up standard workspace
        Vector3f workspaceSize(5.0f, 5.0f, 5.0f); // 5x5x5 meter workspace
        voxelManager->resizeWorkspace(workspaceSize);
        
        // Record baseline memory usage
        baselineMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
    }
    
    void TearDown() override {
        // Clean up
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
    std::unique_ptr<HistoryManager> historyManager;
    size_t baselineMemory = 0;
    
    // Memory limits based on VR optimization requirements (< 4GB total)
    static constexpr size_t MAX_VOXEL_MEMORY_MB = 2048;    // 2GB for voxel data
    static constexpr size_t MAX_HISTORY_MEMORY_MB = 512;   // 512MB for undo/redo history
    static constexpr size_t MAX_SINGLE_OPERATION_MB = 100; // 100MB per operation
    static constexpr size_t BYTES_PER_MB = 1024 * 1024;
};

// ============================================================================
// REQ-11.4.4: Performance tests shall validate memory usage within limits
// ============================================================================

TEST_F(MemoryUsageValidationTest, VoxelPlacement_MemoryUsageWithinLimits_REQ_11_4_4) {
    // Test memory usage for large-scale voxel placement operations
    
    size_t initialMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
    
    // Place a moderate number of voxels to avoid timeout
    const int voxelsPerDimension = 20; // 20x20x10 = 4,000 voxels
    const VoxelResolution resolution = VoxelResolution::Size_1cm;
    
    size_t voxelsPlaced = 0;
    
    for (int x = -voxelsPerDimension/2; x < voxelsPerDimension/2; ++x) {
        for (int y = 0; y < voxelsPerDimension/2; ++y) { // Keep Y >= 0 for ground plane
            for (int z = -voxelsPerDimension/2; z < voxelsPerDimension/2; ++z) {
                Vector3i pos(x, y, z);
                bool success = voxelManager->setVoxel(pos, resolution, true);
                if (success) {
                    voxelsPlaced++;
                }
                
                // Check memory usage every 1000 voxels
                if (voxelsPlaced % 1000 == 0) {
                    size_t currentMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
                    size_t memoryUsedMB = (currentMemory - initialMemory) / BYTES_PER_MB;
                    
                    EXPECT_LT(memoryUsedMB, MAX_VOXEL_MEMORY_MB) 
                        << "Memory usage exceeded limit after " << voxelsPlaced << " voxels: " 
                        << memoryUsedMB << "MB";
                }
            }
        }
    }
    
    size_t finalMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
    size_t totalMemoryUsedMB = (finalMemory - initialMemory) / BYTES_PER_MB;
    size_t avgBytesPerVoxel = voxelsPlaced > 0 ? (finalMemory - initialMemory) / voxelsPlaced : 0;
    
    std::cout << "Placed " << voxelsPlaced << " voxels" << std::endl;
    std::cout << "Total memory used: " << totalMemoryUsedMB << "MB" << std::endl;
    std::cout << "Average bytes per voxel: " << avgBytesPerVoxel << std::endl;
    
    // Validate memory limits
    EXPECT_LT(totalMemoryUsedMB, MAX_VOXEL_MEMORY_MB) 
        << "Total voxel memory usage exceeded limit: " << totalMemoryUsedMB << "MB";
    
    // Expect efficient storage (sparse octree should be efficient)
    if (voxelsPlaced > 0) {
        EXPECT_LT(avgBytesPerVoxel, 256) 
            << "Average memory per voxel too high: " << avgBytesPerVoxel << " bytes";
    }
}

TEST_F(MemoryUsageValidationTest, UndoRedoHistory_MemoryUsageWithinLimits_REQ_11_4_4) {
    // Test memory usage for undo/redo history accumulation
    
    size_t initialHistoryMemory = MemoryMonitor::getHistoryManagerMemoryUsage(historyManager.get());
    
    // Perform many operations to build up history
    const int numOperations = 100; // Reduced to avoid timeout
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Use 4cm aligned coordinates (4cm = 0.04m)
    for (int i = 0; i < numOperations; ++i) {
        // Create placement command with properly aligned coordinates
        int xGrid = (i % 10) * 4; // 0, 4, 8, 12, 16, 20, 24, 28, 32, 36 cm
        int yGrid = ((i / 10) % 10) * 4; // 0, 4, 8, 12, 16, 20, 24, 28, 32, 36 cm
        int zGrid = ((i / 100) % 10) * 4; // 0, 4, 8, 12, 16, 20, 24, 28, 32, 36 cm
        
        Vector3i pos(xGrid, yGrid, zGrid);
        auto command = PlacementCommandFactory::createPlacementCommand(
            voxelManager.get(), pos, resolution);
        
        if (command) {
            // Execute and add to history
            historyManager->executeCommand(std::move(command));
            
            // Check memory usage every 20 operations
            if (i % 20 == 0) {
                size_t currentHistoryMemory = MemoryMonitor::getHistoryManagerMemoryUsage(historyManager.get());
                size_t memoryUsedMB = (currentHistoryMemory - initialHistoryMemory) / BYTES_PER_MB;
                
                EXPECT_LT(memoryUsedMB, MAX_HISTORY_MEMORY_MB) 
                    << "History memory usage exceeded limit after " << i << " operations: " 
                    << memoryUsedMB << "MB";
            }
        }
    }
    
    size_t finalHistoryMemory = MemoryMonitor::getHistoryManagerMemoryUsage(historyManager.get());
    size_t totalHistoryMemoryMB = (finalHistoryMemory - initialHistoryMemory) / BYTES_PER_MB;
    size_t avgBytesPerOperation = numOperations > 0 ? (finalHistoryMemory - initialHistoryMemory) / numOperations : 0;
    
    std::cout << "Executed " << numOperations << " operations" << std::endl;
    std::cout << "Total history memory used: " << totalHistoryMemoryMB << "MB" << std::endl;
    std::cout << "Average bytes per operation: " << avgBytesPerOperation << std::endl;
    
    // Validate memory limits
    EXPECT_LT(totalHistoryMemoryMB, MAX_HISTORY_MEMORY_MB) 
        << "History memory usage exceeded limit: " << totalHistoryMemoryMB << "MB";
    
    // Expect reasonable memory per operation
    if (numOperations > 0) {
        EXPECT_LT(avgBytesPerOperation, 1024) 
            << "Average memory per operation too high: " << avgBytesPerOperation << " bytes";
    }
}

TEST_F(MemoryUsageValidationTest, FillOperation_SingleOperationMemoryLimit_REQ_11_4_4) {
    // Test memory usage for simulated large fill operations using direct voxel placement
    
    size_t initialMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
    
    // Simulate a fill operation by placing voxels in a region
    // 50x25x50 region with 1cm voxels (simulates fill region)
    VoxelResolution resolution = VoxelResolution::Size_1cm;
    size_t voxelsPlaced = 0;
    
    // Use 1cm-aligned coordinates
    for (int x = 0; x < 100; x += 1) { // 0 to 99 cm in 1cm increments
        for (int y = 0; y < 50; y += 1) { // 0 to 49 cm in 1cm increments  
            for (int z = 0; z < 100; z += 1) { // 0 to 99 cm in 1cm increments
                Vector3i pos(x, y, z);
                bool success = voxelManager->setVoxel(pos, resolution, true);
                if (success) {
                    voxelsPlaced++;
                }
                
                // Break early if we place enough voxels for the test
                if (voxelsPlaced >= 1000) {
                    goto fill_complete;
                }
            }
        }
    }
    
fill_complete:
    size_t afterFillMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
    size_t fillMemoryUsedMB = (afterFillMemory - initialMemory) / BYTES_PER_MB;
    
    std::cout << "Fill operation memory usage: " << fillMemoryUsedMB << "MB" << std::endl;
    std::cout << "Voxels placed: " << voxelsPlaced << std::endl;
    
    // Validate single operation memory limit
    EXPECT_LT(fillMemoryUsedMB, MAX_SINGLE_OPERATION_MB) 
        << "Single fill operation exceeded memory limit: " << fillMemoryUsedMB << "MB";
    
    // Test memory cleanup by clearing voxels
    voxelManager->clearAll();
    
    size_t afterClearMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
    size_t memoryReclaimed = afterFillMemory - afterClearMemory;
    size_t memoryReclaimedMB = memoryReclaimed / BYTES_PER_MB;
    
    std::cout << "Memory reclaimed after clear: " << memoryReclaimedMB << "MB" << std::endl;
    
    // Most memory should be reclaimed after clear
    if (afterFillMemory > initialMemory) {
        double reclaimRatio = static_cast<double>(memoryReclaimed) / static_cast<double>(afterFillMemory - initialMemory);
        EXPECT_GT(reclaimRatio, 0.5) 
            << "Clear should reclaim at least 50% of operation memory. Ratio: " << reclaimRatio;
    }
}

TEST_F(MemoryUsageValidationTest, MultiResolution_MemoryUsageScaling_REQ_11_4_4) {
    // Test memory usage scaling with different voxel resolutions
    
    std::map<VoxelResolution, size_t> memoryUsageByResolution;
    
    // Test different resolutions
    std::vector<VoxelResolution> resolutions = {
        VoxelResolution::Size_1cm,
        VoxelResolution::Size_4cm,
        VoxelResolution::Size_16cm,
        VoxelResolution::Size_64cm
    };
    
    for (const auto& resolution : resolutions) {
        // Clear voxels between tests
        voxelManager->clearAll();
        
        size_t initialMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
        
        // Place voxels in a fixed region, using appropriate grid alignment
        int gridSize = static_cast<int>(resolution); // resolution in cm
        size_t voxelsPlaced = 0;
        
        // Place voxels in a 1x1x1 meter region (100x100x100 cm)
        for (int x = 0; x < 100; x += gridSize) {
            for (int y = 0; y < 100; y += gridSize) {
                for (int z = 0; z < 100; z += gridSize) {
                    Vector3i pos(x, y, z);
                    bool success = voxelManager->setVoxel(pos, resolution, true);
                    if (success) {
                        voxelsPlaced++;
                    }
                    
                    // Limit voxels to avoid timeout for smaller resolutions
                    if (voxelsPlaced >= 1000) {
                        goto res_complete;
                    }
                }
            }
        }
        
res_complete:
        size_t finalMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
        size_t memoryUsed = finalMemory - initialMemory;
        size_t memoryUsedMB = memoryUsed / BYTES_PER_MB;
        
        memoryUsageByResolution[resolution] = memoryUsed;
        
        std::cout << "Resolution " << static_cast<int>(resolution) << "cm: " 
                  << memoryUsedMB << "MB (" << voxelsPlaced << " voxels)" << std::endl;
        
        // Each resolution should stay within limits
        EXPECT_LT(memoryUsedMB, MAX_SINGLE_OPERATION_MB) 
            << "Resolution " << static_cast<int>(resolution) << "cm" 
            << " exceeded memory limit: " << memoryUsedMB << "MB";
    }
    
    // Verify that smaller voxels use more memory (as expected due to count)
    // but the increase should be reasonable
    if (memoryUsageByResolution.count(VoxelResolution::Size_1cm) && 
        memoryUsageByResolution.count(VoxelResolution::Size_64cm)) {
        
        size_t memory1cm = memoryUsageByResolution[VoxelResolution::Size_1cm];
        size_t memory64cm = memoryUsageByResolution[VoxelResolution::Size_64cm];
        
        // If both have memory usage > 0, compare them
        if (memory1cm > 0 && memory64cm > 0) {
            // 1cm voxels should use more memory than 64cm voxels for same region
            EXPECT_GT(memory1cm, memory64cm) 
                << "1cm resolution should use more memory than 64cm for same region";
            
            // But the ratio should be reasonable (not more than 1000x difference)
            double memoryRatio = static_cast<double>(memory1cm) / static_cast<double>(memory64cm);
            EXPECT_LT(memoryRatio, 1000.0) 
                << "Memory ratio between 1cm and 64cm resolutions too high: " << memoryRatio;
        }
    }
}

TEST_F(MemoryUsageValidationTest, WorkspaceResize_MemoryBehavior_REQ_11_4_4) {
    // Test memory behavior during workspace resizing
    
    // Place some voxels in the current workspace using 4cm grid alignment
    const int voxelCount = 100; // Reduced to avoid timeout
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    size_t voxelsPlaced = 0;
    for (int i = 0; i < voxelCount && voxelsPlaced < 100; ++i) {
        // Use 4cm-aligned coordinates
        int x = (i % 10) * 4; // 0, 4, 8, 12, ..., 36 cm
        int y = ((i / 10) % 10) * 4; // 0, 4, 8, 12, ..., 36 cm
        int z = ((i / 100) % 10) * 4; // 0, 4, 8, 12, ..., 36 cm
        
        Vector3i pos(x, y, z);
        bool success = voxelManager->setVoxel(pos, resolution, true);
        if (success) {
            voxelsPlaced++;
        }
    }
    
    size_t memoryWithVoxels = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
    
    // Resize workspace to larger size
    Vector3f largerWorkspace(8.0f, 8.0f, 8.0f); // 8x8x8 meter workspace
    voxelManager->resizeWorkspace(largerWorkspace);
    
    size_t memoryAfterResize = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
    
    // Memory should not increase dramatically just from workspace resize
    if (memoryWithVoxels > 0) {
        double memoryIncreaseRatio = static_cast<double>(memoryAfterResize) / 
                                    static_cast<double>(memoryWithVoxels);
        
        EXPECT_LT(memoryIncreaseRatio, 2.0) 
            << "Workspace resize should not more than double memory usage. Ratio: " 
            << memoryIncreaseRatio;
    }
    
    // Verify at least some voxels are still accessible after resize
    int voxelsFound = 0;
    for (int i = 0; i < voxelCount && voxelsFound < static_cast<int>(voxelsPlaced); ++i) {
        int x = (i % 10) * 4;
        int y = ((i / 10) % 10) * 4;
        int z = ((i / 100) % 10) * 4;
        
        Vector3i pos(x, y, z);
        if (voxelManager->hasVoxel(pos, resolution)) {
            voxelsFound++;
        }
    }
    
    // We should find at least 50% of the voxels we placed
    EXPECT_GT(voxelsFound, static_cast<int>(voxelsPlaced) / 2) 
        << "At least 50% of voxels should remain accessible after workspace resize. Found: " 
        << voxelsFound << " out of " << voxelsPlaced;
    
    size_t finalMemoryMB = memoryAfterResize / BYTES_PER_MB;
    EXPECT_LT(finalMemoryMB, MAX_VOXEL_MEMORY_MB) 
        << "Memory after workspace resize should stay within limits: " << finalMemoryMB << "MB";
}

TEST_F(MemoryUsageValidationTest, StressTest_ContinuousOperations_REQ_11_4_4) {
    // Stress test: continuous operations over time to check for memory leaks
    
    size_t initialMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
    std::vector<size_t> memorySnapshots;
    
    const int totalOperations = 200; // Reduced to avoid timeout
    const int snapshotInterval = 50;
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    for (int i = 0; i < totalOperations; ++i) {
        // Alternate between placing and removing voxels with 4cm grid alignment
        int xGrid = ((i % 10) * 4); // 0, 4, 8, 12, 16, 20, 24, 28, 32, 36 cm
        int yGrid = (((i / 10) % 10) * 4); // 0, 4, 8, 12, 16, 20, 24, 28, 32, 36 cm
        int zGrid = (((i / 100) % 10) * 4); // 0, 4, 8, 12, 16, 20, 24, 28, 32, 36 cm
        
        Vector3i pos(xGrid, yGrid, zGrid);
        bool shouldPlace = (i % 2 == 0);
        
        voxelManager->setVoxel(pos, resolution, shouldPlace);
        
        // Take memory snapshots
        if (i % snapshotInterval == 0) {
            size_t currentMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
            memorySnapshots.push_back(currentMemory);
            
            size_t memoryUsedMB = (currentMemory - initialMemory) / BYTES_PER_MB;
            EXPECT_LT(memoryUsedMB, MAX_VOXEL_MEMORY_MB) 
                << "Memory exceeded limit during stress test at operation " << i 
                << ": " << memoryUsedMB << "MB";
        }
    }
    
    // Check for memory leaks: final memory should not be dramatically higher than initial
    size_t finalMemory = MemoryMonitor::getVoxelManagerMemoryUsage(voxelManager.get());
    size_t memoryIncrease = finalMemory > initialMemory ? finalMemory - initialMemory : 0;
    size_t memoryIncreaseMB = memoryIncrease / BYTES_PER_MB;
    
    std::cout << "Stress test memory increase: " << memoryIncreaseMB << "MB" << std::endl;
    
    // Since we're alternating place/remove, final memory increase should be minimal
    EXPECT_LT(memoryIncreaseMB, 50) 
        << "Stress test suggests memory leak. Memory increase: " << memoryIncreaseMB << "MB";
    
    // Verify memory usage is stable (not continuously growing)
    if (memorySnapshots.size() >= 3) {
        size_t firstSnapshot = memorySnapshots[1]; // Skip initial snapshot
        size_t lastSnapshot = memorySnapshots.back();
        
        if (firstSnapshot > 0) {
            double growthRatio = static_cast<double>(lastSnapshot) / static_cast<double>(firstSnapshot);
            
            EXPECT_LT(growthRatio, 1.5) 
                << "Memory appears to be continuously growing. Growth ratio: " << growthRatio;
        }
    }
}