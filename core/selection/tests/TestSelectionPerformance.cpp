#include <gtest/gtest.h>
#include <chrono>
#include "core/selection/SelectionManager.h"
#include "core/selection/SelectionSet.h"
#include "core/selection/BoxSelector.h"
#include "core/selection/SphereSelector.h"
#include "core/selection/FloodFillSelector.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Selection;

class SelectionPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<SelectionManager>();
    }
    
    std::unique_ptr<SelectionManager> manager;
};

// Performance test for large selection operations
TEST_F(SelectionPerformanceTest, LargeSelectionAddRemove) {
    // REQ: Performance optimization for large selections
    const int numVoxels = 10000;
    std::vector<VoxelId> voxels;
    
    // Prepare voxels
    for (int i = 0; i < numVoxels; ++i) {
        int x = i % 100;
        int y = (i / 100) % 100;
        int z = i / 10000;
        voxels.emplace_back(Math::IncrementCoordinates(Math::Vector3i(x*4, y*4, z*4)), 
                           VoxelData::VoxelResolution::Size_4cm);
    }
    
    // Time adding voxels
    auto startTime = std::chrono::high_resolution_clock::now();
    for (const auto& voxel : voxels) {
        manager->selectVoxel(voxel);
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto addDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(manager->getSelectionSize(), numVoxels);
    EXPECT_LT(addDuration.count(), 5000);  // Should complete in less than 5 seconds
    
    // Time stats calculation
    startTime = std::chrono::high_resolution_clock::now();
    SelectionStats stats = manager->getSelectionStats();
    endTime = std::chrono::high_resolution_clock::now();
    auto statsDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(stats.voxelCount, numVoxels);
    EXPECT_LT(statsDuration.count(), 500);  // Stats should be fast
    
    // Time clearing
    startTime = std::chrono::high_resolution_clock::now();
    manager->selectNone();
    endTime = std::chrono::high_resolution_clock::now();
    auto clearDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(manager->getSelectionSize(), 0u);
    EXPECT_LT(clearDuration.count(), 100);  // Clearing should be very fast
}

// Performance test for box selection with high resolution
TEST_F(SelectionPerformanceTest, BoxSelectorHighResolution) {
    BoxSelector selector;
    
    // Large box with 1cm resolution (worst case)
    Math::BoundingBox box(
        Math::Vector3f(-0.5f, 0.0f, -0.5f),
        Math::Vector3f(0.5f, 0.5f, 0.5f)
    );
    
    auto startTime = std::chrono::high_resolution_clock::now();
    SelectionSet result = selector.selectFromWorld(box, VoxelData::VoxelResolution::Size_1cm, false);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Should select 100x50x100 = 500,000 voxels
    EXPECT_GT(result.size(), 100000u);
    EXPECT_LT(duration.count(), 10000);  // Should complete in less than 10 seconds
}

// Performance test for sphere selection with varying radii
TEST_F(SelectionPerformanceTest, SphereSelectorVaryingRadii) {
    SphereSelector selector;
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    
    std::vector<float> radii = {0.1f, 0.2f, 0.5f, 1.0f};
    std::vector<long> durations;
    
    for (float radius : radii) {
        auto startTime = std::chrono::high_resolution_clock::now();
        SelectionSet result = selector.selectFromSphere(center, radius, 
                                                       VoxelData::VoxelResolution::Size_4cm, false);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        durations.push_back(duration.count());
        EXPECT_GT(result.size(), 0u);
    }
    
    // Performance should scale reasonably with radius
    for (auto duration : durations) {
        EXPECT_LT(duration, 5000);  // Each should complete in less than 5 seconds
    }
}

// Performance test for selection set operations
TEST_F(SelectionPerformanceTest, SelectionSetOperations) {
    const int setSize = 5000;
    
    // Create two large selection sets
    SelectionSet set1, set2;
    for (int i = 0; i < setSize; ++i) {
        set1.add(VoxelId(Math::IncrementCoordinates(Math::Vector3i(i, 0, 0)), 
                        VoxelData::VoxelResolution::Size_4cm));
        set2.add(VoxelId(Math::IncrementCoordinates(Math::Vector3i(i/2, 0, 0)), 
                        VoxelData::VoxelResolution::Size_4cm));
    }
    
    // Time union operation
    auto startTime = std::chrono::high_resolution_clock::now();
    SelectionSet unionResult = set1.unionWith(set2);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto unionDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_LT(unionDuration.count(), 1000);  // Should be fast
    
    // Time intersection operation
    startTime = std::chrono::high_resolution_clock::now();
    SelectionSet intersectResult = set1.intersectWith(set2);
    endTime = std::chrono::high_resolution_clock::now();
    auto intersectDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_LT(intersectDuration.count(), 1000);  // Should be fast
    
    // Time filtering operation
    startTime = std::chrono::high_resolution_clock::now();
    auto filtered = set1.filter([](const VoxelId& v) { return v.position.value().x % 2 == 0; });
    endTime = std::chrono::high_resolution_clock::now();
    auto filterDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_LT(filterDuration.count(), 1000);  // Should be fast
}

// Performance test for undo/redo with large selections
TEST_F(SelectionPerformanceTest, UndoRedoLargeSelections) {
    const int historySize = 100;
    const int selectionSize = 1000;
    
    manager->setMaxHistorySize(historySize);
    
    // Create history with large selections
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int h = 0; h < historySize; ++h) {
        // Create a different selection each time
        for (int i = 0; i < selectionSize; ++i) {
            VoxelId voxel(Math::IncrementCoordinates(Math::Vector3i(i + h, 0, 0)), 
                         VoxelData::VoxelResolution::Size_4cm);
            manager->selectVoxel(voxel);
        }
        manager->pushSelectionToHistory();
        manager->selectNone();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto createDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_LT(createDuration.count(), 10000);  // Should complete in reasonable time
    
    // Time undo operations
    startTime = std::chrono::high_resolution_clock::now();
    int undoCount = 0;
    while (manager->canUndo() && undoCount < 50) {
        manager->undoSelection();
        undoCount++;
    }
    endTime = std::chrono::high_resolution_clock::now();
    auto undoDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(undoCount, 50);
    EXPECT_LT(undoDuration.count(), 2000);  // Undo should be fast
    
    // Time redo operations
    startTime = std::chrono::high_resolution_clock::now();
    int redoCount = 0;
    while (manager->canRedo() && redoCount < 50) {
        manager->redoSelection();
        redoCount++;
    }
    endTime = std::chrono::high_resolution_clock::now();
    auto redoDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(redoCount, 50);
    EXPECT_LT(redoDuration.count(), 2000);  // Redo should be fast
}

// Stress test for flood fill with maximum limits
TEST_F(SelectionPerformanceTest, FloodFillMaximumStress) {
    FloodFillSelector selector;
    selector.setMaxVoxels(10000);  // Limit to prevent infinite expansion
    
    VoxelId seed(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), 
                VoxelData::VoxelResolution::Size_8cm);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    SelectionSet result = selector.selectFloodFill(seed, FloodFillCriteria::Connected6);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(result.size(), 10000u);  // Should hit the limit
    EXPECT_LT(duration.count(), 30000);  // Should complete within 30 seconds even at limit
}