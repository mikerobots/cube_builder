#include "core/voxel_data/VoxelDataManager.h"
#include "foundation/logging/Logger.h"

using namespace VoxelEditor;

int main() {
    // Enable debug logging
    Logging::Logger::getInstance().setLevel(Logging::LogLevel::Debug);
    
    try {
        VoxelData::VoxelDataManager manager;
        
        // Simple test that should trigger the infinite loop
        std::cout << "Testing setVoxel..." << std::endl;
        Math::IncrementCoordinates pos(0, 0, 0);
        bool result = manager.setVoxel(pos, VoxelData::VoxelResolution::Size_1cm, true);
        std::cout << "Result: " << result << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    return 0;
}