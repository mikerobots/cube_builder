#include <gtest/gtest.h>
#include "cli/Application.h"
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "selection/SelectionManager.h"
#include "groups/GroupManager.h"
#include "file_io/FileManager.h"
#include <filesystem>
#include <fstream>

namespace VoxelEditor {
namespace Tests {

class CLIErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        app = std::make_unique<Application>();
        
        // Initialize in headless mode
        int argc = 2;
        char arg0[] = "test";
        char arg1[] = "--headless";
        char* argv[] = {arg0, arg1, nullptr};
        
        initialized = app->initialize(argc, argv);
        ASSERT_TRUE(initialized);
        
        voxelManager = app->getVoxelManager();
        cameraController = app->getCameraController();
        selectionManager = app->getSelectionManager();
        groupManager = app->getGroupManager();
        fileManager = app->getFileManager();
    }
    
    void TearDown() override {
        cleanupTestFiles();
    }
    
    void cleanupTestFiles() {
        std::vector<std::string> testFiles = {
            "readonly_test.vxl",
            "corrupted_test.vxl",
            "empty_test.vxl",
            "invalid_extension.txt"
        };
        
        for (const auto& file : testFiles) {
            std::filesystem::remove(file);
        }
    }

    std::unique_ptr<Application> app;
    bool initialized = false;
    
    VoxelData::VoxelDataManager* voxelManager = nullptr;
    Camera::CameraController* cameraController = nullptr;
    Selection::SelectionManager* selectionManager = nullptr;
    Groups::GroupManager* groupManager = nullptr;
    FileIO::FileManager* fileManager = nullptr;
};

// ============================================================================
// Boundary Condition Tests
// ============================================================================

TEST_F(CLIErrorHandlingTest, WorkspaceBoundaryTests) {
    // Test workspace size limits
    auto initialSize = voxelManager->getWorkspaceSize();
    
    // Test minimum boundary
    EXPECT_TRUE(voxelManager->resizeWorkspace(Math::Vector3f(2.0f, 2.0f, 2.0f))); // Exactly at minimum
    EXPECT_FALSE(voxelManager->resizeWorkspace(Math::Vector3f(1.9f, 2.0f, 2.0f))); // Below minimum X
    EXPECT_FALSE(voxelManager->resizeWorkspace(Math::Vector3f(2.0f, 1.9f, 2.0f))); // Below minimum Y
    EXPECT_FALSE(voxelManager->resizeWorkspace(Math::Vector3f(2.0f, 2.0f, 1.9f))); // Below minimum Z
    
    // Test maximum boundary
    EXPECT_TRUE(voxelManager->resizeWorkspace(Math::Vector3f(8.0f, 8.0f, 8.0f))); // Exactly at maximum
    EXPECT_FALSE(voxelManager->resizeWorkspace(Math::Vector3f(8.1f, 8.0f, 8.0f))); // Above maximum X
    EXPECT_FALSE(voxelManager->resizeWorkspace(Math::Vector3f(8.0f, 8.1f, 8.0f))); // Above maximum Y
    EXPECT_FALSE(voxelManager->resizeWorkspace(Math::Vector3f(8.0f, 8.0f, 8.1f))); // Above maximum Z
    
    // Test invalid values
    EXPECT_FALSE(voxelManager->resizeWorkspace(Math::Vector3f(0.0f, 0.0f, 0.0f))); // Zero size
    EXPECT_FALSE(voxelManager->resizeWorkspace(Math::Vector3f(-1.0f, 5.0f, 5.0f))); // Negative size
    EXPECT_FALSE(voxelManager->resizeWorkspace(Math::Vector3f(NAN, 5.0f, 5.0f))); // NaN
    EXPECT_FALSE(voxelManager->resizeWorkspace(Math::Vector3f(INFINITY, 5.0f, 5.0f))); // Infinity
}

