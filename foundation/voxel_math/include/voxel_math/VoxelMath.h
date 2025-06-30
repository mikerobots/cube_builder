#pragma once

/**
 * Comprehensive voxel math library
 * 
 * This header includes all voxel math classes for convenience.
 * Include this single header to access the complete voxel math API.
 */

#include "VoxelBounds.h"
#include "VoxelGrid.h"
#include "FaceOperations.h"
#include "WorkspaceValidation.h"
#include "VoxelCollision.h"
#include "VoxelRaycast.h"
#include "VoxelMathSIMD.h"

namespace VoxelEditor {
namespace Math {

/**
 * Voxel Math Library Information
 */
struct VoxelMathInfo {
    static const char* getVersion() { return "1.0.0"; }
    static const char* getDescription() { return "Comprehensive voxel mathematics library"; }
    
    static bool isSIMDEnabled() { return VoxelMathSIMD::isSIMDAvailable(); }
    static const char* getSIMDInfo() { return VoxelMathSIMD::getSIMDInstructionSet(); }
    static size_t getOptimalBatchSize() { return VoxelMathSIMD::getOptimalBatchSize(); }
};

} // namespace Math
} // namespace VoxelEditor