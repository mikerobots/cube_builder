/**
 * @file test_camera_cube_visibility_simple.cpp
 * @brief Simple headless tests to validate camera-cube visibility mathematically
 */

#include <iostream>
#include <cmath>
#include <vector>
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

// Helper function to check if a point is in camera frustum
bool isPointInFrustum(const Camera::Camera& camera, const Math::Vector3f& point) {
    // Transform point to clip space
    Math::Matrix4f viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();
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
bool isCubeInFrustum(const Camera::Camera& camera, const Math::Vector3f& center, float size) {
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
        if (isPointInFrustum(camera, corner)) {
            return true;
        }
    }
    
    // Also check center
    return isPointInFrustum(camera, center);
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
void printDebugInfo(const std::string& testName, const Camera::Camera& camera, 
                   const Math::Vector3f& cubePos, float cubeSize) {
    std::cout << "\n=== " << testName << " ===" << std::endl;
    std::cout << "Camera position: (" << camera.getPosition().x() << ", " 
              << camera.getPosition().y() << ", " << camera.getPosition().z() << ")" << std::endl;
    std::cout << "Camera target: (" << camera.getTarget().x() << ", " 
              << camera.getTarget().y() << ", " << camera.getTarget().z() << ")" << std::endl;
    std::cout << "Camera forward: (" << camera.getForward().x << ", " 
              << camera.getForward().y << ", " << camera.getForward().z << ")" << std::endl;
    std::cout << "Cube position: (" << cubePos.x << ", " << cubePos.y << ", " << cubePos.z << ")" << std::endl;
    std::cout << "Cube size: " << cubeSize << std::endl;
    
    // Calculate view space position
    Math::Matrix4f viewMatrix = camera.getViewMatrix();
    Math::Vector4f viewPos = viewMatrix * Math::Vector4f(cubePos.x, cubePos.y, cubePos.z, 1.0f);
    std::cout << "Cube view space: (" << viewPos.x << ", " << viewPos.y << ", " << viewPos.z << ")" << std::endl;
    
    // Calculate clip space position
    Math::Matrix4f projMatrix = camera.getProjectionMatrix();
    Math::Vector4f clipPos = projMatrix * viewPos;
    if (std::abs(clipPos.w) > 0.0001f) {
        Math::Vector3f ndcPos(clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w);
        std::cout << "Cube NDC space: (" << ndcPos.x << ", " << ndcPos.y << ", " << ndcPos.z << ")" << std::endl;
    }
}

void test_SingleVoxelAtOrigin_FrontCamera() {
    std::cout << "\n========== TEST: Single Voxel at Origin - Front Camera ==========\n";
    
    // Create voxel data manager
    VoxelData::VoxelDataManager voxelData;
    voxelData.resizeWorkspace(10.0f);
    
    // Place a single voxel at grid position (0,0,0)
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_8cm;
    voxelData.setActiveResolution(resolution);
    voxelData.setVoxel(Math::Vector3i(0, 0, 0), resolution, true);
    
    // Create camera
    Camera::OrbitCamera camera;
    camera.setAspectRatio(800.0f / 600.0f);
    camera.setFieldOfView(60.0f);
    camera.setNearFarPlanes(0.1f, 100.0f);
    
    // Set camera to front view
    camera.setViewPreset(Camera::ViewPreset::FRONT);
    camera.setDistance(5.0f);
    
    // Get voxel world position
    Math::Vector3f voxelPos = getVoxelWorldPos(0, 0, 0, resolution);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    printDebugInfo("Single Voxel at Origin - Front Camera", camera, voxelPos, voxelSize);
    
    // Validate visibility
    bool centerInFrustum = isPointInFrustum(camera, voxelPos);
    bool cubeInFrustum = isCubeInFrustum(camera, voxelPos, voxelSize);
    
    std::cout << "Result: Voxel center in frustum = " << (centerInFrustum ? "YES" : "NO") << std::endl;
    std::cout << "Result: Voxel cube in frustum = " << (cubeInFrustum ? "YES" : "NO") << std::endl;
    
    if (!centerInFrustum || !cubeInFrustum) {
        std::cout << "ERROR: Voxel should be visible!" << std::endl;
    }
}

void test_VoxelGrid3x3x3_IsometricCamera() {
    std::cout << "\n========== TEST: 3x3x3 Grid - Isometric Camera ==========\n";
    
    // Create voxel data manager
    VoxelData::VoxelDataManager voxelData;
    voxelData.resizeWorkspace(10.0f);
    
    // Create 3x3x3 grid of voxels
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    voxelData.setActiveResolution(resolution);
    
    for (int x = 3; x <= 5; x++) {
        for (int y = 3; y <= 5; y++) {
            for (int z = 3; z <= 5; z++) {
                voxelData.setVoxel(Math::Vector3i(x, y, z), resolution, true);
            }
        }
    }
    
    // Create camera
    Camera::OrbitCamera camera;
    camera.setAspectRatio(800.0f / 600.0f);
    camera.setFieldOfView(60.0f);
    camera.setNearFarPlanes(0.1f, 100.0f);
    
    // Set camera to isometric view
    camera.setViewPreset(Camera::ViewPreset::ISOMETRIC);
    
    // Check center voxel
    Math::Vector3f centerVoxelPos = getVoxelWorldPos(4, 4, 4, resolution);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    printDebugInfo("3x3x3 Grid - Isometric Camera", camera, centerVoxelPos, voxelSize);
    
    // Check all voxels visibility
    int visibleCount = 0;
    for (int x = 3; x <= 5; x++) {
        for (int y = 3; y <= 5; y++) {
            for (int z = 3; z <= 5; z++) {
                Math::Vector3f pos = getVoxelWorldPos(x, y, z, resolution);
                if (isCubeInFrustum(camera, pos, voxelSize)) {
                    visibleCount++;
                }
            }
        }
    }
    
    std::cout << "Result: " << visibleCount << " out of 27 voxels are visible" << std::endl;
    
    if (visibleCount != 27) {
        std::cout << "ERROR: All 27 voxels should be visible from isometric view!" << std::endl;
    }
}

void test_LargeVoxel_CloseCamera() {
    std::cout << "\n========== TEST: Large Voxel - Close Camera ==========\n";
    
    // Create voxel data manager
    VoxelData::VoxelDataManager voxelData;
    voxelData.resizeWorkspace(10.0f);
    
    // Use large voxels (32cm)
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    voxelData.setActiveResolution(resolution);
    voxelData.setVoxel(Math::Vector3i(5, 5, 5), resolution, true);
    
    // Create camera
    Camera::OrbitCamera camera;
    camera.setAspectRatio(800.0f / 600.0f);
    camera.setFieldOfView(60.0f);
    camera.setNearFarPlanes(0.1f, 100.0f);
    
    // Position camera very close
    Math::Vector3f voxelPos = getVoxelWorldPos(5, 5, 5, resolution);
    camera.setTarget(Math::WorldCoordinates(voxelPos));
    camera.setDistance(1.0f);
    camera.setYaw(0.0f);
    camera.setPitch(0.0f);
    
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    printDebugInfo("Large Voxel - Close Camera", camera, voxelPos, voxelSize);
    
    bool inFrustum = isCubeInFrustum(camera, voxelPos, voxelSize);
    std::cout << "Result: Large voxel in frustum = " << (inFrustum ? "YES" : "NO") << std::endl;
    
    if (!inFrustum) {
        std::cout << "ERROR: Large voxel should be visible when camera is close!" << std::endl;
    }
}

void test_ExplicitMatrixCalculations() {
    std::cout << "\n========== TEST: Explicit Matrix Calculations ==========\n";
    
    // Create voxel data manager
    VoxelData::VoxelDataManager voxelData;
    voxelData.resizeWorkspace(10.0f);
    
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_8cm;
    voxelData.setActiveResolution(resolution);
    voxelData.setVoxel(Math::Vector3i(6, 6, 6), resolution, true);
    
    // Create camera
    Camera::OrbitCamera camera;
    camera.setAspectRatio(800.0f / 600.0f);
    camera.setFieldOfView(60.0f);
    camera.setNearFarPlanes(0.1f, 100.0f);
    
    // Manually set camera parameters
    Math::WorldCoordinates cameraPos(10.0f, 10.0f, 10.0f);
    Math::WorldCoordinates targetPos(5.0f, 5.0f, 5.0f);
    camera.setPosition(cameraPos);
    camera.setTarget(targetPos);
    
    Math::Vector3f voxelPos = getVoxelWorldPos(6, 6, 6, resolution);
    
    std::cout << "=== Explicit Matrix Calculations ===" << std::endl;
    
    // Manually calculate view matrix components
    Math::Vector3f forward = (targetPos - cameraPos).value().normalized();
    Math::Vector3f right = Math::Vector3f(0, 1, 0).cross(forward).normalized();
    Math::Vector3f up = forward.cross(right);
    
    std::cout << "Camera basis vectors:" << std::endl;
    std::cout << "  Right: (" << right.x << ", " << right.y << ", " << right.z << ")" << std::endl;
    std::cout << "  Up: (" << up.x << ", " << up.y << ", " << up.z << ")" << std::endl;
    std::cout << "  Forward: (" << forward.x << ", " << forward.y << ", " << forward.z << ")" << std::endl;
    
    // Get actual matrices from camera
    Math::Matrix4f viewMatrix = camera.getViewMatrix();
    Math::Matrix4f projMatrix = camera.getProjectionMatrix();
    
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
        
        std::cout << "Result: Voxel in NDC range = " << (inNDC ? "YES" : "NO") << std::endl;
        
        if (!inNDC) {
            std::cout << "ERROR: Voxel should be in NDC range!" << std::endl;
        }
    }
    
    bool inFrustum = isCubeInFrustum(camera, voxelPos, VoxelData::getVoxelSize(resolution));
    std::cout << "Result: Voxel in frustum = " << (inFrustum ? "YES" : "NO") << std::endl;
    
    if (!inFrustum) {
        std::cout << "ERROR: Voxel should be visible in frustum!" << std::endl;
    }
}

int main(int argc, char** argv) {
    std::cout << "Camera-Cube Visibility Tests\n";
    std::cout << "============================\n";
    
    test_SingleVoxelAtOrigin_FrontCamera();
    test_VoxelGrid3x3x3_IsometricCamera();
    test_LargeVoxel_CloseCamera();
    test_ExplicitMatrixCalculations();
    
    std::cout << "\nAll tests completed.\n";
    
    return 0;
}