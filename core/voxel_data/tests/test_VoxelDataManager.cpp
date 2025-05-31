#include <gtest/gtest.h>
#include "../VoxelDataManager.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;

class VoxelDataManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<EventDispatcher>();
        manager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
        
        // Event tracking
        voxelChangedEventCount = 0;
        resolutionChangedEventCount = 0;
        workspaceResizedEventCount = 0;
        
        // TODO: Fix EventDispatcher to support lambda callbacks
        // For now, events will be tested manually
        /*
        eventDispatcher->subscribe<VoxelChangedEvent>([this](const VoxelChangedEvent& event) {
            voxelChangedEventCount++;
            lastVoxelChangedEvent = event;
        });
        
        eventDispatcher->subscribe<ResolutionChangedEvent>([this](const ResolutionChangedEvent& event) {
            resolutionChangedEventCount++;
            lastResolutionChangedEvent = event;
        });
        
        eventDispatcher->subscribe<WorkspaceResizedEvent>([this](const WorkspaceResizedEvent& event) {
            workspaceResizedEventCount++;
            lastWorkspaceResizedEvent = event;
        });
        */
    }
    
    void TearDown() override {
        manager.reset(); // Ensure proper cleanup order
        eventDispatcher.reset();
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> manager;
    
    // Event tracking
    int voxelChangedEventCount;
    int resolutionChangedEventCount;
    int workspaceResizedEventCount;
    
    VoxelChangedEvent lastVoxelChangedEvent{Vector3i::Zero(), VoxelResolution::Size_1cm, false, false};
    ResolutionChangedEvent lastResolutionChangedEvent{VoxelResolution::Size_1cm, VoxelResolution::Size_1cm};
    WorkspaceResizedEvent lastWorkspaceResizedEvent{Vector3f(), Vector3f()};
};

TEST_F(VoxelDataManagerTest, DefaultConstruction) {
    EXPECT_EQ(manager->getActiveResolution(), VoxelResolution::Size_1cm);
    EXPECT_FLOAT_EQ(manager->getActiveVoxelSize(), 0.01f);
    EXPECT_EQ(manager->getWorkspaceSize(), Vector3f(5.0f, 5.0f, 5.0f));
    EXPECT_EQ(manager->getTotalVoxelCount(), 0);
    EXPECT_GT(manager->getMemoryUsage(), 0);
}

TEST_F(VoxelDataManagerTest, ConstructionWithoutEventDispatcher) {
    VoxelDataManager managerNoEvents;
    
    EXPECT_EQ(managerNoEvents.getActiveResolution(), VoxelResolution::Size_1cm);
    EXPECT_EQ(managerNoEvents.getWorkspaceSize(), Vector3f(5.0f, 5.0f, 5.0f));
    EXPECT_EQ(managerNoEvents.getTotalVoxelCount(), 0);
}

TEST_F(VoxelDataManagerTest, BasicVoxelOperations) {
    Vector3i pos(10, 20, 30);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Initially empty
    EXPECT_FALSE(manager->getVoxel(pos, resolution));
    EXPECT_FALSE(manager->hasVoxel(pos, resolution));
    EXPECT_EQ(manager->getVoxelCount(resolution), 0);
    
    // Set voxel
    EXPECT_TRUE(manager->setVoxel(pos, resolution, true));
    EXPECT_TRUE(manager->getVoxel(pos, resolution));
    EXPECT_TRUE(manager->hasVoxel(pos, resolution));
    EXPECT_EQ(manager->getVoxelCount(resolution), 1);
    
    // Check event was dispatched
    EXPECT_EQ(voxelChangedEventCount, 1);
    EXPECT_EQ(lastVoxelChangedEvent.gridPos, pos);
    EXPECT_EQ(lastVoxelChangedEvent.resolution, resolution);
    EXPECT_FALSE(lastVoxelChangedEvent.oldValue);
    EXPECT_TRUE(lastVoxelChangedEvent.newValue);
    
    // Clear voxel
    EXPECT_TRUE(manager->setVoxel(pos, resolution, false));
    EXPECT_FALSE(manager->getVoxel(pos, resolution));
    EXPECT_EQ(manager->getVoxelCount(resolution), 0);
    
    // Check second event
    EXPECT_EQ(voxelChangedEventCount, 2);
    EXPECT_TRUE(lastVoxelChangedEvent.oldValue);
    EXPECT_FALSE(lastVoxelChangedEvent.newValue);
}