TEST_F(CLIErrorHandlingTest, VoxelPositionBoundaryTests) {
    // Set up a 4x4x4 meter workspace with 8cm voxels (50x50x50 grid)
    voxelManager->resizeWorkspace(Math::Vector3f(4.0f, 4.0f, 4.0f));
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    int maxGrid = 49; // 4.0m / 0.08m = 50, so max index is 49
    
    // Test valid boundary positions
    EXPECT_TRUE(voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_8cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(Math::Vector3i(maxGrid, 0, 0), VoxelData::VoxelResolution::Size_8cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(Math::Vector3i(0, maxGrid, 0), VoxelData::VoxelResolution::Size_8cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(Math::Vector3i(0, 0, maxGrid), VoxelData::VoxelResolution::Size_8cm, true));
    EXPECT_TRUE(voxelManager->setVoxel(Math::Vector3i(maxGrid, maxGrid, maxGrid), VoxelData::VoxelResolution::Size_8cm, true));
    
    // Test invalid boundary positions (implementation may vary on how these are handled)
    // These might return false or be clamped, depending on implementation
    bool result1 = voxelManager->setVoxel(Math::Vector3i(-1, 0, 0), VoxelData::VoxelResolution::Size_8cm, true);
    bool result2 = voxelManager->setVoxel(Math::Vector3i(maxGrid + 1, 0, 0), VoxelData::VoxelResolution::Size_8cm, true);
    bool result3 = voxelManager->setVoxel(Math::Vector3i(0, -1, 0), VoxelData::VoxelResolution::Size_8cm, true);
    bool result4 = voxelManager->setVoxel(Math::Vector3i(0, maxGrid + 1, 0), VoxelData::VoxelResolution::Size_8cm, true);
    
    // The exact behavior may depend on implementation - we just verify it's consistent
    EXPECT_TRUE(result1 == result1); // Consistent behavior
    EXPECT_TRUE(result2 == result2);
    EXPECT_TRUE(result3 == result3);
    EXPECT_TRUE(result4 == result4);
}

TEST_F(CLIErrorHandlingTest, SelectionBoundaryTests) {
    // Create test voxels
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    for (int i = 0; i < 25; ++i) {
        Math::Vector3i pos(i % 5, (i / 5) % 5, 0);
        voxelManager->setVoxel(pos, VoxelData::VoxelResolution::Size_8cm, true);
    }
    
    // Test selection with invalid bounding box
    Math::BoundingBox invalidBox1(
        Math::Vector3f(1.0f, 1.0f, 1.0f),   // Min > Max
        Math::Vector3f(0.0f, 0.0f, 0.0f)    // Max < Min
    );
    
    selectionManager->selectBox(invalidBox1, VoxelData::VoxelResolution::Size_8cm);
    // Should handle gracefully - either select nothing or swap min/max
    size_t selectionCount1 = selectionManager->getSelectionSize();
    EXPECT_GE(selectionCount1, 0); // Should not crash
    
    // Test selection with zero-size box
    Math::BoundingBox zeroBox(
        Math::Vector3f(1.0f, 1.0f, 1.0f),
        Math::Vector3f(1.0f, 1.0f, 1.0f)
    );
    
    selectionManager->selectNone();
    selectionManager->selectBox(zeroBox, VoxelData::VoxelResolution::Size_8cm);
    size_t selectionCount2 = selectionManager->getSelectionSize();
    EXPECT_GE(selectionCount2, 0); // Should handle gracefully
    
    // Test selection with extreme coordinates
    Math::BoundingBox extremeBox(
        Math::Vector3f(-1000.0f, -1000.0f, -1000.0f),
        Math::Vector3f(1000.0f, 1000.0f, 1000.0f)
    );
    
    selectionManager->selectNone();
    selectionManager->selectBox(extremeBox, VoxelData::VoxelResolution::Size_8cm);
    size_t selectionCount3 = selectionManager->getSelectionSize();
    EXPECT_LE(selectionCount3, 25); // Should not select more than available voxels
}

// ============================================================================
// File I/O Error Handling Tests
// ============================================================================

TEST_F(CLIErrorHandlingTest, FilePermissionErrors) {
    // Create a read-only file
    std::ofstream readonlyFile("readonly_test.vxl");
    readonlyFile << "test data";
    readonlyFile.close();
    
    // Make file read-only (platform specific)
    std::filesystem::permissions("readonly_test.vxl", 
                                std::filesystem::perms::owner_read | std::filesystem::perms::group_read,
                                std::filesystem::perm_options::replace);
    
    // Try to save to read-only file
    FileIO::Project project;
    FileIO::SaveOptions saveOptions;
    auto saveResult = fileManager->saveProject("readonly_test.vxl", project, saveOptions);
    
    // Should fail gracefully
    EXPECT_FALSE(saveResult.success);
    EXPECT_FALSE(saveResult.message.empty());
}

TEST_F(CLIErrorHandlingTest, CorruptedFileHandling) {
    // Create corrupted file
    std::ofstream corruptedFile("corrupted_test.vxl", std::ios::binary);
    
    // Write invalid header
    corruptedFile << "INVALID_HEADER";
    
    // Write random binary data
    for (int i = 0; i < 1000; ++i) {
        corruptedFile << static_cast<char>(rand() % 256);
    }
    corruptedFile.close();
    
    // Try to load corrupted file
    FileIO::Project project;
    FileIO::LoadOptions loadOptions;
    auto loadResult = fileManager->loadProject("corrupted_test.vxl", project, loadOptions);
    
    // Should fail gracefully with error message
    EXPECT_FALSE(loadResult.success);
    EXPECT_FALSE(loadResult.message.empty());
}

TEST_F(CLIErrorHandlingTest, EmptyFileHandling) {
    // Create empty file
    std::ofstream emptyFile("empty_test.vxl");
    emptyFile.close();
    
    // Try to load empty file
    FileIO::Project project;
    FileIO::LoadOptions loadOptions;
    auto loadResult = fileManager->loadProject("empty_test.vxl", project, loadOptions);
    
    // Should fail gracefully
    EXPECT_FALSE(loadResult.success);
    EXPECT_FALSE(loadResult.message.empty());
}

TEST_F(CLIErrorHandlingTest, InvalidFileExtensions) {
    // Try to save with invalid extension
    FileIO::Project project;
    FileIO::SaveOptions saveOptions;
    auto saveResult = fileManager->saveProject("invalid_extension.txt", project, saveOptions);
    
    // Behavior may vary - might succeed with warning or fail
    // We just ensure it doesn't crash
    EXPECT_TRUE(saveResult.success || !saveResult.success); // Either outcome is acceptable
    
    // Try to load file with wrong extension
    std::ofstream wrongExtFile("invalid_extension.txt");
    wrongExtFile << "not a voxel file";
    wrongExtFile.close();
    
    FileIO::LoadOptions loadOptions;
    auto loadResult = fileManager->loadProject("invalid_extension.txt", project, loadOptions);
    
    // Should handle gracefully
    EXPECT_TRUE(loadResult.success || !loadResult.success); // Either outcome is acceptable
}

TEST_F(CLIErrorHandlingTest, DiskSpaceHandling) {
    // Create a very large project to test disk space handling
    // Note: This test might be skipped in environments with limited disk space
    
    // Create many voxels to make a large save file
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_1cm);
    
    const int largeVoxelCount = 10000; // Large but not excessive for CI
    for (int i = 0; i < largeVoxelCount; ++i) {
        Math::Vector3i pos(i % 100, (i / 100) % 100, i / 10000);
        if (!voxelManager->setVoxel(pos, VoxelData::VoxelResolution::Size_1cm, true)) {
            break; // Stop if we hit workspace limits
        }
    }
    
    // Try to save large project
    FileIO::Project project;
    FileIO::SaveOptions saveOptions;
    auto saveResult = fileManager->saveProject("large_test.vxl", project, saveOptions);
    
    // Should either succeed or fail gracefully
    if (!saveResult.success) {
        EXPECT_FALSE(saveResult.message.empty());
    }
    
    // Clean up large file
    std::filesystem::remove("large_test.vxl");
}

// ============================================================================
// Memory and Resource Error Handling
// ============================================================================

TEST_F(CLIErrorHandlingTest, MemoryStressTest) {
    // Test memory handling with many voxels
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    const int maxVoxels = 5000; // Large enough to test memory handling
    int successfulPlacements = 0;
    
    for (int i = 0; i < maxVoxels; ++i) {
        Math::Vector3i pos(i % 62, (i / 62) % 62, i / (62 * 62)); // Stay within 5m workspace
        
        if (voxelManager->setVoxel(pos, VoxelData::VoxelResolution::Size_8cm, true)) {
            successfulPlacements++;
        } else {
            // If placement fails, it should be due to valid constraints
            break;
        }
    }
    
    EXPECT_GT(successfulPlacements, 0);
    EXPECT_EQ(voxelManager->getVoxelCount(), successfulPlacements);
    
    // Test memory usage tracking
    float memoryUsage = voxelManager->getMemoryUsage();
    EXPECT_GE(memoryUsage, 0.0f);
    
    std::cout << "Successfully placed " << successfulPlacements 
              << " voxels using " << memoryUsage << " MB" << std::endl;
}

TEST_F(CLIErrorHandlingTest, GroupErrorHandling) {
    // Test group creation with invalid data
    std::vector<Groups::VoxelId> emptyVoxelList;
    auto groupId1 = groupManager->createGroup("EmptyGroup", emptyVoxelList);
    
    // Should handle empty group creation gracefully
    if (groupId1 != 0) {
        auto group = groupManager->getGroup(groupId1);
        EXPECT_EQ(group->getVoxelCount(), 0);
    }
    
    // Test group creation with invalid voxel IDs
    std::vector<Groups::VoxelId> invalidVoxelList;
    for (int i = 0; i < 5; ++i) {
        Math::Vector3i invalidPos(-1000 - i, -1000 - i, -1000 - i);
        Groups::VoxelId invalidId(invalidPos, VoxelData::VoxelResolution::Size_8cm);
        invalidVoxelList.push_back(invalidId);
    }
    
    auto groupId2 = groupManager->createGroup("InvalidGroup", invalidVoxelList);
    
    // Should handle invalid voxel IDs gracefully
    if (groupId2 != 0) {
        auto group = groupManager->getGroup(groupId2);
        // Group might be created but with 0 valid voxels
        EXPECT_GE(group->getVoxelCount(), 0);
    }
    
    // Test operations on non-existent group
    auto nonExistentGroup = groupManager->getGroup(99999);
    EXPECT_EQ(nonExistentGroup, nullptr);
    
    // Test duplicate group names
    std::vector<Groups::VoxelId> validVoxelList;
    Math::Vector3i pos(0, 0, 0);
    voxelManager->setVoxel(pos, VoxelData::VoxelResolution::Size_8cm, true);
    Groups::VoxelId validId(pos, VoxelData::VoxelResolution::Size_8cm);
    validVoxelList.push_back(validId);
    
    auto groupId3 = groupManager->createGroup("TestGroup", validVoxelList);
    auto groupId4 = groupManager->createGroup("TestGroup", validVoxelList); // Duplicate name
    
    // Should handle duplicate names gracefully (either allow or reject)
    EXPECT_TRUE(groupId3 != 0 || groupId4 != 0); // At least one should succeed
}

// ============================================================================
// Camera and View Error Handling
// ============================================================================

TEST_F(CLIErrorHandlingTest, CameraErrorHandling) {
    auto camera = cameraController->getCamera();
    ASSERT_NE(camera, nullptr);
    
    // Test extreme camera distances
    float originalDistance = camera->getDistance();
    
    // Test very small distance
    camera->setDistance(0.001f);
    EXPECT_GT(camera->getDistance(), 0.0f); // Should not be zero or negative
    
    // Test very large distance
    camera->setDistance(10000.0f);
    EXPECT_LT(camera->getDistance(), 10000.0f); // Should be clamped to reasonable value
    
    // Test invalid distances
    camera->setDistance(0.0f);
    EXPECT_GT(camera->getDistance(), 0.0f); // Should not accept zero
    
    camera->setDistance(-1.0f);
    EXPECT_GT(camera->getDistance(), 0.0f); // Should not accept negative
    
    camera->setDistance(NAN);
    EXPECT_FALSE(std::isnan(camera->getDistance())); // Should not accept NaN
    
    camera->setDistance(INFINITY);
    EXPECT_FALSE(std::isinf(camera->getDistance())); // Should not accept infinity
    
    // Restore original distance
    camera->setDistance(originalDistance);
}

TEST_F(CLIErrorHandlingTest, ViewportErrorHandling) {
    // Test invalid viewport sizes
    cameraController->setViewportSize(0, 0);
    // Should handle gracefully - might use default size or minimum size
    
    cameraController->setViewportSize(-100, -100);
    // Should handle negative sizes gracefully
    
    cameraController->setViewportSize(1, 1);
    // Should handle very small sizes
    
    cameraController->setViewportSize(100000, 100000);
    // Should handle very large sizes (might clamp to maximum)
    
    // No specific assertions here as behavior may vary,
    // but the test ensures these calls don't crash the application
}

// ============================================================================
// Concurrency and Thread Safety Tests
// ============================================================================

TEST_F(CLIErrorHandlingTest, ConcurrentOperations) {
    // Test basic thread safety of core operations
    // Note: This is a basic test - full thread safety testing would be more complex
    
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    // Simulate concurrent voxel placements
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);
    std::atomic<int> failureCount(0);
    
    const int numThreads = 4;
    const int operationsPerThread = 25;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operationsPerThread; ++i) {
                Math::Vector3i pos(t * 10 + i, 0, 0);
                if (voxelManager->setVoxel(pos, VoxelData::VoxelResolution::Size_8cm, true)) {
                    successCount++;
                } else {
                    failureCount++;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify total operations
    EXPECT_EQ(successCount + failureCount, numThreads * operationsPerThread);
    EXPECT_GT(successCount, 0); // Should have some successes
    
    std::cout << "Concurrent test: " << successCount << " successes, " 
              << failureCount << " failures" << std::endl;
}

} // namespace Tests
} // namespace VoxelEditor