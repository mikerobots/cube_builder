/**
 * @file test_camera_cube_visibility.cpp
 * @brief Headless integration tests to validate camera-cube visibility mathematically
 * 
 * These tests verify that the camera and cube positions are correct by:
 * 1. Creating voxels at known positions
 * 2. Setting up cameras at known positions
 * 3. Mathematically validating that the camera should see the cube
 * 4. Checking frustum culling, ray-cube intersection, etc.
 */

#include <gtest/gtest.h>
#include <cmath>
#include <iostream>
#include "camera/Camera.h"
#include "camera/OrbitCamera.h"
#include "camera/Viewport.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelGrid.h"
#include "voxel_data/VoxelTypes.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Matrix4f.h"
#include "foundation/math/MathUtils.h"

using namespace VoxelEditor;

// Helper stream operator for Vector3i in tests
namespace std {
    ostream& operator<<(ostream& os, const VoxelEditor::Math::Vector3i& v) {
        return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    }
}

class CameraCubeVisibilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create voxel data manager (it has default workspace)
        voxelData = std::make_unique<VoxelData::VoxelDataManager>();
        voxelData->resizeWorkspace(10.0f); // Set workspace to 10x10x10
        
        // Create viewport 800x600
        viewport = std::make_unique<Camera::Viewport>(0, 0, 800, 600);
        
        // Create orbit camera
        camera = std::make_unique<Camera::OrbitCamera>();
        camera->setAspectRatio(800.0f / 600.0f);
        camera->setFieldOfView(60.0f);
        camera->setNearFarPlanes(0.1f, 100.0f);
    }
    
    // Helper function to check if a point is in camera frustum
    bool isPointInFrustum(const Math::Vector3f& point) {
        // Transform point to clip space
        Math::Matrix4f viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();
        Math::Vector4f clipPos = viewProj * Math::Vector4f(point.x, point.y, point.z, 1.0f);
        
        // Perform perspective division
        if (std::abs(clipPos.w) < 0.0001f) return false;
        Math::Vector3f ndcPos(clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w);
        
        // Check if in NDC cube (-1 to 1 on all axes)
        return ndcPos.x >= -1.0f && ndcPos.x <= 1.0f &&
               ndcPos.y >= -1.0f && ndcPos.y <= 1.0f &&
               ndcPos.z >= -1.0f && ndcPos.z <= 1.0f;
    }
    
    // Helper function to check if a cube (AABB) intersects the frustum
    bool isCubeInFrustum(const Math::Vector3f& center, float size) {
        float halfSize = size * 0.5f;
        
        // Check all 8 corners of the cube
        Math::Vector3f corners[8] = {
            center + Math::Vector3f(-halfSize, -halfSize, -halfSize),
            center + Math::Vector3f( halfSize, -halfSize, -halfSize),
            center + Math::Vector3f( halfSize,  halfSize, -halfSize),
            center + Math::Vector3f(-halfSize,  halfSize, -halfSize),
            center + Math::Vector3f(-halfSize, -halfSize,  halfSize),
            center + Math::Vector3f( halfSize, -halfSize,  halfSize),
            center + Math::Vector3f( halfSize,  halfSize,  halfSize),
            center + Math::Vector3f(-halfSize,  halfSize,  halfSize)
        };
        
        // If any corner is in frustum, cube is visible
        for (const auto& corner : corners) {
            if (isPointInFrustum(corner)) {
                return true;
            }
        }
        
        // Also check center
        return isPointInFrustum(center);
    }
    
    // Helper to get world position of a voxel
    Math::Vector3f getVoxelWorldPos(int x, int y, int z, VoxelData::VoxelResolution resolution) {
        float voxelSize = VoxelData::getVoxelSize(resolution);
        // Centered coordinate system: 0,0,0 is at the center
        return Math::Vector3f(
            x * voxelSize,
            y * voxelSize,
            z * voxelSize
        );
    }
    
    // Helper to print debug info
    void printDebugInfo(const std::string& testName, const Math::Vector3f& cubePos, float cubeSize) {
        std::cout << "\n=== " << testName << " ===" << std::endl;
        auto pos = camera->getPosition();
        std::cout << "Camera position: (" << pos.x() << ", " << pos.y() << ", " << pos.z() << ")" << std::endl;
        auto target = camera->getTarget();
        std::cout << "Camera target: (" << target.x() << ", " << target.y() << ", " << target.z() << ")" << std::endl;
        auto forward = camera->getForward();
        std::cout << "Camera forward: (" << forward.x << ", " << forward.y << ", " << forward.z << ")" << std::endl;
        std::cout << "Cube position: (" << cubePos.x << ", " << cubePos.y << ", " << cubePos.z << ")" << std::endl;
        std::cout << "Cube size: " << cubeSize << std::endl;
        
        // Calculate view space position
        Math::Matrix4f viewMatrix = camera->getViewMatrix();
        Math::Vector4f viewPos = viewMatrix * Math::Vector4f(cubePos.x, cubePos.y, cubePos.z, 1.0f);
        std::cout << "Cube view space: (" << viewPos.x << ", " << viewPos.y << ", " << viewPos.z << ")" << std::endl;
        
        // Calculate clip space position
        Math::Matrix4f projMatrix = camera->getProjectionMatrix();
        Math::Vector4f clipPos = projMatrix * viewPos;
        if (std::abs(clipPos.w) > 0.0001f) {
            Math::Vector3f ndcPos(clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w);
            std::cout << "Cube NDC space: (" << ndcPos.x << ", " << ndcPos.y << ", " << ndcPos.z << ")" << std::endl;
        }
    }
    
    std::unique_ptr<VoxelData::VoxelDataManager> voxelData;
    std::unique_ptr<Camera::Viewport> viewport;
    std::unique_ptr<Camera::OrbitCamera> camera;
};

