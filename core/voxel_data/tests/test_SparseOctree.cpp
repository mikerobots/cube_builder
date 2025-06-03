#include <gtest/gtest.h>
#include "../SparseOctree.h"

using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class SparseOctreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize memory pool before each test
        SparseOctree::initializePool(256);
    }
    
    void TearDown() override {
        // Clean up pool after each test
        SparseOctree::shutdownPool();
    }
};

TEST_F(SparseOctreeTest, DefaultConstruction) {
    SparseOctree octree;
    
    // Default octree should be empty
    EXPECT_FALSE(octree.getVoxel(Vector3i(0, 0, 0)));
    EXPECT_FALSE(octree.getVoxel(Vector3i(1, 1, 1)));
    EXPECT_FALSE(octree.getVoxel(Vector3i(100, 100, 100)));
    
    EXPECT_EQ(octree.getVoxelCount(), 0);
    // Empty octree has no allocated nodes (lazy allocation)
    EXPECT_EQ(octree.getMemoryUsage(), 0);
}

TEST_F(SparseOctreeTest, CustomDepthConstruction) {
    SparseOctree octree(5); // Depth 5 supports 32x32x32 grid
    
    // Should handle positions within depth range
    EXPECT_FALSE(octree.getVoxel(Vector3i(31, 31, 31)));
    EXPECT_FALSE(octree.getVoxel(Vector3i(0, 0, 0)));
}

TEST_F(SparseOctreeTest, SingleVoxelOperations) {
    SparseOctree octree;
    Vector3i pos(10, 20, 30);
    
    // Initially false
    EXPECT_FALSE(octree.getVoxel(pos));
    EXPECT_EQ(octree.getVoxelCount(), 0);
    
    // Set voxel
    EXPECT_TRUE(octree.setVoxel(pos, true));
    EXPECT_TRUE(octree.getVoxel(pos));
    EXPECT_EQ(octree.getVoxelCount(), 1);
    
    // Clear voxel
    EXPECT_TRUE(octree.setVoxel(pos, false));
    EXPECT_FALSE(octree.getVoxel(pos));
    EXPECT_EQ(octree.getVoxelCount(), 0);
}

TEST_F(SparseOctreeTest, MultipleVoxelOperations) {
    SparseOctree octree;
    
    std::vector<Vector3i> positions = {
        Vector3i(0, 0, 0),
        Vector3i(1, 1, 1),
        Vector3i(10, 20, 30),
        Vector3i(100, 200, 300),
        Vector3i(500, 600, 700)
    };
    
    // Set all voxels
    for (const auto& pos : positions) {
        EXPECT_TRUE(octree.setVoxel(pos, true));
        EXPECT_TRUE(octree.getVoxel(pos));
    }
    
    EXPECT_EQ(octree.getVoxelCount(), positions.size());
    
    // Verify all are still set
    for (const auto& pos : positions) {
        EXPECT_TRUE(octree.getVoxel(pos));
    }
    
    // Clear some voxels
    EXPECT_TRUE(octree.setVoxel(positions[1], false));
    EXPECT_TRUE(octree.setVoxel(positions[3], false));
    
    EXPECT_EQ(octree.getVoxelCount(), positions.size() - 2);
    
    // Verify correct voxels are cleared
    EXPECT_TRUE(octree.getVoxel(positions[0]));
    EXPECT_FALSE(octree.getVoxel(positions[1]));
    EXPECT_TRUE(octree.getVoxel(positions[2]));
    EXPECT_FALSE(octree.getVoxel(positions[3]));
    EXPECT_TRUE(octree.getVoxel(positions[4]));
}

TEST_F(SparseOctreeTest, NegativeCoordinates) {
    SparseOctree octree;
    
    // SparseOctree doesn't support negative coordinates by design
    // Test that negative coordinates are properly rejected
    std::vector<Vector3i> negativePositions = {
        Vector3i(-1, -1, -1),
        Vector3i(-10, -20, -30),
        Vector3i(-100, 50, -200),
        Vector3i(100, -200, 300)
    };
    
    for (const auto& pos : negativePositions) {
        EXPECT_FALSE(octree.setVoxel(pos, true));
        EXPECT_FALSE(octree.getVoxel(pos));
    }
    
    EXPECT_EQ(octree.getVoxelCount(), 0);
}

