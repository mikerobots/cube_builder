#include <gtest/gtest.h>
#include <chrono>
#include "../VoxelDataManager.h"
#include "../../../foundation/math/CoordinateTypes.h"
#include "../../../foundation/math/CoordinateConverter.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

class VoxelDataBatchOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        voxelManager = std::make_unique<VoxelDataManager>();
        // Set default workspace size to 5m (500cm)
        voxelManager->resizeWorkspace(5.0f);
    }
    
    std::unique_ptr<VoxelDataManager> voxelManager;
};

// Test VoxelChange struct
TEST_F(VoxelDataBatchOperationsTest, VoxelChangeConstruction) {
    IncrementCoordinates pos(10, 20, 30);
    VoxelChange change(pos, VoxelResolution::Size_4cm, false, true);
    
    EXPECT_EQ(change.position.x(), 10);
    EXPECT_EQ(change.position.y(), 20);
    EXPECT_EQ(change.position.z(), 30);
    EXPECT_EQ(change.resolution, VoxelResolution::Size_4cm);
    EXPECT_FALSE(change.oldValue);
    EXPECT_TRUE(change.newValue);
}

// Test BatchResult struct
TEST_F(VoxelDataBatchOperationsTest, BatchResultDefaultConstruction) {
    BatchResult result;
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.totalOperations, 0u);
    EXPECT_EQ(result.successfulOperations, 0u);
    EXPECT_EQ(result.failedOperations, 0u);
    EXPECT_TRUE(result.errorMessage.empty());
    EXPECT_TRUE(result.failedIndices.empty());
    EXPECT_TRUE(result.failureReasons.empty());
}

// Test createBatchChanges method
TEST_F(VoxelDataBatchOperationsTest, CreateBatchChanges_EmptyPositions) {
    std::vector<IncrementCoordinates> positions;
    auto changes = voxelManager->createBatchChanges(positions, VoxelResolution::Size_1cm, true);
    
    EXPECT_TRUE(changes.empty());
}

TEST_F(VoxelDataBatchOperationsTest, CreateBatchChanges_ValidPositions) {
    std::vector<IncrementCoordinates> positions = {
        IncrementCoordinates(0, 0, 0),
        IncrementCoordinates(1, 0, 0),
        IncrementCoordinates(0, 1, 0)
    };
    
    auto changes = voxelManager->createBatchChanges(positions, VoxelResolution::Size_1cm, true);
    
    EXPECT_EQ(changes.size(), 3u);
    
    for (size_t i = 0; i < changes.size(); ++i) {
        EXPECT_EQ(changes[i].position, positions[i]);
        EXPECT_EQ(changes[i].resolution, VoxelResolution::Size_1cm);
        EXPECT_FALSE(changes[i].oldValue);  // Should be false initially
        EXPECT_TRUE(changes[i].newValue);
    }
}

TEST_F(VoxelDataBatchOperationsTest, CreateBatchChanges_WithExistingVoxels) {
    // Place some voxels first
    voxelManager->setVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, true);
    voxelManager->setVoxel(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm, true);
    
    std::vector<IncrementCoordinates> positions = {
        IncrementCoordinates(0, 0, 0),  // Existing voxel
        IncrementCoordinates(1, 0, 0),  // Existing voxel
        IncrementCoordinates(2, 0, 0)   // New position
    };
    
    auto changes = voxelManager->createBatchChanges(positions, VoxelResolution::Size_1cm, false);
    
    EXPECT_EQ(changes.size(), 3u);
    EXPECT_TRUE(changes[0].oldValue);   // Should detect existing voxel
    EXPECT_TRUE(changes[1].oldValue);   // Should detect existing voxel
    EXPECT_FALSE(changes[2].oldValue);  // Should detect empty position
    
    for (const auto& change : changes) {
        EXPECT_FALSE(change.newValue);  // All should be set to false
    }
}

// Test batchValidate method
TEST_F(VoxelDataBatchOperationsTest, BatchValidate_AllValid) {
    std::vector<VoxelChange> changes = {
        VoxelChange(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, false, true),
        VoxelChange(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm, false, true),
        VoxelChange(IncrementCoordinates(0, 1, 0), VoxelResolution::Size_1cm, false, true)
    };
    
    std::vector<PositionValidation> validations;
    bool allValid = voxelManager->batchValidate(changes, validations);
    
    EXPECT_TRUE(allValid);
    EXPECT_EQ(validations.size(), 3u);
    
    for (const auto& validation : validations) {
        EXPECT_TRUE(validation.valid);
        EXPECT_TRUE(validation.aboveGroundPlane);
        EXPECT_TRUE(validation.withinBounds);
        EXPECT_TRUE(validation.alignedToGrid);
        EXPECT_TRUE(validation.noOverlap);
    }
}