TEST_F(CameraCubeVisibilityTest, SingleVoxelAtOrigin_FrontCamera) {
    // Place a single voxel at grid position (0,0,0)
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_8cm;
    voxelData->setActiveResolution(resolution);
    voxelData->setVoxel(Math::Vector3i(0, 0, 0), resolution, true);
    
    // Set camera to front view
    // Set camera to front view
    camera->setPosition(Math::WorldCoordinates(5.0f, 5.0f, 15.0f));
    camera->setTarget(Math::WorldCoordinates(Math::Vector3f(5.0f, 5.0f, 5.0f)));
    camera->setDistance(5.0f);
    
    // Get voxel world position
    Math::Vector3f voxelPos = getVoxelWorldPos(0, 0, 0, resolution);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    printDebugInfo("Single Voxel at Origin - Front Camera", voxelPos, voxelSize);
    
    // Validate visibility
    EXPECT_TRUE(isPointInFrustum(voxelPos)) << "Voxel center should be in frustum";
    EXPECT_TRUE(isCubeInFrustum(voxelPos, voxelSize)) << "Voxel cube should be in frustum";
}

TEST_F(CameraCubeVisibilityTest, VoxelGrid3x3x3_IsometricCamera) {
    // Create 3x3x3 grid of voxels
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    voxelData->setActiveResolution(resolution);
    
    for (int x = 3; x <= 5; x++) {
        for (int y = 3; y <= 5; y++) {
            for (int z = 3; z <= 5; z++) {
                voxelData->setVoxel(Math::Vector3i(x, y, z), resolution, true);
            }
        }
    }
    
    // Set camera to isometric view
    // Set camera to isometric view
    float angle = 45.0f * M_PI / 180.0f;
    camera->setPosition(Math::WorldCoordinates(10.0f, 10.0f, 10.0f));
    camera->setTarget(Math::WorldCoordinates(Math::Vector3f(4.0f, 4.0f, 4.0f)));
    
    // Check center voxel
    Math::Vector3f centerVoxelPos = getVoxelWorldPos(4, 4, 4, resolution);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    printDebugInfo("3x3x3 Grid - Isometric Camera", centerVoxelPos, voxelSize);
    
    // All voxels should be visible
    int visibleCount = 0;
    for (int x = 3; x <= 5; x++) {
        for (int y = 3; y <= 5; y++) {
            for (int z = 3; z <= 5; z++) {
                Math::Vector3f pos = getVoxelWorldPos(x, y, z, resolution);
                if (isCubeInFrustum(pos, voxelSize)) {
                    visibleCount++;
                }
            }
        }
    }
    
    EXPECT_EQ(visibleCount, 27) << "All 27 voxels should be visible from isometric view";
}

TEST_F(CameraCubeVisibilityTest, LargeVoxel_CloseCamera) {
    // Use large voxels (32cm)
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    voxelData->setActiveResolution(resolution);
    voxelData->setVoxel(Math::Vector3i(5, 5, 5), resolution, true);
    
    // Position camera very close
    camera->setTarget(Math::WorldCoordinates(getVoxelWorldPos(5, 5, 5, resolution)));
    camera->setDistance(1.0f);
    camera->setYaw(0.0f);
    camera->setPitch(0.0f);
    
    Math::Vector3f voxelPos = getVoxelWorldPos(5, 5, 5, resolution);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    printDebugInfo("Large Voxel - Close Camera", voxelPos, voxelSize);
    
    EXPECT_TRUE(isCubeInFrustum(voxelPos, voxelSize)) << "Large voxel should be visible when camera is close";
}

