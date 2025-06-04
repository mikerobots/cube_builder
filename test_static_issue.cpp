#include <iostream>
#include <memory>

// Simulate the issue with static variables in methods
class MockRenderer {
public:
    int createShader() {
        static int nextId = 1;
        std::cout << "createShader called, returning " << nextId << std::endl;
        return nextId++;
    }
    
    int createProgram() {
        static int nextId = 100;
        std::cout << "createProgram called, returning " << nextId << std::endl;
        return nextId++;
    }
};

int main() {
    std::cout << "Testing static variables in mock..." << std::endl;
    
    auto renderer = std::make_unique<MockRenderer>();
    
    std::cout << "First shader: " << renderer->createShader() << std::endl;
    std::cout << "Second shader: " << renderer->createShader() << std::endl;
    std::cout << "First program: " << renderer->createProgram() << std::endl;
    
    // Create another instance
    auto renderer2 = std::make_unique<MockRenderer>();
    std::cout << "New renderer shader: " << renderer2->createShader() << std::endl;
    
    return 0;
}