#include <gtest/gtest.h>
#include "../include/visual_feedback/FaceDetector.h"
#include "../../voxel_data/VoxelGrid.h"
#include "../../foundation/logging/Logger.h"
#include <chrono>

using namespace VoxelEditor::Math;
using VoxelEditor::Logging::Logger;
using VoxelEditor::Logging::LogLevel;
// Avoid ambiguity by using explicit namespaces for FaceDirection
namespace VoxelData = VoxelEditor::VoxelData;
namespace VisualFeedback = VoxelEditor::VisualFeedback;

class FaceDetectorPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up minimal logging for performance tests
        Logger::getInstance().setLevel(LogLevel::Warning);
        
        // Default workspace and resolution
        workspaceSize = Vector3f(10.0f, 10.0f, 10.0f);
        resolution = VoxelData::VoxelResolution::Size_32cm;
        grid = std::make_unique<VoxelData::VoxelGrid>(resolution, workspaceSize);
        detector = std::make_unique<VisualFeedback::FaceDetector>();
    }
    
    Vector3f workspaceSize;
    VoxelData::VoxelResolution resolution;
    std::unique_ptr<VoxelData::VoxelGrid> grid;
    std::unique_ptr<VisualFeedback::FaceDetector> detector;
};

// Test performance with many voxels
TEST_F(FaceDetectorPerformanceTest, PerformanceWithManyVoxels) {
    // Fill a plane with voxels
    int voxelSizeCm = static_cast<int>(VoxelData::getVoxelSize(resolution) * 100.0f);
    
    for (int x = 0; x < 500; x += voxelSizeCm) {
        for (int z = 0; z < 500; z += voxelSizeCm) {
            grid->setVoxel(IncrementCoordinates(x, 0, z), true);
        }
    }
    
    // Ray that traverses through many voxels
    VoxelEditor::VisualFeedback::Ray ray(Vector3f(-1.0f, 0.1f, 2.5f), Vector3f(1, 0, 0));
    
    auto start = std::chrono::high_resolution_clock::now();
    VisualFeedback::Face face = detector->detectFace(ray, *grid, resolution);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_TRUE(face.isValid()) << "Should hit first voxel in path";
    EXPECT_LT(duration.count(), 1000) << "Should complete in under 1ms even with many voxels";
    
    std::cout << "Performance: Ray traversal took " << duration.count() << " microseconds" << std::endl;
}

// Test performance with negative coordinates
TEST_F(FaceDetectorPerformanceTest, PerformanceWithNegativeCoordinates) {
    std::cout << "\n--- Performance test: 100 ray casts with negative coordinates ---" << std::endl;
    
    // Place voxels at negative coordinates
    for (int i = -5; i <= -1; i++) {
        grid->setVoxel(IncrementCoordinates(i * 32, 0, i * 32), true);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        VoxelEditor::VisualFeedback::Ray ray(Vector3f(-3.0f, 0.16f, -3.0f), Vector3f(1, 0, 1));
        VisualFeedback::Face face = detector->detectFace(ray, *grid, resolution);
        EXPECT_TRUE(face.isValid()) << "Performance test ray " << i << " should hit voxel";
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "100 ray casts to negative coordinates took " << duration.count() << " microseconds" << std::endl;
    EXPECT_LT(duration.count(), 10000) << "Negative coordinate ray casting should be fast (< 10ms for 100 rays)";
}