TEST_F(CameraCubeVisibilityTest, VoxelBehindCamera) {
    // Place voxel at position that will be behind camera
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_8cm;
    voxelData->setActiveResolution(resolution);
    
    // Camera at (5, 5, 5) looking at (0, 0, 0)
    // So anything with higher coordinates than camera position is behind
    // Use grid position (80, 80, 80) which gives world position (6.44, 6.44, 6.44)
    voxelData->setVoxel(Math::Vector3i(80, 80, 80), resolution, true);
    
    // Set camera looking away from voxel
    camera->setPosition(Math::WorldCoordinates(5.0f, 5.0f, 5.0f));
    camera->setTarget(Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f)));
    
    Math::Vector3f voxelPos = getVoxelWorldPos(80, 80, 80, resolution);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    printDebugInfo("Voxel Behind Camera", voxelPos, voxelSize);
    
    EXPECT_FALSE(isCubeInFrustum(voxelPos, voxelSize)) << "Voxel behind camera should not be visible";
}

TEST_F(CameraCubeVisibilityTest, VoxelRayIntersection) {
    // Test ray-cube intersection from camera through viewport center
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    voxelData->setActiveResolution(resolution);
    voxelData->setVoxel(Math::Vector3i(6, 6, 6), resolution, true);
    
    // Set camera to look directly at voxel
    Math::Vector3f voxelPos = getVoxelWorldPos(6, 6, 6, resolution);
    camera->setTarget(Math::WorldCoordinates(voxelPos));
    camera->setDistance(3.0f);
    camera->setYaw(0.0f);
    camera->setPitch(0.0f);
    
    // Create ray from camera through center of viewport
    Math::Vector3f rayOrigin = camera->getPosition().value();
    Math::Vector3f rayDir = (voxelPos - rayOrigin).normalized();
    
    // Check ray-cube intersection
    float voxelSize = VoxelData::getVoxelSize(resolution);
    float halfSize = voxelSize * 0.5f;
    
    // Simple ray-AABB intersection test
    Math::Vector3f minBounds = voxelPos - Math::Vector3f(halfSize, halfSize, halfSize);
    Math::Vector3f maxBounds = voxelPos + Math::Vector3f(halfSize, halfSize, halfSize);
    
    // Calculate intersection
    Math::Vector3f invDir(1.0f / rayDir.x, 1.0f / rayDir.y, 1.0f / rayDir.z);
    Math::Vector3f t1 = (minBounds - rayOrigin) * invDir;
    Math::Vector3f t2 = (maxBounds - rayOrigin) * invDir;
    
    float tMin = std::max({std::min(t1.x, t2.x), std::min(t1.y, t2.y), std::min(t1.z, t2.z)});
    float tMax = std::min({std::max(t1.x, t2.x), std::max(t1.y, t2.y), std::max(t1.z, t2.z)});
    
    bool intersects = tMax >= tMin && tMax >= 0.0f;
    
    printDebugInfo("Ray-Cube Intersection Test", voxelPos, voxelSize);
    std::cout << "Ray origin: (" << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << ")" << std::endl;
    std::cout << "Ray direction: (" << rayDir.x << ", " << rayDir.y << ", " << rayDir.z << ")" << std::endl;
    std::cout << "Intersection t values: tMin=" << tMin << ", tMax=" << tMax << std::endl;
    
    EXPECT_TRUE(intersects) << "Ray from camera should intersect voxel cube";
    EXPECT_TRUE(isCubeInFrustum(voxelPos, voxelSize)) << "Voxel should be in frustum";
}