TEST_F(SparseOctreeTest, LargeCoordinates) {
    SparseOctree octree;
    
    // SparseOctree has a maximum coordinate range based on root size (default 1024)
    // Test that coordinates outside this range are properly rejected
    std::vector<Vector3i> largePositions = {
        Vector3i(1000000, 1000000, 1000000),
        Vector3i(-1000000, 1000000, -1000000),
        Vector3i(0, 2000000, 0)
    };
    
    for (const auto& pos : largePositions) {
        EXPECT_FALSE(octree.setVoxel(pos, true));
        EXPECT_FALSE(octree.getVoxel(pos));
    }
    
    EXPECT_EQ(octree.getVoxelCount(), 0);
    
    // Test valid large coordinates within bounds
    std::vector<Vector3i> validLargePositions = {
        Vector3i(1000, 1000, 1000),
        Vector3i(500, 800, 900),
        Vector3i(1023, 1023, 1023)  // Max valid coordinate for default size
    };
    
    for (const auto& pos : validLargePositions) {
        EXPECT_TRUE(octree.setVoxel(pos, true));
        EXPECT_TRUE(octree.getVoxel(pos));
    }
    
    EXPECT_EQ(octree.getVoxelCount(), validLargePositions.size());
}

TEST_F(SparseOctreeTest, RedundantOperations) {
    SparseOctree octree;
    Vector3i pos(50, 50, 50);
    
    // Setting true multiple times
    EXPECT_TRUE(octree.setVoxel(pos, true));
    size_t memAfterFirst = octree.getMemoryUsage();
    
    EXPECT_TRUE(octree.setVoxel(pos, true)); // Should succeed but not change state
    EXPECT_TRUE(octree.getVoxel(pos));
    EXPECT_EQ(octree.getVoxelCount(), 1);
    EXPECT_EQ(octree.getMemoryUsage(), memAfterFirst); // Memory shouldn't increase
    
    // Setting false to remove the voxel
    EXPECT_TRUE(octree.setVoxel(pos, false));
    EXPECT_FALSE(octree.getVoxel(pos));
    EXPECT_EQ(octree.getVoxelCount(), 0);
    
    // Setting false on non-existent voxel returns false (voxel doesn't exist)
    EXPECT_FALSE(octree.setVoxel(pos, false)); // Returns false as voxel doesn't exist
    EXPECT_FALSE(octree.getVoxel(pos));
    EXPECT_EQ(octree.getVoxelCount(), 0);
}

TEST_F(SparseOctreeTest, ClearOperation) {
    SparseOctree octree;
    
    // Add multiple voxels
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(octree.setVoxel(Vector3i(i, i, i), true));
    }
    
    EXPECT_EQ(octree.getVoxelCount(), 10);
    size_t memoryWithVoxels = octree.getMemoryUsage();
    
    // Clear all
    octree.clear();
    
    EXPECT_EQ(octree.getVoxelCount(), 0);
    EXPECT_LT(octree.getMemoryUsage(), memoryWithVoxels); // Should use less memory
    
    // Verify all voxels are gone
    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(octree.getVoxel(Vector3i(i, i, i)));
    }
}

TEST_F(SparseOctreeTest, OptimizationOperation) {
    SparseOctree octree;
    
    // Add many voxels
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            for (int z = 0; z < 8; ++z) {
                octree.setVoxel(Vector3i(x, y, z), true);
            }
        }
    }
    
    size_t memoryBeforeOptimize = octree.getMemoryUsage();
    size_t voxelCount = octree.getVoxelCount();
    
    // Optimize should succeed
    octree.optimize();
    
    // Voxel count should remain the same
    EXPECT_EQ(octree.getVoxelCount(), voxelCount);
    
    // All voxels should still be accessible
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            for (int z = 0; z < 8; ++z) {
                EXPECT_TRUE(octree.getVoxel(Vector3i(x, y, z)));
            }
        }
    }
    
    // Memory usage might change (could increase or decrease depending on optimization)
    EXPECT_GT(octree.getMemoryUsage(), 0);
}

TEST_F(SparseOctreeTest, MemoryPoolOperations) {
    // Test that multiple octrees can share the pool
    SparseOctree octree1;
    SparseOctree octree2;
    
    octree1.setVoxel(Vector3i(10, 10, 10), true);
    octree2.setVoxel(Vector3i(20, 20, 20), true);
    
    EXPECT_TRUE(octree1.getVoxel(Vector3i(10, 10, 10)));
    EXPECT_FALSE(octree1.getVoxel(Vector3i(20, 20, 20)));
    
    EXPECT_FALSE(octree2.getVoxel(Vector3i(10, 10, 10)));
    EXPECT_TRUE(octree2.getVoxel(Vector3i(20, 20, 20)));
}

