#pragma once

#include "DualContouring.h"
#include <arm_neon.h>
#include <array>

namespace VoxelEditor {
namespace SurfaceGen {

// ARM NEON optimized implementation for Apple Silicon
class DualContouringNEON : public DualContouring {
public:
    DualContouringNEON();
    virtual ~DualContouringNEON() = default;
    
protected:
    // Override the edge intersection extraction for NEON optimization
    virtual void extractEdgeIntersections(const VoxelData::VoxelGrid& grid) override;
    
private:
    // Process 4 edges at once using NEON
    void processEdgesNEON(const Math::IncrementCoordinates& cellPos,
                         const VoxelData::VoxelGrid& grid,
                         CellData& cell);
    
    // Batch sample 8 vertices at once
    void batchSampleVertices(const Math::IncrementCoordinates vertices[8],
                            const VoxelData::VoxelGrid& grid,
                            float results[8]);
    
    // Fast edge intersection using NEON
    bool findEdgeIntersectionNEON(const Math::IncrementCoordinates& v0,
                                  const Math::IncrementCoordinates& v1,
                                  float val0, float val1,
                                  HermiteData& hermite);
};

} // namespace SurfaceGen
} // namespace VoxelEditor