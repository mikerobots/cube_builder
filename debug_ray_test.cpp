#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Simulate the camera setup and ray generation from the test
int main() {
    std::cout << "Debugging ray visualization test setup\n";
    
    // Default window size from test
    int windowWidth = 1280;
    int windowHeight = 720;
    
    // Center position from test
    int centerX = windowWidth / 2;  // 640
    int centerY = windowHeight / 2; // 360
    
    std::cout << "Window size: " << windowWidth << "x" << windowHeight << "\n";
    std::cout << "Center position: (" << centerX << ", " << centerY << ")\n";
    
    // Convert to NDC (same as MouseInteraction::getMouseRay)
    float ndcX = (2.0f * centerX) / windowWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * centerY) / windowHeight;
    
    std::cout << "NDC coordinates: (" << ndcX << ", " << ndcY << ")\n";
    
    // Check if this produces expected values
    std::cout << "Expected NDC for center: (0, 0)\n";
    std::cout << "Actual NDC: (" << ndcX << ", " << ndcY << ")\n";
    
    // Ray length from FeedbackRenderer is 10.0f
    float rayLength = 10.0f;
    std::cout << "Ray length: " << rayLength << "\n";
    
    // Check various positions from the test
    std::vector<glm::vec2> testPositions = {
        {centerX - 50, centerY},      // Slightly left of center
        {centerX, centerY},           // Center
        {centerX, centerY - 50},      // Slightly above center
        {centerX, centerY + 50},      // Slightly below center
    };
    
    std::cout << "\nTest positions and their NDC coordinates:\n";
    for (size_t i = 0; i < testPositions.size(); ++i) {
        float x = testPositions[i].x;
        float y = testPositions[i].y;
        float ndc_x = (2.0f * x) / windowWidth - 1.0f;
        float ndc_y = 1.0f - (2.0f * y) / windowHeight;
        
        std::cout << "Position " << i << ": (" << x << ", " << y << ") -> NDC(" << ndc_x << ", " << ndc_y << ")\n";
    }
    
    return 0;
}