TEST_F(VoxelDataBatchOperationsTest, BatchValidate_SomeInvalid) {
    std::vector<VoxelChange> changes = {
        VoxelChange(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, false, true),    // Valid
        VoxelChange(IncrementCoordinates(0, -1, 0), VoxelResolution::Size_1cm, false, true),  // Below ground
        VoxelChange(IncrementCoordinates(1000, 0, 0), VoxelResolution::Size_1cm, false, true) // Out of bounds
    };
    
    std::vector<PositionValidation> validations;
    bool allValid = voxelManager->batchValidate(changes, validations);
    
    EXPECT_FALSE(allValid);
    EXPECT_EQ(validations.size(), 3u);
    
    EXPECT_TRUE(validations[0].valid);
    EXPECT_FALSE(validations[1].valid);
    EXPECT_FALSE(validations[1].aboveGroundPlane);
    EXPECT_FALSE(validations[2].valid);
    EXPECT_FALSE(validations[2].withinBounds);
}

// Test batchSetVoxels method
TEST_F(VoxelDataBatchOperationsTest, BatchSetVoxels_AllValid) {
    std::vector<VoxelChange> changes = {
        VoxelChange(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, false, true),
        VoxelChange(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm, false, true),
        VoxelChange(IncrementCoordinates(0, 1, 0), VoxelResolution::Size_1cm, false, true)
    };
    
    auto result = voxelManager->batchSetVoxels(changes);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.totalOperations, 3u);
    EXPECT_EQ(result.successfulOperations, 3u);
    EXPECT_EQ(result.failedOperations, 0u);
    EXPECT_TRUE(result.errorMessage.empty());
    EXPECT_TRUE(result.failedIndices.empty());
    
    // Verify voxels were actually set
    EXPECT_TRUE(voxelManager->getVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm));
    EXPECT_TRUE(voxelManager->getVoxel(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm));
    EXPECT_TRUE(voxelManager->getVoxel(IncrementCoordinates(0, 1, 0), VoxelResolution::Size_1cm));
}

TEST_F(VoxelDataBatchOperationsTest, BatchSetVoxels_WithValidationFailures) {
    std::vector<VoxelChange> changes = {
        VoxelChange(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, false, true),    // Valid
        VoxelChange(IncrementCoordinates(0, -1, 0), VoxelResolution::Size_1cm, false, true),  // Below ground
        VoxelChange(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm, false, true)    // Valid
    };
    
    auto result = voxelManager->batchSetVoxels(changes);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.totalOperations, 3u);
    EXPECT_EQ(result.successfulOperations, 0u);  // Atomicity: none should succeed
    EXPECT_EQ(result.failedOperations, 1u);      // Only one failed validation
    EXPECT_FALSE(result.errorMessage.empty());
    EXPECT_EQ(result.failedIndices.size(), 1u);
    EXPECT_EQ(result.failedIndices[0], 1u);      // Second operation failed
    
    // Verify no voxels were set due to atomicity
    EXPECT_FALSE(voxelManager->getVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm));
    EXPECT_FALSE(voxelManager->getVoxel(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm));
}

TEST_F(VoxelDataBatchOperationsTest, BatchSetVoxels_RemoveVoxels) {
    // First place some voxels
    voxelManager->setVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, true);
    voxelManager->setVoxel(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm, true);
    voxelManager->setVoxel(IncrementCoordinates(0, 1, 0), VoxelResolution::Size_1cm, true);
    
    // Now remove them in batch
    std::vector<VoxelChange> changes = {
        VoxelChange(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, true, false),
        VoxelChange(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm, true, false),
        VoxelChange(IncrementCoordinates(0, 1, 0), VoxelResolution::Size_1cm, true, false)
    };
    
    auto result = voxelManager->batchSetVoxels(changes);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.successfulOperations, 3u);
    
    // Verify voxels were removed
    EXPECT_FALSE(voxelManager->getVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm));
    EXPECT_FALSE(voxelManager->getVoxel(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm));
    EXPECT_FALSE(voxelManager->getVoxel(IncrementCoordinates(0, 1, 0), VoxelResolution::Size_1cm));
}

TEST_F(VoxelDataBatchOperationsTest, BatchSetVoxels_NoChangesNeeded) {
    // Test when voxels are already in the desired state
    std::vector<VoxelChange> changes = {
        VoxelChange(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, false, false),
        VoxelChange(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_1cm, false, false)
    };
    
    auto result = voxelManager->batchSetVoxels(changes);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.totalOperations, 2u);
    EXPECT_EQ(result.successfulOperations, 2u);  // Should count as successful even if no change
    EXPECT_EQ(result.failedOperations, 0u);
}