TEST_F(VoxelDataManagerTest, VoxelPositionOperations) {
    VoxelPosition voxelPos(Vector3i(5, 10, 15), VoxelResolution::Size_2cm);
    
    // Set using VoxelPosition
    EXPECT_TRUE(manager->setVoxel(voxelPos, true));
    EXPECT_TRUE(manager->getVoxel(voxelPos));
    EXPECT_TRUE(manager->hasVoxel(voxelPos));
    
    // Clear using VoxelPosition
    EXPECT_TRUE(manager->setVoxel(voxelPos, false));
    EXPECT_FALSE(manager->getVoxel(voxelPos));
    EXPECT_FALSE(manager->hasVoxel(voxelPos));
}

TEST_F(VoxelDataManagerTest, WorldSpaceOperations) {
    Vector3f worldPos(1.0f, -0.5f, 2.0f);
    VoxelResolution resolution = VoxelResolution::Size_4cm;
    
    // Set voxel at world position
    EXPECT_TRUE(manager->setVoxelAtWorldPos(worldPos, resolution, true));
    EXPECT_TRUE(manager->getVoxelAtWorldPos(worldPos, resolution));
    EXPECT_TRUE(manager->hasVoxelAtWorldPos(worldPos, resolution));
    
    // Test with active resolution
    manager->setActiveResolution(resolution);
    EXPECT_TRUE(manager->setVoxelAtWorldPos(worldPos, true));
    EXPECT_TRUE(manager->getVoxelAtWorldPos(worldPos));
    EXPECT_TRUE(manager->hasVoxelAtWorldPos(worldPos));
}

TEST_F(VoxelDataManagerTest, ResolutionManagement) {
    VoxelResolution originalResolution = manager->getActiveResolution();
    VoxelResolution newResolution = VoxelResolution::Size_8cm;
    
    // Change active resolution
    manager->setActiveResolution(newResolution);
    EXPECT_EQ(manager->getActiveResolution(), newResolution);
    EXPECT_FLOAT_EQ(manager->getActiveVoxelSize(), getVoxelSize(newResolution));
    
    // Check event was dispatched
    EXPECT_EQ(resolutionChangedEventCount, 1);
    EXPECT_EQ(lastResolutionChangedEvent.oldResolution, originalResolution);
    EXPECT_EQ(lastResolutionChangedEvent.newResolution, newResolution);
    
    // Setting same resolution should not trigger event
    manager->setActiveResolution(newResolution);
    EXPECT_EQ(resolutionChangedEventCount, 1); // Should remain 1
    
    // Test invalid resolution
    manager->setActiveResolution(static_cast<VoxelResolution>(255));
    EXPECT_EQ(manager->getActiveResolution(), newResolution); // Should remain unchanged
}

TEST_F(VoxelDataManagerTest, WorkspaceManagement) {
    Vector3f originalSize = manager->getWorkspaceSize();
    Vector3f newSize(8.0f, 6.0f, 4.0f);
    
    // Resize workspace
    EXPECT_TRUE(manager->resizeWorkspace(newSize));
    EXPECT_EQ(manager->getWorkspaceSize(), newSize);
    
    // Check event was dispatched
    EXPECT_EQ(workspaceResizedEventCount, 1);
    EXPECT_EQ(lastWorkspaceResizedEvent.oldSize, originalSize);
    EXPECT_EQ(lastWorkspaceResizedEvent.newSize, newSize);
    
    // Test cubic resize
    EXPECT_TRUE(manager->resizeWorkspace(7.0f));
    EXPECT_EQ(manager->getWorkspaceSize(), Vector3f(7.0f, 7.0f, 7.0f));
    
    // Test invalid resize
    EXPECT_FALSE(manager->resizeWorkspace(Vector3f(1.0f, 1.0f, 1.0f))); // Too small
    EXPECT_EQ(manager->getWorkspaceSize(), Vector3f(7.0f, 7.0f, 7.0f)); // Should remain unchanged
}

