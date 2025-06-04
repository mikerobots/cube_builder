#include <iostream>
#include <iomanip>
#include <memory>
#include "core/camera/OrbitCamera.h"
#include "foundation/events/EventDispatcher.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Events;

int main() {
    // Create camera
    auto eventDispatcher = std::make_unique<EventDispatcher>();
    auto camera = std::make_unique<Camera::OrbitCamera>(eventDispatcher.get());
    
    std::cout << "Testing zoom behavior similar to CLI\n";
    std::cout << "==================================\n\n";
    
    // Show initial state
    std::cout << "Initial distance: " << camera->getDistance() << "\n\n";
    
    // Simulate multiple zoom commands
    float factors[] = {1.5f, 1.5f, 1.5f, 0.8f, 0.8f};
    
    for (int i = 0; i < 5; ++i) {
        float factor = factors[i];
        float currentDistance = camera->getDistance();
        float newDistance = currentDistance / factor;
        
        std::cout << "Zoom " << (i + 1) << " (factor=" << factor << "):\n";
        std::cout << "  Current distance: " << std::fixed << std::setprecision(6) << currentDistance << "\n";
        std::cout << "  Expected new: " << currentDistance / factor << "\n";
        
        camera->setDistance(newDistance);
        
        std::cout << "  Actual new: " << camera->getDistance() << "\n";
        std::cout << "  Change: " << currentDistance << " -> " << camera->getDistance() << "\n\n";
    }
    
    // Test with view preset change
    std::cout << "Testing with view preset change:\n";
    std::cout << "================================\n";
    
    camera->setViewPreset(VoxelEditor::Camera::ViewPreset::FRONT);
    std::cout << "After FRONT view preset: " << camera->getDistance() << "\n";
    
    float currentDistance = camera->getDistance();
    camera->setDistance(currentDistance / 1.5f);
    std::cout << "After zoom 1.5: " << camera->getDistance() << "\n\n";
    
    // Test zoom limits
    std::cout << "Testing zoom limits:\n";
    std::cout << "===================\n";
    
    camera->setDistance(1.0f);
    std::cout << "Set to 1.0: " << camera->getDistance() << "\n";
    
    camera->setDistance(0.3f); // Below min
    std::cout << "Set to 0.3 (below min): " << camera->getDistance() << "\n";
    
    camera->setDistance(150.0f); // Above max
    std::cout << "Set to 150 (above max): " << camera->getDistance() << "\n";
    
    return 0;
}