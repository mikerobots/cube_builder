#include "foundation/voxel_math/include/voxel_math/VoxelMath.h"
#include <iostream>

int main() {
    using namespace VoxelEditor::Math;
    
    std::cout << "=== Voxel Math Library with Eigen Integration ===" << std::endl;
    std::cout << "Library Version: " << VoxelMathInfo::getVersion() << std::endl;
    std::cout << "Description: " << VoxelMathInfo::getDescription() << std::endl;
    std::cout << "SIMD Available: " << (VoxelMathInfo::isSIMDEnabled() ? "Yes" : "No") << std::endl;
    std::cout << "SIMD Info: " << VoxelMathInfo::getSIMDInfo() << std::endl;
    std::cout << "Optimal Batch Size: " << VoxelMathInfo::getOptimalBatchSize() << std::endl;
    
    // Test a simple batch operation
    std::cout << "\n=== Testing Batch Operations ===" << std::endl;
    
    const size_t testCount = 100;
    std::vector<WorldCoordinates> worldCoords;
    std::vector<IncrementCoordinates> incrementCoords(testCount);
    
    // Generate test data
    for (size_t i = 0; i < testCount; ++i) {
        float x = static_cast<float>(i) * 0.01f;  // 0, 0.01, 0.02, etc.
        float y = static_cast<float>(i) * 0.02f;  // 0, 0.02, 0.04, etc.
        float z = static_cast<float>(i) * 0.03f;  // 0, 0.03, 0.06, etc.
        worldCoords.emplace_back(Vector3f(x, y, z));
    }
    
    // Test batch conversion
    VoxelMathSIMD::worldToIncrementBatch(worldCoords.data(), incrementCoords.data(), testCount);
    
    std::cout << "Successfully converted " << testCount << " coordinates" << std::endl;
    std::cout << "First few results:" << std::endl;
    for (size_t i = 0; i < std::min(testCount, size_t(5)); ++i) {
        const Vector3f& world = worldCoords[i].value();
        const Vector3i& inc = incrementCoords[i].value();
        std::cout << "  World(" << world.x << ", " << world.y << ", " << world.z 
                  << ") -> Increment(" << inc.x << ", " << inc.y << ", " << inc.z << ")" << std::endl;
    }
    
    return 0;
}