TEST_F(VoxelDataBatchOperationsTest, BatchSetVoxels_WithOverlaps) {
    // Place a voxel first
    voxelManager->setVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, true);
    
    // Try to place overlapping voxels
    std::vector<VoxelChange> changes = {
        VoxelChange(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_4cm, false, true),  // Would overlap
        VoxelChange(IncrementCoordinates(4, 0, 0), VoxelResolution::Size_4cm, false, true)   // Valid
    };
    
    auto result = voxelManager->batchSetVoxels(changes);
    
    EXPECT_FALSE(result.success);
    EXPECT_GT(result.failedOperations, 0u);
    
    // Verify atomicity - second voxel should not be placed
    EXPECT_FALSE(voxelManager->getVoxel(IncrementCoordinates(4, 0, 0), VoxelResolution::Size_4cm));
}

// Test atomicity with medium batches
TEST_F(VoxelDataBatchOperationsTest, BatchSetVoxels_AtomicityMediumBatch) {
    // Create a medium batch with one invalid operation at the end
    std::vector<VoxelChange> changes;
    
    // Add many valid changes
    for (int i = 0; i < 20; ++i) {
        changes.emplace_back(IncrementCoordinates(i, 0, 0), VoxelResolution::Size_1cm, false, true);
    }
    
    // Add one invalid change at the end
    changes.emplace_back(IncrementCoordinates(0, -1, 0), VoxelResolution::Size_1cm, false, true);
    
    auto result = voxelManager->batchSetVoxels(changes);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.totalOperations, 21u);
    EXPECT_EQ(result.successfulOperations, 0u);  // None should succeed due to atomicity
    EXPECT_EQ(result.failedOperations, 1u);
    
    // Verify none of the valid voxels were placed
    for (int i = 0; i < 20; ++i) {
        EXPECT_FALSE(voxelManager->getVoxel(IncrementCoordinates(i, 0, 0), VoxelResolution::Size_1cm));
    }
}

// Test mixed resolutions
TEST_F(VoxelDataBatchOperationsTest, BatchSetVoxels_MixedResolutions) {
    std::vector<VoxelChange> changes = {
        VoxelChange(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm, false, true),
        VoxelChange(IncrementCoordinates(4, 0, 0), VoxelResolution::Size_4cm, false, true),
        VoxelChange(IncrementCoordinates(0, 16, 0), VoxelResolution::Size_16cm, false, true)
    };
    
    auto result = voxelManager->batchSetVoxels(changes);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.successfulOperations, 3u);
    
    // Verify voxels were placed with correct resolutions
    EXPECT_TRUE(voxelManager->getVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_1cm));
    EXPECT_TRUE(voxelManager->getVoxel(IncrementCoordinates(4, 0, 0), VoxelResolution::Size_4cm));
    EXPECT_TRUE(voxelManager->getVoxel(IncrementCoordinates(0, 16, 0), VoxelResolution::Size_16cm));
}

// Test empty batch
TEST_F(VoxelDataBatchOperationsTest, BatchSetVoxels_EmptyBatch) {
    std::vector<VoxelChange> changes;
    auto result = voxelManager->batchSetVoxels(changes);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.totalOperations, 0u);
    EXPECT_EQ(result.successfulOperations, 0u);
    EXPECT_EQ(result.failedOperations, 0u);
}

// Performance test (basic)
TEST_F(VoxelDataBatchOperationsTest, BatchSetVoxels_Performance) {
    // Create a small batch for quick testing
    std::vector<VoxelChange> changes;
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            for (int z = 0; z < 5; ++z) {
                changes.emplace_back(IncrementCoordinates(x, y, z), VoxelResolution::Size_1cm, false, true);
            }
        }
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = voxelManager->batchSetVoxels(changes);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.successfulOperations, 125u);
    
    // Should complete quickly
    EXPECT_LT(duration.count(), 100);  // Less than 100ms
}

// Test convenience method integration
TEST_F(VoxelDataBatchOperationsTest, ConvenienceMethod_Integration) {
    std::vector<IncrementCoordinates> positions = {
        IncrementCoordinates(0, 0, 0),
        IncrementCoordinates(1, 0, 0),
        IncrementCoordinates(0, 1, 0)
    };
    
    // Create batch and execute
    auto changes = voxelManager->createBatchChanges(positions, VoxelResolution::Size_1cm, true);
    auto result = voxelManager->batchSetVoxels(changes);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.successfulOperations, 3u);
    
    // Verify all voxels were set
    for (const auto& pos : positions) {
        EXPECT_TRUE(voxelManager->getVoxel(pos, VoxelResolution::Size_1cm));
    }
}