TEST_F(CameraCubeVisibilityTest, MultipleVoxelsScreenCoverage) {
    // Test that voxels cover different parts of the screen
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_8cm;
    voxelData->setActiveResolution(resolution);
    
    // Place voxels in a pattern
    std::vector<Math::Vector3i> voxelPositions = {
        {5, 5, 5},    // Center
        {3, 5, 5},    // Left
        {7, 5, 5},    // Right
        {5, 3, 5},    // Bottom
        {5, 7, 5}     // Top
    };
    
    for (const auto& pos : voxelPositions) {
        voxelData->setVoxel(pos, resolution, true);
    }
    
    // Set camera to front view
    // Set camera to front view
    camera->setPosition(Math::WorldCoordinates(5.0f, 5.0f, 15.0f));
    camera->setTarget(Math::WorldCoordinates(Math::Vector3f(5.0f, 5.0f, 5.0f)));
    camera->setDistance(5.0f);
    
    std::cout << "\n=== Multiple Voxels Screen Coverage ===" << std::endl;
    
    // Check each voxel and calculate screen position
    for (const auto& gridPos : voxelPositions) {
        Math::Vector3f worldPos = getVoxelWorldPos(gridPos.x, gridPos.y, gridPos.z, resolution);
        float voxelSize = VoxelData::getVoxelSize(resolution);
        
        // Transform to clip space
        Math::Matrix4f viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();
        Math::Vector4f clipPos = viewProj * Math::Vector4f(worldPos.x, worldPos.y, worldPos.z, 1.0f);
        
        if (std::abs(clipPos.w) > 0.0001f) {
            // Convert to screen coordinates
            Math::Vector3f ndcPos(clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w);
            float screenX = (ndcPos.x + 1.0f) * 0.5f * viewport->getWidth();
            float screenY = (1.0f - ndcPos.y) * 0.5f * viewport->getHeight(); // Flip Y
            
            std::cout << "Voxel at grid(" << gridPos.x << "," << gridPos.y << "," << gridPos.z 
                      << ") -> screen(" << screenX << "," << screenY << ")" << std::endl;
            
            EXPECT_TRUE(isCubeInFrustum(worldPos, voxelSize)) 
                << "Voxel at " << gridPos << " should be visible";
            EXPECT_GE(screenX, 0) << "Screen X should be >= 0";
            EXPECT_LE(screenX, viewport->getWidth()) << "Screen X should be <= viewport width";
            EXPECT_GE(screenY, 0) << "Screen Y should be >= 0";
            EXPECT_LE(screenY, viewport->getHeight()) << "Screen Y should be <= viewport height";
        }
    }
}

// Test voxel visibility with explicit camera matrix verification
TEST_F(CameraCubeVisibilityTest, ExplicitMatrixCalculations) {
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_8cm;
    voxelData->setActiveResolution(resolution);
    voxelData->setVoxel(Math::Vector3i(6, 6, 6), resolution, true);
    
    // Manually set camera parameters
    Math::Vector3f cameraPos(10.0f, 10.0f, 10.0f);
    Math::Vector3f targetPos(5.0f, 5.0f, 5.0f);
    camera->setPosition(Math::WorldCoordinates(cameraPos));
    camera->setTarget(Math::WorldCoordinates(targetPos));
    
    Math::Vector3f voxelPos = getVoxelWorldPos(6, 6, 6, resolution);
    
    std::cout << "\n=== Explicit Matrix Calculations ===" << std::endl;
    
    // Manually calculate view matrix
    Math::Vector3f forward = (targetPos - cameraPos).normalized();
    Math::Vector3f right = Math::Vector3f(0, 1, 0).cross(forward).normalized();
    Math::Vector3f up = forward.cross(right);
    
    std::cout << "Camera basis vectors:" << std::endl;
    std::cout << "  Right: (" << right.x << ", " << right.y << ", " << right.z << ")" << std::endl;
    std::cout << "  Up: (" << up.x << ", " << up.y << ", " << up.z << ")" << std::endl;
    std::cout << "  Forward: (" << forward.x << ", " << forward.y << ", " << forward.z << ")" << std::endl;
    
    // Get actual matrices from camera
    Math::Matrix4f viewMatrix = camera->getViewMatrix();
    Math::Matrix4f projMatrix = camera->getProjectionMatrix();
    
    // Transform voxel through pipeline
    Math::Vector4f worldVec(voxelPos.x, voxelPos.y, voxelPos.z, 1.0f);
    Math::Vector4f viewVec = viewMatrix * worldVec;
    Math::Vector4f clipVec = projMatrix * viewVec;
    
    std::cout << "Transform pipeline:" << std::endl;
    std::cout << "  World: (" << worldVec.x << ", " << worldVec.y << ", " << worldVec.z << ")" << std::endl;
    std::cout << "  View: (" << viewVec.x << ", " << viewVec.y << ", " << viewVec.z << ")" << std::endl;
    std::cout << "  Clip: (" << clipVec.x << ", " << clipVec.y << ", " << clipVec.z << ", " << clipVec.w << ")" << std::endl;
    
    if (std::abs(clipVec.w) > 0.0001f) {
        Math::Vector3f ndcPos(clipVec.x / clipVec.w, clipVec.y / clipVec.w, clipVec.z / clipVec.w);
        std::cout << "  NDC: (" << ndcPos.x << ", " << ndcPos.y << ", " << ndcPos.z << ")" << std::endl;
        
        // Verify NDC is in valid range
        bool inNDC = ndcPos.x >= -1.0f && ndcPos.x <= 1.0f &&
                     ndcPos.y >= -1.0f && ndcPos.y <= 1.0f &&
                     ndcPos.z >= -1.0f && ndcPos.z <= 1.0f;
        
        EXPECT_TRUE(inNDC) << "Voxel should be in NDC range";
    }
    
    EXPECT_TRUE(isCubeInFrustum(voxelPos, VoxelData::getVoxelSize(resolution))) 
        << "Voxel should be visible in frustum";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}