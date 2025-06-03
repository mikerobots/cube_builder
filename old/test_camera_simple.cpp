#include <iostream>
#include <cmath>
#include "core/camera/OrbitCamera.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Vector4f.h"
#include "foundation/math/Matrix4f.h"
#include "foundation/math/MathUtils.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Camera;
using namespace VoxelEditor::Math;

int main() {
    std::cout << "=== Simple Camera Test ===" << std::endl;
    
    // Create camera
    OrbitCamera camera;
    
    // Set up camera like in the main app
    camera.setViewPreset(ViewPreset::ISOMETRIC);
    camera.setTarget(Vector3f(0.64f, 0.64f, 0.64f));  // Voxel center
    camera.setDistance(5.0f);
    camera.setAspectRatio(1280.0f / 720.0f);
    
    // Get camera info
    Vector3f pos = camera.getPosition();
    std::cout << "\nCamera Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    std::cout << "Camera Target: (0.64, 0.64, 0.64)" << std::endl;
    std::cout << "Distance: " << camera.getDistance() << std::endl;
    
    // Get matrices
    Matrix4f viewMatrix = camera.getViewMatrix();
    Matrix4f projMatrix = camera.getProjectionMatrix();
    Matrix4f mvp = projMatrix * viewMatrix;
    
    // Test voxel corners
    float halfSize = 0.64f;  // Half of 128cm voxel
    Vector4f corners[8] = {
        Vector4f(0.0f, 0.0f, 0.0f, 1.0f),  // Min corner
        Vector4f(1.28f, 0.0f, 0.0f, 1.0f),
        Vector4f(0.0f, 1.28f, 0.0f, 1.0f),
        Vector4f(1.28f, 1.28f, 0.0f, 1.0f),
        Vector4f(0.0f, 0.0f, 1.28f, 1.0f),
        Vector4f(1.28f, 0.0f, 1.28f, 1.0f),
        Vector4f(0.0f, 1.28f, 1.28f, 1.0f),
        Vector4f(1.28f, 1.28f, 1.28f, 1.0f)  // Max corner
    };
    
    std::cout << "\n=== Voxel Corner Transformations ===" << std::endl;
    int visibleCount = 0;
    
    for (int i = 0; i < 8; i++) {
        Vector4f worldPos = corners[i];
        Vector4f viewPos = viewMatrix * worldPos;
        Vector4f clipPos = mvp * worldPos;
        
        bool visible = false;
        if (clipPos.w > 0) {
            Vector4f ndc = clipPos / clipPos.w;
            if (ndc.x >= -1.0f && ndc.x <= 1.0f &&
                ndc.y >= -1.0f && ndc.y <= 1.0f &&
                ndc.z >= -1.0f && ndc.z <= 1.0f) {
                visible = true;
                visibleCount++;
            }
        }
        
        std::cout << "Corner " << i << ": World(" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")" << std::endl;
        std::cout << "  View space: (" << viewPos.x << ", " << viewPos.y << ", " << viewPos.z << ")" << std::endl;
        std::cout << "  Clip space: (" << clipPos.x << ", " << clipPos.y << ", " << clipPos.z << ", " << clipPos.w << ")" << std::endl;
        if (clipPos.w > 0) {
            Vector4f ndc = clipPos / clipPos.w;
            std::cout << "  NDC: (" << ndc.x << ", " << ndc.y << ", " << ndc.z << ")" << std::endl;
        }
        std::cout << "  Visible: " << (visible ? "YES" : "NO") << std::endl;
    }
    
    std::cout << "\nTotal visible corners: " << visibleCount << "/8" << std::endl;
    
    // Print matrices for debugging
    std::cout << "\n=== View Matrix ===" << std::endl;
    for (int i = 0; i < 4; i++) {
        std::cout << "[";
        for (int j = 0; j < 4; j++) {
            std::cout << viewMatrix.m[i * 4 + j] << " ";
        }
        std::cout << "]" << std::endl;
    }
    
    std::cout << "\n=== Projection Matrix ===" << std::endl;
    for (int i = 0; i < 4; i++) {
        std::cout << "[";
        for (int j = 0; j < 4; j++) {
            std::cout << projMatrix.m[i * 4 + j] << " ";
        }
        std::cout << "]" << std::endl;
    }
    
    return 0;
}