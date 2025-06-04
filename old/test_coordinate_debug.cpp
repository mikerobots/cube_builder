#include <iostream>
#include "camera/OrbitCamera.h"
#include "camera/Viewport.h"
#include "foundation/math/Matrix4f.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Vector4f.h"

using namespace VoxelEditor;

int main() {
    std::cout << "=== Coordinate System Debug Test ===" << std::endl;
    
    // Create camera setup similar to the app
    Camera::Viewport viewport(0, 0, 2560, 1440);
    Camera::OrbitCamera camera;
    
    // Set up camera like in the app
    camera.setAspectRatio(2560.0f / 1440.0f);
    camera.setFieldOfView(45.0f);
    camera.setNearFarPlanes(0.1f, 1000.0f);
    
    // Test case 1: Front view looking at a voxel at (1.12, 1.12, 1.12)
    std::cout << "\nTest 1: Front view of voxel at (1.12, 1.12, 1.12)" << std::endl;
    camera.setViewPreset(Camera::ViewPreset::FRONT);
    camera.setDistance(5.0f);
    
    // Voxel position (from the debug output)
    Math::Vector3f voxelPos(1.12f, 1.12f, 1.12f);
    Math::Vector3f voxelCorner(0.968f, 0.968f, 0.968f); // First vertex from debug
    
    std::cout << "Camera position: " << camera.getPosition() << std::endl;
    std::cout << "Camera target: " << camera.getTarget() << std::endl;
    std::cout << "Camera forward: " << camera.getForward() << std::endl;
    
    // Transform voxel through matrices
    Math::Matrix4f view = camera.getViewMatrix();
    Math::Matrix4f proj = camera.getProjectionMatrix();
    Math::Matrix4f viewProj = proj * view;
    
    // Transform voxel center
    Math::Vector4f voxelWorld(voxelPos.x, voxelPos.y, voxelPos.z, 1.0f);
    Math::Vector4f voxelView = view * voxelWorld;
    Math::Vector4f voxelClip = proj * voxelView;
    
    std::cout << "\nVoxel center transformation:" << std::endl;
    std::cout << "  World: " << voxelWorld << std::endl;
    std::cout << "  View: " << voxelView << std::endl;
    std::cout << "  Clip: " << voxelClip << std::endl;
    
    if (std::abs(voxelClip.w) > 0.0001f) {
        Math::Vector3f ndc(voxelClip.x / voxelClip.w, 
                          voxelClip.y / voxelClip.w, 
                          voxelClip.z / voxelClip.w);
        std::cout << "  NDC: " << ndc << std::endl;
        
        // Check if in frustum
        bool inFrustum = ndc.x >= -1.0f && ndc.x <= 1.0f &&
                        ndc.y >= -1.0f && ndc.y <= 1.0f &&
                        ndc.z >= -1.0f && ndc.z <= 1.0f;
        std::cout << "  In frustum: " << (inFrustum ? "YES" : "NO") << std::endl;
    }
    
    // Test case 2: Transform a corner vertex
    std::cout << "\nVoxel corner transformation:" << std::endl;
    Math::Vector4f cornerWorld(voxelCorner.x, voxelCorner.y, voxelCorner.z, 1.0f);
    Math::Vector4f cornerView = view * cornerWorld;
    Math::Vector4f cornerClip = proj * cornerView;
    
    std::cout << "  World: " << cornerWorld << std::endl;
    std::cout << "  View: " << cornerView << std::endl;
    std::cout << "  Clip: " << cornerClip << std::endl;
    
    // Test case 3: Set camera to look at workspace center
    std::cout << "\nTest 2: Camera looking at workspace center (1, 1, 1)" << std::endl;
    camera.setTarget(Math::Vector3f(1.0f, 1.0f, 1.0f));
    camera.setDistance(3.0f);
    
    std::cout << "Camera position: " << camera.getPosition() << std::endl;
    std::cout << "Camera target: " << camera.getTarget() << std::endl;
    
    // Re-test voxel transformation
    view = camera.getViewMatrix();
    viewProj = proj * view;
    
    voxelView = view * voxelWorld;
    voxelClip = proj * voxelView;
    
    std::cout << "\nVoxel transformation with new camera:" << std::endl;
    std::cout << "  View: " << voxelView << std::endl;
    std::cout << "  Clip: " << voxelClip << std::endl;
    
    // Print view matrix for debugging
    std::cout << "\nView Matrix:" << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << view(i, j) << " ";
        }
        std::cout << std::endl;
    }
    
    // Print projection matrix
    std::cout << "\nProjection Matrix:" << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << proj(i, j) << " ";
        }
        std::cout << std::endl;
    }
    
    return 0;
}