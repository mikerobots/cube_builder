#include "foundation/logging/Logger.h"
#include <iostream>

int main() {
    std::cout << "Testing Logger..." << std::endl;
    
    try {
        auto& logger = VoxelEditor::Logging::Logger::getInstance();
        logger.info("Test info message");
        logger.debug("Test debug message");
        logger.warning("Test warning message");
        logger.error("Test error message");
        
        std::cout << "Logger test completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception!" << std::endl;
        return 1;
    }
    
    return 0;
}