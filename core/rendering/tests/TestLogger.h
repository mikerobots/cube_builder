#pragma once
#include <string>
#include <iostream>

namespace VoxelEditor {
namespace Rendering {
namespace Test {

// A simple test logger that doesn't use singleton pattern
class TestLogger {
public:
    static void info(const std::string& message) {
        std::cout << "[INFO] " << message << std::endl;
    }
    
    static void debug(const std::string& message) {
        std::cout << "[DEBUG] " << message << std::endl;
    }
    
    static void error(const std::string& message) {
        std::cerr << "[ERROR] " << message << std::endl;
    }
    
    static void warning(const std::string& message) {
        std::cout << "[WARNING] " << message << std::endl;
    }
};

}
}
}