TEST_F(VoxelDataManagerTest, PositionValidation) {
    Vector3i validGridPos(50, 50, 50);
    Vector3i invalidGridPos(-1, 0, 0);
    Vector3f validWorldPos(1.0f, 1.0f, 1.0f);
    Vector3f invalidWorldPos(10.0f, 10.0f, 10.0f);
    
    EXPECT_TRUE(manager->isValidPosition(validGridPos, VoxelResolution::Size_1cm));
    EXPECT_FALSE(manager->isValidPosition(invalidGridPos, VoxelResolution::Size_1cm));
    
    EXPECT_TRUE(manager->isValidWorldPosition(validWorldPos));
    EXPECT_FALSE(manager->isValidWorldPosition(invalidWorldPos));
}

TEST_F(VoxelDataManagerTest, MultipleResolutionVoxels) {
    Vector3i pos(25, 25, 25);
    
    // Set voxels at different resolutions
    for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
        VoxelResolution resolution = static_cast<VoxelResolution>(i);
        EXPECT_TRUE(manager->setVoxel(pos, resolution, true));
        EXPECT_TRUE(manager->getVoxel(pos, resolution));
        EXPECT_EQ(manager->getVoxelCount(resolution), 1);
    }
    
    EXPECT_EQ(manager->getTotalVoxelCount(), static_cast<int>(VoxelResolution::COUNT));
    
    // Clear specific resolution
    manager->clearResolution(VoxelResolution::Size_4cm);
    EXPECT_FALSE(manager->getVoxel(pos, VoxelResolution::Size_4cm));
    EXPECT_EQ(manager->getVoxelCount(VoxelResolution::Size_4cm), 0);
    EXPECT_EQ(manager->getTotalVoxelCount(), static_cast<int>(VoxelResolution::COUNT) - 1);
}

TEST_F(VoxelDataManagerTest, ClearOperations) {
    // Add voxels to multiple resolutions
    std::vector<Vector3i> positions = {
        Vector3i(10, 10, 10),
        Vector3i(20, 20, 20),
        Vector3i(30, 30, 30)
    };
    
    for (int i = 0; i < 3; ++i) {
        VoxelResolution resolution = static_cast<VoxelResolution>(i);
        for (const auto& pos : positions) {
            manager->setVoxel(pos, resolution, true);
        }
    }
    
    EXPECT_GT(manager->getTotalVoxelCount(), 0);
    
    // Clear active resolution
    manager->setActiveResolution(VoxelResolution::Size_1cm);
    manager->clearActiveResolution();
    EXPECT_EQ(manager->getVoxelCount(VoxelResolution::Size_1cm), 0);
    EXPECT_GT(manager->getTotalVoxelCount(), 0); // Others should remain
    
    // Clear all
    manager->clearAll();
    EXPECT_EQ(manager->getTotalVoxelCount(), 0);
    
    for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
        VoxelResolution resolution = static_cast<VoxelResolution>(i);
        EXPECT_EQ(manager->getVoxelCount(resolution), 0);
    }
}

TEST_F(VoxelDataManagerTest, MemoryManagement) {
    size_t initialMemory = manager->getMemoryUsage();
    
    // Add voxels to increase memory usage
    for (int i = 0; i < 10; ++i) {
        manager->setVoxel(Vector3i(i * 10, i * 10, i * 10), VoxelResolution::Size_1cm, true);
    }
    
    size_t memoryWithVoxels = manager->getMemoryUsage();
    EXPECT_GT(memoryWithVoxels, initialMemory);
    
    // Test memory usage by resolution
    size_t resolutionMemory = manager->getMemoryUsage(VoxelResolution::Size_1cm);
    EXPECT_GT(resolutionMemory, 0);
    
    // Optimize memory
    manager->optimizeMemory();
    EXPECT_EQ(manager->getVoxelCount(VoxelResolution::Size_1cm), 10); // Voxels should remain
    
    // Optimize specific resolution
    manager->optimizeMemory(VoxelResolution::Size_1cm);
    EXPECT_EQ(manager->getVoxelCount(VoxelResolution::Size_1cm), 10); // Voxels should remain
    
    // Clear and verify memory decreases
    manager->clearAll();
    size_t memoryAfterClear = manager->getMemoryUsage();
    EXPECT_LT(memoryAfterClear, memoryWithVoxels);
}

