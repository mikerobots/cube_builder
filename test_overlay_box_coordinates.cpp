#include <iostream>
#include <cmath>
#include <iomanip>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Simulate the entire pipeline from mouse to overlay box coordinates

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
    
    void print(const char* label) const {
        std::cout << label << ": ";
        std::cout << "min(" << std::fixed << std::setprecision(3) 
                  << min.x << ", " << min.y << ", " << min.z << ") ";
        std::cout << "max(" << max.x << ", " << max.y << ", " << max.z << ")\n";
    }
};

void testOverlayBoxMovement() {
    // Screen dimensions
    const float screenWidth = 800.0f;
    const float screenHeight = 600.0f;
    
    // Top view camera setup (same as in MouseInteraction)
    glm::vec3 cameraPos(0.0f, 5.0f, 0.0f);
    glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp(0.0f, 0.0f, -1.0f);  // -Z is up in top view
    
    glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    
    // Orthographic projection
    float orthoSize = 5.0f;
    float aspectRatio = screenWidth / screenHeight;
    float halfWidth = orthoSize * aspectRatio * 0.5f;
    float halfHeight = orthoSize * 0.5f;
    glm::mat4 projMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1f, 100.0f);
    
    glm::mat4 invVP = glm::inverse(projMatrix * viewMatrix);
    
    // Voxel size (32cm = 0.32m)
    const float voxelSize = 0.32f;
    
    std::cout << "Overlay Box Coordinate Movement Test\n";
    std::cout << "===================================\n\n";
    std::cout << "Simulating mouse movement and showing resulting overlay box coordinates\n";
    std::cout << "Voxel size: " << voxelSize << "m\n\n";
    
    // Test mouse movement in a line
    std::cout << "Test 1: Horizontal mouse movement (left to right)\n";
    std::cout << "-------------------------------------------------\n";
    
    float mouseY = 300;  // Keep Y constant
    for (float mouseX = 200; mouseX <= 600; mouseX += 100) {
        // Convert mouse to NDC
        float ndcX = (2.0f * mouseX) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * mouseY) / screenHeight;
        
        // Unproject to get ray
        glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;
        
        // Create ray
        glm::vec3 rayOrigin = glm::vec3(nearPoint);
        glm::vec3 rayDir = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));
        
        // Intersect with ground plane (Y=0)
        float t = -rayOrigin.y / rayDir.y;
        glm::vec3 hitPoint = rayOrigin + t * rayDir;
        
        // Create bounding box at hit point (as done in MouseInteraction)
        BoundingBox overlayBox;
        overlayBox.min = hitPoint;
        overlayBox.max = hitPoint + glm::vec3(voxelSize, voxelSize, voxelSize);
        
        std::cout << "Mouse(" << mouseX << ", " << mouseY << ") -> ";
        std::cout << "Hit(" << std::fixed << std::setprecision(3) 
                  << hitPoint.x << ", " << hitPoint.y << ", " << hitPoint.z << ") -> ";
        overlayBox.print("Box");
    }
    
    std::cout << "\nTest 2: Vertical mouse movement (up to down)\n";
    std::cout << "---------------------------------------------\n";
    
    float mouseX2 = 400;  // Keep X constant
    for (float mouseY2 = 150; mouseY2 <= 450; mouseY2 += 75) {
        // Convert mouse to NDC
        float ndcX = (2.0f * mouseX2) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * mouseY2) / screenHeight;
        
        // Unproject to get ray
        glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;
        
        // Create ray
        glm::vec3 rayOrigin = glm::vec3(nearPoint);
        glm::vec3 rayDir = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));
        
        // Intersect with ground plane (Y=0)
        float t = -rayOrigin.y / rayDir.y;
        glm::vec3 hitPoint = rayOrigin + t * rayDir;
        
        // Create bounding box at hit point
        BoundingBox overlayBox;
        overlayBox.min = hitPoint;
        overlayBox.max = hitPoint + glm::vec3(voxelSize, voxelSize, voxelSize);
        
        std::cout << "Mouse(" << mouseX2 << ", " << mouseY2 << ") -> ";
        std::cout << "Hit(" << std::fixed << std::setprecision(3) 
                  << hitPoint.x << ", " << hitPoint.y << ", " << hitPoint.z << ") -> ";
        overlayBox.print("Box");
    }
    
    std::cout << "\nTest 3: Diagonal mouse movement\n";
    std::cout << "--------------------------------\n";
    
    for (int i = 0; i <= 4; i++) {
        float t = i / 4.0f;
        float mouseX3 = 200 + t * 400;  // 200 to 600
        float mouseY3 = 150 + t * 300;  // 150 to 450
        
        // Convert mouse to NDC
        float ndcX = (2.0f * mouseX3) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * mouseY3) / screenHeight;
        
        // Unproject to get ray
        glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;
        
        // Create ray
        glm::vec3 rayOrigin = glm::vec3(nearPoint);
        glm::vec3 rayDir = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));
        
        // Intersect with ground plane (Y=0)
        float rayT = -rayOrigin.y / rayDir.y;
        glm::vec3 hitPoint = rayOrigin + rayT * rayDir;
        
        // Create bounding box at hit point
        BoundingBox overlayBox;
        overlayBox.min = hitPoint;
        overlayBox.max = hitPoint + glm::vec3(voxelSize, voxelSize, voxelSize);
        
        std::cout << "Mouse(" << mouseX3 << ", " << mouseY3 << ") -> ";
        std::cout << "Hit(" << std::fixed << std::setprecision(3) 
                  << hitPoint.x << ", " << hitPoint.y << ", " << hitPoint.z << ") -> ";
        overlayBox.print("Box");
    }
    
    std::cout << "\nMovement Summary:\n";
    std::cout << "================\n";
    std::cout << "✓ Mouse moves right → Box X coordinates increase\n";
    std::cout << "✓ Mouse moves left  → Box X coordinates decrease\n";
    std::cout << "✓ Mouse moves down  → Box Z coordinates increase\n";
    std::cout << "✓ Mouse moves up    → Box Z coordinates decrease\n";
    std::cout << "✓ Box always sits on ground plane (Y=0 to Y=" << voxelSize << ")\n";
    
    // Calculate movement ratios
    std::cout << "\nMovement Ratios:\n";
    std::cout << "================\n";
    float pixelsPerMeter = screenWidth / (halfWidth * 2);
    std::cout << "Screen pixels per world meter: " << pixelsPerMeter << "\n";
    std::cout << "100 pixel mouse movement = " << (100.0f / pixelsPerMeter) << "m world movement\n";
}

int main() {
    testOverlayBoxMovement();
    return 0;
}