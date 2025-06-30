#include <iostream>
#include <iomanip>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Test to verify axis mapping in top view

void testTopViewAxes() {
    std::cout << "Top View Axis Mapping Test\n";
    std::cout << "=========================\n\n";
    
    // Screen dimensions
    const float screenWidth = 800.0f;
    const float screenHeight = 600.0f;
    
    // Top view camera setup - this is what the code uses
    glm::vec3 cameraPos(0.0f, 5.0f, 0.0f);
    glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp(0.0f, 0.0f, -1.0f);  // -Z is up in top view
    
    std::cout << "Camera setup:\n";
    std::cout << "- Position: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")\n";
    std::cout << "- Target: (" << cameraTarget.x << ", " << cameraTarget.y << ", " << cameraTarget.z << ")\n";
    std::cout << "- Up vector: (" << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << ")\n\n";
    
    // Create view matrix
    glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    
    // Let's check what the view matrix does to our axes
    std::cout << "View matrix transformation of world axes:\n";
    
    // Transform world axes to view space
    glm::vec4 worldX(1, 0, 0, 0);  // X-axis in world
    glm::vec4 worldY(0, 1, 0, 0);  // Y-axis in world
    glm::vec4 worldZ(0, 0, 1, 0);  // Z-axis in world
    
    glm::vec4 viewX = viewMatrix * worldX;
    glm::vec4 viewY = viewMatrix * worldY;
    glm::vec4 viewZ = viewMatrix * worldZ;
    
    std::cout << "World X-axis -> View space: (" << viewX.x << ", " << viewX.y << ", " << viewX.z << ")\n";
    std::cout << "World Y-axis -> View space: (" << viewY.x << ", " << viewY.y << ", " << viewY.z << ")\n";
    std::cout << "World Z-axis -> View space: (" << viewZ.x << ", " << viewZ.y << ", " << viewZ.z << ")\n\n";
    
    // Check screen space mapping
    std::cout << "Expected screen space mapping:\n";
    std::cout << "- Screen X (right) should map to World X positive\n";
    std::cout << "- Screen Y (down) should map to World Z positive\n";
    std::cout << "- World Y should remain constant (ground plane)\n\n";
    
    // Test mouse movement
    std::cout << "Testing mouse movement:\n";
    std::cout << "=======================\n\n";
    
    // Create orthographic projection
    float orthoSize = 5.0f;
    float aspectRatio = screenWidth / screenHeight;
    float halfWidth = orthoSize * aspectRatio * 0.5f;
    float halfHeight = orthoSize * 0.5f;
    glm::mat4 projMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1f, 100.0f);
    
    glm::mat4 invVP = glm::inverse(projMatrix * viewMatrix);
    
    // Test vertical mouse movement (should affect Z, not Y)
    std::cout << "Vertical mouse movement test:\n";
    float mouseX = 400;  // Center X
    for (float mouseY = 200; mouseY <= 400; mouseY += 100) {
        // Convert to NDC
        float ndcX = (2.0f * mouseX) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * mouseY) / screenHeight;
        
        // Unproject
        glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;
        
        // Create ray
        glm::vec3 rayOrigin = glm::vec3(nearPoint);
        glm::vec3 rayDir = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));
        
        // Intersect with ground plane (Y=0)
        if (std::abs(rayDir.y) > 0.001f) {
            float t = -rayOrigin.y / rayDir.y;
            glm::vec3 hitPoint = rayOrigin + t * rayDir;
            
            std::cout << "Mouse Y=" << mouseY << " -> World hit: X=" << hitPoint.x 
                      << ", Y=" << hitPoint.y << ", Z=" << hitPoint.z << "\n";
        }
    }
    
    std::cout << "\nDiagnosis:\n";
    std::cout << "If mouse up/down is changing Y instead of Z, the issue might be:\n";
    std::cout << "1. Wrong up vector in camera setup\n";
    std::cout << "2. Incorrect ray direction calculation\n";
    std::cout << "3. Wrong ground plane intersection\n";
}

int main() {
    testTopViewAxes();
    return 0;
}