TEST_F(VoxelDataManagerTest, GridAccess) {
    // Test grid access for valid resolutions
    for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
        VoxelResolution resolution = static_cast<VoxelResolution>(i);
        
        const VoxelGrid* constGrid = manager->getGrid(resolution);
        VoxelGrid* mutableGrid = manager->getGrid(resolution);
        
        EXPECT_NE(constGrid, nullptr);
        EXPECT_NE(mutableGrid, nullptr);
        EXPECT_EQ(constGrid, mutableGrid);
        EXPECT_EQ(constGrid->getResolution(), resolution);
    }
    
    // Test invalid resolution
    const VoxelGrid* invalidGrid = manager->getGrid(static_cast<VoxelResolution>(255));
    EXPECT_EQ(invalidGrid, nullptr);
}

TEST_F(VoxelDataManagerTest, VoxelExport) {
    std::vector<Vector3i> expectedPositions = {
        Vector3i(5, 10, 15),
        Vector3i(25, 30, 35),
        Vector3i(45, 50, 55)
    };
    
    VoxelResolution resolution = VoxelResolution::Size_2cm;
    
    // Set voxels
    for (const auto& pos : expectedPositions) {
        manager->setVoxel(pos, resolution, true);
    }
    
    // Export voxels from specific resolution
    std::vector<VoxelPosition> exportedVoxels = manager->getAllVoxels(resolution);
    EXPECT_EQ(exportedVoxels.size(), expectedPositions.size());
    
    for (const auto& voxelPos : exportedVoxels) {
        EXPECT_EQ(voxelPos.resolution, resolution);
        
        bool found = false;
        for (const auto& expectedPos : expectedPositions) {
            if (voxelPos.gridPos == expectedPos) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }
    
    // Export from active resolution
    manager->setActiveResolution(resolution);
    std::vector<VoxelPosition> activeExportedVoxels = manager->getAllVoxels();
    EXPECT_EQ(activeExportedVoxels.size(), expectedPositions.size());
}

TEST_F(VoxelDataManagerTest, EventDispatcherManagement) {
    Vector3i pos(10, 10, 10);
    
    // Set voxel with event dispatcher
    manager->setVoxel(pos, VoxelResolution::Size_1cm, true);
    EXPECT_EQ(voxelChangedEventCount, 1);
    
    // Remove event dispatcher
    manager->setEventDispatcher(nullptr);
    
    // Operations should still work but no events dispatched
    int previousEventCount = voxelChangedEventCount;
    manager->setVoxel(pos, VoxelResolution::Size_1cm, false);
    EXPECT_EQ(voxelChangedEventCount, previousEventCount);
    
    // Set dispatcher back
    manager->setEventDispatcher(eventDispatcher.get());
    
    // Events should be dispatched again
    manager->setVoxel(pos, VoxelResolution::Size_1cm, true);
    EXPECT_EQ(voxelChangedEventCount, previousEventCount + 1);
}

TEST_F(VoxelDataManagerTest, PerformanceMetrics) {
    // Add voxels to different resolutions
    for (int res = 0; res < 3; ++res) {
        VoxelResolution resolution = static_cast<VoxelResolution>(res);
        for (int i = 0; i < (res + 1) * 5; ++i) {
            manager->setVoxel(Vector3i(i, i, i), resolution, true);
        }
    }
    
    VoxelDataManager::PerformanceMetrics metrics = manager->getPerformanceMetrics();
    
    // Check total counts
    EXPECT_GT(metrics.totalVoxels, 0);
    EXPECT_GT(metrics.totalMemoryUsage, 0);
    EXPECT_GT(metrics.memoryEfficiency, 0.0f);
    EXPECT_LE(metrics.memoryEfficiency, 1.0f);
    
    // Check per-resolution counts
    size_t totalFromResolutions = 0;
    for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
        totalFromResolutions += metrics.voxelsByResolution[i];
    }
    EXPECT_EQ(totalFromResolutions, metrics.totalVoxels);
    
    // First few resolutions should have voxels
    EXPECT_GT(metrics.voxelsByResolution[0], 0); // Size_1cm
    EXPECT_GT(metrics.voxelsByResolution[1], 0); // Size_2cm
    EXPECT_GT(metrics.voxelsByResolution[2], 0); // Size_4cm
}

