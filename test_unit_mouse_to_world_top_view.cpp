#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Test mouse position to world coordinate transformation in top view

void testMouseToWorldTopView() {
    // Screen dimensions
    const float screenWidth = 800.0f;
    const float screenHeight = 600.0f;
    
    // Top view camera setup
    // Looking down from above: camera at (0, 5, 0), looking at (0, 0, 0), up is -Z
    glm::vec3 cameraPos(0.0f, 5.0f, 0.0f);
    glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp(0.0f, 0.0f, -1.0f);  // -Z is up in top view
    
    // Create view matrix
    glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    
    // Create orthographic projection for top view
    float orthoSize = 5.0f;
    float aspectRatio = screenWidth / screenHeight;
    float halfWidth = orthoSize * aspectRatio * 0.5f;
    float halfHeight = orthoSize * 0.5f;
    glm::mat4 projMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1f, 100.0f);
    
    // Combined view-projection matrix
    glm::mat4 vpMatrix = projMatrix * viewMatrix;
    glm::mat4 invVP = glm::inverse(vpMatrix);
    
    std::cout << "Mouse to World Coordinate Test (Top View)\n";
    std::cout << "=========================================\n\n";
    
    // Test cases: mouse positions to test
    struct TestCase {
        float mouseX, mouseY;
        const char* description;
    };
    
    TestCase testCases[] = {
        {400, 300, "Center"},
        {600, 300, "Right"},
        {200, 300, "Left"},
        {400, 450, "Down"},
        {400, 150, "Up"},
        {600, 450, "Right-Down"},
        {200, 150, "Left-Up"}
    };
    
    std::cout << "Expected behavior in top view:\n";
    std::cout << "- Mouse right -> X increases (world right)\n";
    std::cout << "- Mouse left  -> X decreases (world left)\n";
    std::cout << "- Mouse down  -> Z increases (world forward)\n";
    std::cout << "- Mouse up    -> Z decreases (world back)\n";
    std::cout << "- Y should remain 0 (ground plane)\n\n";
    
    for (const auto& test : testCases) {
        // Convert mouse to NDC
        float ndcX = (2.0f * test.mouseX) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * test.mouseY) / screenHeight;
        
        // Create ray from camera through mouse position
        // For ray casting to ground plane, we need two points
        glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        
        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;
        
        // Ray from near to far
        glm::vec3 rayOrigin = glm::vec3(nearPoint);
        glm::vec3 rayDir = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));
        
        // Intersect with ground plane (Y=0)
        float t = -rayOrigin.y / rayDir.y;
        glm::vec3 hitPoint = rayOrigin + t * rayDir;
        
        std::cout << test.description << ": ";
        std::cout << "Mouse(" << test.mouseX << "," << test.mouseY << ") -> ";
        std::cout << "World(" << hitPoint.x << "," << hitPoint.y << "," << hitPoint.z << ")\n";
        
        // Verify Y is 0 (ground plane)
        if (std::abs(hitPoint.y) > 0.001f) {
            std::cout << "  WARNING: Y is not 0! Expected ground plane hit.\n";
        }
        
        // Verify movement directions
        if (test.mouseX > 400 && hitPoint.x <= 0) {
            std::cout << "  ERROR: Mouse moved right but X didn't increase!\n";
        }
        if (test.mouseX < 400 && hitPoint.x >= 0) {
            std::cout << "  ERROR: Mouse moved left but X didn't decrease!\n";
        }
        if (test.mouseY > 300 && hitPoint.z <= 0) {
            std::cout << "  ERROR: Mouse moved down but Z didn't increase!\n";
        }
        if (test.mouseY < 300 && hitPoint.z >= 0) {
            std::cout << "  ERROR: Mouse moved up but Z didn't decrease!\n";
        }
    }
    
    std::cout << "\nDetailed coordinate system check:\n";
    std::cout << "=================================\n";
    
    // Check the exact mapping
    float centerX = 400, centerY = 300;
    float deltaPixels = 100;
    
    // Move right
    float rightX = centerX + deltaPixels;
    float ndcRightX = (2.0f * rightX) / screenWidth - 1.0f;
    float ndcCenterX = (2.0f * centerX) / screenWidth - 1.0f;
    
    glm::vec4 centerNear = invVP * glm::vec4(ndcCenterX, 0, -1, 1);
    glm::vec4 rightNear = invVP * glm::vec4(ndcRightX, 0, -1, 1);
    centerNear /= centerNear.w;
    rightNear /= rightNear.w;
    
    // Ray to ground for center
    glm::vec3 centerRayDir = glm::normalize(glm::vec3(0, -1, 0)); // Top view looks straight down
    float centerT = -cameraPos.y / centerRayDir.y;
    glm::vec3 centerHit = cameraPos + centerT * centerRayDir;
    
    // Ray to ground for right
    glm::vec3 rightRayOrigin = glm::vec3(rightNear);
    glm::vec3 rightRayDir = glm::normalize(glm::vec3(0, -1, 0));
    float rightT = -rightRayOrigin.y / rightRayDir.y;
    glm::vec3 rightHit = rightRayOrigin + rightT * rightRayDir;
    
    std::cout << "Mouse movement analysis:\n";
    std::cout << "Mouse moved right by " << deltaPixels << " pixels\n";
    std::cout << "World X changed by: " << (rightHit.x - centerHit.x) << "\n";
    
    // The relationship should be linear in orthographic projection
    float pixelsPerWorldUnit = screenWidth / (halfWidth * 2);
    float expectedWorldDelta = deltaPixels / pixelsPerWorldUnit;
    std::cout << "Expected world delta: " << expectedWorldDelta << "\n";
    
    // Check if mouse movement matches world movement direction
    std::cout << "\nDirection verification:\n";
    std::cout << "- Mouse right (" << deltaPixels << "px) -> World X change: " 
              << (rightHit.x - centerHit.x > 0 ? "POSITIVE (correct)" : "NEGATIVE (ERROR!)") << "\n";
}

int main() {
    testMouseToWorldTopView();
    return 0;
}