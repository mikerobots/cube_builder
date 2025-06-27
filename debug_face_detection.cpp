#include "core/visual_feedback/include/visual_feedback/GeometricFaceDetector.h"
#include "core/visual_feedback/include/visual_feedback/FeedbackTypes.h"
#include "foundation/math/Vector3f.h"
#include <iostream>

using namespace VoxelEditor;
using namespace VoxelEditor::VisualFeedback;

int main() {
    // Create two voxels: one at (64,96,96) and one at (128,96,96)
    // Shoot ray from left through the hole, should hit the right voxel (128,96,96) at -X face
    
    float voxelSize = 0.32f; // 32cm
    
    // Left voxel at (64,96,96) in increment coordinates
    Math::Vector3f leftVoxelWorld(0.64f, 0.96f, 0.96f);
    
    // Right voxel at (128,96,96) in increment coordinates  
    Math::Vector3f rightVoxelWorld(1.28f, 0.96f, 0.96f);
    
    // Create faces for both voxels
    auto leftFaces = GeometricFaceDetector::createVoxelFaces(leftVoxelWorld, voxelSize);
    auto rightFaces = GeometricFaceDetector::createVoxelFaces(rightVoxelWorld, voxelSize);
    
    // Combine all faces
    std::vector<GeometricFace> allFaces;
    allFaces.insert(allFaces.end(), leftFaces.begin(), leftFaces.end());
    allFaces.insert(allFaces.end(), rightFaces.begin(), rightFaces.end());
    
    // Set unique IDs
    for (size_t i = 0; i < allFaces.size(); ++i) {
        allFaces[i].id = static_cast<int>(i);
    }
    
    // Create ray from left of left voxel, pointing right (+X direction)
    // Ray should go through the gap and hit the -X face of the right voxel
    // Make sure ray goes through the CENTER of the gap, not the edge
    Math::Vector3f rayOrigin(leftVoxelWorld.x - 2.0f, leftVoxelWorld.y + voxelSize/2, leftVoxelWorld.z);
    Math::Vector3f rayDir(1, 0, 0);
    
    VisualFeedback::Ray ray(rayOrigin, rayDir);
    
    std::cout << "Ray origin: (" << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << ")" << std::endl;
    std::cout << "Ray direction: (" << rayDir.x << ", " << rayDir.y << ", " << rayDir.z << ")" << std::endl;
    
    std::cout << "Left voxel center: (" << leftVoxelWorld.x << ", " << leftVoxelWorld.y + voxelSize/2 << ", " << leftVoxelWorld.z << ")" << std::endl;
    std::cout << "Right voxel center: (" << rightVoxelWorld.x << ", " << rightVoxelWorld.y + voxelSize/2 << ", " << rightVoxelWorld.z << ")" << std::endl;
    
    // Test intersection with each face individually
    std::cout << "\n=== Testing individual face intersections ===" << std::endl;
    for (size_t i = 0; i < allFaces.size(); ++i) {
        RayFaceHit hit = GeometricFaceDetector::rayFaceIntersection(ray, allFaces[i]);
        if (hit.hit) {
            std::cout << "Face " << i << " hit at distance " << hit.distance << " point (" 
                      << hit.point.x << ", " << hit.point.y << ", " << hit.point.z << ")" << std::endl;
            
            // Determine which voxel this face belongs to
            if (i < 6) {
                std::cout << "  Belongs to LEFT voxel, face direction: " << (i == 0 ? "+X" : i == 1 ? "-X" : i == 2 ? "+Y" : i == 3 ? "-Y" : i == 4 ? "+Z" : "-Z") << std::endl;
            } else {
                std::cout << "  Belongs to RIGHT voxel, face direction: " << ((i-6) == 0 ? "+X" : (i-6) == 1 ? "-X" : (i-6) == 2 ? "+Y" : (i-6) == 3 ? "-Y" : (i-6) == 4 ? "+Z" : "-Z") << std::endl;
            }
        }
    }
    
    // Test closest face detection
    std::cout << "\n=== Testing closest face detection ===" << std::endl;
    auto closestHit = GeometricFaceDetector::detectClosestFace(ray, allFaces);
    
    if (closestHit.has_value()) {
        std::cout << "Closest hit: Face " << closestHit->faceId << " at distance " << closestHit->distance << std::endl;
        std::cout << "Hit point: (" << closestHit->point.x << ", " << closestHit->point.y << ", " << closestHit->point.z << ")" << std::endl;
        
        if (closestHit->faceId < 6) {
            std::cout << "Hit LEFT voxel (wrong!)" << std::endl;
        } else {
            std::cout << "Hit RIGHT voxel (correct!)" << std::endl;
        }
    } else {
        std::cout << "No hit detected" << std::endl;
    }
    
    return 0;
}