TEST_F(VoxelDataManagerTest, RedundantOperations) {
    Vector3i pos(20, 20, 20);
    VoxelResolution resolution = VoxelResolution::Size_1cm;
    
    // Set voxel multiple times
    EXPECT_TRUE(manager->setVoxel(pos, resolution, true));
    EXPECT_EQ(voxelChangedEventCount, 1);
    
    EXPECT_TRUE(manager->setVoxel(pos, resolution, true)); // Should succeed but not change state
    EXPECT_EQ(voxelChangedEventCount, 1); // No additional event
    EXPECT_EQ(manager->getVoxelCount(resolution), 1);
    
    // Clear voxel multiple times
    EXPECT_TRUE(manager->setVoxel(pos, resolution, false));
    EXPECT_EQ(voxelChangedEventCount, 2);
    
    EXPECT_TRUE(manager->setVoxel(pos, resolution, false)); // Should succeed but not change state
    EXPECT_EQ(voxelChangedEventCount, 2); // No additional event
    EXPECT_EQ(manager->getVoxelCount(resolution), 0);
}

TEST_F(VoxelDataManagerTest, OutOfBoundsOperations) {
    Vector3i outOfBoundsPos(10000, 10000, 10000);
    Vector3f outOfBoundsWorldPos(100.0f, 100.0f, 100.0f);
    
    // Grid operations should fail gracefully
    EXPECT_FALSE(manager->setVoxel(outOfBoundsPos, VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(manager->getVoxel(outOfBoundsPos, VoxelResolution::Size_1cm));
    
    // World operations should fail gracefully
    EXPECT_FALSE(manager->setVoxelAtWorldPos(outOfBoundsWorldPos, VoxelResolution::Size_1cm, true));
    EXPECT_FALSE(manager->getVoxelAtWorldPos(outOfBoundsWorldPos, VoxelResolution::Size_1cm));
    
    // Total voxel count should remain 0
    EXPECT_EQ(manager->getTotalVoxelCount(), 0);
    
    // No events should be dispatched for failed operations
    EXPECT_EQ(voxelChangedEventCount, 0);
}

TEST_F(VoxelDataManagerTest, WorkspaceResizeWithVoxels) {
    // Add voxels near edge of workspace
    Vector3f currentSize = manager->getWorkspaceSize();
    manager->setVoxel(Vector3i(100, 100, 100), VoxelResolution::Size_1cm, true);
    manager->setVoxel(Vector3i(200, 200, 200), VoxelResolution::Size_1cm, true);
    
    EXPECT_EQ(manager->getTotalVoxelCount(), 2);
    
    // Try to shrink workspace significantly
    bool resizeSuccess = manager->resizeWorkspace(Vector3f(2.0f, 2.0f, 2.0f));
    
    if (resizeSuccess) {
        // If resize succeeded, check that workspace changed
        EXPECT_EQ(manager->getWorkspaceSize(), Vector3f(2.0f, 2.0f, 2.0f));
        // Some voxels might have been lost
    } else {
        // If resize failed (due to voxel preservation), workspace should remain unchanged
        EXPECT_EQ(manager->getWorkspaceSize(), currentSize);
        EXPECT_EQ(manager->getTotalVoxelCount(), 2); // Voxels should be preserved
    }
}

TEST_F(VoxelDataManagerTest, LargeScaleOperations) {
    // Test with many voxels across multiple resolutions
    const int voxelsPerResolution = 50;
    const int numResolutions = 3;
    
    for (int res = 0; res < numResolutions; ++res) {
        VoxelResolution resolution = static_cast<VoxelResolution>(res);
        for (int i = 0; i < voxelsPerResolution; ++i) {
            Vector3i pos(i, i % 10, (i * 2) % 20);
            manager->setVoxel(pos, resolution, true);
        }
    }
    
    EXPECT_EQ(manager->getTotalVoxelCount(), voxelsPerResolution * numResolutions);
    
    // Test memory usage scales reasonably
    size_t totalMemory = manager->getMemoryUsage();
    EXPECT_GT(totalMemory, 0);
    
    // Test performance metrics with large dataset
    VoxelDataManager::PerformanceMetrics metrics = manager->getPerformanceMetrics();
    EXPECT_EQ(metrics.totalVoxels, voxelsPerResolution * numResolutions);
    EXPECT_GT(metrics.memoryEfficiency, 0.0f);
}