TEST_F(SparseOctreeTest, NonCopyableDesign) {
    SparseOctree octree;
    
    // Add some voxels
    std::vector<Vector3i> positions = {
        Vector3i(1, 2, 3),
        Vector3i(10, 20, 30),
        Vector3i(100, 200, 300)
    };
    
    for (const auto& pos : positions) {
        octree.setVoxel(pos, true);
    }
    
    EXPECT_EQ(octree.getVoxelCount(), positions.size());
    
    // Verify octree is non-copyable (this is a design test)
    // These lines would not compile:
    // SparseOctree copied(octree);  // Deleted copy constructor
    // SparseOctree assigned = octree; // Deleted copy assignment
    
    for (const auto& pos : positions) {
        EXPECT_TRUE(octree.getVoxel(pos));
    }
}

TEST_F(SparseOctreeTest, StressTestLargeDataset) {
    SparseOctree octree;
    
    const int gridSize = 50;
    size_t expectedVoxels = 0;
    
    // Add voxels in a checkerboard pattern
    for (int x = 0; x < gridSize; ++x) {
        for (int y = 0; y < gridSize; ++y) {
            for (int z = 0; z < gridSize; ++z) {
                if ((x + y + z) % 2 == 0) {
                    octree.setVoxel(Vector3i(x, y, z), true);
                    expectedVoxels++;
                }
            }
        }
    }
    
    EXPECT_EQ(octree.getVoxelCount(), expectedVoxels);
    
    // Verify the pattern
    for (int x = 0; x < gridSize; ++x) {
        for (int y = 0; y < gridSize; ++y) {
            for (int z = 0; z < gridSize; ++z) {
                bool shouldBeSet = (x + y + z) % 2 == 0;
                EXPECT_EQ(octree.getVoxel(Vector3i(x, y, z)), shouldBeSet);
            }
        }
    }
}

TEST_F(SparseOctreeTest, MemoryUsageTracking) {
    SparseOctree octree;
    
    size_t initialMemory = octree.getMemoryUsage();
    EXPECT_EQ(initialMemory, 0); // Empty octree has no nodes allocated
    
    // Add voxels and verify memory increases
    octree.setVoxel(Vector3i(0, 0, 0), true);
    size_t memoryAfterOne = octree.getMemoryUsage();
    EXPECT_GE(memoryAfterOne, initialMemory);
    
    // Add more voxels in different octants
    octree.setVoxel(Vector3i(100, 100, 100), true);
    octree.setVoxel(Vector3i(-100, -100, -100), true);
    octree.setVoxel(Vector3i(100, -100, 100), true);
    
    size_t memoryAfterMultiple = octree.getMemoryUsage();
    EXPECT_GT(memoryAfterMultiple, memoryAfterOne);
    
    // Clear and verify memory decreases
    octree.clear();
    size_t memoryAfterClear = octree.getMemoryUsage();
    EXPECT_LE(memoryAfterClear, initialMemory);
}

TEST_F(SparseOctreeTest, ClusteringEfficiency) {
    SparseOctree octree;
    
    // Add a cluster of voxels (should be memory efficient)
    for (int x = 0; x < 4; ++x) {
        for (int y = 0; y < 4; ++y) {
            for (int z = 0; z < 4; ++z) {
                octree.setVoxel(Vector3i(x, y, z), true);
            }
        }
    }
    
    size_t clusteredMemory = octree.getMemoryUsage();
    size_t clusteredVoxels = octree.getVoxelCount();
    
    // Clear and add scattered voxels (should be less memory efficient)
    octree.clear();
    
    std::vector<Vector3i> scatteredPositions = {
        Vector3i(0, 0, 0),
        Vector3i(1000, 1000, 1000),
        Vector3i(-1000, 500, -500),
        Vector3i(2000, -2000, 2000)
    };
    
    for (const auto& pos : scatteredPositions) {
        octree.setVoxel(pos, true);
    }
    
    size_t scatteredMemory = octree.getMemoryUsage();
    size_t scatteredVoxels = octree.getVoxelCount();
    
    // Scattered voxels should use more memory per voxel than clustered
    float clusteredEfficiency = static_cast<float>(clusteredVoxels) / clusteredMemory;
    float scatteredEfficiency = static_cast<float>(scatteredVoxels) / scatteredMemory;
    
    EXPECT_GT(clusteredEfficiency, scatteredEfficiency);
}